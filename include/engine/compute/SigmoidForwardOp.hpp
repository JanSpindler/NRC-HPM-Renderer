#pragma once

#include <engine/compute/Matrix.hpp>

namespace en
{
	class SigmoidForwardOp
	{
		static kp::Workgroup GetWorkgroup(const Matrix& mat);
		static const std::vector<uint32_t>& GetShaderSpirV();

	private:
		static std::string m_ShaderCode;
		static std::vector<uint32_t> m_ShaderSpirV;
		static bool m_Compiled;
	};
}
