#pragma once

#include <engine/common.hpp>
#include <engine/vulkan/CommandPool.hpp>
#include <engine/vulkan/Shader.hpp>

namespace en
{
	class ImGuiRenderer
	{
	public:
		static void Init(uint32_t width, uint32_t height);
		static void Shutdown();

		static void Resize(uint32_t width, uint32_t height);
		static void StartFrame();
		static void EndFrame(VkQueue queue);

		static bool IsInitialized();
		static VkImage GetImage();

		static void SetBackgroundImageView(VkImageView backgroundImageView);

	private:
		static bool m_Initialized;
		static uint32_t m_Width;
		static uint32_t m_Height;

		static VkDescriptorPool m_ImGuiDescriptorPool;
		static VkDescriptorSetLayout m_DescriptorSetLayout;
		static VkDescriptorPool m_DescriptorPool;

		static VkImageView m_BackgroundImageView;
		static VkSampler m_Sampler;
		static VkDescriptorSet m_DescriptorSet;

		static VkRenderPass m_RenderPass;
		static vk::Shader* m_VertShader;
		static vk::Shader* m_FragShader;
		static VkPipelineLayout m_PipelineLayout;
		static VkPipeline m_Pipeline;

		static VkFormat m_Format;
		static VkImage m_Image;
		static VkDeviceMemory m_ImageMemory;
		static VkImageView m_ImageView;

		static VkFramebuffer m_Framebuffer;
		static vk::CommandPool* m_CommandPool;
		static VkCommandBuffer m_CommandBuffer;

		static void CreateImGuiDescriptorPool(VkDevice device);
		static void CreateDescriptorSetLayout(VkDevice device);
		static void CreateDescriptorPool(VkDevice device);
		static void CreateSampler(VkDevice device);
		static void CreateDescriptorSet(VkDevice device);
		static void CreateRenderPass(VkDevice device);
		static void CreatePipelineLayout(VkDevice device);
		static void CreatePipeline(VkDevice device);
		static void CreateImageResources(VkDevice device);
		static void CreateFramebuffer(VkDevice device);
		static void CreateCommandPoolAndBuffer();

		static void InitImGuiBackend(VkDevice device);
	};
}
