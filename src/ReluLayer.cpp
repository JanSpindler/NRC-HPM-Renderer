#include <engine/compute/ReluLayer.hpp>
#include <engine/compute/ReluForwardOp.hpp>
#include <engine/compute/ReluBackpropOp.hpp>

namespace en
{
	ReluLayer::ReluLayer(KomputeManager& manager, uint32_t size) :
		Layer(manager, size, size)
	{
	}

	std::shared_ptr<kp::Sequence> ReluLayer::RecordSyncDevice(
		KomputeManager& manager,
		std::shared_ptr<kp::Sequence> sequence) const
	{
		std::vector<std::shared_ptr<kp::Tensor>> syncTensors = {
			m_Output.GetTensor() };

		return sequence->record<kp::OpTensorSyncDevice>(syncTensors);
	}

	std::shared_ptr<kp::Sequence> ReluLayer::RecordSyncHost(
		KomputeManager& manager,
		std::shared_ptr<kp::Sequence> sequence) const
	{
		std::vector<std::shared_ptr<kp::Tensor>> syncTensors = {
			m_Output.GetTensor() };

		return sequence->record<kp::OpTensorSyncLocal>(syncTensors);
	}

	std::shared_ptr<kp::Sequence> ReluLayer::RecordResetError(
		KomputeManager& manager,
		std::shared_ptr<kp::Sequence> sequence) const
	{
		return sequence;
	};

	std::shared_ptr<kp::Sequence> ReluLayer::RecordForward(
		KomputeManager& manager,
		std::shared_ptr<kp::Sequence> sequence,
		const Matrix& input) const
	{
		std::vector<std::shared_ptr<kp::Tensor>> params = { input.GetTensor(), m_Output.GetTensor() };

		kp::Workgroup workgroup = ReluForwardOp::GetWorkgroup(input, m_Output);

		std::shared_ptr<kp::Algorithm> algo = manager.algorithm(
			params,
			ReluForwardOp::GetShaderSpirV(),
			workgroup,
			{},
			{});

		return sequence->record<kp::OpAlgoDispatch>(algo);
	}

	std::shared_ptr<kp::Sequence> ReluLayer::RecordBackprop(
		KomputeManager& manager,
		std::shared_ptr<kp::Sequence> sequence,
		const Matrix& oldInput,
		const Matrix& prevError,
		float learningRate) const
	{
		std::vector<std::shared_ptr<kp::Tensor>> params = {
			oldInput.GetTensor(),
			prevError.GetTensor(),
			m_LocalError.GetTensor() };

		kp::Workgroup workgroup = ReluBackpropOp::GetWorkgroup(oldInput, prevError, m_LocalError);

		std::shared_ptr<kp::Algorithm> algo = manager.algorithm(
			params,
			ReluBackpropOp::GetShaderSpirV(),
			workgroup,
			{},
			{});

		return sequence->record<kp::OpAlgoDispatch>(algo);
	}
}
