#pragma once

#include <engine/compute/Matrix.hpp>
#include <engine/graphics/vulkan/Shader.hpp>

namespace en::vk
{
	struct MatmulConfigUniformData
	{
		uint32_t aRowCount;
		uint32_t aColCount;
		uint32_t bRowCount;
		uint32_t bColCount;
	};

	class Matmul
	{
	public:
		static void Init(VkDevice device);
		static void Shutdown(VkDevice device);

		static void Execute(Matrix* matA, Matrix* matB, Matrix* matC);

	private:
		static VkDescriptorSetLayout m_DescriptorSetLayout;
		static VkDescriptorPool m_DescriptorPool;
		static VkDescriptorSet m_DescriptorSet;
		
		static Buffer* m_ConfigUniformBuffer;

		static Shader m_Shader;
		static VkPipelineLayout m_PipelineLayout;
		static VkPipeline m_Pipeline;

		static void CreateDescriptorSetLayout(VkDevice device);
		static void CreateDescriptorPool(VkDevice device);
		static void AllocateDescriptorSet(VkDevice device);
		static void CreatePipelineLayout(VkDevice device);
		static void CreatePipeline(VkDevice device);

		static void UpdateDescriptorSet(const Matrix* matA, const Matrix* matB, const Matrix* matC);
		static void Dispatch(uint32_t rowCount, uint32_t colCount);
	};
}