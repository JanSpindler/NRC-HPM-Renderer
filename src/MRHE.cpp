#include <engine/graphics/MRHE.hpp>

namespace en
{
	VkDescriptorSetLayout MRHE::m_DescSetLayout;
	VkDescriptorPool MRHE::m_DescPool;

	void MRHE::Init(VkDevice device)
	{
		// Create desc set layout
		VkDescriptorSetLayoutBinding uniformBinding;
		uniformBinding.binding = 0;
		uniformBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		uniformBinding.descriptorCount = 1;
		uniformBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
		uniformBinding.pImmutableSamplers = nullptr;

		VkDescriptorSetLayoutBinding hashTablesBinding;
		hashTablesBinding.binding = 1;
		hashTablesBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		hashTablesBinding.descriptorCount = 1;
		hashTablesBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
		hashTablesBinding.pImmutableSamplers = nullptr;

		std::vector<VkDescriptorSetLayoutBinding> bindings = { uniformBinding, hashTablesBinding };

		VkDescriptorSetLayoutCreateInfo layoutCI;
		layoutCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutCI.pNext = nullptr;
		layoutCI.flags = 0;
		layoutCI.bindingCount = bindings.size();
		layoutCI.pBindings = bindings.data();

		VkResult result = vkCreateDescriptorSetLayout(device, &layoutCI, nullptr, &m_DescSetLayout);
		ASSERT_VULKAN(result);

		// Create desc pool
		VkDescriptorPoolSize uniformPoolSize;
		uniformPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		uniformPoolSize.descriptorCount = 1;

		VkDescriptorPoolSize storagePoolSize;
		storagePoolSize.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		storagePoolSize.descriptorCount = 1;

		std::vector<VkDescriptorPoolSize> poolSizes = { uniformPoolSize, storagePoolSize };

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

	void MRHE::Shutdown(VkDevice device)
	{
		vkDestroyDescriptorPool(device, m_DescPool, nullptr);
		vkDestroyDescriptorSetLayout(device, m_DescSetLayout, nullptr);
	}

	VkDescriptorSetLayout MRHE::GetDescriptorSetLayout()
	{
		return m_DescSetLayout;
	}

	MRHE::MRHE() :
		m_UniformData({
			.levelCount = 16,
			.hashTableSize = 16384,
			.featureCount = 2,
			.minRes = 16,
			.maxRes = 512 }),
		m_UniformBuffer(
				sizeof(UniformData),
				VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
				VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
				{}),
		m_HashTablesSize(m_UniformData.levelCount* m_UniformData.hashTableSize* m_UniformData.featureCount * sizeof(float)),
		m_HashTablesBuffer(
			m_HashTablesSize,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, 
			{})
	{
		VkDevice device = VulkanAPI::GetDevice();

		// Push uniform data to buffer
		m_UniformBuffer.SetData(sizeof(UniformData), &m_UniformData, 0, 0);
	
		// Setup hash tables buffer

		// Allocate descriptor set
		VkDescriptorSetAllocateInfo descSetAI;
		descSetAI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		descSetAI.pNext = nullptr;
		descSetAI.descriptorPool = m_DescPool;
		descSetAI.descriptorSetCount = 1;
		descSetAI.pSetLayouts = &m_DescSetLayout;

		VkResult result = vkAllocateDescriptorSets(device, &descSetAI, &m_DescSet);
		ASSERT_VULKAN(result);

		// Write descriptor set
		VkDescriptorBufferInfo uniformBufferInfo;
		uniformBufferInfo.buffer = m_UniformBuffer.GetVulkanHandle();
		uniformBufferInfo.offset = 0;
		uniformBufferInfo.range = sizeof(m_UniformData);

		VkWriteDescriptorSet uniformWrite;
		uniformWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		uniformWrite.pNext = nullptr;
		uniformWrite.dstSet = m_DescSet;
		uniformWrite.dstBinding = 0;
		uniformWrite.dstArrayElement = 0;
		uniformWrite.descriptorCount = 1;
		uniformWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		uniformWrite.pImageInfo = nullptr;
		uniformWrite.pBufferInfo = &uniformBufferInfo;
		uniformWrite.pTexelBufferView = nullptr;

		VkDescriptorBufferInfo hashTablesBufferInfo;
		hashTablesBufferInfo.buffer = m_HashTablesBuffer.GetVulkanHandle();
		hashTablesBufferInfo.offset = 0;
		hashTablesBufferInfo.range = m_HashTablesSize;

		VkWriteDescriptorSet hashTablesWrite;
		hashTablesWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		hashTablesWrite.pNext = nullptr;
		hashTablesWrite.dstSet = m_DescSet;
		hashTablesWrite.dstBinding = 1;
		hashTablesWrite.dstArrayElement = 0;
		hashTablesWrite.descriptorCount = 1;
		hashTablesWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		hashTablesWrite.pImageInfo = nullptr;
		hashTablesWrite.pBufferInfo = &hashTablesBufferInfo;
		hashTablesWrite.pTexelBufferView = nullptr;

		std::vector<VkWriteDescriptorSet> writes = { uniformWrite, hashTablesWrite };

		vkUpdateDescriptorSets(device, writes.size(), writes.data(), 0, nullptr);
	}

	void MRHE::Destroy()
	{
		m_UniformBuffer.Destroy();
	}

	VkDescriptorSet MRHE::GetDescriptorSet() const
	{
		return m_DescSet;
	}
}
