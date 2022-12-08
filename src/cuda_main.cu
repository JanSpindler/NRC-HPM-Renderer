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
#include <engine/graphics/renderer/McHpmRenderer.hpp>
#include <engine/graphics/vulkan/CommandPool.hpp>
#include <engine/graphics/Reference.hpp>
#include <engine/objects/Model.hpp>
#include <engine/graphics/renderer/SimpleModelRenderer.hpp>
#include <openvdb/openvdb.h>
#include <engine/util/LogFile.hpp>

en::Reference* reference = nullptr;
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
	vkDeviceWaitIdle(en::VulkanAPI::GetDevice());

	en::Log::Info("Skipping swapchain resize callback");

	//uint32_t width = en::Window::GetWidth();
	//uint32_t height = en::Window::GetHeight();
	//nrcHpmRenderer->ResizeFrame(width, height);
	//en::ImGuiRenderer::Resize(width, height);
	//en::ImGuiRenderer::SetBackgroundImageView(imageView);
}

struct ViewBenchmarkStats
{
	float mse;
	glm::vec3 bias;
};

struct BenchmarkStats
{
	size_t frameIndex;
	float frameTimeMS;
	float loss;
	std::array<ViewBenchmarkStats, 6> viewStats;

	std::string ToString() const
	{
		std::string str = 
			std::to_string(frameIndex) + " " +
			std::to_string(frameTimeMS) + " " +
			std::to_string(loss) + " ";
		for (size_t i = 0; i < viewStats.size(); i++)
		{
			str += std::to_string(viewStats[i].mse) + " " +
				std::to_string(viewStats[i].bias.x) + " " +
				std::to_string(viewStats[i].bias.y) + " " +
				std::to_string(viewStats[i].bias.z) + " ";
		}

		return str;
	}
};

void Benchmark(const en::Camera* camera, VkQueue queue, size_t frameCount, BenchmarkStats& stats, en::LogFile& logFile)
{
	en::Log::Info("Frame: " + std::to_string(frameCount));
	std::array<en::Reference::Result, 6> results = reference->CompareNrc(*nrcHpmRenderer, camera, queue);

	for (size_t i = 0; i < results.size(); i++)
	{
		stats.viewStats[i].mse = results[i].mse;
		stats.viewStats[i].bias.x = results[i].biasX;
		stats.viewStats[i].bias.y = results[i].biasY;
		stats.viewStats[i].bias.z = results[i].biasZ;
	}

	logFile.WriteLine(stats.ToString());
}

