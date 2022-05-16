#include <engine/compute/SigmoidBackpropOp.hpp>
#include <engine/util/compile_shader.hpp>
#include <engine/util/Log.hpp>

namespace en
{
	std::string SigmoidBackpropOp::m_ShaderCode(R"(
		// The version to use 
        #version 450

        // The buffers are provided via the tensors
        layout(binding = 0) readonly buffer MatOldInput { float matOldInput[]; };
        layout(binding = 1) readonly buffer MatPrevError { float matPrevError[]; };
        layout(binding = 2) writeonly buffer MatLocalError { float matLocalError[]; };

        void main()
		{
			const uint index = gl_GlobalInvocationID.x;
			
			float oldInput = matOldInput[index];
			float prevError = matPrevError[index];

			float sigmoidVal = 1.0 / (1.0 + exp(-oldInput));
			float sigmoidDeriv = sigmoidVal * (1.0 - sigmoidVal);

			float localError = sigmoidDeriv * prevError;
			matLovalError[index] = localError;
        }
	)");

	std::vector<uint32_t> SigmoidBackpropOp::m_ShaderSpirV;
	bool SigmoidBackpropOp::m_Compiled = false;

	kp::Workgroup SigmoidBackpropOp::GetWorkgroup(const Matrix& oldInput, const Matrix& prevError, const Matrix& localError)
	{
		// Check if col vector
		if (!oldInput.IsColVector()
			|| !prevError.IsColVector()
			|| !localError.IsColVector())
		{
			Log::Error("Sigmoid backprop requires 3 col vectors", true);
		}

		// Check if size is equal
		if (oldInput.GetRowCount() != prevError.GetRowCount()
			|| prevError.GetRowCount() != localError.GetRowCount())
		{
			Log::Error("Sigmoid backprop requires 3 vectors of equal size", true);
		}

		//
		return kp::Workgroup({ oldInput.GetRowCount(), 1, 1});
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
