// Start: NN

float nr0[64];
float nr1[64];
float nr2[64];
float nr3[64];
float nr4[64];
float nr5[64];
float nr6[3];

float Sigmoid(float x)
{
	return 1.0 / (1.0 + exp(-x));
}

float Relu(float x)
{
	return max(0.0, x);
}

float SigmoidDeriv(float x)
{
	float sigmoid = Sigmoid(x);
	return sigmoid * (1.0 - sigmoid);
}

float ReluDeriv(float x)
{
	if (x > 0.0)
	{
		return 1.0;
	}
	else
	{
		return 0.0;
	}
}

float GetWeight0(uint row, uint col)
{
	uint linearIndex = row * 64 + col;
	return matWeights0[linearIndex];
}

float GetWeight1(uint row, uint col)
{
	uint linearIndex = row * 64 + col;
	return matWeights1[linearIndex];
}

float GetWeight2(uint row, uint col)
{
	uint linearIndex = row * 64 + col;
	return matWeights2[linearIndex];
}

float GetWeight3(uint row, uint col)
{
	uint linearIndex = row * 64 + col;
	return matWeights3[linearIndex];
}

float GetWeight4(uint row, uint col)
{
	uint linearIndex = row * 64 + col;
	return matWeights4[linearIndex];
}

float GetWeight5(uint row, uint col)
{
	uint linearIndex = row * 64 + col;
	return matWeights5[linearIndex];
}

void ApplyWeights0()
{
	for (uint outRow = 0; outRow < 64; outRow++)
	{
		float sum = 0;
		
		for (uint inCol = 0; inCol < 64; inCol++)
		{
			float inVal = nr0[inCol];
			float weightVal = GetWeight0(outRow, inCol);
			sum += inVal * weightVal;
		}

		nr1[outRow] = sum + matBiases0[outRow];
	}
}

void ApplyWeights1()
{
	for (uint outRow = 0; outRow < 64; outRow++)
	{
		float sum = 0;
		
		for (uint inCol = 0; inCol < 64; inCol++)
		{
			float inVal = nr1[inCol];
			float weightVal = GetWeight1(outRow, inCol);
			sum += inVal * weightVal;
		}

		nr2[outRow] = sum + matBiases1[outRow];
	}
}

void ApplyWeights2()
{
	for (uint outRow = 0; outRow < 64; outRow++)
	{
		float sum = 0;
		
		for (uint inCol = 0; inCol < 64; inCol++)
		{
			float inVal = nr2[inCol];
			float weightVal = GetWeight2(outRow, inCol);
			sum += inVal * weightVal;
		}

		nr3[outRow] = sum + matBiases2[outRow];
	}
}

void ApplyWeights3()
{
	for (uint outRow = 0; outRow < 64; outRow++)
	{
		float sum = 0;
		
		for (uint inCol = 0; inCol < 64; inCol++)
		{
			float inVal = nr3[inCol];
			float weightVal = GetWeight3(outRow, inCol);
			sum += inVal * weightVal;
		}

		nr4[outRow] = sum + matBiases3[outRow];
	}
}

void ApplyWeights4()
{
	for (uint outRow = 0; outRow < 64; outRow++)
	{
		float sum = 0;
		
		for (uint inCol = 0; inCol < 64; inCol++)
		{
			float inVal = nr4[inCol];
			float weightVal = GetWeight4(outRow, inCol);
			sum += inVal * weightVal;
		}

		nr5[outRow] = sum + matBiases4[outRow];
	}
}

void ApplyWeights5()
{
	for (uint outRow = 0; outRow < 3; outRow++)
	{
		float sum = 0;
		
		for (uint inCol = 0; inCol < 64; inCol++)
		{
			float inVal = nr5[inCol];
			float weightVal = GetWeight5(outRow, inCol);
			sum += inVal * weightVal;
		}

		nr6[outRow] = sum + matBiases5[outRow];
	}
}

