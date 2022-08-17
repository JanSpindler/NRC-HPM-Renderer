float oneBlobFeatures[32];

float NormGauss(const float x, const float m, const float sigma)
{
	const float term1 = 1.0 / (sigma * sqrt(2.0 * PI));
	const float term2 = ((x - m) / sigma);
	const float result = term1 * exp(-0.5 * term2 * term2);
	return result;
}

void EncodeDirOneBlob(const vec3 dir)
{
	// Theta and phi in [0, 1]
	const float theta = (atan(dir.z, dir.x) / PI) + 0.5;
	const float phi = (atan(length(dir.xz), dir.y) / PI) + 0.5;

	const float sigma = 1.0 / 16.0; // sqrt(16.0)
	for (uint i = 0; i < 16; i++)
	{
		const float fI = float(i);
		oneBlobFeatures[i] = NormGauss(fI, theta, sigma);
		oneBlobFeatures[i + 16] = NormGauss(fI, phi, sigma);
	}
}
