#pragma once

#include <kompute/Kompute.hpp>
#include <vulkan/vulkan.hpp>
#include <vector>

// TODO: better fix
typedef vk::Instance VulkanInstance;
typedef vk::PhysicalDevice VulkanPhysicalDevice;
typedef vk::Device VulkanDevice;
typedef vk::Queue VulkanQueue;

namespace en
{
	class KomputeManager
	{
	public:
		KomputeManager();

		std::shared_ptr<kp::Algorithm> algorithm(
			const std::vector<std::shared_ptr<kp::Tensor>>& tensors,
			const std::vector<uint32_t>& spirv,
			const kp::Workgroup& workgroup,
			const std::vector<float>& specializationConstants,
			const std::vector<float>& pushConstants)
		{
			return m_Manager->algorithm<>(
				tensors, spirv, workgroup, specializationConstants, pushConstants);
		}

		template<typename S, typename P>
		std::shared_ptr<kp::Algorithm> algorithm(
			const std::vector<std::shared_ptr<kp::Tensor>>& tensors,
			const std::vector<uint32_t>& spirv,
			const kp::Workgroup& workgroup,
			const std::vector<S>& specializationConstants,
			const std::vector<P>& pushConstants)
		{
			return m_Manager->algorithm<S, P>(tensors, spirv, workgroup, specializationConstants, pushConstants);
		}

		std::shared_ptr<kp::TensorT<float>> tensor(
			const std::vector<float>& data,
			kp::Tensor::TensorTypes tensorType = kp::Tensor::TensorTypes::eDevice);

		std::shared_ptr<kp::Tensor> tensor(
			void* data,
			uint32_t elementTotalCount,
			uint32_t elementMemorySize,
			const kp::Tensor::TensorDataTypes& dataType,
			kp::Tensor::TensorTypes tensorType = kp::Tensor::TensorTypes::eDevice);

		std::shared_ptr<kp::Sequence> sequence(uint32_t queueIndex = 0, uint32_t totalTimestamps = 0);

	private:
		std::shared_ptr<VulkanInstance> m_Instance;
		std::shared_ptr<VulkanPhysicalDevice> m_PhysicalDevice;
		std::shared_ptr<VulkanDevice> m_Device;
		std::shared_ptr<kp::Manager> m_Manager;

		uint32_t m_ComputeQFI;
		std::shared_ptr<VulkanQueue> m_ComputeQueue;
	};
}
