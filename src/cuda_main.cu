#include <cuda_main.hpp>

#include <tiny-cuda-nn/config.h>

void RunTcnn()
{
	nlohmann::json config = {
	{"loss", {
		{"otype", "L2"}
	}},
	{"optimizer", {
		{"otype", "Adam"},
		{"learning_rate", 1e-3},
	}},
	//{"encoding", {
	//	{"otype", "HashGrid"},
	//	{"n_levels", 16},
	//	{"n_features_per_level", 2},
	//	{"log2_hashmap_size", 19},
	//	{"base_resolution", 16},
	//	{"per_level_scale", 2.0},
	//}},
	{"network", {
		{"otype", "FullyFusedMLP"},
		{"activation", "ReLU"},
		{"output_activation", "None"},
		{"n_neurons", 64},
		{"n_hidden_layers", 2},
	}},
	};

	const uint32_t n_input_dims = 32;
	const uint32_t n_output_dims = 64;
	const uint32_t n_training_steps = 10;
	const uint32_t batch_size = 128;

	auto model = tcnn::create_from_config(n_input_dims, n_output_dims, config);

	tcnn::GPUMatrix<float> training_batch_inputs(n_input_dims, batch_size);
	tcnn::GPUMatrix<float> training_batch_targets(n_output_dims, batch_size);

	// Use the model
	tcnn::GPUMatrix<float> inference_inputs(n_input_dims, batch_size);
	tcnn::GPUMatrix<float> inference_outputs(n_output_dims, batch_size);
	model.network->inference(inference_inputs, inference_outputs);
}
