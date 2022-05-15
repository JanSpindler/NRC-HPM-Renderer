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
		std::shared_ptr<kp::Sequence> sequence = manager.sequence();
		for (uint32_t i = 0; i < m_Layers.size(); i++)
		{
			Layer* layer = m_Layers[i];
			//sequence = layer->record(manager)
		}

		return Matrix(manager, 1, 1);
	}

	void NeuralNetwork::Backprop(kp::Manager& manager, const Matrix& input, const Matrix& target) const
	{
	}
}
