#include <engine/compute/Tensor.hpp>
#include <engine/util/Log.hpp>

namespace en
{
	Tensor::Tensor(const std::vector<size_t>& shape, float defaultVal) :
		m_Shape(shape),
		m_VkBuffer(m_MemorySize,
			VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			{})
	{
		// Assertions
		if (shape.size() == 0)
			Log::Error("Tensor should have at least 1 dimension", true);

		// Get memory size
		m_FloatCount = 1;
		for (size_t dim : shape)
			m_FloatCount *= dim;
		m_MemorySize = m_FloatCount * sizeof(float);

		if (m_MemorySize == 0)
			Log::Error("Tensor memory size should not be 0", true);

		// Allocate memory
		m_HostData = reinterpret_cast<float*>(malloc(m_MemorySize));

		// Set memory to default value
		for (size_t i = 0; i < m_FloatCount; i++)
			m_HostData[i] = defaultVal;

		// Push to device
		m_VkBuffer.MapMemory(m_MemorySize, m_HostData, 0, 0);
	}

	Tensor Tensor::operator+(const Tensor& other) const
	{
		if (this->m_Shape != other.m_Shape)
			Log::Error("Tensor addition requires equal shape", true);

		Tensor result(m_Shape, 0.0f);
		for (size_t i = 0; i < m_FloatCount; i++)
			result.SetValue(i, this->GetValue(i) + other.GetValue(i));

		return result;
	}

	Tensor Tensor::operator*(const Tensor& other) const
	{
		return Tensor({ 1 }, 0.0f);
	}

	void Tensor::Destroy()
	{
		m_VkBuffer.Destroy();
	}

	float Tensor::GetValue(size_t linearIndex) const
	{
		return m_HostData[linearIndex];
	}

	size_t Tensor::GetLinearIndex(const std::vector<size_t>& indices) const
	{
		if (indices.size() > m_Shape.size())
		{
			Log::Error("Too many indices for linear index", true);
			return SIZE_MAX;
		}
		else if (indices.size() == m_Shape.size())
		{
			size_t index = 0;
			size_t dimProduct = 1;

			for (size_t i = m_Shape.size() - 1; i >= 0; i--)
			{
				index += indices[i] * dimProduct;
				dimProduct *= m_Shape[i];
			}
			
			return index;
		}
		else
		{
			Log::Error("Cant handle this index input yet", true);
			return SIZE_MAX;
		}
	}

	void Tensor::SetValue(size_t linearIndex, float value)
	{
		if (linearIndex < m_FloatCount)
			m_HostData[linearIndex] = value;
	}
}
