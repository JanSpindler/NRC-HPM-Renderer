#define VK_USE_PLATFORM_WIN32_KHR

#include <aclapi.h>
#include <dxgi1_2.h>
#include <windows.h>
#include <VersionHelpers.h>

#include <engine/graphics/NeuralRadianceCache.hpp>
#include <engine/graphics/renderer/NrcHpmRenderer.hpp>
#include <engine/util/Log.hpp>
#include <engine/graphics/vulkan/CommandRecorder.hpp>
#include <glm/gtc/random.hpp>
#include <imgui.h>

#define TINYEXR_IMPLEMENTATION
#include <tinyexr.h>

#define ASSERT_CUDA(error) if (error != cudaSuccess) { en::Log::Error("Cuda assert triggered: " + std::string(cudaGetErrorName(error)), true); }

namespace en
{
	VkDescriptorSetLayout NrcHpmRenderer::m_DescSetLayout;
	VkDescriptorPool NrcHpmRenderer::m_DescPool;

	PFN_vkGetSemaphoreWin32HandleKHR fpGetSemaphoreWin32HandleKHR = nullptr;

	HANDLE GetSemaphoreHandle(VkDevice device, VkSemaphore vkSemaphore)
	{
		if (fpGetSemaphoreWin32HandleKHR == nullptr)
		{
			fpGetSemaphoreWin32HandleKHR = (PFN_vkGetSemaphoreWin32HandleKHR)vkGetDeviceProcAddr(device, "vkGetSemaphoreWin32HandleKHR");
		}

		VkSemaphoreGetWin32HandleInfoKHR vulkanSemaphoreGetWin32HandleInfoKHR = {};
		vulkanSemaphoreGetWin32HandleInfoKHR.sType = VK_STRUCTURE_TYPE_SEMAPHORE_GET_WIN32_HANDLE_INFO_KHR;
		vulkanSemaphoreGetWin32HandleInfoKHR.pNext = NULL;
		vulkanSemaphoreGetWin32HandleInfoKHR.semaphore = vkSemaphore;
		vulkanSemaphoreGetWin32HandleInfoKHR.handleType = VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_WIN32_BIT;

		HANDLE handle;
		fpGetSemaphoreWin32HandleKHR(device, &vulkanSemaphoreGetWin32HandleInfoKHR, &handle);
		return handle;
	}

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

		VkDescriptorSetLayoutBinding nrcRayOriginImageBinding;
		nrcRayOriginImageBinding.binding = 3;
		nrcRayOriginImageBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		nrcRayOriginImageBinding.descriptorCount = 1;
		nrcRayOriginImageBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
		nrcRayOriginImageBinding.pImmutableSamplers = nullptr;

		VkDescriptorSetLayoutBinding nrcRayDirImageBinding;
		nrcRayDirImageBinding.binding = 4;
		nrcRayDirImageBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		nrcRayDirImageBinding.descriptorCount = 1;
		nrcRayDirImageBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
		nrcRayDirImageBinding.pImmutableSamplers = nullptr;

		VkDescriptorSetLayoutBinding nrcInferInputBufferBinding;
		nrcInferInputBufferBinding.binding = 5;
		nrcInferInputBufferBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		nrcInferInputBufferBinding.descriptorCount = 1;
		nrcInferInputBufferBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
		nrcInferInputBufferBinding.pImmutableSamplers = nullptr;

		VkDescriptorSetLayoutBinding nrcInferOutputBufferBinding;
		nrcInferOutputBufferBinding.binding = 6;
		nrcInferOutputBufferBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		nrcInferOutputBufferBinding.descriptorCount = 1;
		nrcInferOutputBufferBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
		nrcInferOutputBufferBinding.pImmutableSamplers = nullptr;

		VkDescriptorSetLayoutBinding nrcTrainInputBufferBinding;
		nrcTrainInputBufferBinding.binding = 7;
		nrcTrainInputBufferBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		nrcTrainInputBufferBinding.descriptorCount = 1;
		nrcTrainInputBufferBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
		nrcTrainInputBufferBinding.pImmutableSamplers = nullptr;

		VkDescriptorSetLayoutBinding nrcTrainTargetBufferBinding;
		nrcTrainTargetBufferBinding.binding = 8;
		nrcTrainTargetBufferBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		nrcTrainTargetBufferBinding.descriptorCount = 1;
		nrcTrainTargetBufferBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
		nrcTrainTargetBufferBinding.pImmutableSamplers = nullptr;

		VkDescriptorSetLayoutBinding uniformBufferBinding;
		uniformBufferBinding.binding = 9;
		uniformBufferBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		uniformBufferBinding.descriptorCount = 1;
		uniformBufferBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
		uniformBufferBinding.pImmutableSamplers = nullptr;

		std::vector<VkDescriptorSetLayoutBinding> bindings = { 
			outputImageBinding,
			primaryRayColorImageBinding,
			primaryRayInfoImageBinding,
			nrcRayOriginImageBinding,
			nrcRayDirImageBinding,
			nrcInferInputBufferBinding,
			nrcInferOutputBufferBinding,
			nrcTrainInputBufferBinding,
			nrcTrainTargetBufferBinding,
			uniformBufferBinding
		};

