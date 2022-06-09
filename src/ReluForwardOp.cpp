#include <engine/compute/ReluForwardOp.hpp>
#include <engine/util/compile_shader.hpp>
#include <engine/util/Log.hpp>

namespace en
{
	std::string ReluForwardOp::m_ShaderCode(R"(
		// The version to use 
		#version 450

		// The buffers are provided via the tensors
		layout(binding = 0) readonly buffer MatIn { float matIn[]; };
		layout(binding = 1) writeonly buffer MatOut { float matOut[]; };

		void main()
		{
			const uint index = gl_GlobalInvocationID.x;
			float inVal = matIn[index];
			
			float outVal = 0.0;

			if (inVal > 20.0)
			{
				outVal = inVal;
			}
			else if (inVal < 20.0)
			{
				outVal = 0.0;
			}
			else
			{
				outVal = log(1.0 + exp(inVal));
			}

			matOut[index] = outVal;
		}
	)");
	std::vector<uint32_t> ReluForwardOp::m_ShaderSpirV;
	bool ReluForwardOp::m_Compiled = false;

	kp::Workgroup ReluForwardOp::GetWorkgroup(const Matrix& input, const Matrix& output)
	{
		// Check if matrix is column vector
		if (input.GetColCount() != 1 || output.GetColCount() != 1)
			Log::Error("Matrix needs to be column vector for relu forward", true);

		// Check if matrix is equally sized
		if (input.GetRowCount() != output.GetRowCount())
			Log::Error("relu forward requires equal size of input and output", true);

		// Return workgroup
		return kp::Workgroup({ input.GetRowCount(), 1, 1 });
	}

	const std::vector<uint32_t>& ReluForwardOp::GetShaderSpirV()
	{
		if (!m_Compiled)
		{
			m_ShaderSpirV = CompileShader(m_ShaderCode);
			m_Compiled = true;
		}

		return m_ShaderSpirV;
	}
}
