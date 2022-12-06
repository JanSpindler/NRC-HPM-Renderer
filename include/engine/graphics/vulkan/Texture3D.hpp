#pragma once

#include <vector>
#include <array>
#include <engine/graphics/common.hpp>

namespace en::vk
{
	class Texture3D
	{
	public:
		static Texture3D FromVDB(const std::string& fileName);

		Texture3D(
			const std::vector<std::vector<std::vector<float>>>& data, 
			VkFilter filter, 
			VkSamplerAddressMode addressMode,
			VkBorderColor borderColor);
		Texture3D(
			const std::array<std::vector<std::vector<std::vector<float>>>, 4>& data, 
			VkFilter filter, 
			VkSamplerAddressMode addressMode,
			VkBorderColor borderColor);

		void Destroy();

		uint32_t GetWidth() const;
		uint32_t GetHeight() const;
		uint32_t GetDepth() const;
		uint32_t GetRealChannelCount() const;
		size_t GetRealSizeInBytes() const;

		VkImageView GetImageView() const;
		VkSampler GetSampler() const;

	private:
		uint32_t m_Width;
		uint32_t m_Height;
		uint32_t m_Depth;
		uint32_t m_RealChannelCount;

		VkImage m_Image;
		VkImageView m_ImageView;
		VkDeviceMemory m_DeviceMemory;
		VkImageLayout m_ImageLayout;
		VkSampler m_Sampler;

		void LoadToDevice(void* data, VkFilter filter, VkSamplerAddressMode addressMode, VkBorderColor borderColor);
		void ChangeLayout(VkImageLayout layout, VkCommandBuffer commandBuffer, VkQueue queue);
		void WriteBufferToImage(VkCommandBuffer commandBuffer, VkQueue queue, VkBuffer buffer);
	};
}
