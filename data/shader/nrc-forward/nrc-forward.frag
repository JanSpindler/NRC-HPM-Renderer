#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_EXT_debug_printf : enable

// Inputs
layout(location = 0) in vec3 pixelWorldPos;
layout(location = 1) in vec2 fragUV;

// Uniforms
layout(set = 0, binding = 1) uniform camera_t
{
	vec3 pos;
} camera;

layout(set = 1, binding = 0) uniform sampler3D densityTex;

layout(set = 1, binding = 1) uniform volumeData_t
{
	vec4 random;
	uint useNN;
	float densityFactor;
	float g;
	uint lowPassIndex;
} volumeData;

layout(set = 2, binding = 0) uniform dir_light_t
{
	vec3 color;
	float zenith;
	vec3 dir;
	float azimuth;
	float strength;
} dir_light;

layout(set = 3, binding = 0) uniform sampler2D lowPassTex;

// NN buffers
layout(std430, set = 4, binding = 0) readonly buffer Weights0
{
	float matWeights0[320]; // 5 x 64
};

layout(std430, set = 4, binding = 1) readonly buffer Weights1
{
	float matWeights1[4096]; // 64 x 64
};

layout(std430, set = 4, binding = 2) readonly buffer Weights2
{
	float matWeights2[4096]; // 64 x 64
};

layout(std430, set = 4, binding = 3) readonly buffer Weights3
{
	float matWeights3[4096]; // 64 x 64
};

layout(std430, set = 4, binding = 4) readonly buffer Weights4
{
	float matWeights4[4096]; // 64 x 64
};

layout(std430, set = 4, binding = 5) readonly buffer Weights5
{
	float matWeights5[192]; // 64 x 3
};

layout(set = 5, binding = 0) uniform PointLight
{
	vec3 pos;
	float strength;
	vec3 color;
} pointLight;

layout(set = 6, binding = 0) uniform sampler2D hdrEnvMap;

layout(set = 6, binding = 1) uniform HdrEnvMapData
{
	float directStrength;
	float hpmStrength;
} hdrEnvMapData;

layout(set = 7, binding = 0) uniform MrheData
{
	uint levelCount;
	uint hashTableSize;
	uint featureCount;
	uint minRes;
	uint maxRes;
	uint resolutions[16];
} mrhe;

layout(std430, set = 7, binding = 1) readonly buffer MRHashTable
{
	float mrHashTable[];
};

// Output
layout(location = 0) out vec4 outColor;

// Constants
const vec3 skySize = vec3(125.0, 85.0, 153.0) / 2.0;
const vec3 skyPos = vec3(0.0);

#define PI 3.14159265359

#define MAX_RAY_DISTANCE 100000.0
#define MIN_RAY_DISTANCE 0.125

#define SAMPLE_COUNT 40

#define IMPORTANCE_SAMPLING

// Random
float preRand = volumeData.random.x * fragUV.x;
float prePreRand = volumeData.random.y * fragUV.y;

float rand(vec2 co)
{
	return fract(sin(dot(co, vec2(12.9898, 78.233))) * 43758.5453);
}

float myRand()
{
	float result = rand(vec2(preRand, prePreRand));
	prePreRand = preRand;
	preRand = result;
	return result;
}

float RandFloat(float maxVal)
{
	float f = myRand();
	return f * maxVal;
}

// MRHE helper

float GetMrheFeature(const uint level, const uint entryIndex, const uint featureIndex)
{
	const uint linearIndex = (mrhe.hashTableSize * mrhe.featureCount * level) + (entryIndex * mrhe.featureCount) + featureIndex;
	const float feature = mrHashTable[linearIndex];
	return feature;
}

uint HashFunc(const uvec3 pos)
{
	const uvec3 primes = uvec3(7919, 6491, 4099);
	uint hash = (pos.x * primes.x) + (pos.y * primes.y) + (pos.z * primes.z);
	hash %= mrhe.hashTableSize;
	return hash;
}

float mrheFeatures[32]; // 16 * 2

