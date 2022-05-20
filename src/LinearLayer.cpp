#include <engine/compute/LinearLayer.hpp>
#include <engine/compute/LinearLayerForwardOp.hpp>
#include <engine/compute/LinearLayerBackpropOp.hpp>
#include <engine/compute/MatSetValueOp.hpp>
#include <engine/compute/MataddOp.hpp>

namespace en
{
	LinearLayer::LinearLayer(kp::Manager& manager, uint32_t inSize, uint32_t outSize) :
		Layer(manager, inSize, outSize),
		m_Weights(manager, outSize, inSize, Matrix::FillType::AllRandom),
		m_DeltaWeights(manager, outSize, inSize),
		m_Biases(manager, outSize, 1, Matrix::FillType::AllRandom)
	{
	}

	std::shared_ptr<kp::Sequence> LinearLayer::RecordSyncDevice(
		kp::Manager& manager,
		std::shared_ptr<kp::Sequence> sequence) const
	{
		std::vector<std::shared_ptr<kp::Tensor>> syncTensors = {
			m_Weights.GetTensor(),
			m_Biases.GetTensor(),
			m_Output.GetTensor(),
			m_LocalError.GetTensor(),
			m_DeltaWeights.GetTensor() };

		return sequence->record<kp::OpTensorSyncDevice>(syncTensors);
	}

	std::shared_ptr<kp::Sequence> LinearLayer::RecordSyncHost(
		kp::Manager& manager,
		std::shared_ptr<kp::Sequence> sequence) const
	{
		std::vector<std::shared_ptr<kp::Tensor>> syncTensors = {
			m_Weights.GetTensor(),
			m_Biases.GetTensor(),
			m_Output.GetTensor(),
			m_LocalError.GetTensor(),
			m_DeltaWeights.GetTensor() };

		return sequence->record<kp::OpTensorSyncLocal>(syncTensors);
	}

	std::shared_ptr<kp::Sequence> LinearLayer::RecordResetError(
		kp::Manager& manager,
		std::shared_ptr<kp::Sequence> sequence) const
	{
		std::vector<std::shared_ptr<kp::Tensor>> params = { m_LocalError.GetTensor() };
		MatSetValueOp::Config config = MatSetValueOp::GetConfig(m_LocalError, 0.0f);
		
		std::shared_ptr<kp::Algorithm> algo = manager.algorithm<float, MatSetValueOp::Config>(
			params,
			MatSetValueOp::GetShaderSpirV(),
			MatSetValueOp::GetWorkgroup(config),
			{},
			{ config });

		return sequence->record<kp::OpAlgoDispatch>(algo, std::vector<MatSetValueOp::Config>{ config });
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

		LinearLayerForwardOp::Config algoConfig = LinearLayerForwardOp::GetConfig(input, m_Weights, m_Biases, m_Output);
		
		kp::Workgroup workgroup = LinearLayerForwardOp::GetWorkgroup(algoConfig);

		std::shared_ptr<kp::Algorithm> algo = manager.algorithm<float, LinearLayerForwardOp::Config>(
			params,
			LinearLayerForwardOp::GetShaderSpirV(),
			workgroup,
			{},
			{ algoConfig });

		return sequence
			->record<kp::OpAlgoDispatch>(algo, std::vector<LinearLayerForwardOp::Config>{ algoConfig });
	}

	std::shared_ptr<kp::Sequence> LinearLayer::RecordBackprop(
		kp::Manager& manager,
		std::shared_ptr<kp::Sequence> sequence,
		const Matrix& oldInput,
		const Matrix& prevError,
		float learningRate) const
	{
		// Backprop
		std::vector<std::shared_ptr<kp::Tensor>> params = { 
			oldInput.GetTensor(),
			prevError.GetTensor(),
			m_LocalError.GetTensor(),
			m_Weights.GetTensor(),
			m_Biases.GetTensor(),
			m_DeltaWeights.GetTensor() };

		LinearLayerBackpropOp::Config backpropConfig = LinearLayerBackpropOp::GetConfig(
			oldInput,
			prevError,
			m_LocalError,
			m_Weights,
			m_Biases,
			m_DeltaWeights,
			learningRate);

		std::shared_ptr<kp::Algorithm> algo = manager.algorithm<float, LinearLayerBackpropOp::Config>(
			params, 
			LinearLayerBackpropOp::GetShaderSpirV(), 
			LinearLayerBackpropOp::GetWorkgroup(backpropConfig), 
			{}, 
			{ backpropConfig });

		sequence = sequence->record<kp::OpAlgoDispatch>(algo, std::vector<LinearLayerBackpropOp::Config>{ backpropConfig });

		// Apply delta weights
		params = { m_Weights.GetTensor(), m_DeltaWeights.GetTensor(), m_Weights.GetTensor() };

		MataddOp::Config mataddConfig = MataddOp::GetConfig(m_Weights, m_DeltaWeights);

		algo = manager.algorithm<float, MataddOp::Config>(
			params, 
			MataddOp::GetShaderSpirV(), 
			MataddOp::GetWorkgroup(mataddConfig), 
			{}, 
			{ mataddConfig });

		return sequence->record<kp::OpAlgoDispatch>(algo, std::vector<MataddOp::Config>{ mataddConfig });
	}
}
