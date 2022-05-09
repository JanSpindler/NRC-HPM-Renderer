#include <engine/compute/Matrix.hpp>

namespace en::vk
{
	Matrix::Matrix(size_t rowCount, size_t colCount, float diagonal) :
		m_RowCount(rowCount),
		m_ColCount(colCount),
		m_MemorySize(rowCount * colCount * sizeof(float)),
		m_HostMemory(reinterpret_cast<float*>(malloc(m_MemorySize))),
		m_Buffer(
			m_MemorySize,
			VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, // TODO: device only memory functionality
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			{})
	{
		if (!isnan(diagonal))
		{
			for (size_t row = 0; row < m_RowCount; row++)
			{
				for (size_t col = 0; col < m_ColCount; col++)
				{
					float value = row == col ? diagonal : 0.0f;
					SetValue(row, col, value);
				}
			}
		}
	}

	Matrix::Matrix(const std::vector<std::vector<float>>& values) :
		m_RowCount(values.size()),
		m_ColCount(values[0].size()),
		m_MemorySize(m_RowCount * m_ColCount * sizeof(float)),
		m_HostMemory(reinterpret_cast<float*>(malloc(m_MemorySize))),
		m_Buffer(
			m_MemorySize,
			VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, // TODO: device only memory functionality
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			{})
	{
		for (size_t row = 0; row < m_RowCount; row++)
		{
			for (size_t col = 0; col < m_ColCount; col++)
			{
				SetValue(row, col, values[row][col]);
			}
		}
	}

	Matrix Matrix::operator+(const Matrix& other) const
	{
		// Check size
		if (m_RowCount != other.m_RowCount || m_ColCount != other.m_ColCount)
			Log::Error("Matrix addition requires equal size", true);

		// Create new matrix
		Matrix result(m_RowCount, m_ColCount);
		for (size_t row = 0; row < m_RowCount; row++)
		{
			for (size_t col = 0; col < m_ColCount; col++)
			{
				float value = GetValue(row, col) + other.GetValue(row, col);
				result.SetValue(row, col, value);
			}
		}

		return result;
	}

	Matrix Matrix::operator*(const Matrix& other) const
	{
		// Check size
		if (m_ColCount != other.m_RowCount)
			Log::Error("Matrix sizes not fit for Matrix multiplication", true);

		// Create new matrix
		Matrix result(m_RowCount, other.m_ColCount);
		
		for (size_t row = 0; row < m_RowCount; row++)
		{
			for (size_t col = 0; col < other.m_ColCount; col++)
			{
				float dotProduct = 0.0f;

				for (size_t oldCounter = 0; oldCounter < other.m_RowCount; oldCounter++)
				{
					float leftVal = GetValue(row, oldCounter);
					float rightVal = other.GetValue(oldCounter, col);
					dotProduct += leftVal * rightVal;
				}

				result.SetValue(row, col, dotProduct);
			}
		}

		return result;
	}

	void Matrix::Destroy()
	{
		m_Buffer.Destroy();
		free(m_HostMemory);
	}

	size_t Matrix::GetRowCount() const
	{
		return m_RowCount;
	}

	size_t Matrix::GetColCount() const
	{
		return m_ColCount;
	}

	size_t Matrix::GetMemorySize() const
	{
		return m_MemorySize;
	}

	const float* Matrix::GetData() const
	{
		return m_HostMemory;
	}

	size_t Matrix::GetLinearIndex(size_t row, size_t col) const
	{
		if (row >= m_RowCount || col >= m_ColCount)
			Log::Error("Matrix[row, col] is outside of matrix memory", true);

		return m_ColCount * row + col;
	}

	float Matrix::GetValue(size_t row, size_t col) const
	{
		return m_HostMemory[GetLinearIndex(row, col)];
	}

	std::string Matrix::ToString() const
	{
		std::string str = "[";

		for (size_t row = 0; row < m_RowCount; row++)
		{
			str += "[";
			
			for (size_t col = 0; col < m_ColCount; col++)
			{
				str += std::to_string(GetValue(row, col));
				str += col == m_ColCount - 1 ? "" : ", ";
			}

			str += row == m_RowCount - 1 ? "]" : "], ";
		}

		str += "]";

		return str;
	}

	void Matrix::SetValue(size_t row, size_t col, float value)
	{
		m_HostMemory[GetLinearIndex(row, col)] = value;
	}
}
