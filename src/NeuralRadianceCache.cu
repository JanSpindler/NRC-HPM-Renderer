#include <cuda_runtime.h>
#include <tiny-cuda-nn/config.h>
#include <engine/graphics/NeuralRadianceCache.hpp>
#include <random>
#include <engine/util/Log.hpp>

#define ASSERT_CUDA(error) if (error != cudaSuccess) { en::Log::Error("Cuda assert triggered: " + std::string(cudaGetErrorName(error)), true); }

namespace en
{
	NeuralRadianceCache::NeuralRadianceCache(
		const nlohmann::json& config, 
		uint32_t inputCount, 
		uint32_t outputCount,
		uint32_t log2BatchSize) 
		:
		m_Model(tcnn::create_from_config(inputCount, outputCount, config)),
		m_InputCount(inputCount),
		m_OutputCount(outputCount),
		m_BatchSize(2 << (log2BatchSize - 1))
	{
	}

	void NeuralRadianceCache::Init(
		uint32_t inferCount,
		uint32_t trainCount,
		float* dCuInferInput,
		float* dCuInferOutput,
		float* dCuTrainInput,
		float* dCuTrainTarget,
		cudaExternalSemaphore_t cudaStartSemaphore,
		cudaExternalSemaphore_t cudaFinishedSemaphore)
	{
		// Init members
		m_CudaStartSemaphore = cudaStartSemaphore;
		m_CudaFinishedSemaphore = cudaFinishedSemaphore;

		// Check batch size compatibility
		if (inferCount % m_BatchSize != 0 || trainCount % m_BatchSize != 0)
		{
			Log::Error("NRC batch size is not compatible with infer count or train count", true);
		}

		// Init infer buffers
		uint32_t inferBatchCount = inferCount / m_BatchSize;
		m_InferInputBatches.resize(inferBatchCount);
		m_InferOutputBatches.resize(inferBatchCount);
		for (uint32_t i = 0; i < inferBatchCount; i++)
		{
			const size_t floatInputOffset = i * m_BatchSize * m_InputCount;
			const size_t floatOutputOffset = i * m_BatchSize * m_OutputCount;
			m_InferInputBatches[i] = tcnn::GPUMatrix<float>(&(dCuInferInput[floatInputOffset]), m_InputCount, m_BatchSize);
			m_InferOutputBatches[i] = tcnn::GPUMatrix<float>(&(dCuInferOutput[floatOutputOffset]), m_OutputCount, m_BatchSize);
		}

		// Init train buffers
		uint32_t trainBatchCount = trainCount / m_BatchSize;
		m_TrainInputBatches.resize(trainBatchCount);
		m_TrainTargetBatches.resize(trainBatchCount);
		for (uint32_t i = 0; i < inferBatchCount; i++)
		{
			const size_t floatInputOffset = i * m_BatchSize * m_InputCount;
			const size_t floatTargetOffset = i * m_BatchSize * m_OutputCount;
			m_TrainInputBatches[i] = tcnn::GPUMatrix<float>(&(dCuTrainInput[floatInputOffset]), m_InputCount, m_BatchSize);
			m_TrainTargetBatches[i] = tcnn::GPUMatrix<float>(&(dCuTrainTarget[floatTargetOffset]), m_OutputCount, m_BatchSize);
		}
	}

	void NeuralRadianceCache::InferAndTrain()
	{
		AwaitCudaStartSemaphore();
		Inference();
		Train();
		SignalCudaFinishedSemaphore();
	}

	void NeuralRadianceCache::Destroy()
	{
	}

	void NeuralRadianceCache::Inference()
	{
		for (size_t i = 0; i < m_InferInputBatches.size(); i++)
		{
			const tcnn::GPUMatrix<float>& inputBatch = m_InferInputBatches[i];
			tcnn::GPUMatrix<float>& outputBatch = m_InferOutputBatches[i];
			m_Model.network->inference(inputBatch, outputBatch);
		}
	}

	void NeuralRadianceCache::Train()
	{
		for (size_t i = 0; i < m_TrainInputBatches.size(); i++)
		{
			const tcnn::GPUMatrix<float>& inputBatch = m_TrainInputBatches[i];
			const tcnn::GPUMatrix<float>& targetBatch = m_TrainTargetBatches[i];
			m_Model.trainer->training_step(inputBatch, targetBatch);
		}
	}

	void NeuralRadianceCache::AwaitCudaStartSemaphore()
	{
		cudaExternalSemaphoreWaitParams extSemaphoreWaitParams;
		memset(&extSemaphoreWaitParams, 0, sizeof(extSemaphoreWaitParams));
		extSemaphoreWaitParams.params.fence.value = 0;
		extSemaphoreWaitParams.flags = 0;

		cudaError_t error = cudaWaitExternalSemaphoresAsync(&m_CudaStartSemaphore, &extSemaphoreWaitParams, 1);
		ASSERT_CUDA(error);
	}

	void NeuralRadianceCache::SignalCudaFinishedSemaphore()
	{
		cudaExternalSemaphoreSignalParams extSemaphoreSignalParams;
		memset(&extSemaphoreSignalParams, 0, sizeof(extSemaphoreSignalParams));
		extSemaphoreSignalParams.params.fence.value = 0;
		extSemaphoreSignalParams.flags = 0;

		cudaError_t error = cudaSignalExternalSemaphoresAsync(&m_CudaFinishedSemaphore, &extSemaphoreSignalParams, 1);
		ASSERT_CUDA(error);
	}
}
