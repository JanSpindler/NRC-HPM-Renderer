#include <engine/graphics/renderer/RestirHpmRenderer.hpp>
#include <engine/graphics/vulkan/CommandRecorder.hpp>

namespace en
{
	VkDescriptorSetLayout RestirHpmRenderer::m_DescSetLayout;
	VkDescriptorPool RestirHpmRenderer::m_DescPool;

	void RestirHpmRenderer::Init(VkDevice device)
	{
		// Create desc set layout
		VkDescriptorSetLayoutBinding outputImageBinding;
		outputImageBinding.binding = 0;
		outputImageBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		outputImageBinding.descriptorCount = 1;
		outputImageBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
		outputImageBinding.pImmutableSamplers = nullptr;

		VkDescriptorSetLayoutBinding pixelInfoImageBinding;
		pixelInfoImageBinding.binding = 1;
		pixelInfoImageBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		pixelInfoImageBinding.descriptorCount = 1;
		pixelInfoImageBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
		pixelInfoImageBinding.pImmutableSamplers = nullptr;

		std::vector<VkDescriptorSetLayoutBinding> bindings = { 
			outputImageBinding,
			pixelInfoImageBinding };

		VkDescriptorSetLayoutCreateInfo layoutCI;
		layoutCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutCI.pNext = nullptr;
		layoutCI.flags = 0;
		layoutCI.bindingCount = bindings.size();
		layoutCI.pBindings = bindings.data();

		VkResult result = vkCreateDescriptorSetLayout(device, &layoutCI, nullptr, &m_DescSetLayout);
		ASSERT_VULKAN(result);

		// Create desc pool
		VkDescriptorPoolSize storageImagePoolSize;
		storageImagePoolSize.type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		storageImagePoolSize.descriptorCount = 2;

		std::vector<VkDescriptorPoolSize> poolSizes = { storageImagePoolSize };

		VkDescriptorPoolCreateInfo poolCI;
		poolCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolCI.pNext = nullptr;
		poolCI.flags = 0;
		poolCI.maxSets = 1;
		poolCI.poolSizeCount = poolSizes.size();
		poolCI.pPoolSizes = poolSizes.data();

		result = vkCreateDescriptorPool(device, &poolCI, nullptr, &m_DescPool);
		ASSERT_VULKAN(result);
	}
	
	void RestirHpmRenderer::Shutdown(VkDevice device)
	{
		vkDestroyDescriptorPool(device, m_DescPool, nullptr);
		vkDestroyDescriptorSetLayout(device, m_DescSetLayout, nullptr);
	}

	RestirHpmRenderer::RestirHpmRenderer(
		uint32_t width,
		uint32_t height,
		const Camera& camera,
		const VolumeData& volumeData,
		const DirLight& dirLight,
		const PointLight& pointLight,
		const HdrEnvMap& hdrEnvMap,
		VolumeReservoir& volumeReservoir)
		:
		m_Width(width),
		m_Height(height),
		m_Camera(camera),
		m_VolumeData(volumeData),
		m_DirLight(dirLight),
		m_PointLight(pointLight),
		m_HdrEnvMap(hdrEnvMap),
		m_VolumeReservoir(volumeReservoir),
		m_CommandPool(VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, VulkanAPI::GetGraphicsQFI()),
		m_RenderShader("restir/render.comp", false),
		m_SpatialReuseShader("restir/spatial_reuse.comp", false),
		m_LocalInitShader("restir/local_init.comp", false)
	{
		m_VolumeReservoir.Init(m_Width * m_Height);

		VkDevice device = VulkanAPI::GetDevice();

		m_CommandPool.AllocateBuffers(1, VK_COMMAND_BUFFER_LEVEL_PRIMARY);
		m_CommandBuffer = m_CommandPool.GetBuffer(0);

		CreatePipelineLayout(device);

		InitSpecializationConstants();

		CreateLocalInitPipeline(device);
		CreateSpatialReusePipeline(device);
		CreateRenderPipeline(device);

		CreateOutputImage(device);
		CreatePixelInfoImage(device);

		AllocateAndUpdateDescriptorSet(device);
	
		RecordCommandBuffer();
	}

	void RestirHpmRenderer::Render(VkQueue queue)
	{
		VkSubmitInfo submitInfo;
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.pNext = nullptr;
		submitInfo.waitSemaphoreCount = 0;
		submitInfo.pWaitSemaphores = nullptr;
		submitInfo.pWaitDstStageMask = nullptr;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &m_CommandBuffer;
		submitInfo.signalSemaphoreCount = 0;
		submitInfo.pSignalSemaphores = nullptr;

		VkResult result = vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
		ASSERT_VULKAN(result);
	}

