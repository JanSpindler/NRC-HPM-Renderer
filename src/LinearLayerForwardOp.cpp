#include <engine/compute/LinearLayerForwardOp.hpp>
#include <engine/util/compile_shader.hpp>
#include <engine/util/Log.hpp>

namespace en
{
	std::string LinearLayerForwardOp::m_ShaderCode(R"(
		// The version to use 
        #version 450

		// Push constants
		layout(push_constant) uniform PushConstants
		{
			uint inSize;
			uint outSize;
        } config;

        // The buffers are provided via the tensors
        layout(binding = 0) readonly buffer MatInput { float matInput[]; };
        layout(binding = 1) readonly buffer MatWeights { float matWeights[]; };
        layout(binding = 2) readonly buffer MatBiases { float matBiases[]; };
		layout(binding = 3) writeonly buffer MatOutput { float matOutput[]; }; 

		void MatmulWeights(uint index)
		{
			// Get row / col / counts
			const uint outRow = index;
			const uint outCol = 0;

			const uint outRowCount = config.outSize;
			const uint outColCount = 1;

			const uint leftRowCount = config.inSize;
			const uint leftColCount = config.outSize;
			const uint rightRowCount = config.inSize;
			const uint rightColCount = 1;

			// Check row and col
			if (outRow >= outRowCount || outCol >= outColCount)
			{
			    return;
			}

			// Matrix multiplication WEIGHTS * INPUT
			float dotProduct = 0.0;
			
			for (uint i = 0; i < rightRowCount; i++)
			{
			    float leftVal = matWeights[outRow * leftColCount + i];
			    float rightVal = matInput[i * rightColCount + outCol];
			    dotProduct += leftVal * rightVal;
			}

			uint outIndex = outRow * outColCount + outCol;
			matOutput[outIndex] = dotProduct;
		}

		void AddBiases(uint index)
		{
			matOutput[index] += matBiases[index];
		}

        void main()
		{
			const uint index = gl_GlobalInvocationID.x;
			MatmulWeights(index);
			AddBiases(index);
        }
	)");

	std::vector<uint32_t> LinearLayerForwardOp::m_ShaderSpirV;
	bool LinearLayerForwardOp::m_Compiled = false;

	LinearLayerForwardOp::Config LinearLayerForwardOp::GetConfig(const Matrix& input, const Matrix& weights, const Matrix& biases, const Matrix& output)
	{
		// Check sizes
		if (input.GetColCount() != 1
			|| output.GetColCount() != 1
			|| biases.GetColCount() != 1)
		{
			Log::Error("Input, output and biases need to be column vectors", true);
		}

		if (input.GetRowCount() != weights.GetColCount())
			Log::Error("Input row count needs to equal weights col count", true);

		if (weights.GetRowCount() != output.GetRowCount())
			Log::Error("Weights row count needs to equal output row count", true);

		if (output.GetRowCount() != biases.GetRowCount())
			Log::Error("Output row count needs to equal biases row count", true);

		// Create config
		Config config;
		config.inSize = input.GetRowCount();
		config.outSize = output.GetRowCount();

		//
		return config;
	}
	
	kp::Workgroup LinearLayerForwardOp::GetWorkgroup(const LinearLayerForwardOp::Config& config)
	{
		return kp::Workgroup({ config.outSize, 1, 1 });
	}
	
	const std::vector<uint32_t>& LinearLayerForwardOp::GetShaderSpirV()
	{
		if (!m_Compiled)
		{
			m_ShaderSpirV = CompileShader(m_ShaderCode);
			m_Compiled = true;
		}

		return m_ShaderSpirV;
	}
}
