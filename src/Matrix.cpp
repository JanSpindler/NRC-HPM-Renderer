#include <engine/compute/Matrix.hpp>
#include <engine/util/Log.hpp>

namespace en
{
	Matrix::Matrix(kp::Manager& manager, uint32_t rowCount, uint32_t colCount, FillType fillType, float value) :
		m_RowCount(rowCount),
		m_ColCount(colCount),
		m_ElementCount(rowCount * colCount),
		m_DataSize(m_ElementCount * sizeof(float)),
		m_Data(reinterpret_cast<float*>(malloc(m_DataSize)))
	{
		if (fillType == FillType::Diagonal)
		{
			for (uint32_t row = 0; row < m_RowCount; row++)
			{
				for (uint32_t col = 0; col < m_ColCount; col++)
				{
					float setValue = row == col ? value : 0.0f;
					SetValue(row, col, setValue);
				}
			}
		}
		else if (fillType == FillType::All)
		{
			for (uint32_t i = 0; i < m_ElementCount; i++)
			{
				m_Data[i] = value;
			}
		}

		m_Tensor = manager.tensor(m_Data, m_ElementCount, sizeof(float), kp::Tensor::TensorDataTypes::eFloat);
	}

	Matrix::Matrix(kp::Manager& manager, const std::vector<std::vector<float>> values) :
		m_RowCount(values.size()),
		m_ColCount(values[0].size()),
		m_ElementCount(m_RowCount * m_ColCount),
		m_DataSize(m_ElementCount * sizeof(float)),
		m_Data(reinterpret_cast<float*>(malloc(m_DataSize)))
	{
		for (uint32_t row = 0; row < m_RowCount; row++)
		{
			for (uint32_t col = 0; col < m_ColCount; col++)
			{
				SetValue(row, col, values[row][col]);
			}
		}

		m_Tensor = manager.tensor(m_Data, m_ElementCount, sizeof(float), kp::Tensor::TensorDataTypes::eFloat);
	}

	Matrix::~Matrix()
	{
		if (m_Data != nullptr)
		{
			delete m_Data;
			m_Data = nullptr;
		}
	}

	void Matrix::SyncTensorToMatrix()
	{
		memcpy(m_Data, m_Tensor->rawData(), m_DataSize);
	}

	uint32_t Matrix::GetRowCount() const
	{
		return m_RowCount;
	}

	uint32_t Matrix::GetColCount() const
	{
		return m_ColCount;
	}

	std::shared_ptr<kp::Tensor> Matrix::GetTensor()
	{
		return m_Tensor;
	}

	std::vector<float> Matrix::GetDataVector() const
	{
		// TODO
		return m_Tensor->vector<float>();
		//return { m_Data, m_Data + m_ElementCount };
	}

	std::string Matrix::ToString() const
	{
		std::string str = "[";
		for (uint32_t row = 0; row < m_RowCount; row++)
		{
			str += "[";
			for (uint32_t col = 0; col < m_ColCount; col++)
			{
				str += std::to_string(GetValue(row, col));
				
				if (col < m_ColCount - 1)
					str += ", ";
			}
			str += "]";

			if (row < m_RowCount - 1)
				str += ",\n";
		}

		return str;
	}

	uint32_t Matrix::GetLinearIndex(uint32_t row, uint32_t col) const
	{
		if (row >= m_RowCount || col >= m_ColCount)
			Log::Error("row or col too large for this Matrix", true);

		return row * m_ColCount + col;
	}

	float Matrix::GetValue(uint32_t row, uint32_t col) const
	{
		return m_Data[GetLinearIndex(row, col)];
	}

	void Matrix::SetValue(uint32_t row, uint32_t col, float value)
	{
		m_Data[GetLinearIndex(row, col)] = value;
	}
}
