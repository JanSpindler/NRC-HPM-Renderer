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
			uint32_t posID;
			uint32_t dirID;

			nlohmann::json jsonConfig;

			NNEncodingConfig();
			NNEncodingConfig(uint32_t posID, uint32_t dirID);
		};

		struct HpmSceneConfig
		{
			uint32_t id;

			float dirLightStrength;
			float pointLightStrength;
			std::string hdrEnvMapPath;
			float hdrEnvMapStrength;

			HpmSceneConfig();
			HpmSceneConfig(uint32_t id);
		};

		// nn
		std::string lossFn;
		std::string optimizer;
		float learningRate;
		float emaDecay;
		NNEncodingConfig encoding;
		uint32_t nnWidth;
		uint32_t nnDepth;
		uint32_t log2BatchSize;

		// scene
		HpmSceneConfig scene;

		// renderer
		float trainSampleRatio;
		float trainRingBufSize;
		uint32_t trainSpp;
		uint32_t primaryRayLength;

		AppConfig();
		AppConfig(const std::vector<char*>& argv);

		std::string GetName() const;

		void RenderImGui() const;
	};
}
