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
			uint32_t id = UINT32_MAX;

			float dirLightStrength = 0.0f;
			float pointLightStrength = 0.0f;
			std::string hdrEnvMapPath;
			float hdrEnvMapStrength = 0.0f;
			float density = 0.0f;
			bool dynamic = false;

			HpmSceneConfig();
			HpmSceneConfig(uint32_t id);
		};

		// nn
		std::string lossFn;
		std::string optimizer;
		float learningRate = 0.0f;
		float emaDecay = 0.0f;
		NNEncodingConfig encoding;
		uint32_t nnWidth = 0;
		uint32_t nnDepth = 0;
		uint32_t log2InferBatchSize = 0;
		uint32_t log2TrainBatchSize = 0;
		uint32_t trainBatchCount = 0;

		// scene
		HpmSceneConfig scene;

		// renderer
		float trainRingBufSize = 0.0f;
		uint32_t trainSpp = 0;
		uint32_t primaryRayLength = 0;
		float primaryRayProb = 0.0f;
		uint32_t trainRayLength = 0;

		AppConfig();
		AppConfig(const std::vector<char*>& argv);

		std::string GetName() const;

		void RenderImGui() const;
	};
}
