#pragma once

#include <string>
#include <glm/glm.hpp>
#include <json/json.hpp>

namespace en
{
	struct AppConfig
	{
		struct NNEncodingConfig
		{
			// TODO
			uint32_t id;
			nlohmann::json jsonConfig;

			NNEncodingConfig();
			NNEncodingConfig(uint32_t id);
		};

		struct HpmSceneConfig
		{
			uint32_t id;

			float dirLightStrength;
			float pointLightStrength;
			std::string hdrEnvMapPath;
			float hdrEnvMapDirectStrength;
			float hdrEnvMapHpmStrength;

			HpmSceneConfig();
			HpmSceneConfig(uint32_t id);
		};

		// nn
		std::string lossFn;
		std::string optimizer;
		float learningRate;
		NNEncodingConfig encoding;
		uint32_t nnWidth;
		uint32_t nnDepth;
		uint32_t log2BatchSize;

		// scene
		HpmSceneConfig scene;

		// renderer
		uint32_t renderWidth;
		uint32_t renderHeight;
		float trainSampleRatio;
		uint32_t trainSpp;

		AppConfig();
		AppConfig(int argc, char** argv);

		std::string GetName() const;

		void RenderImGui() const;
	};
}
