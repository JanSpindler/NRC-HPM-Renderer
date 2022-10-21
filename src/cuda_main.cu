//#include <cuda_main.hpp>

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
#include <engine/graphics/renderer/NrcHpmRenderer.hpp>
#include <engine/graphics/NeuralRadianceCache.hpp>
#include <engine/util/read_file.hpp>
#include <engine/util/Input.hpp>
#include <engine/util/Time.hpp>
#include <engine/HpmScene.hpp>
#include <engine/AppConfig.hpp>

#include <cuda_runtime.h>
#include <tiny-cuda-nn/config.h>
#include <vulkan/vulkan.h>

#define ASSERT_CUDA(error) if (error != cudaSuccess) { en::Log::Error("Cuda assert triggered: " + std::string(cudaGetErrorName(error)), true); }

en::NrcHpmRenderer* hpmRenderer = nullptr;

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

	if (hpmRenderer != nullptr && en::ImGuiRenderer::IsInitialized())
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

bool RunAppConfigInstance(const en::AppConfig& appConfig)
{
	// Start engine
	const std::string appName("NRC-HPM-Renderer");
	uint32_t width = appConfig.renderWidth;
	uint32_t height = appConfig.renderHeight;
	en::Log::Info("Starting " + appName);
	en::Window::Init(width, height, false, appName);
	en::Input::Init(en::Window::GetGLFWHandle());
	en::VulkanAPI::Init(appName);
	const VkDevice device = en::VulkanAPI::GetDevice();
	const uint32_t qfi = en::VulkanAPI::GetGraphicsQFI();
	const VkQueue queue = en::VulkanAPI::GetGraphicsQueue();

	// Init nrc
	nlohmann::json config = {
	{"loss", {
		{"otype", appConfig.lossFn}
	}},
	{"optimizer", {
		{"otype", appConfig.optimizer},
		{"learning_rate", appConfig.learningRate},
		//{"clipping_magnitude", 0.1},
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
		{"n_neurons", appConfig.nnWidth},
		{"n_hidden_layers", appConfig.nnDepth},
	}},
	};

	en::NeuralRadianceCache nrc(config, 14);

	en::HpmScene hpmScene(appConfig);

	// Setup rendering
	en::Camera camera(
		glm::vec3(0.0f, 0.0f, -64.0f),
		glm::vec3(0.0f, 0.0f, 1.0f),
		glm::vec3(0.0f, 1.0f, 0.0f),
		static_cast<float>(width) / static_cast<float>(height),
		glm::radians(60.0f),
		0.1f,
		100.0f);

	// Init rendering pipeline
	en::vk::Swapchain swapchain(width, height, RecordSwapchainCommandBuffer, SwapchainResizeCallback);

	hpmRenderer = new en::NrcHpmRenderer(
		width,
		height,
		appConfig.trainSampleRatio,
		appConfig.trainSpp,
		camera,
		hpmScene,
		nrc);

	en::ImGuiRenderer::Init(width, height);
	en::ImGuiRenderer::SetBackgroundImageView(hpmRenderer->GetImageView());

	// Swapchain rerecording because imgui renderer is now available
	swapchain.Resize(width, height);

	// Main loop
	VkResult result;
	size_t frameCount = 0;
	bool shutdown = false;
	bool restartAfterClose = false;
	while (!en::Window::IsClosed() && !shutdown)
	{
		// Update
		en::Window::Update();
		en::Input::Update();
		en::Time::Update();

		width = en::Window::GetWidth();
		height = en::Window::GetHeight();

		float deltaTime = static_cast<float>(en::Time::GetDeltaTime());
		uint32_t fps = en::Time::GetFps();

		// Physics
		en::Input::HandleUserCamInput(&camera, deltaTime);
		camera.SetAspectRatio(width, height);
		camera.UpdateUniformBuffer();

		// Render
		hpmRenderer->Render(queue);
		result = vkQueueWaitIdle(queue);
		ASSERT_VULKAN(result);

		// Imgui
		en::ImGuiRenderer::StartFrame();

		ImGui::Begin("Statistics");
		ImGui::Text("DeltaTime %f", deltaTime);
		ImGui::Text("FPS %d", fps);
		ImGui::Text("NRC Loss %f", nrc.GetLoss());
		ImGui::End();

		ImGui::Begin("Controls");
		shutdown = ImGui::Button("Shutdown");
		ImGui::Checkbox("Restart after shutdown", &restartAfterClose);
		ImGui::End();

		hpmScene.Update(true);

		en::ImGuiRenderer::EndFrame(queue, VK_NULL_HANDLE);
		result = vkQueueWaitIdle(queue);
		ASSERT_VULKAN(result);

		// Display
		swapchain.DrawAndPresent(VK_NULL_HANDLE, VK_NULL_HANDLE);
		frameCount++;
	}
	result = vkDeviceWaitIdle(device);
	ASSERT_VULKAN(result);

	// End
	hpmRenderer->Destroy();
	delete hpmRenderer;
	en::ImGuiRenderer::Shutdown();
	swapchain.Destroy(true);

	hpmScene.Destroy();
	camera.Destroy();
	nrc.Destroy();

	en::VulkanAPI::Shutdown();
	en::Window::Shutdown();
	en::Log::Info("Ending " + appName);

	return restartAfterClose;
}

int main()
{
	en::AppConfig appConfig;
	appConfig.lossFn = "RelativeL2";
	appConfig.optimizer = "Adam";
	appConfig.learningRate = 1e-3f;

	appConfig.dirLightStrength = 4.0f;
	appConfig.pointLightStrength = 0.0f;
	appConfig.hdrEnvMapPath = "data/image/photostudio.hdr";
	appConfig.hdrEnvMapDirectStrength = 1.0f;
	appConfig.hdrEnvMapHpmStrength = 0.0f;
	
	appConfig.renderWidth = 1920;
	appConfig.renderHeight = 1080;
	appConfig.trainSampleRatio = 0.05f;
	appConfig.trainSpp = 1;
	appConfig.nnWidth = 64;
	appConfig.nnDepth = 6;

	bool restartRunConfig;
	do {
		restartRunConfig = RunAppConfigInstance(appConfig);
	} while (restartRunConfig);

	return 0;
}
