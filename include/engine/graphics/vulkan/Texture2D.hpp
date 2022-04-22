#pragma once

#include <engine/graphics/common.hpp>
#include <vector>
#include <array>

namespace en::vk
{
	class Texture2D
	{
	public:
		static void Init();
		static void Shutdown();

		static const Texture2D* GetDummyTex();

		Texture2D(const std::string& fileName, VkFilter filter, VkSamplerAddressMode addressMode);
		Texture2D(const std::array<std::vector<std::vector<float>>, 4>& data, VkFilter filter, VkSamplerAddressMode addressMode);

		void Destroy();

		uint32_t GetWidth() const;
		uint32_t GetHeight() const;
		uint32_t GetRealChannelCount() const;
		uint32_t GetSourceChannelCount() const;
		size_t GetSizeInBytes() const;

		VkImageView GetImageView() const;
		VkSampler GetSampler() const;

	private:
		static Texture2D* m_DummyTex;

		uint32_t m_Width;
		uint32_t m_Height;
		uint32_t m_RealChannelCount;
		uint32_t m_SourceChannelCount;

		VkImage m_Image;
		VkImageView m_ImageView;
		VkDeviceMemory m_DeviceMemory;
		VkImageLayout m_ImageLayout;
		VkSampler m_Sampler;

		void LoadToDevice(const void* data, VkFilter filter, VkSamplerAddressMode addressMode);
		void ChangeLayout(VkImageLayout layout, VkCommandBuffer commandBuffer, VkQueue queue);
		void WriteBufferToImage(VkCommandBuffer commandBuffer, VkQueue queue, VkBuffer buffer);
	};
}
