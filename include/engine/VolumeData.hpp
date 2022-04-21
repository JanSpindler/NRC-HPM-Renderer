#pragma once

#include <engine/vulkan/Texture3D.hpp>

namespace en
{
	class VolumeData
	{
	public:
		static void Init(VkDevice device);
		static void Shutdown(VkDevice device);
		static VkDescriptorSetLayout GetDescriptorSetLayout();

		VolumeData(const vk::Texture3D* densityTex);

		void Destroy();

		void RenderImGui();

		VkDescriptorSet GetDescriptorSet() const;

	private:
		static VkDescriptorSetLayout m_DescriptorSetLayout;
		static VkDescriptorPool m_DescriptorPool;

		const vk::Texture3D* m_DensityTex;
	};
}
