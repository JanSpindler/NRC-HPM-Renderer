#include <engine/objects/Material.hpp>
#include <engine/graphics/VulkanAPI.hpp>

namespace en
{
	VkDescriptorSetLayout Material::m_DescriptorSetLayout;
	VkDescriptorPool Material::m_DescriptorPool;

	void Material::Init()
	{
		VkDevice device = VulkanAPI::GetDevice();

		// Create Descriptor Set Layout
		VkDescriptorSetLayoutBinding uniformBufferBinding;
		uniformBufferBinding.binding = 0;
		uniformBufferBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		uniformBufferBinding.descriptorCount = 1;
		uniformBufferBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		uniformBufferBinding.pImmutableSamplers = nullptr;

		VkDescriptorSetLayoutBinding diffuseTexBinding;
		diffuseTexBinding.binding = 1;
		diffuseTexBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		diffuseTexBinding.descriptorCount = 1;
		diffuseTexBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		diffuseTexBinding.pImmutableSamplers = nullptr;

		std::vector<VkDescriptorSetLayoutBinding> layoutBindings = { uniformBufferBinding, diffuseTexBinding };

		VkDescriptorSetLayoutCreateInfo descSetLayoutCreateInfo;
		descSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		descSetLayoutCreateInfo.pNext = nullptr;
		descSetLayoutCreateInfo.flags = 0;
		descSetLayoutCreateInfo.bindingCount = layoutBindings.size();
		descSetLayoutCreateInfo.pBindings = layoutBindings.data();

		VkResult result = vkCreateDescriptorSetLayout(device, &descSetLayoutCreateInfo, nullptr, &m_DescriptorSetLayout);
		ASSERT_VULKAN(result);

		// Create Descriptor Pool
		VkDescriptorPoolSize uniformBufferPoolSize;
		uniformBufferPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		uniformBufferPoolSize.descriptorCount = 1;

		VkDescriptorPoolSize diffuseTexPoolSize;
		diffuseTexPoolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		diffuseTexPoolSize.descriptorCount = 1;

		std::vector<VkDescriptorPoolSize> poolSizes = { uniformBufferPoolSize, diffuseTexPoolSize };

		VkDescriptorPoolCreateInfo descPoolCreateInfo;
		descPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		descPoolCreateInfo.pNext = nullptr;
		descPoolCreateInfo.flags = 0;
		descPoolCreateInfo.maxSets = MAX_COUNT;
		descPoolCreateInfo.poolSizeCount = poolSizes.size();
		descPoolCreateInfo.pPoolSizes = poolSizes.data();

		result = vkCreateDescriptorPool(device, &descPoolCreateInfo, nullptr, &m_DescriptorPool);
		ASSERT_VULKAN(result);
	}

	void Material::Shutdown()
	{
		VkDevice device = VulkanAPI::GetDevice();

		vkDestroyDescriptorPool(device, m_DescriptorPool, nullptr);
		vkDestroyDescriptorSetLayout(device, m_DescriptorSetLayout, nullptr);
	}

	VkDescriptorSetLayout Material::GetDescriptorSetLayout()
	{
		return m_DescriptorSetLayout;
	}

