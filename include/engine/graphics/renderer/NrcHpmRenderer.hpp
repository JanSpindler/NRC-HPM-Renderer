#include <engine/graphics/Camera.hpp>
#include <engine/objects/VolumeData.hpp>
#include <engine/graphics/DirLight.hpp>
#include <engine/graphics/PointLight.hpp>
#include <engine/graphics/HdrEnvMap.hpp>
#include <engine/graphics/NeuralRadianceCache.hpp>
#include <engine/graphics/vulkan/Shader.hpp>
#include <engine/graphics/vulkan/CommandPool.hpp>

namespace en
{
	class NrcHpmRenderer
	{
	public:
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
			const NeuralRadianceCache& nrc);

		void Render(VkQueue queue) const;
		void Destroy();

		//void ResizeFrame(uint32_t width, uint32_t height);

		VkImage GetImage() const;
		VkImageView GetImageView() const;
		size_t GetImageDataSize() const;

	private:
		struct SpecializationData
		{
			uint32_t renderWidth;
			uint32_t renderHeight;

			uint32_t trainWidth;
			uint32_t trainHeight;
			
			uint32_t posEncoding;
			uint32_t dirEncoding;
			
			uint32_t posFreqCount;
			uint32_t posMinFreq;
			uint32_t posMaxFreq;
			uint32_t posLevelCount;
			uint32_t posHashTableSize;
			uint32_t posFeatureCount;

			uint32_t dirFreqCount;
			uint32_t dirFeatureCount;

			uint32_t layerCount;
			uint32_t layerWidth;
			uint32_t inputFeatureCount;
			float nrcLearningRate;
			float mrheLearningRate;
		};

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

		VkPipelineLayout m_PipelineLayout;

		SpecializationData m_SpecData;
		std::vector<VkSpecializationMapEntry> m_SpecMapEntries;
		VkSpecializationInfo m_SpecInfo;

		VkRenderPass m_RenderPass;
		vk::Shader m_VertShader;
		vk::Shader m_FragShader;
		VkPipeline m_RenderPipeline;

		vk::Shader m_TrainShader;
		VkPipeline m_TrainPipeline;

		vk::Shader m_StepShader;
		VkPipeline m_StepPipeline;

		vk::Shader m_MrheStepShader;
		VkPipeline m_MrheStepPipeline;

		vk::Shader m_GenRaysShader;
		VkPipeline m_GenRaysPipeline;

		VkImage m_ColorImage;
		VkDeviceMemory m_ColorImageMemory;
		VkImageView m_ColorImageView;

		VkFramebuffer m_Framebuffer;
		vk::CommandPool m_CommandPool;
		VkCommandBuffer m_CommandBuffer;

		void CreatePipelineLayout(VkDevice device);

		void InitSpecializationConstants();

		void CreateRenderRenderPass(VkDevice device);
		void CreateRenderPipeline(VkDevice device);

		void CreateTrainPipeline(VkDevice device);

		void CreateStepPipeline(VkDevice device);

		void CreateMrheStepPipeline(VkDevice device);

		void CreateGenRaysShader(VkDevice device);

		void CreateColorImage(VkDevice device);
		void CreateFramebuffer(VkDevice device);

		void RecordCommandBuffer();
	};
}