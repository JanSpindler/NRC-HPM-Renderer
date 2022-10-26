#include <engine/AppConfig.hpp>
#include <engine/util/Log.hpp>
#include <imgui.h>

namespace en
{
	AppConfig::NNEncodingConfig::NNEncodingConfig()
	{
	}

	AppConfig::NNEncodingConfig::NNEncodingConfig(uint32_t id) :
		id(id)
	{
		switch (id)
		{
		case 0:
			jsonConfig =
			{ "encoding", {
				{"otype", "Composite"},
				{"reduction", "Concatenation"},
				{"nested", {
					{
						{"otype", "HashGrid"},
						{"n_dims_to_encode", 3},
						{"n_levels", 16},
						{"n_features_per_level", 2},
						{"log2_hashmap_size", 19},
						{"base_resolution", 16},
						{"per_level_scale", 2.0},
					},
					{
						{"otype", "OneBlob"},
						{"n_dims_to_encode", 2},
						{"n_bins", 4},
					},
				}},
			} };
			break;
		default:
			Log::Error("NNEncodingConfig ID is invalid", true);
			break;
		}
	}

	AppConfig::HpmSceneConfig::HpmSceneConfig()
	{
	}

	AppConfig::HpmSceneConfig::HpmSceneConfig(uint32_t id) :
		id(id)
	{
		switch (id)
		{
		case 0:
			dirLightStrength = 16.0f;
			pointLightStrength = 0.0f;
			hdrEnvMapPath = "data/image/photostudio.hdr";
			hdrEnvMapDirectStrength = 1.0f;
			hdrEnvMapHpmStrength = 0.0f;
			break;
		default:
			Log::Error("HpmSceneConfig ID is invalid", true);
			break;
		}
	}

	AppConfig::AppConfig()
	{
	}

	AppConfig::AppConfig(int argc, char** argv)
	{
		if (argc != 13) { Log::Error("Argument count does not match requirements for AppConfig", true); }

		size_t index = 1;

		lossFn = std::string(argv[index++]);
		optimizer = std::string(argv[index++]);
		learningRate = std::stof(argv[index++]);
		encoding = NNEncodingConfig(std::stoi(argv[index++]));
		nnWidth = std::stoi(argv[index++]);
		nnDepth = std::stoi(argv[index++]);
		log2BatchSize = std::stoi(argv[index++]);

		scene = HpmSceneConfig(std::stoi(argv[index++]));

		renderWidth = std::stoi(argv[index++]);
		renderHeight = std::stoi(argv[index++]);
		trainSampleRatio = std::stof(argv[index++]);
		trainSpp = std::stoi(argv[index++]);
	}

	void AppConfig::RenderImGui() const
	{
		ImGui::Begin("AppConfig");
		ImGui::Text(lossFn.c_str());
		ImGui::Text(optimizer.c_str());
		ImGui::Text("Learning rate %f", learningRate);
		ImGui::Text("Encoding %d", encoding.id);
		ImGui::Text("NN Width %d", nnWidth);
		ImGui::Text("NN Depth %d", nnDepth);
		ImGui::Text("Log2 batch size %d", log2BatchSize);
		ImGui::Text("Scene %d", scene.id);
		ImGui::Text("Render width %d", renderWidth);
		ImGui::Text("Render height %d", renderHeight);
		ImGui::Text("Train sample ratio %f", trainSampleRatio);
		ImGui::Text("Train spp %d", trainSpp);
		ImGui::End();
	}
}
