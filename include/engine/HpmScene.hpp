#pragma once

#include <vector>
#include <engine/graphics/DirLight.hpp>
#include <engine/graphics/PointLight.hpp>
#include <engine/graphics/HdrEnvMap.hpp>
#include <engine/graphics/vulkan/Texture3D.hpp>
#include <engine/objects/VolumeData.hpp>

namespace en
{
	class HpmScene
	{
	public:
		static const std::vector<VkDescriptorSetLayout>& GetDescriptorSetLayout();
		
		HpmScene();
		
		void Update(bool renderImgui);

		void Destroy();

		const std::vector<VkDescriptorSet>& GetDescriptorSets() const;

	private:
		static std::vector<VkDescriptorSetLayout> s_DescriptorSetLayout;

		DirLight* m_DirLight = nullptr;
		PointLight* m_PointLight = nullptr;
		HdrEnvMap* m_HdrEnvMap = nullptr;

		vk::Texture3D* m_Density3DTex = nullptr;
		VolumeData* m_VolumeData = nullptr;

		std::vector<VkDescriptorSet> m_DescSets;
	};
}
