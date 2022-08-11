#pragma once

#include <engine/graphics/vulkan/Texture3D.hpp>
#include <glm/glm.hpp>
#include <engine/graphics/vulkan/Buffer.hpp>
#include <engine/graphics/Camera.hpp>

namespace en
{
	struct VolumeUniformData
	{
		glm::vec4 random;
		uint32_t useNN;
		uint32_t showNonNN;
		float densityFactor;
		float g;
		int noNnSpp;
		int withNnSpp;
	};

	class VolumeData
	{
	public:
		static void Init(VkDevice device);
		static void Shutdown(VkDevice device);
		static VkDescriptorSetLayout GetDescriptorSetLayout();

		VolumeData(const vk::Texture3D* densityTex);

		void Update(bool cameraChanged);
		void Destroy();

		void RenderImGui();

		VkDescriptorSet GetDescriptorSet() const;

	private:
		static VkDescriptorSetLayout m_DescriptorSetLayout;
		static VkDescriptorPool m_DescriptorPool;

		VkDescriptorSet m_DescriptorSet;

		const vk::Texture3D* m_DensityTex;

		VolumeUniformData m_UniformData;
		vk::Buffer m_UniformBuffer;

		void UpdateDescriptorSet();
	};
}
