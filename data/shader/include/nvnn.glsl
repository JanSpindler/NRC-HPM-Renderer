#version 460
#include "common.glsl"

void LoadWeightsMat(const uint hiddenIndex, out fcoopmatNV coopMat)
{
	// Calc neuron count and startpos
	uint startPos;
	uint neuronCount;

	if (hiddenIndex == 0)
	{
		startPos = 0;
		neuronCount = INPUT_WEIGHTS_COUNT;
	}
	else if (hiddenIndex == LAYER_COUNT)
	{
		startPos = INPUT_WEIGHTS_COUNT + max(0, ((LAYER_COUNT -  1) * HIDDEN_WEIGHTS_COUNT));
		neuronCount = OUTPUT_WEIGHTS_COUNT;
	}
	else
	{
		startPos = INPUT_WEIGHTS_COUNT + ((hiddenIndex - 1) * HIDDEN_WEIGHTS_COUNT);
		neuronCount = HIDDEN_WEIGHTS_COUNT;
	}

	coopMatLoadNV(coopMat, weights, startPos, neuronCount, true);
}
