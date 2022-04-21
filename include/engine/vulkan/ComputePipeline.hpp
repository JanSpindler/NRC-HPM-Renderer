#pragma once

#include <engine/vulkan/Buffer.hpp>
#include <engine/vulkan/Shader.hpp>
#include <glm/glm.hpp>

namespace en::vk
{
	const uint32_t MAX_COMPUTE_TASK_COUNT = 128;

	// Contains buffer and descriptor set for a compute pipeline execution
	class ComputeTask
	{
	public:
		static void Init();
		static void Shutdown();
		static VkDescriptorSetLayout GetDescriptorSetLayout();

		ComputeTask(const vk::Buffer* inputBuffer, const vk::Buffer* outputBuffer, const glm::uvec3& groupCount);

		void Destroy();

		const glm::uvec3& GetGroupCount() const;
		VkDescriptorSet GetDescriptorSet() const;

	private:
		static VkDescriptorSetLayout m_DescriptorSetLayout;
		static VkDescriptorPool m_DescriptorPool;

		const vk::Buffer* m_InputBuffer;
		const vk::Buffer* m_OutputBuffer;
		const glm::uvec3& m_GroupCount;

		VkDescriptorSet m_DescriptorSet;
	};

	// Simple Vulkan Compute Wrapper with 1 in and 1 out buffer
	class ComputePipeline
	{
	public:
		static void Init();
		static void Shutdown();

		ComputePipeline(const vk::Shader* shader);

		void Destroy();

		void RecordDispatch(VkCommandBuffer commandBuffer, const ComputeTask* computeTask) const;
		void Execute(VkQueue queue, uint32_t qfi, const ComputeTask* computeTask) const;

	private:
		static VkPipelineLayout m_PipelineLayout;

		const vk::Shader* m_Shader;
		VkPipeline m_Pipeline;
	};
}
