#include <engine/graphics/PointLightArray.hpp>

namespace en
{
	VkDescriptorSetLayout PointLightArray::s_DescSetLayout = VK_NULL_HANDLE;
	VkDescriptorPool PointLightArray::s_DescPool = VK_NULL_HANDLE;

	void PointLightArray::Init(VkDevice device)
	{
		VkDescriptorSetLayoutBinding;
	}

	void PointLightArray::Shutdown(VkDevice device)
	{

	}

	VkDescriptorSetLayout PointLightArray::GetDescriptorSetLayout()
	{
		return s_DescSetLayout;
	}
}