	void RestirHpmRenderer::Destroy()
	{
		VkDevice device = VulkanAPI::GetDevice();

		m_CommandPool.Destroy();

		vkDestroyImageView(device, m_PixelInfoImageView, nullptr);
		vkFreeMemory(device, m_PixelInfoImageMemory, nullptr);
		vkDestroyImage(device, m_PixelInfoImage, nullptr);

		vkDestroyImageView(device, m_OutputImageView, nullptr);
		vkFreeMemory(device, m_OutputImageMemory, nullptr);
		vkDestroyImage(device, m_OutputImage, nullptr);

		vkDestroyPipeline(device, m_RenderPipeline, nullptr);
		m_RenderShader.Destroy();

		vkDestroyPipeline(device, m_SpatialReusePipeline, nullptr);
		m_SpatialReuseShader.Destroy();

		vkDestroyPipeline(device, m_LocalInitPipeline, nullptr);
		m_LocalInitShader.Destroy();

		vkDestroyPipelineLayout(device, m_PipelineLayout, nullptr);
	}

	VkImage RestirHpmRenderer::GetImage() const
	{
		return m_OutputImage;
	}

	VkImageView RestirHpmRenderer::GetImageView() const
	{
		return m_OutputImageView;
	}

	void RestirHpmRenderer::CreatePipelineLayout(VkDevice device)
	{
		std::vector<VkDescriptorSetLayout> descSetLayouts = {
			Camera::GetDescriptorSetLayout(),
			VolumeData::GetDescriptorSetLayout(),
			DirLight::GetDescriptorSetLayout(),
			VolumeReservoir::GetDescriptorSetLayout(),
			PointLight::GetDescriptorSetLayout(),
			HdrEnvMap::GetDescriptorSetLayout(),
			m_DescSetLayout };

		VkPipelineLayoutCreateInfo layoutCI;
		layoutCI.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		layoutCI.pNext = nullptr;
		layoutCI.flags = 0;
		layoutCI.setLayoutCount = descSetLayouts.size();
		layoutCI.pSetLayouts = descSetLayouts.data();
		layoutCI.pushConstantRangeCount = 0;
		layoutCI.pPushConstantRanges = nullptr;

		VkResult result = vkCreatePipelineLayout(device, &layoutCI, nullptr, &m_PipelineLayout);
		ASSERT_VULKAN(result);
	}

	void RestirHpmRenderer::InitSpecializationConstants()
	{
		// Fill specialization data
		m_SpecData.renderWidth = m_Width;
		m_SpecData.renderHeight = m_Height;
		
		m_SpecData.pathVertexCount = m_VolumeReservoir.GetPathVertexCount();

		m_SpecData.spatialKernelSize = m_VolumeReservoir.GetSpatialKernelSize();

		// Fill map entries
		VkSpecializationMapEntry renderWidthEntry;
		renderWidthEntry.constantID = 0;
		renderWidthEntry.offset = offsetof(SpecializationData, SpecializationData::renderWidth);
		renderWidthEntry.size = sizeof(uint32_t);

		VkSpecializationMapEntry renderHeightEntry;
		renderHeightEntry.constantID = 1;
		renderHeightEntry.offset = offsetof(SpecializationData, SpecializationData::renderHeight);
		renderHeightEntry.size = sizeof(uint32_t);

		VkSpecializationMapEntry pathVertexCountEntry;
		pathVertexCountEntry.constantID = 2;
		pathVertexCountEntry.offset = offsetof(SpecializationData, SpecializationData::pathVertexCount);
		pathVertexCountEntry.size = sizeof(uint32_t);

		VkSpecializationMapEntry spatialKernelSizeEntry;
		spatialKernelSizeEntry.constantID = 3;
		spatialKernelSizeEntry.offset = offsetof(SpecializationData, SpecializationData::spatialKernelSize);
		spatialKernelSizeEntry.size = sizeof(uint32_t);

		m_SpecMapEntries = { 
			renderWidthEntry, 
			renderHeightEntry, 
			
			pathVertexCountEntry,
		
			spatialKernelSizeEntry };

		// Update specialization info
		m_SpecInfo.mapEntryCount = m_SpecMapEntries.size();
		m_SpecInfo.pMapEntries = m_SpecMapEntries.data();
		m_SpecInfo.dataSize = sizeof(SpecializationData);
		m_SpecInfo.pData = &m_SpecData;
	}

