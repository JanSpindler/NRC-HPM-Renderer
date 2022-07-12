#pragma once

#include <engine/graphics/vulkan/Shader.hpp>
#include <engine/graphics/vulkan/CommandPool.hpp>
#include <engine/objects/VolumeData.hpp>
#include <engine/graphics/Camera.hpp>
#include <engine/graphics/DirLight.hpp>
#include <string>
#include <engine/compute/NrcDataset.hpp>
#include <engine/graphics/NeuralRadianceCache.hpp>

namespace en
{
	class DensityPathTracer
	{
	public:
		DensityPathTracer(
			uint32_t width,
			uint32_t height,
			const NeuralRadianceCache* nrc,
			const Camera* camera,
			const VolumeData* volumeData,
			const DirLight* sun);

		void Render(VkQueue queue) const;
		void Destroy();

		void ResizeFrame(uint32_t width, uint32_t height);

		void ExportForTraining(VkQueue queue, std::vector<en::NrcInput>& inputs, std::vector<en::NrcTarget>& targets);

		//void ExportImageToHost(VkQueue queue, uint64_t index);

		VkImage GetImage() const;
		VkImageView GetImageView() const;

	private:
		uint32_t m_FrameWidth;
		uint32_t m_FrameHeight;

		const NeuralRadianceCache* m_Nrc;
		const Camera* m_Camera;
		const VolumeData* m_VolumeData;
		const DirLight* m_Sun;

		VkRenderPass m_RenderPass;
		vk::Shader m_VertShader;
		vk::Shader m_FragShader;
		VkPipelineLayout m_PipelineLayout;
		VkPipeline m_Pipeline;

		VkImage m_ColorImage;
		VkDeviceMemory m_ColorImageMemory;
		VkImageView m_ColorImageView;

		VkImage m_PosImage;
		VkDeviceMemory m_PosImageMemory;
		VkImageView m_PosImageView;

		VkImage m_DirImage;
		VkDeviceMemory m_DirImageMemory;
		VkImageView m_DirImageView;

		VkFramebuffer m_Framebuffer;
		vk::CommandPool m_CommandPool;
		VkCommandBuffer m_CommandBuffer;

		void CreateRenderPass(VkDevice device);
		void CreatePipelineLayout(VkDevice device);
		void CreatePipeline(VkDevice device);
		void CreateColorImage(VkDevice device);
		void CreatePosImage(VkDevice device);
		void CreateDirImage(VkDevice device);
		void CreateFramebuffer(VkDevice device);
		void RecordCommandBuffer();
	};
}
