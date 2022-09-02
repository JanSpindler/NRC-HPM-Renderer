#include <engine/graphics/NeuralRadianceCache.hpp>
#include <random>

namespace en
{
	VkDescriptorSetLayout NeuralRadianceCache::m_DescSetLayout;
	VkDescriptorPool NeuralRadianceCache::m_DescPool;
	
	void NeuralRadianceCache::Init(VkDevice device)
	{
		// Create layout
		VkDescriptorSetLayoutBinding renderNeuronsBufferBinding;
		renderNeuronsBufferBinding.binding = 0;
		renderNeuronsBufferBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		renderNeuronsBufferBinding.descriptorCount = 1;
		renderNeuronsBufferBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
		renderNeuronsBufferBinding.pImmutableSamplers = nullptr;

		VkDescriptorSetLayoutBinding trainNeuronsBufferBinding;
		trainNeuronsBufferBinding.binding = 1;
		trainNeuronsBufferBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		trainNeuronsBufferBinding.descriptorCount = 1;
		trainNeuronsBufferBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
		trainNeuronsBufferBinding.pImmutableSamplers = nullptr;

		VkDescriptorSetLayoutBinding weightsBufferBinding;
		weightsBufferBinding.binding = 2;
		weightsBufferBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		weightsBufferBinding.descriptorCount = 1;
		weightsBufferBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
		weightsBufferBinding.pImmutableSamplers = nullptr;

		VkDescriptorSetLayoutBinding deltaWeightsBufferBinding;
		deltaWeightsBufferBinding.binding = 3;
		deltaWeightsBufferBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		deltaWeightsBufferBinding.descriptorCount = 1;
		deltaWeightsBufferBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
		deltaWeightsBufferBinding.pImmutableSamplers = nullptr;

		VkDescriptorSetLayoutBinding momentumWeightsBufferBinding;
		momentumWeightsBufferBinding.binding = 4;
		momentumWeightsBufferBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		momentumWeightsBufferBinding.descriptorCount = 1;
		momentumWeightsBufferBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
		momentumWeightsBufferBinding.pImmutableSamplers = nullptr;

		VkDescriptorSetLayoutBinding biasesBufferBinding;
		biasesBufferBinding.binding = 5;
		biasesBufferBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		biasesBufferBinding.descriptorCount = 1;
		biasesBufferBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
		biasesBufferBinding.pImmutableSamplers = nullptr;

		VkDescriptorSetLayoutBinding deltaBiasesBufferBinding;
		deltaBiasesBufferBinding.binding = 6;
		deltaBiasesBufferBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		deltaBiasesBufferBinding.descriptorCount = 1;
		deltaBiasesBufferBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
		deltaBiasesBufferBinding.pImmutableSamplers = nullptr;

		VkDescriptorSetLayoutBinding momentumBiasesBufferBinding;
		momentumBiasesBufferBinding.binding = 7;
		momentumBiasesBufferBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		momentumBiasesBufferBinding.descriptorCount = 1;
		momentumBiasesBufferBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
		momentumBiasesBufferBinding.pImmutableSamplers = nullptr;

		VkDescriptorSetLayoutBinding mrheBufferBinding;
		mrheBufferBinding.binding = 8;
		mrheBufferBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		mrheBufferBinding.descriptorCount = 1;
		mrheBufferBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
		mrheBufferBinding.pImmutableSamplers = nullptr;

		VkDescriptorSetLayoutBinding deltaMrheBufferBinding;
		deltaMrheBufferBinding.binding = 9;
		deltaMrheBufferBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		deltaMrheBufferBinding.descriptorCount = 1;
		deltaMrheBufferBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
		deltaMrheBufferBinding.pImmutableSamplers = nullptr;

		VkDescriptorSetLayoutBinding mrheResBufferBinding;
		mrheResBufferBinding.binding = 10;
		mrheResBufferBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		mrheResBufferBinding.descriptorCount = 1;
		mrheResBufferBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
		mrheResBufferBinding.pImmutableSamplers = nullptr;

		std::vector<VkDescriptorSetLayoutBinding> bindings = {
			renderNeuronsBufferBinding,
			trainNeuronsBufferBinding,
			weightsBufferBinding,
			deltaWeightsBufferBinding,
			momentumWeightsBufferBinding,
			biasesBufferBinding,
			deltaBiasesBufferBinding,
			momentumBiasesBufferBinding,
			mrheBufferBinding,
			deltaMrheBufferBinding,
			mrheResBufferBinding };

		VkDescriptorSetLayoutCreateInfo layoutCI;
		layoutCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutCI.pNext = nullptr;
		layoutCI.flags = 0;
		layoutCI.bindingCount = bindings.size();
		layoutCI.pBindings = bindings.data();

		VkResult result = vkCreateDescriptorSetLayout(device, &layoutCI, nullptr, &m_DescSetLayout);
		ASSERT_VULKAN(result);

		// Create pool
		VkDescriptorPoolSize storagePoolSize;
		storagePoolSize.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		storagePoolSize.descriptorCount = 11;

		std::vector<VkDescriptorPoolSize> poolSizes = { storagePoolSize };

		VkDescriptorPoolCreateInfo poolCI;
		poolCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolCI.pNext = nullptr;
		poolCI.flags = 0;
		poolCI.maxSets = 1;
		poolCI.poolSizeCount = poolSizes.size();
		poolCI.pPoolSizes = poolSizes.data();

		result = vkCreateDescriptorPool(device, &poolCI, nullptr, &m_DescPool);
		ASSERT_VULKAN(result);
	}

