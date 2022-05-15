#pragma once

#include <engine/compute/Matrix.hpp>

namespace en
{
	class SigmoidForwardOp
	{
	public:
		static kp::Workgroup GetWorkgroup(const Matrix& input, const Matrix& output);
		static const std::vector<uint32_t>& GetShaderSpirV();

	private:
		static std::string m_ShaderCode;
		static std::vector<uint32_t> m_ShaderSpirV;
		static bool m_Compiled;
	};
}
