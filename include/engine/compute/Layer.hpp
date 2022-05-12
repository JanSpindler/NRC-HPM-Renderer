#pragma once

#include <engine/compute/Matrix.hpp>

namespace en::vk
{
	class Layer
	{
	public:
		Layer(uint32_t inputSize, uint32_t outputSize);

		virtual void Destroy() = 0;

		virtual void Forward(Matrix* input) = 0;
		virtual void Backprop(Matrix* totalJacobian) = 0;

		uint32_t GetInputSize() const;
		uint32_t GetOutputSize() const;

	protected:
		uint32_t m_InputSize;
		uint32_t m_OutputSize;

		virtual void GetLocalJacobian(Matrix* localJacobian) = 0;
	};
}
