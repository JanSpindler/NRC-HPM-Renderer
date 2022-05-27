#pragma once

#include <engine/compute/Matrix.hpp>
#include <engine/compute/KomputeManager.hpp>

namespace en
{
	class Dataset
	{
	public:
		static Dataset FromMnist(KomputeManager& manager, const std::vector<std::vector<uint8_t>>& inputs, const std::vector<uint8_t>& labels);

		Dataset(const std::vector<Matrix>& inputs, const std::vector<Matrix>& targets);

		size_t GetSize() const;

		const Matrix& GetInput(size_t index) const;
		
		const Matrix& GetTarget(size_t index) const;

	private:
		std::vector<Matrix> m_Inputs;
		std::vector<Matrix> m_Targets;
	};
}
