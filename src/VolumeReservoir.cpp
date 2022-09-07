#include <engine/graphics/VolumeReservoir.hpp>

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

		std::vector<VkDescriptorSetLayoutBinding> bindings = { reservoirBufferBinding };

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
		storageBufferPoolSize.descriptorCount = 1;

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

	VolumeReservoir::VolumeReservoir()
	{
		InitReservoir();
	}

	void VolumeReservoir::Destroy()
	{
		m_ReservoirBuffer->Destroy();
		delete m_ReservoirBuffer;
	}

	VkDescriptorSet VolumeReservoir::GetDescriptorSet() const
	{
		return m_DescSet;
	}

	void VolumeReservoir::InitReservoir()
	{
		m_ReservoirBuffer = new vk::Buffer(
			1, 
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, 
			{});
	}
}
