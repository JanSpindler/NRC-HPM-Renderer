#include <engine/graphics/DirLight.hpp>
#include <glm/gtx/transform.hpp>
#include <imgui.h>

glm::vec3 VecFromAngles(float zenith, float azimuth)
{
	// construct using vec4, discards w.
	return glm::vec3(
		// azimuth starts at positive x.
		glm::rotate(azimuth, glm::vec3(0.0f, 1.0f, 0.0f)) *
		glm::rotate(zenith, glm::vec3(1.0f, 0.0f, 0.0f)) *
		glm::vec4(0, 1, 0, 1)
	);
}

namespace en
{
	VkDescriptorSetLayout DirLight::m_DescriptorSetLayout;
	VkDescriptorPool DirLight::m_Pool;

	void DirLight::Init()
	{
		VkDevice device = VulkanAPI::GetDevice();

		// Create Descriptor Set Layout
		VkDescriptorSetLayoutBinding layoutBinding;
		layoutBinding.binding = 0;
		layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		layoutBinding.descriptorCount = 1;
		// in atmosphere.frag and aerial_perspective-compute.
		layoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT;
		layoutBinding.pImmutableSamplers = nullptr;

		VkDescriptorSetLayoutCreateInfo descSetLayoutCreateInfo;
		descSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		descSetLayoutCreateInfo.pNext = nullptr;
		descSetLayoutCreateInfo.flags = 0;
		descSetLayoutCreateInfo.bindingCount = 1;
		descSetLayoutCreateInfo.pBindings = &layoutBinding;

		VkResult result = vkCreateDescriptorSetLayout(device, &descSetLayoutCreateInfo, nullptr, &m_DescriptorSetLayout);
		ASSERT_VULKAN(result);

		// Create Descriptor Pool
		VkDescriptorPoolSize poolSize;
		poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		poolSize.descriptorCount = 1;

		VkDescriptorPoolCreateInfo descPoolCreateInfo;
		descPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		descPoolCreateInfo.pNext = nullptr;
		descPoolCreateInfo.flags = 0;
		descPoolCreateInfo.maxSets = 1;
		descPoolCreateInfo.poolSizeCount = 1;
		descPoolCreateInfo.pPoolSizes = &poolSize;

		result = vkCreateDescriptorPool(device, &descPoolCreateInfo, nullptr, &m_Pool);
		ASSERT_VULKAN(result);
	}

	void DirLight::Shutdown()
	{
		VkDevice device = VulkanAPI::GetDevice();

		vkDestroyDescriptorSetLayout(device, m_DescriptorSetLayout, nullptr);
		vkDestroyDescriptorPool(device, m_Pool, nullptr);
	}

	VkDescriptorSetLayout DirLight::GetDescriptorSetLayout()
	{
		return m_DescriptorSetLayout;
	}

	DirLight::DirLight(float zenith, float azimuth, glm::vec3 color, float strength) :
		m_DirLightData {
			color,
			zenith,
			VecFromAngles(zenith, azimuth),
			azimuth,
			strength },
			m_UniformBuffer{ vk::Buffer(
				sizeof(DirLightData),
				VK_MEMORY_PROPERTY_HOST_COHERENT_BIT |
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
				VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
				{}
			) }
	{
		VkDevice device = VulkanAPI::GetDevice();

		// Allocate Descriptor Set
		VkDescriptorSetAllocateInfo allocateInfo;
		allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocateInfo.pNext = nullptr;
		allocateInfo.descriptorPool = m_Pool;
		allocateInfo.descriptorSetCount = 1;
		allocateInfo.pSetLayouts = &m_DescriptorSetLayout;

		VkResult result = vkAllocateDescriptorSets(device, &allocateInfo, &m_DescriptorSet);
		ASSERT_VULKAN(result);

		// Write Descriptor Set
		VkDescriptorBufferInfo bufferInfo;
		bufferInfo.buffer = m_UniformBuffer.GetVulkanHandle();
		bufferInfo.offset = 0;
		bufferInfo.range = sizeof(DirLightData);

		VkWriteDescriptorSet writeDescSet;
		writeDescSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writeDescSet.pNext = nullptr;
		writeDescSet.dstSet = m_DescriptorSet;
		writeDescSet.dstBinding = 0;
		writeDescSet.dstArrayElement = 0;
		writeDescSet.descriptorCount = 1;
		writeDescSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		writeDescSet.pImageInfo = nullptr;
		writeDescSet.pBufferInfo = &bufferInfo;
		writeDescSet.pTexelBufferView = nullptr;

		vkUpdateDescriptorSets(device, 1, &writeDescSet, 0, nullptr);

		m_UniformBuffer.SetData(sizeof(DirLightData), &m_DirLightData, 0, 0);
	}

	void DirLight::Destroy()
	{
		m_UniformBuffer.Destroy();
	}

	float DirLight::GetZenith() const
	{
		return m_DirLightData.m_Zenith;
	}

	float DirLight::GetAzimuth() const
	{
		return m_DirLightData.m_Azimuth;
	}

	void DirLight::SetZenith(float z)
	{
		m_DirLightData.m_Zenith = z;
		m_DirLightData.m_Dir = VecFromAngles(z, m_DirLightData.m_Azimuth);
		m_UniformBuffer.SetData(sizeof(DirLightData), &m_DirLightData, 0, 0);
	}

	void DirLight::SetAzimuth(float a)
	{
		m_DirLightData.m_Azimuth = a;
		m_DirLightData.m_Dir = VecFromAngles(m_DirLightData.m_Zenith, a);
		m_UniformBuffer.SetData(sizeof(DirLightData), &m_DirLightData, 0, 0);
	}

	void DirLight::SetColor(glm::vec3 c)
	{
		m_DirLightData.m_Color = c;
		m_UniformBuffer.SetData(sizeof(glm::vec3), &c, offsetof(DirLightData, m_Color), 0);
	}

	VkDescriptorSet DirLight::GetDescriptorSet() const
	{
		return m_DescriptorSet;
	}

	void DirLight::RenderImgui()
	{
		ImGui::Begin("Dir Light");
		ImGui::DragFloat("zenith", &m_DirLightData.m_Zenith, 0.001);
		ImGui::DragFloat("azimuth", &m_DirLightData.m_Azimuth, 0.001);
		ImGui::DragFloat("Strength", &m_DirLightData.m_Strenth, 0.01);

		m_DirLightData.m_Dir = VecFromAngles(m_DirLightData.m_Zenith, m_DirLightData.m_Azimuth);
		m_UniformBuffer.SetData(sizeof(DirLightData), &m_DirLightData, 0, 0);

		ImGui::End();
	}
}
