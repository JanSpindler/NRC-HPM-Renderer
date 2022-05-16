#pragma once

#include <engine/compute/Matrix.hpp>

namespace en
{
	class MatSetValueOp
	{
	public:
		struct Config
		{
			uint32_t rowCount;
			uint32_t colCount;
			float value;
		};

		static Config GetConfig(const Matrix& mat, float value);
		static kp::Workgroup GetWorkgroup(const Config& config);
		static const std::vector<uint32_t>& GetShaderSpirV();

	private:
		static std::string m_ShaderCode;
		static std::vector<uint32_t> m_ShaderSpirV;
		static bool m_Compiled;
	};
}
