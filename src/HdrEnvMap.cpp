#include <engine/graphics/HdrEnvMap.hpp>
#include <engine/util/read_file.hpp>
#include <engine/graphics/VulkanAPI.hpp>
#include <engine/graphics/vulkan/Buffer.hpp>
#include <engine/graphics/vulkan/CommandRecorder.hpp>
#include <engine/graphics/vulkan/CommandPool.hpp>
#include <imgui.h>

namespace en
{
	VkDescriptorSetLayout HdrEnvMap::m_DescSetLayout;
	VkDescriptorPool HdrEnvMap::m_DescPool;

	void HdrEnvMap::Init(VkDevice device)
	{
		// Create descriptor set layout
		VkDescriptorSetLayoutBinding hdrTexBinding;
		hdrTexBinding.binding = 0;
		hdrTexBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		hdrTexBinding.descriptorCount = 1;
		hdrTexBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
		hdrTexBinding.pImmutableSamplers = nullptr;

		VkDescriptorSetLayoutBinding uniformBinding;
		uniformBinding.binding = 1;
		uniformBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		uniformBinding.descriptorCount = 1;
		uniformBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT;
		uniformBinding.pImmutableSamplers = nullptr;

		std::vector<VkDescriptorSetLayoutBinding> bindings = { hdrTexBinding, uniformBinding };

		VkDescriptorSetLayoutCreateInfo layoutCI;
		layoutCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutCI.pNext = nullptr;
		layoutCI.flags = 0;
		layoutCI.bindingCount = bindings.size();
		layoutCI.pBindings = bindings.data();

		VkResult result = vkCreateDescriptorSetLayout(device, &layoutCI, nullptr, &m_DescSetLayout);
		ASSERT_VULKAN(result);

		// Create descriptor pool
		VkDescriptorPoolSize imagePoolSize;
		imagePoolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		imagePoolSize.descriptorCount = 1;

		VkDescriptorPoolSize uniformPoolSize;
		uniformPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		uniformPoolSize.descriptorCount = 1;

		std::vector<VkDescriptorPoolSize> poolSizes = { imagePoolSize, uniformPoolSize };

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

	void HdrEnvMap::Shutdown(VkDevice device)
	{
		vkDestroyDescriptorPool(device, m_DescPool, nullptr);
		vkDestroyDescriptorSetLayout(device, m_DescSetLayout, nullptr);
	}

	VkDescriptorSetLayout HdrEnvMap::GetDescriptorSetLayout()
	{
		return m_DescSetLayout;
	}

	HdrEnvMap::HdrEnvMap(const std::vector<float>& hdr4f, uint32_t width, uint32_t height) :
		m_Width(width),
		m_Height(height),
		m_RawSize(width * height * 4 * sizeof(float)),
		m_ImageLayout(VK_IMAGE_LAYOUT_PREINITIALIZED),
		m_UniformData({ .directStrength = 1.0f, .hpmStrength = 1.0f }),
		m_UniformBuffer(
			sizeof(UniformData), 
			VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, 
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, 
			{})
	{
		VkDevice device = VulkanAPI::GetDevice();
		VkQueue queue = VulkanAPI::GetGraphicsQueue();
		
		// Uniform
		m_UniformBuffer.SetData(sizeof(UniformData), &m_UniformData, 0, 0);

		// Staging Buffer
		vk::Buffer stagingBuffer(
			m_RawSize,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			{});

		stagingBuffer.SetData(m_RawSize, hdr4f.data(), 0, 0);

		// Create Image
		VkFormat format = VK_FORMAT_R32G32B32A32_SFLOAT;

		VkImageCreateInfo imageCreateInfo;
		imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageCreateInfo.pNext = nullptr;
		imageCreateInfo.flags = 0;
		imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
		imageCreateInfo.format = format;
		imageCreateInfo.extent = { m_Width, m_Height, 1 };
		imageCreateInfo.mipLevels = 1;
		imageCreateInfo.arrayLayers = 1;
		imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		imageCreateInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		imageCreateInfo.queueFamilyIndexCount = 0;
		imageCreateInfo.pQueueFamilyIndices = nullptr;
		imageCreateInfo.initialLayout = m_ImageLayout;

		VkResult result = vkCreateImage(device, &imageCreateInfo, nullptr, &m_Image);
		ASSERT_VULKAN(result);

		// Image Memory
		VkMemoryRequirements memoryRequirements;
		vkGetImageMemoryRequirements(device, m_Image, &memoryRequirements);

		VkMemoryAllocateInfo allocateInfo;
		allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocateInfo.pNext = nullptr;
		allocateInfo.allocationSize = memoryRequirements.size;
		allocateInfo.memoryTypeIndex = VulkanAPI::FindMemoryType(
			memoryRequirements.memoryTypeBits,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		result = vkAllocateMemory(device, &allocateInfo, nullptr, &m_DeviceMemory);
		ASSERT_VULKAN(result);

		result = vkBindImageMemory(device, m_Image, m_DeviceMemory, 0);
		ASSERT_VULKAN(result);

		// Transfer data
		vk::CommandPool commandPool = vk::CommandPool(VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, VulkanAPI::GetGraphicsQFI());
		commandPool.AllocateBuffers(1, VK_COMMAND_BUFFER_LEVEL_PRIMARY);
		VkCommandBuffer commandBuffer = commandPool.GetBuffer(0);

		ChangeLayout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, commandBuffer, queue);
		WriteBufferToImage(commandBuffer, queue, stagingBuffer.GetVulkanHandle());
		ChangeLayout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, commandBuffer, queue);

		stagingBuffer.Destroy();
		commandPool.Destroy();

		// Create ImageView
		VkImageViewCreateInfo imageViewCreateInfo;
		imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		imageViewCreateInfo.pNext = nullptr;
		imageViewCreateInfo.flags = 0;
		imageViewCreateInfo.image = m_Image;
		imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		imageViewCreateInfo.format = format;
		imageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		imageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		imageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		imageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
		imageViewCreateInfo.subresourceRange.levelCount = 1;
		imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
		imageViewCreateInfo.subresourceRange.layerCount = 1;

		result = vkCreateImageView(device, &imageViewCreateInfo, nullptr, &m_ImageView);
		ASSERT_VULKAN(result);

		// Create Sampler
		VkFilter filter = VK_FILTER_LINEAR;
		VkSamplerAddressMode addressMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;

		VkSamplerCreateInfo samplerCreateInfo;
		samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerCreateInfo.pNext = nullptr;
		samplerCreateInfo.flags = 0;
		samplerCreateInfo.magFilter = filter;
		samplerCreateInfo.minFilter = filter;
		samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		samplerCreateInfo.addressModeU = addressMode;
		samplerCreateInfo.addressModeV = addressMode;
		samplerCreateInfo.addressModeW = addressMode;
		samplerCreateInfo.mipLodBias = 0.0f;
		samplerCreateInfo.anisotropyEnable = VK_FALSE;
		samplerCreateInfo.maxAnisotropy = 0.0f;
		samplerCreateInfo.compareEnable = VK_FALSE;
		samplerCreateInfo.compareOp = VK_COMPARE_OP_ALWAYS;
		samplerCreateInfo.minLod = 0.0f;
		samplerCreateInfo.maxLod = 0.0f;
		samplerCreateInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
		samplerCreateInfo.unnormalizedCoordinates = VK_FALSE;

		result = vkCreateSampler(device, &samplerCreateInfo, nullptr, &m_Sampler);
		ASSERT_VULKAN(result);

		// Allocate desc set
		VkDescriptorSetAllocateInfo descSetAI;
		descSetAI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		descSetAI.pNext = nullptr;
		descSetAI.descriptorPool = m_DescPool;
		descSetAI.descriptorSetCount = 1;
		descSetAI.pSetLayouts = &m_DescSetLayout;

		result = vkAllocateDescriptorSets(device, &descSetAI, &m_DescSet);
		ASSERT_VULKAN(result);

		// Upload to desc set
		VkDescriptorImageInfo hdrImageInfo;
		hdrImageInfo.sampler = m_Sampler;
		hdrImageInfo.imageView = m_ImageView;
		hdrImageInfo.imageLayout = m_ImageLayout;

		VkWriteDescriptorSet hdrTexWrite;
		hdrTexWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		hdrTexWrite.pNext = nullptr;
		hdrTexWrite.dstSet = m_DescSet;
		hdrTexWrite.dstBinding = 0;
		hdrTexWrite.dstArrayElement = 0;
		hdrTexWrite.descriptorCount = 1;
		hdrTexWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		hdrTexWrite.pImageInfo = &hdrImageInfo;
		hdrTexWrite.pBufferInfo = nullptr;
		hdrTexWrite.pTexelBufferView = nullptr;

		VkDescriptorBufferInfo uniformBufferInfo;
		uniformBufferInfo.buffer = m_UniformBuffer.GetVulkanHandle();
		uniformBufferInfo.offset = 0;
		uniformBufferInfo.range = sizeof(UniformData);

		VkWriteDescriptorSet uniformWrite;
		uniformWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		uniformWrite.pNext = nullptr;
		uniformWrite.dstSet = m_DescSet;
		uniformWrite.dstBinding = 1;
		uniformWrite.dstArrayElement = 0;
		uniformWrite.descriptorCount = 1;
		uniformWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		uniformWrite.pImageInfo = nullptr;
		uniformWrite.pBufferInfo = &uniformBufferInfo;
		uniformWrite.pTexelBufferView = nullptr;

		std::vector<VkWriteDescriptorSet> writes = { hdrTexWrite, uniformWrite };

		vkUpdateDescriptorSets(device, writes.size(), writes.data(), 0, nullptr);
	}

