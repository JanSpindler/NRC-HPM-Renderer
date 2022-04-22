#pragma once

#include <engine/graphics/common.hpp>
#include <vector>

namespace en::vk
{
	class CommandPool
	{
	public:
		CommandPool(VkCommandPoolCreateFlags flags, uint32_t queueFamilyIndex);

		void Destroy();

		void AllocateBuffers(uint32_t bufferCount, VkCommandBufferLevel level);
		void FreeBuffers();

		uint32_t GetBufferCount() const;
		const std::vector<VkCommandBuffer>& GetBuffers() const;
		VkCommandBuffer GetBuffer(uint32_t index) const;

	private:
		VkCommandPool m_Handle;
		std::vector<VkCommandBuffer> m_Buffers;
	};
}
