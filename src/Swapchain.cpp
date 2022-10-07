#include <engine/graphics/vulkan/Swapchain.hpp>
#include <engine/graphics/VulkanAPI.hpp>
#include <engine/graphics/Window.hpp>
#include <engine/util/Log.hpp>

namespace en::vk
{
	Swapchain::Swapchain(
		uint32_t width,
		uint32_t height,
		void (*recordCommandBufferFunc)(VkCommandBuffer, VkImage),
		void (*resizeCallback)())
		:
		m_CommandPool(0, VulkanAPI::GetGraphicsQFI()),
		m_RecordCommandBufferFunc(recordCommandBufferFunc),
		m_ResizeCallback(resizeCallback),
		m_Width(width),
		m_Height(height)
	{
		VkDevice device = VulkanAPI::GetDevice();
		m_ImageCount = VulkanAPI::GetSwapchainImageCount();
		VkSurfaceFormatKHR surfaceFormat = VulkanAPI::GetSurfaceFormat();

		CreateSwapchain(device, surfaceFormat, width, height, VK_NULL_HANDLE);
		RetreiveImages(device);
		CreateCommandBuffers();
		CreateSyncObjects(device);
	}

	void Swapchain::Destroy(bool destroySwapchain)
	{
		VkDevice device = VulkanAPI::GetDevice();

		vkDestroySemaphore(device, m_ImageAvailableSemaphore, nullptr);
		vkDestroySemaphore(device, m_RenderFinishedSemaphore, nullptr);

		m_CommandPool.Destroy();

		if (destroySwapchain)
			vkDestroySwapchainKHR(device, m_Handle, nullptr);

		m_Images.clear();
	}

	void Swapchain::Resize(uint32_t width, uint32_t height)
	{
		Destroy(false);

		VkSwapchainKHR oldSwapchain = m_Handle;

		m_Width = width;
		m_Height = height;

		VkDevice device = VulkanAPI::GetDevice();
		m_ImageCount = VulkanAPI::GetSwapchainImageCount();
		VkSurfaceFormatKHR surfaceFormat = VulkanAPI::GetSurfaceFormat();

		CreateSwapchain(device, surfaceFormat, width, height, oldSwapchain); // TODO: reuse old swapchain
		RetreiveImages(device);
		m_CommandPool = CommandPool(0, VulkanAPI::GetGraphicsQFI());
		CreateCommandBuffers();
		CreateSyncObjects(device);

		vkDestroySwapchainKHR(device, oldSwapchain, nullptr);
	}

