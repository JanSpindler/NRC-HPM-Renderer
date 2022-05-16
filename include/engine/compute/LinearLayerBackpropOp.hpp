#pragma once

#include <engine/compute/Matrix.hpp>

namespace en
{
	class LinearLayerBackpropOp
	{
	public:
		struct Config
		{
			uint32_t inputSize;
			uint32_t outputSize;
			float learningRate;
		};

		static Config GetConfig(
			const Matrix& oldInput, 
			const Matrix& prevError, 
			const Matrix& localError,
			const Matrix& weights,
			const Matrix& biases,
			const Matrix& deltaWeights,
			float learningRate);
		static kp::Workgroup GetWorkgroup(const Config& config);
		static const std::vector<uint32_t>& GetShaderSpirV();

	private:
		static std::string m_ShaderCode;
		static std::vector<uint32_t> m_ShaderSpirV;
		static bool m_Compiled;
	};
}
