#include <cuda_main.hpp>

#define VK_USE_PLATFORM_WIN32_KHR

#include <aclapi.h>
#include <dxgi1_2.h>
#include <windows.h>
#include <VersionHelpers.h>

#include <engine/util/Log.hpp>
#include <engine/graphics/Window.hpp>
#include <engine/graphics/VulkanAPI.hpp>
#include <engine/graphics/vulkan/CommandPool.hpp>
#include <engine/graphics/vulkan/CommandRecorder.hpp>
#include <engine/graphics/renderer/ImGuiRenderer.hpp>
#include <engine/graphics/vulkan/Swapchain.hpp>
#include <imgui.h>
#include <glm/glm.hpp>
#include <glm/gtc/random.hpp>

#include <tiny-cuda-nn/config.h>
#include <vulkan/vulkan.h>

#define ASSERT_CUDA(error) if (error != cudaSuccess) { en::Log::Error("Cuda assert triggered: " + std::string(cudaGetErrorName(error)), true); }

PFN_vkGetMemoryWin32HandleKHR fpGetMemoryWin32HandleKHR;
PFN_vkGetSemaphoreWin32HandleKHR fpGetSemaphoreWin32HandleKHR;

en::vk::CommandPool* commandPool;
VkCommandBuffer commandBuffer;

//VkDescriptorPool descPool;
//VkDescriptorSetLayout descSetLayout;
//VkDescriptorSet descSet;

VkExternalMemoryHandleTypeFlagBits externalMemoryHandleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT;
VkImage image;
VkDeviceMemory imageMemory;
VkImageView imageView;
VkSampler sampler;

VkExternalSemaphoreHandleTypeFlagBitsKHR externalSemaphoreHandleType = VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_WIN32_BIT;
VkSemaphore vkCudaStartSemaphore;
VkSemaphore vkCudaFinishedSemaphore;
cudaExternalSemaphore_t cuCudaStartSemaphore;
cudaExternalSemaphore_t cuCudaFinishedSemaphore;

cudaStream_t streamToRun = 0;

HANDLE GetImageMemoryHandle(VkDevice device)
{
	HANDLE handle;

	VkMemoryGetWin32HandleInfoKHR vkMemoryGetWin32HandleInfoKHR = {};
	vkMemoryGetWin32HandleInfoKHR.sType = VK_STRUCTURE_TYPE_MEMORY_GET_WIN32_HANDLE_INFO_KHR;
	vkMemoryGetWin32HandleInfoKHR.pNext = NULL;
	vkMemoryGetWin32HandleInfoKHR.memory = imageMemory;
	vkMemoryGetWin32HandleInfoKHR.handleType = externalMemoryHandleType;

	fpGetMemoryWin32HandleKHR(device, &vkMemoryGetWin32HandleInfoKHR, &handle);
	return handle;
}

HANDLE GetSemaphoreHandle(VkDevice device, VkSemaphore vkSemaphore)
{
	HANDLE handle;

	VkSemaphoreGetWin32HandleInfoKHR vulkanSemaphoreGetWin32HandleInfoKHR = {};
	vulkanSemaphoreGetWin32HandleInfoKHR.sType = VK_STRUCTURE_TYPE_SEMAPHORE_GET_WIN32_HANDLE_INFO_KHR;
	vulkanSemaphoreGetWin32HandleInfoKHR.pNext = NULL;
	vulkanSemaphoreGetWin32HandleInfoKHR.semaphore = vkSemaphore;
	vulkanSemaphoreGetWin32HandleInfoKHR.handleType = externalSemaphoreHandleType;

	fpGetSemaphoreWin32HandleKHR(device, &vulkanSemaphoreGetWin32HandleInfoKHR, &handle);
	return handle;
}

void LoadVulkanProcAddr()
{
	fpGetMemoryWin32HandleKHR = (PFN_vkGetMemoryWin32HandleKHR)vkGetInstanceProcAddr(
		en::VulkanAPI::GetInstance(), 
		"vkGetMemoryWin32HandleKHR");

	fpGetSemaphoreWin32HandleKHR = (PFN_vkGetSemaphoreWin32HandleKHR)vkGetDeviceProcAddr(
		en::VulkanAPI::GetDevice(),
		"vkGetSemaphoreWin32HandleKHR");
}

