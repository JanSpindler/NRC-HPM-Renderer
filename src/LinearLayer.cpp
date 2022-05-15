#include <engine/compute/LinearLayer.hpp>
#include <engine/compute/LinearLayerForwardOp.hpp>

namespace en
{
	LinearLayer::LinearLayer(kp::Manager& manager, uint32_t inSize, uint32_t outSize) :
		Layer(manager, inSize, outSize),
		m_Weights(manager, outSize, inSize, Matrix::FillType::AllRandom),
		m_Biases(manager, outSize, 1, Matrix::FillType::AllRandom)
	{
	}

	std::shared_ptr<kp::Sequence> LinearLayer::RecordForward(
		kp::Manager& manager,
		std::shared_ptr<kp::Sequence> sequence,
		const Matrix& input) const
	{
		std::vector<std::shared_ptr<kp::Tensor>> params = { 
			input.GetTensor(), 
			m_Weights.GetTensor(), 
			m_Biases.GetTensor(), 
			m_Output.GetTensor() };
		std::vector<std::shared_ptr<kp::Tensor>> syncTensors = { params.begin() + 1, params.end() };

		LinearLayerForwardOp::Config algoConfig = LinearLayerForwardOp::GetConfig(input, m_Weights, m_Biases, m_Output);
		
		kp::Workgroup workgroup = LinearLayerForwardOp::GetWorkgroup(algoConfig);

		std::shared_ptr<kp::Algorithm> algo = manager.algorithm<float, LinearLayerForwardOp::Config>(
			params,
			LinearLayerForwardOp::GetShaderSpirV(),
			workgroup,
			{},
			{ algoConfig });

		return sequence
			->record<kp::OpTensorSyncDevice>(syncTensors)
			->record<kp::OpAlgoDispatch>(algo, std::vector<LinearLayerForwardOp::Config>{ algoConfig });
	}
}
