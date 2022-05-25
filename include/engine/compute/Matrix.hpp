#pragma once

#include <kompute/Kompute.hpp>
#include <string>
#include <engine/compute/KomputeManager.hpp>

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
			AllRandom = 3,
		};

		Matrix(KomputeManager& manager, uint32_t rowCount, uint32_t colCount, FillType fillType = FillType::None, float value = 0.0f);
		Matrix(KomputeManager& manager, const std::vector<std::vector<float>> values);

		uint32_t GetRowCount() const;
		uint32_t GetColCount() const;
		std::shared_ptr<kp::Tensor> GetTensor() const;
		std::vector<float> GetDataVector() const;
		std::string ToString() const;

		bool IsRowVector() const;
		bool IsColVector() const;

	private:
		uint32_t m_RowCount;
		uint32_t m_ColCount;
		uint32_t m_ElementCount;
		uint32_t m_DataSize;
		std::shared_ptr<kp::Tensor> m_Tensor;

		uint32_t GetLinearIndex(uint32_t row, uint32_t col) const;
	};
}
