#include <engine/VolumeData.hpp>
#include <vector>

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

	VolumeData::VolumeData(const vk::Texture3D* densityTex)
	{

	}

	void VolumeData::Destroy()
	{

	}

	void VolumeData::RenderImGui()
	{

	}

	VkDescriptorSet VolumeData::GetDescriptorSet() const
	{
		return VK_NULL_HANDLE;
	}
}
