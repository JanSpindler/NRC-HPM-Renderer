#pragma once

#include <string>
#include <glm/glm.hpp>

namespace en
{
	struct AppConfig
	{
		// nn
		std::string lossFn;
		std::string optimizer;
		float learningRate;
		// TODO: encoding
		uint32_t nnWidth;
		uint32_t nnDepth;

		// scene
		float dirLightStrength;
		float pointLightStrength;
		std::string hdrEnvMapPath;
		float hdrEnvMapDirectStrength;
		float hdrEnvMapHpmStrength;

		// renderer
		uint32_t renderWidth;
		uint32_t renderHeight;
		float trainSampleRatio;
		uint32_t trainSpp;
	};
}
