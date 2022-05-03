#include <engine/compute/Matrix.hpp>

namespace en::vk
{
	Matrix::Matrix(size_t rowCount, size_t colCount) :
		m_RowCount(rowCount),
		m_ColCount(colCount)
	{
	}

	size_t Matrix::GetRowCount() const
	{
		return m_RowCount;
	}

	size_t Matrix::GetColCount() const
	{
		return m_ColCount;
	}
}
