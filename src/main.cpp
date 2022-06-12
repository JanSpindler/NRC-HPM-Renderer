#include <engine/util/Log.hpp>
#include <engine/graphics/Window.hpp>
#include <engine/graphics/VulkanAPI.hpp>
#include <engine/graphics/vulkan/Swapchain.hpp>
#include <engine/graphics/vulkan/CommandRecorder.hpp>
#include <engine/graphics/Camera.hpp>
#include <engine/graphics/renderer/ImGuiRenderer.hpp>
#include <imgui.h>
#include <engine/graphics/renderer/DensityPathTracer.hpp>
#include <engine/util/read_file.hpp>
#include <engine/graphics/vulkan/Texture3D.hpp>
#include <engine/objects/VolumeData.hpp>
#include <engine/util/Input.hpp>
#include <engine/util/Time.hpp>
#include <engine/graphics/Sun.hpp>
#include <engine/compute/Matrix.hpp>
#include <mnist/mnist_reader.hpp>
#include <kompute/Kompute.hpp>
#include <engine/compute/MatmulOp.hpp>
#include <engine/compute/NeuralNetwork.hpp>
#include <engine/compute/SigmoidLayer.hpp>
#include <engine/compute/LinearLayer.hpp>
#include <engine/compute/KomputeManager.hpp>
#include <engine/compute/ReluLayer.hpp>
#include <filesystem>
#include <set>
#include <engine/util/openexr_helper.hpp>
#include <engine/graphics/renderer/NrcHpmRenderer.hpp>
#include <engine/compute/NrcDataset.hpp>
#include <thread>

en::VolumeData* trainVolumeData = nullptr;
en::DensityPathTracer* pathTracer = nullptr;
en::NrcHpmRenderer* nrcHpmRenderer = nullptr;

en::KomputeManager* manager;
en::NeuralNetwork* nn;

std::vector<en::NrcInput> trainInputs;
std::vector<en::NrcTarget> trainTargets;

static bool doneTraining;

void LoadTrainData(std::vector<en::NrcInput>& trainInputs, std::vector<en::NrcTarget>& trainTargets)
{
	// Load data
	std::set<std::filesystem::path> colorFiles;
	std::set<std::filesystem::path> posFiles;
	std::set<std::filesystem::path> dirFiles;

	for (const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator("data/output/color"))
	{
		colorFiles.insert(entry.path().filename());
	}

	for (const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator("data/output/pos"))
	{
		posFiles.insert(entry.path().filename());
	}

	for (const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator("data/output/dir"))
	{
		dirFiles.insert(entry.path().filename());
	}

	std::vector<std::vector<float>> colorDatas;
	std::vector<std::vector<float>> posDatas;
	std::vector<std::vector<float>> dirDatas;

	for (const auto& colorFile : colorFiles)
	{
		const auto& posFile = posFiles.find(colorFile);
		const auto& dirFile = dirFiles.find(colorFile);

		if (posFile != posFiles.end() &&
			dirFile != dirFiles.end())
		{
			float* data;
			int width;
			int height;
			bool hasAlpha;

			// Read color data
			std::string colorStr = std::string("data/output/color/" + colorFile.filename().string());
			en::ReadEXR(colorStr.c_str(), data, width, height, hasAlpha);
			std::vector<float> colorData(width * height * 4);
			memcpy(colorData.data(), data, width * height * 4 * sizeof(float));
			delete data;
			colorDatas.push_back(colorData);

			// Read pos data
			std::string posStr = std::string("data/output/pos/" + (*posFile).filename().string());
			en::ReadEXR(posStr.c_str(), data, width, height, hasAlpha);
			std::vector<float> posData(width * height * 4);
			memcpy(posData.data(), data, width * height * 4 * sizeof(float));
			delete data;
			posDatas.push_back(posData);

			// Read dir data
			std::string dirStr = std::string("data/output/dir/" + (*dirFile).filename().string());
			en::ReadEXR(dirStr.c_str(), data, width, height, hasAlpha);
			std::vector<float> dirData(width * height * 4);
			memcpy(dirData.data(), data, width * height * 4 * sizeof(float));
			delete data;
			dirDatas.push_back(dirData);
		}
	}

	// Convert data into input format
	trainInputs.clear();
	trainTargets.clear();

	for (size_t frame = 0; frame < colorDatas.size(); frame++)
	{
		for (size_t pixel = 0; pixel < colorDatas[frame].size(); pixel++)
		{
			float color[4];
			memcpy(color, &colorDatas[frame][pixel], 4 * sizeof(float));

			float pos[4];
			memcpy(pos, &colorDatas[frame][pixel], 4 * sizeof(float));

			float dir[4];
			memcpy(dir, &colorDatas[frame][pixel], 4 * sizeof(float));

			en::NrcTarget target;
			target.r = color[0];
			target.g = color[1];
			target.b = color[2];
			target.a = color[3];
			trainTargets.push_back(target);

			en::NrcInput input;
			input.x = pos[0];
			input.y = pos[1];
			input.z = pos[2];
			input.theta = dir[0];
			input.phi = dir[1];
			trainInputs.push_back(input);
		}
	}

	colorDatas.clear();
	posDatas.clear();
	dirDatas.clear();

	en::Log::Info("Training data has been loaded");
}

