#include <engine/graphics/vulkan/Texture3D.hpp>
#include <engine/graphics/vulkan/CommandRecorder.hpp>
#include <engine/graphics/VulkanAPI.hpp>
#include <engine/graphics/vulkan/CommandPool.hpp>
#include <engine/graphics/vulkan/Buffer.hpp>
#include <array>
#include <openvdb/openvdb.h>

namespace en::vk
{
	Texture3D Texture3D::FromVDB(const std::string& fileName)
	{
		// Load grids from file
		Log::Info("Opening density VDB file");
		openvdb::io::File file(fileName);
		file.open();
		openvdb::GridPtrVecPtr grids = file.getGrids();
		file.close();

		// Find density grid
		openvdb::FloatGrid::Ptr densityGrid = nullptr;
		for (size_t gridIdx = 0; gridIdx < grids->size(); gridIdx++)
		{
			openvdb::GridBase::Ptr gridBase = grids->at(0);
			if (gridBase->isType<openvdb::FloatGrid>())
			{
				Log::Info("Found float grid");
				for (auto metaIt = gridBase->beginMeta(); metaIt != gridBase->endMeta(); metaIt++)
				{
					Log::Info("\t" + metaIt->first + ": " + metaIt->second->str());
				}
				densityGrid = openvdb::gridPtrCast<openvdb::FloatGrid>(gridBase);
			}
		}

		// Error check
		if (densityGrid == nullptr) { en::Log::Error("No density volume found in vdb file", true); }

		// Get size
		openvdb::Vec3i boxMin = densityGrid->metaValue<openvdb::Vec3i>("file_bbox_min");
		openvdb::Vec3i boxMax = densityGrid->metaValue<openvdb::Vec3i>("file_bbox_max");
		openvdb::Vec3i boxExtent = boxMax - boxMin + openvdb::Vec3i(1);
		const size_t uniformVoxelCount = boxExtent.x() * boxExtent.y() * boxExtent.z();
		
		// Create 3d float array
		std::vector<std::vector<std::vector<float>>> data(boxExtent.x());
		for (std::vector<std::vector<float>>& vvf : data)
		{
			vvf.resize(boxExtent.y());
			for (std::vector<float>& vf : vvf) { vf.resize(boxExtent.z()); }
		}

		// Read data from grid
		for (auto valIt = densityGrid->cbeginValueOn(); valIt; ++valIt)
		{
			openvdb::CoordBBox bBox;
			valIt.getBoundingBox(bBox);
			const float value = valIt.getValue();
			for (auto bBoxIt = bBox.begin(); bBoxIt; ++bBoxIt)
			{
				openvdb::Vec3i bBoxItPos = (*bBoxIt).asVec3i() - boxMin;
				data[bBoxItPos.x()][bBoxItPos.y()][bBoxItPos.z()] = value;
			}
		}

		// Return texture
		return Texture3D(
			data, 
			VK_FILTER_LINEAR, 
			VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER, 
			VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK);
	}

	Texture3D::Texture3D(
		const std::vector<std::vector<std::vector<float>>>& data, 
		VkFilter filter, 
		VkSamplerAddressMode addressMode,
		VkBorderColor borderColor)
		:
		m_Width(data.size()),
		m_Height(data[0].size()),
		m_Depth(data[0][0].size()),
		m_RealChannelCount(4),
		m_ImageLayout(VK_IMAGE_LAYOUT_PREINITIALIZED)
	{
		// TODO: check for homogenous size

		// Copy to linear buffer
		std::vector<uint8_t> dataArray(m_Width * m_Height * m_Depth * 4);
		for (uint32_t i = 0; i < m_Width; i++)
		{
			for (uint32_t j = 0; j < m_Height; j++)
			{
				for (uint32_t k = 0; k < m_Depth; k++)
				{
					uint8_t value = static_cast<uint8_t>(data[i][j][k] * 255.0f);
					uint32_t index = 4 * i + 4 * m_Width * j + 4 * m_Width * m_Height * k;
					dataArray[index + 0] = value;
					dataArray[index + 1] = value;
					dataArray[index + 2] = value;
					dataArray[index + 3] = 1;
				}
			}
		}

		LoadToDevice(dataArray.data(), filter, addressMode, borderColor);
	}