	void NeuralRadianceCache::Shutdown(VkDevice device)
	{
		vkDestroyDescriptorPool(device, m_DescPool, nullptr);
		vkDestroyDescriptorSetLayout(device, m_DescSetLayout, nullptr);
	}

	VkDescriptorSetLayout NeuralRadianceCache::GetDescriptorSetLayout()
	{
		return m_DescSetLayout;
	}

	NeuralRadianceCache::NeuralRadianceCache(size_t layerCount, size_t layerWidth, float nrcLearningRate, uint32_t batchSize) :
		m_PosEncoding(PosEncoding::Direct),
		m_DirEncoding(DirEncoding::Direct),
		m_LayerCount(layerCount),
		m_LayerWidth(layerWidth),
		m_NrcLearningRate(nrcLearningRate),
		m_BatchSize(batchSize),
		m_MrheLearningRate(0.0f),
		m_InputFeatureCount(0),
		m_PosFreqCount(0),
		m_PosMinFreq(0),
		m_PosMaxFreq(0),
		m_PosLevelCount(0),
		m_PosFeatureCount(0),
		m_DirFreqCount(0),
		m_DirFeatureCount(0)
	{
		if (layerCount < 1)
		{
			Log::Error("NeuralRadianceCache needs at least 1 hidden layer", true);
		}
	}

	void NeuralRadianceCache::SetPosFrequencyEncoding(uint32_t freqCount)
	{
		if (m_PosEncoding == PosEncoding::Direct)
		{
			m_PosEncoding = PosEncoding::Direct_Frequency;
		}

		m_PosFreqCount = freqCount;
	}

	void NeuralRadianceCache::SetPosMrheEncoding(
		uint32_t minFreq, 
		uint32_t maxFreq, 
		uint32_t levelCount, 
		uint32_t hashTableSize, 
		uint32_t featureCount, 
		float learningRate)
	{
		m_PosEncoding = PosEncoding::Direct_Frequency_Mrhe;

		m_PosMinFreq = minFreq;
		m_PosMaxFreq = maxFreq;
		m_PosLevelCount = levelCount;
		m_PosHashTableSize = hashTableSize;
		m_PosFeatureCount = featureCount;
		m_MrheLearningRate = learningRate;
	}

	void NeuralRadianceCache::SetDirFrequencyEncoding(uint32_t freqCount)
	{
		if (m_DirEncoding == DirEncoding::Direct)
		{
			m_DirEncoding = DirEncoding::Direct_Frequency;
		}

		m_DirFreqCount = freqCount;
	}
	
	void NeuralRadianceCache::SetDirOneBlobEncoding(uint32_t featureCount)
	{
		m_DirEncoding = DirEncoding::Direct_Frequency_OneBlob;

		m_DirFeatureCount = featureCount;
	}
	
