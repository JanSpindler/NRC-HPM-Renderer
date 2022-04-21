#pragma once

#include <engine/vulkan/Shader.hpp>
#include <engine/vulkan/CommandPool.hpp>

namespace en
{
	class DensityPathTracer
	{
	public:
		DensityPathTracer(uint32_t width, uint32_t height);

		void Render(VkQueue queue) const;
		void Destroy();

		void ResizeFrame(uint32_t width, uint32_t height);

		VkImage GetImage() const;
		VkImageView GetImageView() const;

	private:
		uint32_t m_FrameWidth;
		uint32_t m_FrameHeight;

		VkRenderPass m_RenderPass;
		vk::Shader m_VertShader;
		vk::Shader m_FragShader;
		VkPipelineLayout m_PipelineLayout;
		VkPipeline m_Pipeline;
		VkImage m_Image;
		VkDeviceMemory m_ImageMemory;
		VkImageView m_ImageView;
		VkFramebuffer m_Framebuffer;
		vk::CommandPool m_CommandPool;
		VkCommandBuffer m_CommandBuffer;

		void CreateRenderPass(VkDevice device);
		void CreatePipelineLayout(VkDevice device);
		void CreatePipeline(VkDevice device);
		void CreateImage(VkDevice device);
		void CreateImageView(VkDevice device);
		void CreateFramebuffer(VkDevice device);
		void RecordCommandBuffer();
	};
}
