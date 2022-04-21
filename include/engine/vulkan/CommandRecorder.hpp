#pragma once

#include <engine/common.hpp>

namespace en::vk
{
	class CommandRecorder
	{
	public:
		static void ImageLayoutTransfer(
			VkCommandBuffer commandBuffer,
			VkImage image,
			VkImageLayout srcLayout,
			VkImageLayout dstLayout,
			VkAccessFlags srcAccessMask,
			VkAccessFlags dstAccessMask,
			VkPipelineStageFlags srcStageMask,
			VkPipelineStageFlags dstStageMask);
	};
}