void ActivateNr1()
{
	for (uint i = 0; i < 64; i++)
	{
		//nr1[i] = Sigmoid(nr1[i]);
		nr1[i] = Relu(nr1[i]);
	}
}

void ActivateNr2()
{
	for (uint i = 0; i < 64; i++)
	{
		//nr2[i] = Sigmoid(nr2[i]);
		nr2[i] = Relu(nr2[i]);
	}
}

void ActivateNr3()
{
	for (uint i = 0; i < 64; i++)
	{
		//nr3[i] = Sigmoid(nr3[i]);
		nr3[i] = Relu(nr3[i]);
	}
}

void ActivateNr4()
{
	for (uint i = 0; i < 64; i++)
	{
		//nr4[i] = Sigmoid(nr4[i]);
		nr4[i] = Relu(nr4[i]);
	}
}

void ActivateNr5()
{
	for (uint i = 0; i < 64; i++)
	{
		//nr5[i] = Sigmoid(nr5[i]);
		nr5[i] = Relu(nr5[i]);
	}
}

void ActivateNr6()
{
	for (uint i = 0; i < 4; i++)
	{
		//nr6[i] = Sigmoid(nr6[i]);
		nr6[i] = Relu(nr6[i]);
	}
}

void Forward()
{
	// Assert: pos and dir are encoded correctly

	//debugPrintfEXT("%f, %f, %f, %f, %f\n", nr0[0], nr0[1], nr0[2], nr0[3], nr0[4]);

	ApplyWeights0();
	ActivateNr1();
	
	ApplyWeights1();
	ActivateNr2();
	
	ApplyWeights2();
	ActivateNr3();
	
	ApplyWeights3();
	ActivateNr4();

	ApplyWeights4();
	ActivateNr5();
	
	ApplyWeights5();
	ActivateNr6();

	// nr6 contains result
	//debugPrintfEXT("%f, %f, %f\n", nr6[0], nr6[1], nr6[2]);
}

void Backprop5()
{
	// Backprop end activation
	for (uint i = 0; i < 3; i++)
	{
		nr6[i] *= ReluDeriv(nr5[i]);
	}

	// Calc delta Weights5
	for (uint row = 0; row < 3; row++)
	{
		for (uint col = 0; col < 64; col++)
		{
			uint linearIndex = (row * 64) + col;
			float deltaWeight = -nr5[col] * nr6[row];
			atomicAdd(matDeltaWeights5[linearIndex], deltaWeight * ONE_OVER_PIXEL_COUNT);
		}

		atomicAdd(matDeltaBiases5[row], -nr6[row] * ONE_OVER_PIXEL_COUNT);
	}

	// Backprop weights5
	for (uint col = 0; col < 64; col++)
	{
		float error = 0.0;
		for (uint row = 0; row < 3; row++)
		{
			uint linearIndex = row * 64 + col;
			error += matWeights5[linearIndex] * nr6[row];
		}
		nr5[col] = error;
	}
}

void Backprop4()
{
	// Backprop end activation
	for (uint i = 0; i < 64; i++)
	{
		nr5[i] *= ReluDeriv(nr4[i]);
	}

	// Calc delta Weights5
	for (uint row = 0; row < 64; row++)
	{
		for (uint col = 0; col < 64; col++)
		{
			uint linearIndex = (row * 64) + col;
			float deltaWeight = -nr4[col] * nr5[row];
			atomicAdd(matDeltaWeights4[linearIndex], deltaWeight * ONE_OVER_PIXEL_COUNT);
		}
		
		atomicAdd(matDeltaBiases4[row], -nr5[row] * ONE_OVER_PIXEL_COUNT);
	}

	// Backprop weights5
	for (uint col = 0; col < 64; col++)
	{
		float error = 0.0;
		for (uint row = 0; row < 64; row++)
		{
			uint linearIndex = row * 64 + col;
			error += matWeights4[linearIndex] * nr5[row];
		}
		nr4[col] = error;
	}
}

