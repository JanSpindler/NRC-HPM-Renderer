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

		virtual std::shared_ptr<kp::Sequence> RecordSyncDevice(
			kp::Manager& manager,
			std::shared_ptr<kp::Sequence> sequence) const override;

		virtual std::shared_ptr<kp::Sequence> RecordResetError(
			kp::Manager& manager,
			std::shared_ptr<kp::Sequence> sequence) const override;

		virtual std::shared_ptr<kp::Sequence> RecordForward(
			kp::Manager& manager,
			std::shared_ptr<kp::Sequence> sequence,
			const Matrix& input) const override;

		virtual std::shared_ptr<kp::Sequence> RecordBackprop(
			kp::Manager& manager,
			std::shared_ptr<kp::Sequence> sequence,
			const Matrix& oldInput,
			const Matrix& prevError,
			float learningRate) const override;

	private:
		Matrix m_Weights;
		Matrix m_DeltaWeights;
		Matrix m_Biases;
	};
}
