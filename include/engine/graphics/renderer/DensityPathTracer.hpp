#pragma once

#include <engine/graphics/vulkan/Shader.hpp>
#include <engine/graphics/vulkan/CommandPool.hpp>
#include <engine/objects/VolumeData.hpp>
#include <engine/graphics/Camera.hpp>
#include <engine/graphics/Sun.hpp>
#include <string>

namespace en
{
	class DensityPathTracer
	{
	public:
		static void Init(VkDevice device);
		static void Shutdown(VkDevice device);

		DensityPathTracer(
			uint32_t width, 
			uint32_t height, 
			const Camera* camera, 
			const VolumeData* volumeData,
			const Sun* sun);

		void Render(VkQueue queue) const;
		void Destroy();

		void ResizeFrame(uint32_t width, uint32_t height);

		void ExportImageToHost(VkQueue queue, uint64_t index);

		VkImage GetImage() const;
		VkImageView GetImageView() const;
		size_t GetImageDataSize() const;

	private:
		static VkDescriptorSetLayout m_DescriptorSetLayout;
		static VkDescriptorPool m_DescriptorPool;

		uint32_t m_FrameWidth;
		uint32_t m_FrameHeight;

		const Camera* m_Camera;
		const VolumeData* m_VolumeData;
		const Sun* m_Sun;

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

		VkImage m_LowPassImage;
		VkDeviceMemory m_LowPassImageMemory;
		VkImageView m_LowPassImageView;
		VkSampler m_LowPassSampler;
		VkDescriptorSet m_DescriptorSet;

		VkFramebuffer m_Framebuffer;
		vk::CommandPool m_CommandPool;
		VkCommandBuffer m_CommandBuffer;

		void CreateRenderPass(VkDevice device);
		void CreatePipelineLayout(VkDevice device);
		void CreatePipeline(VkDevice device);
		void CreateColorImage(VkDevice device);
		void CreatePosImage(VkDevice device);
		void CreateDirImage(VkDevice device);
		void CreateLowPassResources(VkDevice device);
		void CreateLowPassImage(VkDevice device);
		void CreateFramebuffer(VkDevice device);
		void RecordCommandBuffer();
	};
}
