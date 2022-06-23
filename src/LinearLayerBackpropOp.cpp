#include <engine/compute/LinearLayerBackpropOp.hpp>
#include <engine/util/compile_shader.hpp>
#include <engine/util/Log.hpp>

namespace en
{
	std::string LinearLayerBackpropOp::m_ShaderCode(R"(
		// The version to use 
        #version 450
		#extension GL_EXT_shader_atomic_float : enable

		// Push constants
		layout(push_constant) uniform PushConstants
		{
			uint inputSize;
			uint outputSize;
			float learningRate;
        } config;

        // The buffers are provided via the tensors
		layout(binding = 0) readonly buffer MatOldInput { float matOldInput[]; };
		layout(binding = 1) readonly buffer MatPrevError { float matPrevError[]; };
		layout(binding = 2) buffer MatLocalError { float matLocalError[]; };
		layout(binding = 3) buffer MatWeights { float matWeights[]; };
		layout(binding = 4) buffer MatBiases { float matBiases[]; };
		layout(binding = 5) buffer MatDeltaWeights { float matDeltaWeights[]; };

		#define isNaN(x) ( (x) != (x)    )
		#define isInf(x) ( (x) == (x)+1. )

		float rand(vec2 co)
		{
			return fract(sin(dot(co, vec2(12.9898, 78.233))) * 43758.5453);
		}

		void LearnWeights(uint outRow, uint outCol)
		{
			uint linearIndex = outRow * config.inputSize + outCol;
			
			float oldDeltaWeight = matDeltaWeights[linearIndex];
			float newDeltaWeight = -matOldInput[outCol] * matPrevError[outRow] * config.learningRate;

			float beta = 0.0;
			matDeltaWeights[linearIndex] = (beta * oldDeltaWeight) + ((1.0 - beta) * newDeltaWeight);

			// Correct nans / infs
			float weight = matWeights[linearIndex];			
			if (isNaN(weight) || isInf(weight))
			{
				matWeights[linearIndex] = 0.0;//rand(vec2(float(outRow), float(outCol)));
				matDeltaWeights[linearIndex] = 0.0;
			}
		}

		void LearnBiases(uint outRow)
		{
			float oldBias = matBiases[outRow];
			float deltaBias = matPrevError[outRow] * config.learningRate;
			matBiases[outRow] = oldBias - deltaBias;
		}

		void CalcLocalError(uint outRow, uint outCol)
		{
			uint linearIndex = outRow * config.inputSize + outCol;
			float deltaError = matWeights[linearIndex] * matPrevError[outRow];
			atomicAdd(matLocalError[outCol], deltaError);
		}

        void main()
		{
			// Get row / col / counts
			const uint outRow = gl_GlobalInvocationID.x; // outIndex
			const uint outCol = gl_GlobalInvocationID.y; // inIndex

			LearnWeights(outRow, outCol);
			
			//if (outCol == 0)
			//{
			//	LearnBiases(outRow);
			//}

			CalcLocalError(outRow, outCol);
		}
	)");
	
	std::vector<uint32_t> LinearLayerBackpropOp::m_ShaderSpirV;
	bool LinearLayerBackpropOp::m_Compiled = false;

	LinearLayerBackpropOp::Config LinearLayerBackpropOp::GetConfig(
		const Matrix& oldInput,
		const Matrix& prevError,
		const Matrix& localError,
		const Matrix& weights,
		const Matrix& biases,
		const Matrix& deltaWeights,
		float learningRate)
	{
		// Check if col vector
		if (!oldInput.IsColVector()
			|| !prevError.IsColVector()
			|| !localError.IsColVector())
		{
			Log::Error("Linear layer backprop requires 3 col vectors", true);
		}

		// TODO: check size of weights and biases

		// Calculate config
		Config config;
		config.inputSize = oldInput.GetRowCount();
		config.outputSize = prevError.GetRowCount();
		config.learningRate = learningRate;

		//
		return config;
	}
	
	kp::Workgroup LinearLayerBackpropOp::GetWorkgroup(const LinearLayerBackpropOp::Config& config)
	{
		return kp::Workgroup({ config.outputSize, config.inputSize, 1 });
	}
	
	const std::vector<uint32_t>& LinearLayerBackpropOp::GetShaderSpirV()
	{
		if (!m_Compiled)
		{
			m_ShaderSpirV = CompileShader(m_ShaderCode);
			m_Compiled = true;
		}

		return m_ShaderSpirV;
	}
}
