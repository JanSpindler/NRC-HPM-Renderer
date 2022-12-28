#pragma once

#include <engine/graphics/PointLight.hpp>

namespace en
{
	class PointLightArray
	{
	public:
		static void Init(VkDevice device);
		static void Shutdown(VkDevice device);
		static VkDescriptorSetLayout GetDescriptorSetLayout();

	private:
		static VkDescriptorSetLayout s_DescSetLayout;
		static VkDescriptorPool s_DescPool;
	};
}
