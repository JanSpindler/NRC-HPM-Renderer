#pragma once

#include <engine/compute/Layer.hpp>

namespace en::vk
{
	class LinearLayer : public Layer
	{
	public:
		LinearLayer(uint32_t inputSize, uint32_t outputSize);

		void Destroy() override;

		void Forward(Matrix* input) override;
		void Backprop(Matrix* totalJacobian) override;

	private:
		void GetLocalJacobian(Matrix* localJacobian) override;
	};
}
