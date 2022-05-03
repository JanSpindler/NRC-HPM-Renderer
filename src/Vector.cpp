#include <engine/compute/Vector.hpp>
#include <engine/util/Log.hpp>

namespace en::vk
{
	Vector::Vector(size_t dimension) :
		m_Dimension(dimension),
		m_MemorySize(dimension * sizeof(float)),
		m_Buffer(
			m_MemorySize,
			VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, // TODO: device only memory functionality
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			{})
	{
	}

	Vector Vector::operator+(const Vector& other) const
	{
		if (m_Dimension != other.m_Dimension)
			Log::Error("Vector addition requires equal dimension", true);

		Vector sum(m_Dimension);
		for (size_t i = 0; i < m_Dimension; i++)
			sum.SetValue(i, GetValue(i) + other.GetValue(i));

		return sum;
	}

	void Vector::operator+=(const Vector& other)
	{
		if (m_Dimension != other.m_Dimension)
			Log::Error("Vector addition requires equal dimension", true);

		for (size_t i = 0; i < m_Dimension; i++)
			SetValue(i, GetValue(i) + other.GetValue(i));
	}

	float Vector::operator*(const Vector& other) const
	{
		if (m_Dimension != other.m_Dimension)
			Log::Error("Vector dot product requires equal dimension", true);

		float dotProduct = 0.0f;
		for (size_t i = 0; i < m_Dimension; i++)
			dotProduct += GetValue(i) * other.GetValue(i);

		return dotProduct;
	}

	void Vector::Destroy()
	{
		m_Buffer.Destroy();
	}

	size_t Vector::GetDimension() const
	{
		return m_Dimension;
	}

	size_t Vector::GetMemorySize() const
	{
		return m_MemorySize;
	}

	float Vector::GetValue(size_t index) const
	{
		if (index >= m_Dimension)
			Log::Error("Index must be lower than vector dimension", true);

		// TODO: sync host memory with device memory if not synced
		return m_HostMemory[index];
	}

	const float* Vector::GetData() const
	{
		// TODO: sync memory
		return m_HostMemory;
	}

	void Vector::SetValue(size_t index, float value)
	{
		if (index >= m_Dimension)
			Log::Error("Index must be lower than vector dimension", true);

		m_HostMemory[index] = value;
		// TODO: sync memory
	}
}
