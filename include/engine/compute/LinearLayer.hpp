#pragma once

#include <engine/compute/Matrix.hpp>
#include <kompute/Kompute.hpp>
#include <engine/compute/Layer.hpp>

namespace en
{
	class LinearLayer : public Layer
	{
	public:
		LinearLayer(kp::Manager& manager, uint32_t inSize, uint32_t outSize);

		virtual std::shared_ptr<kp::Sequence> record(
			kp::Manager& manager,
			std::shared_ptr<kp::Sequence> sequence,
			const Matrix& input,
			const Matrix& output) const override;

	private:
		Matrix m_Weights;
		Matrix m_Biases;
	};
}
