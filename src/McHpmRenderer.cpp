#include <engine/graphics/renderer/McHpmRenderer.hpp>
#include <engine/graphics/vulkan/CommandRecorder.hpp>
#include <glm/gtc/random.hpp>
#include <tinyexr.h>

namespace en
{
	VkDescriptorSetLayout McHpmRenderer::s_DescSetLayout = VK_NULL_HANDLE;
	VkDescriptorPool McHpmRenderer::s_DescPool = VK_NULL_HANDLE;

	void McHpmRenderer::Init(VkDevice device)
	{
		// Create desc set layout
		VkDescriptorSetLayoutBinding outputImageBinding;
		outputImageBinding.binding = 0;
		outputImageBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		outputImageBinding.descriptorCount = 1;
		outputImageBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
		outputImageBinding.pImmutableSamplers = nullptr;

		VkDescriptorSetLayoutBinding infoImageBinding;
		infoImageBinding.binding = 1;
		infoImageBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		infoImageBinding.descriptorCount = 1;
		infoImageBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
		infoImageBinding.pImmutableSamplers = nullptr;

		VkDescriptorSetLayoutBinding uniformBufferBinding;
		uniformBufferBinding.binding = 2;
		uniformBufferBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		uniformBufferBinding.descriptorCount = 1;
		uniformBufferBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
		uniformBufferBinding.pImmutableSamplers = nullptr;

		std::vector<VkDescriptorSetLayoutBinding> bindings = {
			outputImageBinding,
			infoImageBinding,
			uniformBufferBinding
		};

		VkDescriptorSetLayoutCreateInfo layoutCI;
		layoutCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutCI.pNext = nullptr;
		layoutCI.flags = 0;
		layoutCI.bindingCount = bindings.size();
		layoutCI.pBindings = bindings.data();

		VkResult result = vkCreateDescriptorSetLayout(device, &layoutCI, nullptr, &s_DescSetLayout);
		ASSERT_VULKAN(result);

		// Create desc pool
		VkDescriptorPoolSize storageImagePS;
		storageImagePS.type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		storageImagePS.descriptorCount = 2;

		VkDescriptorPoolSize uniformBufferPS;
		uniformBufferPS.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		uniformBufferPS.descriptorCount = 1;

		std::vector<VkDescriptorPoolSize> poolSizes = { storageImagePS, uniformBufferPS };

		VkDescriptorPoolCreateInfo poolCI;
		poolCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolCI.pNext = nullptr;
		poolCI.flags = 0;
		poolCI.maxSets = 1;
		poolCI.poolSizeCount = poolSizes.size();
		poolCI.pPoolSizes = poolSizes.data();

		result = vkCreateDescriptorPool(device, &poolCI, nullptr, &s_DescPool);
		ASSERT_VULKAN(result);
	}

	void McHpmRenderer::Shutdown(VkDevice device)
	{
		vkDestroyDescriptorPool(device, s_DescPool, nullptr);
		vkDestroyDescriptorSetLayout(device, s_DescSetLayout, nullptr);
	}

	McHpmRenderer::McHpmRenderer(uint32_t width, uint32_t height, uint32_t spp, const Camera& camera, const HpmScene& scene) :
		m_RenderWidth(width),
		m_RenderHeight(height),
		m_Spp(spp),
		m_Camera(camera),
		m_HpmScene(scene),
		m_UniformBuffer(
			sizeof(UniformData),
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			{}),
		m_RenderShader("mc/render.comp", false),
		m_CommandPool(VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, VulkanAPI::GetGraphicsQFI())
	{
		// Init components
		VkDevice device = VulkanAPI::GetDevice();

		m_CommandPool.AllocateBuffers(2, VK_COMMAND_BUFFER_LEVEL_PRIMARY);
		m_RenderCommandBuffer = m_CommandPool.GetBuffer(0);
		m_RandomTasksCmdBuf = m_CommandPool.GetBuffer(1);

		CreatePipelineLayout(device);

		InitSpecializationConstants();

		CreateRenderPipeline(device);

		CreateOutputImage(device);
		CreateInfoImage(device);

		AllocateAndUpdateDescriptorSet(device);

		CreateQueryPool(device);

		RecordRenderCommandBuffer();
	}

