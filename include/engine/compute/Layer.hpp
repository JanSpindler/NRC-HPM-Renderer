#pragma once

#include <engine/compute/Matrix.hpp>

namespace en
{
	class Layer
	{
	public:
		Layer(kp::Manager& manager, uint32_t inSize, uint32_t outSize);

		virtual std::shared_ptr<kp::Sequence> record(
			kp::Manager& manager,
			std::shared_ptr<kp::Sequence> sequence,
			const Matrix& input,
			const Matrix& output) const = 0;

	protected:
		Matrix m_TotalJacobian;
	};
}