	Material::Material(const glm::vec4& diffuseColor, const vk::Texture2D* diffuseTex) :
		m_UniformData({
			diffuseColor,
			diffuseTex != nullptr }),
			m_DiffuseTex(diffuseTex),
			m_UniformBuffer(new vk::Buffer(
				static_cast<VkDeviceSize>(sizeof(UniformData)),
				VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
				VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
				{}))
	{
		UpdateUniformBuffer();

		VkDevice device = VulkanAPI::GetDevice();

		// Uniform Descriptor Set
		VkDescriptorSetAllocateInfo allocateInfo;
		allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocateInfo.pNext = nullptr;
		allocateInfo.descriptorPool = m_DescriptorPool;
		allocateInfo.descriptorSetCount = 1;
		allocateInfo.pSetLayouts = &m_DescriptorSetLayout;

		VkResult result = vkAllocateDescriptorSets(device, &allocateInfo, &m_DescriptorSet);
		ASSERT_VULKAN(result);

		VkDescriptorBufferInfo uniformBufferInfo;
		uniformBufferInfo.buffer = m_UniformBuffer->GetVulkanHandle();
		uniformBufferInfo.offset = 0;
		uniformBufferInfo.range = static_cast<VkDeviceSize>(sizeof(UniformData));

		VkWriteDescriptorSet uniformBufferWrite;
		uniformBufferWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		uniformBufferWrite.pNext = nullptr;
		uniformBufferWrite.dstSet = m_DescriptorSet;
		uniformBufferWrite.dstBinding = 0;
		uniformBufferWrite.dstArrayElement = 0;
		uniformBufferWrite.descriptorCount = 1;
		uniformBufferWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		uniformBufferWrite.pImageInfo = nullptr;
		uniformBufferWrite.pBufferInfo = &uniformBufferInfo;
		uniformBufferWrite.pTexelBufferView = nullptr;

		VkDescriptorImageInfo diffuseTexInfo;
		if (m_UniformData.useDiffuseTex)
		{
			diffuseTexInfo.sampler = m_DiffuseTex->GetSampler();
			diffuseTexInfo.imageView = m_DiffuseTex->GetImageView();
			diffuseTexInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		}
		else
		{
			const vk::Texture2D* dummyTex = vk::Texture2D::GetDummyTex();
			diffuseTexInfo.sampler = dummyTex->GetSampler();
			diffuseTexInfo.imageView = dummyTex->GetImageView();
			diffuseTexInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		}

		VkWriteDescriptorSet diffuseTexWrite;
		diffuseTexWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		diffuseTexWrite.pNext = nullptr;
		diffuseTexWrite.dstSet = m_DescriptorSet;
		diffuseTexWrite.dstBinding = 1;
		diffuseTexWrite.dstArrayElement = 0;
		diffuseTexWrite.descriptorCount = 1;
		diffuseTexWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		diffuseTexWrite.pImageInfo = &diffuseTexInfo;
		diffuseTexWrite.pBufferInfo = nullptr;
		diffuseTexWrite.pTexelBufferView = nullptr;

		std::vector<VkWriteDescriptorSet> writeDescSets = { uniformBufferWrite, diffuseTexWrite };

		vkUpdateDescriptorSets(device, writeDescSets.size(), writeDescSets.data(), 0, nullptr);
	}

	void Material::Destroy()
	{
		m_UniformBuffer->Destroy();
		delete m_UniformBuffer;
	}

	const glm::vec4& Material::GetDiffuseColor() const
	{
		return m_UniformData.diffuseColor;
	}

	void Material::SetDiffuseColor(const glm::vec4& diffuseColor)
	{
		m_UniformData.diffuseColor = diffuseColor;
		UpdateUniformBuffer();
	}

	const vk::Texture2D* Material::GetDiffuseTex() const
	{
		return m_DiffuseTex;
	}

	void Material::SetDiffuseTex(const vk::Texture2D* diffuseTex)
	{
		m_DiffuseTex = diffuseTex;

		// Write Descriptor Set
		VkDescriptorImageInfo diffuseTexInfo;
		if (m_UniformData.useDiffuseTex)
		{
			diffuseTexInfo.sampler = m_DiffuseTex->GetSampler();
			diffuseTexInfo.imageView = m_DiffuseTex->GetImageView();
			diffuseTexInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		}
		else
		{
			const vk::Texture2D* dummyTex = vk::Texture2D::GetDummyTex();
			diffuseTexInfo.sampler = dummyTex->GetSampler();
			diffuseTexInfo.imageView = dummyTex->GetImageView();
			diffuseTexInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		}

		VkWriteDescriptorSet diffuseTexWrite;
		diffuseTexWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		diffuseTexWrite.pNext = nullptr;
		diffuseTexWrite.dstSet = m_DescriptorSet;
		diffuseTexWrite.dstBinding = 1;
		diffuseTexWrite.dstArrayElement = 0;
		diffuseTexWrite.descriptorCount = 1;
		diffuseTexWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		diffuseTexWrite.pImageInfo = &diffuseTexInfo;
		diffuseTexWrite.pBufferInfo = nullptr;
		diffuseTexWrite.pTexelBufferView = nullptr;

		vkUpdateDescriptorSets(VulkanAPI::GetDevice(), 1, &diffuseTexWrite, 0, nullptr);

		// Update Uniform
		bool useDiffuseTex = diffuseTex != nullptr;
		if (useDiffuseTex != m_UniformData.useDiffuseTex)
		{
			m_UniformData.useDiffuseTex = useDiffuseTex;
			UpdateUniformBuffer();
		}
	}

	VkDescriptorSet Material::GetDescriptorSet() const
	{
		return m_DescriptorSet;
	}

	void Material::UpdateUniformBuffer() const
	{
		m_UniformBuffer->SetData(static_cast<VkDeviceSize>(sizeof(UniformData)), &m_UniformData, 0, 0);
	}
}
