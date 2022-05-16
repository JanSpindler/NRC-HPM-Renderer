#include <engine/compute/Layer.hpp>

namespace en
{
	Layer::Layer(kp::Manager& manager, uint32_t inSize, uint32_t outSize) :
		m_Output(manager, outSize, 1),
		m_TotalJacobian(manager, inSize, outSize)
	{
	}

	const Matrix& Layer::GetOutput() const
	{
		return m_Output;
	}

	const Matrix& Layer::GetTotalJacobian() const
	{
		return m_TotalJacobian;
	}
}
