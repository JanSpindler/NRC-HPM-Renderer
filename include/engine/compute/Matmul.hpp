#pragma once

#include <engine/compute/Matrix.hpp>
#include <engine/graphics/vulkan/Shader.hpp>

namespace en::vk
{
	class Matmul
	{
	public:
		static void Init(VkDevice device);
		static void Shutdown(VkDevice device);

		static void Execute(const Matrix* matA, const Matrix* matB);

	private:
		static VkDescriptorSetLayout m_DescriptorSetLayout;
		static VkDescriptorPool m_DescriptorPool;
		static VkDescriptorSet m_DescriptorSet;
		
		static Shader m_Shader;

		static void CreateDescriptorSetLayout();
		static void CreateDescriptorPool();
	};
}
