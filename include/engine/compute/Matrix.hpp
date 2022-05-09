#pragma once

#include <engine/graphics/vulkan/Buffer.hpp>
#include <string>
#include <vector>

namespace en::vk
{
	class Matrix
	{
	public:
		Matrix(size_t rowCount, size_t colCount, float diagonal = NAN);
		Matrix(const std::vector<std::vector<float>>& values);

		Matrix operator+(const Matrix& other) const;
		Matrix operator*(const Matrix& other) const;

		void Destroy();

		size_t GetRowCount() const;
		size_t GetColCount() const;
		size_t GetMemorySize() const;
		size_t GetLinearIndex(size_t row, size_t col) const;
		float GetValue(size_t row, size_t col) const;
		const float* GetData() const;
		std::string ToString() const;

		void SetValue(size_t row, size_t col, float value);

	private:
		size_t m_RowCount;
		size_t m_ColCount;
		size_t m_MemorySize;
		float* m_HostMemory;
		vk::Buffer m_Buffer;
	};
}