void Backprop3()
{
	// Backprop end activation
	for (uint i = 0; i < 64; i++)
	{
		nr4[i] *= ReluDeriv(nr3[i]);
	}

	// Calc delta Weights5
	for (uint row = 0; row < 64; row++)
	{
		for (uint col = 0; col < 64; col++)
		{
			uint linearIndex = (row * 64) + col;
			float deltaWeight = -nr3[col] * nr4[row];
			atomicAdd(matDeltaWeights3[linearIndex], deltaWeight * ONE_OVER_PIXEL_COUNT);
		}
		
		atomicAdd(matDeltaBiases3[row], -nr4[row] * ONE_OVER_PIXEL_COUNT);
	}

	// Backprop weights5
	for (uint col = 0; col < 64; col++)
	{
		float error = 0.0;
		for (uint row = 0; row < 64; row++)
		{
			uint linearIndex = row * 64 + col;
			error += matWeights3[linearIndex] * nr4[row];
		}
		nr3[col] = error;
	}
}

void Backprop2()
{
	// Backprop end activation
	for (uint i = 0; i < 64; i++)
	{
		nr3[i] *= ReluDeriv(nr2[i]);
	}

	// Calc delta Weights5
	for (uint row = 0; row < 64; row++)
	{
		for (uint col = 0; col < 64; col++)
		{
			uint linearIndex = (row * 64) + col;
			float deltaWeight = -nr2[col] * nr3[row];
			atomicAdd(matDeltaWeights2[linearIndex], deltaWeight * ONE_OVER_PIXEL_COUNT);
		}
		
		atomicAdd(matDeltaBiases2[row], -nr3[row] * ONE_OVER_PIXEL_COUNT);
	}

	// Backprop weights5
	for (uint col = 0; col < 64; col++)
	{
		float error = 0.0;
		for (uint row = 0; row < 64; row++)
		{
			uint linearIndex = row * 64 + col;
			error += matWeights2[linearIndex] * nr3[row];
		}
		nr2[col] = error;
	}
}

void Backprop1()
{
	// Backprop end activation
	for (uint i = 0; i < 64; i++)
	{
		nr2[i] *= ReluDeriv(nr1[i]);
	}

	// Calc delta Weights5
	for (uint row = 0; row < 64; row++)
	{
		for (uint col = 0; col < 64; col++)
		{
			uint linearIndex = (row * 64) + col;
			float deltaWeight = -nr1[col] * nr2[row];
			atomicAdd(matDeltaWeights1[linearIndex], deltaWeight * ONE_OVER_PIXEL_COUNT);
		}
		
		atomicAdd(matDeltaBiases1[row], -nr2[row] * ONE_OVER_PIXEL_COUNT);
	}

	// Backprop weights5
	for (uint col = 0; col < 64; col++)
	{
		float error = 0.0;
		for (uint row = 0; row < 64; row++)
		{
			uint linearIndex = row * 64 + col;
			error += matWeights1[linearIndex] * nr2[row];
		}
		nr1[col] = error;
	}
}

void Backprop0()
{
	// Backprop end activation
	for (uint i = 0; i < 64; i++)
	{
		nr1[i] *= ReluDeriv(nr0[i]);
	}

	// Calc delta Weights5
	for (uint row = 0; row < 64; row++)
	{
		for (uint col = 0; col < 64; col++)
		{
			uint linearIndex = (row * 64) + col;
			float deltaWeight = -nr0[col] * nr1[row];
			atomicAdd(matDeltaWeights0[linearIndex], deltaWeight * ONE_OVER_PIXEL_COUNT);
		}
		
		atomicAdd(matDeltaBiases0[row], -nr1[row] * ONE_OVER_PIXEL_COUNT);
	}

	// Backprop to mrhe
	for (uint col = 0; col < 64; col++)
	{
		float error = 0.0;
		for (uint row = 0; row < 64; row++)
		{
			uint linearIndex = (row * 64) + col;
			error += matWeights0[linearIndex] * nr1[row];
		}
		nr0[col] = error;
	}
}