void EncodePosMrhe(const vec3 pos)
{
	const vec3 normPos = (pos / skySize) + vec3(0.5);

	for (uint level = 0; level < mrhe.levelCount; level++)
	{
		// Get level resolution
		const uint res = mrhe.resolutions[level];
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

		// Extract neighbour features
		vec2 neighbourFeatures[8];
		for (uint neigh = 0; neigh < 8; neigh++)
		{
			const uint entryIndex = HashFunc(uvec3(neighbours[neigh]));
			neighbourFeatures[neigh] = vec2(GetMrheFeature(level, entryIndex, 0), GetMrheFeature(level, entryIndex, 1));
		}

		// Linearly interpolate neightbour features
		vec3 lerpFactors = pos - neighbours[0];

		vec2 zLerpFeatures[4];
		for (uint i = 0; i < 4; i++)
		{
			zLerpFeatures[i] = 
				(neighbourFeatures[i] * lerpFactors.z) + 
				(neighbourFeatures[4 + i] * (1.0 - lerpFactors.z));
		}

		vec2 yLerpFeatures[2];
		for (uint i = 0; i < 2; i++)
		{
			yLerpFeatures[i] =
				(zLerpFeatures[i] * lerpFactors.y) +
				(zLerpFeatures[2 + i] * (1.0 - lerpFactors.y));
		}

		vec2 xLerpFeatures =
			(yLerpFeatures[0] * lerpFactors.x) +
			(yLerpFeatures[1] * (1.0 - lerpFactors.x));

		// Store in feature array
		mrheFeatures[(level * mrhe.featureCount) + 0] = xLerpFeatures.x;
		mrheFeatures[(level * mrhe.featureCount) + 1] = xLerpFeatures.y;
	}
}

// NN helper
float Sigmoid(float x)
{
	return 1.0 / (1.0 + exp(-x));
}

float Relu(float x)
{
	return max(0.0, x);
}

float nr0[5];
float nr1[64];
float nr2[64];
float nr3[64];
float nr4[64];
float nr5[64];
float nr6[3];

float GetWeight0(uint row, uint col)
{
	uint linearIndex = row * 5 + col;
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
		float sum = 0.0;
		
		for (uint inCol = 0; inCol < 5; inCol++)
		{
			float inVal = nr0[inCol];
			float weightVal = GetWeight0(outRow, inCol);
			sum += inVal * weightVal;
		}

		nr1[outRow] = sum;
	}
}

void ApplyWeights1()
{
	for (uint outRow = 0; outRow < 64; outRow++)
	{
		float sum = 0.0;
		
		for (uint inCol = 0; inCol < 64; inCol++)
		{
			float inVal = nr1[inCol];
			float weightVal = GetWeight1(outRow, inCol);
			sum += inVal * weightVal;
		}

		nr2[outRow] = sum;
	}
}

void ApplyWeights2()
{
	for (uint outRow = 0; outRow < 64; outRow++)
	{
		float sum = 0.0;
		
		for (uint inCol = 0; inCol < 64; inCol++)
		{
			float inVal = nr2[inCol];
			float weightVal = GetWeight2(outRow, inCol);
			sum += inVal * weightVal;
		}

		nr3[outRow] = sum;
	}
}

void ApplyWeights3()
{
	for (uint outRow = 0; outRow < 64; outRow++)
	{
		float sum = 0.0;
		
		for (uint inCol = 0; inCol < 64; inCol++)
		{
			float inVal = nr3[inCol];
			float weightVal = GetWeight3(outRow, inCol);
			sum += inVal * weightVal;
		}

		nr4[outRow] = sum;
	}
}

void ApplyWeights4()
{
	for (uint outRow = 0; outRow < 64; outRow++)
	{
		float sum = 0.0;
		
		for (uint inCol = 0; inCol < 64; inCol++)
		{
			float inVal = nr4[inCol];
			float weightVal = GetWeight4(outRow, inCol);
			sum += inVal * weightVal;
		}

		nr5[outRow] = sum;
	}
}

