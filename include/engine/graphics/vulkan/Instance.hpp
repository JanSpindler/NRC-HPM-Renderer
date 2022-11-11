#pragma once

#include <engine/graphics/common.hpp>

namespace en::vk
{
	class Instance
	{
	public:
		Instance();
		Instance(const std::string& appName, uint32_t apiVersion);

		void Destroy();

		VkInstance GetVkHandle() const;

	private:
		VkInstance m_VkHandle = VK_NULL_HANDLE;

		void ListSupportedLayers();
		void ListSupportedExtensions();
	};
}