	void NeuralRadianceCache::Init(size_t renderSampleCount, size_t trainSampleCount)
	{
		// Calc input feature count
		m_InputFeatureCount = 5;
		switch (m_PosEncoding)
		{
		case PosEncoding::Direct_Frequency:
			m_InputFeatureCount += 2 * 3 * m_PosFreqCount;
			break;
		case PosEncoding::Direct_Frequency_Mrhe:
			m_InputFeatureCount += (2 * 3 * m_PosFreqCount) + (m_PosFeatureCount * m_PosLevelCount);
			break;
		default:
			break;
		}

		switch (m_DirEncoding)
		{
		case DirEncoding::Direct_Frequency:
			m_InputFeatureCount += 2 * 2 * m_DirFreqCount;
			break;
		case DirEncoding::Direct_Frequency_OneBlob:
			m_InputFeatureCount += (2 * 2 * m_DirFreqCount) + (m_DirFeatureCount * 2);
			break;
		default:
			break;
		}

		Log::Info("NeuralRadianceCache has " + std::to_string(m_InputFeatureCount) + " input features");

		// Init
		InitNn(renderSampleCount, trainSampleCount);
		InitMrhe();

		AllocateAndUpdateDescSet();
	}
	
	void NeuralRadianceCache::Destroy()
	{
		m_RenderNeuronsBuffer->Destroy();
		m_TrainNeuronsBuffer->Destroy();
		m_WeightsBuffer->Destroy();
		m_DeltaWeightsBuffer->Destroy();
		m_MomentumWeightsBuffer->Destroy();
		m_BiasesBuffer->Destroy();
		m_DeltaBiasesBuffer->Destroy();
		m_MomentumBiasesBuffer->Destroy();
		m_MrheBuffer->Destroy();
		m_DeltaMrheBuffer->Destroy();
		m_MrheResolutionsBuffer->Destroy();

		delete m_RenderNeuronsBuffer;
		delete m_TrainNeuronsBuffer;
		delete m_WeightsBuffer;
		delete m_DeltaWeightsBuffer;
		delete m_MomentumWeightsBuffer;
		delete m_BiasesBuffer;
		delete m_DeltaBiasesBuffer;
		delete m_MomentumBiasesBuffer;
		delete m_MrheBuffer;
		delete m_DeltaMrheBuffer;
	}

	uint32_t NeuralRadianceCache::GetPosEncoding() const
	{
		return static_cast<uint32_t>(m_PosEncoding);
	}

	uint32_t NeuralRadianceCache::GetDirEncoding() const
	{
		return static_cast<uint32_t>(m_DirEncoding);
	}

	uint32_t NeuralRadianceCache::GetPosFreqCount() const
	{
		return m_PosFreqCount;
	}

	uint32_t NeuralRadianceCache::GetPosMinFreq() const
	{
		return m_PosMinFreq;
	}
	
	uint32_t NeuralRadianceCache::GetPosMaxFreq() const
	{
		return m_PosMaxFreq;
	}
	
	uint32_t NeuralRadianceCache::GetPosLevelCount() const
	{
		return m_PosLevelCount;
	}
	
	uint32_t NeuralRadianceCache::GetPosHashTableSize() const
	{
		return m_PosHashTableSize;
	}
	
	uint32_t NeuralRadianceCache::GetPosFeatureCount() const
	{
		return m_PosFeatureCount;
	}

	uint32_t NeuralRadianceCache::GetDirFreqCount() const
	{
		return m_DirFreqCount;
	}

	uint32_t NeuralRadianceCache::GetDirFeatureCount() const
	{
		return m_DirFeatureCount;
	}

	uint32_t NeuralRadianceCache::GetLayerCount() const
	{
		return static_cast<uint32_t>(m_LayerCount);
	}

	uint32_t NeuralRadianceCache::GetLayerWidth() const
	{
		return static_cast<uint32_t>(m_LayerWidth);
	}

	uint32_t NeuralRadianceCache::GetInputFeatureCount() const
	{
		return m_InputFeatureCount;
	}

	float NeuralRadianceCache::GetNrcLearningRate() const
	{
		return m_NrcLearningRate;
	}

	float NeuralRadianceCache::GetMrheLearningRate() const
	{
		return m_MrheLearningRate;
	}

	uint32_t NeuralRadianceCache::GetBatchSize() const
	{
		return m_BatchSize;
	}

	size_t NeuralRadianceCache::GetWeightsCount() const
	{
		return m_WeightsCount;
	}

	size_t NeuralRadianceCache::GetBiasesCount() const
	{
		return m_BiasesCount;
	}

