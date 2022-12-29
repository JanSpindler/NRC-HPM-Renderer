#include <engine/cuda_common.hpp>
#include <engine/graphics/NeuralRadianceCache.hpp>
#include <random>
#include <engine/util/Log.hpp>

namespace en
{
	const uint32_t NeuralRadianceCache::sc_InputCount = 5;
	const uint32_t NeuralRadianceCache::sc_OutputCount = 3;

	NeuralRadianceCache::NeuralRadianceCache(const AppConfig& appConfig) :
		m_InferBatchSize(2 << (appConfig.log2InferBatchSize - 1)),
		m_TrainBatchSize(2 << (appConfig.log2TrainBatchSize - 1)),
		m_TrainBatchCount(appConfig.trainBatchCount)
	{
		nlohmann::json modelConfig = {
			{"loss", {
				{"otype", appConfig.lossFn}
			}},
			{"optimizer", {
				{"otype", "EMA"},
				{"decay", appConfig.emaDecay},
				{"nested", {
					{"otype", appConfig.optimizer},
					{"learning_rate", appConfig.learningRate},
					//{"l2_reg", 0.0001},
				}}
			}},
			appConfig.encoding.jsonConfig,
			{"network", {
				{"otype", "FullyFusedMLP"},
				{"activation", "ReLU"},
				{"output_activation", "None"},
				{"n_neurons", appConfig.nnWidth},
				{"n_hidden_layers", appConfig.nnDepth},
			}},
		};

		m_Model = tcnn::create_from_config(sc_InputCount, sc_OutputCount, modelConfig);
	}

	void NeuralRadianceCache::Init(
		uint32_t inferCount,
		float* dCuInferInput,
		float* dCuInferOutput,
		float* dCuTrainInput,
		float* dCuTrainTarget,
		cudaExternalSemaphore_t cudaStartSemaphore,
		cudaExternalSemaphore_t cudaFinishedSemaphore)
	{
		// Check if sample counts are compatible
		if (inferCount % 16 != 0) { en::Log::Error("NRC requires inferCount to be a multiple of 16", true); }

		// Init members
		m_CudaStartSemaphore = cudaStartSemaphore;
		m_CudaFinishedSemaphore = cudaFinishedSemaphore;

		// Init big buffer
		const uint32_t trainCount = m_TrainBatchCount * m_TrainBatchSize;

		m_InferInput = tcnn::GPUMatrix<float>(dCuInferInput, sc_InputCount, inferCount);
		m_InferOutput = tcnn::GPUMatrix<float>(dCuInferOutput, sc_OutputCount, inferCount);
		m_TrainInput = tcnn::GPUMatrix<float>(dCuTrainInput, sc_InputCount, trainCount);
		m_TrainTarget = tcnn::GPUMatrix<float>(dCuTrainTarget, sc_OutputCount, trainCount);

		// Init infer buffers
		const uint32_t inferBatchCount = inferCount / m_InferBatchSize;
		const uint32_t inferLastBatchSize = inferCount - (inferBatchCount * m_InferBatchSize);
		m_InferInputBatches.resize(inferBatchCount);
		m_InferOutputBatches.resize(inferBatchCount);
		
		for (uint32_t i = 0; i < inferBatchCount; i++)
		{
			m_InferInputBatches[i] = m_InferInput.slice_cols(i * m_InferBatchSize, m_InferBatchSize);
			m_InferOutputBatches[i] = m_InferOutput.slice_cols(i * m_InferBatchSize, m_InferBatchSize);
		}

		if (inferLastBatchSize > 0)
		{
			m_InferInputBatches.push_back(m_InferInput.slice_cols(inferBatchCount * m_InferBatchSize, inferLastBatchSize));
			m_InferOutputBatches.push_back(m_InferOutput.slice_cols(inferBatchCount * m_InferBatchSize, inferLastBatchSize));
		}

		// Init train buffers
		m_TrainInputBatches.resize(m_TrainBatchCount);
		m_TrainTargetBatches.resize(m_TrainBatchCount);

		for (uint32_t i = 0; i < m_TrainBatchCount; i++)
		{
			m_TrainInputBatches[i] = m_TrainInput.slice_cols(i * m_TrainBatchSize, m_TrainBatchSize);
			m_TrainTargetBatches[i] = m_TrainTarget.slice_cols(i * m_TrainBatchSize, m_TrainBatchSize);
		}

		en::Log::Info("Infer batch count: " + std::to_string(m_InferInputBatches.size()));
	}

	void NeuralRadianceCache::InferAndTrain(const uint32_t* inferFilter, bool train)
	{
		AwaitCudaStartSemaphore();
		Inference(inferFilter);
		if (train) { Train(); }
		SignalCudaFinishedSemaphore();
	}

	void NeuralRadianceCache::Destroy()
	{
	}

	float NeuralRadianceCache::GetLoss() const
	{
		return m_Loss;
	}

	size_t NeuralRadianceCache::GetInferBatchCount() const
	{
		return m_InferInputBatches.size();
	}

	size_t NeuralRadianceCache::GetTrainBatchCount() const
	{
		return m_TrainInputBatches.size();
	}

	uint32_t NeuralRadianceCache::GetInferBatchSize() const
	{
		return m_InferBatchSize;
	}

	uint32_t NeuralRadianceCache::GetTrainBatchSize() const
	{
		return m_TrainBatchSize;
	}

	void NeuralRadianceCache::Inference(const uint32_t* inferFilter)
	{
		for (size_t i = 0; i < m_InferInputBatches.size(); i++)
		{
			if (inferFilter[i] > 0)
			{
				const tcnn::GPUMatrix<float>& inputBatch = m_InferInputBatches[i];
				tcnn::GPUMatrix<float>& outputBatch = m_InferOutputBatches[i];
				m_Model.network->inference(inputBatch, outputBatch);
			}
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