void TrainNrc(
	en::KomputeManager& manager,
	en::NeuralNetwork& nn,
	const std::vector<en::NrcInput>& inputs,
	const std::vector<en::NrcTarget>& targets,
	float learningRate,
	size_t count)
{
	size_t realCount = std::min(count, inputs.size());

	for (size_t i = 0; i < realCount; i++)
	{
		en::Matrix inputMat(manager, { { inputs[i].x }, { inputs[i].y }, { inputs[i].z }, { inputs[i].theta }, { inputs[i].phi } });
		en::Matrix targetMat(manager, { { targets[i].r }, { targets[i].g }, { targets[i].b }, { targets[i].a } });

		nn.Backprop(manager, inputMat, targetMat, learningRate);
	}
}

void TestNrc(
	en::KomputeManager& manager,
	en::NeuralNetwork& nn,
	const std::vector<en::NrcInput>& inputs,
	const std::vector<en::NrcTarget>& targets,
	size_t count)
{
	const size_t realCount = std::min(count, inputs.size());

	float error = 0.0f;

	for (size_t i = 0; i < realCount; i++)
	{
		if (realCount >= 10 && 0 == (i % (realCount / 10)))
		{
			en::Log::Info("Test Image: " + std::to_string(i));
		}

		en::Matrix inputMat(manager, { { inputs[i].x }, { inputs[i].y }, { inputs[i].z }, { inputs[i].theta }, { inputs[i].phi } });
		std::vector<float> targetVec = { targets[i].r, targets[i].g, targets[i].b, targets[i].a };

		en::Matrix output = nn.Forward(manager, inputMat);
		std::vector<float> outputVec = output.GetDataVector();

		for (uint8_t channel = 0; channel < 4; channel++)
		{
			float distance = targetVec[channel] - outputVec[channel];
			error += distance * distance;
		}
	}

	error /= (static_cast<float>(realCount) * 4.0f);
	en::Log::Info("Error: " + std::to_string(error));
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

	if (nrcHpmRenderer != nullptr && en::ImGuiRenderer::IsInitialized())
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

	uint32_t width = en::Window::GetWidth();
	uint32_t height = en::Window::GetHeight();
	nrcHpmRenderer->ResizeFrame(width, height);
	en::ImGuiRenderer::Resize(width, height);
	en::ImGuiRenderer::SetBackgroundImageView(nrcHpmRenderer->GetImageView());
}

void RunNrcHpmTrainer()
{
	VkQueue queue = en::VulkanAPI::GetComputeQueue();

	// Update
	trainVolumeData->Update(true);

	// Render
	pathTracer->Render(queue);
	VkResult result = vkQueueWaitIdle(queue);
	ASSERT_VULKAN(result);

	// Get train data
	pathTracer->ExportForTraining(queue, trainInputs, trainTargets);

	// NN learn
	nn->SyncLayersToDevice(*manager);
	TrainNrc(*manager, *nn, trainInputs, trainTargets, 0.001f, SIZE_MAX);
	nn->SyncLayersToHost(*manager);

	// Sync exit
	doneTraining = true;

	en::Log::Info("Nn update");
}

