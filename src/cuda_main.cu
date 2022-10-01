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
	{"encoding", {
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
	}},
	{"network", {
		{"otype", "FullyFusedMLP"},
		{"activation", "ReLU"},
		{"output_activation", "None"},
		{"n_neurons", 128},
		{"n_hidden_layers", 6},
	}},
	};

	const uint32_t n_input_dims = 5;
	const uint32_t n_output_dims = 3;
	const uint32_t n_inference_steps = 36;
	const uint32_t n_training_steps = 10;
	const uint32_t batch_size = 16384;

	tcnn::TrainableModel model = tcnn::create_from_config(n_input_dims, n_output_dims, config);

	tcnn::GPUMatrix<float> training_batch_inputs(n_input_dims, batch_size);
	tcnn::GPUMatrix<float> training_batch_targets(n_output_dims, batch_size);

	tcnn::GPUMatrix<float> inference_inputs(n_input_dims, batch_size);
	tcnn::GPUMatrix<float> inference_outputs(n_output_dims, batch_size);
	
	for (uint32_t i = 0; i < n_inference_steps; i++)
	{
		model.network->inference(inference_inputs, inference_outputs);
	}
}