	size_t NeuralRadianceCache::GetMrheCount() const
	{
		return m_MrheCount;
	}

	VkDescriptorSet NeuralRadianceCache::GetDescriptorSet() const
	{
		return m_DescSet;
	}

	void NeuralRadianceCache::InitNn(size_t renderSampleCount, size_t trainSampleCount)
	{
		const size_t neuronsPerSample = m_InputFeatureCount + (m_LayerCount * m_LayerWidth) + 3;
		m_RenderNeuronsBufferSize = neuronsPerSample * renderSampleCount * sizeof(float);
		m_TrainNeuronsBufferSize = neuronsPerSample * trainSampleCount * sizeof(float);

		m_WeightsCount = 
			(m_InputFeatureCount * m_LayerWidth) + 
			(m_LayerWidth * m_LayerWidth * (m_LayerCount - 1)) +
			(m_LayerWidth * 3);
		m_WeightsBufferSize = m_WeightsCount * sizeof(float);

		m_BiasesCount = ((m_LayerCount) * m_LayerWidth) + 3;
		m_BiasesBufferSize = m_BiasesCount * sizeof(float);

		CreateNnBuffers();
		FillNnBuffers();
	}

	void NeuralRadianceCache::CreateNnBuffers()
	{
		// Neurons
		m_RenderNeuronsBuffer = new vk::Buffer(
			m_RenderNeuronsBufferSize, 
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, 
			{});

		m_TrainNeuronsBuffer = new vk::Buffer(
			m_TrainNeuronsBufferSize,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			{});

		// Weights
		m_WeightsBuffer = new vk::Buffer(
			m_WeightsBufferSize,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			{});

		m_DeltaWeightsBuffer = new vk::Buffer(
			m_WeightsBufferSize,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			{});

		m_MomentumWeightsBuffer = new vk::Buffer(
			m_WeightsBufferSize,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			{});

		// Biases
		m_BiasesBuffer = new vk::Buffer(
			m_BiasesBufferSize,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			{});

		m_DeltaBiasesBuffer = new vk::Buffer(
			m_BiasesBufferSize,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			{});

		m_MomentumBiasesBuffer = new vk::Buffer(
			m_BiasesBufferSize,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			{});
	}

