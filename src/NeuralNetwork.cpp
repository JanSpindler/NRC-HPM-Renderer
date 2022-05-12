#include <engine/compute/NeuralNetwork.hpp>

namespace en::vk
{
	NeuralNetwork::NeuralNetwork(const std::vector<Layer*>& layers) :
		m_Layers(layers)
	{
	}

	void NeuralNetwork::Destroy()
	{
		for (Layer* layer : m_Layers)
		{
			layer->Destroy();
			delete layer;
		}
	}

	Matrix NeuralNetwork::Forward(const Matrix& input)
	{
		return Matrix(1, 1, 0.0f);
	}

	void NeuralNetwork::Backprop(const Matrix& target)
	{

	}
}