void ApplyWeights5()
{
	for (uint outRow = 0; outRow < 3; outRow++)
	{
		float sum = 0.0;
		
		for (uint inCol = 0; inCol < 64; inCol++)
		{
			float inVal = nr5[inCol];
			float weightVal = GetWeight5(outRow, inCol);
			sum += inVal * weightVal;
		}

		nr6[outRow] = sum;
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
	for (uint i = 0; i < 3; i++)
	{
		//nr6[i] = Sigmoid(nr6[i]);
		nr6[i] = Relu(nr6[i]);
	}
}

void EncodeRay(vec3 pos, const vec3 dir)
{
	pos /= skySize.y;
	const float theta = atan(dir.y, dir.x);
	const float phi = atan(length(dir.xy), dir.z);

	nr0[0] = pos.x;
	nr0[1] = pos.y;
	nr0[2] = pos.z;
	nr0[3] = theta;
	nr0[4] = phi;
}

vec3 Forward(vec3 ro, const vec3 rd)
{
	EncodePosMrhe(ro);
	EncodeRay(ro, rd);

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

	vec3 outputCol;
	outputCol.x = max(0.0, nr6[0]);
	outputCol.y = max(0.0, nr6[1]);
	outputCol.z = max(0.0, nr6[2]);

	//debugPrintfEXT("%f, %f, %f\n", nr6[0], nr6[1], nr6[2]);

	return outputCol;
}

// Path trace helper
float sky_sdf(vec3 pos)
{
	vec3 d = abs(pos - skyPos) - skySize / 2;
	return length(max(d, 0)) + min(max(d.x, max(d.y, d.z)), 0);
}

vec3[2] find_entry_exit(vec3 ro, vec3 rd)
{
	// rd should be normalized

	float dist;
	do
	{
		dist = sky_sdf(ro);
		ro  += dist * rd;
	} while (dist > MIN_RAY_DISTANCE && dist < MAX_RAY_DISTANCE);
	vec3 entry = ro;

	ro += rd * length(2 * skySize);//ro += rd * MAX_RAY_DISTANCE * 2;
	rd *= -1.0;
	do
	{
		dist = sky_sdf(ro);
		ro += dist * rd;
	} while (dist > MIN_RAY_DISTANCE && dist < MAX_RAY_DISTANCE);
	vec3 exit = ro;
	
	return vec3[2]( entry, exit );
}

void gen_sample_points(vec3 start_pos, vec3 end_pos, out vec3 samples[SAMPLE_COUNT])
{
	vec3 dir = end_pos - start_pos;
	for (int i = 0; i < SAMPLE_COUNT; i++)
		samples[i] = start_pos + dir * (float(i) / float(SAMPLE_COUNT));
}

vec3 get_sky_uvw(vec3 pos)
{
	return ((pos - skyPos) / skySize) + vec3(0.5);
}

float getDensity(vec3 pos)
{
	return volumeData.densityFactor * texture(densityTex, get_sky_uvw(pos)).x;
}

float hg_phase_func(const float cos_theta)
{
#ifdef IMPORTANCE_SAMPLING
	return 1.0;
#else
	const float g = volumeData.g;
	const float g2 = g * g;
	const float result = 0.5 * (1 - g2) / pow(1 + g2 - (2 * g * cos_theta), 1.5);
	return result;
#endif
}

mat4 rotationMatrix(vec3 axis, float angle)
{
    axis = normalize(axis);
    float s = sin(angle);
    float c = cos(angle);
    float oc = 1.0 - c;
    
    return mat4(oc * axis.x * axis.x + c,           oc * axis.x * axis.y - axis.z * s,  oc * axis.z * axis.x + axis.y * s,  0.0,
                oc * axis.x * axis.y + axis.z * s,  oc * axis.y * axis.y + c,           oc * axis.y * axis.z - axis.x * s,  0.0,
                oc * axis.z * axis.x - axis.y * s,  oc * axis.y * axis.z + axis.x * s,  oc * axis.z * axis.z + c,           0.0,
                0.0,                                0.0,                                0.0,                                1.0);
}

vec3 NewRayDir(vec3 oldRayDir)
{
	// Assert: length(oldRayDir) == 1.0
	// Assert: rand's are in [0.0, 1.0]

	oldRayDir = normalize(oldRayDir);

	// Get any orthogonal vector
	//return  c<a  ? (b,-a,0) : (0,-c,b)
	vec3 orthoDir = oldRayDir.z < oldRayDir.x ? vec3(oldRayDir.y, -oldRayDir.x, 0.0) : vec3(0.0, -oldRayDir.z, oldRayDir.y);
	orthoDir = normalize(orthoDir);

	// Rotate around that orthoDir
#ifdef IMPORTANCE_SAMPLING
	float g = volumeData.g;
	float cosTheta;
	if (abs(g) < 0.001)
    {
		cosTheta = 1 - 2 * RandFloat(1.0);
	}
	else
	{
		float sqrTerm = (1 - g * g) / (1 - g + (2 * g * RandFloat(1.0)));
		cosTheta = (1 + (g * g) - (sqrTerm * sqrTerm)) / (2 * g);
	}
	float angle = acos(cosTheta);
#else
	float angle = RandFloat(PI);
#endif
	mat4 rotMat = rotationMatrix(orthoDir, angle);
	vec3 newRayDir = (rotMat * vec4(oldRayDir, 1.0)).xyz;

	// Rotate around oldRayDir
	angle = RandFloat(2.0 * PI);
	rotMat = rotationMatrix(oldRayDir, angle);
	newRayDir = (rotMat * vec4(newRayDir, 1.0)).xyz;

	return normalize(newRayDir);
}

float GetTransmittance(const vec3 start, const vec3 end, const uint count)
{
	const vec3 dir = end - start;
	const float stepSize = length(dir) / float(count);

	if (stepSize == 0.0)
	{
		return 1.0;
	}

	float transmittance = 1.0;
	for (uint i = 0; i < count; i++)
	{
		const float factor = float(i) / float(count);
		const vec3 samplePoint = start + (factor * dir);
		const float density = getDensity(samplePoint);
		const float t_r = exp(-density * stepSize);
		transmittance *= t_r;
	}

	return transmittance;
}

vec3 TraceDirLight(const vec3 pos, const vec3 dir)
{
	if (dir_light.strength == 0.0)
	{
		return vec3(0.0);
	}

	const float transmittance = GetTransmittance(pos, find_entry_exit(pos, -normalize(dir_light.dir))[1], 32);
	const float phase = hg_phase_func(dot(dir_light.dir, -dir));
	const vec3 dirLighting = vec3(1.0f) * transmittance * dir_light.strength * phase;
	return dirLighting;
}

vec3 TracePointLight(const vec3 pos, const vec3 dir)
{
	if (pointLight.strength == 0.0)
	{
		return vec3(0.0);
	}

	const float transmittance = GetTransmittance(pointLight.pos, pos, 32);
	const float phase = hg_phase_func(dot(normalize(pointLight.pos - pos), -dir));
	const vec3 pointLighting = pointLight.color * pointLight.strength * transmittance * phase;
	return pointLighting;
}

vec3 SampleHdrEnvMap(const vec3 dir, const bool hpm)
{
	// Assert: dir is normalized

	const vec2 invAtan = vec2(0.1591, 0.3183);

	vec2 uv = vec2(atan(dir.z, dir.x), asin(dir.y));
    uv *= invAtan;
    uv += 0.5;

	const float strength = hpm ? hdrEnvMapData.hpmStrength : hdrEnvMapData.directStrength;

	return texture(hdrEnvMap, uv).xyz * strength;
}

vec3 SampleHdrEnvMap(const vec3 pos, const vec3 dir, uint sampleCount)
{
	vec3 light = vec3(0.0);

	for (uint i = 0; i < sampleCount; i++)
	{
		const vec3 randomDir = NewRayDir(dir);
		const float phase = hg_phase_func(dot(randomDir, -dir));
		const vec3 exit = find_entry_exit(pos, randomDir)[1];
		const float transmittance = GetTransmittance(pos, exit, 32);
		const vec3 sampleLight = SampleHdrEnvMap(randomDir, true) * phase;

		light += sampleLight;
	}

	light /= float(sampleCount);

	return light;
}

vec3 TraceScene(const vec3 pos, const vec3 dir)
{
	const vec3 totalLight = TraceDirLight(pos, dir) + TracePointLight(pos, dir) * SampleHdrEnvMap(pos, dir, 16);
	return totalLight;
}

#define TRUE_TRACE_SAMPLE_COUNT 64
vec4 TracePath(const vec3 rayOrigin, const vec3 rayDir, bool useNN)
{	
	vec3 scatteredLight = vec3(0.0);
	float transmittance = 1.0;

	const vec3 entry = find_entry_exit(rayOrigin, rayDir)[0];
	
	vec3 currentPoint = entry;
	vec3 lastPoint = entry;
	
	vec3 currentDir = rayDir;
	vec3 lastDir = vec3(0.0);

	float totalTermProb = 1.0;

	for (uint i = 0; i < TRUE_TRACE_SAMPLE_COUNT; i++)
	{
		const float density = getDensity(currentPoint);

		if (density > 0.0)
		{
			if (useNN)
			{
				if (RandFloat(1.0) > totalTermProb)
				{
					const float dirPhase = hg_phase_func(dot(currentDir, -lastDir));
					scatteredLight += transmittance * Forward(currentPoint, currentDir) * dirPhase;
					return vec4(scatteredLight, transmittance);
				}
				totalTermProb *= 0.25;
			}

			// Get scene lighting
			const vec3 sceneLighting = TraceScene(currentPoint, currentDir);

			// Phase factor
			const float dirPhase = hg_phase_func(dot(currentDir, -lastDir));

			// Transmittance calculation
			const vec3 s_int = density * sceneLighting * dirPhase;
			const float t_r = GetTransmittance(currentPoint, lastPoint, 32);

			scatteredLight += transmittance * s_int;
			transmittance *= t_r;

			// Low transmittance early exit
			if (transmittance < 0.01)
			{
				break;
			}

			// Update last
			lastPoint = currentPoint;
			lastDir = currentDir;

			// Generate new direction
			currentDir = NewRayDir(currentDir);
		}

		// Generate new point
		const vec3 exit = find_entry_exit(currentPoint, currentDir)[1];
		const float maxDistance = distance(exit, currentPoint) * 0.1;
		const float nextDistance = RandFloat(maxDistance);
		currentPoint = currentPoint + (currentDir * nextDistance);
	}

	return vec4(scatteredLight, transmittance);
}

// Main
void main()
{
	// Setup
	const vec3 ro = camera.pos;
	const vec3 rd = normalize(pixelWorldPos - ro);

	// SDF
	const vec3[2] entry_exit = find_entry_exit(ro, rd);
	const vec3 entry = entry_exit[0];
	const vec3 exit = entry_exit[1];

	vec3 envMapColor = SampleHdrEnvMap(rd, false);

	if (sky_sdf(entry) > MAX_RAY_DISTANCE)
	{
		outColor = vec4(envMapColor, 1.0);
		return;
	}

	// Render
	const vec4 oldColor = texture(lowPassTex, fragUV);
	const vec4 traceResult = TracePath(ro, rd, volumeData.useNN == 1);
	const float transmittance = traceResult.w;

	if (transmittance == 1.0 && oldColor.w == 1.0)
	{
		outColor = vec4(envMapColor, 1.0);
		return;
	}

	envMapColor = SampleHdrEnvMap(rd, false);

	const float primaryRayTransmittance = GetTransmittance(entry, exit, 64);

	const vec4 newColor = vec4(traceResult.xyz + (envMapColor * primaryRayTransmittance), transmittance);

	const float lowPassIndex = float(volumeData.lowPassIndex);
	const float alpha = lowPassIndex / (lowPassIndex + 1.0);
	outColor = ((1.0 - alpha) * newColor) + (alpha * oldColor);
}
