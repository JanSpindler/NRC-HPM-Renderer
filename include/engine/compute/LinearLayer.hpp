#pragma once

#include <engine/compute/Matrix.hpp>
#include <kompute/Kompute.hpp>
#include <engine/compute/Layer.hpp>
#include <engine/compute/KomputeManager.hpp>

namespace en
{
	class LinearLayer : public Layer
	{
	public:
		LinearLayer(KomputeManager& manager, uint32_t inSize, uint32_t outSize);

		virtual std::shared_ptr<kp::Sequence> RecordSyncDevice(
			KomputeManager& manager,
			std::shared_ptr<kp::Sequence> sequence) const override;

		virtual std::shared_ptr<kp::Sequence> RecordSyncHost(
			KomputeManager& manager,
			std::shared_ptr<kp::Sequence> sequence) const override;

		virtual std::shared_ptr<kp::Sequence> RecordResetError(
			KomputeManager& manager,
			std::shared_ptr<kp::Sequence> sequence) const override;

		virtual std::shared_ptr<kp::Sequence> RecordForward(
			KomputeManager& manager,
			std::shared_ptr<kp::Sequence> sequence,
			const Matrix& input) const override;

		virtual std::shared_ptr<kp::Sequence> RecordBackprop(
			KomputeManager& manager,
			std::shared_ptr<kp::Sequence> sequence,
			const Matrix& oldInput,
			const Matrix& prevError,
			float learningRate) const override;

		const Matrix& GetWeights() const;

	private:
		Matrix m_Weights;
		Matrix m_DeltaWeights;
		Matrix m_Biases;
	};
}
