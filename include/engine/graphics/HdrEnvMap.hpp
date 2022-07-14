#pragma once

#include <vector>
#include <engine/graphics/vulkan/Buffer.hpp>

namespace en
{
	class HdrEnvMap
	{
	public:
		static void Init(VkDevice device);
		static void Shutdown(VkDevice device);
		static VkDescriptorSetLayout GetDescriptorSetLayout();

		HdrEnvMap(const std::vector<float>& hdr4f, uint32_t width, uint32_t height);

		void Destroy();

		void RenderImGui();

		VkDescriptorSet GetDescriptorSet() const;

	private:
		struct UniformData
		{
			float directStrength;
			float hpmStrength;
		};

		static VkDescriptorSetLayout m_DescSetLayout;
		static VkDescriptorPool m_DescPool;

		uint32_t m_Width;
		uint32_t m_Height;
		VkDeviceSize m_RawSize;

		VkImage m_Image;
		VkImageView m_ImageView;
		VkDeviceMemory m_DeviceMemory;
		VkImageLayout m_ImageLayout;
		VkSampler m_Sampler;

		UniformData m_UniformData;
		vk::Buffer m_UniformBuffer;

		VkDescriptorSet m_DescSet;

		void ChangeLayout(VkImageLayout layout, VkCommandBuffer commandBuffer, VkQueue queue);
		void WriteBufferToImage(VkCommandBuffer commandBuffer, VkQueue queue, VkBuffer buffer);
	};
}
