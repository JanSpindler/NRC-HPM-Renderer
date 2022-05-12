#include <engine/compute/LinearLayer.hpp>

namespace en::vk
{
	LinearLayer::LinearLayer(uint32_t inputSize, uint32_t outputSize) :
		Layer(inputSize, outputSize)
	{
	}

	void LinearLayer::Destroy()
	{

	}

	void LinearLayer::Forward(Matrix* input)
	{

	}

	void LinearLayer::Backprop(Matrix* totalJacobian)
	{

	}

	void LinearLayer::GetLocalJacobian(Matrix* localJacobian)
	{
		// Jacobian of linear layer is the weight matrix
	}
}
