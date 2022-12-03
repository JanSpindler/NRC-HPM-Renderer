#pragma once

#include <array>
#include <engine/graphics/Camera.hpp>
#include <engine/AppConfig.hpp>
#include <engine/HpmScene.hpp>
#include <engine/graphics/vulkan/CommandPool.hpp>
#include <engine/graphics/vulkan/Shader.hpp>
#include <engine/graphics/renderer/NrcHpmRenderer.hpp>
#include <engine/graphics/renderer/McHpmRenderer.hpp>

namespace en
{
	class Reference
	{
	public:
		struct Result
		{
			float mse;
			float biasX;
			float biasY;
			float biasZ;

			void Norm(uint32_t width, uint32_t height);
		};

		Reference(
			uint32_t width,
			uint32_t height,
			const AppConfig& appConfig,
			const HpmScene& scene,
			VkQueue queue);

		std::array<Result, 6> CompareNrc(NrcHpmRenderer& renderer, const Camera* oldCamera, VkQueue queue);
		std::array<Result, 6> CompareMc(McHpmRenderer& renderer, const Camera* oldCamera, VkQueue queue);
		void Destroy();

	private:
		struct SpecializationData
		{
			uint32_t width;
			uint32_t height;
		};

		uint32_t m_Width = 0;
		uint32_t m_Height = 0;

		VkDescriptorSetLayout m_DescSetLayout;
		VkDescriptorPool m_DescPool;
		VkDescriptorSet m_DescSet;

		vk::CommandPool m_CmdPool;
		VkCommandBuffer m_CmdBuf;

		SpecializationData m_SpecData = {};
		std::vector<VkSpecializationMapEntry> m_SpecEntries;
		VkSpecializationInfo m_SpecInfo;

		VkPipelineLayout m_PipelineLayout;
		vk::Shader m_CmpShader;
		VkPipeline m_CmpPipeline;

		std::array<en::Camera*, 6> m_RefCameras = { nullptr, nullptr, nullptr, nullptr, nullptr, nullptr };
		std::array<VkImage, 6> m_RefImages = {};
		std::array<VkDeviceMemory, 6> m_RefImageMemories = {};
		std::array<VkImageView, 6> m_RefImageViews = {};

		vk::Buffer m_ResultStagingBuffer;
		vk::Buffer m_ResultBuffer;

		void CreateDescriptor();
		void UpdateDescriptor(VkImageView refImageView, VkImageView cmpImageView);

		void InitSpecInfo();
		void CreatePipelineLayout();
		void CreateCmpPipeline();

		void RecordCmpCmdBuf();

		void CreateRefCameras();
		void CreateRefImages(VkQueue queue);
		void GenRefImages(const AppConfig& appConfig, const HpmScene& scene, VkQueue queue);
		void CopyToRefImage(uint32_t imageIdx, VkImage srcImage, VkQueue queue);
	};
}
