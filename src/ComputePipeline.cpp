#include <engine/vulkan/ComputePipeline.hpp>
#include <engine/VulkanAPI.hpp>
#include <engine/vulkan/CommandPool.hpp>

namespace en::vk
{
    VkDescriptorSetLayout ComputeTask::m_DescriptorSetLayout;
    VkDescriptorPool ComputeTask::m_DescriptorPool;
	
	void ComputeTask::Init()
	{
        VkDevice device = VulkanAPI::GetDevice();

		// Create descriptor set layout
        VkDescriptorSetLayoutBinding inBinding;
        inBinding.binding = 0;
        inBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        inBinding.descriptorCount = 1;
        inBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
        inBinding.pImmutableSamplers = nullptr;

        VkDescriptorSetLayoutBinding outBinding;
        outBinding.binding = 1;
        outBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        outBinding.descriptorCount = 1;
        outBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
        outBinding.pImmutableSamplers = nullptr;

        std::vector<VkDescriptorSetLayoutBinding> bindings = { inBinding, outBinding };

        VkDescriptorSetLayoutCreateInfo layoutCreateInfo;
        layoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutCreateInfo.pNext = nullptr;
        layoutCreateInfo.flags = 0;
        layoutCreateInfo.bindingCount = bindings.size();
        layoutCreateInfo.pBindings = bindings.data();

        VkResult result = vkCreateDescriptorSetLayout(device, &layoutCreateInfo, nullptr, &m_DescriptorSetLayout);
        ASSERT_VULKAN(result);

		// Create descriptor pool
        VkDescriptorPoolSize storageBufferPoolSize;
        storageBufferPoolSize.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        storageBufferPoolSize.descriptorCount = 2;

        std::vector<VkDescriptorPoolSize> poolSizes = { storageBufferPoolSize };

        VkDescriptorPoolCreateInfo poolCreateInfo;
        poolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolCreateInfo.pNext = nullptr;
        poolCreateInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        poolCreateInfo.maxSets = MAX_COMPUTE_TASK_COUNT;
        poolCreateInfo.poolSizeCount = poolSizes.size();
        poolCreateInfo.pPoolSizes = poolSizes.data();

        result = vkCreateDescriptorPool(device, &poolCreateInfo, nullptr, &m_DescriptorPool);
        ASSERT_VULKAN(result);
	}

	void ComputeTask::Shutdown()
	{
        VkDevice device = VulkanAPI::GetDevice();
        vkDestroyDescriptorPool(device, m_DescriptorPool, nullptr);
        vkDestroyDescriptorSetLayout(device, m_DescriptorSetLayout, nullptr);
	}

    VkDescriptorSetLayout ComputeTask::GetDescriptorSetLayout()
    {
        return m_DescriptorSetLayout;
    }

    ComputeTask::ComputeTask(const vk::Buffer* inputBuffer, const vk::Buffer* outputBuffer, const glm::uvec3& groupCount) :
        m_InputBuffer(inputBuffer),
        m_OutputBuffer(outputBuffer),
        m_GroupCount(groupCount)
    {
        VkDevice device = VulkanAPI::GetDevice();

        // Allocate descriptor set
        VkDescriptorSetAllocateInfo allocateInfo;
        allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocateInfo.pNext = nullptr;
        allocateInfo.descriptorPool = m_DescriptorPool;
        allocateInfo.descriptorSetCount = 1;
        allocateInfo.pSetLayouts = &m_DescriptorSetLayout;

        VkResult result = vkAllocateDescriptorSets(device, &allocateInfo, &m_DescriptorSet);
        ASSERT_VULKAN(result);

        // Write descriptor set
        VkDescriptorBufferInfo inBufferInfo;
        inBufferInfo.buffer = m_InputBuffer->GetVulkanHandle();
        inBufferInfo.offset = 0;
        inBufferInfo.range = m_InputBuffer->GetUsedSize();

        VkWriteDescriptorSet inBufferWrite;
        inBufferWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        inBufferWrite.pNext = nullptr;
        inBufferWrite.dstSet = m_DescriptorSet;
        inBufferWrite.dstBinding = 0;
        inBufferWrite.dstArrayElement = 0;
        inBufferWrite.descriptorCount = 1;
        inBufferWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        inBufferWrite.pImageInfo = nullptr;
        inBufferWrite.pBufferInfo = &inBufferInfo;
        inBufferWrite.pTexelBufferView = nullptr;

        VkDescriptorBufferInfo outBufferInfo;
        outBufferInfo.buffer = m_OutputBuffer->GetVulkanHandle();
        outBufferInfo.offset = 0;
        outBufferInfo.range = m_OutputBuffer->GetUsedSize();

        VkWriteDescriptorSet outBufferWrite;
        outBufferWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        outBufferWrite.pNext = nullptr;
        outBufferWrite.dstSet = m_DescriptorSet;
        outBufferWrite.dstBinding = 1;
        outBufferWrite.dstArrayElement = 0;
        outBufferWrite.descriptorCount = 1;
        outBufferWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        outBufferWrite.pImageInfo = nullptr;
        outBufferWrite.pBufferInfo = &outBufferInfo;
        outBufferWrite.pTexelBufferView = nullptr;

        std::vector<VkWriteDescriptorSet> descWrites = { inBufferWrite, outBufferWrite };

        vkUpdateDescriptorSets(VulkanAPI::GetDevice(), descWrites.size(), descWrites.data(), 0, nullptr);
    }

