#include <engine/cuda_common.hpp>
#include <engine/graphics/NeuralRadianceCache.hpp>
#include <random>
#include <engine/util/Log.hpp>

namespace en
{
	const uint32_t NeuralRadianceCache::sc_InputCount = 5;
	const uint32_t NeuralRadianceCache::sc_OutputCount = 3;

	NeuralRadianceCache::NeuralRadianceCache(const AppConfig& appConfig) :
		m_BatchSize(2 << (appConfig.log2BatchSize - 1))
	{
		nlohmann::json modelConfig = {
			{"loss", {
				{"otype", appConfig.lossFn}
			}},
			{"optimizer", {
				{"otype", appConfig.optimizer},
				{"learning_rate", appConfig.learningRate},
			}},
			appConfig.encoding.jsonConfig,
			//{"encoding", {
			//	{"otype", "Composite"},
			//	{"reduction", "Concatenation"},
			//	{"nested", {
			//		{
			//			{"otype", "HashGrid"},
			//			{"n_dims_to_encode", 3},
			//			{"n_levels", 16},
			//			{"n_features_per_level", 2},
			//			{"log2_hashmap_size", 19},
			//			{"base_resolution", 16},
			//			{"per_level_scale", 2.0},
			//		},
			//		{
			//			{"otype", "OneBlob"},
			//			{"n_dims_to_encode", 2},
			//			{"n_bins", 4},
			//		},
			//	}},
			//}},
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
			m_InferInputBatches[i] = tcnn::GPUMatrix<float>(&(dCuInferInput[floatInputOffset]), sc_InputCount, m_BatchSize);
			m_InferOutputBatches[i] = tcnn::GPUMatrix<float>(&(dCuInferOutput[floatOutputOffset]), sc_OutputCount, m_BatchSize);
			floatInputOffset += m_BatchSize * sc_InputCount;
			floatOutputOffset += m_BatchSize * sc_OutputCount;
		}

		if (inferLastBatchSize > 0)
		{
			m_InferInputBatches.push_back(tcnn::GPUMatrix<float>(&(dCuInferInput[floatInputOffset]), sc_InputCount, inferLastBatchSize));
			m_InferOutputBatches.push_back(tcnn::GPUMatrix<float>(&(dCuInferOutput[floatOutputOffset]), sc_OutputCount, inferLastBatchSize));
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
			m_TrainInputBatches[i] = tcnn::GPUMatrix<float>(&(dCuTrainInput[floatInputOffset]), sc_InputCount, m_BatchSize);
			m_TrainTargetBatches[i] = tcnn::GPUMatrix<float>(&(dCuTrainTarget[floatOutputOffset]), sc_OutputCount, m_BatchSize);
			floatInputOffset += m_BatchSize * sc_InputCount;
			floatOutputOffset += m_BatchSize * sc_OutputCount;
		}

		if (trainLastBatchSize > 0)
		{
			m_TrainInputBatches.push_back(tcnn::GPUMatrix<float>(&(dCuTrainInput[floatInputOffset]), sc_InputCount, trainLastBatchSize));
			m_TrainTargetBatches.push_back(tcnn::GPUMatrix<float>(&(dCuTrainTarget[floatOutputOffset]), sc_OutputCount, trainLastBatchSize));
		}
	}

	void NeuralRadianceCache::InferAndTrain(const uint32_t* inferFilter)
	{
		AwaitCudaStartSemaphore();
		Inference(inferFilter);
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

	size_t NeuralRadianceCache::GetInferBatchCount() const
	{
		return m_InferInputBatches.size();
	}

	size_t NeuralRadianceCache::GetTrainBatchCount() const
	{
		return m_TrainInputBatches.size();
	}

	uint32_t NeuralRadianceCache::GetBatchSize() const
	{
		return m_BatchSize;
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

		//m_TrainCounter++;
		//if (m_TrainCounter == 32)
		//{
		//	m_Model.trainer.get()->update_hyperparams(
		//		{"optimizer", {
		//			{"otype", "Adam"},
		//			{"learning_rate", 1e-3f},
		//			{"absolute_decay", 0.0f}
		//		}});
		//	en::Log::Info("Hello");
		//}
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
