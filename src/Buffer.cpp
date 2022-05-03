#include <engine/graphics/vulkan/Buffer.hpp>
#include <engine/graphics/vulkan/CommandPool.hpp>
#include <cstring>

namespace en::vk
{
	void Buffer::Copy(const Buffer* src, Buffer* dest, VkDeviceSize size)
	{
		VkQueue queue = VulkanAPI::GetGraphicsQueue();

		CommandPool commandPool(0, VulkanAPI::GetGraphicsQFI());
		commandPool.AllocateBuffers(1, VK_COMMAND_BUFFER_LEVEL_PRIMARY);
		VkCommandBuffer commandBuffer = commandPool.GetBuffer(0);

		VkCommandBufferBeginInfo beginInfo;
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.pNext = nullptr;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		beginInfo.pInheritanceInfo = nullptr;

		VkResult result = vkBeginCommandBuffer(commandBuffer, &beginInfo);
		ASSERT_VULKAN(result);

		VkBufferCopy bufferCopy;
		bufferCopy.srcOffset = 0;
		bufferCopy.dstOffset = 0;
		bufferCopy.size = size;

		vkCmdCopyBuffer(commandBuffer, src->GetVulkanHandle(), dest->GetVulkanHandle(), 1, &bufferCopy);

		result = vkEndCommandBuffer(commandBuffer);
		ASSERT_VULKAN(result);

		VkSubmitInfo submitInfo;
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.pNext = nullptr;
		submitInfo.waitSemaphoreCount = 0;
		submitInfo.pWaitSemaphores = nullptr;
		submitInfo.pWaitDstStageMask = nullptr;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffer;
		submitInfo.signalSemaphoreCount = 0;
		submitInfo.pSignalSemaphores = nullptr;

		result = vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
		ASSERT_VULKAN(result);

		result = vkQueueWaitIdle(queue);
		ASSERT_VULKAN(result);

		commandPool.Destroy();
	}

	Buffer::Buffer(VkDeviceSize size, VkMemoryPropertyFlags memoryProperties, VkBufferUsageFlags usage, const std::vector<uint32_t>& qfis) :
		m_Mapped(false),
		m_UsedSize(size)
	{
		VkDevice device = VulkanAPI::GetDevice();

		// Create
		VkBufferCreateInfo createInfo;
		createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		createInfo.pNext = nullptr;
		createInfo.flags = 0;
		createInfo.size = size;
		createInfo.usage = usage;
		if (qfis.size() == 0)
		{
			createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
			createInfo.queueFamilyIndexCount = 0;
			createInfo.pQueueFamilyIndices = nullptr;
		}
		else
		{
			createInfo.sharingMode = VK_SHARING_MODE_CONCURRENT;
			createInfo.queueFamilyIndexCount = qfis.size();
			createInfo.pQueueFamilyIndices = qfis.data();
		}

		VkResult result = vkCreateBuffer(device, &createInfo, nullptr, &m_VulkanHandle);
		ASSERT_VULKAN(result);

		// Find Memory Type
		VkMemoryRequirements memoryRequirements;
		vkGetBufferMemoryRequirements(device, m_VulkanHandle, &memoryRequirements);
		uint32_t memoryTypeIndex = VulkanAPI::FindMemoryType(memoryRequirements.memoryTypeBits, memoryProperties);

		// Allocate Memory
		VkMemoryAllocateInfo allocateInfo;
		allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocateInfo.pNext = nullptr;
		allocateInfo.allocationSize = memoryRequirements.size;
		allocateInfo.memoryTypeIndex = memoryTypeIndex;

		result = vkAllocateMemory(device, &allocateInfo, nullptr, &m_DeviceMemory);
		ASSERT_VULKAN(result);

		// Bind Memory
		result = vkBindBufferMemory(device, m_VulkanHandle, m_DeviceMemory, 0);
		ASSERT_VULKAN(result);
	}

	void Buffer::Destroy()
	{
		VkDevice device = VulkanAPI::GetDevice();

		vkFreeMemory(device, m_DeviceMemory, nullptr);
		vkDestroyBuffer(device, m_VulkanHandle, nullptr);
	}

	void Buffer::MapMemory(VkDeviceSize offset, void** memory)
	{
		if (m_Mapped)
			Log::Error("Vulkan Device Memory is already mapped", true);

		VkResult result = vkMapMemory(VulkanAPI::GetDevice(), m_DeviceMemory, offset, m_UsedSize, 0, memory);
		ASSERT_VULKAN(result);

		m_Mapped = true;
	}

	void Buffer::UnmapMemory()
	{
		if (!m_Mapped)
			Log::Warn("Vulkan Device Memory was not mapped");

		vkUnmapMemory(VulkanAPI::GetDevice(), m_DeviceMemory);

		m_Mapped = false;
	}

	VkBuffer Buffer::GetVulkanHandle() const
	{
		return m_VulkanHandle;
	}

	VkDeviceSize Buffer::GetUsedSize() const
	{
		return m_UsedSize;
	}

	void Buffer::GetData(VkDeviceSize size, void* dst, VkDeviceSize offset, VkMemoryMapFlags mapFlags)
	{
		VkDevice device = VulkanAPI::GetDevice();
		
		void* mappedMemory;
		MapMemory(0, &mappedMemory);

		memcpy(dst, mappedMemory, static_cast<size_t>(size));

		UnmapMemory();
	}

	void Buffer::SetData(VkDeviceSize size, const void* data, VkDeviceSize offset, VkMemoryMapFlags mapFlags)
	{
		VkDevice device = VulkanAPI::GetDevice();
		VkResult result;

		void* mappedMemory;
		result = vkMapMemory(device, m_DeviceMemory, offset, size, mapFlags, &mappedMemory);
		ASSERT_VULKAN(result);

		memcpy(mappedMemory, data, static_cast<size_t>(size));

		vkUnmapMemory(device, m_DeviceMemory);
	}

}