void RunNrcHpm()
{
	std::string appName("NRC-HPM-Renderer");
	uint32_t width = 512;
	uint32_t height = width;

	// Start engine
	en::Log::Info("Starting " + appName);

	en::Window::Init(width, height, true, appName);
	en::Input::Init(en::Window::GetGLFWHandle());
	en::VulkanAPI::Init(appName);

	{
		// Load data
		auto density3D = en::ReadFileDensity3D("data/cloud_sixteenth", 125, 85, 153);
		en::vk::Texture3D density3DTex(density3D, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER);
		en::VolumeData volumeData(&density3DTex);
		trainVolumeData = new en::VolumeData(&density3DTex);

		// Setup rendering
		en::Camera camera(
			glm::vec3(0.0f, 0.0f, -5.0f),
			glm::vec3(0.0f, 0.0f, 1.0f),
			glm::vec3(0.0f, 1.0f, 0.0f),
			static_cast<float>(width) / static_cast<float>(height),
			glm::radians(60.0f),
			0.1f,
			100.0f);

		en::Sun sun(-1.57f, 0.0f, glm::vec3(1.0f));

		en::vk::Swapchain swapchain(width, height, RecordSwapchainCommandBuffer, SwapchainResizeCallback);

		pathTracer = new en::DensityPathTracer(16, 8, trainVolumeData, &sun); // To generate 128 batches
		nrcHpmRenderer = new en::NrcHpmRenderer(width, height, &camera, &volumeData, &sun);

		en::ImGuiRenderer::Init(width, height);
		en::ImGuiRenderer::SetBackgroundImageView(nrcHpmRenderer->GetImageView());

		swapchain.Resize(width, height); // Rerecords commandbuffers (needs to be called if renderer are created)

		// NN
		//LoadTrainData(trainInputs, trainTargets);

		manager = new en::KomputeManager();

		std::vector<en::Layer*> layers = {
			new en::LinearLayer(*manager, 5, 64), // 0
			new en::SigmoidLayer(*manager, 64),
			new en::LinearLayer(*manager, 64, 64), // 1
			new en::ReluLayer(*manager, 64),
			new en::LinearLayer(*manager, 64, 64), // 2
			new en::ReluLayer(*manager, 64),
			new en::LinearLayer(*manager, 64, 64), // 3
			new en::ReluLayer(*manager, 64),
			new en::LinearLayer(*manager, 64, 64), // 4
			new en::ReluLayer(*manager, 64),
			new en::LinearLayer(*manager, 64, 4), // 5
			new en::SigmoidLayer(*manager, 4) };

		nn = new en::NeuralNetwork(layers);

		doneTraining = false;
		std::thread* trainerThread = new std::thread(RunNrcHpmTrainer);

		// Main loop
		VkDevice device = en::VulkanAPI::GetDevice();
		VkQueue graphicsQueue = en::VulkanAPI::GetGraphicsQueue();
		VkResult result;
		size_t counter = 0;
		while (!en::Window::IsClosed())
		{
			//nn.SyncLayersToDevice(manager);
			//TrainNrc(manager, nn, distr, trainInputs, trainTargets, 0.1f, 16);
			//nn.SyncLayersToHost(manager);

			// Update
			en::Window::Update();
			en::Input::Update();
			en::Time::Update();

			width = en::Window::GetWidth();
			height = en::Window::GetHeight();

			float deltaTime = static_cast<float>(en::Time::GetDeltaTime());
			uint32_t fps = en::Time::GetFps();
			en::Input::HandleUserCamInput(&camera, deltaTime);
			en::Window::SetTitle(appName + " | Delta time: " + std::to_string(deltaTime) + "s | Fps: " + std::to_string(fps));

			// Physics
			camera.SetAspectRatio(width, height);
			camera.UpdateUniformBuffer();

			// Render
			if (doneTraining)
			{
				doneTraining = false;
				trainerThread->join();
				delete trainerThread;

				nrcHpmRenderer->UpdateNnData(*manager, *nn);
				
				trainerThread = new std::thread(RunNrcHpmTrainer);
			}

			nrcHpmRenderer->Render(graphicsQueue);
			result = vkQueueWaitIdle(graphicsQueue);
			ASSERT_VULKAN(result);

			en::ImGuiRenderer::StartFrame();

			volumeData.RenderImGui();
			volumeData.Update(camera.HasChanged());
			sun.RenderImgui();

			en::ImGuiRenderer::EndFrame(graphicsQueue);
			result = vkQueueWaitIdle(graphicsQueue);
			ASSERT_VULKAN(result);

			swapchain.DrawAndPresent();

			//		if (0 == (counter % 100))
			//		{
			//			pathTracer->ExportImageToHost(graphicsQueue, en::Time::GetTimeStamp());
			//		}

			counter++;
		}
		result = vkDeviceWaitIdle(device);
		ASSERT_VULKAN(result);

		trainerThread->join();
		delete trainerThread;

		delete nn;
		delete manager;

		// End
		density3DTex.Destroy();

		trainVolumeData->Destroy();
		delete trainVolumeData;
		volumeData.Destroy();

		en::ImGuiRenderer::Shutdown();

		nrcHpmRenderer->Destroy();
		delete nrcHpmRenderer;

		swapchain.Destroy(true);

		camera.Destroy();

		sun.Destroy();
	}

	en::VulkanAPI::Shutdown();
	en::Window::Shutdown();

	en::Log::Info("Ending " + appName);
}

int main()
{
	RunNrcHpm();
	
	return 0;
}
