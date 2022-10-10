#include <engine/graphics/NeuralRadianceCache.hpp>
#include <engine/graphics/renderer/NrcHpmRenderer.hpp>
#include <engine/util/Log.hpp>
#include <engine/graphics/vulkan/CommandRecorder.hpp>

#define ASSERT_CUDA(error) if (error != cudaSuccess) { en::Log::Error("Cuda assert triggered: " + std::string(cudaGetErrorName(error)), true); }

namespace en
{
	VkDescriptorSetLayout NrcHpmRenderer::m_DescSetLayout;
	VkDescriptorPool NrcHpmRenderer::m_DescPool;

	void NrcHpmRenderer::Init(VkDevice device)
	{
		// Create desc set layout
		VkDescriptorSetLayoutBinding outputImageBinding;
		outputImageBinding.binding = 0;
		outputImageBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		outputImageBinding.descriptorCount = 1;
		outputImageBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
		outputImageBinding.pImmutableSamplers = nullptr;

		VkDescriptorSetLayoutBinding primaryRayColorImageBinding;
		primaryRayColorImageBinding.binding = 1;
		primaryRayColorImageBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		primaryRayColorImageBinding.descriptorCount = 1;
		primaryRayColorImageBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
		primaryRayColorImageBinding.pImmutableSamplers = nullptr;

		VkDescriptorSetLayoutBinding primaryRayInfoImageBinding;
		primaryRayInfoImageBinding.binding = 2;
		primaryRayInfoImageBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		primaryRayInfoImageBinding.descriptorCount = 1;
		primaryRayInfoImageBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
		primaryRayInfoImageBinding.pImmutableSamplers = nullptr;

		VkDescriptorSetLayoutBinding neuralRayOriginImageBinding;
		neuralRayOriginImageBinding.binding = 3;
		neuralRayOriginImageBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		neuralRayOriginImageBinding.descriptorCount = 1;
		neuralRayOriginImageBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
		neuralRayOriginImageBinding.pImmutableSamplers = nullptr;

		VkDescriptorSetLayoutBinding neuralRayDirImageBinding;
		neuralRayDirImageBinding.binding = 4;
		neuralRayDirImageBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		neuralRayDirImageBinding.descriptorCount = 1;
		neuralRayDirImageBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
		neuralRayDirImageBinding.pImmutableSamplers = nullptr;

		VkDescriptorSetLayoutBinding neuralRayColorImageBinding;
		neuralRayColorImageBinding.binding = 5;
		neuralRayColorImageBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		neuralRayColorImageBinding.descriptorCount = 1;
		neuralRayColorImageBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
		neuralRayColorImageBinding.pImmutableSamplers = nullptr;

		VkDescriptorSetLayoutBinding neuralRayTargetImageBinding;
		neuralRayTargetImageBinding.binding = 6;
		neuralRayTargetImageBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		neuralRayTargetImageBinding.descriptorCount = 1;
		neuralRayTargetImageBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
		neuralRayTargetImageBinding.pImmutableSamplers = nullptr;

		std::vector<VkDescriptorSetLayoutBinding> bindings = { 
			outputImageBinding,
			primaryRayColorImageBinding,
			primaryRayInfoImageBinding,
			neuralRayOriginImageBinding,
			neuralRayDirImageBinding,
			neuralRayColorImageBinding,
			neuralRayTargetImageBinding };

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
		storageImagePoolSize.descriptorCount = 7;

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

	void NrcHpmRenderer::Shutdown(VkDevice device)
	{
		vkDestroyDescriptorPool(device, m_DescPool, nullptr);
		vkDestroyDescriptorSetLayout(device, m_DescSetLayout, nullptr);
	}

	NrcHpmRenderer::NrcHpmRenderer(
		uint32_t width,
		uint32_t height,
		uint32_t trainWidth,
		uint32_t trainHeight,
		const Camera& camera,
		const VolumeData& volumeData,
		const DirLight& dirLight,
		const PointLight& pointLight,
		const HdrEnvMap& hdrEnvMap,
		NeuralRadianceCache& nrc)
		:
		m_FrameWidth(width),
		m_FrameHeight(height),
		m_TrainWidth(trainWidth),
		m_TrainHeight(trainHeight),
		m_GenRaysShader("nrc/gen_rays.comp", false),
		m_CalcNrcTargetsShader("nrc/calc_targets.comp", false),
		m_RenderShader("nrc/render.comp", false),
		m_CommandPool(VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, VulkanAPI::GetGraphicsQFI()),
		m_Camera(camera),
		m_VolumeData(volumeData),
		m_DirLight(dirLight),
		m_PointLight(pointLight),
		m_HdrEnvMap(hdrEnvMap),
		m_Nrc(nrc)
	{
		CreateNrcBuffers();

		m_Nrc.Init(
			m_FrameWidth * m_FrameHeight, 
			m_TrainWidth * m_TrainHeight,
			nullptr,
			nullptr,
			nullptr,
			nullptr);

		VkDevice device = VulkanAPI::GetDevice();

		m_CommandPool.AllocateBuffers(1, VK_COMMAND_BUFFER_LEVEL_PRIMARY);
		m_CommandBuffer = m_CommandPool.GetBuffer(0);

		CreateNrcBuffers();

		CreatePipelineLayout(device);

		InitSpecializationConstants();

		CreateGenRaysPipeline(device);
		CreateCalcNrcTargetsPipeline(device);
		CreateRenderPipeline(device);

		CreateOutputImage(device);
		CreatePrimaryRayColorImage(device);
		CreatePrimaryRayInfoImage(device);
		CreateNeuralRayTargetImage(device);

		AllocateAndUpdateDescriptorSet(device);

		RecordCommandBuffer();
	}

	void NrcHpmRenderer::Render(VkQueue queue) const
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

	void NrcHpmRenderer::Destroy()
	{
		VkDevice device = VulkanAPI::GetDevice();

		m_CommandPool.Destroy();

		vkDestroyImageView(device, m_NeuralRayTargetImageView, nullptr);
		vkFreeMemory(device, m_NeuralRayTargetImageMemory, nullptr);
		vkDestroyImage(device, m_NeuralRayTargetImage, nullptr);

		vkDestroyImageView(device, m_PrimaryRayInfoImageView, nullptr);
		vkFreeMemory(device, m_PrimaryRayInfoImageMemory, nullptr);
		vkDestroyImage(device, m_PrimaryRayInfoImage, nullptr);

		vkDestroyImageView(device, m_PrimaryRayColorImageView, nullptr);
		vkFreeMemory(device, m_PrimaryRayColorImageMemory, nullptr);
		vkDestroyImage(device, m_PrimaryRayColorImage, nullptr);

		vkDestroyImageView(device, m_OutputImageView, nullptr);
		vkFreeMemory(device, m_OutputImageMemory, nullptr);
		vkDestroyImage(device, m_OutputImage, nullptr);

		vkDestroyPipeline(device, m_RenderPipeline, nullptr);
		m_RenderShader.Destroy();
		
		vkDestroyPipeline(device, m_CalcNrcTargetsPipeline, nullptr);
		m_CalcNrcTargetsShader.Destroy();

		vkDestroyPipeline(device, m_GenRaysPipeline, nullptr);
		m_GenRaysShader.Destroy();

		vkDestroyPipelineLayout(device, m_PipelineLayout, nullptr);
	
		m_NrcTrainTargetBuffer->Destroy();
		delete m_NrcTrainTargetBuffer;

		m_NrcTrainInputBuffer->Destroy();
		delete m_NrcTrainInputBuffer;

		m_NrcInferOutputBuffer->Destroy();
		delete m_NrcInferOutputBuffer;
		
		m_NrcInferInputBuffer->Destroy();
		delete m_NrcInferInputBuffer;
	}

	VkImage NrcHpmRenderer::GetImage() const
	{
		return m_OutputImage;
	}

	VkImageView NrcHpmRenderer::GetImageView() const
	{
		return m_OutputImageView;
	}

	void NrcHpmRenderer::CreateNrcBuffers()
	{
		// Create buffers
		vk::Buffer* m_NrcInferInputBuffer = new vk::Buffer(
			1, 
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, 
			{},
			VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT);

		vk::Buffer* m_NrcInferOutputBuffer = new vk::Buffer(
			1,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			{},
			VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT);
		
		vk::Buffer* m_NrcTrainInputBuffer = new vk::Buffer(
			1,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			{},
			VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT);
		
		vk::Buffer* m_NrcTrainTargetBuffer = new vk::Buffer(
			1,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			{},
			VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT);

		// Get cuda external memory
		cudaExternalMemoryHandleDesc cuExtMemHandleDesc{};
		cuExtMemHandleDesc.type = cudaExternalMemoryHandleTypeOpaqueWin32;

		cuExtMemHandleDesc.handle.win32.handle = m_NrcInferInputBuffer->GetMemoryWin32Handle();
		cuExtMemHandleDesc.size = 1;
		cudaError_t cudaResult = cudaImportExternalMemory(&m_NrcInferInputCuExtMem, &cuExtMemHandleDesc);
		ASSERT_CUDA(cudaResult);

		cuExtMemHandleDesc.handle.win32.handle = m_NrcInferOutputBuffer->GetMemoryWin32Handle();
		cuExtMemHandleDesc.size = 1;
		cudaResult = cudaImportExternalMemory(&m_NrcInferOutputCuExtMem, &cuExtMemHandleDesc);
		ASSERT_CUDA(cudaResult);

		cuExtMemHandleDesc.handle.win32.handle = m_NrcTrainInputBuffer->GetMemoryWin32Handle();
		cuExtMemHandleDesc.size = 1;
		cudaResult = cudaImportExternalMemory(&m_NrcTrainInputCuExtMem, &cuExtMemHandleDesc);
		ASSERT_CUDA(cudaResult);

		cuExtMemHandleDesc.handle.win32.handle = m_NrcTrainTargetBuffer->GetMemoryWin32Handle();
		cuExtMemHandleDesc.size = 1;
		cudaResult = cudaImportExternalMemory(&m_NrcTrainTargetCuExtMem, &cuExtMemHandleDesc);
		ASSERT_CUDA(cudaResult);

		// Get cuda buffer
		cudaExternalMemoryBufferDesc cudaExtBufferDesc{};
		cudaExtBufferDesc.offset = 0;
		cudaExtBufferDesc.flags = 0;

		cudaExtBufferDesc.size = 1;
		cudaResult = cudaExternalMemoryGetMappedBuffer(&m_NrcInferInputDCuBuffer, m_NrcInferInputCuExtMem, &cudaExtBufferDesc);
		ASSERT_CUDA(cudaResult);

		cudaExtBufferDesc.size = 1;
		cudaResult = cudaExternalMemoryGetMappedBuffer(&m_NrcInferOutputDCuBuffer, m_NrcInferOutputCuExtMem, &cudaExtBufferDesc);
		ASSERT_CUDA(cudaResult);

		cudaExtBufferDesc.size = 1;
		cudaResult = cudaExternalMemoryGetMappedBuffer(&m_NrcTrainInputDCuBuffer, m_NrcTrainInputCuExtMem, &cudaExtBufferDesc);
		ASSERT_CUDA(cudaResult);

		cudaExtBufferDesc.size = 1;
		cudaResult = cudaExternalMemoryGetMappedBuffer(&m_NrcTrainTargetDCuBuffer, m_NrcTrainTargetCuExtMem, &cudaExtBufferDesc);
		ASSERT_CUDA(cudaResult);
	}

	void NrcHpmRenderer::CreatePipelineLayout(VkDevice device)
	{
		std::vector<VkDescriptorSetLayout> layouts = {
			Camera::GetDescriptorSetLayout(),
			VolumeData::GetDescriptorSetLayout(),
			DirLight::GetDescriptorSetLayout(),
			PointLight::GetDescriptorSetLayout(),
			HdrEnvMap::GetDescriptorSetLayout(),
			m_DescSetLayout };

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

	void NrcHpmRenderer::InitSpecializationConstants()
	{
		// Fill struct
		m_SpecData.renderWidth = m_FrameWidth;
		m_SpecData.renderHeight = m_FrameHeight;
		
		m_SpecData.trainWidth = m_TrainWidth;
		m_SpecData.trainHeight = m_TrainHeight;

		// Init map entries
		// Render size
		VkSpecializationMapEntry renderWidthEntry;
		renderWidthEntry.constantID = 0;
		renderWidthEntry.offset = offsetof(SpecializationData, SpecializationData::renderWidth);
		renderWidthEntry.size = sizeof(uint32_t);

		VkSpecializationMapEntry renderHeightEntry;
		renderHeightEntry.constantID = 1;
		renderHeightEntry.offset = offsetof(SpecializationData, SpecializationData::renderHeight);
		renderHeightEntry.size = sizeof(uint32_t);

		// Train size
		VkSpecializationMapEntry trainWidthEntry;
		trainWidthEntry.constantID = 2;
		trainWidthEntry.offset = offsetof(SpecializationData, SpecializationData::trainWidth);
		trainWidthEntry.size = sizeof(uint32_t);

		VkSpecializationMapEntry trainHeightEntry;
		trainHeightEntry.constantID = 3;
		trainHeightEntry.offset = offsetof(SpecializationData, SpecializationData::trainHeight);
		trainHeightEntry.size = sizeof(uint32_t);

		m_SpecMapEntries = {
			renderWidthEntry,
			renderHeightEntry,

			trainWidthEntry,
			trainHeightEntry };

		m_SpecInfo.mapEntryCount = m_SpecMapEntries.size();
		m_SpecInfo.pMapEntries = m_SpecMapEntries.data();
		m_SpecInfo.dataSize = sizeof(SpecializationData);
		m_SpecInfo.pData = &m_SpecData;
	}

	void NrcHpmRenderer::CreateGenRaysPipeline(VkDevice device)
	{
		VkPipelineShaderStageCreateInfo shaderStage;
		shaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shaderStage.pNext = nullptr;
		shaderStage.flags = 0;
		shaderStage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
		shaderStage.module = m_GenRaysShader.GetVulkanModule();
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

		VkResult result = vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &pipelineCI, nullptr, &m_GenRaysPipeline);
		ASSERT_VULKAN(result);
	}

	void NrcHpmRenderer::CreateCalcNrcTargetsPipeline(VkDevice device)
	{
		VkPipelineShaderStageCreateInfo shaderStage;
		shaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shaderStage.pNext = nullptr;
		shaderStage.flags = 0;
		shaderStage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
		shaderStage.module = m_CalcNrcTargetsShader.GetVulkanModule();
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

		VkResult result = vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &pipelineCI, nullptr, &m_CalcNrcTargetsPipeline);
		ASSERT_VULKAN(result);
	}

