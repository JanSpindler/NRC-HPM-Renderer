#pragma once

#include <vector>
#include <engine/graphics/DirLight.hpp>
#include <engine/graphics/PointLight.hpp>
#include <engine/graphics/HdrEnvMap.hpp>
#include <engine/graphics/vulkan/Texture3D.hpp>
#include <engine/objects/VolumeData.hpp>
#include <engine/AppConfig.hpp>

namespace en
{
	class HpmScene
	{
	public:
		static const std::vector<VkDescriptorSetLayout>& GetDescriptorSetLayout();
		
		HpmScene(const AppConfig& appConfig);
		
		void Update(bool renderImgui, float deltaTime);

		void Destroy();

		bool IsDynamic() const;
		const std::vector<VkDescriptorSet>& GetDescriptorSets() const;
		const VolumeData* GetVolumeData() const;
		const HdrEnvMap* GetHdrEnvMap() const;

	private:
		static std::vector<VkDescriptorSetLayout> s_DescriptorSetLayout;

		const uint32_t m_ID = UINT32_MAX;
		const bool m_Dynamic = false;

		DirLight* m_DirLight = nullptr;
		PointLight* m_PointLight = nullptr;
		HdrEnvMap* m_HdrEnvMap = nullptr;

		vk::Texture3D* m_Density3DTex = nullptr;
		VolumeData* m_VolumeData = nullptr;

		std::vector<VkDescriptorSet> m_DescSets;
	};
}
