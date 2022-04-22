#pragma once

#include <engine/graphics/VulkanAPI.hpp>

namespace en::vk
{
	class Buffer
	{
	public:
		static void Copy(const Buffer* src, Buffer* dest, VkDeviceSize size);

		Buffer(VkDeviceSize size, VkMemoryPropertyFlags memoryProperties, VkBufferUsageFlags usage, const std::vector<uint32_t>& qfis);

		void Destroy();
		void MapMemory(VkDeviceSize size, const void* data, VkDeviceSize offset, VkMemoryMapFlags mapFlags);
		void GetData(VkDeviceSize size, void* dst, VkDeviceSize offset, VkMemoryMapFlags mapFlags);

		VkBuffer GetVulkanHandle() const;

		VkDeviceSize GetUsedSize() const;

	private:
		VkBuffer m_VulkanHandle;
		VkDeviceMemory m_DeviceMemory;
		VkDeviceSize m_UsedSize;
	};
}
