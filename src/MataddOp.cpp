#include <engine/compute/MataddOp.hpp>
#include <engine/util/Log.hpp>
#include <engine/util/compile_shader.hpp>

namespace en
{
	std::string MataddOp::m_ShaderCode = std::string(R"(
        // The version to use 
        #version 450

		// Push constants
		layout(push_constant) uniform PushConstants
		{
			uint rowCount;
			uint colCount;
        } config;

        // The buffers are provided via the tensors
        layout(binding = 0) readonly buffer MatLeft { float matLeft[]; };
        layout(binding = 1) readonly buffer MatRight { float matRight[]; };
        layout(binding = 2) writeonly buffer MatResult { float matResult[]; };

        void main()
		{
			// Get row / col / counts
			const uint outRow = gl_GlobalInvocationID.x;
			const uint outCol = gl_GlobalInvocationID.y;

			const uint outRowCount = config.rowCount;
			const uint outColCount = config.colCount;

			// Check row and col
			if (outRow >= outRowCount || outCol >= outColCount)
			{
			    return;
			}

			// Matrix addition
			uint index = outRow * outColCount + outCol;
			matResult[index] = matLeft[index] + matRight[index];
        }
      )");

	std::vector<uint32_t> MataddOp::m_ShaderSpirV;
	bool MataddOp::m_Compiled = false;

	MataddOp::Config MataddOp::GetConfig(const Matrix& matLeft, const Matrix& matRight)
	{
		// Check size for matrix addition
		if (matLeft.GetRowCount() != matRight.GetRowCount()
			|| matLeft.GetColCount() != matRight.GetColCount())
		{
			Log::Error("Left and right matrix must be equally sized for matrix addition", true);
		}

		// Create config
		Config config;
		config.rowCount = matLeft.GetRowCount();
		config.colCount = matLeft.GetColCount();
		
		return config;
	}
	
	kp::Workgroup MataddOp::GetWorkgroup(const Config& config)
	{
		return kp::Workgroup({ config.rowCount, config.colCount, 1 });
	}
	
	const std::vector<uint32_t>& MataddOp::GetShaderSpirV()
	{
		if (!m_Compiled)
		{
			m_ShaderSpirV = CompileShader(m_ShaderCode);
			m_Compiled = true;
		}

		return m_ShaderSpirV;
	}
}
