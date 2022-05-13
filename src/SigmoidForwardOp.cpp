#include <engine/compute/SigmoidForwardOp.hpp>
#include <engine/util/compile_shader.hpp>
#include <engine/util/Log.hpp>

namespace en
{
	std::string SigmoidForwardOp::m_ShaderCode(R"(
		// The version to use 
		#version 450

		// The buffers are provided via the tensors
		layout(binding = 0) readonly buffer MatIn { float matIn[]; };
		layout(binding = 1) writeonly buffer MatOut { float matOut[]; };

		void main()
		{
			const uint index = gl_GlobalInvocationID.x;
			float inVal = matIn[index];
			float outVal = 1.0 / (1.0 + exp(-inVal));
			matOut[index] = outVal;
		}
	)");
	std::vector<uint32_t> SigmoidForwardOp::m_ShaderSpirV;
	bool SigmoidForwardOp::m_Compiled = false;

	kp::Workgroup SigmoidForwardOp::GetWorkgroup(const Matrix& mat)
	{
		// Check if matrix is column vector
		if (mat.GetColCount() != 1)
			Log::Error("Matrix needs to be column vector for sigmoid forwars", true);

		// Return workgroup
		return kp::Workgroup({ mat.GetRowCount(), 1, 1 });
	}

	const std::vector<uint32_t>& SigmoidForwardOp::GetShaderSpirV()
	{
		if (!m_Compiled)
		{
			m_ShaderSpirV = CompileShader(m_ShaderCode);
			m_Compiled = true;
		}

		return m_ShaderSpirV;
	}
}