void CreateCommandBuffer(uint32_t qfi)
{
	commandPool = new en::vk::CommandPool(VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, qfi);
	commandPool->AllocateBuffers(1, VK_COMMAND_BUFFER_LEVEL_PRIMARY);
	commandBuffer = commandPool->GetBuffer(0);
}

void CreateImage(VkDevice device, VkQueue queue, uint32_t width, uint32_t height)
{
	VkFormat format = VK_FORMAT_R32G32B32A32_SFLOAT;

	// Create Image
	VkExternalMemoryImageCreateInfo vkExternalMemImageCreateInfo = {};
	vkExternalMemImageCreateInfo.sType = VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO;
	vkExternalMemImageCreateInfo.pNext = nullptr;
	vkExternalMemImageCreateInfo.handleTypes = externalMemoryHandleType;

	VkImageCreateInfo imageCI;
	imageCI.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageCI.pNext = &vkExternalMemImageCreateInfo;
	imageCI.flags = 0;
	imageCI.imageType = VK_IMAGE_TYPE_2D;
	imageCI.format = format;
	imageCI.extent = { width, height, 1 };
	imageCI.mipLevels = 1;
	imageCI.arrayLayers = 1;
	imageCI.samples = VK_SAMPLE_COUNT_1_BIT;
	imageCI.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageCI.usage = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	imageCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageCI.queueFamilyIndexCount = 0;
	imageCI.pQueueFamilyIndices = nullptr;
	imageCI.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

	VkResult result = vkCreateImage(device, &imageCI, nullptr, &image);
	ASSERT_VULKAN(result);

	// Image Memory
	SECURITY_ATTRIBUTES winSecurityAttributes{};
	
	VkExportMemoryWin32HandleInfoKHR vulkanExportMemoryWin32HandleInfoKHR = {};
	vulkanExportMemoryWin32HandleInfoKHR.sType = VK_STRUCTURE_TYPE_EXPORT_MEMORY_WIN32_HANDLE_INFO_KHR;
	vulkanExportMemoryWin32HandleInfoKHR.pNext = NULL;
	vulkanExportMemoryWin32HandleInfoKHR.pAttributes = &winSecurityAttributes;
	vulkanExportMemoryWin32HandleInfoKHR.dwAccess = DXGI_SHARED_RESOURCE_READ | DXGI_SHARED_RESOURCE_WRITE;
	vulkanExportMemoryWin32HandleInfoKHR.name = (LPCWSTR)NULL;

	VkExportMemoryAllocateInfoKHR vulkanExportMemoryAllocateInfoKHR = {};
	vulkanExportMemoryAllocateInfoKHR.sType = VK_STRUCTURE_TYPE_EXPORT_MEMORY_ALLOCATE_INFO_KHR;
	vulkanExportMemoryAllocateInfoKHR.pNext = &vulkanExportMemoryWin32HandleInfoKHR;
	vulkanExportMemoryAllocateInfoKHR.handleTypes = externalMemoryHandleType;
	
	VkMemoryRequirements memoryRequirements;
	vkGetImageMemoryRequirements(device, image, &memoryRequirements);

	VkMemoryAllocateInfo allocateInfo;
	allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocateInfo.pNext = &vulkanExportMemoryAllocateInfoKHR;
	allocateInfo.allocationSize = memoryRequirements.size;
	allocateInfo.memoryTypeIndex = en::VulkanAPI::FindMemoryType(
		memoryRequirements.memoryTypeBits,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	result = vkAllocateMemory(device, &allocateInfo, nullptr, &imageMemory);
	ASSERT_VULKAN(result);

	result = vkBindImageMemory(device, image, imageMemory, 0);
	ASSERT_VULKAN(result);

	// Change image layout
	VkCommandBufferBeginInfo beginInfo;
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.pNext = nullptr;
	beginInfo.flags = 0;
	beginInfo.pInheritanceInfo = nullptr;

	result = vkBeginCommandBuffer(commandBuffer, &beginInfo);
	ASSERT_VULKAN(result);

	en::vk::CommandRecorder::ImageLayoutTransfer(
		commandBuffer,
		image,
		VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_GENERAL,
		VK_ACCESS_NONE,
		VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT,
		VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
		VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);

	result = vkEndCommandBuffer(commandBuffer);
	ASSERT_VULKAN(result);

	VkSubmitInfo submitInfo;
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.pNext = nullptr;
	submitInfo.waitSemaphoreCount = 0;
	submitInfo.pWaitSemaphores = nullptr;
	submitInfo.pWaitDstStageMask = nullptr;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;
	submitInfo.signalSemaphoreCount = 0;
	submitInfo.pSignalSemaphores = nullptr;

	result = vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
	ASSERT_VULKAN(result);
	result = vkQueueWaitIdle(queue);
	ASSERT_VULKAN(result);

	// Create image view
	VkImageViewCreateInfo imageViewCI;
	imageViewCI.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	imageViewCI.pNext = nullptr;
	imageViewCI.flags = 0;
	imageViewCI.image = image;
	imageViewCI.viewType = VK_IMAGE_VIEW_TYPE_2D;
	imageViewCI.format = format;
	imageViewCI.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
	imageViewCI.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
	imageViewCI.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
	imageViewCI.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
	imageViewCI.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	imageViewCI.subresourceRange.baseArrayLayer = 0;
	imageViewCI.subresourceRange.baseMipLevel = 0;
	imageViewCI.subresourceRange.layerCount = 1;
	imageViewCI.subresourceRange.levelCount = 1;

	result = vkCreateImageView(device, &imageViewCI, nullptr, &imageView);
	ASSERT_VULKAN(result);

	// Create sampler
	VkSamplerCreateInfo samplerCI;
	samplerCI.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerCI.pNext = nullptr;
	samplerCI.flags = 0;
	samplerCI.magFilter = VK_FILTER_LINEAR;
	samplerCI.minFilter = VK_FILTER_LINEAR;
	samplerCI.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerCI.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
	samplerCI.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
	samplerCI.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
	samplerCI.mipLodBias = 0.0f;
	samplerCI.anisotropyEnable = VK_FALSE;
	samplerCI.maxAnisotropy = 0.0f;
	samplerCI.compareEnable = VK_FALSE;
	samplerCI.compareOp = VK_COMPARE_OP_LESS;
	samplerCI.minLod = 0.0f;
	samplerCI.maxLod = 1.0f;
	samplerCI.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerCI.unnormalizedCoordinates = VK_FALSE;

	result = vkCreateSampler(device, &samplerCI, nullptr, &sampler);
	ASSERT_VULKAN(result);
}

void CreateSyncObjects(VkDevice device)
{
	// Create vulkan semaphores
	SECURITY_ATTRIBUTES winSecurityAttributes;

	VkExportSemaphoreWin32HandleInfoKHR vulkanExportSemaphoreWin32HandleInfoKHR = {};
	vulkanExportSemaphoreWin32HandleInfoKHR.sType = VK_STRUCTURE_TYPE_EXPORT_SEMAPHORE_WIN32_HANDLE_INFO_KHR;
	vulkanExportSemaphoreWin32HandleInfoKHR.pNext = NULL;
	vulkanExportSemaphoreWin32HandleInfoKHR.pAttributes = &winSecurityAttributes;
	vulkanExportSemaphoreWin32HandleInfoKHR.dwAccess = DXGI_SHARED_RESOURCE_READ | DXGI_SHARED_RESOURCE_WRITE;
	vulkanExportSemaphoreWin32HandleInfoKHR.name = (LPCWSTR)NULL;

	VkExportSemaphoreCreateInfoKHR vulkanExportSemaphoreCreateInfo = {};
	vulkanExportSemaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_EXPORT_SEMAPHORE_CREATE_INFO_KHR;
	vulkanExportSemaphoreCreateInfo.pNext = &vulkanExportSemaphoreWin32HandleInfoKHR;
	vulkanExportSemaphoreCreateInfo.handleTypes = VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_WIN32_BIT;
	vulkanExportSemaphoreCreateInfo.pNext = NULL;
	vulkanExportSemaphoreCreateInfo.handleTypes = externalSemaphoreHandleType;
	
	VkSemaphoreCreateInfo semaphoreCI;
	semaphoreCI.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	semaphoreCI.pNext = &vulkanExportSemaphoreCreateInfo;
	semaphoreCI.flags = 0;

	VkResult result = vkCreateSemaphore(device, &semaphoreCI, nullptr, &vkCudaStartSemaphore);
	ASSERT_VULKAN(result);

	result = vkCreateSemaphore(device, &semaphoreCI, nullptr, &vkCudaFinishedSemaphore);
	ASSERT_VULKAN(result);

	// Import vulkan semaphore to cuda
	cudaExternalSemaphoreHandleDesc extCudaStartSemaphoreHD{};
	extCudaStartSemaphoreHD.type = cudaExternalSemaphoreHandleTypeOpaqueWin32;
	extCudaStartSemaphoreHD.handle.win32.handle = GetSemaphoreHandle(device, vkCudaStartSemaphore);

	cudaError_t error = cudaImportExternalSemaphore(&cuCudaStartSemaphore, &extCudaStartSemaphoreHD);
	ASSERT_CUDA(error);

	cudaExternalSemaphoreHandleDesc extCudaFinishedSemaphoreHD{};
	extCudaFinishedSemaphoreHD.type = cudaExternalSemaphoreHandleTypeOpaqueWin32;
	extCudaFinishedSemaphoreHD.handle.win32.handle = GetSemaphoreHandle(device, vkCudaFinishedSemaphore);

	error = cudaImportExternalSemaphore(&cuCudaFinishedSemaphore, &extCudaFinishedSemaphoreHD);
	ASSERT_CUDA(error);
}

/*void InitDescriptor(VkDevice device)
{
	// Create desc set layout
	VkDescriptorSetLayoutBinding imageBinding;
	imageBinding.binding = 0;
	imageBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	imageBinding.descriptorCount = 1;
	imageBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	imageBinding.pImmutableSamplers = nullptr;

	VkDescriptorSetLayoutCreateInfo layoutCI;
	layoutCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutCI.pNext = nullptr;
	layoutCI.flags = 0;
	layoutCI.bindingCount = 1;
	layoutCI.pBindings = &imageBinding;

	VkResult result = vkCreateDescriptorSetLayout(device, &layoutCI, nullptr, &descSetLayout);
	ASSERT_VULKAN(result);

	// Create desc pool
	VkDescriptorPoolSize combinedImageSamplerPoolSize;
	combinedImageSamplerPoolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	combinedImageSamplerPoolSize.descriptorCount = 1;

	VkDescriptorPoolCreateInfo poolCI;
	poolCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolCI.pNext = nullptr;
	poolCI.flags = 0;
	poolCI.maxSets = 1;
	poolCI.poolSizeCount = 1;
	poolCI.pPoolSizes = &combinedImageSamplerPoolSize;

	result = vkCreateDescriptorPool(device, &poolCI, nullptr, &descPool);
	ASSERT_VULKAN(result);

	// Allocate desc set
	VkDescriptorSetAllocateInfo descSetAI;
	descSetAI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	descSetAI.pNext = nullptr;
	descSetAI.descriptorPool = descPool;
	descSetAI.descriptorSetCount = 1;
	descSetAI.pSetLayouts = &descSetLayout;

	result = vkAllocateDescriptorSets(device, &descSetAI, &descSet);
	ASSERT_VULKAN(result);

	// Update desc set
	VkDescriptorImageInfo imageInfo;
	imageInfo.sampler = sampler;
	imageInfo.imageView = imageView;
	imageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

	VkWriteDescriptorSet imageWrite;
	imageWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	imageWrite.pNext = nullptr;
	imageWrite.dstSet = descSet;
	imageWrite.dstBinding = 0;
	imageWrite.dstArrayElement = 0;
	imageWrite.descriptorCount = 1;
	imageWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	imageWrite.pImageInfo = &imageInfo;
	imageWrite.pBufferInfo = nullptr;
	imageWrite.pTexelBufferView = nullptr;

	vkUpdateDescriptorSets(device, 1, &imageWrite, 0, nullptr);
}*/

void DestroyVulkanResources(VkDevice device)
{
	vkDestroySemaphore(device, vkCudaFinishedSemaphore, nullptr);
	vkDestroySemaphore(device, vkCudaStartSemaphore, nullptr);

	vkDestroySampler(device, sampler, nullptr);
	vkDestroyImageView(device, imageView, nullptr);
	vkFreeMemory(device, imageMemory, nullptr);
	vkDestroyImage(device, image, nullptr);

	//vkDestroyDescriptorPool(device, descPool, nullptr);
	//vkDestroyDescriptorSetLayout(device, descSetLayout, nullptr);

	commandPool->Destroy();
	delete commandPool;
}

void CuVkSemaphoreSignal(cudaExternalSemaphore_t& extSemaphore)
{
	cudaExternalSemaphoreSignalParams extSemaphoreSignalParams;
	memset(&extSemaphoreSignalParams, 0, sizeof(extSemaphoreSignalParams));
	extSemaphoreSignalParams.params.fence.value = 0;
	extSemaphoreSignalParams.flags = 0;

	cudaError_t error = cudaSignalExternalSemaphoresAsync(&extSemaphore, &extSemaphoreSignalParams, 1, streamToRun);
	ASSERT_CUDA(error);
}

void CuVkSemaphoreWait(cudaExternalSemaphore_t& extSemaphore)
{
	cudaExternalSemaphoreWaitParams extSemaphoreWaitParams;
	memset(&extSemaphoreWaitParams, 0, sizeof(extSemaphoreWaitParams));
	extSemaphoreWaitParams.params.fence.value = 0;
	extSemaphoreWaitParams.flags = 0;

	cudaError_t error = cudaWaitExternalSemaphoresAsync(&extSemaphore, &extSemaphoreWaitParams, 1, streamToRun);
	ASSERT_CUDA(error);
}

void RecordSwapchainCommandBuffer(VkCommandBuffer commandBuffer, VkImage image)
{
	uint32_t width = en::Window::GetWidth();
	uint32_t height = en::Window::GetHeight();

	VkCommandBufferBeginInfo beginInfo;
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.pNext = nullptr;
	beginInfo.flags = 0;
	beginInfo.pInheritanceInfo = nullptr;

	VkResult result = vkBeginCommandBuffer(commandBuffer, &beginInfo);
	if (result != VK_SUCCESS)
		en::Log::Error("Failed to begin VkCommandBuffer", true);

	en::vk::CommandRecorder::ImageLayoutTransfer(
		commandBuffer,
		image,
		VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		VK_ACCESS_NONE_KHR,
		VK_ACCESS_TRANSFER_WRITE_BIT,
		VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
		VK_PIPELINE_STAGE_TRANSFER_BIT);

	if (en::ImGuiRenderer::IsInitialized())
	{
		VkImageCopy imageCopy;
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
		imageCopy.extent = { width, height, 1 };

		vkCmdCopyImage(
			commandBuffer,
			en::ImGuiRenderer::GetImage(),
			VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			image,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1,
			&imageCopy);
	}

	en::vk::CommandRecorder::ImageLayoutTransfer(
		commandBuffer,
		image,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
		VK_ACCESS_TRANSFER_WRITE_BIT,
		VK_ACCESS_COLOR_ATTACHMENT_READ_BIT,
		VK_PIPELINE_STAGE_TRANSFER_BIT,
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);

	result = vkEndCommandBuffer(commandBuffer);
	if (result != VK_SUCCESS)
		en::Log::Error("Failed to end VkCommandBuffer", true);
}

void SwapchainResizeCallback()
{
	en::Window::WaitForUsableSize();
	vkDeviceWaitIdle(en::VulkanAPI::GetDevice()); // TODO: causes error with multithreaded rendering

	en::Log::Info("Skipping swapchain resize callback");

	//uint32_t width = en::Window::GetWidth();
	//uint32_t height = en::Window::GetHeight();
	//nrcHpmRenderer->ResizeFrame(width, height);
	//en::ImGuiRenderer::Resize(width, height);
	//en::ImGuiRenderer::SetBackgroundImageView(imageView);
}

void RunTcnn()
{
	// Start engine
	const std::string appName("NRC-HPM-Renderer");
	uint32_t width = 768; // Multiple of 128 for nrc batch size
	uint32_t height = width;
	en::Log::Info("Starting " + appName);
	en::Window::Init(width, height, false, appName);
	en::VulkanAPI::Init(appName);
	const VkDevice device = en::VulkanAPI::GetDevice();
	const uint32_t qfi = en::VulkanAPI::GetGraphicsQFI();
	const VkQueue queue = en::VulkanAPI::GetGraphicsQueue();

	en::vk::Swapchain swapchain(width, height, RecordSwapchainCommandBuffer, SwapchainResizeCallback);

	LoadVulkanProcAddr();
	CreateCommandBuffer(en::VulkanAPI::GetGraphicsQFI());
	CreateImage(device, queue, width, height);
	CreateSyncObjects(device);

	en::ImGuiRenderer::Init(width, height);
	en::ImGuiRenderer::SetBackgroundImageView(imageView);

	// Swapchain rerecording because imgui renderer is now available
	swapchain.Resize(width, height);

	/*// Init tcnn
	nlohmann::json config = {
	{"loss", {
		{"otype", "L2"}
	}},
	{"optimizer", {
		{"otype", "Adam"},
		{"learning_rate", 1e-3},
	}},
	{"encoding", {
		{"otype", "Composite"},
		{"reduction", "Concatenation"},
		{"nested", {
			{
				{"otype", "HashGrid"},
				{"n_dims_to_encode", 3},
				{"n_levels", 16},
				{"n_features_per_level", 2},
				{"log2_hashmap_size", 19},
				{"base_resolution", 16},
				{"per_level_scale", 2.0},
			},
			{
				{"otype", "OneBlob"},
				{"n_dims_to_encode", 2},
				{"n_bins", 4},
			},
		}},
	}},
	{"network", {
		{"otype", "FullyFusedMLP"},
		{"activation", "ReLU"},
		{"output_activation", "None"},
		{"n_neurons", 128},
		{"n_hidden_layers", 6},
	}},
	};

	const uint32_t n_input_dims = 5;
	const uint32_t n_output_dims = 3;
	const uint32_t n_inference_steps = 36;
	const uint32_t n_training_steps = 10;
	const uint32_t batch_size = 16384;

	tcnn::TrainableModel model = tcnn::create_from_config(n_input_dims, n_output_dims, config);

	tcnn::GPUMatrix<float> training_batch_inputs(n_input_dims, batch_size);
	tcnn::GPUMatrix<float> training_batch_targets(n_output_dims, batch_size);

	tcnn::GPUMatrix<float> inference_inputs(n_input_dims, batch_size);
	tcnn::GPUMatrix<float> inference_outputs(n_output_dims, batch_size);

	tcnn::GPUMemory<uint8_t> tcnnMemory(batch_size * n_input_dims * sizeof(float));*/

	// Interop test
	cudaExternalMemoryHandleDesc cuExtMemHandleDesc;
	memset(&cuExtMemHandleDesc, 0, sizeof(cudaExternalMemoryHandleDesc));
	cuExtMemHandleDesc.type = cudaExternalMemoryHandleTypeOpaqueWin32;
	cuExtMemHandleDesc.handle.win32.handle = GetImageMemoryHandle(device);
	cuExtMemHandleDesc.size = width * height * 4 * sizeof(float);
	
	cudaExternalMemory_t cuVkImageMemory;
	cudaError_t cudaResult = cudaImportExternalMemory(&cuVkImageMemory, &cuExtMemHandleDesc);
	ASSERT_CUDA(cudaResult);

	// Main loop
	VkResult result;
	while (!en::Window::IsClosed())
	{
		// Update
		en::Window::Update();
		width = en::Window::GetWidth();
		height = en::Window::GetHeight();

		// Render frame
		const glm::vec4 randomColor = glm::linearRand(glm::vec4(0.0), glm::vec4(1.0));


		// Imgui
		en::ImGuiRenderer::StartFrame();

		ImGui::Begin("Hello world");

		ImGui::End();

		// Display
		en::ImGuiRenderer::EndFrame(queue);
		result = vkQueueWaitIdle(queue);
		ASSERT_VULKAN(result);

		swapchain.DrawAndPresent();
	}
	result = vkDeviceWaitIdle(device);
	ASSERT_VULKAN(result);

	// End
	DestroyVulkanResources(device);
	
	en::ImGuiRenderer::Shutdown();

	swapchain.Destroy(true);

	en::VulkanAPI::Shutdown();
	en::Window::Shutdown();

	en::Log::Info("Ending " + appName);
}
