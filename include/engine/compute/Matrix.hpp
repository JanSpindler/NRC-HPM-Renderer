#pragma once

#include <kompute/Kompute.hpp>
#include <string>

namespace en
{
	class Matrix
	{
	public:
		enum class FillType
		{
			None = 0,
			Diagonal = 1,
			All = 2,
		};

		Matrix(kp::Manager& manager, uint32_t rowCount, uint32_t colCount, FillType fillType = FillType::None, float value = 0.0f);
		Matrix(kp::Manager& manager, const std::vector<std::vector<float>> values);
		~Matrix();

		void SyncTensorToMatrix();

		uint32_t GetRowCount() const;
		uint32_t GetColCount() const;
		std::shared_ptr<kp::Tensor> GetTensor();
		std::vector<float> GetDataVector() const;
		std::string ToString() const;

	private:
		uint32_t m_RowCount;
		uint32_t m_ColCount;
		uint32_t m_ElementCount;
		uint32_t m_DataSize;
		float* m_Data = nullptr;
		std::shared_ptr<kp::Tensor> m_Tensor;

		uint32_t GetLinearIndex(uint32_t row, uint32_t col) const;
		float GetValue(uint32_t row, uint32_t col) const;
		void SetValue(uint32_t row, uint32_t col, float value);
	};
}
