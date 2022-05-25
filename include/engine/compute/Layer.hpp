#pragma once

#include <engine/compute/Matrix.hpp>
#include <engine/compute/KomputeManager.hpp>

namespace en
{
	class Layer
	{
	public:
		Layer(KomputeManager& manager, uint32_t inSize, uint32_t outSize);

		virtual std::shared_ptr<kp::Sequence> RecordSyncDevice(
			KomputeManager& manager,
			std::shared_ptr<kp::Sequence> sequence) const = 0;

		virtual std::shared_ptr<kp::Sequence> RecordSyncHost(
			KomputeManager& manager,
			std::shared_ptr<kp::Sequence> sequence) const = 0;

		virtual std::shared_ptr<kp::Sequence> RecordResetError(
			KomputeManager& manager,
			std::shared_ptr<kp::Sequence> sequence) const = 0;

		virtual std::shared_ptr<kp::Sequence> RecordForward(
			KomputeManager& manager,
			std::shared_ptr<kp::Sequence> sequence,
			const Matrix& input) const = 0;

		virtual std::shared_ptr<kp::Sequence> RecordBackprop(
			KomputeManager& manager,
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