	void NrcHpmRenderer::CreateRenderPipeline(VkDevice device)
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

	void NrcHpmRenderer::CreateOutputImage(VkDevice device)
	{
		VkFormat format = VK_FORMAT_R32G32B32A32_SFLOAT;

		// Create Image
		VkImageCreateInfo imageCI;
		imageCI.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageCI.pNext = nullptr;
		imageCI.flags = 0;
		imageCI.imageType = VK_IMAGE_TYPE_2D;
		imageCI.format = format;
		imageCI.extent = { m_FrameWidth, m_FrameHeight, 1 };
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

	void NrcHpmRenderer::CreatePrimaryRayColorImage(VkDevice device)
	{
		VkFormat format = VK_FORMAT_R32G32B32A32_SFLOAT;

		// Create Image
		VkImageCreateInfo imageCI;
		imageCI.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageCI.pNext = nullptr;
		imageCI.flags = 0;
		imageCI.imageType = VK_IMAGE_TYPE_2D;
		imageCI.format = format;
		imageCI.extent = { m_FrameWidth, m_FrameHeight, 1 };
		imageCI.mipLevels = 1;
		imageCI.arrayLayers = 1;
		imageCI.samples = VK_SAMPLE_COUNT_1_BIT;
		imageCI.tiling = VK_IMAGE_TILING_OPTIMAL;
		imageCI.usage = VK_IMAGE_USAGE_STORAGE_BIT;
		imageCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		imageCI.queueFamilyIndexCount = 0;
		imageCI.pQueueFamilyIndices = nullptr;
		imageCI.initialLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;

		VkResult result = vkCreateImage(device, &imageCI, nullptr, &m_PrimaryRayColorImage);
		ASSERT_VULKAN(result);

		// Image Memory
		VkMemoryRequirements memoryRequirements;
		vkGetImageMemoryRequirements(device, m_PrimaryRayColorImage, &memoryRequirements);

		VkMemoryAllocateInfo allocateInfo;
		allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocateInfo.pNext = nullptr;
		allocateInfo.allocationSize = memoryRequirements.size;
		allocateInfo.memoryTypeIndex = VulkanAPI::FindMemoryType(
			memoryRequirements.memoryTypeBits,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		result = vkAllocateMemory(device, &allocateInfo, nullptr, &m_PrimaryRayColorImageMemory);
		ASSERT_VULKAN(result);

		result = vkBindImageMemory(device, m_PrimaryRayColorImage, m_PrimaryRayColorImageMemory, 0);
		ASSERT_VULKAN(result);

		// Create image view
		VkImageViewCreateInfo imageViewCI;
		imageViewCI.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		imageViewCI.pNext = nullptr;
		imageViewCI.flags = 0;
		imageViewCI.image = m_PrimaryRayColorImage;
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

		result = vkCreateImageView(device, &imageViewCI, nullptr, &m_PrimaryRayColorImageView);
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
			m_PrimaryRayColorImage,
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

	void NrcHpmRenderer::CreatePrimaryRayInfoImage(VkDevice device)
	{
		VkFormat format = VK_FORMAT_R32G32B32A32_SFLOAT;

		// Create Image
		VkImageCreateInfo imageCI;
		imageCI.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageCI.pNext = nullptr;
		imageCI.flags = 0;
		imageCI.imageType = VK_IMAGE_TYPE_2D;
		imageCI.format = format;
		imageCI.extent = { m_FrameWidth, m_FrameHeight, 1 };
		imageCI.mipLevels = 1;
		imageCI.arrayLayers = 1;
		imageCI.samples = VK_SAMPLE_COUNT_1_BIT;
		imageCI.tiling = VK_IMAGE_TILING_OPTIMAL;
		imageCI.usage = VK_IMAGE_USAGE_STORAGE_BIT;
		imageCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		imageCI.queueFamilyIndexCount = 0;
		imageCI.pQueueFamilyIndices = nullptr;
		imageCI.initialLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;

		VkResult result = vkCreateImage(device, &imageCI, nullptr, &m_PrimaryRayInfoImage);
		ASSERT_VULKAN(result);

		// Image Memory
		VkMemoryRequirements memoryRequirements;
		vkGetImageMemoryRequirements(device, m_PrimaryRayInfoImage, &memoryRequirements);

		VkMemoryAllocateInfo allocateInfo;
		allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocateInfo.pNext = nullptr;
		allocateInfo.allocationSize = memoryRequirements.size;
		allocateInfo.memoryTypeIndex = VulkanAPI::FindMemoryType(
			memoryRequirements.memoryTypeBits,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		result = vkAllocateMemory(device, &allocateInfo, nullptr, &m_PrimaryRayInfoImageMemory);
		ASSERT_VULKAN(result);

		result = vkBindImageMemory(device, m_PrimaryRayInfoImage, m_PrimaryRayInfoImageMemory, 0);
		ASSERT_VULKAN(result);

		// Create image view
		VkImageViewCreateInfo imageViewCI;
		imageViewCI.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		imageViewCI.pNext = nullptr;
		imageViewCI.flags = 0;
		imageViewCI.image = m_PrimaryRayInfoImage;
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

		result = vkCreateImageView(device, &imageViewCI, nullptr, &m_PrimaryRayInfoImageView);
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
			m_PrimaryRayInfoImage,
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

	void NrcHpmRenderer::CreateNeuralRayTargetImage(VkDevice device)
	{
		VkFormat format = VK_FORMAT_R32G32B32A32_SFLOAT;

		// Create Image
		VkImageCreateInfo imageCI;
		imageCI.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageCI.pNext = nullptr;
		imageCI.flags = 0;
		imageCI.imageType = VK_IMAGE_TYPE_2D;
		imageCI.format = format;
		imageCI.extent = { m_TrainWidth, m_TrainHeight, 1 };
		imageCI.mipLevels = 1;
		imageCI.arrayLayers = 1;
		imageCI.samples = VK_SAMPLE_COUNT_1_BIT;
		imageCI.tiling = VK_IMAGE_TILING_OPTIMAL;
		imageCI.usage = VK_IMAGE_USAGE_STORAGE_BIT;
		imageCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		imageCI.queueFamilyIndexCount = 0;
		imageCI.pQueueFamilyIndices = nullptr;
		imageCI.initialLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;

		VkResult result = vkCreateImage(device, &imageCI, nullptr, &m_NeuralRayTargetImage);
		ASSERT_VULKAN(result);

		// Image Memory
		VkMemoryRequirements memoryRequirements;
		vkGetImageMemoryRequirements(device, m_NeuralRayTargetImage, &memoryRequirements);

		VkMemoryAllocateInfo allocateInfo;
		allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocateInfo.pNext = nullptr;
		allocateInfo.allocationSize = memoryRequirements.size;
		allocateInfo.memoryTypeIndex = VulkanAPI::FindMemoryType(
			memoryRequirements.memoryTypeBits,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		result = vkAllocateMemory(device, &allocateInfo, nullptr, &m_NeuralRayTargetImageMemory);
		ASSERT_VULKAN(result);

		result = vkBindImageMemory(device, m_NeuralRayTargetImage, m_NeuralRayTargetImageMemory, 0);
		ASSERT_VULKAN(result);

		// Create image view
		VkImageViewCreateInfo imageViewCI;
		imageViewCI.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		imageViewCI.pNext = nullptr;
		imageViewCI.flags = 0;
		imageViewCI.image = m_NeuralRayTargetImage;
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

		result = vkCreateImageView(device, &imageViewCI, nullptr, &m_NeuralRayTargetImageView);
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
			m_NeuralRayTargetImage,
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

	void NrcHpmRenderer::AllocateAndUpdateDescriptorSet(VkDevice device)
	{
		// Allocate
		VkDescriptorSetAllocateInfo descSetAI;
		descSetAI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		descSetAI.pNext = nullptr;
		descSetAI.descriptorPool = m_DescPool;
		descSetAI.descriptorSetCount = 1;
		descSetAI.pSetLayouts = &m_DescSetLayout;

		VkResult result = vkAllocateDescriptorSets(device, &descSetAI, &m_DescSet);
		ASSERT_VULKAN(result);

		// Write
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

		VkDescriptorImageInfo primaryRayColorImageInfo;
		primaryRayColorImageInfo.sampler = VK_NULL_HANDLE;
		primaryRayColorImageInfo.imageView = m_PrimaryRayColorImageView;
		primaryRayColorImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

		VkWriteDescriptorSet primaryRayColorImageWrite;
		primaryRayColorImageWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		primaryRayColorImageWrite.pNext = nullptr;
		primaryRayColorImageWrite.dstSet = m_DescSet;
		primaryRayColorImageWrite.dstBinding = 1;
		primaryRayColorImageWrite.dstArrayElement = 0;
		primaryRayColorImageWrite.descriptorCount = 1;
		primaryRayColorImageWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		primaryRayColorImageWrite.pImageInfo = &primaryRayColorImageInfo;
		primaryRayColorImageWrite.pBufferInfo = nullptr;
		primaryRayColorImageWrite.pTexelBufferView = nullptr;

		VkDescriptorImageInfo primaryRayInfoImageInfo;
		primaryRayInfoImageInfo.sampler = VK_NULL_HANDLE;
		primaryRayInfoImageInfo.imageView = m_PrimaryRayInfoImageView;
		primaryRayInfoImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

		VkWriteDescriptorSet primaryRayInfoImageWrite;
		primaryRayInfoImageWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		primaryRayInfoImageWrite.pNext = nullptr;
		primaryRayInfoImageWrite.dstSet = m_DescSet;
		primaryRayInfoImageWrite.dstBinding = 2;
		primaryRayInfoImageWrite.dstArrayElement = 0;
		primaryRayInfoImageWrite.descriptorCount = 1;
		primaryRayInfoImageWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		primaryRayInfoImageWrite.pImageInfo = &primaryRayInfoImageInfo;
		primaryRayInfoImageWrite.pBufferInfo = nullptr;
		primaryRayInfoImageWrite.pTexelBufferView = nullptr;

		VkDescriptorImageInfo neuralRayTargetImageInfo;
		neuralRayTargetImageInfo.sampler = VK_NULL_HANDLE;
		neuralRayTargetImageInfo.imageView = m_NeuralRayTargetImageView;
		neuralRayTargetImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

		VkWriteDescriptorSet neuralRayTargetImageWrite;
		neuralRayTargetImageWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		neuralRayTargetImageWrite.pNext = nullptr;
		neuralRayTargetImageWrite.dstSet = m_DescSet;
		neuralRayTargetImageWrite.dstBinding = 6;
		neuralRayTargetImageWrite.dstArrayElement = 0;
		neuralRayTargetImageWrite.descriptorCount = 1;
		neuralRayTargetImageWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		neuralRayTargetImageWrite.pImageInfo = &neuralRayTargetImageInfo;
		neuralRayTargetImageWrite.pBufferInfo = nullptr;
		neuralRayTargetImageWrite.pTexelBufferView = nullptr;

		std::vector<VkWriteDescriptorSet> writes = { 
			outputImageWrite,
			primaryRayColorImageWrite,
			primaryRayInfoImageWrite,
			neuralRayTargetImageWrite };

		vkUpdateDescriptorSets(device, writes.size(), writes.data(), 0, nullptr);
	}

	void NrcHpmRenderer::RecordCommandBuffer()
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

		// Gen rays pipeline
		vkCmdBindPipeline(m_CommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_GenRaysPipeline);
		vkCmdDispatch(m_CommandBuffer, m_FrameWidth / 32, m_FrameHeight, 1);

		vkCmdPipelineBarrier(
			m_CommandBuffer,
			VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
			VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
			VK_DEPENDENCY_DEVICE_GROUP_BIT,
			1, &memoryBarrier,
			0, nullptr,
			0, nullptr);

		// Calc nrc targets pipeline
		vkCmdBindPipeline(m_CommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_CalcNrcTargetsPipeline);
		vkCmdDispatch(m_CommandBuffer, 1, 1, 1); // TODO

		vkCmdPipelineBarrier(
			m_CommandBuffer,
			VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
			VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
			VK_DEPENDENCY_DEVICE_GROUP_BIT,
			1, &memoryBarrier,
			0, nullptr,
			0, nullptr);

		// Render pipeline
		vkCmdBindPipeline(m_CommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_RenderPipeline);
		vkCmdDispatch(m_CommandBuffer, m_FrameWidth / 32, m_FrameHeight, 1);

		// End
		result = vkEndCommandBuffer(m_CommandBuffer);
		if (result != VK_SUCCESS)
			Log::Error("Failed to end VkCommandBuffer", true);
	}
}
