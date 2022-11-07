#pragma once

#include <engine/graphics/Camera.hpp>
#include <engine/graphics/vulkan/Shader.hpp>
#include <engine/graphics/vulkan/CommandPool.hpp>
#include <engine/HpmScene.hpp>

namespace en
{
	class McHpmRenderer
	{
	public:
		static void Init(VkDevice device);
		static void Shutdown(VkDevice device);

		McHpmRenderer(uint32_t width, uint32_t height, uint32_t pathLength, bool blend, const Camera* camera, const HpmScene& scene);

		void Render(VkQueue queue);
		void Destroy();

		void ExportOutputImageToFile(VkQueue queue, const std::string& filePath) const;
		void EvaluateTimestampQueries();
		void RenderImGui();
		float CompareReferenceMSE(VkQueue queue, const float* referenceData) const;

		VkImage GetImage() const;
		VkImageView GetImageView() const;

		void SetCamera(VkQueue queue, const Camera* camera);

	private:
		struct SpecializationData
		{
			uint32_t renderWidth;
			uint32_t renderHeight;
			uint32_t pathLength;

			float volumeDensityFactor;
			float volumeG;

			float hdrEnvMapStrength;
		};

		struct UniformData
		{
			glm::vec4 random;
			float blendFactor;
		};

		static VkDescriptorSetLayout s_DescSetLayout;
		static VkDescriptorPool s_DescPool;

		uint32_t m_RenderWidth;
		uint32_t m_RenderHeight;
		uint32_t m_PathLength;

		bool m_ShouldBlend = false;
		uint32_t m_BlendIndex = 1;

		const Camera* m_Camera;
		const HpmScene& m_HpmScene;

		VkPipelineLayout m_PipelineLayout;

		SpecializationData m_SpecData;
		std::vector<VkSpecializationMapEntry> m_SpecMapEntries;
		VkSpecializationInfo m_SpecInfo;

		UniformData m_UniformData = { glm::vec4(0.0f) };
		vk::Buffer m_UniformBuffer;

		vk::Shader m_RenderShader;
		VkPipeline m_RenderPipeline;

		VkImage m_OutputImage;
		VkDeviceMemory m_OutputImageMemory;
		VkImageView m_OutputImageView;

		VkImage m_InfoImage;
		VkDeviceMemory m_InfoImageMemory;
		VkImageView m_InfoImageView;

		VkDescriptorSet m_DescSet;

		const uint32_t c_QueryCount = 2;
		uint32_t m_QueryIndex = 0;
		VkQueryPool m_QueryPool;
		float m_TimePeriod = 0.0f;

		vk::CommandPool m_CommandPool;
		VkCommandBuffer m_RenderCommandBuffer;
		VkCommandBuffer m_RandomTasksCmdBuf;

		void CreatePipelineLayout(VkDevice device);

		void InitSpecializationConstants();

		void CreateRenderPipeline(VkDevice device);

		void CreateOutputImage(VkDevice device);
		void CreateInfoImage(VkDevice device);

		void AllocateAndUpdateDescriptorSet(VkDevice device);

		void CreateQueryPool(VkDevice device);

		void RecordRenderCommandBuffer();
	};
}
