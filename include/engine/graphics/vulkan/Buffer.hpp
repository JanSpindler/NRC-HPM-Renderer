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

		void MapMemory(VkDeviceSize offset, void** memory);
		void UnmapMemory();

		VkBuffer GetVulkanHandle() const;
		VkDeviceSize GetUsedSize() const;
		void GetData(VkDeviceSize size, void* dst, VkDeviceSize offset, VkMemoryMapFlags mapFlags);
		
		void SetData(VkDeviceSize size, const void* data, VkDeviceSize offset, VkMemoryMapFlags mapFlags);

	private:
		bool m_Mapped;

		VkBuffer m_VulkanHandle;
		VkDeviceMemory m_DeviceMemory;
		VkDeviceSize m_UsedSize;
	};
}
