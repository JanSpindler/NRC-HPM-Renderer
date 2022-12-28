#include <engine/graphics/Reference.hpp>
#include <filesystem>
#include <engine/graphics/renderer/McHpmRenderer.hpp>
#include <engine/graphics/VulkanAPI.hpp>
#include <engine/graphics/vulkan/CommandRecorder.hpp>
#include <tinyexr.h>

namespace en
{
	float Reference::Result::GetBias() const
	{
		return ownMean - refMean;
	}

	float Reference::Result::GetRelBias() const
	{
		return GetBias() / refMean;
	}

	float Reference::Result::GetCV() const
	{
		return std::sqrt(ownVar) / ownMean;
	}

	Reference::Reference(
		uint32_t width, 
		uint32_t height, 
		const AppConfig& appConfig, 
		const HpmScene& scene, 
		VkQueue queue)
		:
		m_Width(width),
		m_Height(height),
		m_CmdPool(VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, VulkanAPI::GetGraphicsQFI()),
		m_Cmp1Shader("ref/cmp1.comp", false),
		m_NormShader("ref/norm.comp", false),
		m_Cmp2Shader("ref/cmp2.comp", false),
		m_ResultStagingBuffer(
			sizeof(Reference::Result), 
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
			VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			{}),
		m_ResultBuffer(
			sizeof(Reference::Result),
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			{})
	{
		m_CmdPool.AllocateBuffers(1, VK_COMMAND_BUFFER_LEVEL_PRIMARY);
		m_CmdBuf = m_CmdPool.GetBuffer(0);

		CreateDescriptor();

		InitSpecInfo();
		CreatePipelineLayout();
		CreateCmp1Pipeline();
		CreateNormPipeline();
		CreateCmp2Pipeline();

		CreateRefCameras();
		CreateRefImages(queue);
		GenRefImages(appConfig, scene, queue);
	}

	Reference::Result Reference::CompareNrc(NrcHpmRenderer& renderer, const Camera* oldCamera, VkQueue queue)
	{
		Result result{};
		{
			// Render on noisy renderer
			renderer.SetCamera(queue, m_RefCamera);
			renderer.Render(queue, false);
			ASSERT_VULKAN(vkQueueWaitIdle(queue));

			// Update
			UpdateDescriptor(m_RefImageView, renderer.GetImageView());
			RecordCmpCmdBuf();

			// Submit comparision
			VkSubmitInfo submitInfo = {};
			submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
			submitInfo.pNext = nullptr;
			submitInfo.waitSemaphoreCount = 0;
			submitInfo.pWaitSemaphores = nullptr;
			submitInfo.pWaitDstStageMask = nullptr;
			submitInfo.commandBufferCount = 1;
			submitInfo.pCommandBuffers = &m_CmdBuf;
			submitInfo.signalSemaphoreCount = 0;
			submitInfo.pSignalSemaphores = nullptr;
			ASSERT_VULKAN(vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));
			ASSERT_VULKAN(vkQueueWaitIdle(queue));

			// Sync result buffer to host
			vk::Buffer::Copy(&m_ResultBuffer, &m_ResultStagingBuffer, sizeof(Result));
			m_ResultStagingBuffer.GetData(sizeof(Result), &result, 0, 0);

			// Eval results
			Log::Info(
				"MSE: " + std::to_string(result.mse) +
				" | rBias: " + std::to_string(result.GetRelBias()) +
				" | CV: " + std::to_string(result.GetCV()));
		}

