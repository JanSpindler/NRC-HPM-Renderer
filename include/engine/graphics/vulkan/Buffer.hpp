#pragma once

#include <engine/graphics/VulkanAPI.hpp>

typedef void* HANDLE;

namespace en::vk
{
	class Buffer
	{
	public:
		static void Copy(const Buffer* src, Buffer* dest, VkDeviceSize size);

		Buffer(
			VkDeviceSize size, 
			VkMemoryPropertyFlags memoryProperties, 
			VkBufferUsageFlags usage, 
			const std::vector<uint32_t>& qfis,
			VkExternalMemoryHandleTypeFlagBits extMemType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_FLAG_BITS_MAX_ENUM);

		void Destroy();

		void MapMemory(VkDeviceSize offset, void** memory);
		void UnmapMemory();

		VkBuffer GetVulkanHandle() const;
		VkDeviceSize GetUsedSize() const;
		void GetData(VkDeviceSize size, void* dst, VkDeviceSize offset, VkMemoryMapFlags mapFlags);

#ifdef _WIN64
		HANDLE GetMemoryWin32Handle() const;
#else
		int GetMemoryFd() const;
#endif

		void SetData(VkDeviceSize size, const void* data, VkDeviceSize offset, VkMemoryMapFlags mapFlags);

	private:
		bool m_Mapped;

		VkBuffer m_VulkanHandle;
		VkDeviceMemory m_DeviceMemory;
		VkDeviceSize m_UsedSize;

#ifdef _WIN64
		HANDLE m_Win32Handle = NULL;
#else
		int m_Fd = 0;
#endif
	};
}
