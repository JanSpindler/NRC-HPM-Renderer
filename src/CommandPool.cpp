#include <engine/graphics/vulkan/CommandPool.hpp>
#include <engine/graphics/VulkanAPI.hpp>

namespace en::vk
{
	CommandPool::CommandPool(VkCommandPoolCreateFlags flags, uint32_t queueFamilyIndex)
	{
		VkCommandPoolCreateInfo createInfo;
		createInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		createInfo.pNext = nullptr;
		createInfo.flags = flags;
		createInfo.queueFamilyIndex = queueFamilyIndex;

		VkResult result = vkCreateCommandPool(VulkanAPI::GetDevice(), &createInfo, nullptr, &m_Handle);
		ASSERT_VULKAN(result);
	}

	void CommandPool::Destroy()
	{
		FreeBuffers();
		vkDestroyCommandPool(VulkanAPI::GetDevice(), m_Handle, nullptr);
	}

	void CommandPool::AllocateBuffers(uint32_t bufferCount, VkCommandBufferLevel level)
	{
		VkCommandBufferAllocateInfo allocateInfo;
		allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocateInfo.pNext = nullptr;
		allocateInfo.commandPool = m_Handle;
		allocateInfo.level = level;
		allocateInfo.commandBufferCount = bufferCount;

		std::vector<VkCommandBuffer> newBuffers(bufferCount);
		VkResult result = vkAllocateCommandBuffers(VulkanAPI::GetDevice(), &allocateInfo, newBuffers.data());
		ASSERT_VULKAN(result);

		for (VkCommandBuffer commandBuffer : newBuffers)
			m_Buffers.push_back(commandBuffer);
	}

	void CommandPool::FreeBuffers()
	{
		uint32_t bufferCount = m_Buffers.size();
		if (bufferCount > 0)
		{
			vkFreeCommandBuffers(VulkanAPI::GetDevice(), m_Handle, bufferCount, m_Buffers.data());
			m_Buffers.clear();
		}
	}

	uint32_t CommandPool::GetBufferCount() const
	{
		return m_Buffers.size();
	}

	const std::vector<VkCommandBuffer>& CommandPool::GetBuffers() const
	{
		return m_Buffers;
	}

	VkCommandBuffer CommandPool::GetBuffer(uint32_t index) const
	{
		return m_Buffers[index];
	}
}
