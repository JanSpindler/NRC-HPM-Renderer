#pragma once

#include <engine/graphics/vulkan/Buffer.hpp>
#include <tiny-cuda-nn/config.h>
#include <vector>

namespace en
{
	class NeuralRadianceCache
	{
	public:
		NeuralRadianceCache(
			const nlohmann::json& config, 
			uint32_t inputCount, 
			uint32_t outputCount,
			uint32_t batchSize);

		void Init(
			uint32_t inferCount, 
			uint32_t trainCount, 
			float* dCuInferInput, 
			float* dCuInferOutput, 
			float* dCuTrainInput, 
			float* dCuTrainTarget);

	private:
		const uint32_t m_InputCount;
		const uint32_t m_OutputCount;
		const uint32_t m_BatchSize;

		tcnn::TrainableModel m_Model;
		std::vector<tcnn::GPUMatrix<float>> m_InferInputBatches;
		std::vector<tcnn::GPUMatrix<float>> m_InferOutputBatches;
		std::vector<tcnn::GPUMatrix<float>> m_TrainInputBatches;
		std::vector<tcnn::GPUMatrix<float>> m_TrainTargetBatches;
	};
}
