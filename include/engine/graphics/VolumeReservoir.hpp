/*#pragma once

#include <engine/graphics/vulkan/Buffer.hpp>

namespace en
{
	class VolumeReservoir
	{
	public:
		static void Init(VkDevice device);
		static void Shutdown(VkDevice device);
		static VkDescriptorSetLayout GetDescriptorSetLayout();

		VolumeReservoir(uint32_t pathVertexCount, uint32_t spatialKernelSize, uint32_t temporalKernelSize);

		void Init(uint32_t pixelCount);

		void Destroy();

		uint32_t GetPathVertexCount() const;
		uint32_t GetSpatialKernelSize() const;
		uint32_t GetTemporalKernelSize() const;

		VkDescriptorSet GetDescriptorSet() const;

	private:
		static VkDescriptorSetLayout m_DescSetLayout;
		static VkDescriptorPool m_DescPool;

		uint32_t m_PathVertexCount;
		uint32_t m_SpatialKernelSize;
		uint32_t m_TemporalKernelSize;

		size_t m_ReservoirBufferSize;
		size_t m_OldViewProjMatBufferSize;
		size_t m_OldReservoirsBufferSize;

		vk::Buffer* m_ReservoirBuffer;
		vk::Buffer* m_OldViewProjMatBuffer;
		vk::Buffer* m_OldReservoirsBuffer;

		VkDescriptorSet m_DescSet;

		void InitBuffers();

		void AllocateAndUpdateDescriptorSet(VkDevice device);
	};
}
*/