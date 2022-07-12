#pragma once

#include <vector>
#include <engine/graphics/common.hpp>

namespace en
{
	class HdrEnvMap
	{
	public:
		HdrEnvMap(const std::vector<float>& hdr4f, uint32_t width, uint32_t height);

		void Destroy();

	private:
		uint32_t m_Width;
		uint32_t m_Height;
		VkDeviceSize m_RawSize;

		VkImage m_Image;
		VkImageView m_ImageView;
		VkDeviceMemory m_DeviceMemory;
		VkImageLayout m_ImageLayout;
		VkSampler m_Sampler;

		void ChangeLayout(VkImageLayout layout, VkCommandBuffer commandBuffer, VkQueue queue);
		void WriteBufferToImage(VkCommandBuffer commandBuffer, VkQueue queue, VkBuffer buffer);
	};
}
