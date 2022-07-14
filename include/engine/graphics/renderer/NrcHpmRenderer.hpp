#pragma once

#include <engine/graphics/vulkan/Shader.hpp>
#include <engine/graphics/vulkan/CommandPool.hpp>
#include <engine/objects/VolumeData.hpp>
#include <engine/graphics/Camera.hpp>
#include <engine/graphics/DirLight.hpp>
#include <string>
#include <engine/compute/NeuralNetwork.hpp>
#include <array>
#include <engine/graphics/NeuralRadianceCache.hpp>
#include <engine/graphics/PointLight.hpp>
#include <engine/graphics/HdrEnvMap.hpp>
#include <engine/graphics/MRHE.hpp>

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
			uint32_t trainWidth,
			uint32_t trainHeight,
			const Camera& camera,
			const VolumeData& volumeData,
			const DirLight& dirLight,
			const PointLight& pointLight,
			const HdrEnvMap& hdrEnvMap,
			const NeuralRadianceCache& nrc,
			const MRHE& mrhe);

		void Render(VkQueue queue) const;
		void Destroy();

		void ResizeFrame(uint32_t width, uint32_t height);

		VkImage GetImage() const;
		VkImageView GetImageView() const;
		size_t GetImageDataSize() const;

	private:
		static VkDescriptorSetLayout m_DescriptorSetLayout;
		static VkDescriptorPool m_DescriptorPool;

		uint32_t m_FrameWidth;
		uint32_t m_FrameHeight;

		uint32_t m_TrainWidth;
		uint32_t m_TrainHeight;

		const Camera& m_Camera;
		const VolumeData& m_VolumeData;
		const DirLight& m_DirLight;
		const PointLight& m_PointLight;
		const HdrEnvMap& m_HdrEnvMap;
		const NeuralRadianceCache& m_Nrc;
		const MRHE& m_Mrhe;

		VkPipelineLayout m_PipelineLayout;

		VkRenderPass m_RenderRenderPass;
		vk::Shader m_RenderVertShader;
		vk::Shader m_RenderFragShader;
		VkPipeline m_RenderPipeline;

		vk::Shader m_TrainShader;
		VkPipeline m_TrainPipeline;
		
		vk::Shader m_StepShader;
		VkPipeline m_StepPipeline;

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
		
		void CreatePipelineLayout(VkDevice device);

		void CreateRenderRenderPass(VkDevice device);
		void CreateRenderPipeline(VkDevice device);
		
		void CreateTrainPipeline(VkDevice device);

		void CreateStepPipeline(VkDevice device);

		void CreateColorImage(VkDevice device);
		void CreateLowPassResources(VkDevice device);
		void CreateLowPassImage(VkDevice device);
		void CreateFramebuffer(VkDevice device);
		
		void RecordCommandBuffer();
	};
}
