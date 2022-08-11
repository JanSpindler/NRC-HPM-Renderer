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

		VkImage m_ColorImage;
		VkImageView m_ColorImageView;
		VkDeviceMemory m_ColorImageMemory;
		VkImageLayout m_ColorImageLayout;
		
		// Cdf of X
		VkImage m_CdfXImage;
		VkImageView m_CdfXImageView;
		VkDeviceMemory m_CdfXImageMemory;
		VkImageLayout m_CdfXImageLayout;
		
		// Cdf of Y given X
		VkImage m_CdfYImage;
		VkImageView m_CdfYImageView;
		VkDeviceMemory m_CdfYImageMemory;
		VkImageLayout m_CdfYImageLayout;

		VkSampler m_Sampler;

		UniformData m_UniformData;
		vk::Buffer m_UniformBuffer;

		VkDescriptorSet m_DescSet;

		void CreateColorImage(VkDevice device);
		void CreateCdfXImage(VkDevice device);
		void CreateCdfYImage(VkDevice device);

		void ChangeColorImageLayout(VkImageLayout layout, VkCommandBuffer commandBuffer, VkQueue queue);
		void WriteBufferToColorImage(VkCommandBuffer commandBuffer, VkQueue queue, VkBuffer buffer);
	};
}
