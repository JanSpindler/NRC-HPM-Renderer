#pragma once
#include <engine/graphics/vulkan/Buffer.hpp>
#include <glm/common.hpp>
#include <glm/glm.hpp>

// properly aligned by default!
struct DirLightData
{
	glm::vec3 m_Color;
	float m_Zenith;
	glm::vec3 m_Dir;
	float m_Azimuth;
	float m_Strenth;
};

namespace en
{
	class DirLight
	{
	public:
		static void Init();
		static void Shutdown();
		static VkDescriptorSetLayout GetDescriptorSetLayout();

		DirLight(float zenith, float azimuth, glm::vec3 color, float strength);

		void Destroy();

		float GetZenith() const;
		float GetAzimuth() const;

		void SetZenith(float z);
		void SetAzimuth(float a);
		void SetColor(glm::vec3 c);

		VkDescriptorSet GetDescriptorSet() const;

		void RenderImgui();

	private:
		static VkDescriptorSetLayout m_DescriptorSetLayout;
		static VkDescriptorPool m_Pool;

		DirLightData m_DirLightData;
		VkDescriptorSet m_DescriptorSet;
		vk::Buffer m_UniformBuffer;

		void UpdateBuffer();
	};
}