    void ComputeTask::Destroy()
    {
        VkResult result = vkFreeDescriptorSets(VulkanAPI::GetDevice(), m_DescriptorPool, 1, &m_DescriptorSet);
        ASSERT_VULKAN(result);
    }

    const glm::uvec3& ComputeTask::GetGroupCount() const
    {
        return m_GroupCount;
    }

    VkDescriptorSet ComputeTask::GetDescriptorSet() const
    {
        return m_DescriptorSet;
    }

    VkPipelineLayout ComputePipeline::m_PipelineLayout;

    void ComputePipeline::Init()
    {
        ComputeTask::Init();

        VkDescriptorSetLayout descSetLayout = ComputeTask::GetDescriptorSetLayout();

        VkPipelineLayoutCreateInfo createInfo;
        createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        createInfo.pNext = nullptr;
        createInfo.flags = 0;
        createInfo.setLayoutCount = 1;
        createInfo.pSetLayouts = &descSetLayout;
        createInfo.pushConstantRangeCount = 0;
        createInfo.pPushConstantRanges = nullptr;

        VkResult result = vkCreatePipelineLayout(VulkanAPI::GetDevice(), &createInfo, nullptr, &m_PipelineLayout);
        ASSERT_VULKAN(result);
    }

    void ComputePipeline::Shutdown()
    {
        vkDestroyPipelineLayout(VulkanAPI::GetDevice(), m_PipelineLayout, nullptr);
        ComputeTask::Shutdown();
    }

    ComputePipeline::ComputePipeline(const vk::Shader* shader) :
        m_Shader(shader)
    {
        VkDevice device = VulkanAPI::GetDevice();

        // Create pipeline
        VkPipelineShaderStageCreateInfo shaderStageCreateInfo;
        shaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        shaderStageCreateInfo.pNext = nullptr;
        shaderStageCreateInfo.flags = 0;
        shaderStageCreateInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
        shaderStageCreateInfo.module = m_Shader->GetVulkanModule();
        shaderStageCreateInfo.pName = "main";
        shaderStageCreateInfo.pSpecializationInfo = nullptr;

        VkComputePipelineCreateInfo createInfo;
        createInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
        createInfo.pNext = nullptr;
        createInfo.flags = 0;
        createInfo.stage = shaderStageCreateInfo;
        createInfo.layout = m_PipelineLayout;
        createInfo.basePipelineHandle = VK_NULL_HANDLE;
        createInfo.basePipelineIndex = 0;

        VkResult result = vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &createInfo, nullptr, &m_Pipeline);
        ASSERT_VULKAN(result);
    }

    void ComputePipeline::Destroy()
    {
        vkDestroyPipeline(VulkanAPI::GetDevice(), m_Pipeline, nullptr);
    }

    void ComputePipeline::RecordDispatch(VkCommandBuffer commandBuffer, const ComputeTask* computeTask) const
    {
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_Pipeline);
        
        VkDescriptorSet descriptorSet = computeTask->GetDescriptorSet();
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_PipelineLayout, 0, 1, &descriptorSet, 0, nullptr);

        const glm::uvec3& groupCount = computeTask->GetGroupCount();
        vkCmdDispatch(commandBuffer, groupCount.x, groupCount.y, groupCount.z);
    }

    void ComputePipeline::Execute(VkQueue queue, uint32_t qfi, const ComputeTask* computeTask) const
    {
        // Create commandPool and allocate commandBuffer
        vk::CommandPool commandPool(0, VulkanAPI::GetGraphicsQFI()); // TODO: get correct queue + make member of NoiseGenerator
        commandPool.AllocateBuffers(1, VK_COMMAND_BUFFER_LEVEL_PRIMARY);
        VkCommandBuffer commandBuffer = commandPool.GetBuffer(0);

        // Begin commandBuffer
        VkCommandBufferBeginInfo beginInfo;
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.pNext = nullptr;
        beginInfo.flags = 0;
        beginInfo.pInheritanceInfo = nullptr;

        VkResult result = vkBeginCommandBuffer(commandBuffer, &beginInfo);
        ASSERT_VULKAN(result);

        // Execute pipeline
        RecordDispatch(commandBuffer, computeTask);

        // End commandBuffer
        result = vkEndCommandBuffer(commandBuffer);
        ASSERT_VULKAN(result);

        // Submit to queue
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

        result = vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
        ASSERT_VULKAN(result);

        // Implement optional waiting later
        result = vkQueueWaitIdle(queue);
        ASSERT_VULKAN(result);

        // End
        commandPool.Destroy();
    }
}
