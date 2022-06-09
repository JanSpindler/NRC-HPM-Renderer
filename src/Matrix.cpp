#include <engine/compute/Matrix.hpp>
#include <engine/util/Log.hpp>
#include <random>

namespace en
{
	Matrix::Matrix(KomputeManager& manager, uint32_t rowCount, uint32_t colCount, FillType fillType, float value) :
		m_RowCount(rowCount),
		m_ColCount(colCount),
		m_ElementCount(rowCount * colCount),
		m_DataSize(m_ElementCount * sizeof(float))
	{
		std::vector<float> data(m_ElementCount);

		if (fillType == FillType::Diagonal)
		{
			for (uint32_t row = 0; row < m_RowCount; row++)
			{
				for (uint32_t col = 0; col < m_ColCount; col++)
				{
					data[GetLinearIndex(row, col)] = row == col ? value : 0.0f;
				}
			}
		}
		else if (fillType == FillType::All)
		{
			for (uint32_t i = 0; i < m_ElementCount; i++)
			{
				data[i] = value;
			}
		}
		else if (fillType == FillType::AllRandom)
		{
			std::default_random_engine generator((std::random_device()()));
			std::normal_distribution<float> distribution(0.0f, std::pow(static_cast<float>(m_RowCount), -0.5f));
			float norm = static_cast<float>(m_ColCount);

			for (uint32_t row = 0; row < m_RowCount; row++)
			{
				for (uint32_t col = 0; col < m_ColCount; col++)
				{
					data[GetLinearIndex(row, col)] = distribution(generator);// / norm;
				}
			}
		}

		m_Tensor = manager.tensor(data);
	}

	Matrix::Matrix(KomputeManager& manager, const std::vector<std::vector<float>> values) :
		m_RowCount(values.size()),
		m_ColCount(values[0].size()),
		m_ElementCount(m_RowCount * m_ColCount),
		m_DataSize(m_ElementCount * sizeof(float))
	{
		std::vector<float> data(m_ElementCount);

		for (uint32_t row = 0; row < m_RowCount; row++)
		{
			for (uint32_t col = 0; col < m_ColCount; col++)
			{
				data[GetLinearIndex(row, col)] = values[row][col];
			}
		}

		m_Tensor = manager.tensor(data);
	}

	uint32_t Matrix::GetRowCount() const
	{
		return m_RowCount;
	}

	uint32_t Matrix::GetColCount() const
	{
		return m_ColCount;
	}

	std::shared_ptr<kp::Tensor> Matrix::GetTensor() const
	{
		return m_Tensor;
	}

	std::vector<float> Matrix::GetDataVector() const
	{
		if (m_Tensor->isInit())
			return m_Tensor->vector<float>();
		return {};
	}

	std::string Matrix::ToString() const
	{
		std::vector<float> data = GetDataVector();

		std::string str = "[";
		for (uint32_t row = 0; row < m_RowCount; row++)
		{
			str += "[";
			for (uint32_t col = 0; col < m_ColCount; col++)
			{
				str += std::to_string(data[GetLinearIndex(row, col)]);
				
				if (col < m_ColCount - 1)
					str += ", ";
			}
			str += "]";

			if (row < m_RowCount - 1)
				str += ",\n";
		}
		str += "]";

		return str;
	}

	bool Matrix::IsRowVector() const
	{
		return m_RowCount == 1;
	}

	bool Matrix::IsColVector() const
	{
		return m_ColCount == 1;
	}

	uint32_t Matrix::GetLinearIndex(uint32_t row, uint32_t col) const
	{
		if (row >= m_RowCount || col >= m_ColCount)
			Log::Error("row or col too large for this Matrix", true);

		return row * m_ColCount + col;
	}
}