		renderer.SetCamera(queue, oldCamera);
		return result;
	}

	Reference::Result Reference::CompareMc(McHpmRenderer& renderer, const Camera* oldCamera, VkQueue queue)
	{
		Result result{};
		{
			// Render on noisy renderer
			renderer.SetCamera(queue, m_RefCamera);
			renderer.Render(queue);
			ASSERT_VULKAN(vkQueueWaitIdle(queue));

			// Update
			UpdateDescriptor(m_RefImageView, renderer.GetImageView());
			RecordCmpCmdBuf();

			// Submit comparision
			VkSubmitInfo submitInfo = {};
			submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
			submitInfo.pNext = nullptr;
			submitInfo.waitSemaphoreCount = 0;
			submitInfo.pWaitSemaphores = nullptr;
			submitInfo.pWaitDstStageMask = nullptr;
			submitInfo.commandBufferCount = 1;
			submitInfo.pCommandBuffers = &m_CmdBuf;
			submitInfo.signalSemaphoreCount = 0;
			submitInfo.pSignalSemaphores = nullptr;
			ASSERT_VULKAN(vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));
			ASSERT_VULKAN(vkQueueWaitIdle(queue));

			// Sync result buffer to host
			vk::Buffer::Copy(&m_ResultBuffer, &m_ResultStagingBuffer, sizeof(Result));
			m_ResultStagingBuffer.GetData(sizeof(Result), &result, 0, 0);

			// Eval results
			Log::Info(
				"MSE: " + std::to_string(result.mse) +
				" | rBias: " + std::to_string(result.GetRelBias()) +
				" | Var: " + std::to_string(result.ownVar));
		}

		renderer.SetCamera(queue, oldCamera);
		return result;
	}

	void Reference::Destroy()
	{
		VkDevice device = VulkanAPI::GetDevice();

		m_RefCamera->Destroy();
		delete m_RefCamera;

		vkDestroyImageView(device, m_RefImageView, nullptr);
		vkFreeMemory(device, m_RefImageMemory, nullptr);
		vkDestroyImage(device, m_RefImage, nullptr);

		vkDestroyPipeline(device, m_Cmp2Pipeline, nullptr);
		m_Cmp2Shader.Destroy();

		vkDestroyPipeline(device, m_NormPipeline, nullptr);
		m_NormShader.Destroy();

		vkDestroyPipeline(device, m_Cmp1Pipeline, nullptr);
		m_Cmp1Shader.Destroy();

		vkDestroyPipelineLayout(device, m_PipelineLayout, nullptr);

		vkDestroyDescriptorPool(device, m_DescPool, nullptr);
		vkDestroyDescriptorSetLayout(device, m_DescSetLayout, nullptr);

		m_CmdPool.Destroy();

		m_ResultBuffer.Destroy();
		m_ResultStagingBuffer.Destroy();
	}

	void Reference::CreateDescriptor()
	{
		VkDevice device = VulkanAPI::GetDevice();

		// Create desc set layout
		uint32_t binding = 0;

		VkDescriptorSetLayoutBinding refImageBinding = {};
		refImageBinding.binding = binding++;
		refImageBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		refImageBinding.descriptorCount = 1;
		refImageBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
		refImageBinding.pImmutableSamplers = nullptr;

		VkDescriptorSetLayoutBinding cmdImageBinding = {};
		cmdImageBinding.binding = binding++;
		cmdImageBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		cmdImageBinding.descriptorCount = 1;
		cmdImageBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
		cmdImageBinding.pImmutableSamplers = nullptr;

		VkDescriptorSetLayoutBinding resultBufferBinding = {};
		resultBufferBinding.binding = binding++;
		resultBufferBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		resultBufferBinding.descriptorCount = 1;
		resultBufferBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
		resultBufferBinding.pImmutableSamplers = nullptr;

		std::vector<VkDescriptorSetLayoutBinding> bindings = { refImageBinding, cmdImageBinding, resultBufferBinding };

		VkDescriptorSetLayoutCreateInfo layoutCI = {};
		layoutCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutCI.pNext = nullptr;
		layoutCI.flags = 0;
		layoutCI.bindingCount = bindings.size();
		layoutCI.pBindings = bindings.data();
		ASSERT_VULKAN(vkCreateDescriptorSetLayout(device, &layoutCI, nullptr, &m_DescSetLayout));

		// Create desc pool
		VkDescriptorPoolSize storageImagePS = {};
		storageImagePS.type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		storageImagePS.descriptorCount = 2;

		VkDescriptorPoolSize storageBufferPS = {};
		storageBufferPS.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		storageBufferPS.descriptorCount = 1;

		std::vector<VkDescriptorPoolSize> poolSizes = { storageImagePS, storageBufferPS };

		VkDescriptorPoolCreateInfo poolCI = {};
		poolCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolCI.pNext = nullptr;
		poolCI.flags = 0;
		poolCI.maxSets = 1;
		poolCI.poolSizeCount = poolSizes.size();
		poolCI.pPoolSizes = poolSizes.data();
		ASSERT_VULKAN(vkCreateDescriptorPool(device, &poolCI, nullptr, &m_DescPool));

		// Allocate desc set
		VkDescriptorSetAllocateInfo descSetAI = {};
		descSetAI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		descSetAI.pNext = nullptr;
		descSetAI.descriptorPool = m_DescPool;
		descSetAI.descriptorSetCount = 1;
		descSetAI.pSetLayouts = &m_DescSetLayout;
		ASSERT_VULKAN(vkAllocateDescriptorSets(device, &descSetAI, &m_DescSet));
	}

	void Reference::UpdateDescriptor(VkImageView refImageView, VkImageView cmpImageView)
	{
		uint32_t binding = 0;

		VkDescriptorImageInfo refImageInfo = {};
		refImageInfo.sampler = VK_NULL_HANDLE;
		refImageInfo.imageView = refImageView;
		refImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
		
		VkWriteDescriptorSet refImageWrite = {};
		refImageWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		refImageWrite.pNext = nullptr;
		refImageWrite.dstSet = m_DescSet;
		refImageWrite.dstBinding = binding++;
		refImageWrite.dstArrayElement = 0;
		refImageWrite.descriptorCount = 1;
		refImageWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		refImageWrite.pImageInfo = &refImageInfo;
		refImageWrite.pBufferInfo = nullptr;
		refImageWrite.pTexelBufferView = nullptr;

		VkDescriptorImageInfo cmpImageInfo = {};
		cmpImageInfo.sampler = VK_NULL_HANDLE;
		cmpImageInfo.imageView = cmpImageView;
		cmpImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

		VkWriteDescriptorSet cmpImageWrite = {};
		cmpImageWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		cmpImageWrite.pNext = nullptr;
		cmpImageWrite.dstSet = m_DescSet;
		cmpImageWrite.dstBinding = binding++;
		cmpImageWrite.dstArrayElement = 0;
		cmpImageWrite.descriptorCount = 1;
		cmpImageWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		cmpImageWrite.pImageInfo = &cmpImageInfo;
		cmpImageWrite.pBufferInfo = nullptr;
		cmpImageWrite.pTexelBufferView = nullptr;

		VkDescriptorBufferInfo resultBufferInfo = {};
		resultBufferInfo.buffer = m_ResultBuffer.GetVulkanHandle();
		resultBufferInfo.offset = 0;
		resultBufferInfo.range = sizeof(Result);

		VkWriteDescriptorSet resultBufferWrite = {};
		resultBufferWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		resultBufferWrite.pNext = nullptr;
		resultBufferWrite.dstSet = m_DescSet;
		resultBufferWrite.dstBinding = binding++;
		resultBufferWrite.dstArrayElement = 0;
		resultBufferWrite.descriptorCount = 1;
		resultBufferWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		resultBufferWrite.pImageInfo = nullptr;
		resultBufferWrite.pBufferInfo = &resultBufferInfo;
		resultBufferWrite.pTexelBufferView = nullptr;

		std::vector<VkWriteDescriptorSet> writes = { refImageWrite, cmpImageWrite, resultBufferWrite };
		vkUpdateDescriptorSets(VulkanAPI::GetDevice(), writes.size(), writes.data(), 0, nullptr);
	}

	void Reference::InitSpecInfo()
	{
		m_SpecData.width = m_Width;
		m_SpecData.height = m_Height;

		uint32_t constantID = 0;

		VkSpecializationMapEntry widthEntry = {};
		widthEntry.constantID = constantID++;
		widthEntry.offset = offsetof(SpecializationData, SpecializationData::width);
		widthEntry.size = sizeof(uint32_t);

		VkSpecializationMapEntry heightEntry = {};
		heightEntry.constantID = constantID++;
		heightEntry.offset = offsetof(SpecializationData, SpecializationData::height);
		heightEntry.size = sizeof(uint32_t);

		m_SpecEntries = { widthEntry, heightEntry };

		m_SpecInfo.mapEntryCount = m_SpecEntries.size();
		m_SpecInfo.pMapEntries = m_SpecEntries.data();
		m_SpecInfo.dataSize = sizeof(SpecializationData);
		m_SpecInfo.pData = &m_SpecData;
	}

	void Reference::CreatePipelineLayout()
	{
		VkPipelineLayoutCreateInfo layoutCI = {};
		layoutCI.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		layoutCI.pNext = nullptr;
		layoutCI.flags = 0;
		layoutCI.setLayoutCount = 1;
		layoutCI.pSetLayouts = &m_DescSetLayout;
		layoutCI.pushConstantRangeCount = 0;
		layoutCI.pPushConstantRanges = nullptr;
		ASSERT_VULKAN(vkCreatePipelineLayout(VulkanAPI::GetDevice(), &layoutCI, nullptr, &m_PipelineLayout));
	}

	void Reference::CreateCmp1Pipeline()
	{
		VkPipelineShaderStageCreateInfo stageCI = {};
		stageCI.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		stageCI.pNext = nullptr;
		stageCI.flags = 0;
		stageCI.stage = VK_SHADER_STAGE_COMPUTE_BIT;
		stageCI.module = m_Cmp1Shader.GetVulkanModule();
		stageCI.pName = "main";
		stageCI.pSpecializationInfo = &m_SpecInfo;

		VkComputePipelineCreateInfo pipelineCI = {};
		pipelineCI.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
		pipelineCI.pNext = nullptr;
		pipelineCI.flags = 0;
		pipelineCI.stage = stageCI;
		pipelineCI.layout = m_PipelineLayout;
		pipelineCI.basePipelineHandle = VK_NULL_HANDLE;
		pipelineCI.basePipelineIndex = 0;
		ASSERT_VULKAN(vkCreateComputePipelines(VulkanAPI::GetDevice(), VK_NULL_HANDLE, 1, &pipelineCI, nullptr, &m_Cmp1Pipeline));
	}

	void Reference::CreateNormPipeline()
	{
		VkPipelineShaderStageCreateInfo stageCI = {};
		stageCI.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		stageCI.pNext = nullptr;
		stageCI.flags = 0;
		stageCI.stage = VK_SHADER_STAGE_COMPUTE_BIT;
		stageCI.module = m_NormShader.GetVulkanModule();
		stageCI.pName = "main";
		stageCI.pSpecializationInfo = &m_SpecInfo;

		VkComputePipelineCreateInfo pipelineCI = {};
		pipelineCI.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
		pipelineCI.pNext = nullptr;
		pipelineCI.flags = 0;
		pipelineCI.stage = stageCI;
		pipelineCI.layout = m_PipelineLayout;
		pipelineCI.basePipelineHandle = VK_NULL_HANDLE;
		pipelineCI.basePipelineIndex = 0;
		ASSERT_VULKAN(vkCreateComputePipelines(VulkanAPI::GetDevice(), VK_NULL_HANDLE, 1, &pipelineCI, nullptr, &m_NormPipeline));
	}

	void Reference::CreateCmp2Pipeline()
	{
		VkPipelineShaderStageCreateInfo stageCI = {};
		stageCI.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		stageCI.pNext = nullptr;
		stageCI.flags = 0;
		stageCI.stage = VK_SHADER_STAGE_COMPUTE_BIT;
		stageCI.module = m_Cmp2Shader.GetVulkanModule();
		stageCI.pName = "main";
		stageCI.pSpecializationInfo = &m_SpecInfo;

		VkComputePipelineCreateInfo pipelineCI = {};
		pipelineCI.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
		pipelineCI.pNext = nullptr;
		pipelineCI.flags = 0;
		pipelineCI.stage = stageCI;
		pipelineCI.layout = m_PipelineLayout;
		pipelineCI.basePipelineHandle = VK_NULL_HANDLE;
		pipelineCI.basePipelineIndex = 0;
		ASSERT_VULKAN(vkCreateComputePipelines(VulkanAPI::GetDevice(), VK_NULL_HANDLE, 1, &pipelineCI, nullptr, &m_Cmp2Pipeline));
	}

	void Reference::RecordCmpCmdBuf()
	{
		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.pNext = nullptr;
		beginInfo.flags = 0;
		beginInfo.pInheritanceInfo = nullptr;
		ASSERT_VULKAN(vkBeginCommandBuffer(m_CmdBuf, &beginInfo));

		vkCmdFillBuffer(m_CmdBuf, m_ResultBuffer.GetVulkanHandle(), 0, sizeof(Result), 0);

		vkCmdBindDescriptorSets(m_CmdBuf, VK_PIPELINE_BIND_POINT_COMPUTE, m_PipelineLayout, 0, 1, &m_DescSet, 0, nullptr);

		vkCmdBindPipeline(m_CmdBuf, VK_PIPELINE_BIND_POINT_COMPUTE, m_Cmp1Pipeline);
		vkCmdDispatch(m_CmdBuf, m_Width / 32, m_Height, 1);

		vkCmdBindPipeline(m_CmdBuf, VK_PIPELINE_BIND_POINT_COMPUTE, m_NormPipeline);
		vkCmdDispatch(m_CmdBuf, 1, 1, 1);

		vkCmdBindPipeline(m_CmdBuf, VK_PIPELINE_BIND_POINT_COMPUTE, m_Cmp2Pipeline);
		vkCmdDispatch(m_CmdBuf, m_Width / 32, m_Height, 1);

		ASSERT_VULKAN(vkEndCommandBuffer(m_CmdBuf));
	}

	void Reference::CreateRefCameras()
	{
		const float aspectRatio = static_cast<float>(m_Width) / static_cast<float>(m_Height);

		m_RefCamera = new en::Camera(
			glm::vec3(64.0f, 0.0f, 0.0f),
			glm::vec3(-1.0f, 0.0f, 0.0f),
			glm::vec3(0.0f, 1.0f, 0.0f),
			aspectRatio,
			glm::radians(60.0f),
			0.1f,
			100.0f);
	}

	void Reference::CreateRefImages(VkQueue queue)
	{
		VkDevice device = VulkanAPI::GetDevice();

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
			imageCI.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
			imageCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
			imageCI.queueFamilyIndexCount = 0;
			imageCI.pQueueFamilyIndices = nullptr;
			imageCI.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

			VkResult result = vkCreateImage(device, &imageCI, nullptr, &m_RefImage);
			ASSERT_VULKAN(result);

			// Image Memory
			VkMemoryRequirements memoryRequirements;
			vkGetImageMemoryRequirements(device, m_RefImage, &memoryRequirements);

			VkMemoryAllocateInfo allocateInfo;
			allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
			allocateInfo.pNext = nullptr;
			allocateInfo.allocationSize = memoryRequirements.size;
			allocateInfo.memoryTypeIndex = VulkanAPI::FindMemoryType(
				memoryRequirements.memoryTypeBits,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

			result = vkAllocateMemory(device, &allocateInfo, nullptr, &m_RefImageMemory);
			ASSERT_VULKAN(result);

			result = vkBindImageMemory(device, m_RefImage, m_RefImageMemory, 0);
			ASSERT_VULKAN(result);

			// Create image view
			VkImageViewCreateInfo imageViewCI;
			imageViewCI.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			imageViewCI.pNext = nullptr;
			imageViewCI.flags = 0;
			imageViewCI.image = m_RefImage;
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

			result = vkCreateImageView(device, &imageViewCI, nullptr, &m_RefImageView);
			ASSERT_VULKAN(result);

			// Change image layout
			VkCommandBufferBeginInfo beginInfo;
			beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			beginInfo.pNext = nullptr;
			beginInfo.flags = 0;
			beginInfo.pInheritanceInfo = nullptr;

			result = vkBeginCommandBuffer(m_CmdBuf, &beginInfo);
			ASSERT_VULKAN(result);

			vk::CommandRecorder::ImageLayoutTransfer(
				m_CmdBuf,
				m_RefImage,
				VK_IMAGE_LAYOUT_UNDEFINED,
				VK_IMAGE_LAYOUT_GENERAL,
				VK_ACCESS_NONE,
				VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT,
				VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
				VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);

			result = vkEndCommandBuffer(m_CmdBuf);
			ASSERT_VULKAN(result);

			VkSubmitInfo submitInfo;
			submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
			submitInfo.pNext = nullptr;
			submitInfo.waitSemaphoreCount = 0;
			submitInfo.pWaitSemaphores = nullptr;
			submitInfo.pWaitDstStageMask = nullptr;
			submitInfo.commandBufferCount = 1;
			submitInfo.pCommandBuffers = &m_CmdBuf;
			submitInfo.signalSemaphoreCount = 0;
			submitInfo.pSignalSemaphores = nullptr;

			VkQueue queue = VulkanAPI::GetGraphicsQueue();
			result = vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
			ASSERT_VULKAN(result);
			result = vkQueueWaitIdle(queue);
			ASSERT_VULKAN(result);
		}
	}

	void Reference::GenRefImages(const AppConfig& appConfig, const HpmScene& scene,	VkQueue queue)
	{
		const uint32_t sceneID = appConfig.scene.id;

		// Create reference folder if not exists
		std::string referenceDirPath = "reference/" + std::to_string(sceneID) + "/";
#if __cplusplus >= 201703L
		en::Log::Warn("C++ version lower then 17. Cant create reference data");
#else
		// If reference image folder does not exists create reference images
		if (!std::filesystem::is_directory(referenceDirPath) || !std::filesystem::exists(referenceDirPath))
		{
			en::Log::Info("Reference folder for scene " + std::to_string(sceneID) + " was not found. Creating reference images");

			// Create reference renderer
			McHpmRenderer refRenderer(m_Width, m_Height, 64, true, m_RefCamera, scene);

			// Create folder
			std::filesystem::create_directory(referenceDirPath);

			// Generate reference data
			{
				en::Log::Info("Generating reference image " + std::to_string(0));

				// Generate reference image
				for (size_t frame = 0; frame < 8192; frame++)
				{
					refRenderer.Render(queue);
					ASSERT_VULKAN(vkQueueWaitIdle(queue));
				}

				// Export reference image
				refRenderer.ExportOutputImageToFile(queue, referenceDirPath + std::to_string(0) + ".exr");

				// Copy to ref image for faster comparison
				//CopyToRefImage(i, refRenderer.GetImage(), queue);
			}

			refRenderer.Destroy();
		}
#endif

		// Load reference images from path
		const size_t imageBufferSize = 4 * sizeof(float) * m_Width * m_Height;
		vk::Buffer stagingBuffer(
			imageBufferSize,
			VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			{});

		{
			const std::string refImagePath = referenceDirPath + std::to_string(0) + ".exr";

			// Load exr to memory
			float* rgba = nullptr;
			int width = -1;
			int height = -1;
			if (TINYEXR_SUCCESS != LoadEXR(&rgba, &width, &height, refImagePath.c_str(), nullptr))
			{
				Log::Error("TinyEXR failed to load " + refImagePath, true);
			}
			if (width != m_Width || height != m_Height) { Log::Error(refImagePath + " has wrong resolution", true); }

			// Load to staging buffer
			stagingBuffer.SetData(imageBufferSize, rgba, 0, 0);
			free(rgba);

			// Load to gpu
			VkCommandBufferBeginInfo beginInfo = {};
			beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			beginInfo.pNext = nullptr;
			beginInfo.flags = 0;
			beginInfo.pInheritanceInfo = nullptr;
			ASSERT_VULKAN(vkBeginCommandBuffer(m_CmdBuf, &beginInfo));

			VkBufferImageCopy bufferImageCopy = {};
			bufferImageCopy.bufferOffset = 0;
			bufferImageCopy.bufferRowLength = m_Width;
			bufferImageCopy.bufferImageHeight = m_Height;
			bufferImageCopy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			bufferImageCopy.imageSubresource.mipLevel = 0;
			bufferImageCopy.imageSubresource.baseArrayLayer = 0;
			bufferImageCopy.imageSubresource.layerCount = 1;
			bufferImageCopy.imageOffset = { 0, 0, 0 };
			bufferImageCopy.imageExtent = { m_Width, m_Height, 1 };

			vkCmdCopyBufferToImage(m_CmdBuf, stagingBuffer.GetVulkanHandle(), m_RefImage, VK_IMAGE_LAYOUT_GENERAL, 1, &bufferImageCopy);

			ASSERT_VULKAN(vkEndCommandBuffer(m_CmdBuf));

			VkSubmitInfo submitInfo = {};
			submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
			submitInfo.pNext = nullptr;
			submitInfo.waitSemaphoreCount = 0;
			submitInfo.pWaitSemaphores = nullptr;
			submitInfo.pWaitDstStageMask = nullptr;
			submitInfo.commandBufferCount = 1;
			submitInfo.pCommandBuffers = &m_CmdBuf;
			submitInfo.signalSemaphoreCount = 0;
			submitInfo.pSignalSemaphores = nullptr;
			ASSERT_VULKAN(vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));
			ASSERT_VULKAN(vkQueueWaitIdle(queue));
		}

		stagingBuffer.Destroy();
	}

	void Reference::CopyToRefImage(uint32_t imageIdx, VkImage srcImage, VkQueue queue)
	{
		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.pNext = nullptr;
		beginInfo.flags = 0;
		beginInfo.pInheritanceInfo = nullptr;
		ASSERT_VULKAN(vkBeginCommandBuffer(m_CmdBuf, &beginInfo));

		VkImageCopy imageCopy = {};
		imageCopy.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageCopy.srcSubresource.mipLevel = 0;
		imageCopy.srcSubresource.baseArrayLayer = 0;
		imageCopy.srcSubresource.layerCount = 1;
		imageCopy.srcOffset = { 0, 0, 0 };
		imageCopy.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageCopy.dstSubresource.mipLevel = 0;
		imageCopy.dstSubresource.baseArrayLayer = 0;
		imageCopy.dstSubresource.layerCount = 1;
		imageCopy.dstOffset = { 0, 0, 0 };
		imageCopy.extent = { m_Width, m_Height, 1 };
		vkCmdCopyImage(
			m_CmdBuf, 
			srcImage, 
			VK_IMAGE_LAYOUT_GENERAL, 
			m_RefImage, 
			VK_IMAGE_LAYOUT_GENERAL, 
			1, 
			&imageCopy);

		ASSERT_VULKAN(vkEndCommandBuffer(m_CmdBuf));

		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.pNext = nullptr;
		submitInfo.waitSemaphoreCount = 0;
		submitInfo.pWaitSemaphores = nullptr;
		submitInfo.pWaitDstStageMask = nullptr;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &m_CmdBuf;
		submitInfo.signalSemaphoreCount = 0;
		submitInfo.pSignalSemaphores = nullptr;

		ASSERT_VULKAN(vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));
		ASSERT_VULKAN(vkQueueWaitIdle(queue));
	}
}
