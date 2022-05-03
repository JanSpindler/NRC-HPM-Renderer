#pragma once

#include <engine/graphics/vulkan/Buffer.hpp>

namespace en::vk
{
	class Matrix
	{
	public:
		Matrix(size_t rowCount, size_t colCount);

		size_t GetRowCount() const;
		size_t GetColCount() const;

	private:
		size_t m_RowCount;
		size_t m_ColCount;
	};
}
