#include <engine/compute/SigmoidLayer.hpp>
#include <engine/compute/SigmoidForwardOp.hpp>

namespace en
{
	SigmoidLayer::SigmoidLayer(kp::Manager& manager, uint32_t size) :
		Layer(manager, size, size)
	{
	}

	std::shared_ptr<kp::Sequence> SigmoidLayer::RecordForward(
		kp::Manager& manager,
		std::shared_ptr<kp::Sequence> sequence,
		const Matrix& input) const
	{
		std::vector<std::shared_ptr<kp::Tensor>> params = { input.GetTensor(), m_Output.GetTensor() };
		std::vector<std::shared_ptr<kp::Tensor>> syncTensors = { params.begin() + 1, params.end() };

		kp::Workgroup workgroup = SigmoidForwardOp::GetWorkgroup(input, m_Output);

		std::shared_ptr<kp::Algorithm> algo = manager.algorithm(
			params,
			SigmoidForwardOp::GetShaderSpirV(),
			workgroup,
			{},
			{});

		return sequence
			->record<kp::OpTensorSyncDevice>(syncTensors)
			->record<kp::OpAlgoDispatch>(algo);
	}
}
