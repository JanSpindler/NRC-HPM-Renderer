#include <engine/objects/VolumeData.hpp>
#include <engine/graphics/VulkanAPI.hpp>
#include <vector>
#include <imgui.h>

namespace en
{
	VkDescriptorSetLayout VolumeData::m_DescriptorSetLayout;
	VkDescriptorPool VolumeData::m_DescriptorPool;

	void VolumeData::Init(VkDevice device)
	{
		// Create descriptor set layout
		VkDescriptorSetLayoutBinding densityTexBinding;
		densityTexBinding.binding = 0;
		densityTexBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		densityTexBinding.descriptorCount = 1;
		densityTexBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		densityTexBinding.pImmutableSamplers = nullptr;

		std::vector<VkDescriptorSetLayoutBinding> bindings = { densityTexBinding };

		VkDescriptorSetLayoutCreateInfo layoutCI;
		layoutCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutCI.pNext = nullptr;
		layoutCI.flags = 0;
		layoutCI.bindingCount = bindings.size();
		layoutCI.pBindings = bindings.data();

		VkResult result = vkCreateDescriptorSetLayout(device, &layoutCI, nullptr, &m_DescriptorSetLayout);
		ASSERT_VULKAN(result);

		// Create descriptor pool
		VkDescriptorPoolSize densityTexPoolSize;
		densityTexPoolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		densityTexPoolSize.descriptorCount = 1;

		std::vector<VkDescriptorPoolSize> poolSizes = { densityTexPoolSize };

		VkDescriptorPoolCreateInfo poolCI;
		poolCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolCI.pNext = nullptr;
		poolCI.flags = 0;
		poolCI.maxSets = 1;
		poolCI.poolSizeCount = poolSizes.size();
		poolCI.pPoolSizes = poolSizes.data();

		result = vkCreateDescriptorPool(device, &poolCI, nullptr, &m_DescriptorPool);
		ASSERT_VULKAN(result);
	}

	void VolumeData::Shutdown(VkDevice device)
	{
		vkDestroyDescriptorPool(device, m_DescriptorPool, nullptr);
		vkDestroyDescriptorSetLayout(device, m_DescriptorSetLayout, nullptr);
	}

	VkDescriptorSetLayout VolumeData::GetDescriptorSetLayout()
	{
		return m_DescriptorSetLayout;
	}

	VolumeData::VolumeData(const vk::Texture3D* densityTex) :
		m_DensityTex(densityTex)
	{
		// Create and update descriptor set
		VkDescriptorSetAllocateInfo descSetAI;
		descSetAI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		descSetAI.pNext = nullptr;
		descSetAI.descriptorPool = m_DescriptorPool;
		descSetAI.descriptorSetCount = 1;
		descSetAI.pSetLayouts = &m_DescriptorSetLayout;

		VkResult result = vkAllocateDescriptorSets(VulkanAPI::GetDevice(), &descSetAI, &m_DescriptorSet);
		ASSERT_VULKAN(result);

		UpdateDescriptorSet();
	}

	void VolumeData::RenderImGui()
	{
		ImGui::Begin("HPM Volume");

		ImGui::End();
	}

	VkDescriptorSet VolumeData::GetDescriptorSet() const
	{
		return m_DescriptorSet;
	}

	void VolumeData::UpdateDescriptorSet()
	{
		// Density tex
		VkDescriptorImageInfo densityTexImageInfo;
		densityTexImageInfo.sampler = m_DensityTex->GetSampler();
		densityTexImageInfo.imageView = m_DensityTex->GetImageView();
		densityTexImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		VkWriteDescriptorSet densityTexWrite;
		densityTexWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		densityTexWrite.pNext = nullptr;
		densityTexWrite.dstSet = m_DescriptorSet;
		densityTexWrite.dstBinding = 0;
		densityTexWrite.dstArrayElement = 0;
		densityTexWrite.descriptorCount = 1;
		densityTexWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		densityTexWrite.pImageInfo = &densityTexImageInfo;
		densityTexWrite.pBufferInfo = nullptr;
		densityTexWrite.pTexelBufferView = nullptr;

		// Update
		vkUpdateDescriptorSets(VulkanAPI::GetDevice(), 1, &densityTexWrite, 0, nullptr);
	}
}
