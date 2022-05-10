#include <engine/compute/Matmul.hpp>

namespace en::vk
{
	VkDescriptorSetLayout Matmul::m_DescriptorSetLayout;
	VkDescriptorPool Matmul::m_DescriptorPool;
	VkDescriptorSet Matmul::m_DescriptorSet;

	Shader Matmul::m_Shader;

	void Matmul::Init(VkDevice device)
	{
		// Create descriptor set layout
		VkDescriptorSetLayoutBinding matABinding;
		matABinding.binding = 0;
		matABinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		matABinding.descriptorCount = 1;
		matABinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
		matABinding.pImmutableSamplers = nullptr;

		VkDescriptorSetLayoutBinding matBBinding;
		matBBinding.binding = 1;
		matBBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		matBBinding.descriptorCount = 1;
		matBBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
		matBBinding.pImmutableSamplers = nullptr;

		VkDescriptorSetLayoutBinding matCBinding;
		matCBinding.binding = 2;
		matCBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		matCBinding.descriptorCount = 1;
		matCBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
		matCBinding.pImmutableSamplers = nullptr;

		std::vector<VkDescriptorSetLayoutBinding> bindings = { matABinding, matBBinding, matCBinding };

		VkDescriptorSetLayoutCreateInfo layoutCI;
		layoutCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutCI.pNext = nullptr;
		layoutCI.flags = 0;
		layoutCI.bindingCount = bindings.size();
		layoutCI.pBindings = bindings.data();

		VkResult result = vkCreateDescriptorSetLayout(device, &layoutCI, nullptr, &m_DescriptorSetLayout);
		ASSERT_VULKAN(result);

		// Create descriptor pool
		VkDescriptorPoolSize matAPoolSize;
		matAPoolSize.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		matAPoolSize.descriptorCount = 1;

		VkDescriptorPoolSize matBPoolSize;
		matBPoolSize.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		matBPoolSize.descriptorCount = 1;

		VkDescriptorPoolSize matCPoolSize;
		matCPoolSize.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		matCPoolSize.descriptorCount = 1;

		std::vector<VkDescriptorPoolSize> poolSizes = { matAPoolSize, matBPoolSize, matCPoolSize };

		VkDescriptorPoolCreateInfo poolCI;
		poolCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolCI.pNext = nullptr;
		poolCI.flags = 0;
		poolCI.maxSets = 1;
		poolCI.poolSizeCount = poolSizes.size();
		poolCI.pPoolSizes = poolSizes.data();

		result = vkCreateDescriptorPool(device, &poolCI, nullptr, &m_DescriptorPool);
		ASSERT_VULKAN(result);
	
		// Allocate descriptor set
		VkDescriptorSetAllocateInfo descSetAI;
		descSetAI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		descSetAI.pNext = nullptr;
		descSetAI.descriptorPool = m_DescriptorPool;
		descSetAI.descriptorSetCount = 1;
		descSetAI.pSetLayouts = &m_DescriptorSetLayout;

		result = vkAllocateDescriptorSets(device, &descSetAI, &m_DescriptorSet);
		ASSERT_VULKAN(result);
	}

	void Matmul::Shutdown(VkDevice device)
	{
		vkDestroyDescriptorPool(device, m_DescriptorPool, nullptr);
		vkDestroyDescriptorSetLayout(device, m_DescriptorSetLayout, nullptr);
	}

	void Matmul::Execute(const Matrix* matA, const Matrix* matB)
	{
		// Update descriptor set
	}
}
