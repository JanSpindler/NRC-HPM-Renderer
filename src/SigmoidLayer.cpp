#include <engine/compute/SigmoidLayer.hpp>

namespace en::vk
{
	SigmoidLayer::SigmoidLayer(uint32_t inputSize, uint32_t outputSize) :
		Layer(inputSize, outputSize)
	{
	}

	void SigmoidLayer::Destroy()
	{

	}

	void SigmoidLayer::Forward(Matrix* input)
	{
		for (uint32_t row = 0; row < input->GetRowCount(); row++)
		{
			for (uint32_t col = 0; col < input->GetColCount(); col++)
			{
				float oldVal = input->GetValue(row, col);
				input->SetValue(row, col, Sigmoid(oldVal));
			}
		}
	}

	void SigmoidLayer::Backprop(Matrix* totalJacobian)
	{

	}

	void SigmoidLayer::GetLocalJacobian(Matrix* localJacobian)
	{

	}

	float SigmoidLayer::Sigmoid(float x)
	{
		return 1.0f / (1.0f + std::exp(-x));
	}

	float SigmoidLayer::SigmoidDeriv(float x)
	{
		float sigmoidVal = Sigmoid(x);
		return sigmoidVal * (1.0f - sigmoidVal);
	}
}
