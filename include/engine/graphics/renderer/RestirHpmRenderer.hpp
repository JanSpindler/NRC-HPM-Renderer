#pragma once

#include <engine/graphics/Camera.hpp>
#include <engine/objects/VolumeData.hpp>
#include <engine/graphics/DirLight.hpp>
#include <engine/graphics/PointLight.hpp>
#include <engine/graphics/HdrEnvMap.hpp>
#include <engine/graphics/VolumeReservoir.hpp>
#include <engine/graphics/vulkan/Shader.hpp>
#include <engine/graphics/vulkan/CommandPool.hpp>

namespace en
{
	class RestirHpmRenderer
	{
	public:
		static void Init(VkDevice device);
		static void Shutdown(VkDevice device);

		RestirHpmRenderer(
			uint32_t width,
			uint32_t height,
			const Camera& camera,
			const VolumeData& volumeData,
			const DirLight& dirLight,
			const PointLight& pointLight,
			const HdrEnvMap& hdrEnvMap,
			VolumeReservoir& volumeReservoir);

		void Render(VkQueue queue);
		void Destroy();

		VkImage GetImage() const;
		VkImageView GetImageView() const;

	private:
		struct SpecializationData
		{
			uint32_t renderWidth;
			uint32_t renderHeight;

			uint32_t pathVertexCount;
			
			uint32_t spatialKernelSize;
		};

		static VkDescriptorSetLayout m_DescSetLayout;
		static VkDescriptorPool m_DescPool;

		const uint32_t m_Width;
		const uint32_t m_Height;

		const Camera& m_Camera;
		const VolumeData& m_VolumeData;
		const DirLight& m_DirLight;
		const PointLight& m_PointLight;
		const HdrEnvMap& m_HdrEnvMap;
		VolumeReservoir& m_VolumeReservoir;

		VkPipelineLayout m_PipelineLayout;

		SpecializationData m_SpecData;
		std::vector<VkSpecializationMapEntry> m_SpecMapEntries;
		VkSpecializationInfo m_SpecInfo;

		vk::Shader m_LocalInitShader;
		VkPipeline m_LocalInitPipeline;

		vk::Shader m_SpatialReuseShader;
		VkPipeline m_SpatialReusePipeline;

		vk::Shader m_RenderShader;
		VkPipeline m_RenderPipeline;

		VkImage m_OutputImage; // rgba32f output color
		VkDeviceMemory m_OutputImageMemory;
		VkImageView m_OutputImageView;

		VkImage m_PixelInfoImage;
		VkDeviceMemory m_PixelInfoImageMemory;
		VkImageView m_PixelInfoImageView;

		VkDescriptorSet m_DescSet;

		vk::CommandPool m_CommandPool;
		VkCommandBuffer m_CommandBuffer;

		void CreatePipelineLayout(VkDevice device);

		void InitSpecializationConstants();

		void CreateLocalInitPipeline(VkDevice device);
		void CreateSpatialReusePipeline(VkDevice device);
		void CreateRenderPipeline(VkDevice device);

		void CreateOutputImage(VkDevice device);
		void CreatePixelInfoImage(VkDevice device);

		void AllocateAndUpdateDescriptorSet(VkDevice device);

		void RecordCommandBuffer();
	};
}
