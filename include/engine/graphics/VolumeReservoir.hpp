#pragma once

#include <engine/graphics/vulkan/Buffer.hpp>

namespace en
{
	class VolumeReservoir
	{
	public:
		static void Init(VkDevice device);
		static void Shutdown(VkDevice device);
		static VkDescriptorSetLayout GetDescriptorSetLayout();

		VolumeReservoir();

		void Destroy();

		VkDescriptorSet GetDescriptorSet() const;

	private:
		static VkDescriptorSetLayout m_DescSetLayout;
		static VkDescriptorPool m_DescPool;

		vk::Buffer* m_ReservoirBuffer;

		VkDescriptorSet m_DescSet;

		void InitReservoir();
	};
}
