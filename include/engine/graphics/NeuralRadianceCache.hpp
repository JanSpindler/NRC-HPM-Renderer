#pragma once

#include <tiny-cuda-nn/config.h>
#include <engine/graphics/vulkan/Buffer.hpp>
#include <vector>
#include <engine/AppConfig.hpp>

namespace en
{
	class NeuralRadianceCache
	{
	public:
		NeuralRadianceCache(const AppConfig& appConfig);

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
		size_t GetInferBatchCount() const;
		size_t GetTrainBatchCount() const;

	private:
		static const uint32_t sc_InputCount;
		static const uint32_t sc_OutputCount;

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
