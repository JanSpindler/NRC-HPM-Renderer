#pragma once

#include <engine/graphics/vulkan/Buffer.hpp>

namespace en
{
	class NeuralRadianceCache
	{
	public:
		enum class PosEncoding
		{
			Direct = 0,
			Direct_Frequency = 1,
			Direct_Frequency_Mrhe = 2
		};

		enum class DirEncoding
		{
			Direct = 0,
			Direct_Frequency = 1,
			Direct_Frequency_OneBlob = 2
		};

		static void Init(VkDevice device);
		static void Shutdown(VkDevice device);
		static VkDescriptorSetLayout GetDescriptorSetLayout();

		NeuralRadianceCache(size_t layerCount, size_t layerWidth, float nrcLearningRate, uint32_t batchSize);

		void SetPosFrequencyEncoding(uint32_t freqCount);
		void SetPosMrheEncoding(
			uint32_t minFreq, 
			uint32_t maxFreq, 
			uint32_t levelCount, 
			uint32_t hashTableSize, 
			uint32_t featureCount, 
			float learningRate);

		void SetDirFrequencyEncoding(uint32_t freqCount);
		void SetDirOneBlobEncoding(uint32_t featureCount);

		void Init(size_t renderSampleCount, size_t trainSampleCount);

		void Destroy();

		uint32_t GetPosEncoding() const;
		uint32_t GetDirEncoding() const;

		uint32_t GetPosFreqCount() const;
		uint32_t GetPosMinFreq() const;
		uint32_t GetPosMaxFreq() const;
		uint32_t GetPosLevelCount() const;
		uint32_t GetPosHashTableSize() const;
		uint32_t GetPosFeatureCount() const;

		uint32_t GetDirFreqCount() const;
		uint32_t GetDirFeatureCount() const;
		
		uint32_t GetLayerCount() const;
		uint32_t GetLayerWidth() const;
		uint32_t GetInputFeatureCount() const;
		float GetNrcLearningRate() const;
		float GetMrheLearningRate() const;
		uint32_t GetBatchSize() const;

		size_t GetWeightsCount() const;
		size_t GetBiasesCount() const;
		size_t GetMrheCount() const;

		VkDescriptorSet GetDescriptorSet() const;

	private:
		static VkDescriptorSetLayout m_DescSetLayout;
		static VkDescriptorPool m_DescPool;

		PosEncoding m_PosEncoding;
		DirEncoding m_DirEncoding;

		uint32_t m_PosFreqCount;
		uint32_t m_PosMinFreq;
		uint32_t m_PosMaxFreq;
		uint32_t m_PosLevelCount;
		uint32_t m_PosHashTableSize;
		uint32_t m_PosFeatureCount;

		uint32_t m_DirFreqCount;
		uint32_t m_DirFeatureCount;

		size_t m_LayerCount;
		size_t m_LayerWidth;
		uint32_t m_InputFeatureCount;
		float m_NrcLearningRate;
		float m_MrheLearningRate;
		uint32_t m_BatchSize;

		vk::Buffer* m_RenderNeuronsBuffer;
		vk::Buffer* m_TrainNeuronsBuffer;

		vk::Buffer* m_WeightsBuffer;
		vk::Buffer* m_DeltaWeightsBuffer;
		vk::Buffer* m_MomentumWeightsBuffer;

		vk::Buffer* m_BiasesBuffer;
		vk::Buffer* m_DeltaBiasesBuffer;
		vk::Buffer* m_MomentumBiasesBuffer;

		vk::Buffer* m_MrheBuffer;
		vk::Buffer* m_DeltaMrheBuffer;
		vk::Buffer* m_MrheResolutionsBuffer;

		size_t m_WeightsCount;
		size_t m_BiasesCount;
		size_t m_MrheCount;

		size_t m_RenderNeuronsBufferSize;
		size_t m_TrainNeuronsBufferSize;
		size_t m_WeightsBufferSize;
		size_t m_BiasesBufferSize;
		size_t m_MrheBufferSize;
		size_t m_MrheResolutionsBufferSize;

		VkDescriptorSet m_DescSet;

		void InitNn(size_t renderSampleCount, size_t trainSampleCount);
		void CreateNnBuffers();
		void FillNnBuffers();
		
		void InitMrhe();

		void AllocateAndUpdateDescSet();
	};
}
