#pragma once

#include <engine/common.hpp>
#include <vector>

namespace en
{
	struct PhysicalDeviceInfo
	{
		VkPhysicalDevice vulkanHandle;
		VkPhysicalDeviceProperties properties;
		VkPhysicalDeviceFeatures features;
		VkPhysicalDeviceMemoryProperties memoryProperties;
		std::vector<VkQueueFamilyProperties> queueFamilies;
	};

	// Serves as wrapper for Vulkan initialization, shutdown and other common vulkan calls
	class VulkanAPI
	{
	public:
		static void Init(const std::string& appName);
		static void Shutdown();

		static uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
		static bool IsFormatSupported(VkFormat format, VkImageTiling imageTiling, VkFormatFeatureFlags featureFlags);
		static VkFormat FindSupportedFormat(
			const std::vector<VkFormat>& formats,
			VkImageTiling imageTiling,
			VkFormatFeatureFlags featureFlags);

		static VkInstance GetInstance();

		static VkSurfaceKHR GetSurface();
		static uint32_t GetSwapchainImageCount();
		static VkSurfaceFormatKHR GetSurfaceFormat();
		static VkPresentModeKHR GetPresentMode();

		static VkPhysicalDevice GetPhysicalDevice();
		static uint32_t GetGraphicsQFI();
		static uint32_t GetComputeQFI();
		static uint32_t GetPresentQFI();

		static VkDevice GetDevice();
		static VkQueue GetGraphicsQueue();
		static VkQueue GetComputeQueue();
		static VkQueue GetPresentQueue();

	private:
		static VkInstance m_Instance;

		static VkSurfaceKHR m_Surface;
		static VkSurfaceCapabilitiesKHR m_SurfaceCapabilities;
		static VkSurfaceFormatKHR m_SurfaceFormat;
		static VkPresentModeKHR m_PresentMode;

		static PhysicalDeviceInfo m_PhysicalDeviceInfo;
		static uint32_t m_GraphicsQFI;
		static uint32_t m_ComputeQFI;
		static uint32_t m_PresentQFI;

		static VkDevice m_Device;
		static VkQueue m_GraphicsQueue;
		static VkQueue m_ComputeQueue;
		static VkQueue m_PresentQueue;

		static void CreateInstance(const std::string& appName);
		static void PickPhysicalDevice();
		static void CreateDevice();
	};
}
