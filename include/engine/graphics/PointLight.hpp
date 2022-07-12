#pragma once

#include <engine/graphics/vulkan/Buffer.hpp>
#include <glm/glm.hpp>

namespace en
{
	class PointLight
	{
	public:
		static void Init(VkDevice device);
		static void Shutdown(VkDevice device);
		static VkDescriptorSetLayout GetDescriptorSetLayout();

		PointLight(const glm::vec3& pos, const glm::vec3& color, float stength);

		void Destroy();

		void RenderImGui();

		VkDescriptorSet GetDescriptorSet() const;

	private:
		static VkDescriptorSetLayout m_DescSetLayout;
		static VkDescriptorPool m_DescPool;

		struct UniformData
		{
			glm::vec3 pos;
			float strength;
			glm::vec3 color;
		};

		UniformData m_UniformData;
		vk::Buffer m_UniformBuffer;
		VkDescriptorSet m_DescSet;
	};
}
