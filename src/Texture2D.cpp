#include <engine/vulkan/Texture2D.hpp>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <engine/Log.hpp>
#include <engine/VulkanAPI.hpp>
#include <engine/vulkan/Buffer.hpp>
#include <engine/vulkan/CommandPool.hpp>
#include <engine/vulkan/CommandRecorder.hpp>

namespace en::vk
{
	Texture2D* Texture2D::m_DummyTex;

	void Texture2D::Init()
	{
		m_DummyTex = new vk::Texture2D("data/image/dummy.png", VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_REPEAT);
	}

	void Texture2D::Shutdown()
	{
		m_DummyTex->Destroy();
		delete m_DummyTex;
	}

	const Texture2D* Texture2D::GetDummyTex()
	{
		return m_DummyTex;
	}

	Texture2D::Texture2D(const std::string& fileName, VkFilter filter, VkSamplerAddressMode addressMode) :
		m_ImageLayout(VK_IMAGE_LAYOUT_PREINITIALIZED)
	{
		int width;
		int height;
		int channelCount;

		stbi_uc* data = stbi_load(fileName.c_str(), &width, &height, &channelCount, STBI_rgb_alpha);
		if (data == nullptr)
			Log::Error("Failed to load Texture2D " + fileName, true);

		m_Width = static_cast<uint32_t>(width);
		m_Height = static_cast<uint32_t>(height);
		m_RealChannelCount = 4;
		m_SourceChannelCount = static_cast<uint32_t>(channelCount);

		LoadToDevice(data, filter, addressMode);

		stbi_image_free(data);
	}

	Texture2D::Texture2D(const std::array<std::vector<std::vector<float>>, 4>& data, VkFilter filter, VkSamplerAddressMode addressMode) :
		m_Width(data[0].size()),
		m_Height(data[0][0].size()),
		m_SourceChannelCount(4),
		m_RealChannelCount(4),
		m_ImageLayout(VK_IMAGE_LAYOUT_PREINITIALIZED)
	{
		// TODO: check for homogenous data size
		
		// Copy data into linear buffer
		std::vector<uint8_t> dataArray(m_Width * m_Height * m_RealChannelCount);
		for (uint32_t i = 0; i < m_Width; i++)
		{
			for (uint32_t j = 0; j < m_Height; j++)
			{
				for (uint32_t c = 0; c < m_RealChannelCount; c++)
				{
					float fVal = std::max(0.0f, static_cast<float>(data[c][i][j] * 255.0f));
					uint32_t index = c + m_RealChannelCount * i + m_RealChannelCount * m_Width * j;
					dataArray[index] = static_cast<uint8_t>(fVal);
				}
			}
		}

		LoadToDevice(dataArray.data(), filter, addressMode);
	}

	void Texture2D::Destroy()
	{
		VkDevice device = VulkanAPI::GetDevice();

		vkFreeMemory(device, m_DeviceMemory, nullptr);
		vkDestroySampler(device, m_Sampler, nullptr);
		vkDestroyImageView(device, m_ImageView, nullptr);
		vkDestroyImage(device, m_Image, nullptr);
	}

	uint32_t Texture2D::GetWidth() const
	{
		return m_Width;
	}

	uint32_t Texture2D::GetHeight() const
	{
		return m_Height;
	}

	uint32_t Texture2D::GetRealChannelCount() const
	{
		return m_RealChannelCount;
	}

	uint32_t Texture2D::GetSourceChannelCount() const
	{
		return m_SourceChannelCount;
	}

	size_t Texture2D::GetSizeInBytes() const
	{
		return m_Width * m_Height * m_RealChannelCount;
	}

	VkImageView Texture2D::GetImageView() const
	{
		return m_ImageView;
	}

	VkSampler Texture2D::GetSampler() const
	{
		return m_Sampler;
	}

	void Texture2D::LoadToDevice(const void* data, VkFilter filter, VkSamplerAddressMode addressMode)
	{
		VkDevice device = VulkanAPI::GetDevice();
		VkDeviceSize size = static_cast<VkDeviceSize>(GetSizeInBytes());
		VkQueue queue = VulkanAPI::GetGraphicsQueue(); // TODO: GetTransferQueue
		VkResult result;

		// Create image layout transfer commandpool + commandbuffer
		CommandPool commandPool = CommandPool(VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, VulkanAPI::GetGraphicsQFI());
		commandPool.AllocateBuffers(1, VK_COMMAND_BUFFER_LEVEL_PRIMARY);
		VkCommandBuffer commandBuffer = commandPool.GetBuffer(0);

		// Staging Buffer
		Buffer stagingBuffer(
			GetSizeInBytes(),
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			{});

		stagingBuffer.MapMemory(size, data, 0, 0);

		// Create Image
		VkFormat format = VK_FORMAT_R8G8B8A8_UNORM;

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

		result = vkCreateImage(device, &imageCreateInfo, nullptr, &m_Image);
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
		ChangeLayout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, commandBuffer, queue);

		WriteBufferToImage(commandBuffer, queue, stagingBuffer.GetVulkanHandle());

		ChangeLayout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, commandBuffer, queue);

		// Destroy Staging buffer
		stagingBuffer.Destroy();

		// Destroy CommandBuffer and CommandPool
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
	}

	void Texture2D::ChangeLayout(VkImageLayout layout, VkCommandBuffer commandBuffer, VkQueue queue)
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

	void Texture2D::WriteBufferToImage(VkCommandBuffer commandBuffer, VkQueue queue, VkBuffer buffer)
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
