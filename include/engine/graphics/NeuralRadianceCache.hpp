#pragma once

#include <tiny-cuda-nn/config.h>
#include <engine/graphics/vulkan/Buffer.hpp>
#include <vector>

namespace en
{
	class NeuralRadianceCache
	{
	public:
		NeuralRadianceCache(const nlohmann::json& config, uint32_t log2BatchSize);

		void Init(
			uint32_t inferCount,
			uint32_t trainCount,
			float* dCuInferInput, 
			float* dCuInferOutput, 
			float* dCuTrainInput, 
			float* dCuTrainTarget,
			cudaExternalSemaphore_t cudaStartSemaphore,
			cudaExternalSemaphore_t cudaFinishedSemaphore);

		void InferAndTrain();

		void Destroy();

		float GetLoss() const;

	private:
		const uint32_t m_InputCount;
		const uint32_t m_OutputCount;
		const uint32_t m_BatchSize;

		tcnn::TrainableModel m_Model;
		std::vector<tcnn::GPUMatrix<float>> m_InferInputBatches;
		std::vector<tcnn::GPUMatrix<float>> m_InferOutputBatches;
		std::vector<tcnn::GPUMatrix<float>> m_TrainInputBatches;
		std::vector<tcnn::GPUMatrix<float>> m_TrainTargetBatches;

		cudaExternalSemaphore_t m_CudaStartSemaphore;
		cudaExternalSemaphore_t m_CudaFinishedSemaphore;

		float m_Loss = 0.0f;
		size_t m_TrainCounter = 0;

		void Inference();
		void Train();
		void AwaitCudaStartSemaphore();
		void SignalCudaFinishedSemaphore();
	};
}
