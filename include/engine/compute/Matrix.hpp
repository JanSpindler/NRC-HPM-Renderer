#pragma once

#include <engine/graphics/vulkan/Buffer.hpp>
#include <string>
#include <vector>

namespace en::vk
{
	class Matrix
	{
	public:
		Matrix(uint32_t rowCount, uint32_t colCount, float diagonal = NAN);
		Matrix(const std::vector<std::vector<float>>& values);

		Matrix operator+(const Matrix& other) const;
		Matrix operator*(const Matrix& other) const;

		void Destroy();

		void CopyToDevice();
		void CopyToHost();

		uint32_t GetRowCount() const;
		uint32_t GetColCount() const;
		uint32_t GetMemorySize() const;
		uint32_t GetLinearIndex(uint32_t row, uint32_t col) const;
		float GetValue(uint32_t row, uint32_t col) const;
		const float* GetData() const;
		std::string ToString() const;
		VkBuffer GetBufferVulkanHandle() const;

		void SetValue(uint32_t row, uint32_t col, float value);

	private:
		uint32_t m_RowCount;
		uint32_t m_ColCount;
		uint32_t m_MemorySize;
		float* m_HostMemory;
		vk::Buffer m_Buffer;
	};
}