		VkDescriptorSetLayoutCreateInfo layoutCI;
		layoutCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutCI.pNext = nullptr;
		layoutCI.flags = 0;
		layoutCI.bindingCount = bindings.size();
		layoutCI.pBindings = bindings.data();

		VkResult result = vkCreateDescriptorSetLayout(device, &layoutCI, nullptr, &m_DescSetLayout);
		ASSERT_VULKAN(result);

		// Create desc pool
		VkDescriptorPoolSize storageImagePS;
		storageImagePS.type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		storageImagePS.descriptorCount = 5;

		VkDescriptorPoolSize storageBufferPS;
		storageBufferPS.type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		storageBufferPS.descriptorCount = 4;

		VkDescriptorPoolSize uniformBufferPS;
		uniformBufferPS.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		uniformBufferPS.descriptorCount = 1;

		std::vector<VkDescriptorPoolSize> poolSizes = { storageImagePS, storageBufferPS, uniformBufferPS };

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
		float trainSampleRatio,
		uint32_t trainSpp,
		const Camera& camera,
		const HpmScene& hpmScene,
		NeuralRadianceCache& nrc)
		:
		m_RenderWidth(width),
		m_RenderHeight(height),
		m_TrainSpp(trainSpp),
		m_GenRaysShader("nrc/gen_rays.comp", false),
		m_PrepRayInfoShader("nrc/prep_ray_info.comp", false),
		m_PrepTrainRaysShader("nrc/prep_train_rays.comp", false),
		m_RenderShader("nrc/render.comp", false),
		m_CommandPool(VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, VulkanAPI::GetGraphicsQFI()),
		m_Camera(camera),
		m_HpmScene(hpmScene),
		m_Nrc(nrc),
		m_UniformBuffer(
			sizeof(UniformData), 
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, 
			{})
	{
		// Calc train sample extent
		float sqrtTrainSampleRatio = std::sqrt(trainSampleRatio);
		m_TrainWidth = sqrtTrainSampleRatio * m_RenderWidth;
		m_TrainHeight = sqrtTrainSampleRatio * m_RenderHeight;
		m_TrainWidth -= m_TrainWidth % 16;
		m_TrainHeight -= m_TrainHeight % 16;

		// Init components
		VkDevice device = VulkanAPI::GetDevice();

		CreateSyncObjects(device);

		CreateNrcBuffers();

		m_Nrc.Init(
			m_RenderWidth * m_RenderHeight, 
			m_TrainWidth * m_TrainHeight,
			reinterpret_cast<float*>(m_NrcInferInputDCuBuffer),
			reinterpret_cast<float*>(m_NrcInferOutputDCuBuffer),
			reinterpret_cast<float*>(m_NrcTrainInputDCuBuffer),
			reinterpret_cast<float*>(m_NrcTrainTargetDCuBuffer),
			m_CuExtCudaStartSemaphore, 
			m_CuExtCudaFinishedSemaphore);

		m_CommandPool.AllocateBuffers(3, VK_COMMAND_BUFFER_LEVEL_PRIMARY);
		m_PreCudaCommandBuffer = m_CommandPool.GetBuffer(0);
		m_PostCudaCommandBuffer = m_CommandPool.GetBuffer(1);
		m_RandomTasksCmdBuf = m_CommandPool.GetBuffer(2);

		CreatePipelineLayout(device);

		InitSpecializationConstants();

		CreateGenRaysPipeline(device);
		CreatePrepRayInfoPipeline(device);
		CreatePrepTrainRaysPipeline(device);
		CreateRenderPipeline(device);

		CreateOutputImage(device);
		CreatePrimaryRayColorImage(device);
		CreatePrimaryRayInfoImage(device);
		CreateNrcRayOriginImage(device);
		CreateNrcRayDirImage(device);

		AllocateAndUpdateDescriptorSet(device);

		CreateQueryPool(device);

		RecordPreCudaCommandBuffer();
		RecordPostCudaCommandBuffer();
	}

	void NrcHpmRenderer::Render(VkQueue queue)
	{
		// Generate random
		m_UniformData.random = glm::linearRand(glm::vec4(0.0f), glm::vec4(1.0f));
		m_UniformBuffer.SetData(sizeof(UniformData), &m_UniformData, 0, 0);

		// Pre cuda
		VkSubmitInfo submitInfo;
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.pNext = nullptr;
		submitInfo.waitSemaphoreCount = 0;
		submitInfo.pWaitSemaphores = nullptr;
		submitInfo.pWaitDstStageMask = nullptr;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &m_PreCudaCommandBuffer;
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = &m_CudaStartSemaphore;

		VkResult result = vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
		ASSERT_VULKAN(result);

		// Cuda
		m_Nrc.InferAndTrain();
		
		// Post cuda
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.pNext = nullptr;
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = &m_CudaFinishedSemaphore;
		VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
		submitInfo.pWaitDstStageMask = &waitStage;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &m_PostCudaCommandBuffer;
		submitInfo.signalSemaphoreCount = 0;
		submitInfo.pSignalSemaphores = nullptr;

		result = vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
		ASSERT_VULKAN(result);
	}

	void NrcHpmRenderer::Destroy()
	{
		VkDevice device = VulkanAPI::GetDevice();

		m_CommandPool.Destroy();

		m_UniformBuffer.Destroy();

		vkDestroyQueryPool(device, m_QueryPool, nullptr);

		vkDestroyImageView(device, m_NrcRayDirImageView, nullptr);
		vkFreeMemory(device, m_NrcRayDirImageMemory, nullptr);
		vkDestroyImage(device, m_NrcRayDirImage, nullptr);

		vkDestroyImageView(device, m_NrcRayOriginImageView, nullptr);
		vkFreeMemory(device, m_NrcRayOriginImageMemory, nullptr);
		vkDestroyImage(device, m_NrcRayOriginImage, nullptr);

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
		
		vkDestroyPipeline(device, m_PrepTrainRaysPipeline, nullptr);
		m_PrepTrainRaysShader.Destroy();

		vkDestroyPipeline(device, m_PrepRayInfoPipeline, nullptr);
		m_PrepRayInfoShader.Destroy();

		vkDestroyPipeline(device, m_GenRaysPipeline, nullptr);
		m_GenRaysShader.Destroy();

		vkDestroyPipelineLayout(device, m_PipelineLayout, nullptr);
	
		m_NrcTrainTargetBuffer->Destroy();
		delete m_NrcTrainTargetBuffer;
		ASSERT_CUDA(cudaDestroyExternalMemory(m_NrcTrainTargetCuExtMem));

		m_NrcTrainInputBuffer->Destroy();
		delete m_NrcTrainInputBuffer;
		ASSERT_CUDA(cudaDestroyExternalMemory(m_NrcTrainInputCuExtMem));

		m_NrcInferOutputBuffer->Destroy();
		delete m_NrcInferOutputBuffer;
		ASSERT_CUDA(cudaDestroyExternalMemory(m_NrcInferOutputCuExtMem));

		m_NrcInferInputBuffer->Destroy();
		delete m_NrcInferInputBuffer;
		ASSERT_CUDA(cudaDestroyExternalMemory(m_NrcInferInputCuExtMem));

		vkDestroySemaphore(device, m_CudaFinishedSemaphore, nullptr);
		ASSERT_CUDA(cudaDestroyExternalSemaphore(m_CuExtCudaFinishedSemaphore));
		
		vkDestroySemaphore(device, m_CudaStartSemaphore, nullptr);
		ASSERT_CUDA(cudaDestroyExternalSemaphore(m_CuExtCudaStartSemaphore));
	}

	void NrcHpmRenderer::ExportImageToFile(VkQueue queue, const std::string& filePath) const
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

	void NrcHpmRenderer::EvaluateTimestampQueries()
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

		for (size_t i = 0; i < c_QueryCount - 1; i++)
		{
			m_TimePeriods[i] = c_TimestampPeriodInMS * static_cast<float>(queryResults[i + 1] - queryResults[i]);
		}
		m_TimePeriods[c_QueryCount - 1] = c_TimestampPeriodInMS * static_cast<float>(queryResults[c_QueryCount - 1] - queryResults[0]);
	}

	void NrcHpmRenderer::RenderImGui() const
	{
		ImGui::Begin("NrcHpmRenderer stats");
		
		size_t periodIndex = 0;
		ImGui::Text("GenRays Time %f ms", m_TimePeriods[periodIndex++]);
		ImGui::Text("PrepRayInfo Time %f ms", m_TimePeriods[periodIndex++]);
		ImGui::Text("PrepTrainRays Time %f ms", m_TimePeriods[periodIndex++]);
		ImGui::Text("Cuda Time %f ms", m_TimePeriods[periodIndex++]);
		ImGui::Text("Render Time %f ms", m_TimePeriods[periodIndex++]);
		ImGui::Text("Total Time %f ms", m_TimePeriods[periodIndex++]);
		ImGui::Text("Theoretical FPS %f", 1000.0f / m_TimePeriods[c_QueryCount - 1]);

		ImGui::End();
	}

	VkImage NrcHpmRenderer::GetImage() const
	{
		return m_OutputImage;
	}

	VkImageView NrcHpmRenderer::GetImageView() const
	{
		return m_OutputImageView;
	}

	void NrcHpmRenderer::CreateSyncObjects(VkDevice device)
	{
		// Create vk semaphore
		VkExportSemaphoreCreateInfoKHR vulkanExportSemaphoreCreateInfo = {};
		vulkanExportSemaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_EXPORT_SEMAPHORE_CREATE_INFO_KHR;
		vulkanExportSemaphoreCreateInfo.pNext = nullptr;
		vulkanExportSemaphoreCreateInfo.handleTypes = VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_WIN32_BIT;

		VkSemaphoreCreateInfo semaphoreCI;
		semaphoreCI.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		semaphoreCI.pNext = &vulkanExportSemaphoreCreateInfo;
		semaphoreCI.flags = 0;

		VkResult result = vkCreateSemaphore(device, &semaphoreCI, nullptr, &m_CudaStartSemaphore);
		ASSERT_VULKAN(result);

		result = vkCreateSemaphore(device, &semaphoreCI, nullptr, &m_CudaFinishedSemaphore);
		ASSERT_VULKAN(result);

		// Export semaphore to cuda
		cudaExternalSemaphoreHandleDesc extCudaSemaphoreHD{};
		extCudaSemaphoreHD.type = cudaExternalSemaphoreHandleTypeOpaqueWin32;

		extCudaSemaphoreHD.handle.win32.handle = GetSemaphoreHandle(device, m_CudaStartSemaphore);
		cudaError_t error = cudaImportExternalSemaphore(&m_CuExtCudaStartSemaphore, &extCudaSemaphoreHD);
		ASSERT_CUDA(error);

		extCudaSemaphoreHD.handle.win32.handle = GetSemaphoreHandle(device, m_CudaFinishedSemaphore);
		error = cudaImportExternalSemaphore(&m_CuExtCudaFinishedSemaphore, &extCudaSemaphoreHD);
		ASSERT_CUDA(error);
	}

	void NrcHpmRenderer::CreateNrcBuffers()
	{
		// Calculate sizes
		m_NrcInferInputBufferSize = m_RenderWidth * m_RenderHeight * 5 * sizeof(float);
		m_NrcInferOutputBufferSize = m_RenderWidth * m_RenderHeight * 3 * sizeof(float);
		m_NrcTrainInputBufferSize = m_TrainWidth * m_TrainHeight * 5 * sizeof(float);
		m_NrcTrainTargetBufferSize = m_TrainWidth * m_TrainHeight * 3 * sizeof(float);

		// Create buffers
		m_NrcInferInputBuffer = new vk::Buffer(
			m_NrcInferInputBufferSize, 
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, 
			{},
			VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT);

		m_NrcInferOutputBuffer = new vk::Buffer(
			m_NrcInferOutputBufferSize,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			{},
			VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT);
		
		m_NrcTrainInputBuffer = new vk::Buffer(
			m_NrcTrainInputBufferSize,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			{},
			VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT);
		
		m_NrcTrainTargetBuffer = new vk::Buffer(
			m_NrcTrainTargetBufferSize,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			{},
			VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT);

		// Get cuda external memory
		cudaExternalMemoryHandleDesc cuExtMemHandleDesc{};
		cuExtMemHandleDesc.type = cudaExternalMemoryHandleTypeOpaqueWin32;

		cuExtMemHandleDesc.handle.win32.handle = m_NrcInferInputBuffer->GetMemoryWin32Handle();
		cuExtMemHandleDesc.size = m_NrcInferInputBufferSize;
		cudaError_t cudaResult = cudaImportExternalMemory(&m_NrcInferInputCuExtMem, &cuExtMemHandleDesc);
		ASSERT_CUDA(cudaResult);

		cuExtMemHandleDesc.handle.win32.handle = m_NrcInferOutputBuffer->GetMemoryWin32Handle();
		cuExtMemHandleDesc.size = m_NrcInferOutputBufferSize;
		cudaResult = cudaImportExternalMemory(&m_NrcInferOutputCuExtMem, &cuExtMemHandleDesc);
		ASSERT_CUDA(cudaResult);

		cuExtMemHandleDesc.handle.win32.handle = m_NrcTrainInputBuffer->GetMemoryWin32Handle();
		cuExtMemHandleDesc.size = m_NrcTrainInputBufferSize;
		cudaResult = cudaImportExternalMemory(&m_NrcTrainInputCuExtMem, &cuExtMemHandleDesc);
		ASSERT_CUDA(cudaResult);

		cuExtMemHandleDesc.handle.win32.handle = m_NrcTrainTargetBuffer->GetMemoryWin32Handle();
		cuExtMemHandleDesc.size = m_NrcTrainTargetBufferSize;
		cudaResult = cudaImportExternalMemory(&m_NrcTrainTargetCuExtMem, &cuExtMemHandleDesc);
		ASSERT_CUDA(cudaResult);

		// Get cuda buffer
		cudaExternalMemoryBufferDesc cudaExtBufferDesc{};
		cudaExtBufferDesc.offset = 0;
		cudaExtBufferDesc.flags = 0;

		cudaExtBufferDesc.size = m_NrcInferInputBufferSize;
		cudaResult = cudaExternalMemoryGetMappedBuffer(&m_NrcInferInputDCuBuffer, m_NrcInferInputCuExtMem, &cudaExtBufferDesc);
		ASSERT_CUDA(cudaResult);

		cudaExtBufferDesc.size = m_NrcInferOutputBufferSize;
		cudaResult = cudaExternalMemoryGetMappedBuffer(&m_NrcInferOutputDCuBuffer, m_NrcInferOutputCuExtMem, &cudaExtBufferDesc);
		ASSERT_CUDA(cudaResult);

		cudaExtBufferDesc.size = m_NrcTrainInputBufferSize;
		cudaResult = cudaExternalMemoryGetMappedBuffer(&m_NrcTrainInputDCuBuffer, m_NrcTrainInputCuExtMem, &cudaExtBufferDesc);
		ASSERT_CUDA(cudaResult);

		cudaExtBufferDesc.size = m_NrcTrainTargetBufferSize;
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
		m_SpecData.renderWidth = m_RenderWidth;
		m_SpecData.renderHeight = m_RenderHeight;
		
		m_SpecData.trainWidth = m_TrainWidth;
		m_SpecData.trainHeight = m_TrainHeight;

		m_SpecData.trainSpp = m_TrainSpp;

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

		VkSpecializationMapEntry trainWidthEntry;
		trainWidthEntry.constantID = 2;
		trainWidthEntry.offset = offsetof(SpecializationData, SpecializationData::trainWidth);
		trainWidthEntry.size = sizeof(uint32_t);

		VkSpecializationMapEntry trainHeightEntry;
		trainHeightEntry.constantID = 3;
		trainHeightEntry.offset = offsetof(SpecializationData, SpecializationData::trainHeight);
		trainHeightEntry.size = sizeof(uint32_t);

		VkSpecializationMapEntry trainSppEntry;
		trainSppEntry.constantID = 4;
		trainSppEntry.offset = offsetof(SpecializationData, SpecializationData::trainSpp);
		trainSppEntry.size = sizeof(uint32_t);

		VkSpecializationMapEntry volumeDensityFactorEntry;
		volumeDensityFactorEntry.constantID = 5;
		volumeDensityFactorEntry.offset = offsetof(SpecializationData, SpecializationData::volumeDensityFactor);
		volumeDensityFactorEntry.size = sizeof(float);

		VkSpecializationMapEntry volumeGEntry;
		volumeGEntry.constantID = 6;
		volumeGEntry.offset = offsetof(SpecializationData, SpecializationData::volumeG);
		volumeGEntry.size = sizeof(float);

		VkSpecializationMapEntry hdrEnvMapStrengthEntry;
		hdrEnvMapStrengthEntry.constantID = 7;
		hdrEnvMapStrengthEntry.offset = offsetof(SpecializationData, SpecializationData::hdrEnvMapStrength);
		hdrEnvMapStrengthEntry.size = sizeof(float);

		m_SpecMapEntries = {
			renderWidthEntry,
			renderHeightEntry,
			trainWidthEntry,
			trainHeightEntry,
			trainSppEntry,
			volumeDensityFactorEntry,
			volumeGEntry,
			hdrEnvMapStrengthEntry
		};

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

	void NrcHpmRenderer::CreatePrepRayInfoPipeline(VkDevice device)
	{
		VkPipelineShaderStageCreateInfo shaderStage;
		shaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shaderStage.pNext = nullptr;
		shaderStage.flags = 0;
		shaderStage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
		shaderStage.module = m_PrepRayInfoShader.GetVulkanModule();
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

		VkResult result = vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &pipelineCI, nullptr, &m_PrepRayInfoPipeline);
		ASSERT_VULKAN(result);
	}

	void NrcHpmRenderer::CreatePrepTrainRaysPipeline(VkDevice device)
	{
		VkPipelineShaderStageCreateInfo shaderStage;
		shaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shaderStage.pNext = nullptr;
		shaderStage.flags = 0;
		shaderStage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
		shaderStage.module = m_PrepTrainRaysShader.GetVulkanModule();
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

		VkResult result = vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &pipelineCI, nullptr, &m_PrepTrainRaysPipeline);
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
		imageCI.extent = { m_RenderWidth, m_RenderHeight, 1 };
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

		result = vkBeginCommandBuffer(m_RandomTasksCmdBuf, &beginInfo);
		ASSERT_VULKAN(result);

		vk::CommandRecorder::ImageLayoutTransfer(
			m_RandomTasksCmdBuf,
			m_PrimaryRayColorImage,
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
		imageCI.extent = { m_RenderWidth, m_RenderHeight, 1 };
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

		result = vkBeginCommandBuffer(m_RandomTasksCmdBuf, &beginInfo);
		ASSERT_VULKAN(result);

		vk::CommandRecorder::ImageLayoutTransfer(
			m_RandomTasksCmdBuf,
			m_PrimaryRayInfoImage,
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

	void NrcHpmRenderer::CreateNrcRayOriginImage(VkDevice device)
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
		imageCI.usage = VK_IMAGE_USAGE_STORAGE_BIT;
		imageCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		imageCI.queueFamilyIndexCount = 0;
		imageCI.pQueueFamilyIndices = nullptr;
		imageCI.initialLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;

		VkResult result = vkCreateImage(device, &imageCI, nullptr, &m_NrcRayOriginImage);
		ASSERT_VULKAN(result);

		// Image Memory
		VkMemoryRequirements memoryRequirements;
		vkGetImageMemoryRequirements(device, m_NrcRayOriginImage, &memoryRequirements);

		VkMemoryAllocateInfo allocateInfo;
		allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocateInfo.pNext = nullptr;
		allocateInfo.allocationSize = memoryRequirements.size;
		allocateInfo.memoryTypeIndex = VulkanAPI::FindMemoryType(
			memoryRequirements.memoryTypeBits,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		result = vkAllocateMemory(device, &allocateInfo, nullptr, &m_NrcRayOriginImageMemory);
		ASSERT_VULKAN(result);

		result = vkBindImageMemory(device, m_NrcRayOriginImage, m_NrcRayOriginImageMemory, 0);
		ASSERT_VULKAN(result);

		// Create image view
		VkImageViewCreateInfo imageViewCI;
		imageViewCI.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		imageViewCI.pNext = nullptr;
		imageViewCI.flags = 0;
		imageViewCI.image = m_NrcRayOriginImage;
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

		result = vkCreateImageView(device, &imageViewCI, nullptr, &m_NrcRayOriginImageView);
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
			m_NrcRayOriginImage,
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

	void NrcHpmRenderer::CreateNrcRayDirImage(VkDevice device)
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
		imageCI.usage = VK_IMAGE_USAGE_STORAGE_BIT;
		imageCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		imageCI.queueFamilyIndexCount = 0;
		imageCI.pQueueFamilyIndices = nullptr;
		imageCI.initialLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;

		VkResult result = vkCreateImage(device, &imageCI, nullptr, &m_NrcRayDirImage);
		ASSERT_VULKAN(result);

		// Image Memory
		VkMemoryRequirements memoryRequirements;
		vkGetImageMemoryRequirements(device, m_NrcRayDirImage, &memoryRequirements);

		VkMemoryAllocateInfo allocateInfo;
		allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocateInfo.pNext = nullptr;
		allocateInfo.allocationSize = memoryRequirements.size;
		allocateInfo.memoryTypeIndex = VulkanAPI::FindMemoryType(
			memoryRequirements.memoryTypeBits,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		result = vkAllocateMemory(device, &allocateInfo, nullptr, &m_NrcRayDirImageMemory);
		ASSERT_VULKAN(result);

		result = vkBindImageMemory(device, m_NrcRayDirImage, m_NrcRayDirImageMemory, 0);
		ASSERT_VULKAN(result);

		// Create image view
		VkImageViewCreateInfo imageViewCI;
		imageViewCI.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		imageViewCI.pNext = nullptr;
		imageViewCI.flags = 0;
		imageViewCI.image = m_NrcRayDirImage;
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

		result = vkCreateImageView(device, &imageViewCI, nullptr, &m_NrcRayDirImageView);
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
			m_NrcRayDirImage,
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

		VkDescriptorImageInfo nrcRayOriginImageInfo;
		nrcRayOriginImageInfo.sampler = VK_NULL_HANDLE;
		nrcRayOriginImageInfo.imageView = m_NrcRayOriginImageView;
		nrcRayOriginImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

		VkWriteDescriptorSet nrcRayOriginImageWrite;
		nrcRayOriginImageWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		nrcRayOriginImageWrite.pNext = nullptr;
		nrcRayOriginImageWrite.dstSet = m_DescSet;
		nrcRayOriginImageWrite.dstBinding = 3;
		nrcRayOriginImageWrite.dstArrayElement = 0;
		nrcRayOriginImageWrite.descriptorCount = 1;
		nrcRayOriginImageWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		nrcRayOriginImageWrite.pImageInfo = &nrcRayOriginImageInfo;
		nrcRayOriginImageWrite.pBufferInfo = nullptr;
		nrcRayOriginImageWrite.pTexelBufferView = nullptr;

		VkDescriptorImageInfo nrcRayDirImageInfo;
		nrcRayDirImageInfo.sampler = VK_NULL_HANDLE;
		nrcRayDirImageInfo.imageView = m_NrcRayDirImageView;
		nrcRayDirImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

		VkWriteDescriptorSet nrcRayDirImageWrite;
		nrcRayDirImageWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		nrcRayDirImageWrite.pNext = nullptr;
		nrcRayDirImageWrite.dstSet = m_DescSet;
		nrcRayDirImageWrite.dstBinding = 4;
		nrcRayDirImageWrite.dstArrayElement = 0;
		nrcRayDirImageWrite.descriptorCount = 1;
		nrcRayDirImageWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		nrcRayDirImageWrite.pImageInfo = &nrcRayDirImageInfo;
		nrcRayDirImageWrite.pBufferInfo = nullptr;
		nrcRayDirImageWrite.pTexelBufferView = nullptr;

		// Storage buffer writes
		VkDescriptorBufferInfo nrcInferInputBufferInfo;
		nrcInferInputBufferInfo.buffer = m_NrcInferInputBuffer->GetVulkanHandle();
		nrcInferInputBufferInfo.offset = 0;
		nrcInferInputBufferInfo.range = m_NrcInferInputBufferSize;

		VkWriteDescriptorSet nrcInferInputBufferWrite;
		nrcInferInputBufferWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		nrcInferInputBufferWrite.pNext = nullptr;
		nrcInferInputBufferWrite.dstSet = m_DescSet;
		nrcInferInputBufferWrite.dstBinding = 5;
		nrcInferInputBufferWrite.dstArrayElement = 0;
		nrcInferInputBufferWrite.descriptorCount = 1;
		nrcInferInputBufferWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		nrcInferInputBufferWrite.pImageInfo = nullptr;
		nrcInferInputBufferWrite.pBufferInfo = &nrcInferInputBufferInfo;
		nrcInferInputBufferWrite.pTexelBufferView = nullptr;

		VkDescriptorBufferInfo nrcInferOutputBufferInfo;
		nrcInferOutputBufferInfo.buffer = m_NrcInferOutputBuffer->GetVulkanHandle();
		nrcInferOutputBufferInfo.offset = 0;
		nrcInferOutputBufferInfo.range = m_NrcInferOutputBufferSize;

		VkWriteDescriptorSet nrcInferOutputBufferWrite;
		nrcInferOutputBufferWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		nrcInferOutputBufferWrite.pNext = nullptr;
		nrcInferOutputBufferWrite.dstSet = m_DescSet;
		nrcInferOutputBufferWrite.dstBinding = 6;
		nrcInferOutputBufferWrite.dstArrayElement = 0;
		nrcInferOutputBufferWrite.descriptorCount = 1;
		nrcInferOutputBufferWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		nrcInferOutputBufferWrite.pImageInfo = nullptr;
		nrcInferOutputBufferWrite.pBufferInfo = &nrcInferOutputBufferInfo;
		nrcInferOutputBufferWrite.pTexelBufferView = nullptr;

		VkDescriptorBufferInfo nrcTrainInputBufferInfo;
		nrcTrainInputBufferInfo.buffer = m_NrcTrainInputBuffer->GetVulkanHandle();
		nrcTrainInputBufferInfo.offset = 0;
		nrcTrainInputBufferInfo.range = m_NrcTrainInputBufferSize;

		VkWriteDescriptorSet nrcTrainInputBufferWrite;
		nrcTrainInputBufferWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		nrcTrainInputBufferWrite.pNext = nullptr;
		nrcTrainInputBufferWrite.dstSet = m_DescSet;
		nrcTrainInputBufferWrite.dstBinding = 7;
		nrcTrainInputBufferWrite.dstArrayElement = 0;
		nrcTrainInputBufferWrite.descriptorCount = 1;
		nrcTrainInputBufferWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		nrcTrainInputBufferWrite.pImageInfo = nullptr;
		nrcTrainInputBufferWrite.pBufferInfo = &nrcTrainInputBufferInfo;
		nrcTrainInputBufferWrite.pTexelBufferView = nullptr;

		VkDescriptorBufferInfo nrcTrainTargetBufferInfo;
		nrcTrainTargetBufferInfo.buffer = m_NrcTrainTargetBuffer->GetVulkanHandle();
		nrcTrainTargetBufferInfo.offset = 0;
		nrcTrainTargetBufferInfo.range = m_NrcTrainTargetBufferSize;

		VkWriteDescriptorSet nrcTrainTargetBufferWrite;
		nrcTrainTargetBufferWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		nrcTrainTargetBufferWrite.pNext = nullptr;
		nrcTrainTargetBufferWrite.dstSet = m_DescSet;
		nrcTrainTargetBufferWrite.dstBinding = 8;
		nrcTrainTargetBufferWrite.dstArrayElement = 0;
		nrcTrainTargetBufferWrite.descriptorCount = 1;
		nrcTrainTargetBufferWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		nrcTrainTargetBufferWrite.pImageInfo = nullptr;
		nrcTrainTargetBufferWrite.pBufferInfo = &nrcTrainTargetBufferInfo;
		nrcTrainTargetBufferWrite.pTexelBufferView = nullptr;

		// Uniform buffer write
		VkDescriptorBufferInfo uniformBufferInfo;
		uniformBufferInfo.buffer = m_UniformBuffer.GetVulkanHandle();
		uniformBufferInfo.offset = 0;
		uniformBufferInfo.range = sizeof(UniformData);

		VkWriteDescriptorSet uniformBufferWrite;
		uniformBufferWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		uniformBufferWrite.pNext = nullptr;
		uniformBufferWrite.dstSet = m_DescSet;
		uniformBufferWrite.dstBinding = 9;
		uniformBufferWrite.dstArrayElement = 0;
		uniformBufferWrite.descriptorCount = 1;
		uniformBufferWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		uniformBufferWrite.pImageInfo = nullptr;
		uniformBufferWrite.pBufferInfo = &uniformBufferInfo;
		uniformBufferWrite.pTexelBufferView = nullptr;

		// Write writes
		std::vector<VkWriteDescriptorSet> writes = { 
			outputImageWrite,
			primaryRayColorImageWrite,
			primaryRayInfoImageWrite,
			nrcRayOriginImageWrite,
			nrcRayDirImageWrite,
			nrcInferInputBufferWrite,
			nrcInferOutputBufferWrite,
			nrcTrainInputBufferWrite,
			nrcTrainTargetBufferWrite,
			uniformBufferWrite
		};

		vkUpdateDescriptorSets(device, writes.size(), writes.data(), 0, nullptr);
	}

	void NrcHpmRenderer::CreateQueryPool(VkDevice device)
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

	void NrcHpmRenderer::RecordPreCudaCommandBuffer()
	{
		m_QueryIndex = 0;

		// Begin
		VkCommandBufferBeginInfo beginInfo;
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.pNext = nullptr;
		beginInfo.flags = 0;
		beginInfo.pInheritanceInfo = nullptr;

		VkResult result = vkBeginCommandBuffer(m_PreCudaCommandBuffer, &beginInfo);
		ASSERT_VULKAN(result);

		// Reset query pool
		vkCmdResetQueryPool(m_PreCudaCommandBuffer, m_QueryPool, 0, c_QueryCount);

		// Collect descriptor sets
		std::vector<VkDescriptorSet> descSets = { m_Camera.GetDescriptorSet() };
		const std::vector<VkDescriptorSet>& hpmSceneDescSets = m_HpmScene.GetDescriptorSets();
		descSets.insert(descSets.end(), hpmSceneDescSets.begin(), hpmSceneDescSets.end());
		descSets.push_back(m_DescSet);

		// Create memory barrier
		VkMemoryBarrier memoryBarrier;
		memoryBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
		memoryBarrier.pNext = nullptr;
		memoryBarrier.srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
		memoryBarrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;

		// Bind descriptor sets
		vkCmdBindDescriptorSets(
			m_PreCudaCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_PipelineLayout,
			0, descSets.size(), descSets.data(),
			0, nullptr);

		// Timestamp
		vkCmdWriteTimestamp(m_PreCudaCommandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, m_QueryPool, m_QueryIndex++);

		// Gen rays pipeline
		vkCmdBindPipeline(m_PreCudaCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_GenRaysPipeline);
		vkCmdDispatch(m_PreCudaCommandBuffer, m_RenderWidth / 32, m_RenderHeight, 1);

		vkCmdPipelineBarrier(
			m_PreCudaCommandBuffer,
			VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
			VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
			VK_DEPENDENCY_DEVICE_GROUP_BIT,
			1, &memoryBarrier,
			0, nullptr,
			0, nullptr);

		// Timestamp
		vkCmdWriteTimestamp(m_PreCudaCommandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, m_QueryPool, m_QueryIndex++);

		// Prep ray info
		vkCmdBindPipeline(m_PreCudaCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_PrepRayInfoPipeline);
		vkCmdDispatch(m_PreCudaCommandBuffer, m_RenderWidth / 32, m_RenderHeight, 1);

		vkCmdPipelineBarrier(
			m_PreCudaCommandBuffer,
			VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
			VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
			VK_DEPENDENCY_DEVICE_GROUP_BIT,
			1, &memoryBarrier,
			0, nullptr,
			0, nullptr);

		// Timestamp
		vkCmdWriteTimestamp(m_PreCudaCommandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, m_QueryPool, m_QueryIndex++);

		// Prep train rays
		vkCmdBindPipeline(m_PreCudaCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_PrepTrainRaysPipeline);
		vkCmdDispatch(m_PreCudaCommandBuffer, m_TrainWidth / 32, m_TrainHeight, 1);

		// Timestamp
		vkCmdWriteTimestamp(m_PreCudaCommandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, m_QueryPool, m_QueryIndex++);

		// End
		result = vkEndCommandBuffer(m_PreCudaCommandBuffer);
		ASSERT_VULKAN(result);
	}

	void NrcHpmRenderer::RecordPostCudaCommandBuffer()
	{
		// Begin command buffer
		VkCommandBufferBeginInfo beginInfo;
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.pNext = nullptr;
		beginInfo.flags = 0;
		beginInfo.pInheritanceInfo = nullptr;

		VkResult result = vkBeginCommandBuffer(m_PostCudaCommandBuffer, &beginInfo);
		ASSERT_VULKAN(result);

		// Collect descriptor sets
		std::vector<VkDescriptorSet> descSets = { m_Camera.GetDescriptorSet() };
		const std::vector<VkDescriptorSet>& hpmSceneDescSets = m_HpmScene.GetDescriptorSets();
		descSets.insert(descSets.end(), hpmSceneDescSets.begin(), hpmSceneDescSets.end());
		descSets.push_back(m_DescSet);

		// Bind descriptor sets
		vkCmdBindDescriptorSets(
			m_PostCudaCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_PipelineLayout,
			0, descSets.size(), descSets.data(),
			0, nullptr);

		// Timestamp
		vkCmdWriteTimestamp(m_PostCudaCommandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, m_QueryPool, m_QueryIndex++);

		// Render pipeline
		vkCmdBindPipeline(m_PostCudaCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_RenderPipeline);
		vkCmdDispatch(m_PostCudaCommandBuffer, m_RenderWidth / 32, m_RenderHeight, 1);

		// Timestamp
		vkCmdWriteTimestamp(m_PostCudaCommandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, m_QueryPool, m_QueryIndex++);

		// End command buffer
		result = vkEndCommandBuffer(m_PostCudaCommandBuffer);
		ASSERT_VULKAN(result);
	}
}
