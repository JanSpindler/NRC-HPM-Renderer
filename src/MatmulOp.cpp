#include <engine/compute/MatmulOp.hpp>
#include <engine/util/compile_shader.hpp>
#include <engine/util/Log.hpp>

namespace en
{
	std::string MatmulOp::m_ShaderCode = std::string(R"(
        // The version to use 
        #version 450

		// Push constants
		layout(push_constant) uniform PushConstants
		{
            uint leftRowCount;
			uint leftColCount;
			uint rightRowCount;
			uint rightColCount;
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

			const uint outRowCount = config.leftRowCount;
			const uint outColCount = config.rightColCount;

			// ASSERT: matLeft.colCount == matRight.rowCount;

			// Check row and col
			if (outRow >= outRowCount || outCol >= outColCount)
			{
			    return;
			}

			// Matrix multiplication A * B
			float dotProduct = 0.0;
			
			for (uint i = 0; i < config.rightRowCount; i++)
			{
			    float leftVal = matLeft[outRow * config.leftColCount + i];
			    float rightVal = matRight[i * config.rightColCount + outCol];
			    dotProduct += leftVal * rightVal;
			}

			uint outIndex = outRow * outColCount + outCol;
			matResult[outIndex] = dotProduct;
        }
      )");

	std::vector<uint32_t> MatmulOp::m_ShaderSpirV;
	bool MatmulOp::m_Compiled = false;

	MatmulOp::Config MatmulOp::GetConfig(const Matrix& matLeft, const Matrix& matRight)
	{
		// Check size for matrix multiplication
		if (matLeft.GetColCount() != matRight.GetRowCount())
			Log::Error("Left matrix columns do not match right matrix rows -> cant multiply", true);

		// Create config
		Config config;
		config.leftRowCount = matLeft.GetRowCount();
		config.leftColCount = matLeft.GetColCount();
		config.rightRowCount = matRight.GetRowCount();
		config.rightColCount = matRight.GetColCount();
		
		return config;
	}
	
	kp::Workgroup MatmulOp::GetWorkgroup(const MatmulOp::Config& config)
	{
		return kp::Workgroup({ config.leftRowCount, config.rightColCount, 1 });
	}

	const std::vector<uint32_t>& MatmulOp::GetShaderSpirV()
	{
		if (!m_Compiled)
		{
			m_ShaderSpirV = CompileShader(m_ShaderCode);
			m_Compiled = true;
		}

		return m_ShaderSpirV;
	}
}
