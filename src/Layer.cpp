#include <engine/compute/Layer.hpp>

namespace en
{
	Layer::Layer(kp::Manager& manager, uint32_t inSize, uint32_t outSize) :
		m_Output(manager, outSize, 1),
		m_LocalError(manager, outSize, 1)
	{
	}

	const Matrix& Layer::GetOutput() const
	{
		return m_Output;
	}

	const Matrix& Layer::GetLocalError() const
	{
		return m_LocalError;
	}
}
