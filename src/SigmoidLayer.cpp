#include <engine/compute/SigmoidLayer.hpp>
#include <engine/compute/SigmoidForwardOp.hpp>
#include <engine/compute/SigmoidBackpropOp.hpp>

namespace en
{
	SigmoidLayer::SigmoidLayer(kp::Manager& manager, uint32_t size) :
		Layer(manager, size, size)
	{
	}

	std::shared_ptr<kp::Sequence> SigmoidLayer::RecordSyncDevice(
		kp::Manager& manager,
		std::shared_ptr<kp::Sequence> sequence) const
	{
		std::vector<std::shared_ptr<kp::Tensor>> syncTensors = {
			m_Output.GetTensor() };

		return sequence->record<kp::OpTensorSyncDevice>(syncTensors);
	}

	std::shared_ptr<kp::Sequence> SigmoidLayer::RecordForward(
		kp::Manager& manager,
		std::shared_ptr<kp::Sequence> sequence,
		const Matrix& input) const
	{
		std::vector<std::shared_ptr<kp::Tensor>> params = { input.GetTensor(), m_Output.GetTensor() };

		kp::Workgroup workgroup = SigmoidForwardOp::GetWorkgroup(input, m_Output);

		std::shared_ptr<kp::Algorithm> algo = manager.algorithm(
			params,
			SigmoidForwardOp::GetShaderSpirV(),
			workgroup,
			{},
			{});

		return sequence->record<kp::OpAlgoDispatch>(algo);
	}

	std::shared_ptr<kp::Sequence> SigmoidLayer::RecordBackprop(
		kp::Manager& manager,
		std::shared_ptr<kp::Sequence> sequence,
		const Matrix& oldInput,
		const Matrix& prevError,
		float learningRate) const
	{
		std::vector<std::shared_ptr<kp::Tensor>> params = {
			oldInput.GetTensor(),
			prevError.GetTensor(),
			m_LocalError.GetTensor() };

		kp::Workgroup workgroup = SigmoidBackpropOp::GetWorkgroup(oldInput, prevError, m_LocalError);

		std::shared_ptr<kp::Algorithm> algo = manager.algorithm(
			params,
			SigmoidForwardOp::GetShaderSpirV(),
			workgroup,
			{},
			{});

		return sequence->record<kp::OpAlgoDispatch>(algo);
	}
}
