#pragma once

#include <engine/common.hpp>
#include <vector>
#include <engine/vulkan/CommandPool.hpp>

namespace en::vk
{
	class Swapchain
	{
	public:
		Swapchain(
			uint32_t width,
			uint32_t height,
			void (*recordCommandBufferFunc)(VkCommandBuffer, VkImage),
			void (*resizeCallback)());

		void Destroy(bool destroySwapchain);

		void Resize(uint32_t width, uint32_t height);
		void DrawAndPresent();

		VkSwapchainKHR GetHandle() const;

	private:
		VkSwapchainKHR m_Handle;

		void (*m_ResizeCallback)();
		uint32_t m_Width;
		uint32_t m_Height;

		uint32_t m_ImageCount;
		std::vector<VkImage> m_Images;

		void (*m_RecordCommandBufferFunc)(VkCommandBuffer, VkImage);
		CommandPool m_CommandPool;
		VkSemaphore m_ImageAvailableSemaphore;
		VkSemaphore m_RenderFinishedSemaphore;

		void CreateSwapchain(VkDevice device, VkSurfaceFormatKHR surfaceFormat, uint32_t width, uint32_t height, VkSwapchainKHR oldSwapchain);
		void RetreiveImages(VkDevice device);
		void CreateCommandBuffers();
		void CreateSyncObjects(VkDevice device);
	};
}
