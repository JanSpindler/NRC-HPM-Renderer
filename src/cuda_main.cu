#include <engine/cuda_common.hpp>
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
#include <filesystem>
#include <engine/graphics/renderer/McHpmRenderer.hpp>

en::NrcHpmRenderer* nrcHpmRenderer = nullptr;
en::McHpmRenderer* mcHpmRenderer = nullptr;

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

	if (nrcHpmRenderer != nullptr && mcHpmRenderer != nullptr && en::ImGuiRenderer::IsInitialized())
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

void Benchmark(
	uint32_t width, 
	uint32_t height, 
	uint32_t sceneID, 
	const en::AppConfig& appConfig, 
	const en::HpmScene& scene, 
	VkQueue queue)
{
	// Create benchmark camera
	const float aspectRatio = static_cast<float>(width) / static_cast<float>(height);

	std::array<en::Camera, 6> cameras = {
		en::Camera(
			glm::vec3(64.0f, 0.0f, 0.0f),
			glm::vec3(-1.0f, 0.0f, 0.0f),
			glm::vec3(0.0f, 1.0f, 0.0f),
			aspectRatio,
			glm::radians(60.0f),
			0.1f,
			100.0f),
		en::Camera(
			glm::vec3(-64.0f, 0.0f, 0.0f),
			glm::vec3(1.0f, 0.0f, 0.0f),
			glm::vec3(0.0f, 1.0f, 0.0f),
			aspectRatio,
			glm::radians(60.0f),
			0.1f,
			100.0f),
		en::Camera(
			glm::vec3(0.0f, 64.0f, 0.0f),
			glm::vec3(0.0f, -1.0f, 0.0f),
			glm::vec3(1.0f, 0.0f, 0.0f),
			aspectRatio,
			glm::radians(60.0f),
			0.1f,
			100.0f),
		en::Camera(
			glm::vec3(0.0f, -64.0f, 0.0f),
			glm::vec3(0.0f, 1.0f, 0.0f),
			glm::vec3(1.0f, 0.0f, 0.0f),
			aspectRatio,
			glm::radians(60.0f),
			0.1f,
			100.0f),
		en::Camera(
			glm::vec3(0.0f, 0.0f, 64.0f),
			glm::vec3(0.0f, 0.0f, -1.0f),
			glm::vec3(0.0f, 1.0f, 0.0f),
			aspectRatio,
			glm::radians(60.0f),
			0.1f,
			100.0f),
		en::Camera(
			glm::vec3(0.0f, 0.0f, -64.0f),
			glm::vec3(0.0f, 0.0f, 1.0f),
			glm::vec3(0.0f, 1.0f, 0.0f),
			aspectRatio,
			glm::radians(60.0f),
			0.1f,
			100.0f),
	};

	// Create output path if not exists
	std::string outputDirPath = "output/ " + appConfig.GetName() + "/";
	if (!std::filesystem::is_directory(outputDirPath) || !std::filesystem::exists(outputDirPath))
	{
		std::filesystem::create_directory(outputDirPath);
	}

	// Create reference folder if not exists
	std::string referenceDirPath = "output/" + std::to_string(sceneID) + "/";
#if __cplusplic >= 201703L
	en::Log::Warn("C++ version lower then 17. Cant create reference data");
#else
	if (!std::filesystem::is_directory(referenceDirPath) || !std::filesystem::exists(referenceDirPath))
	{
		en::Log::Info("Reference folder for scene " + std::to_string(sceneID) + " was not found. Creating reference images");

		// Create folder
		std::filesystem::create_directory(referenceDirPath);

		// Create ground truth renderer
		en::McHpmRenderer* gtRenderer = nullptr;

		for (size_t i = 0; i < cameras.size(); i++)
		{
			en::Log::Info("Generating reference image " + std::to_string(i));

			// Set new camera
			if (gtRenderer == nullptr) { gtRenderer = new en::McHpmRenderer(width, height, 64, &cameras[i], scene); }
			else { gtRenderer->SetCamera(&cameras[i]); }

			// Generate reference image
			for (size_t frame = 0; frame < 1024; frame++)
			{
				gtRenderer->Render(queue);
				ASSERT_VULKAN(vkQueueWaitIdle(queue));
			}

			// Export reference image
			gtRenderer->ExportOutputImageToFile(queue, referenceDirPath + std::to_string(i) + ".exr");
		}

		// Destroy resources
		gtRenderer->Destroy();
		delete gtRenderer;
	}
#endif

	// Test frame
	for (size_t i = 0; i < cameras.size(); i++)
	{
		nrcHpmRenderer->SetCamera(&cameras[i]);
		nrcHpmRenderer->Render(queue);
		ASSERT_VULKAN(vkQueueWaitIdle(queue));
		nrcHpmRenderer->ExportOutputImageToFile(queue, outputDirPath + "nrc_" + std::to_string(i) + ".exr");
	
		mcHpmRenderer->SetCamera(&cameras[i]);
		mcHpmRenderer->Render(queue);
		ASSERT_VULKAN(vkQueueWaitIdle(queue));
		mcHpmRenderer->ExportOutputImageToFile(queue, outputDirPath + "mc_" + std::to_string(i) + ".exr");
	}

	// Destroy resources
	for (size_t i = 0; i < cameras.size(); i++) { cameras[i].Destroy(); }
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

	// Renderer select
	const std::vector<char*> rendererMenuItems = { "MC", "NRC" }; // TODO: Restir
	const char* currentRendererMenuItem = rendererMenuItems[1];
	uint32_t rendererId = 1;

	// Init resources
	en::NeuralRadianceCache nrc(appConfig);

	en::HpmScene hpmScene(appConfig);

	// Setup rendering
	en::Camera camera(
		glm::vec3(64.0f, 0.0f, 0.0f),
		glm::vec3(-1.0f, 0.0f, 0.0f),
		glm::vec3(0.0f, 1.0f, 0.0f),
		static_cast<float>(width) / static_cast<float>(height),
		glm::radians(60.0f),
		0.1f,
		100.0f);

	// Init rendering pipeline
	en::vk::Swapchain swapchain(width, height, RecordSwapchainCommandBuffer, SwapchainResizeCallback);

	nrcHpmRenderer = new en::NrcHpmRenderer(
		width,
		height,
		appConfig.trainSampleRatio,
		appConfig.trainSpp,
		&camera,
		hpmScene,
		nrc);

	mcHpmRenderer = new en::McHpmRenderer(width, height, 32, &camera, hpmScene);

	en::ImGuiRenderer::Init(width, height);
	switch (rendererId)
	{
	case 0: // MC
		en::ImGuiRenderer::SetBackgroundImageView(mcHpmRenderer->GetImageView());
		break;
	case 1: // NRC
		en::ImGuiRenderer::SetBackgroundImageView(nrcHpmRenderer->GetImageView());
		break;
	default: // Error
		en::Log::Error("Renderer ID is invalid", true);
		break;
	}

	// Swapchain rerecording because imgui renderer is now available
	swapchain.Resize(width, height);

	// Main loop
	VkResult result;
	size_t frameCount = 0;
	bool shutdown = false;
	bool restartAfterClose = false;
	while (!en::Window::IsClosed() && !shutdown)
	{
		// Exit
		//if (frameCount == 10) { break; }

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
		// Always render nrc for training
		//nrcHpmRenderer->Render(queue);
		//result = vkQueueWaitIdle(queue);
		//ASSERT_VULKAN(result);
		//nrcHpmRenderer->EvaluateTimestampQueries();

		switch (rendererId)
		{
		case 0: // MC
			mcHpmRenderer->Render(queue);
			result = vkQueueWaitIdle(queue);
			ASSERT_VULKAN(result);
			mcHpmRenderer->EvaluateTimestampQueries();
			break;
		case 1: // NRC
			nrcHpmRenderer->Render(queue);
			result = vkQueueWaitIdle(queue);
			ASSERT_VULKAN(result);
			nrcHpmRenderer->EvaluateTimestampQueries();
			break;
		default: // Error
			en::Log::Error("Renderer ID is invalid", true);
			break;
		}

		// Imgui
		en::ImGuiRenderer::StartFrame();

		ImGui::Begin("Statistics");
		ImGui::Text((std::string("Framecount ") + std::to_string(frameCount)).c_str());
		ImGui::Text("DeltaTime %f", deltaTime);
		ImGui::Text("FPS %d", fps);
		ImGui::Text("NRC Loss %f", nrc.GetLoss());
		ImGui::End();

		ImGui::Begin("Controls");
		shutdown = ImGui::Button("Shutdown");
		ImGui::Checkbox("Restart after shutdown", &restartAfterClose);
		
		if (ImGui::BeginCombo("##combo", currentRendererMenuItem))
		{
			for (int i = 0; i < rendererMenuItems.size(); i++)
			{
				bool selected = (currentRendererMenuItem == rendererMenuItems[i]);
				if (ImGui::Selectable(rendererMenuItems[i], selected))
				{
					if (i != rendererId)
					{
						rendererId = i;
						switch (rendererId)
						{
						case 0: // MC
							en::ImGuiRenderer::SetBackgroundImageView(mcHpmRenderer->GetImageView());
							break;
						case 1: // NRC
							en::ImGuiRenderer::SetBackgroundImageView(nrcHpmRenderer->GetImageView());
							break;
						default: // Error
							en::Log::Error("Renderer ID is invalid", true);
							break;
						}
					}
					currentRendererMenuItem = rendererMenuItems[i]; 
				};
				if (selected) { ImGui::SetItemDefaultFocus(); }
			}
			ImGui::EndCombo();
		}

		ImGui::End();

		mcHpmRenderer->RenderImGui();
		nrcHpmRenderer->RenderImGui();

		hpmScene.Update(true);

		appConfig.RenderImGui();

		en::ImGuiRenderer::EndFrame(queue, VK_NULL_HANDLE);
		result = vkQueueWaitIdle(queue);
		ASSERT_VULKAN(result);

		// Display
		swapchain.DrawAndPresent(VK_NULL_HANDLE, VK_NULL_HANDLE);
		frameCount++;
	}

	// Evaluate at end
	Benchmark(appConfig.renderWidth, appConfig.renderHeight, appConfig.scene.id, appConfig, hpmScene, queue);

	// Stop gpu work
	result = vkDeviceWaitIdle(device);
	ASSERT_VULKAN(result);

	// End
	mcHpmRenderer->Destroy();
	delete mcHpmRenderer;
	
	nrcHpmRenderer->Destroy();
	delete nrcHpmRenderer;
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

int main(int argc, char** argv)
{
	en::AppConfig appConfig(argc, argv);

	bool restartRunConfig;
	do {
		restartRunConfig = RunAppConfigInstance(appConfig);
	} while (restartRunConfig);

	return 0;
}