	Texture3D::Texture3D(
		const std::array<std::vector<std::vector<std::vector<float>>>, 4>& data, 
		VkFilter filter, 
		VkSamplerAddressMode addressMode,
		VkBorderColor borderColor)
		:
		m_Width(data[0].size()),
		m_Height(data[0][0].size()),
		m_Depth(data[0][0][0].size()),
		m_RealChannelCount(4),
		m_ImageLayout(VK_IMAGE_LAYOUT_PREINITIALIZED)
	{
		// TODO: check for homogenous size

		// Copy to linear buffer
		std::vector<uint8_t> dataArray(m_Width * m_Height * m_Depth * 4);
		for (uint32_t i = 0; i < m_Width; i++)
		{
			for (uint32_t j = 0; j < m_Height; j++)
			{
				for (uint32_t k = 0; k < m_Depth; k++)
				{
					for (uint32_t c = 0; c < 4; c++)
					{
						float fVal = std::max(0.0f, static_cast<float>(data[c][i][j][k]) * 255.0f);
						uint32_t index = c + 4 * i + 4 * m_Width * j + 4 * m_Width * m_Height * k;
						dataArray[index] = static_cast<uint8_t>(fVal);
					}
				}
			}
		}

		LoadToDevice(dataArray.data(), filter, addressMode, borderColor);
	}

	void Texture3D::Destroy()
	{
		VkDevice device = VulkanAPI::GetDevice();

		vkFreeMemory(device, m_DeviceMemory, nullptr);
		vkDestroySampler(device, m_Sampler, nullptr);
		vkDestroyImageView(device, m_ImageView, nullptr);
		vkDestroyImage(device, m_Image, nullptr);
	}

	uint32_t Texture3D::GetWidth() const
	{
		return m_Width;
	}

	uint32_t Texture3D::GetHeight() const
	{
		return m_Height;
	}

	uint32_t Texture3D::GetDepth() const
	{
		return m_Depth;
	}

	uint32_t Texture3D::GetRealChannelCount() const
	{
		return m_RealChannelCount;
	}

	size_t Texture3D::GetRealSizeInBytes() const
	{
		return m_Width * m_Height * m_Depth * m_RealChannelCount;
	}

	VkImageView Texture3D::GetImageView() const
	{
		return m_ImageView;
	}

	VkSampler Texture3D::GetSampler() const
	{
		return m_Sampler;
	}

	void Texture3D::LoadToDevice(void* data, VkFilter filter, VkSamplerAddressMode addressMode, VkBorderColor borderColor)
	{
		VkDevice device = VulkanAPI::GetDevice();
		VkDeviceSize size = static_cast<VkDeviceSize>(GetRealSizeInBytes());
		VkQueue queue = VulkanAPI::GetGraphicsQueue(); // TODO: GetTransferQueue
		VkResult result;

		// Create image layout transfer commandpool + commandbuffer
		CommandPool commandPool = CommandPool(VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, VulkanAPI::GetGraphicsQFI());
		commandPool.AllocateBuffers(1, VK_COMMAND_BUFFER_LEVEL_PRIMARY);
		VkCommandBuffer commandBuffer = commandPool.GetBuffer(0);

		// Staging Buffer
		Buffer stagingBuffer(
			GetRealSizeInBytes(),
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			{});

		stagingBuffer.SetData(size, data, 0, 0);

		// Create Image
		VkFormat format = VK_FORMAT_R8G8B8A8_UNORM;

		VkImageCreateInfo imageCreateInfo;
		imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageCreateInfo.pNext = nullptr;
		imageCreateInfo.flags = 0;
		imageCreateInfo.imageType = VK_IMAGE_TYPE_3D;
		imageCreateInfo.format = format;
		imageCreateInfo.extent = { m_Width, m_Height, m_Depth };
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
		imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_3D;
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
		samplerCreateInfo.borderColor = borderColor;
		samplerCreateInfo.unnormalizedCoordinates = VK_FALSE;

		result = vkCreateSampler(device, &samplerCreateInfo, nullptr, &m_Sampler);
		ASSERT_VULKAN(result);
	}

	void Texture3D::ChangeLayout(VkImageLayout layout, VkCommandBuffer commandBuffer, VkQueue queue)
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

	void Texture3D::WriteBufferToImage(VkCommandBuffer commandBuffer, VkQueue queue, VkBuffer buffer)
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
		bufferImageCopy.imageExtent = { m_Width, m_Height, m_Depth };

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