	void NeuralRadianceCache::FillNnBuffers()
	{
		std::default_random_engine generator((std::random_device()()));

		// Render neurons
		vk::Buffer neuronsStagingBuffer(
			m_RenderNeuronsBufferSize,
			VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			{});
		float* neuronsData = reinterpret_cast<float*>(malloc(m_RenderNeuronsBufferSize));
		const size_t renderNeuronsCount = m_RenderNeuronsBufferSize / sizeof(float);
		
		for (size_t i = 0; i < renderNeuronsCount; i++)
		{
			neuronsData[i] = 0.0f;
		}
		neuronsStagingBuffer.SetData(m_RenderNeuronsBufferSize, neuronsData, 0, 0);
		vk::Buffer::Copy(&neuronsStagingBuffer, m_RenderNeuronsBuffer, m_RenderNeuronsBufferSize);

		free(neuronsData);
		neuronsStagingBuffer.Destroy();

		// Train neurons
		neuronsStagingBuffer = vk::Buffer(
			m_TrainNeuronsBufferSize,
			VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			{});
		neuronsData = reinterpret_cast<float*>(malloc(m_TrainNeuronsBufferSize));
		const size_t trainNeuronsCount = m_TrainNeuronsBufferSize / sizeof(float);

		for (size_t i = 0; i < trainNeuronsCount; i++)
		{
			neuronsData[i] = 0.0f;
		}
		neuronsStagingBuffer.SetData(m_TrainNeuronsBufferSize, neuronsData, 0, 0);
		vk::Buffer::Copy(&neuronsStagingBuffer, m_TrainNeuronsBuffer, m_TrainNeuronsBufferSize);

		free(neuronsData);
		neuronsStagingBuffer.Destroy();

		// Weights
		vk::Buffer weightsStagingBuffer(
			m_WeightsBufferSize, 
			VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, 
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
			{});
		float* weightsData = reinterpret_cast<float*>(malloc(m_WeightsBufferSize));
		const size_t weightsCount = m_WeightsBufferSize / sizeof(float);
		std::normal_distribution<float> weightsDist(0.0f, 1.0 / static_cast<float>(m_LayerWidth));

		for (size_t i = 0; i < weightsCount; i++)
		{
			weightsData[i] = weightsDist(generator);
		}
		weightsStagingBuffer.SetData(m_WeightsBufferSize, weightsData, 0, 0);
		vk::Buffer::Copy(&weightsStagingBuffer, m_WeightsBuffer, m_WeightsBufferSize);

		for (size_t i = 0; i < weightsCount; i++)
		{
			weightsData[i] = 0.0f;
		}
		weightsStagingBuffer.SetData(m_WeightsBufferSize, weightsData, 0, 0);
		vk::Buffer::Copy(&weightsStagingBuffer, m_DeltaWeightsBuffer, m_WeightsBufferSize);
		vk::Buffer::Copy(&weightsStagingBuffer, m_MomentumWeightsBuffer, m_WeightsBufferSize);

		free(weightsData);
		weightsStagingBuffer.Destroy();

		// Biases
		vk::Buffer biasesStagingBuffer(
			m_BiasesBufferSize,
			VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			{});
		float* biasesData = reinterpret_cast<float*>(malloc(m_BiasesBufferSize));
		const size_t biasesCount = m_BiasesBufferSize / sizeof(float);
		std::normal_distribution<float> biasesDist(0.125f, 1.0 / static_cast<float>(m_LayerWidth));

		for (size_t i = 0; i < biasesCount; i++)
		{
			biasesData[i] = biasesDist(generator);
		}
		biasesStagingBuffer.SetData(m_BiasesBufferSize, biasesData, 0, 0);
		vk::Buffer::Copy(&biasesStagingBuffer, m_BiasesBuffer, m_BiasesBufferSize);
		
		for (size_t i = 0; i < biasesCount; i++)
		{
			biasesData[i] = 0.0f;
		}
		biasesStagingBuffer.SetData(m_BiasesBufferSize, biasesData, 0, 0);
		vk::Buffer::Copy(&biasesStagingBuffer, m_DeltaBiasesBuffer, m_BiasesBufferSize);
		vk::Buffer::Copy(&biasesStagingBuffer, m_MomentumBiasesBuffer, m_BiasesBufferSize);

		free(biasesData);
		biasesStagingBuffer.Destroy();
	}

	void NeuralRadianceCache::InitMrhe()
	{
		std::default_random_engine generator((std::random_device()()));

		// Calc size
		m_MrheCount = m_PosFeatureCount * m_PosLevelCount * m_PosHashTableSize;
		m_MrheBufferSize = m_MrheCount * sizeof(float);
		m_MrheBufferSize = std::max(m_MrheBufferSize, static_cast<size_t>(1)); // need minimum size > 0

		m_MrheResolutionsBufferSize = std::max(static_cast<size_t>(1), m_PosLevelCount * sizeof(uint32_t)); // need minimum size > 0

		// Create buffers
		m_MrheBuffer = new vk::Buffer(
			m_MrheBufferSize,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			{});

		m_DeltaMrheBuffer = new vk::Buffer(
			m_MrheBufferSize,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			{});

		m_MrheResolutionsBuffer = new vk::Buffer(
			m_MrheResolutionsBufferSize,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			{});

		// Fill buffers
		vk::Buffer mrheStagingBuffer(
			m_MrheBufferSize,
			VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			{});
		float* mrheData = reinterpret_cast<float*>(malloc(m_MrheBufferSize));
		std::normal_distribution<float> mrheDist(0.0, 0.5f);

		for (size_t i = 0; i < m_MrheCount; i++)
		{
			mrheData[i] = mrheDist(generator);
		}
		mrheStagingBuffer.SetData(m_MrheBufferSize, mrheData, 0, 0);
		vk::Buffer::Copy(&mrheStagingBuffer, m_MrheBuffer, m_MrheBufferSize);

		for (size_t i = 0; i < m_MrheCount; i++)
		{
			mrheData[i] = 0.0f;
		}
		mrheStagingBuffer.SetData(m_MrheBufferSize, mrheData, 0, 0);
		vk::Buffer::Copy(&mrheStagingBuffer, m_DeltaMrheBuffer, m_MrheBufferSize);

		free(mrheData);
		mrheStagingBuffer.Destroy();

		// Res
		vk::Buffer mrheResStagingBuffer(
			m_MrheResolutionsBufferSize,
			VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			{});

		uint32_t* mrheResolutions = reinterpret_cast<uint32_t*>(malloc(m_MrheResolutionsBufferSize));

		// Init mrhe resolutions
		float b = std::exp(
			(std::log(static_cast<float>(m_PosMaxFreq)) - std::log(static_cast<float>(m_PosMinFreq))) /
			static_cast<float>(m_PosLevelCount - 1));
		for (size_t i = 0; i < m_PosLevelCount; i++)
		{
			float resF =
				static_cast<float>(m_PosMinFreq) *
				std::pow(b, static_cast<float>(i));
			mrheResolutions[i] = static_cast<uint32_t>(resF);
		}

		mrheResStagingBuffer.SetData(m_MrheResolutionsBufferSize, mrheResolutions, 0, 0);
		vk::Buffer::Copy(&mrheResStagingBuffer, m_MrheResolutionsBuffer, m_MrheResolutionsBufferSize);

		free(mrheResolutions);
		mrheResStagingBuffer.Destroy();
	}

