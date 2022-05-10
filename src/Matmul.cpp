#include <engine/compute/Matmul.hpp>
#include <engine/util/Log.hpp>
#include <engine/graphics/vulkan/CommandPool.hpp>

namespace en::vk
{
	VkDescriptorSetLayout Matmul::m_DescriptorSetLayout;
	VkDescriptorPool Matmul::m_DescriptorPool;
	VkDescriptorSet Matmul::m_DescriptorSet;

	Buffer* Matmul::m_ConfigUniformBuffer;

	Shader Matmul::m_Shader;
	VkPipelineLayout Matmul::m_PipelineLayout;
	VkPipeline Matmul::m_Pipeline;

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

		m_Shader = Shader("matmul/matmul.comp", false);
		CreatePipelineLayout(device);
		CreatePipeline(device);
	}

	void Matmul::Shutdown(VkDevice device)
	{
		vkDestroyPipeline(device, m_Pipeline, nullptr);
		vkDestroyPipelineLayout(device, m_PipelineLayout, nullptr);
		m_Shader.Destroy();

		m_ConfigUniformBuffer->Destroy();
		delete m_ConfigUniformBuffer;

		vkDestroyDescriptorPool(device, m_DescriptorPool, nullptr);
		vkDestroyDescriptorSetLayout(device, m_DescriptorSetLayout, nullptr);
	}

	void Matmul::Execute(Matrix* matA, Matrix* matB, Matrix* matC)
	{
		// Check size
		if (matC->GetRowCount() != matA->GetRowCount() ||
			matC->GetColCount() != matB->GetColCount() ||
			matA->GetColCount() != matB->GetRowCount())
		{
			Log::Error("Matrix sizes not fit for Matmul::Execute()", true);
		}

		// Copy data to device
		matA->CopyToDevice();
		matB->CopyToDevice();

		// Set config
		MatmulConfigUniformData configData = {
			.aRowCount = matA->GetRowCount(),
			.aColCount = matA->GetColCount(),
			.bRowCount = matB->GetRowCount(),
			.bColCount = matB->GetColCount() };

		m_ConfigUniformBuffer->SetData(sizeof(MatmulConfigUniformData), &configData, 0, 0);

		// Update descriptor set
		UpdateDescriptorSet(matA, matB, matC);

		// Dispatch
		Dispatch(matC->GetRowCount(), matC->GetColCount());

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

	void Matmul::CreatePipelineLayout(VkDevice device)
	{
		VkPipelineLayoutCreateInfo pipelineLayoutCI;
		pipelineLayoutCI.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutCI.pNext = nullptr;
		pipelineLayoutCI.flags = 0;
		pipelineLayoutCI.setLayoutCount = 1;
		pipelineLayoutCI.pSetLayouts = &m_DescriptorSetLayout;
		pipelineLayoutCI.pushConstantRangeCount = 0;
		pipelineLayoutCI.pPushConstantRanges = nullptr;

		VkResult result = vkCreatePipelineLayout(device, &pipelineLayoutCI, nullptr, &m_PipelineLayout);
		ASSERT_VULKAN(result);
	}

	void Matmul::CreatePipeline(VkDevice device)
	{
		VkPipelineShaderStageCreateInfo shaderStageCI;
		shaderStageCI.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shaderStageCI.pNext = nullptr;
		shaderStageCI.flags = 0;
		shaderStageCI.stage = VK_SHADER_STAGE_COMPUTE_BIT;
		shaderStageCI.module = m_Shader.GetVulkanModule();
		shaderStageCI.pName = "main";
		shaderStageCI.pSpecializationInfo = nullptr;

		VkComputePipelineCreateInfo pipelineCI;
		pipelineCI.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
		pipelineCI.pNext = nullptr;
		pipelineCI.flags = 0;
		pipelineCI.stage = shaderStageCI;
		pipelineCI.layout = m_PipelineLayout;
		pipelineCI.basePipelineHandle = VK_NULL_HANDLE;
		pipelineCI.basePipelineIndex = 0;

		VkResult result = vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &pipelineCI, nullptr, &m_Pipeline);
		ASSERT_VULKAN(result);
	}

	void Matmul::UpdateDescriptorSet(const Matrix* matA, const Matrix* matB, const Matrix* matC)
	{
		VkDescriptorBufferInfo configBufferInfo;
		configBufferInfo.buffer = m_ConfigUniformBuffer->GetVulkanHandle();
		configBufferInfo.offset = 0;
		configBufferInfo.range = sizeof(MatmulConfigUniformData);

		VkWriteDescriptorSet configWrite;
		configWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		configWrite.pNext = nullptr;
		configWrite.dstSet = m_DescriptorSet;
		configWrite.dstBinding = 0;
		configWrite.dstArrayElement = 0;
		configWrite.descriptorCount = 1;
		configWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		configWrite.pImageInfo = nullptr;
		configWrite.pBufferInfo = &configBufferInfo;
		configWrite.pTexelBufferView = nullptr;

		VkDescriptorBufferInfo matABufferInfo;
		matABufferInfo.buffer = matA->GetBufferVulkanHandle();
		matABufferInfo.offset = 0;
		matABufferInfo.range = matA->GetMemorySize();

		VkWriteDescriptorSet matAWrite;
		matAWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		matAWrite.pNext = nullptr;
		matAWrite.dstSet = m_DescriptorSet;
		matAWrite.dstBinding = 1;
		matAWrite.dstArrayElement = 0;
		matAWrite.descriptorCount = 1;
		matAWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		matAWrite.pImageInfo = nullptr;
		matAWrite.pBufferInfo = &matABufferInfo;
		matAWrite.pTexelBufferView = nullptr;

		VkDescriptorBufferInfo matBBufferInfo;
		matBBufferInfo.buffer = matB->GetBufferVulkanHandle();
		matBBufferInfo.offset = 0;
		matBBufferInfo.range = matB->GetMemorySize();

		VkWriteDescriptorSet matBWrite;
		matBWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		matBWrite.pNext = nullptr;
		matBWrite.dstSet = m_DescriptorSet;
		matBWrite.dstBinding = 2;
		matBWrite.dstArrayElement = 0;
		matBWrite.descriptorCount = 1;
		matBWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		matBWrite.pImageInfo = nullptr;
		matBWrite.pBufferInfo = &matBBufferInfo;
		matBWrite.pTexelBufferView = nullptr;

		VkDescriptorBufferInfo matCBufferInfo;
		matCBufferInfo.buffer = matC->GetBufferVulkanHandle();
		matCBufferInfo.offset = 0;
		matCBufferInfo.range = matC->GetMemorySize();

		VkWriteDescriptorSet matCWrite;
		matCWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		matCWrite.pNext = nullptr;
		matCWrite.dstSet = m_DescriptorSet;
		matCWrite.dstBinding = 3;
		matCWrite.dstArrayElement = 0;
		matCWrite.descriptorCount = 1;
		matCWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		matCWrite.pImageInfo = nullptr;
		matCWrite.pBufferInfo = &matCBufferInfo;
		matCWrite.pTexelBufferView = nullptr;

		std::vector<VkWriteDescriptorSet> writes = { configWrite, matAWrite, matBWrite, matCWrite };

		vkUpdateDescriptorSets(VulkanAPI::GetDevice(), writes.size(), writes.data(), 0, nullptr);
	}

	void Matmul::Dispatch(uint32_t rowCount, uint32_t colCount)
	{
		// TODO: commandbuffer does not need to be rerecorded every time

		// Create command pool and buffer
		CommandPool commandPool(0, VulkanAPI::GetComputeQFI());
		commandPool.AllocateBuffers(1, VK_COMMAND_BUFFER_LEVEL_PRIMARY);
		VkCommandBuffer commandBuffer = commandPool.GetBuffer(0);

		// Begin command buffer
		VkCommandBufferBeginInfo beginInfo;
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.pNext = nullptr;
		beginInfo.flags = 0;
		beginInfo.pInheritanceInfo = nullptr;

		VkResult result = vkBeginCommandBuffer(commandBuffer, &beginInfo);
		ASSERT_VULKAN(result);

		// Bind pipeline
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_Pipeline);
		
		// Bind descriptor set
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_PipelineLayout, 0, 1, &m_DescriptorSet, 0, nullptr);

		// Dispatch
		vkCmdDispatch(commandBuffer, rowCount, colCount, 1);

		// End command buffer
		result = vkEndCommandBuffer(commandBuffer);
		ASSERT_VULKAN(result);

		// Submit
		VkSubmitInfo submitInfo;
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.pNext = nullptr;
		submitInfo.waitSemaphoreCount = 0;
		submitInfo.pWaitSemaphores = nullptr;
		submitInfo.pWaitDstStageMask = nullptr;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffer;
		submitInfo.signalSemaphoreCount = 0;
		submitInfo.pSignalSemaphores = nullptr;

		VkQueue queue = VulkanAPI::GetComputeQueue();

		result = vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
		ASSERT_VULKAN(result);

		result = vkQueueWaitIdle(queue);
		ASSERT_VULKAN(result);

		// Destroy
		commandPool.Destroy();
	}
}
