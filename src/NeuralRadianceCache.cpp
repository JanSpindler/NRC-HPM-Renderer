#include <engine/graphics/NeuralRadianceCache.hpp>
#include <random>

namespace en
{
	NeuralRadianceCache::NeuralRadianceCache(PosEncoding posEncoding, DirEncoding dirEncoding, size_t layerCount, size_t layerWidth) :
		m_PosEncoding(posEncoding),
		m_DirEncoding(dirEncoding),
		m_LayerCount(layerCount),
		m_LayerWidth(layerWidth),
		m_UniformData({
			.posFreqCount = 0,
			.posMinFreq = 0,
			.posMaxFreq = 0,
			.posLevelCount = 0,
			.posFeatureCount = 0,
			.dirFreqCount = 0,
			.dirFeatureCount = 0 })
	{
	}

	void NeuralRadianceCache::SetPosFrequencyEncoding(uint32_t freqCount)
	{
		m_UniformData.posFreqCount = freqCount;
	}

	void NeuralRadianceCache::SetPosMrheEncoding(uint32_t minFreq, uint32_t maxFreq, uint32_t levelCount, uint32_t featureCount)
	{
		m_UniformData.posMinFreq = minFreq;
		m_UniformData.posMaxFreq = maxFreq;
		m_UniformData.posLevelCount = levelCount;
		m_UniformData.posFeatureCount = featureCount;
	}

	void NeuralRadianceCache::SetDirFrequencyEncoding(uint32_t freqCount)
	{
		m_UniformData.dirFreqCount = freqCount;
	}
	
	void NeuralRadianceCache::SetDirOneBlobEncoding(uint32_t featureCount)
	{
		m_UniformData.dirFeatureCount = featureCount;
	}
	
	void NeuralRadianceCache::Init()
	{
		InitNn();

		if (m_PosEncoding == PosEncoding::Mrhe)
		{
			InitMrhe();
		}
	}
	
	void NeuralRadianceCache::Destroy()
	{

	}

	uint32_t NeuralRadianceCache::GetInputFeatureCount() const
	{
		return m_InputFeatureCount;
	}

	void NeuralRadianceCache::InitNn()
	{
		size_t weightsBufferSize = 
			(m_InputFeatureCount * m_LayerWidth) + 
			(m_LayerWidth * m_LayerWidth * (m_LayerCount - 1)) +
			(m_LayerWidth * 3);
		weightsBufferSize *= sizeof(float);

		size_t biasesBufferSize = ((m_LayerCount) * m_LayerWidth) + 3;
		biasesBufferSize *= sizeof(float);

		CreateNnBuffers(weightsBufferSize, biasesBufferSize);
		FillNnBuffers(weightsBufferSize, biasesBufferSize);
	}

	void NeuralRadianceCache::CreateNnBuffers(size_t weightsBufferSize, size_t biasesBufferSize)
	{
		m_WeightsBuffer = new vk::Buffer(
			weightsBufferSize,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			{});

		m_DeltaWeightsBuffer = new vk::Buffer(
			weightsBufferSize,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			{});

		m_MomentumWeightsBuffer = new vk::Buffer(
			weightsBufferSize,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			{});

		m_BiasesBuffer = new vk::Buffer(
			biasesBufferSize,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			{});

		m_DeltaBiasesBuffer = new vk::Buffer(
			biasesBufferSize,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			{});

		m_MomentumBiasesBuffer = new vk::Buffer(
			biasesBufferSize,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			{});
	}

	void NeuralRadianceCache::FillNnBuffers(size_t weightsBufferSize, size_t biasesBufferSize)
	{
		std::default_random_engine generator((std::random_device()()));

		vk::Buffer weightsStagingBuffer(
			weightsBufferSize, 
			VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, 
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
			{});
		float* weightsData = reinterpret_cast<float*>(malloc(weightsBufferSize));
		const size_t weightsCount = weightsBufferSize / sizeof(float);
		std::normal_distribution<float> weightsDist(0.0f, 1.0 / static_cast<float>(m_LayerWidth));

		for (size_t i = 0; i < weightsCount; i++)
		{
			weightsData[i] = weightsDist(generator);
		}
		weightsStagingBuffer.SetData(weightsBufferSize, weightsData, 0, 0);
		vk::Buffer::Copy(&weightsStagingBuffer, m_WeightsBuffer, weightsBufferSize);

		for (size_t i = 0; i < weightsCount; i++)
		{
			weightsData[i] = 0.0f;
		}
		weightsStagingBuffer.SetData(weightsBufferSize, weightsData, 0, 0);
		vk::Buffer::Copy(&weightsStagingBuffer, m_DeltaWeightsBuffer, weightsBufferSize);
		vk::Buffer::Copy(&weightsStagingBuffer, m_MomentumWeightsBuffer, weightsBufferSize);
	}

	void NeuralRadianceCache::InitMrhe()
	{

	}
}
