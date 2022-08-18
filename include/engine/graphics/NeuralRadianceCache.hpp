#pragma once

#include <engine/graphics/vulkan/Buffer.hpp>

namespace en
{
	class NeuralRadianceCache
	{
	public:
		enum class PosEncoding
		{
			Direct,
			Frequency,
			Mrhe
		};

		enum class DirEncoding
		{
			Direct,
			Frequency,
			OneBlob
		};

		NeuralRadianceCache(PosEncoding posEncoding, DirEncoding dirEncoding, size_t layerCount, size_t layerWidth);

		void SetPosFrequencyEncoding(uint32_t freqCount);
		void SetPosMrheEncoding(uint32_t minFreq, uint32_t maxFreq, uint32_t levelCount, uint32_t featureCount);

		void SetDirFrequencyEncoding(uint32_t freqCount);
		void SetDirOneBlobEncoding(uint32_t featureCount);

		void Init();

		void Destroy();

		uint32_t GetInputFeatureCount() const;

	private:
		struct UniformData
		{
			uint32_t posFreqCount;
			uint32_t posMinFreq;
			uint32_t posMaxFreq;
			uint32_t posLevelCount;
			uint32_t posFeatureCount;

			uint32_t dirFreqCount;
			uint32_t dirFeatureCount;
		};

		PosEncoding m_PosEncoding;
		DirEncoding m_DirEncoding;

		size_t m_LayerCount;
		size_t m_LayerWidth;

		uint32_t m_InputFeatureCount;

		UniformData m_UniformData;
		
		vk::Buffer* m_ResultsBuffer;

		vk::Buffer* m_WeightsBuffer;
		vk::Buffer* m_DeltaWeightsBuffer;
		vk::Buffer* m_MomentumWeightsBuffer;

		vk::Buffer* m_BiasesBuffer;
		vk::Buffer* m_DeltaBiasesBuffer;
		vk::Buffer* m_MomentumBiasesBuffer;

		vk::Buffer* m_MrheBuffer;
		vk::Buffer* m_DeltaMrheBuffer;

		void InitNn();
		void CreateNnBuffers(size_t weightsBufferSize, size_t biasesBufferSize);
		void FillNnBuffers(size_t weightsBufferSize, size_t biasesBufferSize);
		
		void InitMrhe();
	};
}
