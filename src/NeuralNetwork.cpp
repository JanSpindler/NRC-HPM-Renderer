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

	void NeuralNetwork::Backprop(kp::Manager& manager, const Matrix& input, const Matrix& target) const
	{
	}
}
