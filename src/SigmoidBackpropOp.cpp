#include <engine/compute/SigmoidBackpropOp.hpp>
#include <engine/util/compile_shader.hpp>

namespace en
{
	std::string SigmoidBackpropOp::m_ShaderCode(R"(
	
	)");

	std::vector<uint32_t> SigmoidBackpropOp::m_ShaderSpirV;
	bool SigmoidBackpropOp::m_Compiled = false;

	SigmoidBackpropOp::Config SigmoidBackpropOp::GetConfig(const Matrix&)
	{
		return { 0 };
	}

	kp::Workgroup SigmoidBackpropOp::GetWorkgroup(const SigmoidBackpropOp::Config& config)
	{
		return kp::Workgroup({ config.size, 1, 1 });
	}

	const std::vector<uint32_t>& SigmoidBackpropOp::GetShaderSpirV()
	{
		if (!m_Compiled)
		{
			m_ShaderSpirV = CompileShader(m_ShaderCode);
			m_Compiled = true;
		}

		return m_ShaderSpirV;
	}
}
