#pragma once

#include <engine/graphics/common.hpp>
#include <engine/graphics/vulkan/Shader.hpp>
#include <engine/graphics/vulkan/CommandPool.hpp>
#include <engine/objects/Model.hpp>
#include <engine/graphics/Camera.hpp>

namespace en
{
	class SimpleModelRenderer
	{
	public:
		SimpleModelRenderer(uint32_t width, uint32_t height, const Camera* camera);

		void Render(VkQueue queue) const;
		void Destroy();

		void ResizeFrame(uint32_t width, uint32_t height);

		void AddModelInstance(const ModelInstance* modelInstance);
		void RemoveModelInstance(const ModelInstance* modelInstance);

		VkImage GetColorImage() const;
		VkImageView GetColorImageView() const;

		void SetImGuiCommandBuffer(VkCommandBuffer imGuiCommandBuffer);

	private:
		VkCommandBuffer m_ImGuiCommandBuffer = VK_NULL_HANDLE;

		std::vector<const ModelInstance*> m_ModelInstances;

		uint32_t m_FrameWidth;
		uint32_t m_FrameHeight;
		const Camera* m_Camera;

		VkRenderPass m_RenderPass;
		vk::Shader m_VertShader;
		vk::Shader m_FragShader;
		VkPipelineLayout m_PipelineLayout;
		VkPipeline m_Pipeline;

		VkFormat m_ColorFormat;
		VkImage m_ColorImage;
		VkDeviceMemory m_ColorImageMemory;
		VkImageView m_ColorImageView;

		VkFormat m_DepthFormat;
		VkImage m_DepthImage;
		VkDeviceMemory m_DepthImageMemory;
		VkImageView m_DepthImageView;

		VkFramebuffer m_Framebuffer;
		vk::CommandPool m_CommandPool;
		VkCommandBuffer m_CommandBuffer;

		void FindFormats();
		void CreateRenderPass(VkDevice device);
		void CreatePipelineLayout(VkDevice device);
		void CreatePipeline(VkDevice device);

		void CreateColorImageObjects(VkDevice device);
		void CreateDepthImageObjects(VkDevice device);
		void CreateFramebuffer(VkDevice device);
		void RecordCommandBuffer();
	};
}