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

		VolumeReservoir(uint32_t pathVertexCount);

		void Init(uint32_t pixelCount);

		void Destroy();

		uint32_t GetPathVertexCount() const;

		VkDescriptorSet GetDescriptorSet() const;

	private:
		static VkDescriptorSetLayout m_DescSetLayout;
		static VkDescriptorPool m_DescPool;

		uint32_t m_PathVertexCount;

		vk::Buffer* m_ReservoirBuffer;

		VkDescriptorSet m_DescSet;

		void InitReservoir(size_t reservoirBufferSize);

		void AllocateAndUpdateDescriptorSet(VkDevice device);
	};
}
