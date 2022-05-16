#pragma once

#include <engine/compute/Layer.hpp>

namespace en
{
	class SigmoidLayer : public Layer
	{
	public:
		SigmoidLayer(kp::Manager& manager, uint32_t size);

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
	};
}
