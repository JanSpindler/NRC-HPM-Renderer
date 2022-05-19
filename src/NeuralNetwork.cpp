#include <engine/compute/NeuralNetwork.hpp>

namespace en
{
	NeuralNetwork::NeuralNetwork(std::vector<Layer*> layers) :
		m_Layers(layers)
	{
	}

	NeuralNetwork::~NeuralNetwork()
	{
		for (Layer* layer : m_Layers)
		{
			delete layer;
		}
	}

	Matrix NeuralNetwork::Forward(kp::Manager& manager, const Matrix& input) const
	{
		//
		std::shared_ptr<kp::Sequence> sequence = manager.sequence();

		// Sync tensors to device
		sequence = sequence->record<kp::OpTensorSyncDevice>(std::vector<std::shared_ptr<kp::Tensor>>{input.GetTensor()});

		for (uint32_t i = 0; i < m_Layers.size(); i++)
		{
			sequence = m_Layers[i]->RecordSyncDevice(manager, sequence);
		}

		// Forward
		Matrix currentInput = input;

		for (uint32_t i = 0; i < m_Layers.size(); i++)
		{
			Layer* layer = m_Layers[i];
			sequence = layer->RecordForward(manager, sequence, currentInput);
			currentInput = layer->GetOutput();
		}
		
		// Sync output tensor to host and eval
		sequence
			->record<kp::OpTensorSyncLocal>({ currentInput.GetTensor() })
			->eval();

		//
		return currentInput;
	}

	void NeuralNetwork::Backprop(kp::Manager& manager, const Matrix& input, const Matrix& target, float learningRate) const
	{
		//
		Matrix output = Forward(manager, input);

		//
		std::shared_ptr<kp::Sequence> sequence = manager.sequence();

		// Calculate error
		std::vector<float> outputVec = output.GetDataVector();
		std::vector<float> targetVec = target.GetDataVector();
		std::vector<std::vector<float>> errorVec(outputVec.size());
		
		for (uint32_t i = 0; i < target.GetRowCount(); i++)
		{
			float absError = targetVec[i] - outputVec[i];
			errorVec[i] = { absError * absError };
		}

		Matrix error(manager, errorVec);

		// Sync tensors
		sequence = sequence->record<kp::OpTensorSyncDevice>(
			std::vector<std::shared_ptr<kp::Tensor>> { error.GetTensor(), target.GetTensor() });

		// Backprop
		for (size_t i = m_Layers.size() - 1; i > 0 ; i--)
		{
			Layer* layer = m_Layers[i];
			Layer* nextLayer = m_Layers[i - 1];

			const Matrix& prevError = i == m_Layers.size() - 1 ? error : m_Layers[i + 1]->GetLocalError();

			sequence = layer->RecordResetError(manager, sequence);
			sequence = layer->RecordBackprop(manager, sequence, nextLayer->GetOutput(), prevError, learningRate);
		}

		sequence = m_Layers[0]->RecordBackprop(manager, sequence, input, m_Layers[1]->GetLocalError(), learningRate)->eval();
	}
}
