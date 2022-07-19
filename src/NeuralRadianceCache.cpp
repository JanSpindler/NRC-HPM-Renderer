#include <engine/graphics/NeuralRadianceCache.hpp>
#include <random>

namespace en
{
	VkDescriptorSetLayout NeuralRadianceCache::m_DescSetLayout;
	VkDescriptorPool NeuralRadianceCache::m_DescPool;

	void NeuralRadianceCache::Init(VkDevice device)
	{
		// Create desc set layout
		// Weights
		VkDescriptorSetLayoutBinding weights0Binding;
		weights0Binding.binding = 0;
		weights0Binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		weights0Binding.descriptorCount = 1;
		weights0Binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT;
		weights0Binding.pImmutableSamplers = nullptr;

		VkDescriptorSetLayoutBinding weights1Binding;
		weights1Binding.binding = 1;
		weights1Binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		weights1Binding.descriptorCount = 1;
		weights1Binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT;
		weights1Binding.pImmutableSamplers = nullptr;

		VkDescriptorSetLayoutBinding weights2Binding;
		weights2Binding.binding = 2;
		weights2Binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		weights2Binding.descriptorCount = 1;
		weights2Binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT;
		weights2Binding.pImmutableSamplers = nullptr;

		VkDescriptorSetLayoutBinding weights3Binding;
		weights3Binding.binding = 3;
		weights3Binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		weights3Binding.descriptorCount = 1;
		weights3Binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT;
		weights3Binding.pImmutableSamplers = nullptr;

		VkDescriptorSetLayoutBinding weights4Binding;
		weights4Binding.binding = 4;
		weights4Binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		weights4Binding.descriptorCount = 1;
		weights4Binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT;
		weights4Binding.pImmutableSamplers = nullptr;

		VkDescriptorSetLayoutBinding weights5Binding;
		weights5Binding.binding = 5;
		weights5Binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		weights5Binding.descriptorCount = 1;
		weights5Binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT;
		weights5Binding.pImmutableSamplers = nullptr;

		// Delta weights
		VkDescriptorSetLayoutBinding deltaWeights0Binding;
		deltaWeights0Binding.binding = 6;
		deltaWeights0Binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		deltaWeights0Binding.descriptorCount = 1;
		deltaWeights0Binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT;
		deltaWeights0Binding.pImmutableSamplers = nullptr;

		VkDescriptorSetLayoutBinding deltaWeights1Binding;
		deltaWeights1Binding.binding = 7;
		deltaWeights1Binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		deltaWeights1Binding.descriptorCount = 1;
		deltaWeights1Binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT;
		deltaWeights1Binding.pImmutableSamplers = nullptr;

		VkDescriptorSetLayoutBinding deltaWeights2Binding;
		deltaWeights2Binding.binding = 8;
		deltaWeights2Binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		deltaWeights2Binding.descriptorCount = 1;
		deltaWeights2Binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT;
		deltaWeights2Binding.pImmutableSamplers = nullptr;

		VkDescriptorSetLayoutBinding deltaWeights3Binding;
		deltaWeights3Binding.binding = 9;
		deltaWeights3Binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		deltaWeights3Binding.descriptorCount = 1;
		deltaWeights3Binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT;
		deltaWeights3Binding.pImmutableSamplers = nullptr;

		VkDescriptorSetLayoutBinding deltaWeights4Binding;
		deltaWeights4Binding.binding = 10;
		deltaWeights4Binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		deltaWeights4Binding.descriptorCount = 1;
		deltaWeights4Binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT;
		deltaWeights4Binding.pImmutableSamplers = nullptr;

		VkDescriptorSetLayoutBinding deltaWeights5Binding;
		deltaWeights5Binding.binding = 11;
		deltaWeights5Binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		deltaWeights5Binding.descriptorCount = 1;
		deltaWeights5Binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT;
		deltaWeights5Binding.pImmutableSamplers = nullptr;

		// Momentum 1 weights
		VkDescriptorSetLayoutBinding momentum1Weights0Binding;
		momentum1Weights0Binding.binding = 12;
		momentum1Weights0Binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		momentum1Weights0Binding.descriptorCount = 1;
		momentum1Weights0Binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT;
		momentum1Weights0Binding.pImmutableSamplers = nullptr;

		VkDescriptorSetLayoutBinding momentum1Weights1Binding;
		momentum1Weights1Binding.binding = 13;
		momentum1Weights1Binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		momentum1Weights1Binding.descriptorCount = 1;
		momentum1Weights1Binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT;
		momentum1Weights1Binding.pImmutableSamplers = nullptr;

		VkDescriptorSetLayoutBinding momentum1Weights2Binding;
		momentum1Weights2Binding.binding = 14;
		momentum1Weights2Binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		momentum1Weights2Binding.descriptorCount = 1;
		momentum1Weights2Binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT;
		momentum1Weights2Binding.pImmutableSamplers = nullptr;

		VkDescriptorSetLayoutBinding momentum1Weights3Binding;
		momentum1Weights3Binding.binding = 15;
		momentum1Weights3Binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		momentum1Weights3Binding.descriptorCount = 1;
		momentum1Weights3Binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT;
		momentum1Weights3Binding.pImmutableSamplers = nullptr;

		VkDescriptorSetLayoutBinding momentum1Weights4Binding;
		momentum1Weights4Binding.binding = 16;
		momentum1Weights4Binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		momentum1Weights4Binding.descriptorCount = 1;
		momentum1Weights4Binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT;
		momentum1Weights4Binding.pImmutableSamplers = nullptr;

		VkDescriptorSetLayoutBinding momentum1Weights5Binding;
		momentum1Weights5Binding.binding = 17;
		momentum1Weights5Binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		momentum1Weights5Binding.descriptorCount = 1;
		momentum1Weights5Binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT;
		momentum1Weights5Binding.pImmutableSamplers = nullptr;

		// Biases
		VkDescriptorSetLayoutBinding biases0Binding;
		biases0Binding.binding = 18;
		biases0Binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		biases0Binding.descriptorCount = 1;
		biases0Binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT;
		biases0Binding.pImmutableSamplers = nullptr;

		VkDescriptorSetLayoutBinding biases1Binding;
		biases1Binding.binding = 19;
		biases1Binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		biases1Binding.descriptorCount = 1;
		biases1Binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT;
		biases1Binding.pImmutableSamplers = nullptr;

		VkDescriptorSetLayoutBinding biases2Binding;
		biases2Binding.binding = 20;
		biases2Binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		biases2Binding.descriptorCount = 1;
		biases2Binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT;
		biases2Binding.pImmutableSamplers = nullptr;

		VkDescriptorSetLayoutBinding biases3Binding;
		biases3Binding.binding = 21;
		biases3Binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		biases3Binding.descriptorCount = 1;
		biases3Binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT;
		biases3Binding.pImmutableSamplers = nullptr;

		VkDescriptorSetLayoutBinding biases4Binding;
		biases4Binding.binding = 22;
		biases4Binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		biases4Binding.descriptorCount = 1;
		biases4Binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT;
		biases4Binding.pImmutableSamplers = nullptr;

		VkDescriptorSetLayoutBinding biases5Binding;
		biases5Binding.binding = 23;
		biases5Binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		biases5Binding.descriptorCount = 1;
		biases5Binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT;
		biases5Binding.pImmutableSamplers = nullptr;

		// Delta biases
		VkDescriptorSetLayoutBinding deltabiases0Binding;
		deltabiases0Binding.binding = 24;
		deltabiases0Binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		deltabiases0Binding.descriptorCount = 1;
		deltabiases0Binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT;
		deltabiases0Binding.pImmutableSamplers = nullptr;

		VkDescriptorSetLayoutBinding deltabiases1Binding;
		deltabiases1Binding.binding = 25;
		deltabiases1Binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		deltabiases1Binding.descriptorCount = 1;
		deltabiases1Binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT;
		deltabiases1Binding.pImmutableSamplers = nullptr;

		VkDescriptorSetLayoutBinding deltabiases2Binding;
		deltabiases2Binding.binding = 26;
		deltabiases2Binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		deltabiases2Binding.descriptorCount = 1;
		deltabiases2Binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT;
		deltabiases2Binding.pImmutableSamplers = nullptr;

		VkDescriptorSetLayoutBinding deltabiases3Binding;
		deltabiases3Binding.binding = 27;
		deltabiases3Binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		deltabiases3Binding.descriptorCount = 1;
		deltabiases3Binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT;
		deltabiases3Binding.pImmutableSamplers = nullptr;

		VkDescriptorSetLayoutBinding deltabiases4Binding;
		deltabiases4Binding.binding = 28;
		deltabiases4Binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		deltabiases4Binding.descriptorCount = 1;
		deltabiases4Binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT;
		deltabiases4Binding.pImmutableSamplers = nullptr;

		VkDescriptorSetLayoutBinding deltabiases5Binding;
		deltabiases5Binding.binding = 29;
		deltabiases5Binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		deltabiases5Binding.descriptorCount = 1;
		deltabiases5Binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT;
		deltabiases5Binding.pImmutableSamplers = nullptr;

		// Config uniform buffer
		VkDescriptorSetLayoutBinding configBinding;
		configBinding.binding = 30;
		configBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		configBinding.descriptorCount = 1;
		configBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
		configBinding.pImmutableSamplers = nullptr;

		// Stats buffer
		VkDescriptorSetLayoutBinding statsBinding;
		statsBinding.binding = 31;
		statsBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		statsBinding.descriptorCount = 1;
		statsBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
		statsBinding.pImmutableSamplers = nullptr;

		std::vector<VkDescriptorSetLayoutBinding> layoutBindings = {
			weights0Binding,
			weights1Binding,
			weights2Binding,
			weights3Binding,
			weights4Binding,
			weights5Binding,
			deltaWeights0Binding,
			deltaWeights1Binding,
			deltaWeights2Binding,
			deltaWeights3Binding,
			deltaWeights4Binding,
			deltaWeights5Binding,
			momentum1Weights0Binding,
			momentum1Weights1Binding,
			momentum1Weights2Binding,
			momentum1Weights3Binding,
			momentum1Weights4Binding,
			momentum1Weights5Binding,
			biases0Binding,
			biases1Binding,
			biases2Binding,
			biases3Binding,
			biases4Binding,
			biases5Binding,
			deltabiases0Binding,
			deltabiases1Binding,
			deltabiases2Binding,
			deltabiases3Binding,
			deltabiases4Binding,
			deltabiases5Binding,
			configBinding,
			statsBinding };

		VkDescriptorSetLayoutCreateInfo layoutCI;
		layoutCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutCI.pNext = nullptr;
		layoutCI.flags = 0;
		layoutCI.bindingCount = layoutBindings.size();
		layoutCI.pBindings = layoutBindings.data();

		VkResult result = vkCreateDescriptorSetLayout(device, &layoutCI, nullptr, &m_DescSetLayout);
		ASSERT_VULKAN(result);

		// Create desc pool
		VkDescriptorPoolSize bufferPoolSize;
		bufferPoolSize.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		bufferPoolSize.descriptorCount = 31;

		VkDescriptorPoolSize uniformPoolSize;
		uniformPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		uniformPoolSize.descriptorCount = 1;

		std::vector<VkDescriptorPoolSize> poolSizes = { bufferPoolSize, uniformPoolSize };

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

	VkDescriptorSetLayout NeuralRadianceCache::GetDescSetLayout()
	{
		return m_DescSetLayout;
	}

	NeuralRadianceCache::NeuralRadianceCache(float learningRate, float weightDecay, float beta1) :
		m_ConfigData({ .learningRate = learningRate, .weightDecay = weightDecay, .beta1 = beta1 }),
		m_StatsData({ .mseLoss = 0.0f }),
		m_ConfigUniformBuffer(
			sizeof(ConfigData), 
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			{}),
		m_StatsBuffer(
			sizeof(StatsData),
			VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			{})
	{
		// Set config
		vk::Buffer stagingBuffer(
			sizeof(ConfigData),
			VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			{});
		stagingBuffer.SetData(sizeof(ConfigData), &m_ConfigData, 0, 0);
		vk::Buffer::Copy(&stagingBuffer, &m_ConfigUniformBuffer, sizeof(ConfigData));
		stagingBuffer.Destroy();

		// Set stats
		m_StatsBuffer.SetData(sizeof(StatsData), &m_StatsData, 0, 0);

		InitWeightBuffers();
		InitBiasBuffers();

		// Allocate desc set
		VkDescriptorSetAllocateInfo descSetAI;
		descSetAI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		descSetAI.pNext = nullptr;
		descSetAI.descriptorPool = m_DescPool;
		descSetAI.descriptorSetCount = 1;
		descSetAI.pSetLayouts = &m_DescSetLayout;

		VkResult result = vkAllocateDescriptorSets(VulkanAPI::GetDevice(), &descSetAI, &m_DescSet);
		ASSERT_VULKAN(result);

		// Sizes
		std::array<size_t, 6> weightsSizes = {
			34 * 64 * sizeof(float), // 32 mrhe + 2 dir
			64 * 64 * sizeof(float),
			64 * 64 * sizeof(float),
			64 * 64 * sizeof(float),
			64 * 64 * sizeof(float),
			64 * 3 * sizeof(float) };

		std::array<size_t, 6> biasesSizes = {
			64 * sizeof(float), // 32 mrhe + 2 dir
			64 * sizeof(float),
			64 * sizeof(float),
			64 * sizeof(float),
			64 * sizeof(float),
			3 * sizeof(float) };

		// Update descriptor set
		// Set buffer infos
		std::vector<VkDescriptorBufferInfo> bufferInfos(30);
		for (size_t i = 0; i < m_Weights.size(); i++)
		{
			bufferInfos[i].buffer = m_Weights[i]->GetVulkanHandle();
			bufferInfos[i].offset = 0;
			bufferInfos[i].range = weightsSizes[i];

			bufferInfos[i + 6].buffer = m_DeltaWeights[i]->GetVulkanHandle();
			bufferInfos[i + 6].offset = 0;
			bufferInfos[i + 6].range = weightsSizes[i];

			bufferInfos[i + 12].buffer = m_Momentum1Weights[i]->GetVulkanHandle();
			bufferInfos[i + 12].offset = 0;
			bufferInfos[i + 12].range = weightsSizes[i];

			bufferInfos[i + 18].buffer = m_Biases[i]->GetVulkanHandle();
			bufferInfos[i + 18].offset = 0;
			bufferInfos[i + 18].range = biasesSizes[i];

			bufferInfos[i + 24].buffer = m_DeltaBiases[i]->GetVulkanHandle();
			bufferInfos[i + 24].offset = 0;
			bufferInfos[i + 24].range = biasesSizes[i];
		}

		// Set writes
		std::vector<VkWriteDescriptorSet> writes(24);
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

		VkDescriptorBufferInfo configBufferInfo;
		configBufferInfo.buffer = m_ConfigUniformBuffer.GetVulkanHandle();
		configBufferInfo.offset = 0;
		configBufferInfo.range = sizeof(ConfigData);

		VkWriteDescriptorSet configWrite;
		configWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		configWrite.pNext = nullptr;
		configWrite.dstSet = m_DescSet;
		configWrite.dstBinding = 30;
		configWrite.dstArrayElement = 0;
		configWrite.descriptorCount = 1;
		configWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		configWrite.pImageInfo = nullptr;
		configWrite.pBufferInfo = &configBufferInfo;
		configWrite.pTexelBufferView = nullptr;

		VkDescriptorBufferInfo statsBufferInfo;
		statsBufferInfo.buffer = m_StatsBuffer.GetVulkanHandle();
		statsBufferInfo.offset = 0;
		statsBufferInfo.range = sizeof(StatsData);

		VkWriteDescriptorSet statsWrite;
		statsWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		statsWrite.pNext = nullptr;
		statsWrite.dstSet = m_DescSet;
		statsWrite.dstBinding = 31;
		statsWrite.dstArrayElement = 0;
		statsWrite.descriptorCount = 1;
		statsWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		statsWrite.pImageInfo = nullptr;
		statsWrite.pBufferInfo = &statsBufferInfo;
		statsWrite.pTexelBufferView = nullptr;

		writes.push_back(configWrite);
		writes.push_back(statsWrite);

		vkUpdateDescriptorSets(VulkanAPI::GetDevice(), writes.size(), writes.data(), 0, nullptr);
	}

	void NeuralRadianceCache::Destroy()
	{
		for (size_t i = 0; i < m_Weights.size(); i++)
		{
			m_Weights[i]->Destroy();
			delete m_Weights[i];

			m_DeltaWeights[i]->Destroy();
			delete m_DeltaWeights[i];

			m_Momentum1Weights[i]->Destroy();
			delete m_Momentum1Weights[i];

			m_Biases[i]->Destroy();
			delete m_Biases[i];

			m_DeltaBiases[i]->Destroy();
			delete m_DeltaBiases[i];
		}

		m_ConfigUniformBuffer.Destroy();
		m_StatsBuffer.Destroy();
	}

	void NeuralRadianceCache::ResetStats()
	{
		m_StatsData.mseLoss = 0.0f;
		m_StatsBuffer.SetData(sizeof(StatsData), &m_StatsData, 0, 0);
	}

	VkDescriptorSet NeuralRadianceCache::GetDescSet() const
	{
		return m_DescSet;
	}

	const NeuralRadianceCache::StatsData& NeuralRadianceCache::GetStats()
	{
		m_StatsBuffer.GetData(sizeof(StatsData), &m_StatsData, 0, 0);
		return m_StatsData;
	}

	void NeuralRadianceCache::PrintWeights() const
	{
		std::array<size_t, 6> sizes = {
			34 * 64 * sizeof(float),
			64 * 64 * sizeof(float),
			64 * 64 * sizeof(float),
			64 * 64 * sizeof(float),
			64 * 64 * sizeof(float),
			64 * 3 * sizeof(float) };

		for (size_t i = 0; i < m_Weights.size(); i++)
		{
			vk::Buffer stagingBuffer(
				sizes[i], 
				VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, 
				VK_BUFFER_USAGE_TRANSFER_DST_BIT, 
				{});

			vk::Buffer::Copy(m_Weights[i], &stagingBuffer, sizes[i]);

			float* data = reinterpret_cast<float*>(malloc(sizes[i]));

			stagingBuffer.GetData(sizes[i], data, 0, 0);

			std::string str = "Weights " + std::to_string(i) + ": [";
			for (size_t weight = 0; weight < (sizes[i] / sizeof(float)); weight++)
			{
				str += std::to_string(data[weight]) + ", ";
			}
			str += "]";
			Log::Info(str);

			stagingBuffer.Destroy();

			free(data);
		}
	}

	void NeuralRadianceCache::InitWeightBuffers()
	{
		// Init weight sizes
		std::array<size_t, 6> sizes = {
			34 * 64 * sizeof(float), // 32 mrhe + 2 dir
			64 * 64 * sizeof(float),
			64 * 64 * sizeof(float),
			64 * 64 * sizeof(float),
			64 * 64 * sizeof(float),
			64 * 3 * sizeof(float) };

		// Init weights and delta weights buffer
		for (size_t i = 0; i < m_Weights.size(); i++)
		{
			m_Weights[i] = new vk::Buffer(
				sizes[i],
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
				{});

			m_DeltaWeights[i] = new vk::Buffer(
				sizes[i],
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
				{});

			m_Momentum1Weights[i] = new vk::Buffer(sizes[i],
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
				{});
		}

		// Set weights and delta weights
		std::default_random_engine generator((std::random_device()()));
		std::normal_distribution<float> distribution(0.0f, 1.0);

		for (size_t i = 0; i < m_Weights.size(); i++)
		{
			vk::Buffer stagingBuffer = vk::Buffer(
				sizes[i],
				VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
				VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
				{});

			float* data = reinterpret_cast<float*>(malloc(sizes[i]));

			// Weights
			for (size_t weight = 0; weight < (sizes[i] / sizeof(float)); weight++)
			{
				data[weight] = distribution(generator) * 0.01f;
			}

			stagingBuffer.SetData(sizes[i], data, 0, 0);
			vk::Buffer::Copy(&stagingBuffer, m_Weights[i], sizes[i]);

			// Delta weights and momentum 1 weights
			for (size_t weight = 0; weight < (sizes[i] / sizeof(float)); weight++)
			{
				data[weight] = 0.0;
			}

			stagingBuffer.SetData(sizes[i], data, 0, 0);
			vk::Buffer::Copy(&stagingBuffer, m_DeltaWeights[i], sizes[i]);
			vk::Buffer::Copy(&stagingBuffer, m_Momentum1Weights[i], sizes[i]);

			free(data);
			stagingBuffer.Destroy();
		}
	}

	void NeuralRadianceCache::InitBiasBuffers()
	{
		// Init biases sizes
		std::array<size_t, 6> biasesSizes = {
			64 * sizeof(float), // 32 mrhe + 2 dir
			64 * sizeof(float),
			64 * sizeof(float),
			64 * sizeof(float),
			64 * sizeof(float),
			3 * sizeof(float) };

		// Init biases and delta biases buffer
		for (size_t i = 0; i < m_Biases.size(); i++)
		{
			m_Biases[i] = new vk::Buffer(
				biasesSizes[i],
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
				{});

			m_DeltaBiases[i] = new vk::Buffer(
				biasesSizes[i],
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
				{});
		}

		// TODO: init biases
	}
}