	void RestirHpmRenderer::CreateLocalInitPipeline(VkDevice device)
	{
		VkPipelineShaderStageCreateInfo shaderStageCI;
		shaderStageCI.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shaderStageCI.pNext = nullptr;
		shaderStageCI.flags = 0;
		shaderStageCI.stage = VK_SHADER_STAGE_COMPUTE_BIT;
		shaderStageCI.module = m_LocalInitShader.GetVulkanModule();
		shaderStageCI.pName = "main";
		shaderStageCI.pSpecializationInfo = &m_SpecInfo;

		VkComputePipelineCreateInfo pipelineCI;
		pipelineCI.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
		pipelineCI.pNext = nullptr;
		pipelineCI.flags = 0;
		pipelineCI.stage = shaderStageCI;
		pipelineCI.layout = m_PipelineLayout;
		pipelineCI.basePipelineHandle = VK_NULL_HANDLE;
		pipelineCI.basePipelineIndex = 0;

		VkResult result = vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &pipelineCI, nullptr, &m_LocalInitPipeline);
		ASSERT_VULKAN(result);
	}

	void RestirHpmRenderer::CreateSpatialReusePipeline(VkDevice device)
	{
		VkPipelineShaderStageCreateInfo shaderStageCI;
		shaderStageCI.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shaderStageCI.pNext = nullptr;
		shaderStageCI.flags = 0;
		shaderStageCI.stage = VK_SHADER_STAGE_COMPUTE_BIT;
		shaderStageCI.module = m_SpatialReuseShader.GetVulkanModule();
		shaderStageCI.pName = "main";
		shaderStageCI.pSpecializationInfo = &m_SpecInfo;

		VkComputePipelineCreateInfo pipelineCI;
		pipelineCI.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
		pipelineCI.pNext = nullptr;
		pipelineCI.flags = 0;
		pipelineCI.stage = shaderStageCI;
		pipelineCI.layout = m_PipelineLayout;
		pipelineCI.basePipelineHandle = VK_NULL_HANDLE;
		pipelineCI.basePipelineIndex = 0;

		VkResult result = vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &pipelineCI, nullptr, &m_SpatialReusePipeline);
		ASSERT_VULKAN(result);
	}

	void RestirHpmRenderer::CreateRenderPipeline(VkDevice device)
	{
		VkPipelineShaderStageCreateInfo shaderStageCI;
		shaderStageCI.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shaderStageCI.pNext = nullptr;
		shaderStageCI.flags = 0;
		shaderStageCI.stage = VK_SHADER_STAGE_COMPUTE_BIT;
		shaderStageCI.module = m_RenderShader.GetVulkanModule();
		shaderStageCI.pName = "main";
		shaderStageCI.pSpecializationInfo = &m_SpecInfo;

		VkComputePipelineCreateInfo pipelineCI;
		pipelineCI.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
		pipelineCI.pNext = nullptr;
		pipelineCI.flags = 0;
		pipelineCI.stage = shaderStageCI;
		pipelineCI.layout = m_PipelineLayout;
		pipelineCI.basePipelineHandle = VK_NULL_HANDLE;
		pipelineCI.basePipelineIndex = 0;

		VkResult result = vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &pipelineCI, nullptr, &m_RenderPipeline);
		ASSERT_VULKAN(result);
	}

	void RestirHpmRenderer::CreateOutputImage(VkDevice device)
	{
		VkFormat format = VK_FORMAT_R32G32B32A32_SFLOAT;

		// Create Image
		VkImageCreateInfo imageCI;
		imageCI.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageCI.pNext = nullptr;
		imageCI.flags = 0;
		imageCI.imageType = VK_IMAGE_TYPE_2D;
		imageCI.format = format;
		imageCI.extent = { m_Width, m_Height, 1 };
		imageCI.mipLevels = 1;
		imageCI.arrayLayers = 1;
		imageCI.samples = VK_SAMPLE_COUNT_1_BIT;
		imageCI.tiling = VK_IMAGE_TILING_OPTIMAL;
		imageCI.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT;
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

		result = vkBeginCommandBuffer(m_CommandBuffer, &beginInfo);
		ASSERT_VULKAN(result);

		vk::CommandRecorder::ImageLayoutTransfer(
			m_CommandBuffer,
			m_OutputImage,
			VK_IMAGE_LAYOUT_PREINITIALIZED,
			VK_IMAGE_LAYOUT_GENERAL,
			VK_ACCESS_NONE,
			VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT,
			VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
			VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);

		result = vkEndCommandBuffer(m_CommandBuffer);
		ASSERT_VULKAN(result);

		VkSubmitInfo submitInfo;
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.pNext = nullptr;
		submitInfo.waitSemaphoreCount = 0;
		submitInfo.pWaitSemaphores = nullptr;
		submitInfo.pWaitDstStageMask = nullptr;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &m_CommandBuffer;
		submitInfo.signalSemaphoreCount = 0;
		submitInfo.pSignalSemaphores = nullptr;

		VkQueue queue = VulkanAPI::GetGraphicsQueue();
		result = vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
		ASSERT_VULKAN(result);
		result = vkQueueWaitIdle(queue);
		ASSERT_VULKAN(result);
	}

	void RestirHpmRenderer::CreatePixelInfoImage(VkDevice device)
	{
		VkFormat format = VK_FORMAT_R32G32B32A32_SFLOAT;

		// Create Image
		VkImageCreateInfo imageCI;
		imageCI.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageCI.pNext = nullptr;
		imageCI.flags = 0;
		imageCI.imageType = VK_IMAGE_TYPE_2D;
		imageCI.format = format;
		imageCI.extent = { m_Width, m_Height, 1 };
		imageCI.mipLevels = 1;
		imageCI.arrayLayers = 1;
		imageCI.samples = VK_SAMPLE_COUNT_1_BIT;
		imageCI.tiling = VK_IMAGE_TILING_OPTIMAL;
		imageCI.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT;
		imageCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		imageCI.queueFamilyIndexCount = 0;
		imageCI.pQueueFamilyIndices = nullptr;
		imageCI.initialLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;

		VkResult result = vkCreateImage(device, &imageCI, nullptr, &m_PixelInfoImage);
		ASSERT_VULKAN(result);

		// Image Memory
		VkMemoryRequirements memoryRequirements;
		vkGetImageMemoryRequirements(device, m_PixelInfoImage, &memoryRequirements);

		VkMemoryAllocateInfo allocateInfo;
		allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocateInfo.pNext = nullptr;
		allocateInfo.allocationSize = memoryRequirements.size;
		allocateInfo.memoryTypeIndex = VulkanAPI::FindMemoryType(
			memoryRequirements.memoryTypeBits,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		result = vkAllocateMemory(device, &allocateInfo, nullptr, &m_PixelInfoImageMemory);
		ASSERT_VULKAN(result);

		result = vkBindImageMemory(device, m_PixelInfoImage, m_PixelInfoImageMemory, 0);
		ASSERT_VULKAN(result);

		// Create image view
		VkImageViewCreateInfo imageViewCI;
		imageViewCI.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		imageViewCI.pNext = nullptr;
		imageViewCI.flags = 0;
		imageViewCI.image = m_PixelInfoImage;
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

		result = vkCreateImageView(device, &imageViewCI, nullptr, &m_PixelInfoImageView);
		ASSERT_VULKAN(result);

		// Change image layout
		VkCommandBufferBeginInfo beginInfo;
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.pNext = nullptr;
		beginInfo.flags = 0;
		beginInfo.pInheritanceInfo = nullptr;

		result = vkBeginCommandBuffer(m_CommandBuffer, &beginInfo);
		ASSERT_VULKAN(result);

		vk::CommandRecorder::ImageLayoutTransfer(
			m_CommandBuffer,
			m_PixelInfoImage,
			VK_IMAGE_LAYOUT_PREINITIALIZED,
			VK_IMAGE_LAYOUT_GENERAL,
			VK_ACCESS_NONE,
			VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT,
			VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
			VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);

		result = vkEndCommandBuffer(m_CommandBuffer);
		ASSERT_VULKAN(result);

		VkSubmitInfo submitInfo;
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.pNext = nullptr;
		submitInfo.waitSemaphoreCount = 0;
		submitInfo.pWaitSemaphores = nullptr;
		submitInfo.pWaitDstStageMask = nullptr;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &m_CommandBuffer;
		submitInfo.signalSemaphoreCount = 0;
		submitInfo.pSignalSemaphores = nullptr;

		VkQueue queue = VulkanAPI::GetGraphicsQueue();
		result = vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
		ASSERT_VULKAN(result);
		result = vkQueueWaitIdle(queue);
		ASSERT_VULKAN(result);
	}

	void RestirHpmRenderer::AllocateAndUpdateDescriptorSet(VkDevice device)
	{
		// Allocate descriptor set
		VkDescriptorSetAllocateInfo descSetAI;
		descSetAI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		descSetAI.pNext = nullptr;
		descSetAI.descriptorPool = m_DescPool;
		descSetAI.descriptorSetCount = 1;
		descSetAI.pSetLayouts = &m_DescSetLayout;

		VkResult result = vkAllocateDescriptorSets(device, &descSetAI, &m_DescSet);
		ASSERT_VULKAN(result);

		// Output image
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

		// Pixel info image
		VkDescriptorImageInfo pixelInfoImageInfo;
		pixelInfoImageInfo.sampler = VK_NULL_HANDLE;
		pixelInfoImageInfo.imageView = m_PixelInfoImageView;
		pixelInfoImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

		VkWriteDescriptorSet pixelInfoImageWrite;
		pixelInfoImageWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		pixelInfoImageWrite.pNext = nullptr;
		pixelInfoImageWrite.dstSet = m_DescSet;
		pixelInfoImageWrite.dstBinding = 1;
		pixelInfoImageWrite.dstArrayElement = 0;
		pixelInfoImageWrite.descriptorCount = 1;
		pixelInfoImageWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		pixelInfoImageWrite.pImageInfo = &pixelInfoImageInfo;
		pixelInfoImageWrite.pBufferInfo = nullptr;
		pixelInfoImageWrite.pTexelBufferView = nullptr;

		// Update descriptor set
		std::vector<VkWriteDescriptorSet> writes = { 
			outputImageWrite,
			pixelInfoImageWrite };
		
		vkUpdateDescriptorSets(device, writes.size(), writes.data(), 0, nullptr);
	}

	void RestirHpmRenderer::RecordCommandBuffer()
	{
		// Begin
		VkCommandBufferBeginInfo beginInfo;
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.pNext = nullptr;
		beginInfo.flags = 0;
		beginInfo.pInheritanceInfo = nullptr;

		VkResult result = vkBeginCommandBuffer(m_CommandBuffer, &beginInfo);
		if (result != VK_SUCCESS)
			Log::Error("Failed to begin VkCommandBuffer", true);

		// Collect descriptor sets
		std::vector<VkDescriptorSet> descSets = {
			m_Camera.GetDescriptorSet(),
			m_VolumeData.GetDescriptorSet(),
			m_DirLight.GetDescriptorSet(),
			m_VolumeReservoir.GetDescriptorSet(),
			m_PointLight.GetDescriptorSet(),
			m_HdrEnvMap.GetDescriptorSet(),
			m_DescSet };

		// Create memory barrier
		VkMemoryBarrier memoryBarrier;
		memoryBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
		memoryBarrier.pNext = nullptr;
		memoryBarrier.srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
		memoryBarrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;

		// Bind descriptor sets
		vkCmdBindDescriptorSets(
			m_CommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_PipelineLayout,
			0, descSets.size(), descSets.data(),
			0, nullptr);

		// Local init pipeline
		vkCmdBindPipeline(m_CommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_LocalInitPipeline);
		vkCmdDispatch(m_CommandBuffer, m_Width / 8, m_Height / 4, 1);

		vkCmdPipelineBarrier(
			m_CommandBuffer,
			VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
			VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
			VK_DEPENDENCY_DEVICE_GROUP_BIT,
			1, &memoryBarrier,
			0, nullptr,
			0, nullptr);

		// Spacial reuse pipeline
		if (m_SpecData.spatialKernelSize != 1)
		{
			vkCmdBindPipeline(m_CommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_SpatialReusePipeline);
			vkCmdDispatch(m_CommandBuffer, m_Width / 8, m_Height / 4, 1);

			vkCmdPipelineBarrier(
				m_CommandBuffer,
				VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
				VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
				VK_DEPENDENCY_DEVICE_GROUP_BIT,
				1, &memoryBarrier,
				0, nullptr,
				0, nullptr);
		}

		// Render pipeline
		vkCmdBindPipeline(m_CommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_RenderPipeline);
		vkCmdDispatch(m_CommandBuffer, m_Width / 8, m_Height / 4, 1);

		// End
		result = vkEndCommandBuffer(m_CommandBuffer);
		if (result != VK_SUCCESS)
			Log::Error("Failed to end VkCommandBuffer", true);
	}
}
