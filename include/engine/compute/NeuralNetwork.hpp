#pragma once

#include <vector>
#include <engine/compute/Layer.hpp>
#include <engine/compute/KomputeManager.hpp>

namespace en
{
	class NeuralNetwork 
	{
	public:
		NeuralNetwork(std::vector<Layer*> layers);
		~NeuralNetwork();

		Matrix Forward(KomputeManager& manager, const Matrix& input) const;
		void Backprop(KomputeManager& manager, const Matrix& input, const Matrix& target, float learningRate) const;

		void SyncLayersToDevice(KomputeManager& manager);
		void SyncLayersToHost(KomputeManager& manager);

		std::string ToString() const;

	private:
		std::vector<Layer*> m_Layers;
	};
}