	void McHpmRenderer::Render(VkQueue queue)
	{
		// Generate random
		m_UniformData.random = glm::linearRand(glm::vec4(0.0f), glm::vec4(1.0f));
		m_UniformBuffer.SetData(sizeof(UniformData), &m_UniformData, 0, 0);

		// Render
		VkSubmitInfo submitInfo;
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.pNext = nullptr;
		submitInfo.waitSemaphoreCount = 0;
		submitInfo.pWaitSemaphores = nullptr;
		submitInfo.pWaitDstStageMask = nullptr;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &m_RenderCommandBuffer;
		submitInfo.signalSemaphoreCount = 0;
		submitInfo.pSignalSemaphores = nullptr;

		ASSERT_VULKAN(vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));
	}

	void McHpmRenderer::Destroy()
	{
		VkDevice device = VulkanAPI::GetDevice();

		m_CommandPool.Destroy();

		m_UniformBuffer.Destroy();

		vkDestroyQueryPool(device, m_QueryPool, nullptr);

		vkDestroyImageView(device, m_InfoImageView, nullptr);
		vkFreeMemory(device, m_InfoImageMemory, nullptr);
		vkDestroyImage(device, m_InfoImage, nullptr);

		vkDestroyImageView(device, m_OutputImageView, nullptr);
		vkFreeMemory(device, m_OutputImageMemory, nullptr);
		vkDestroyImage(device, m_OutputImage, nullptr);

		vkDestroyPipeline(device, m_RenderPipeline, nullptr);
		m_RenderShader.Destroy();

		vkDestroyPipelineLayout(device, m_PipelineLayout, nullptr);
	}

	void McHpmRenderer::ExportImageToFile(VkQueue queue, const std::string& filePath) const
	{
		const size_t floatCount = m_RenderWidth * m_RenderHeight * 4;
		const size_t bufferSize = floatCount * sizeof(float);

		vk::Buffer vkBuffer(
			bufferSize,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			{});

		VkCommandBufferBeginInfo cmdBufBI;
		cmdBufBI.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		cmdBufBI.pNext = nullptr;
		cmdBufBI.flags = 0;
		cmdBufBI.pInheritanceInfo = nullptr;
		ASSERT_VULKAN(vkBeginCommandBuffer(m_RandomTasksCmdBuf, &cmdBufBI));

		VkBufferImageCopy region;
		region.bufferOffset = 0;
		region.bufferRowLength = m_RenderWidth;
		region.bufferImageHeight = m_RenderHeight;
		region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		region.imageSubresource.mipLevel = 0;
		region.imageSubresource.baseArrayLayer = 0;
		region.imageSubresource.layerCount = 1;
		region.imageOffset = { 0, 0, 0 };
		region.imageExtent = { m_RenderWidth, m_RenderHeight, 1 };

		vkCmdCopyImageToBuffer(m_RandomTasksCmdBuf, m_OutputImage, VK_IMAGE_LAYOUT_GENERAL, vkBuffer.GetVulkanHandle(), 1, &region);

		ASSERT_VULKAN(vkEndCommandBuffer(m_RandomTasksCmdBuf));

		VkSubmitInfo submitInfo;
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.pNext = nullptr;
		submitInfo.waitSemaphoreCount = 0;
		submitInfo.pWaitSemaphores = nullptr;
		submitInfo.pWaitDstStageMask = nullptr;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &m_RandomTasksCmdBuf;
		submitInfo.signalSemaphoreCount = 0;
		submitInfo.pSignalSemaphores = nullptr;

		ASSERT_VULKAN(vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));
		ASSERT_VULKAN(vkQueueWaitIdle(queue));

		std::vector<float> buffer(floatCount);
		vkBuffer.GetData(bufferSize, buffer.data(), 0, 0);
		vkBuffer.Destroy();

		if (TINYEXR_SUCCESS != SaveEXR(buffer.data(), m_RenderWidth, m_RenderHeight, 4, 0, filePath.c_str(), nullptr))
		{
			en::Log::Error("TINYEXR Error", true);
		}
	}

	void McHpmRenderer::EvaluateTimestampQueries()
	{
		VkDevice device = VulkanAPI::GetDevice();
		std::vector<uint64_t> queryResults(c_QueryCount);
		ASSERT_VULKAN(vkGetQueryPoolResults(
			device,
			m_QueryPool,
			0,
			c_QueryCount,
			sizeof(uint64_t) * c_QueryCount,
			queryResults.data(),
			sizeof(uint64_t),
			VK_QUERY_RESULT_64_BIT));
		vkResetQueryPool(device, m_QueryPool, 0, c_QueryCount);

		const size_t timePeriodCount = c_QueryCount - 1;
		const float timestampPeriodInMS = VulkanAPI::GetTimestampPeriod() * 1e-6f;
		std::vector<float> timePeriods(timePeriodCount);
		for (size_t i = 0; i < timePeriodCount; i++)
		{
			timePeriods[i] = timestampPeriodInMS * static_cast<float>(queryResults[i + 1] - queryResults[i]);
		}
	}

	VkImage McHpmRenderer::GetImage() const
	{
		return m_OutputImage;
	}

	VkImageView McHpmRenderer::GetImageView() const
	{
		return m_OutputImageView;
	}

	void McHpmRenderer::CreatePipelineLayout(VkDevice device)
	{
		std::vector<VkDescriptorSetLayout> layouts = {
			Camera::GetDescriptorSetLayout(),
			VolumeData::GetDescriptorSetLayout(),
			DirLight::GetDescriptorSetLayout(),
			PointLight::GetDescriptorSetLayout(),
			HdrEnvMap::GetDescriptorSetLayout(),
			s_DescSetLayout };

		VkPipelineLayoutCreateInfo layoutCreateInfo;
		layoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		layoutCreateInfo.pNext = nullptr;
		layoutCreateInfo.flags = 0;
		layoutCreateInfo.setLayoutCount = layouts.size();
		layoutCreateInfo.pSetLayouts = layouts.data();
		layoutCreateInfo.pushConstantRangeCount = 0;
		layoutCreateInfo.pPushConstantRanges = nullptr;

		VkResult result = vkCreatePipelineLayout(device, &layoutCreateInfo, nullptr, &m_PipelineLayout);
		ASSERT_VULKAN(result);
	}

	void McHpmRenderer::InitSpecializationConstants()
	{
		// Fill struct
		m_SpecData.renderWidth = m_RenderWidth;
		m_SpecData.renderHeight = m_RenderHeight;
		m_SpecData.spp = m_Spp;

		m_SpecData.volumeDensityFactor = m_HpmScene.GetVolumeData()->GetDensityFactor();
		m_SpecData.volumeG = m_HpmScene.GetVolumeData()->GetG();

		m_SpecData.hdrEnvMapStrength = m_HpmScene.GetHdrEnvMap()->GetStrength();

		// Init map entries
		VkSpecializationMapEntry renderWidthEntry;
		renderWidthEntry.constantID = 0;
		renderWidthEntry.offset = offsetof(SpecializationData, SpecializationData::renderWidth);
		renderWidthEntry.size = sizeof(uint32_t);

		VkSpecializationMapEntry renderHeightEntry;
		renderHeightEntry.constantID = 1;
		renderHeightEntry.offset = offsetof(SpecializationData, SpecializationData::renderHeight);
		renderHeightEntry.size = sizeof(uint32_t);

		VkSpecializationMapEntry sppEntry;
		sppEntry.constantID = 2;
		sppEntry.offset = offsetof(SpecializationData, SpecializationData::spp);
		sppEntry.size = sizeof(uint32_t);

		VkSpecializationMapEntry volumeDensityFactorEntry;
		volumeDensityFactorEntry.constantID = 3;
		volumeDensityFactorEntry.offset = offsetof(SpecializationData, SpecializationData::volumeDensityFactor);
		volumeDensityFactorEntry.size = sizeof(float);

		VkSpecializationMapEntry volumeGEntry;
		volumeGEntry.constantID = 4;
		volumeGEntry.offset = offsetof(SpecializationData, SpecializationData::volumeG);
		volumeGEntry.size = sizeof(float);

		VkSpecializationMapEntry hdrEnvMapStrengthEntry;
		hdrEnvMapStrengthEntry.constantID = 5;
		hdrEnvMapStrengthEntry.offset = offsetof(SpecializationData, SpecializationData::hdrEnvMapStrength);
		hdrEnvMapStrengthEntry.size = sizeof(float);

		m_SpecMapEntries = {
			renderWidthEntry,
			renderHeightEntry,
			sppEntry,
			volumeDensityFactorEntry,
			volumeGEntry,
			hdrEnvMapStrengthEntry
		};

		m_SpecInfo.mapEntryCount = m_SpecMapEntries.size();
		m_SpecInfo.pMapEntries = m_SpecMapEntries.data();
		m_SpecInfo.dataSize = sizeof(SpecializationData);
		m_SpecInfo.pData = &m_SpecData;
	}

	void McHpmRenderer::CreateRenderPipeline(VkDevice device)
	{
		VkPipelineShaderStageCreateInfo shaderStage;
		shaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shaderStage.pNext = nullptr;
		shaderStage.flags = 0;
		shaderStage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
		shaderStage.module = m_RenderShader.GetVulkanModule();
		shaderStage.pName = "main";
		shaderStage.pSpecializationInfo = &m_SpecInfo;

		VkComputePipelineCreateInfo pipelineCI;
		pipelineCI.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
		pipelineCI.pNext = nullptr;
		pipelineCI.flags = 0;
		pipelineCI.stage = shaderStage;
		pipelineCI.layout = m_PipelineLayout;
		pipelineCI.basePipelineHandle = VK_NULL_HANDLE;
		pipelineCI.basePipelineIndex = 0;

		VkResult result = vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &pipelineCI, nullptr, &m_RenderPipeline);
		ASSERT_VULKAN(result);
	}

	void McHpmRenderer::CreateOutputImage(VkDevice device)
	{
		VkFormat format = VK_FORMAT_R32G32B32A32_SFLOAT;

		// Create Image
		VkImageCreateInfo imageCI;
		imageCI.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageCI.pNext = nullptr;
		imageCI.flags = 0;
		imageCI.imageType = VK_IMAGE_TYPE_2D;
		imageCI.format = format;
		imageCI.extent = { m_RenderWidth, m_RenderHeight, 1 };
		imageCI.mipLevels = 1;
		imageCI.arrayLayers = 1;
		imageCI.samples = VK_SAMPLE_COUNT_1_BIT;
		imageCI.tiling = VK_IMAGE_TILING_OPTIMAL;
		imageCI.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
		imageCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		imageCI.queueFamilyIndexCount = 0;
		imageCI.pQueueFamilyIndices = nullptr;
		imageCI.initialLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;

		VkResult result = vkCreateImage(device, &imageCI, nullptr, &m_OutputImage);
		ASSERT_VULKAN(result);

		// Image Memory
		VkMemoryRequirements memoryRequirements;
		vkGetImageMemoryRequirements(device, m_OutputImage, &memoryRequirements);

		VkMemoryAllocateInfo allocateInfo;
		allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocateInfo.pNext = nullptr;
		allocateInfo.allocationSize = memoryRequirements.size;
		allocateInfo.memoryTypeIndex = VulkanAPI::FindMemoryType(
			memoryRequirements.memoryTypeBits,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		result = vkAllocateMemory(device, &allocateInfo, nullptr, &m_OutputImageMemory);
		ASSERT_VULKAN(result);

		result = vkBindImageMemory(device, m_OutputImage, m_OutputImageMemory, 0);
		ASSERT_VULKAN(result);

		// Create image view
		VkImageViewCreateInfo imageViewCI;
		imageViewCI.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		imageViewCI.pNext = nullptr;
		imageViewCI.flags = 0;
		imageViewCI.image = m_OutputImage;
		imageViewCI.viewType = VK_IMAGE_VIEW_TYPE_2D;
		imageViewCI.format = format;
		imageViewCI.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		imageViewCI.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		imageViewCI.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		imageViewCI.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		imageViewCI.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageViewCI.subresourceRange.baseMipLevel = 0;
		imageViewCI.subresourceRange.levelCount = 1;
		imageViewCI.subresourceRange.baseArrayLayer = 0;
		imageViewCI.subresourceRange.layerCount = 1;

		result = vkCreateImageView(device, &imageViewCI, nullptr, &m_OutputImageView);
		ASSERT_VULKAN(result);

		// Change image layout
		VkCommandBufferBeginInfo beginInfo;
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.pNext = nullptr;
		beginInfo.flags = 0;
		beginInfo.pInheritanceInfo = nullptr;

		result = vkBeginCommandBuffer(m_RandomTasksCmdBuf, &beginInfo);
		ASSERT_VULKAN(result);

		vk::CommandRecorder::ImageLayoutTransfer(
			m_RandomTasksCmdBuf,
			m_OutputImage,
			VK_IMAGE_LAYOUT_PREINITIALIZED,
			VK_IMAGE_LAYOUT_GENERAL,
			VK_ACCESS_NONE,
			VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT,
			VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
			VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);

		result = vkEndCommandBuffer(m_RandomTasksCmdBuf);
		ASSERT_VULKAN(result);

		VkSubmitInfo submitInfo;
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.pNext = nullptr;
		submitInfo.waitSemaphoreCount = 0;
		submitInfo.pWaitSemaphores = nullptr;
		submitInfo.pWaitDstStageMask = nullptr;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &m_RandomTasksCmdBuf;
		submitInfo.signalSemaphoreCount = 0;
		submitInfo.pSignalSemaphores = nullptr;

		VkQueue queue = VulkanAPI::GetGraphicsQueue();
		result = vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
		ASSERT_VULKAN(result);
		result = vkQueueWaitIdle(queue);
		ASSERT_VULKAN(result);
	}

	void McHpmRenderer::CreateInfoImage(VkDevice device)
	{
		VkFormat format = VK_FORMAT_R32G32B32A32_SFLOAT;

		// Create Image
		VkImageCreateInfo imageCI;
		imageCI.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageCI.pNext = nullptr;
		imageCI.flags = 0;
		imageCI.imageType = VK_IMAGE_TYPE_2D;
		imageCI.format = format;
		imageCI.extent = { m_RenderWidth, m_RenderHeight, 1 };
		imageCI.mipLevels = 1;
		imageCI.arrayLayers = 1;
		imageCI.samples = VK_SAMPLE_COUNT_1_BIT;
		imageCI.tiling = VK_IMAGE_TILING_OPTIMAL;
		imageCI.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
		imageCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		imageCI.queueFamilyIndexCount = 0;
		imageCI.pQueueFamilyIndices = nullptr;
		imageCI.initialLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;

		VkResult result = vkCreateImage(device, &imageCI, nullptr, &m_InfoImage);
		ASSERT_VULKAN(result);

		// Image Memory
		VkMemoryRequirements memoryRequirements;
		vkGetImageMemoryRequirements(device, m_InfoImage, &memoryRequirements);

		VkMemoryAllocateInfo allocateInfo;
		allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocateInfo.pNext = nullptr;
		allocateInfo.allocationSize = memoryRequirements.size;
		allocateInfo.memoryTypeIndex = VulkanAPI::FindMemoryType(
			memoryRequirements.memoryTypeBits,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		result = vkAllocateMemory(device, &allocateInfo, nullptr, &m_InfoImageMemory);
		ASSERT_VULKAN(result);

		result = vkBindImageMemory(device, m_InfoImage, m_InfoImageMemory, 0);
		ASSERT_VULKAN(result);

		// Create image view
		VkImageViewCreateInfo imageViewCI;
		imageViewCI.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		imageViewCI.pNext = nullptr;
		imageViewCI.flags = 0;
		imageViewCI.image = m_InfoImage;
		imageViewCI.viewType = VK_IMAGE_VIEW_TYPE_2D;
		imageViewCI.format = format;
		imageViewCI.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		imageViewCI.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		imageViewCI.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		imageViewCI.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		imageViewCI.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageViewCI.subresourceRange.baseMipLevel = 0;
		imageViewCI.subresourceRange.levelCount = 1;
		imageViewCI.subresourceRange.baseArrayLayer = 0;
		imageViewCI.subresourceRange.layerCount = 1;

		result = vkCreateImageView(device, &imageViewCI, nullptr, &m_InfoImageView);
		ASSERT_VULKAN(result);

		// Change image layout
		VkCommandBufferBeginInfo beginInfo;
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.pNext = nullptr;
		beginInfo.flags = 0;
		beginInfo.pInheritanceInfo = nullptr;

		result = vkBeginCommandBuffer(m_RandomTasksCmdBuf, &beginInfo);
		ASSERT_VULKAN(result);

		vk::CommandRecorder::ImageLayoutTransfer(
			m_RandomTasksCmdBuf,
			m_InfoImage,
			VK_IMAGE_LAYOUT_PREINITIALIZED,
			VK_IMAGE_LAYOUT_GENERAL,
			VK_ACCESS_NONE,
			VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT,
			VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
			VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);

		result = vkEndCommandBuffer(m_RandomTasksCmdBuf);
		ASSERT_VULKAN(result);

		VkSubmitInfo submitInfo;
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.pNext = nullptr;
		submitInfo.waitSemaphoreCount = 0;
		submitInfo.pWaitSemaphores = nullptr;
		submitInfo.pWaitDstStageMask = nullptr;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &m_RandomTasksCmdBuf;
		submitInfo.signalSemaphoreCount = 0;
		submitInfo.pSignalSemaphores = nullptr;

		VkQueue queue = VulkanAPI::GetGraphicsQueue();
		result = vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
		ASSERT_VULKAN(result);
		result = vkQueueWaitIdle(queue);
		ASSERT_VULKAN(result);
	}

	void McHpmRenderer::AllocateAndUpdateDescriptorSet(VkDevice device)
	{
		// Allocate
		VkDescriptorSetAllocateInfo descSetAI;
		descSetAI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		descSetAI.pNext = nullptr;
		descSetAI.descriptorPool = s_DescPool;
		descSetAI.descriptorSetCount = 1;
		descSetAI.pSetLayouts = &s_DescSetLayout;

		VkResult result = vkAllocateDescriptorSets(device, &descSetAI, &m_DescSet);
		ASSERT_VULKAN(result);

		// Write
		// Storage image writes
		VkDescriptorImageInfo outputImageInfo;
		outputImageInfo.sampler = VK_NULL_HANDLE;
		outputImageInfo.imageView = m_OutputImageView;
		outputImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

		VkWriteDescriptorSet outputImageWrite;
		outputImageWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		outputImageWrite.pNext = nullptr;
		outputImageWrite.dstSet = m_DescSet;
		outputImageWrite.dstBinding = 0;
		outputImageWrite.dstArrayElement = 0;
		outputImageWrite.descriptorCount = 1;
		outputImageWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		outputImageWrite.pImageInfo = &outputImageInfo;
		outputImageWrite.pBufferInfo = nullptr;
		outputImageWrite.pTexelBufferView = nullptr;

		VkDescriptorImageInfo infoImageInfo;
		infoImageInfo.sampler = VK_NULL_HANDLE;
		infoImageInfo.imageView = m_InfoImageView;
		infoImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

		VkWriteDescriptorSet infoImageWrite;
		infoImageWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		infoImageWrite.pNext = nullptr;
		infoImageWrite.dstSet = m_DescSet;
		infoImageWrite.dstBinding = 1;
		infoImageWrite.dstArrayElement = 0;
		infoImageWrite.descriptorCount = 1;
		infoImageWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		infoImageWrite.pImageInfo = &infoImageInfo;
		infoImageWrite.pBufferInfo = nullptr;
		infoImageWrite.pTexelBufferView = nullptr;

		// Uniform buffer write
		VkDescriptorBufferInfo uniformBufferInfo;
		uniformBufferInfo.buffer = m_UniformBuffer.GetVulkanHandle();
		uniformBufferInfo.offset = 0;
		uniformBufferInfo.range = sizeof(UniformData);

		VkWriteDescriptorSet uniformBufferWrite;
		uniformBufferWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		uniformBufferWrite.pNext = nullptr;
		uniformBufferWrite.dstSet = m_DescSet;
		uniformBufferWrite.dstBinding = 2;
		uniformBufferWrite.dstArrayElement = 0;
		uniformBufferWrite.descriptorCount = 1;
		uniformBufferWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		uniformBufferWrite.pImageInfo = nullptr;
		uniformBufferWrite.pBufferInfo = &uniformBufferInfo;
		uniformBufferWrite.pTexelBufferView = nullptr;

		// Write writes
		std::vector<VkWriteDescriptorSet> writes = {
			outputImageWrite,
			infoImageWrite,
			uniformBufferWrite
		};

		vkUpdateDescriptorSets(device, writes.size(), writes.data(), 0, nullptr);
	}

	void McHpmRenderer::CreateQueryPool(VkDevice device)
	{
		VkQueryPoolCreateInfo queryPoolCI;
		queryPoolCI.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
		queryPoolCI.pNext = nullptr;
		queryPoolCI.flags = 0;
		queryPoolCI.queryType = VK_QUERY_TYPE_TIMESTAMP;
		queryPoolCI.queryCount = c_QueryCount;
		queryPoolCI.pipelineStatistics = 0;

		ASSERT_VULKAN(vkCreateQueryPool(device, &queryPoolCI, nullptr, &m_QueryPool));
	}

	void McHpmRenderer::RecordRenderCommandBuffer()
	{
		m_QueryIndex = 0;

		// Begin command buffer
		VkCommandBufferBeginInfo beginInfo;
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.pNext = nullptr;
		beginInfo.flags = 0;
		beginInfo.pInheritanceInfo = nullptr;

		VkResult result = vkBeginCommandBuffer(m_RenderCommandBuffer, &beginInfo);
		ASSERT_VULKAN(result);

		// Reset query pool
		vkCmdResetQueryPool(m_RenderCommandBuffer, m_QueryPool, 0, c_QueryCount);

		// Collect descriptor sets
		std::vector<VkDescriptorSet> descSets = { m_Camera.GetDescriptorSet() };
		const std::vector<VkDescriptorSet>& hpmSceneDescSets = m_HpmScene.GetDescriptorSets();
		descSets.insert(descSets.end(), hpmSceneDescSets.begin(), hpmSceneDescSets.end());
		descSets.push_back(m_DescSet);

		// Bind descriptor sets
		vkCmdBindDescriptorSets(
			m_RenderCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_PipelineLayout,
			0, descSets.size(), descSets.data(),
			0, nullptr);

		// Timestamp
		vkCmdWriteTimestamp(m_RenderCommandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, m_QueryPool, m_QueryIndex++);

		// Render pipeline
		vkCmdBindPipeline(m_RenderCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_RenderPipeline);
		vkCmdDispatch(m_RenderCommandBuffer, m_RenderWidth / 32, m_RenderHeight, 1);

		// Timestamp
		vkCmdWriteTimestamp(m_RenderCommandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, m_QueryPool, m_QueryIndex++);

		// End command buffer
		result = vkEndCommandBuffer(m_RenderCommandBuffer);
		ASSERT_VULKAN(result);
	}
}
