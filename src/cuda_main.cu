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

#include <tiny-cuda-nn/config.h>
#include <vulkan/vulkan.h>

#define ASSERT_CUDA(error) if (error != cudaSuccess) { en::Log::Error("Cuda assert triggered: " + std::string(cudaGetErrorName(error)), true); }

PFN_vkGetMemoryWin32HandleKHR fpGetMemoryWin32HandleKHR;
PFN_vkGetSemaphoreWin32HandleKHR fpGetSemaphoreWin32HandleKHR;

VkImage image;
VkDeviceMemory imageMemory;

VkExternalMemoryHandleTypeFlagBits externalMemoryHandleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT;

void LoadVulkanProcAddr()
{
	fpGetMemoryWin32HandleKHR = (PFN_vkGetMemoryWin32HandleKHR)vkGetInstanceProcAddr(
		en::VulkanAPI::GetInstance(), 
		"vkGetMemoryWin32HandleKHR");

	fpGetSemaphoreWin32HandleKHR = (PFN_vkGetSemaphoreWin32HandleKHR)vkGetDeviceProcAddr(
		en::VulkanAPI::GetDevice(),
		"vkGetSemaphoreWin32HandleKHR");
}

void CreateImage(uint32_t width, uint32_t height)
{
	VkFormat format = VK_FORMAT_R32G32B32A32_SFLOAT;
	VkDevice device = en::VulkanAPI::GetDevice();

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
	imageCI.usage = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
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
	en::vk::CommandPool commandPool(0, en::VulkanAPI::GetGraphicsQFI());
	commandPool.AllocateBuffers(1, VK_COMMAND_BUFFER_LEVEL_PRIMARY);
	VkCommandBuffer commandBuffer = commandPool.GetBuffer(0);

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

	VkQueue queue = en::VulkanAPI::GetGraphicsQueue();
	result = vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
	ASSERT_VULKAN(result);
	result = vkQueueWaitIdle(queue);
	ASSERT_VULKAN(result);
}

HANDLE GetImageMemoryHandle()
{
	HANDLE handle;

	VkMemoryGetWin32HandleInfoKHR vkMemoryGetWin32HandleInfoKHR = {};
	vkMemoryGetWin32HandleInfoKHR.sType = VK_STRUCTURE_TYPE_MEMORY_GET_WIN32_HANDLE_INFO_KHR;
	vkMemoryGetWin32HandleInfoKHR.pNext = NULL;
	vkMemoryGetWin32HandleInfoKHR.memory = imageMemory;
	vkMemoryGetWin32HandleInfoKHR.handleType = (VkExternalMemoryHandleTypeFlagBitsKHR)VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT;

	fpGetMemoryWin32HandleKHR(en::VulkanAPI::GetDevice(), &vkMemoryGetWin32HandleInfoKHR, &handle);
	return handle;
}

void RunTcnn()
{
	// Start engine
	std::string appName("NRC-HPM-Renderer");
	uint32_t width = 768; // Multiple of 128 for nrc batch size
	uint32_t height = width;
	en::Log::Info("Starting " + appName);
	en::Window::Init(width, height, false, appName);
	en::VulkanAPI::Init(appName);

	// Init tcnn
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

	tcnn::GPUMemory<uint8_t> tcnnMemory(batch_size * n_input_dims * sizeof(float));

	// Interop test
	LoadVulkanProcAddr();
	CreateImage(width, height);
	
	cudaExternalMemoryHandleDesc cuExtMemHandleDesc;
	memset(&cuExtMemHandleDesc, 0, sizeof(cudaExternalMemoryHandleDesc));
	cuExtMemHandleDesc.type = cudaExternalMemoryHandleTypeOpaqueWin32;
	cuExtMemHandleDesc.handle.win32.handle = GetImageMemoryHandle();
	cuExtMemHandleDesc.size = width * height * 4 * sizeof(float);
	
	cudaExternalMemory_t cuVkImageMemory;
	cudaError_t cudaResult = cudaImportExternalMemory(&cuVkImageMemory, &cuExtMemHandleDesc);
	ASSERT_CUDA(cudaResult);

	// Main loop
	VkDevice device = en::VulkanAPI::GetDevice();
	VkQueue graphicsQueue = en::VulkanAPI::GetGraphicsQueue();
	VkResult result;
	while (!en::Window::IsClosed())
	{
		// Update
		en::Window::Update();
		width = en::Window::GetWidth();
		height = en::Window::GetHeight();
	}
	result = vkDeviceWaitIdle(device);
	ASSERT_VULKAN(result);

	// End
	en::VulkanAPI::Shutdown();
	en::Window::Shutdown();

	en::Log::Info("Ending " + appName);
}
