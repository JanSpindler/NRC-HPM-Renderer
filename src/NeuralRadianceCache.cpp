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
			deltaWeights5Binding };

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
		bufferPoolSize.descriptorCount = 12;

		std::vector<VkDescriptorPoolSize> poolSizes = { bufferPoolSize };

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

	NeuralRadianceCache::NeuralRadianceCache()
	{
		// Init sizes
		std::array<size_t, 6> sizes = { 
			5 * 64,
			64 * 64,
			64 * 64, 
			64 * 64, 
			64 * 64, 
			64 * 3 };

		// Init weights and delta weights buffer
		for (size_t i = 0; i < m_Weights.size(); i++)
		{
			m_Weights[i] = new vk::Buffer(
				sizes[i],
				VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, // TODO: device local
				VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
				{});

			m_DeltaWeights[i] = new vk::Buffer(
				sizes[i],
				VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
				VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
				{});
		}

		// Set weights and delta weights
		std::default_random_engine generator((std::random_device()()));
		std::normal_distribution<float> distribution(0.0f, 1.0);
		
		for (size_t i = 0; i < m_Weights.size(); i++)
		{
			float norm = static_cast<float>(sizes[i]);
			
			float* data = reinterpret_cast<float*>(malloc(sizeof(float) * sizes[i]));

			// Weights
			for (size_t weight = 0; weight < sizes[i]; weight++)
			{
				data[weight] = distribution(generator);// / norm;
			}

			m_Weights[i]->SetData(sizes[i], data, 0, 0);

			// Delta weights
			for (size_t weight = 0; weight < sizes[i]; weight++)
			{
				data[weight] = 0.0;
			}

			m_DeltaWeights[i]->SetData(sizes[i], data, 0, 0);

			free(data);
		}

		// Allocate desc set
		VkDescriptorSetAllocateInfo descSetAI;
		descSetAI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		descSetAI.pNext = nullptr;
		descSetAI.descriptorPool = m_DescPool;
		descSetAI.descriptorSetCount = 1;
		descSetAI.pSetLayouts = &m_DescSetLayout;

		VkResult result = vkAllocateDescriptorSets(VulkanAPI::GetDevice(), &descSetAI, &m_DescSet);
		ASSERT_VULKAN(result);

		// Update descriptor set
		// Set buffer infos
		std::array<VkDescriptorBufferInfo, 12> bufferInfos;
		for (size_t i = 0; i < m_Weights.size(); i++)
		{
			bufferInfos[i].buffer = m_Weights[i]->GetVulkanHandle();
			bufferInfos[i].offset = 0;
			bufferInfos[i].range = sizes[i];

			bufferInfos[i + m_Weights.size()].buffer = m_DeltaWeights[i]->GetVulkanHandle();
			bufferInfos[i + m_Weights.size()].offset = 0;
			bufferInfos[i + m_Weights.size()].range = sizes[i];
		}

		// Set writes
		std::array<VkWriteDescriptorSet, 12> writes;
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
		}
	}

	VkDescriptorSet NeuralRadianceCache::GetDescSet() const
	{
		return m_DescSet;
	}
}
