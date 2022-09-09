#include <engine/graphics/VolumeReservoir.hpp>
#include <engine/util/Log.hpp>

namespace en
{
	VkDescriptorSetLayout VolumeReservoir::m_DescSetLayout;
	VkDescriptorPool VolumeReservoir::m_DescPool;

	void VolumeReservoir::Init(VkDevice device)
	{
		// Create descriptor set layout
		VkDescriptorSetLayoutBinding reservoirBufferBinding;
		reservoirBufferBinding.binding = 0;
		reservoirBufferBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		reservoirBufferBinding.descriptorCount = 1;
		reservoirBufferBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
		reservoirBufferBinding.pImmutableSamplers = nullptr;

		VkDescriptorSetLayoutBinding oldViewProjMatBufferBinding;
		oldViewProjMatBufferBinding.binding = 1;
		oldViewProjMatBufferBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		oldViewProjMatBufferBinding.descriptorCount = 1;
		oldViewProjMatBufferBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
		oldViewProjMatBufferBinding.pImmutableSamplers = nullptr;

		VkDescriptorSetLayoutBinding oldReservoirsBufferBinding;
		oldReservoirsBufferBinding.binding = 2;
		oldReservoirsBufferBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		oldReservoirsBufferBinding.descriptorCount = 1;
		oldReservoirsBufferBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
		oldReservoirsBufferBinding.pImmutableSamplers = nullptr;

		std::vector<VkDescriptorSetLayoutBinding> bindings = { 
			reservoirBufferBinding, 
			oldViewProjMatBufferBinding, 
			oldReservoirsBufferBinding };

		VkDescriptorSetLayoutCreateInfo layoutCI;
		layoutCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutCI.pNext = nullptr;
		layoutCI.flags = 0;
		layoutCI.bindingCount = bindings.size();
		layoutCI.pBindings = bindings.data();

		VkResult result = vkCreateDescriptorSetLayout(device, &layoutCI, nullptr, &m_DescSetLayout);
		ASSERT_VULKAN(result);

		// Create descriptor pool
		VkDescriptorPoolSize storageBufferPoolSize;
		storageBufferPoolSize.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		storageBufferPoolSize.descriptorCount = 3;

		std::vector<VkDescriptorPoolSize> poolSizes = { storageBufferPoolSize };

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

	void VolumeReservoir::Shutdown(VkDevice device)
	{
		vkDestroyDescriptorPool(device, m_DescPool, nullptr);
		vkDestroyDescriptorSetLayout(device, m_DescSetLayout, nullptr);
	}

	VkDescriptorSetLayout VolumeReservoir::GetDescriptorSetLayout()
	{
		return m_DescSetLayout;
	}

	VolumeReservoir::VolumeReservoir(
		uint32_t pathVertexCount, 
		uint32_t spatialKernelSize,
		uint32_t temporalKernelSize) 
		:
		m_PathVertexCount(pathVertexCount),
		m_SpatialKernelSize(spatialKernelSize),
		m_TemporalKernelSize(temporalKernelSize)
	{
		if (m_SpatialKernelSize % 2 != 1)
		{
			Log::Error("VolumeReservoir spacial kernel size must be an odd number", true);
		}
	}

	void VolumeReservoir::Init(uint32_t pixelCount)
	{
		m_ReservoirBufferSize = static_cast<size_t>(pixelCount * m_PathVertexCount) * 3 * sizeof(float);
		m_OldViewProjMatBufferSize = std::max(1ull, m_TemporalKernelSize * 16 * sizeof(float));
		m_OldReservoirsBufferSize = std::max(1ull, m_ReservoirBufferSize * m_TemporalKernelSize);

		InitBuffers();
		AllocateAndUpdateDescriptorSet(VulkanAPI::GetDevice());
	}

	void VolumeReservoir::Destroy()
	{
		m_OldReservoirsBuffer->Destroy();
		delete m_OldReservoirsBuffer;

		m_OldViewProjMatBuffer->Destroy();
		delete m_OldViewProjMatBuffer;

		m_ReservoirBuffer->Destroy();
		delete m_ReservoirBuffer;
	}

	uint32_t VolumeReservoir::GetPathVertexCount() const
	{
		return m_PathVertexCount;
	}

	uint32_t VolumeReservoir::GetSpatialKernelSize() const
	{
		return m_SpatialKernelSize;
	}

	uint32_t VolumeReservoir::GetTemporalKernelSize() const
	{
		return m_TemporalKernelSize;
	}

	VkDescriptorSet VolumeReservoir::GetDescriptorSet() const
	{
		return m_DescSet;
	}

	void VolumeReservoir::InitBuffers()
	{
		m_ReservoirBuffer = new vk::Buffer(
			m_ReservoirBufferSize,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			{});

		m_OldViewProjMatBuffer = new vk::Buffer(
			m_OldViewProjMatBufferSize,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			{});

		m_OldReservoirsBuffer = new vk::Buffer(
			m_OldReservoirsBufferSize,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			{});
	}

	void VolumeReservoir::AllocateAndUpdateDescriptorSet(VkDevice device)
	{
		// Allocate descriptor set
		VkDescriptorSetAllocateInfo descSetAI;
		descSetAI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		descSetAI.pNext = nullptr;
		descSetAI.descriptorPool = m_DescPool;
		descSetAI.descriptorSetCount = 1;
		descSetAI.pSetLayouts = &m_DescSetLayout;

		VkResult result = vkAllocateDescriptorSets(device, &descSetAI, &m_DescSet);
		ASSERT_VULKAN(result);

		// Reservoir buffer
		VkDescriptorBufferInfo reservoirBufferInfo;
		reservoirBufferInfo.buffer = m_ReservoirBuffer->GetVulkanHandle();
		reservoirBufferInfo.offset = 0;
		reservoirBufferInfo.range = m_ReservoirBufferSize;

		VkWriteDescriptorSet reservoirBufferWrite;
		reservoirBufferWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		reservoirBufferWrite.pNext = nullptr;
		reservoirBufferWrite.dstSet = m_DescSet;
		reservoirBufferWrite.dstBinding = 0;
		reservoirBufferWrite.dstArrayElement = 0;
		reservoirBufferWrite.descriptorCount = 1;
		reservoirBufferWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		reservoirBufferWrite.pImageInfo = nullptr;
		reservoirBufferWrite.pBufferInfo = &reservoirBufferInfo;
		reservoirBufferWrite.pTexelBufferView = nullptr;

		// Old view proj mat buffer
		VkDescriptorBufferInfo oldViewProjMatBufferInfo;
		oldViewProjMatBufferInfo.buffer = m_OldViewProjMatBuffer->GetVulkanHandle();
		oldViewProjMatBufferInfo.offset = 0;
		oldViewProjMatBufferInfo.range = m_OldViewProjMatBufferSize;

		VkWriteDescriptorSet oldViewProjMatBufferWrite;
		oldViewProjMatBufferWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		oldViewProjMatBufferWrite.pNext = nullptr;
		oldViewProjMatBufferWrite.dstSet = m_DescSet;
		oldViewProjMatBufferWrite.dstBinding = 1;
		oldViewProjMatBufferWrite.dstArrayElement = 0;
		oldViewProjMatBufferWrite.descriptorCount = 1;
		oldViewProjMatBufferWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		oldViewProjMatBufferWrite.pImageInfo = nullptr;
		oldViewProjMatBufferWrite.pBufferInfo = &oldViewProjMatBufferInfo;
		oldViewProjMatBufferWrite.pTexelBufferView = nullptr;

		// Old reservoirs buffer
		VkDescriptorBufferInfo oldReservoirsBufferInfo;
		oldReservoirsBufferInfo.buffer = m_OldReservoirsBuffer->GetVulkanHandle();
		oldReservoirsBufferInfo.offset = 0;
		oldReservoirsBufferInfo.range = m_OldReservoirsBufferSize;

		VkWriteDescriptorSet oldReservoirsBufferWrite;
		oldReservoirsBufferWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		oldReservoirsBufferWrite.pNext = nullptr;
		oldReservoirsBufferWrite.dstSet = m_DescSet;
		oldReservoirsBufferWrite.dstBinding = 2;
		oldReservoirsBufferWrite.dstArrayElement = 0;
		oldReservoirsBufferWrite.descriptorCount = 1;
		oldReservoirsBufferWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		oldReservoirsBufferWrite.pImageInfo = nullptr;
		oldReservoirsBufferWrite.pBufferInfo = &oldReservoirsBufferInfo;
		oldReservoirsBufferWrite.pTexelBufferView = nullptr;

		// Update descriptor set
		std::vector<VkWriteDescriptorSet> writes = { 
			reservoirBufferWrite, 
			oldViewProjMatBufferWrite, 
			oldReservoirsBufferWrite };

		vkUpdateDescriptorSets(device, writes.size(), writes.data(), 0, nullptr);
	}
}
