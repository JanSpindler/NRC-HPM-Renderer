#include <engine/compute/KomputeManager.hpp>
#include <engine/graphics/VulkanAPI.hpp>

namespace en
{
    KomputeManager::KomputeManager() :
        m_Instance(new vk::Instance(en::VulkanAPI::GetInstance())),
        m_PhysicalDevice(new vk::PhysicalDevice(en::VulkanAPI::GetPhysicalDevice())),
        m_Device(new vk::Device(en::VulkanAPI::GetDevice())),
        m_Manager(new kp::Manager(m_Instance, m_PhysicalDevice, m_Device)),
        //m_Manager(new kp::Manager()),
        m_ComputeQFI(VulkanAPI::GetComputeQFI()),
        m_ComputeQueue(new vk::Queue(VulkanAPI::GetComputeQueue()))
	{
	}

    std::shared_ptr<kp::TensorT<float>> KomputeManager::tensor(
        const std::vector<float>& data,
        kp::Tensor::TensorTypes tensorType)
    {
        return m_Manager->tensorT<float>(data, tensorType);
    }

    std::shared_ptr<kp::Tensor> KomputeManager::tensor(
        void* data,
        uint32_t elementTotalCount,
        uint32_t elementMemorySize,
        const kp::Tensor::TensorDataTypes& dataType,
        kp::Tensor::TensorTypes tensorType)
    {
        return m_Manager->tensor(data, elementTotalCount, elementMemorySize, dataType, tensorType);
    }

    std::shared_ptr<kp::Sequence> KomputeManager::sequence(uint32_t queueIndex, uint32_t totalTimestamps)
    {
        std::shared_ptr<kp::Sequence> sq{ new kp::Sequence(
            m_PhysicalDevice,
            m_Device,
            m_ComputeQueue,
            m_ComputeQFI,
            totalTimestamps) };

        return sq;
        //return m_Manager->sequence(queueIndex, totalTimestamps);
    }
}
