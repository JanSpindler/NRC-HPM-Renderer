#pragma once

#include <engine/compute/Matrix.hpp>

namespace en
{
	// Matmul wraps helper functions for matrix multiplication using the Kompute library
	class MatmulOp
	{
	public:
		struct Config
		{
			uint32_t leftRowCount;
			uint32_t leftColCount;
			uint32_t rightRowCount;
			uint32_t rightColCount;
		};

		static Config GetConfig(const Matrix& matLeft, const Matrix& matRight);
		static kp::Workgroup GetWorkgroup(const Config& config);
		static const std::vector<uint32_t>& GetShaderSpirV();

	private:
		static std::string m_ShaderCode;
		static std::vector<uint32_t> m_ShaderSpirV;
		static bool m_Compiled;
	};
}