bool RunAppConfigInstance(const en::AppConfig& appConfig)
{
	// Start engine
	const std::string appName("NRC-HPM-Renderer");
	uint32_t width = 1920;
	uint32_t height = 1080;
	en::Log::Info("Starting " + appName);

	en::Window::Init(width, height, false, appName);
	if (en::Window::IsSupported()) { en::Input::Init(en::Window::GetGLFWHandle()); }
	en::VulkanAPI::Init(appName);
	const VkDevice device = en::VulkanAPI::GetDevice();
	const uint32_t qfi = en::VulkanAPI::GetGraphicsQFI();
	const VkQueue queue = en::VulkanAPI::GetGraphicsQueue();

	// Renderer select
	const std::vector<char*> rendererMenuItems = { "MC", "NRC", "Model" };
	const char* currentRendererMenuItem = rendererMenuItems[1];
	uint32_t rendererId = 1;

	// Init resources
	en::Log::Info("Initializing rendering resources");

	en::NeuralRadianceCache nrc(appConfig);

	en::HpmScene hpmScene(appConfig);

	const float aspectRatio = static_cast<float>(width) / static_cast<float>(height);
	en::Camera camera(
		glm::vec3(64.0f, 0.0f, 0.0f),
		glm::vec3(-1.0f, 0.0f, 0.0f),
		glm::vec3(0.0f, 1.0f, 0.0f),
		aspectRatio,
		glm::radians(60.0f),
		0.1f,
		100.0f);

	en::Model dragonModel("dragon.obj", true);
	en::ModelInstance dragonModelInstance(&dragonModel, glm::mat4(1.0f));

	// Init reference
	reference = new en::Reference(width, height, appConfig, hpmScene, queue);

	// Init rendering pipeline
	en::Log::Info("Initializing renderers");

	en::vk::Swapchain* swapchain = nullptr;
	if (en::Window::IsSupported())
	{
		swapchain = new en::vk::Swapchain(width, height, RecordSwapchainCommandBuffer, SwapchainResizeCallback);
	}

	en::SimpleModelRenderer modelRenderer(width, height, &camera);
	modelRenderer.AddModelInstance(&dragonModelInstance);

	nrcHpmRenderer = new en::NrcHpmRenderer(
		width,
		height,
		appConfig.trainSampleRatio,
		appConfig.trainSpp,
		appConfig.primaryRayLength,
		false,
		&camera,
		hpmScene,
		nrc);

	mcHpmRenderer = new en::McHpmRenderer(width, height, 32, false, &camera, hpmScene);

	if (en::Window::IsSupported())
	{
		en::ImGuiRenderer::Init(width, height);
		switch (rendererId)
		{
		case 0: // MC
			en::ImGuiRenderer::SetBackgroundImageView(mcHpmRenderer->GetImageView());
			break;
		case 1: // NRC
			en::ImGuiRenderer::SetBackgroundImageView(nrcHpmRenderer->GetImageView());
			break;
		case 2: // Model
			en::ImGuiRenderer::SetBackgroundImageView(modelRenderer.GetColorImageView());
			break;
		default: // Error
			en::Log::Error("Renderer ID is invalid", true);
			break;
		}
	}

	// Swapchain rerecording because imgui renderer is now available
	if (en::Window::IsSupported()) { swapchain->Resize(width, height); }

	// Main loop
	en::Log::Info("Starting main loop");
	BenchmarkStats stats;
	en::LogFile logFile("output/ " + appConfig.GetName() + "/log.txt");
	VkResult result;
	size_t frameCount = 0;
	bool shutdown = false;
	bool restartAfterClose = false;
	bool benchmark = true;
	bool continueLoop = en::Window::IsSupported() ? !en::Window::IsClosed() : true;
	while (continueLoop && !shutdown)
	{
		// Update
		if (en::Window::IsSupported())
		{
			en::Window::Update();
			en::Input::Update();
		}
		en::Time::Update();

		if (en::Window::IsSupported())
		{
			width = en::Window::GetWidth();
			height = en::Window::GetHeight();
		}

		float deltaTime = static_cast<float>(en::Time::GetDeltaTime());
		uint32_t fps = en::Time::GetFps();

		// Physics
		if (en::Window::IsSupported())
		{
			en::Input::HandleUserCamInput(&camera, deltaTime);
			camera.SetAspectRatio(width, height);
		}
		camera.UpdateUniformBuffer();

		// Render
		switch (rendererId)
		{
		case 0: // MC
			mcHpmRenderer->Render(queue);
			result = vkQueueWaitIdle(queue);
			ASSERT_VULKAN(result);
			mcHpmRenderer->EvaluateTimestampQueries();
			break;
		case 1: // NRC
			nrcHpmRenderer->Render(queue, true);
			result = vkQueueWaitIdle(queue);
			ASSERT_VULKAN(result);
			nrcHpmRenderer->EvaluateTimestampQueries();
			break;
		case 2: // Model
			modelRenderer.Render(queue);
			ASSERT_VULKAN(vkQueueWaitIdle(queue));
			break;
		default: // Error
			en::Log::Error("Renderer ID is invalid", true);
			break;
		}

		//
		const float nrcLoss = nrc.GetLoss();

		// Imgui
		if (en::Window::IsSupported())
		{
			en::ImGuiRenderer::StartFrame();

			ImGui::Begin("Statistics");
			ImGui::Text((std::string("Framecount ") + std::to_string(frameCount)).c_str());
			ImGui::Text("DeltaTime %f", deltaTime);
			ImGui::Text("FPS %d", fps);
			ImGui::Text("NRC Loss %f", nrcLoss);
			ImGui::End();

			ImGui::Begin("Controls");
			shutdown = ImGui::Button("Shutdown");
			ImGui::Checkbox("Restart after shutdown", &restartAfterClose);
			ImGui::Checkbox("Benchmark", &benchmark);

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
							case 2: // Model
								en::ImGuiRenderer::SetBackgroundImageView(modelRenderer.GetColorImageView());
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
		}

		// Display
		if (en::Window::IsSupported()) { swapchain->DrawAndPresent(VK_NULL_HANDLE, VK_NULL_HANDLE); }

		// Benchmark
		stats.frameIndex = frameCount;
		stats.frameTimeMS = nrcHpmRenderer->GetFrameTimeMS();
		stats.loss = nrc.GetLoss();
		if (benchmark && frameCount % 1 == 0) { Benchmark(&camera, queue, frameCount, stats, logFile); }

		// Exit if loss is invalid
		if (std::isnan(nrcLoss) || std::isinf(nrcLoss))
		{
			en::Log::Error("NRC Loss is " + std::to_string(nrcLoss), false);
			break;
		}

		//
		frameCount++;
		continueLoop = en::Window::IsSupported() ? !en::Window::IsClosed() : true;
	}

	// Stop gpu work
	result = vkDeviceWaitIdle(device);
	ASSERT_VULKAN(result);

	// End
	mcHpmRenderer->Destroy();
	delete mcHpmRenderer;
	
	nrcHpmRenderer->Destroy();
	delete nrcHpmRenderer;
	en::ImGuiRenderer::Shutdown();
	if (en::Window::IsSupported) { swapchain->Destroy(true); }

	modelRenderer.Destroy();

	reference->Destroy();
	delete reference; 

	dragonModelInstance.Destroy();
	dragonModel.Destroy();
	camera.Destroy();
	hpmScene.Destroy();
	nrc.Destroy();

	en::VulkanAPI::Shutdown();
	if (en::Window::IsSupported()) { en::Window::Shutdown(); }
	en::Log::Info("Ending " + appName);

	return restartAfterClose;
}

int main(int argc, char** argv)
{
	// Init openvdb
	openvdb::initialize();

	// Read arguments for app config
	std::vector<char*> myargv(argc);
	std::memcpy(myargv.data(), argv, sizeof(char*) * argc);
	if (argc == 1)
	{
		en::Log::Info("No arguments found. Loading defaults");
		myargv = { 
			"NRC-HPM-Renderer", 
			"RelativeL2", "Adam", "0.001", "0.99",
			"2", "0", 
			"64", "6", "15", 
			"1", 
			"0.05", "1.0", "1", "2"
		};
	}

	// Create app config
	en::AppConfig appConfig(myargv);

	// Create output path if not exists
	std::string outputDirPath = "output/ " + appConfig.GetName() + "/";
	if (!std::filesystem::is_directory(outputDirPath) || !std::filesystem::exists(outputDirPath))
	{
		std::filesystem::create_directory(outputDirPath);
	}

	// Run
	bool restartRunConfig;
	do {
		restartRunConfig = RunAppConfigInstance(appConfig);
	} while (restartRunConfig);

	// Exit
	return 0;
}
