#include <cuda_runtime.h>
#include <tiny-cuda-nn/config.h>
#include <engine/graphics/NeuralRadianceCache.hpp>
#include <random>
#include <engine/util/Log.hpp>

#define ASSERT_CUDA(error) if (error != cudaSuccess) { en::Log::Error("Cuda assert triggered: " + std::string(cudaGetErrorName(error)), true); }

namespace en
{
	NeuralRadianceCache::NeuralRadianceCache(const nlohmann::json& config, uint32_t log2BatchSize) :
		m_Model(tcnn::create_from_config(5, 3, config)),
		m_InputCount(5),
		m_OutputCount(3),
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
		// Check if sample counts are compatible
		if (inferCount % 16 != 0) { en::Log::Error("NRC requires inferCount to be a multiple of 16", true); }
		if (trainCount % 16 != 0) { en::Log::Error("NRC required trainCount to be a multiple of 16", true); }

		// Init members
		m_CudaStartSemaphore = cudaStartSemaphore;
		m_CudaFinishedSemaphore = cudaFinishedSemaphore;

		// Init infer buffers
		uint32_t inferBatchCount = inferCount / m_BatchSize;
		uint32_t inferLastBatchSize = inferCount - (inferBatchCount * m_BatchSize);
		m_InferInputBatches.resize(inferBatchCount);
		m_InferOutputBatches.resize(inferBatchCount);
		
		size_t floatInputOffset = 0;
		size_t floatOutputOffset = 0;
		for (uint32_t i = 0; i < inferBatchCount; i++)
		{
			m_InferInputBatches[i] = tcnn::GPUMatrix<float>(&(dCuInferInput[floatInputOffset]), m_InputCount, m_BatchSize);
			m_InferOutputBatches[i] = tcnn::GPUMatrix<float>(&(dCuInferOutput[floatOutputOffset]), m_OutputCount, m_BatchSize);
			floatInputOffset += m_BatchSize * m_InputCount;
			floatOutputOffset += m_BatchSize * m_OutputCount;
		}

		if (inferLastBatchSize > 0)
		{
			m_InferInputBatches.push_back(tcnn::GPUMatrix<float>(&(dCuInferInput[floatInputOffset]), m_InputCount, inferLastBatchSize));
			m_InferOutputBatches.push_back(tcnn::GPUMatrix<float>(&(dCuInferOutput[floatOutputOffset]), m_OutputCount, inferLastBatchSize));
		}

		// Init train buffers
		uint32_t trainBatchCount = trainCount / m_BatchSize;
		uint32_t trainLastBatchSize = trainCount - (trainBatchCount * m_BatchSize);
		m_TrainInputBatches.resize(trainBatchCount);
		m_TrainTargetBatches.resize(trainBatchCount);

		floatInputOffset = 0;
		floatOutputOffset = 0;
		for (uint32_t i = 0; i < trainBatchCount; i++)
		{
			m_TrainInputBatches[i] = tcnn::GPUMatrix<float>(&(dCuTrainInput[floatInputOffset]), m_InputCount, m_BatchSize);
			m_TrainTargetBatches[i] = tcnn::GPUMatrix<float>(&(dCuTrainTarget[floatOutputOffset]), m_OutputCount, m_BatchSize);
			floatInputOffset += m_BatchSize * m_InputCount;
			floatOutputOffset += m_BatchSize * m_OutputCount;
		}

		if (trainLastBatchSize > 0)
		{
			m_TrainInputBatches.push_back(tcnn::GPUMatrix<float>(&(dCuTrainInput[floatInputOffset]), m_InputCount, trainLastBatchSize));
			m_TrainTargetBatches.push_back(tcnn::GPUMatrix<float>(&(dCuTrainTarget[floatOutputOffset]), m_OutputCount, trainLastBatchSize));
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

	float NeuralRadianceCache::GetLoss() const
	{
		return m_Loss;
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
			auto forwardContext = m_Model.trainer->training_step(inputBatch, targetBatch);
			m_Loss = m_Model.trainer->loss(*forwardContext.get());
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
