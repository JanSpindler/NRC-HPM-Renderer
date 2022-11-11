#include <engine/graphics/vulkan/Instance.hpp>
#include <engine/graphics/Window.hpp>

namespace en::vk
{
	Instance::Instance() {}

	Instance::Instance(const std::string& appName, uint32_t apiVersion)
	{
		ListSupportedLayers();
		ListSupportedExtensions();

		// Select wanted layers
		std::vector<const char*> layers = {
#ifdef _DEBUG
			"VK_LAYER_KHRONOS_validation"
#endif
		};

		// Select wanted extensions
		std::vector<const char*> extensions;
		if (Window::IsSupported()) { extensions = Window::GetVulkanExtensions(); }
		extensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
		extensions.push_back(VK_KHR_EXTERNAL_MEMORY_CAPABILITIES_EXTENSION_NAME);
		extensions.push_back(VK_KHR_EXTERNAL_SEMAPHORE_CAPABILITIES_EXTENSION_NAME);

		// Create
		VkApplicationInfo appInfo;
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pNext = nullptr;
		appInfo.pApplicationName = appName.c_str();
		appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.pEngineName = "Engine";
		appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.apiVersion = VK_API_VERSION_1_3;

		VkInstanceCreateInfo createInfo;
		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pNext = nullptr;
		createInfo.flags = 0;
		createInfo.pApplicationInfo = &appInfo;
		createInfo.enabledLayerCount = layers.size();
		createInfo.ppEnabledLayerNames = layers.data();
		createInfo.enabledExtensionCount = extensions.size();
		createInfo.ppEnabledExtensionNames = extensions.data();

		VkResult result = vkCreateInstance(&createInfo, nullptr, &m_VkHandle);
		ASSERT_VULKAN(result);
	}

	void Instance::Destroy()
	{
		vkDestroyInstance(m_VkHandle, nullptr);
	}

	VkInstance Instance::GetVkHandle() const
	{
		return m_VkHandle;
	}

	void Instance::ListSupportedLayers()
	{
		// List supported layers
		uint32_t supportedLayerCount;
		vkEnumerateInstanceLayerProperties(&supportedLayerCount, nullptr);
		std::vector<VkLayerProperties> supportedLayers(supportedLayerCount);
		vkEnumerateInstanceLayerProperties(&supportedLayerCount, supportedLayers.data());

		Log::Info("Supported Instance Layers:");
		for (const VkLayerProperties& layer : supportedLayers)
			Log::Info("\t-" + std::string(layer.layerName) + " | " + std::string(layer.description));
	}

	void Instance::ListSupportedExtensions()
	{
		// List supported extensions
		uint32_t supportedExtensionCount;
		vkEnumerateInstanceExtensionProperties(nullptr, &supportedExtensionCount, nullptr);
		std::vector<VkExtensionProperties> supportedExtensions(supportedExtensionCount);
		vkEnumerateInstanceExtensionProperties(nullptr, &supportedExtensionCount, supportedExtensions.data());

		Log::Info("Supported Instance Extensions:");
		for (const VkExtensionProperties& extension : supportedExtensions)
			Log::Info("\t-" + std::string(extension.extensionName));
	}
}
