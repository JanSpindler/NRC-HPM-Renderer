#include <engine/graphics/PointLight.hpp>
#include <imgui.h>
#include <glm/gtc/type_ptr.hpp>

namespace en
{
	VkDescriptorSetLayout PointLight::m_DescSetLayout;
	VkDescriptorPool PointLight::m_DescPool;

	void PointLight::Init(VkDevice device)
	{
		// Create desc set layout
		VkDescriptorSetLayoutBinding uniformBinding;
		uniformBinding.binding = 0;
		uniformBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		uniformBinding.descriptorCount = 1;
		uniformBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT;
		uniformBinding.pImmutableSamplers = nullptr;

		std::vector<VkDescriptorSetLayoutBinding> bindings = { uniformBinding };

		VkDescriptorSetLayoutCreateInfo layoutCI;
		layoutCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutCI.pNext = nullptr;
		layoutCI.flags = 0;
		layoutCI.bindingCount = bindings.size();
		layoutCI.pBindings = bindings.data();

		VkResult result = vkCreateDescriptorSetLayout(device, &layoutCI, nullptr, &m_DescSetLayout);
		ASSERT_VULKAN(result);

		// Create desc pool
		VkDescriptorPoolSize uniformBufferPS;
		uniformBufferPS.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		uniformBufferPS.descriptorCount = 1;

		std::vector<VkDescriptorPoolSize> poolSizes = { uniformBufferPS };

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

	void PointLight::Shutdown(VkDevice device)
	{
		vkDestroyDescriptorPool(device, m_DescPool, nullptr);
		vkDestroyDescriptorSetLayout(device, m_DescSetLayout, nullptr);
	}

	VkDescriptorSetLayout PointLight::GetDescriptorSetLayout()
	{
		return m_DescSetLayout;
	}

	PointLight::PointLight(const glm::vec3& pos, const glm::vec3& color, float strength) :
		m_UniformData({ .pos = pos, .strength = strength, .color = color }),
		m_UniformBuffer(
			sizeof(UniformData), 
			VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, 
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, 
			{})
	{
		VkDevice device = VulkanAPI::GetDevice();

		// Push data to buffer
		m_UniformBuffer.SetData(sizeof(UniformData), &m_UniformData, 0, 0);

		// Allocate desc set
		VkDescriptorSetAllocateInfo descSetAI;
		descSetAI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		descSetAI.pNext = nullptr;
		descSetAI.descriptorPool = m_DescPool;
		descSetAI.descriptorSetCount = 1;
		descSetAI.pSetLayouts = &m_DescSetLayout;

		VkResult result = vkAllocateDescriptorSets(device, &descSetAI, &m_DescSet);
		ASSERT_VULKAN(result);

		// Update desc set
		VkDescriptorBufferInfo uniformBufferInfo;
		uniformBufferInfo.buffer = m_UniformBuffer.GetVulkanHandle();
		uniformBufferInfo.offset = 0;
		uniformBufferInfo.range = sizeof(UniformData);

		VkWriteDescriptorSet uniformWrite;
		uniformWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		uniformWrite.pNext = nullptr;
		uniformWrite.dstSet = m_DescSet;
		uniformWrite.dstBinding = 0;
		uniformWrite.dstArrayElement = 0;
		uniformWrite.descriptorCount = 1;
		uniformWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		uniformWrite.pImageInfo = nullptr;
		uniformWrite.pBufferInfo = &uniformBufferInfo;
		uniformWrite.pTexelBufferView = nullptr;

		vkUpdateDescriptorSets(device, 1, &uniformWrite, 0, nullptr);
	}

	void PointLight::Destroy()
	{
		m_UniformBuffer.Destroy();
	}

	void PointLight::RenderImGui()
	{
		glm::vec3 oldPos = m_UniformData.pos;
		glm::vec3 oldColor = m_UniformData.color;
		float oldStrength = m_UniformData.strength;

		ImGui::Begin("Point Light");

		ImGui::SliderFloat3("Pos", glm::value_ptr(m_UniformData.pos), -16.0f, 16.0f);
		ImGui::SliderFloat3("Color", glm::value_ptr(m_UniformData.color), 0.0f, 1.0f);
		ImGui::DragFloat("Strength", &m_UniformData.strength, 0.1f, 0.0f);
		
		ImGui::End();

		if (m_UniformData.strength < 0.0f)
		{
			m_UniformData.strength = 0.0f;
		}

		if (oldPos != m_UniformData.pos ||
			oldColor != m_UniformData.color ||
			oldStrength != m_UniformData.strength)
		{
			m_UniformBuffer.SetData(sizeof(m_UniformData), &m_UniformData, 0, 0);
		}
	}

	VkDescriptorSet PointLight::GetDescriptorSet() const
	{
		return m_DescSet;
	}
}
