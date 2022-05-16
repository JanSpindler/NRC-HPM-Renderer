#pragma once

#include <engine/compute/Matrix.hpp>

namespace en
{
	class Layer
	{
	public:
		Layer(kp::Manager& manager, uint32_t inSize, uint32_t outSize);

		virtual std::shared_ptr<kp::Sequence> RecordSyncDevice(
			kp::Manager& manager,
			std::shared_ptr<kp::Sequence> sequence) const = 0;

		virtual std::shared_ptr<kp::Sequence> RecordResetError(
			kp::Manager& manager,
			std::shared_ptr<kp::Sequence> sequence) const = 0;

		virtual std::shared_ptr<kp::Sequence> RecordForward(
			kp::Manager& manager,
			std::shared_ptr<kp::Sequence> sequence,
			const Matrix& input) const = 0;

		virtual std::shared_ptr<kp::Sequence> RecordBackprop(
			kp::Manager& manager,
			std::shared_ptr<kp::Sequence> sequence,
			const Matrix& oldInput,
			const Matrix& prevError,
			float learningRate) const = 0;

		const Matrix& GetOutput() const;

		const Matrix& GetLocalError() const;

	protected:
		Matrix m_Output;
		Matrix m_LocalError;
	};
}
