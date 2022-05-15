#pragma once

#include <vector>
#include <engine/compute/Layer.hpp>

namespace en
{
	class NeuralNetwork 
	{
	public:
		NeuralNetwork(std::vector<Layer*> layers);
		~NeuralNetwork();

		Matrix Forward(kp::Manager& manager, const Matrix& input) const;
		void Backprop(kp::Manager& manager, const Matrix& input, const Matrix& target) const;

	private:
		std::vector<Layer*> m_Layers;
	};
}
