#pragma once

#include <glm/glm.hpp>
#include <engine/graphics/vulkan/Buffer.hpp>
#include <engine/graphics/vulkan/Texture2D.hpp>

namespace en
{
	class Material
	{
	public:
		static const uint32_t MAX_COUNT = 8192;

		static void Init();
		static void Shutdown();

		static VkDescriptorSetLayout GetDescriptorSetLayout();

		Material(const glm::vec4& diffuseColor, const vk::Texture2D* diffuseTex);

		void Destroy();

		const glm::vec4& GetDiffuseColor() const;
		void SetDiffuseColor(const glm::vec4& diffuseColor);

		const vk::Texture2D* GetDiffuseTex() const;
		void SetDiffuseTex(const vk::Texture2D* diffuseTex);

		VkDescriptorSet GetDescriptorSet() const;

	private:
		struct UniformData
		{
			glm::vec4 diffuseColor;
			uint32_t useDiffuseTex;
		};

		static VkDescriptorSetLayout m_DescriptorSetLayout;
		static VkDescriptorPool m_DescriptorPool;

		UniformData m_UniformData;
		const vk::Texture2D* m_DiffuseTex;

		vk::Buffer* m_UniformBuffer;
		VkDescriptorSet m_DescriptorSet;

		void UpdateUniformBuffer() const;
	};
}
