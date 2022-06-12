#pragma once

#include <engine/graphics/vulkan/Shader.hpp>
#include <engine/graphics/vulkan/CommandPool.hpp>
#include <engine/objects/VolumeData.hpp>
#include <engine/graphics/Camera.hpp>
#include <engine/graphics/Sun.hpp>
#include <string>
#include <engine/compute/NeuralNetwork.hpp>
#include <array>

namespace en
{
	class NrcHpmRenderer
	{
	public:
		static void Init(VkDevice device);
		static void Shutdown(VkDevice device);

		NrcHpmRenderer(
			uint32_t width,
			uint32_t height,
			const Camera* camera,
			const VolumeData* volumeData,
			const Sun* sun);

		void Render(VkQueue queue) const;
		void Destroy();

		void ResizeFrame(uint32_t width, uint32_t height);

		void UpdateNnData(KomputeManager& manager, NeuralNetwork& nn);

		VkImage GetImage() const;
		VkImageView GetImageView() const;
		size_t GetImageDataSize() const;

	private:
		static VkDescriptorSetLayout m_DescriptorSetLayout;
		static VkDescriptorSetLayout m_NnDSL;
		static VkDescriptorPool m_DescriptorPool;

		uint32_t m_FrameWidth;
		uint32_t m_FrameHeight;

		const Camera* m_Camera;
		const VolumeData* m_VolumeData;
		const Sun* m_Sun;

		VkRenderPass m_RenderPass;

		vk::Shader m_NrcForwardVertShader;
		vk::Shader m_NrcForwardFragShader;
		VkPipelineLayout m_NrcForwardPipelineLayout;
		VkPipeline m_NrcForwardPipeline;
		std::array<vk::Buffer*, 12> m_NrcForwardBuffers;
		VkDescriptorSet m_NrcForwardDS;
		
		VkImage m_ColorImage;
		VkDeviceMemory m_ColorImageMemory;
		VkImageView m_ColorImageView;

		VkImage m_LowPassImage;
		VkDeviceMemory m_LowPassImageMemory;
		VkImageView m_LowPassImageView;
		VkSampler m_LowPassSampler;
		VkDescriptorSet m_DescriptorSet;

		VkFramebuffer m_Framebuffer;
		vk::CommandPool m_CommandPool;
		VkCommandBuffer m_CommandBuffer;

		void CreateRenderPass(VkDevice device);
		
		void CreateNrcForwardResources(VkDevice device);
		void CreateNrcForwardPipelineLayout(VkDevice device);
		void CreateNrcForwardPipeline(VkDevice device);

		void CreateColorImage(VkDevice device);
		
		void CreateLowPassResources(VkDevice device);
		void CreateLowPassImage(VkDevice device);

		void CreateFramebuffer(VkDevice device);
		
		void RecordCommandBuffer();
	};
}
