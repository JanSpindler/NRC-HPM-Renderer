#include <engine/HpmScene.hpp>
#include <engine/util/read_file.hpp>

namespace en
{
	std::vector<VkDescriptorSetLayout> HpmScene::s_DescriptorSetLayout;

	const std::vector<VkDescriptorSetLayout>& HpmScene::GetDescriptorSetLayout()
	{
		if (s_DescriptorSetLayout.size() == 0)
		{
			s_DescriptorSetLayout = {
				VolumeData::GetDescriptorSetLayout(),
				DirLight::GetDescriptorSetLayout(),
				PointLight::GetDescriptorSetLayout(),
				HdrEnvMap::GetDescriptorSetLayout()
			};
		}

		return s_DescriptorSetLayout;
	}

	HpmScene::HpmScene(const AppConfig& appConfig)
	{
		// Lighting
		m_DirLight = new DirLight(-1.57f, 0.0f, glm::vec3(1.0f), appConfig.dirLightStrength);
		
		m_PointLight = new PointLight(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f, 1.0f, 1.0f), appConfig.pointLightStrength);

		int hdrWidth, hdrHeight;
		std::vector<float> hdr4fData = en::ReadFileHdr4f(appConfig.hdrEnvMapPath, hdrWidth, hdrHeight, 1000.0f);
		std::array<std::vector<float>, 2> hdrCdf = en::Hdr4fToCdf(hdr4fData, hdrWidth, hdrHeight);
		m_HdrEnvMap = new HdrEnvMap(
			appConfig.hdrEnvMapDirectStrength,
			appConfig.hdrEnvMapHpmStrength,
			hdrWidth,
			hdrHeight,
			hdr4fData,
			hdrCdf[0],
			hdrCdf[1]);

		// Load data
		auto density3D = en::ReadFileDensity3D("data/cloud_sixteenth", 125, 85, 153);
		m_Density3DTex = new vk::Texture3D(
			density3D,
			VK_FILTER_LINEAR,
			VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
			VK_BORDER_COLOR_INT_OPAQUE_BLACK);
		m_VolumeData = new VolumeData(m_Density3DTex);

		// Store desc sets
		m_DescSets = {
			m_VolumeData->GetDescriptorSet(),
			m_DirLight->GetDescriptorSet(),
			m_PointLight->GetDescriptorSet(),
			m_HdrEnvMap->GetDescriptorSet()
		};
	}

	void HpmScene::Update(bool renderImgui)
	{
		if (renderImgui)
		{
			m_VolumeData->RenderImGui();
			m_DirLight->RenderImgui();
			m_PointLight->RenderImGui();
			m_HdrEnvMap->RenderImGui();
		}

		m_VolumeData->Update();
	}

	void HpmScene::Destroy()
	{
		m_VolumeData->Destroy();
		delete m_VolumeData;

		m_Density3DTex->Destroy();
		delete m_Density3DTex;

		m_HdrEnvMap->Destroy();
		delete m_HdrEnvMap;

		m_PointLight->Destroy();
		delete m_PointLight;

		m_DirLight->Destroy();
		delete m_DirLight;
	}

	const std::vector<VkDescriptorSet>& HpmScene::GetDescriptorSets() const
	{
		return m_DescSets;
	}
}
