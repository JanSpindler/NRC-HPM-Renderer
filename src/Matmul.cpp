#include <engine/compute/Matmul.hpp>
#include <engine/util/Log.hpp>

namespace en::vk
{
	VkDescriptorSetLayout Matmul::m_DescriptorSetLayout;
	VkDescriptorPool Matmul::m_DescriptorPool;
	VkDescriptorSet Matmul::m_DescriptorSet;

	Buffer* Matmul::m_ConfigUniformBuffer;

	Shader Matmul::m_Shader;

	void Matmul::Init(VkDevice device)
	{
		CreateDescriptorSetLayout(device);
		CreateDescriptorPool(device);
		AllocateDescriptorSet(device);

		m_ConfigUniformBuffer = new Buffer(
			sizeof(MatmulConfigUniformData),
			VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			{});
	}

	void Matmul::Shutdown(VkDevice device)
	{
		m_ConfigUniformBuffer->Destroy();
		delete m_ConfigUniformBuffer;

		vkDestroyDescriptorPool(device, m_DescriptorPool, nullptr);
		vkDestroyDescriptorSetLayout(device, m_DescriptorSetLayout, nullptr);
	}

	void Matmul::Execute(Matrix* matA, Matrix* matB, Matrix* matC)
	{
		// Check size
		if (matC->GetRowCount() != matA->GetRowCount() || matC->GetColCount() != matB->GetColCount())
			Log::Error("Matrix sizes not fit for Matmul::Execute()", true);

		// Copy data to device
		matA->CopyToDevice();
		matB->CopyToDevice();

		// Set config
		MatmulConfigUniformData configData = {
			.aRowCount = matA->GetRowCount(),
			.aColCount = matA->GetColCount(),
			.bRowCount = matB->GetRowCount(),
			.bColCount = matB->GetColCount() };

		// Update descriptor set

		// Copy data to host
		matC->CopyToHost();
	}

	void Matmul::CreateDescriptorSetLayout(VkDevice device)
	{
		VkDescriptorSetLayoutBinding configBinding;
		configBinding.binding = 0;
		configBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		configBinding.descriptorCount = 1;
		configBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
		configBinding.pImmutableSamplers = nullptr;

		VkDescriptorSetLayoutBinding matABinding;
		matABinding.binding = 1;
		matABinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		matABinding.descriptorCount = 1;
		matABinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
		matABinding.pImmutableSamplers = nullptr;

		VkDescriptorSetLayoutBinding matBBinding;
		matBBinding.binding = 2;
		matBBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		matBBinding.descriptorCount = 1;
		matBBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
		matBBinding.pImmutableSamplers = nullptr;

		VkDescriptorSetLayoutBinding matCBinding;
		matCBinding.binding = 3;
		matCBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		matCBinding.descriptorCount = 1;
		matCBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
		matCBinding.pImmutableSamplers = nullptr;

		std::vector<VkDescriptorSetLayoutBinding> bindings = { configBinding, matABinding, matBBinding, matCBinding };

		VkDescriptorSetLayoutCreateInfo layoutCI;
		layoutCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutCI.pNext = nullptr;
		layoutCI.flags = 0;
		layoutCI.bindingCount = bindings.size();
		layoutCI.pBindings = bindings.data();

		VkResult result = vkCreateDescriptorSetLayout(device, &layoutCI, nullptr, &m_DescriptorSetLayout);
		ASSERT_VULKAN(result);
	}

	void Matmul::CreateDescriptorPool(VkDevice device)
	{
		VkDescriptorPoolSize configPoolSize;
		configPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		configPoolSize.descriptorCount = 1;

		VkDescriptorPoolSize matAPoolSize;
		matAPoolSize.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		matAPoolSize.descriptorCount = 1;

		VkDescriptorPoolSize matBPoolSize;
		matBPoolSize.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		matBPoolSize.descriptorCount = 1;

		VkDescriptorPoolSize matCPoolSize;
		matCPoolSize.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		matCPoolSize.descriptorCount = 1;

		std::vector<VkDescriptorPoolSize> poolSizes = { configPoolSize, matAPoolSize, matBPoolSize, matCPoolSize };

		VkDescriptorPoolCreateInfo poolCI;
		poolCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolCI.pNext = nullptr;
		poolCI.flags = 0;
		poolCI.maxSets = 1;
		poolCI.poolSizeCount = poolSizes.size();
		poolCI.pPoolSizes = poolSizes.data();

		VkResult result = vkCreateDescriptorPool(device, &poolCI, nullptr, &m_DescriptorPool);
		ASSERT_VULKAN(result);
	}

	void Matmul::AllocateDescriptorSet(VkDevice device)
	{
		VkDescriptorSetAllocateInfo descSetAI;
		descSetAI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		descSetAI.pNext = nullptr;
		descSetAI.descriptorPool = m_DescriptorPool;
		descSetAI.descriptorSetCount = 1;
		descSetAI.pSetLayouts = &m_DescriptorSetLayout;

		VkResult result = vkAllocateDescriptorSets(device, &descSetAI, &m_DescriptorSet);
		ASSERT_VULKAN(result);
	}
}
