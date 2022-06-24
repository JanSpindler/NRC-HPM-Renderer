#pragma once

#include <engine/graphics/vulkan/Buffer.hpp>
#include <array>

namespace en
{
	class NeuralRadianceCache
	{
	public:
		static void Init(VkDevice device);
		static void Shutdown(VkDevice device);
		static VkDescriptorSetLayout GetDescSetLayout();

		NeuralRadianceCache();

		void Destroy();

		VkDescriptorSet GetDescSet() const;

	private:
		static VkDescriptorSetLayout m_DescSetLayout;
		static VkDescriptorPool m_DescPool;

		std::array<vk::Buffer*, 6> m_Weights;
		std::array<vk::Buffer*, 6> m_DeltaWeights;

		VkDescriptorSet m_DescSet;
	};
}
