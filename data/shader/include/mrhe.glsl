float GetMrheFeature(const uint level, const uint entryIndex, const uint featureIndex)
{
	const uint linearIndex = (POS_HASH_TABLE_SIZE * POS_FEATURE_COUNT * level) + (entryIndex * POS_FEATURE_COUNT) + featureIndex;
	const float feature = mrhe[linearIndex];
	return feature;
}

uint HashFunc(const uvec3 pos)
{
	const uvec3 primes = uvec3(1, 19349663, 83492791);
	uint hash = (pos.x * primes.x) + (pos.y * primes.y) + (pos.z * primes.z);
	hash %= POS_HASH_TABLE_SIZE;
	return hash;
}

void EncodePosMrhe(const vec3 normPos, const uint mrheInputBaseIndex)
{
	for (uint level = 0; level < POS_LEVEL_COUNT; level++)
	{
		// Get level resolution
		const uint res = mrheResolutions[level];
		const vec3 resPos = normPos * float(res);

		// Get all 8 neighbours
		const vec3 floorPos = floor(resPos);

		vec3 neighbours[8]; // 2^3
		for (uint x = 0; x < 2; x++)
		{
			for (uint y = 0; y < 2; y++)
			{
				for (uint z = 0; z < 2; z++)
				{
					uint linearIndex = (x * 4) + (y * 2) + z;
					neighbours[linearIndex] = floorPos + vec3(uvec3(x, y, z));
				}
			}
		}

		// Get neighbour indices and store
		uint neighbourIndices[8];
		for (uint neigh = 0; neigh < 8; neigh++)
		{
			const uint index = HashFunc(uvec3(neighbours[neigh]));
			neighbourIndices[neigh] = index;
		}

		// Extract neighbour features
		float neighbourFeatures[8 * POS_FEATURE_COUNT];
		for (uint neigh = 0; neigh < 8; neigh++)
		{
			const uint entryIndex = neighbourIndices[neigh];
			for (uint feature = 0; feature < POS_FEATURE_COUNT; feature++)
			{
				neighbourFeatures[(neigh * POS_FEATURE_COUNT) + feature] = GetMrheFeature(level, entryIndex, feature);
			}
		}

		// Linearly interpolate neightbour features
		vec3 lerpFactors = resPos - neighbours[0];
		float linearLerpFactor[8];
		for (uint neigh = 0; neigh < 8; neigh++)
		{
			const float xFactor = (neigh >> 2) > 0 ? (1.0 - lerpFactors.x) : lerpFactors.x;
			const float yFactor = (neigh >> 1) > 0 ? (1.0 - lerpFactors.y) : lerpFactors.y;
			const float zFactor = (neigh >> 0) > 0 ? (1.0 - lerpFactors.z) : lerpFactors.z;
			linearLerpFactor[neigh] = xFactor * yFactor * zFactor;
		}

		// Store
		const uint levelBaseIndex = mrheInputBaseIndex + (level * POS_FEATURE_COUNT);
		for (uint feature = 0; feature < POS_FEATURE_COUNT; feature++)
		{
			float sum = 0.0;
			for (uint neigh = 0; neigh < 8; neigh++)
			{
				sum += linearLerpFactor[neigh] * neighbourFeatures[(neigh * POS_FEATURE_COUNT) + feature];
			}
			neurons[levelBaseIndex + feature] = sum;
		}
	}
}

/*void BackpropMrhe()
{
	for (uint level = 0; level < 16; level++)
	{
		const vec3 lerpFactors = allLerpFactors[level];

		uint neighbourIndices[8];
		for (uint neigh = 0; neigh < 8; neigh++)
		{
			const uint linearIndex = (level * 8) + neigh;
			neighbourIndices[neigh] = allNeighbourIndices[linearIndex];
		}

		const vec2 error = vec2(nr0[(level * 2) + 0], nr0[(level * 2) + 0]);

		for (uint x = 0; x < 2; x++)
		{
			for (uint y = 0; y < 2; y++)
			{
				for (uint z = 0; z < 2; z++)
				{
					const uint linearIndex = (x * 4) + (y * 2) + z;
					const uint tableEntryIndex = neighbourIndices[linearIndex];

					const float xFactor = x == 1 ? lerpFactors.x : (1.0 - lerpFactors.x);
					const float yFactor = y == 1 ? lerpFactors.y : (1.0 - lerpFactors.y);
					const float zFactor = z == 1 ? lerpFactors.z : (1.0 - lerpFactors.z);
					const float errorWeight = xFactor * yFactor * zFactor;
					const vec2 delta = -error * errorWeight * ONE_OVER_PIXEL_COUNT;

					atomicAdd(mrDeltaHashTable[(2 * tableEntryIndex) + 0], delta.x);
					atomicAdd(mrDeltaHashTable[(2 * tableEntryIndex) + 1], delta.y);
				}
			}
		}
	}
}*/
