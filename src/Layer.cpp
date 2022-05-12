#include <engine/compute/Layer.hpp>

namespace en::vk
{
	Layer::Layer(uint32_t inputSize, uint32_t outputSize) :
		m_InputSize(inputSize),
		m_OutputSize(outputSize)
	{
	}

	uint32_t Layer::GetInputSize() const
	{
		return m_InputSize;
	}

	uint32_t Layer::GetOutputSize() const
	{
		return m_OutputSize;
	}
}
