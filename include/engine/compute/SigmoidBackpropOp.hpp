#pragma once

#include <engine/compute/Matrix.hpp>

namespace en
{
	class SigmoidBackpropOp
	{
	public:
		static kp::Workgroup GetWorkgroup(const Matrix& oldInput, const Matrix& prevError, const Matrix& localError);
		static const std::vector<uint32_t>& GetShaderSpirV();

	private:
		static std::string m_ShaderCode;
		static std::vector<uint32_t> m_ShaderSpirV;
		static bool m_Compiled;
	};
}
