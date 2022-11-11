#pragma once

#include <engine/graphics/Camera.hpp>
#include <engine/graphics/vulkan/Shader.hpp>
#include <engine/graphics/vulkan/CommandPool.hpp>
#include <engine/HpmScene.hpp>
#include <cuda_runtime.h>

namespace en
{
	class NeuralRadianceCache;

	class NrcHpmRenderer
	{
	public:
		static void Init(VkDevice device);
		static void Shutdown(VkDevice device);

		NrcHpmRenderer(
			uint32_t width,
			uint32_t height,
			float trainSampleRatio,
			uint32_t trainSpp,
			bool blend,
			const Camera* camera,
			const HpmScene& hpmScene,
			NeuralRadianceCache& nrc);

		void Render(VkQueue queue);
		void Destroy();

		void ExportOutputImageToFile(VkQueue queue, const std::string& filePath) const;
		void EvaluateTimestampQueries();
		void RenderImGui();
		float CompareReferenceMSE(VkQueue queue, const float* referenceData) const;

		VkImage GetImage() const;
		VkImageView GetImageView() const;
		bool IsBlending() const;

		void SetCamera(VkQueue queue, const Camera* camera);
		void SetBlend(bool blend);

	private:
		struct SpecializationData
		{
			uint32_t renderWidth;
			uint32_t renderHeight;
			uint32_t trainWidth;
			uint32_t trainHeight;
			uint32_t trainSpp;

			uint32_t batchSize;

			float volumeDensityFactor;
			float volumeG;

			float hdrEnvMapStrength;
		};

		struct UniformData
		{
			glm::vec4 random;
			uint32_t showNrc;
			float blendFactor;
		};

		static VkDescriptorSetLayout m_DescSetLayout;
		static VkDescriptorPool m_DescPool;

		uint32_t m_RenderWidth;
		uint32_t m_RenderHeight;
		uint32_t m_TrainWidth;
		uint32_t m_TrainHeight;
		uint32_t m_TrainSpp;

		bool m_ShouldBlend = false;
		uint32_t m_BlendIndex = 1;

		const Camera* m_Camera;
		const HpmScene& m_HpmScene;
		NeuralRadianceCache& m_Nrc;

		VkSemaphore m_CudaStartSemaphore;
		cudaExternalSemaphore_t m_CuExtCudaStartSemaphore;

		VkSemaphore m_CudaFinishedSemaphore;
		cudaExternalSemaphore_t m_CuExtCudaFinishedSemaphore;

		VkDeviceSize m_NrcInferInputBufferSize;
		vk::Buffer* m_NrcInferInputBuffer = nullptr;
		cudaExternalMemory_t m_NrcInferInputCuExtMem;
		void* m_NrcInferInputDCuBuffer;

		VkDeviceSize m_NrcInferOutputBufferSize;
		vk::Buffer* m_NrcInferOutputBuffer = nullptr;
		cudaExternalMemory_t m_NrcInferOutputCuExtMem;
		void* m_NrcInferOutputDCuBuffer;

		VkDeviceSize m_NrcTrainInputBufferSize;
		vk::Buffer* m_NrcTrainInputBuffer = nullptr;
		cudaExternalMemory_t m_NrcTrainInputCuExtMem;
		void* m_NrcTrainInputDCuBuffer;

		VkDeviceSize m_NrcTrainTargetBufferSize;
		vk::Buffer* m_NrcTrainTargetBuffer = nullptr;
		cudaExternalMemory_t m_NrcTrainTargetCuExtMem;
		void* m_NrcTrainTargetDCuBuffer;

		VkDeviceSize m_NrcInferFilterBufferSize = 0;
		void* m_NrcInferFilterData = nullptr;
		vk::Buffer* m_NrcInferFilterBuffer = nullptr;

		VkDeviceSize m_NrcTrainRayResBufferSize = 0;
		vk::Buffer* m_NrcTrainRayResBuffer;

		VkPipelineLayout m_PipelineLayout;

		SpecializationData m_SpecData;
		std::vector<VkSpecializationMapEntry> m_SpecMapEntries;
		VkSpecializationInfo m_SpecInfo;

		UniformData m_UniformData = { glm::vec4(0.0f), 1 };
		vk::Buffer m_UniformBuffer;

		vk::Shader m_GenRaysShader;
		VkPipeline m_GenRaysPipeline;

		vk::Shader m_PrepInferRaysShader;
		VkPipeline m_PrepInferRaysPipeline;

		vk::Shader m_PrepTrainRaysShader;
		VkPipeline m_PrepTrainRaysPipeline;

		vk::Shader m_RenderShader;
		VkPipeline m_RenderPipeline;

		VkImage m_OutputImage; // rgba32f output color
		VkDeviceMemory m_OutputImageMemory;
		VkImageView m_OutputImageView;

		VkImage m_PrimaryRayColorImage; // rgb32f primary ray output color + a32f transmittance
		VkDeviceMemory m_PrimaryRayColorImageMemory;
		VkImageView m_PrimaryRayColorImageView;

		VkImage m_PrimaryRayInfoImage;
		VkDeviceMemory m_PrimaryRayInfoImageMemory;
		VkImageView m_PrimaryRayInfoImageView;

		VkImage m_NrcRayOriginImage;
		VkDeviceMemory m_NrcRayOriginImageMemory;
		VkImageView m_NrcRayOriginImageView;

		VkImage m_NrcRayDirImage;
		VkDeviceMemory m_NrcRayDirImageMemory;
		VkImageView m_NrcRayDirImageView;

		VkDescriptorSet m_DescSet;

		const float c_TimestampPeriodInMS = VulkanAPI::GetTimestampPeriod() * 1e-6f;
		const uint32_t c_QueryCount = 6;
		std::vector<float> m_TimePeriods = std::vector<float>(c_QueryCount);
		uint32_t m_QueryIndex = 0;
		VkQueryPool m_QueryPool;

		vk::CommandPool m_CommandPool;
		VkCommandBuffer m_PreCudaCommandBuffer;
		VkCommandBuffer m_PostCudaCommandBuffer;
		VkCommandBuffer m_RandomTasksCmdBuf;

		void CreateSyncObjects(VkDevice device);

		void CreateNrcBuffers();

		void CreatePipelineLayout(VkDevice device);

		void InitSpecializationConstants();

		void CreateGenRaysPipeline(VkDevice device);
		void CreatePrepInferRaysPipeline(VkDevice device);
		void CreatePrepTrainRaysPipeline(VkDevice device);
		void CreateRenderPipeline(VkDevice device);

		void CreateOutputImage(VkDevice device);
		void CreatePrimaryRayColorImage(VkDevice device);
		void CreatePrimaryRayInfoImage(VkDevice device);
		void CreateNrcRayOriginImage(VkDevice device);
		void CreateNrcRayDirImage(VkDevice device);

		void AllocateAndUpdateDescriptorSet(VkDevice device);

		void CreateQueryPool(VkDevice device);

		void RecordPreCudaCommandBuffer();
		void RecordPostCudaCommandBuffer();
	};
}