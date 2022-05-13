#pragma once

#include <engine/compute/Matrix.hpp>

namespace en
{
	class LinearLayerForwardOp
	{
	public:
		struct Config
		{
			uint32_t inSize; // Column count of weight matrix or row count of input
			uint32_t outSize; // Row count of weight matrix or row count of output or row count of bias vector
		};

		static Config GetConfig(const Matrix& input, const Matrix& weights, const Matrix& biases, const Matrix& output);
		static kp::Workgroup GetWorkgroup(const Config& config);
		static const std::vector<uint32_t>& GetShaderSpirV();

	private:
		static std::string m_ShaderCode;
		static std::vector<uint32_t> m_ShaderSpirV;
		static bool m_Compiled;
	};
}
