#include <engine/compute/MatSetValueOp.hpp>
#include <engine/util/compile_shader.hpp>

namespace en
{
	std::string MatSetValueOp::m_ShaderCode(R"(
		// The version to use 
        #version 450

		// Push constants
		layout(push_constant) uniform PushConstants
		{
			uint rowCount;
			uint colCount;
			float value;
        } config;

        // The buffers are provided via the tensors
        layout(binding = 0) readonly buffer Mat { float mat[]; };

        void main()
		{
			const uint outRow = gl_GlobalInvocationID.x;
			const uint outCol = gl_GlobalInvocationID.y;

			uint linearIndex = outRow * config.colCount + outCol;
			mat[linearIndex] = config.value;
        }
	)");

	std::vector<uint32_t> MatSetValueOp::m_ShaderSpirV;
	bool MatSetValueOp::m_Compiled = false;

	MatSetValueOp::Config MatSetValueOp::GetConfig(const Matrix& mat, float value)
	{
		Config config;
		config.rowCount = mat.GetRowCount();
		config.colCount = mat.GetColCount();
		config.value = value;

		return config;
	}
	
	kp::Workgroup MatSetValueOp::GetWorkgroup(const MatSetValueOp::Config& config)
	{
		return kp::Workgroup({ config.rowCount, config.colCount, 1 });
	}
	
	const std::vector<uint32_t>& MatSetValueOp::GetShaderSpirV()
	{
		if (!m_Compiled)
		{
			m_ShaderSpirV = CompileShader(m_ShaderCode);
			m_Compiled = true;
		}

		return m_ShaderSpirV;
	}
}
