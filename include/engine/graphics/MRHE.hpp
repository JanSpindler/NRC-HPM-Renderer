#pragma once

#include <engine/graphics/vulkan/Buffer.hpp>

namespace en
{
	class MRHE
	{
	public:
		static void Init(VkDevice device);
		static void Shutdown(VkDevice device);
		static VkDescriptorSetLayout GetDescriptorSetLayout();

		MRHE();

		void Destroy();

		VkDescriptorSet GetDescriptorSet() const;

		size_t GetHashTableSize() const;

	private:
		struct UniformData
		{
			uint32_t levelCount;
			uint32_t hashTableSize;
			uint32_t featureCount;
			uint32_t minRes;
			uint32_t maxRes;
			uint32_t resolutions[16];
		};
		
		static VkDescriptorSetLayout m_DescSetLayout;
		static VkDescriptorPool m_DescPool;

		UniformData m_UniformData;
		vk::Buffer m_UniformBuffer;
		VkDescriptorSet m_DescSet;

		size_t m_HashTablesSize;
		vk::Buffer m_HashTablesBuffer;
		vk::Buffer m_DeltaHashTablesBuffer;
	};
}
