#pragma once

#include <engine/graphics/vulkan/Texture3D.hpp>
#include <glm/glm.hpp>
#include <engine/graphics/vulkan/Buffer.hpp>
#include <engine/graphics/Camera.hpp>

namespace en
{
	class VolumeData
	{
	public:
		static void Init(VkDevice device);
		static void Shutdown(VkDevice device);
		static VkDescriptorSetLayout GetDescriptorSetLayout();

		VolumeData(const vk::Texture3D* densityTex, float densityFactor, float g);

		void Destroy();

		void RenderImGui();

		float GetDensityFactor() const;
		float GetG() const;
		VkDescriptorSet GetDescriptorSet() const;
		VkExtent3D GetExtent() const;

	private:
		static VkDescriptorSetLayout m_DescriptorSetLayout;
		static VkDescriptorPool m_DescriptorPool;

		float m_DensityFactor = 0.0;
		float m_G = 0.0;

		VkDescriptorSet m_DescriptorSet;

		const vk::Texture3D* m_DensityTex;

		void UpdateDescriptorSet();
	};
}
