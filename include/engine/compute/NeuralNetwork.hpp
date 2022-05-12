#pragma once

#include <vector>
#include <engine/compute/Layer.hpp>

namespace en::vk
{
	class NeuralNetwork
	{
	public:
		NeuralNetwork(const std::vector<Layer*>& layers);

		void Destroy();

		Matrix Forward(const Matrix& input);
		void Backprop(const Matrix& target);

	private:
		const std::vector<Layer*>& m_Layers;
	};
}