	void HdrEnvMap::Destroy()
	{
		VkDevice device = VulkanAPI::GetDevice();

		m_UniformBuffer.Destroy();

		vkDestroySampler(device, m_Sampler, nullptr);
		vkFreeMemory(device, m_DeviceMemory, nullptr);
		vkDestroyImageView(device, m_ImageView, nullptr);
		vkDestroyImage(device, m_Image, nullptr);
	}

	void HdrEnvMap::RenderImGui()
	{
		ImGui::Begin("Hdr Env Map");
		
		ImGui::DragFloat("Direct Strength", &m_UniformData.directStrength, 0.01f);
		ImGui::DragFloat("HPM Strength", &m_UniformData.hpmStrength, 0.01f);

		if (m_UniformData.directStrength < 0.0f)
		{
			m_UniformData.directStrength = 0.0f;
		}

		if (m_UniformData.hpmStrength < 0.0f)
		{
			m_UniformData.hpmStrength = 0.0f;
		}

		ImGui::End();

		m_UniformBuffer.SetData(sizeof(UniformData), &m_UniformData, 0, 0);
	}

	VkDescriptorSet HdrEnvMap::GetDescriptorSet() const
	{
		return m_DescSet;
	}

	void HdrEnvMap::ChangeLayout(VkImageLayout layout, VkCommandBuffer commandBuffer, VkQueue queue)
	{
		VkAccessFlags srcAccessMask;
		VkAccessFlags dstAccessMask;

		VkPipelineStageFlags srcStageFlags = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
		VkPipelineStageFlags dstStageFlags = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;

		if (m_ImageLayout == VK_IMAGE_LAYOUT_PREINITIALIZED && layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
		{
			srcAccessMask = VK_ACCESS_NONE_KHR;
			dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		}
		else if (m_ImageLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
		{
			srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		}
		else
		{
			Log::Error("Unknown image layout transision in Texture2D", true);
		}

		VkCommandBufferBeginInfo beginInfo;
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.pNext = nullptr;
		beginInfo.flags = 0;
		beginInfo.pInheritanceInfo = nullptr;

		VkResult result = vkBeginCommandBuffer(commandBuffer, &beginInfo);
		ASSERT_VULKAN(result);

		vk::CommandRecorder::ImageLayoutTransfer(
			commandBuffer,
			m_Image,
			m_ImageLayout,
			layout,
			srcAccessMask,
			dstAccessMask,
			srcStageFlags,
			dstStageFlags);

		result = vkEndCommandBuffer(commandBuffer);
		ASSERT_VULKAN(result);

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

		result = vkQueueWaitIdle(queue);
		ASSERT_VULKAN(result);

		m_ImageLayout = layout;
	}

	void HdrEnvMap::WriteBufferToImage(VkCommandBuffer commandBuffer, VkQueue queue, VkBuffer buffer)
	{
		VkCommandBufferBeginInfo beginInfo;
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.pNext = nullptr;
		beginInfo.flags = 0;
		beginInfo.pInheritanceInfo = nullptr;

		VkResult result = vkBeginCommandBuffer(commandBuffer, &beginInfo);
		ASSERT_VULKAN(result);

		VkBufferImageCopy bufferImageCopy;
		bufferImageCopy.bufferOffset = 0;
		bufferImageCopy.bufferRowLength = 0;
		bufferImageCopy.bufferImageHeight = 0;
		bufferImageCopy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		bufferImageCopy.imageSubresource.mipLevel = 0;
		bufferImageCopy.imageSubresource.baseArrayLayer = 0;
		bufferImageCopy.imageSubresource.layerCount = 1;
		bufferImageCopy.imageOffset = { 0, 0, 0 };
		bufferImageCopy.imageExtent = { m_Width, m_Height, 1 };

		vkCmdCopyBufferToImage(commandBuffer, buffer, m_Image, m_ImageLayout, 1, &bufferImageCopy);

		result = vkEndCommandBuffer(commandBuffer);
		ASSERT_VULKAN(result);

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

		result = vkQueueWaitIdle(queue);
		ASSERT_VULKAN(result);
	}
}
