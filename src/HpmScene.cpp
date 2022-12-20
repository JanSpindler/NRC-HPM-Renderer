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
		m_DirLight = new DirLight(-1.57f, 0.0f, glm::vec3(1.0f), appConfig.scene.dirLightStrength);
		
		m_PointLight = new PointLight(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f, 1.0f, 1.0f), appConfig.scene.pointLightStrength);

		int hdrWidth, hdrHeight;
		std::vector<float> hdr4fData = en::ReadFileHdr4f(appConfig.scene.hdrEnvMapPath, hdrWidth, hdrHeight, 10000.0f);
		std::array<std::vector<float>, 2> hdrCdf = en::Hdr4fToCdf(hdr4fData, hdrWidth, hdrHeight);
		m_HdrEnvMap = new HdrEnvMap(
			appConfig.scene.hdrEnvMapStrength,
			hdrWidth,
			hdrHeight,
			hdr4fData,
			hdrCdf[0],
			hdrCdf[1]);

		// Load data
		m_Density3DTex = new vk::Texture3D(vk::Texture3D::FromVDB("data/volume/wdas_cloud_quarter.vdb"));
		m_VolumeData = new VolumeData(m_Density3DTex, 1.0f, 0.8f);

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
		}
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

	const VolumeData* HpmScene::GetVolumeData() const
	{
		return m_VolumeData;
	}

	const HdrEnvMap* HpmScene::GetHdrEnvMap() const
	{
		return m_HdrEnvMap;
	}
}