	void NeuralRadianceCache::AllocateAndUpdateDescSet()
	{
		VkDevice device = VulkanAPI::GetDevice();
		
		// Allocate desc set
		VkDescriptorSetAllocateInfo descSetAI;
		descSetAI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		descSetAI.pNext = nullptr;
		descSetAI.descriptorPool = m_DescPool;
		descSetAI.descriptorSetCount = 1;
		descSetAI.pSetLayouts = &m_DescSetLayout;

		VkResult result = vkAllocateDescriptorSets(device, &descSetAI, &m_DescSet);
		ASSERT_VULKAN(result);

		// Update desc set
		std::vector<VkDescriptorBufferInfo> bufferInfos(11);
		for (size_t i = 0; i < bufferInfos.size(); i++)
		{
			bufferInfos[i].offset = 0;
		}

		bufferInfos[0].buffer = m_RenderNeuronsBuffer->GetVulkanHandle();
		bufferInfos[0].range = m_RenderNeuronsBufferSize;
		bufferInfos[1].buffer = m_TrainNeuronsBuffer->GetVulkanHandle();
		bufferInfos[1].range = m_TrainNeuronsBufferSize;

		bufferInfos[2].buffer = m_WeightsBuffer->GetVulkanHandle();
		bufferInfos[2].range = m_WeightsBufferSize;
		bufferInfos[3].buffer = m_DeltaWeightsBuffer->GetVulkanHandle();
		bufferInfos[3].range = m_WeightsBufferSize;
		bufferInfos[4].buffer = m_MomentumWeightsBuffer->GetVulkanHandle();
		bufferInfos[4].range = m_WeightsBufferSize;

		bufferInfos[5].buffer = m_BiasesBuffer->GetVulkanHandle();
		bufferInfos[5].range = m_BiasesBufferSize;
		bufferInfos[6].buffer = m_DeltaBiasesBuffer->GetVulkanHandle();
		bufferInfos[6].range = m_BiasesBufferSize;
		bufferInfos[7].buffer = m_MomentumBiasesBuffer->GetVulkanHandle();
		bufferInfos[7].range = m_BiasesBufferSize;

		bufferInfos[8].buffer = m_MrheBuffer->GetVulkanHandle();
		bufferInfos[8].range = m_MrheBufferSize;
		bufferInfos[9].buffer = m_DeltaMrheBuffer->GetVulkanHandle();
		bufferInfos[9].range = m_MrheBufferSize;
		bufferInfos[10].buffer = m_MrheResolutionsBuffer->GetVulkanHandle();
		bufferInfos[10].range = m_MrheResolutionsBufferSize;

		std::vector<VkWriteDescriptorSet> writes(bufferInfos.size());
		for (size_t i = 0; i < writes.size(); i++)
		{
			writes[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			writes[i].pNext = nullptr;
			writes[i].dstSet = m_DescSet;
			writes[i].dstBinding = i;
			writes[i].dstArrayElement = 0;
			writes[i].descriptorCount = 1;
			writes[i].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			writes[i].pImageInfo = nullptr;
			writes[i].pBufferInfo = &bufferInfos[i];
			writes[i].pTexelBufferView = nullptr;
		}

		vkUpdateDescriptorSets(device, writes.size(), writes.data(), 0, nullptr);
	}
}
