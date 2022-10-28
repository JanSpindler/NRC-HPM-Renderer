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
		densityTexBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT;
		densityTexBinding.pImmutableSamplers = nullptr;;

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
		poolCI.maxSets = 2;
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

	VolumeData::VolumeData(const vk::Texture3D* densityTex, float densityFactor, float g) :
		m_DensityFactor(densityFactor),
		m_G(g),
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

	void VolumeData::Destroy()
	{
	}

	void VolumeData::RenderImGui()
	{
		ImGui::Begin("HPM Volume");
		ImGui::Text("Density Factor %f", m_DensityFactor);
		ImGui::Text("G %f", m_G);
		ImGui::End();
	}

	float VolumeData::GetDensityFactor() const
	{
		return m_DensityFactor;
	}

	float VolumeData::GetG() const
	{
		return m_G;
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
		std::vector<VkWriteDescriptorSet> writes = { densityTexWrite };

		vkUpdateDescriptorSets(VulkanAPI::GetDevice(), writes.size(), writes.data(), 0, nullptr);
	}
}
