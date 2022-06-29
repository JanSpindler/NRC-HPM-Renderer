#include <engine/objects/VolumeData.hpp>
#include <engine/graphics/VulkanAPI.hpp>
#include <vector>
#include <imgui.h>
#include <glm/gtc/random.hpp>

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
		densityTexBinding.pImmutableSamplers = nullptr;

		VkDescriptorSetLayoutBinding uniformBufferBinding;
		uniformBufferBinding.binding = 1;
		uniformBufferBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		uniformBufferBinding.descriptorCount = 1;
		uniformBufferBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT;
		uniformBufferBinding.pImmutableSamplers = nullptr;

		std::vector<VkDescriptorSetLayoutBinding> bindings = { densityTexBinding, uniformBufferBinding };

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

		VkDescriptorPoolSize uniformBufferPoolSize;
		uniformBufferPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		uniformBufferPoolSize.descriptorCount = 1;

		std::vector<VkDescriptorPoolSize> poolSizes = { densityTexPoolSize, uniformBufferPoolSize };

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

	VolumeData::VolumeData(const vk::Texture3D* densityTex) :
		m_DensityTex(densityTex),
		m_UniformBuffer(
			sizeof(VolumeUniformData), 
			VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			{}),
		m_UniformData({ 
			.random = glm::vec4(0.0f),
			.useNN = 0,
			.densityFactor = 2.0f,
			.g = 0.8f,
			.sigmaS = 0.7f,
			.sigmaE = 0.05f,
			.brightness = 0.0f,
			.lowPassIndex = 0 })
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

	void VolumeData::Update(bool cameraChanged)
	{
		m_UniformData.random = glm::linearRand(glm::vec4(0.0f), glm::vec4(1.0f));

		if (m_UniformData.lowPassIndex < 1000000)
			m_UniformData.lowPassIndex++;

		if (cameraChanged)
			m_UniformData.lowPassIndex = 0;

		m_UniformBuffer.SetData(sizeof(VolumeUniformData), &m_UniformData, 0, 0);
	}

	void VolumeData::Destroy()
	{
		m_UniformBuffer.Destroy();
	}

	void VolumeData::RenderImGui()
	{
		ImGui::Begin("HPM Volume");

		ImGui::Checkbox("Use NN", reinterpret_cast<bool*>(&m_UniformData.useNN));
		ImGui::SliderFloat("Density Factor", &m_UniformData.densityFactor, 0.0f, 16.0f);
		ImGui::SliderFloat("G", &m_UniformData.g, 0.001f, 0.999f);
		ImGui::SliderFloat("Sigma S", &m_UniformData.sigmaS, 0.001f, 1.0f);
		ImGui::SliderFloat("Sigma E", &m_UniformData.sigmaE, 0.001f, 1.0f);
		ImGui::DragFloat("Brightness Exponent", &m_UniformData.brightness, 0.001f);
		if (ImGui::Button("Reset Low Pass Filter"))
			m_UniformData.lowPassIndex = 0;
		ImGui::Text(std::string("Low Pass Index: " + std::to_string(m_UniformData.lowPassIndex)).c_str());
		
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

		// Uniform buffer
		VkDescriptorBufferInfo uniformBufferInfo;
		uniformBufferInfo.buffer = m_UniformBuffer.GetVulkanHandle();
		uniformBufferInfo.offset = 0;
		uniformBufferInfo.range = sizeof(VolumeUniformData);

		VkWriteDescriptorSet uniformBufferWrite;
		uniformBufferWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		uniformBufferWrite.pNext = nullptr;
		uniformBufferWrite.dstSet = m_DescriptorSet;
		uniformBufferWrite.dstBinding = 1;
		uniformBufferWrite.dstArrayElement = 0;
		uniformBufferWrite.descriptorCount = 1;
		uniformBufferWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		uniformBufferWrite.pImageInfo = nullptr;
		uniformBufferWrite.pBufferInfo = &uniformBufferInfo;
		uniformBufferWrite.pTexelBufferView = nullptr;

		// Update
		std::vector<VkWriteDescriptorSet> writes = { densityTexWrite, uniformBufferWrite };

		vkUpdateDescriptorSets(VulkanAPI::GetDevice(), writes.size(), writes.data(), 0, nullptr);
	}
}
