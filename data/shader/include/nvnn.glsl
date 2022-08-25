/*void LoadWeightsMat(const uint hiddenIndex, const uint type, out fcoopmatNV coopMat)
{
	// Calc neuron count and startpos
	uint startPos;
	uint neuronCount;

	if (hiddenIndex == 0)
	{
		startPos = 0;
	}
	else if (hiddenIndex == LAYER_COUNT)
	{
		startPos = INPUT_WEIGHTS_COUNT + max(0, ((LAYER_COUNT -  1) * HIDDEN_WEIGHTS_COUNT));
	}
	else
	{
		startPos = INPUT_WEIGHTS_COUNT + ((hiddenIndex - 1) * HIDDEN_WEIGHTS_COUNT);
	}

	if (type == NN_MAT_TYPE_OG)
	{
		coopMatLoadNV(coopMat, weights, startPos, 1, true);
	}
	else if (type == NN_MAT_TYPE_DELTA)
	{
		coopMatLoadNV(coopMat, deltaWeights, startPos, 1, true);
	}
	else if (type == NN_MAT_TYPE_MOMENTUM)
	{
		coopMatLoadNV(coopMat, momentumWeights, startPos, 1, true);
	}
	else
	{
		debugPrintfEXT("Shader: Unknown nn mat type\n");
	}
}

void LoadBiasesMat(const uint hiddenIndex, const uint type, out fcoopmatNV coopMat)
{
	// Calc startpos
	uint startPos;

	if (hiddenIndex == LAYER_COUNT)
	{
		startPos = LAYER_WIDTH * LAYER_COUNT;
	}
	else
	{
		startPos = LAYER_WIDTH * hiddenIndex;
	}

	if (type == NN_MAT_TYPE_OG)
	{
		coopMatLoadNV(coopMat, biases, startPos, 1, true);
	}
	else if (type == NN_MAT_TYPE_DELTA)
	{
		coopMatLoadNV(coopMat, deltaBiases, startPos, 1, true);
	}
	else if (type == NN_MAT_TYPE_MOMENTUM)
	{
		coopMatLoadNV(coopMat, momentumBiases, startPos, 1, true);
	}
	else
	{
		debugPrintfEXT("Shader: Unknown nn mat type\n");
	}
}

void LoadNeuronsMat(const uint index, out fcoopmatNV coopMat)
{
	// TODO:
}

void EncodeRay(const vec3 rayOrigin, const vec3 rayDir)
{
	
}

void Forward(const vec3 rayOrigin, const vec3 rayDir)
{
	EncodeRay(rayOrigin, rayDir);

	// Load first layer
	fcoopmatNV<32, gl_ScopeSubgroup, LAYER_WIDTH_AFTER_INPUT, INPUT_FEATURE_COUNT> firstWeightsMat;
	LoadWeightsMat(0, NN_MAT_TYPE_OG, firstWeightsMat);

	fcoopmatNV<32, gl_ScopeSubgroup, LAYER_WIDTH_AFTER_INPUT, 1> firstBiasesMat;
	LoadBiasesMat(0, NN_MAT_TYPE_OG, firstBiasesMat);

	fcoopmatNV<32, gl_ScopeSubgroup, INPUT_FEATURE_COUNT, 1> inputNeurons;
	LoadNeuronsMat(0, inputNeurons);

	// Load hidden neurons
	fcoopmatNV<32, gl_ScopeSubgroup, LAYER_WIDTH, 1> hiddenNeurons[LAYER_COUNT];
	for (uint i = 0; i < LAYER_COUNT; i++)
	{
		LoadNeuronsMat(1 + i, hiddenNeurons[i]);
	}

	fcoopmatNV<32, gl_ScopeSubgroup, 3, 1> outputNeurons;
	LoadNeuronsMat(LAYER_COUNT + 1, outputNeurons);

	for (uint i = 0; i < LAYER_COUNT; i++)
	{

	}
}
*/