	void Swapchain::DrawAndPresent(VkSemaphore waitSemaphore, VkSemaphore signalSemaphore) // TODO: use one image for drawing and another one for presenting
	{
		VkDevice device = VulkanAPI::GetDevice();
		VkQueue graphicsQueue = VulkanAPI::GetGraphicsQueue();
		VkQueue presentQueue = VulkanAPI::GetPresentQueue();

		uint32_t newWidth = Window::GetWidth();
		uint32_t newHeight = Window::GetHeight();
		bool resized = false;
		if (newWidth != m_Width || newHeight != m_Height)
			resized = true;

		// Aquire image from swapchain
		uint32_t imageIndex;
		VkResult result = vkAcquireNextImageKHR(device, m_Handle, UINT64_MAX, m_ImageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);
		if (resized || result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
		{
			m_ResizeCallback();
			Resize(Window::GetWidth(), Window::GetHeight());
			return;
		}
		else if (result != VK_SUCCESS)
		{
			Log::Error("Failed to aquire next swapchain VkImage", true);
		}

		// Submit correct CommandBuffer
		std::vector<VkSemaphore> renderWaitSemaphores = { m_ImageAvailableSemaphore };
		std::vector<VkPipelineStageFlags> renderWaitStages = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		if (waitSemaphore != VK_NULL_HANDLE)
		{
			renderWaitSemaphores.push_back(waitSemaphore);
			renderWaitStages.push_back(VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
		}
		std::vector<VkSemaphore> renderSignalSemaphores = { m_RenderFinishedSemaphore };
		if (signalSemaphore != VK_NULL_HANDLE)
		{
			renderSignalSemaphores.push_back(signalSemaphore);
		}
		std::vector<VkSemaphore> presentWaitSemaphores = { m_RenderFinishedSemaphore };

		VkCommandBuffer commandBuffer = m_CommandPool.GetBuffer(imageIndex);

		VkSubmitInfo submitInfo;
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.pNext = nullptr;
		submitInfo.waitSemaphoreCount = renderWaitSemaphores.size();
		submitInfo.pWaitSemaphores = renderWaitSemaphores.data();
		submitInfo.pWaitDstStageMask = renderWaitStages.data();
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffer;
		submitInfo.signalSemaphoreCount = renderSignalSemaphores.size();
		submitInfo.pSignalSemaphores = renderSignalSemaphores.data();

		result = vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
		ASSERT_VULKAN(result);

		// Presentation
		VkPresentInfoKHR presentInfo;
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.pNext = nullptr;
		presentInfo.waitSemaphoreCount = presentWaitSemaphores.size();
		presentInfo.pWaitSemaphores = presentWaitSemaphores.data();
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = &m_Handle;
		presentInfo.pImageIndices = &imageIndex;
		presentInfo.pResults = nullptr;

		result = vkQueuePresentKHR(presentQueue, &presentInfo);
		ASSERT_VULKAN(result);

		result = vkQueueWaitIdle(graphicsQueue);
		ASSERT_VULKAN(result);
	}

	VkSwapchainKHR Swapchain::GetHandle() const
	{
		return m_Handle;
	}

	void Swapchain::CreateSwapchain(VkDevice device, VkSurfaceFormatKHR surfaceFormat, uint32_t width, uint32_t height, VkSwapchainKHR oldSwapchain)
	{
		uint32_t graphicsQFI = VulkanAPI::GetGraphicsQFI();
		uint32_t presentQFI = VulkanAPI::GetPresentQFI();

		VkSwapchainCreateInfoKHR createInfo;
		createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		createInfo.pNext = nullptr;
		createInfo.flags = 0;
		createInfo.surface = VulkanAPI::GetSurface();
		createInfo.minImageCount = m_ImageCount;
		createInfo.imageFormat = surfaceFormat.format;
		createInfo.imageColorSpace = surfaceFormat.colorSpace;
		createInfo.imageExtent = { width, height };
		createInfo.imageArrayLayers = 1;
		createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		if (graphicsQFI == presentQFI)
		{
			createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
			createInfo.queueFamilyIndexCount = 0;
			createInfo.pQueueFamilyIndices = nullptr;
		}
		else
		{
			std::vector<uint32_t> queueFamilyIndices = { graphicsQFI, presentQFI };
			createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
			createInfo.queueFamilyIndexCount = queueFamilyIndices.size();
			createInfo.pQueueFamilyIndices = queueFamilyIndices.data();
		}
		createInfo.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
		createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		createInfo.presentMode = VulkanAPI::GetPresentMode();
		createInfo.clipped = VK_TRUE;
		createInfo.oldSwapchain = oldSwapchain;

		VkResult result = vkCreateSwapchainKHR(device, &createInfo, nullptr, &m_Handle);
		ASSERT_VULKAN(result);
	}

	void Swapchain::RetreiveImages(VkDevice device)
	{
		vkGetSwapchainImagesKHR(device, m_Handle, &m_ImageCount, nullptr);
		m_Images.resize(m_ImageCount);
		vkGetSwapchainImagesKHR(device, m_Handle, &m_ImageCount, m_Images.data());
	}

	void Swapchain::CreateCommandBuffers()
	{
		m_CommandPool.AllocateBuffers(m_ImageCount, VK_COMMAND_BUFFER_LEVEL_PRIMARY);
		for (uint32_t i = 0; i < m_ImageCount; i++)
			m_RecordCommandBufferFunc(m_CommandPool.GetBuffer(i), m_Images[i]);
	}

	void Swapchain::CreateSyncObjects(VkDevice device)
	{
		VkSemaphoreCreateInfo semaphoreCreateInfo;
		semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		semaphoreCreateInfo.pNext = nullptr;
		semaphoreCreateInfo.flags = 0;

		VkResult result = vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &m_ImageAvailableSemaphore);
		ASSERT_VULKAN(result);

		result = vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &m_RenderFinishedSemaphore);
		ASSERT_VULKAN(result);
	}
}
