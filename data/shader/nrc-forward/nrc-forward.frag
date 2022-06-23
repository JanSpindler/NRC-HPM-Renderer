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
	uint singleScatter;
	float densityFactor;
	float g;
	float sigmaS;
	float sigmaE;
	float brightness;
	uint lowPassIndex;
} volumeData;

layout(set = 2, binding = 0) uniform dir_light_t // TODO: raname to sun_t
{
	vec3 color;
	float zenith;
	vec3 dir;
	float azimuth;
	float strength;
} dir_light;

layout(set = 3, binding = 0) uniform sampler2D lowPassTex;

// NN buffers
layout(std140, set = 4, binding = 0) readonly buffer Weights0
{
	float matWeights0[320]; // 5 x 64
};

layout(std140, set = 4, binding = 1) readonly buffer Weights1
{
	float matWeights1[4096]; // 64 x 64
};

layout(std140, set = 4, binding = 2) readonly buffer Weights2
{
	float matWeights2[4096]; // 64 x 64
};

layout(std140, set = 4, binding = 3) readonly buffer Weights3
{
	float matWeights3[4096]; // 64 x 64
};

layout(std140, set = 4, binding = 4) readonly buffer Weights4
{
	float matWeights4[4096]; // 64 x 64
};

layout(std140, set = 4, binding = 5) readonly buffer Weights5
{
	float matWeights5[256]; // 64 x 4
};

layout(std140, set = 4, binding = 6) readonly buffer Biases0
{
	float matBiases0[64];
};

layout(std140, set = 4, binding = 7) readonly buffer Biases1
{
	float matBiases1[64];
};

layout(std140, set = 4, binding = 8) readonly buffer Biases2
{
	float matBiases2[64];
};

layout(std140, set = 4, binding = 9) readonly buffer Biases3
{
	float matBiases3[64];
};

layout(std140, set = 4, binding = 10) readonly buffer Biases4
{
	float matBiases4[64];
};

layout(std140, set = 4, binding = 11) readonly buffer Biases5
{
	float matBiases5[64];
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
#define SECONDARY_SAMPLE_COUNT 12

#define SAMPLE_COUNT0 16
#define SAMPLE_COUNT1 16
#define SAMPLE_COUNT2 8
#define SAMPLE_COUNT3 8

#define SIGMA_S volumeData.sigmaS
#define SIGMA_E volumeData.sigmaE

#define NRC_PROB 0.5

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

// NN helper
float Sigmoid(float x)
{
	return 1.0 / (1.0 + exp(-x));
}

float Relu(float x)
{
	return x > 0.0 ? x : x * 0.01;
}

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

void ApplyWeights0(in float[5] nr0, out float[64] nr1)
{
	for (uint outRow = 0; outRow < 64; outRow++)
	{
		float sum = 0;
		
		for (uint inCol = 0; inCol < 5; inCol++)
		{
			float inVal = nr0[inCol];
			float weightVal = GetWeight0(outRow, inCol);
			sum += inVal * weightVal;
		}

		nr1[outRow] = sum;
	}
}

void ApplyWeights1(in float[64] nr1, out float[64] nr2)
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

		nr2[outRow] = sum;
	}
}

void ApplyWeights2(in float[64] nr2, out float[64] nr3)
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

		nr3[outRow] = sum;
	}
}

void ApplyWeights3(in float[64] nr3, out float[64] nr4)
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

		nr4[outRow] = sum;
	}
}

void ApplyWeights4(in float[64] nr4, out float[64] nr5)
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

		nr5[outRow] = sum;
	}
}

void ApplyWeights5(in float[64] nr5, out float[4] nr6)
{
	for (uint outRow = 0; outRow < 4; outRow++)
	{
		float sum = 0;
		
		for (uint inCol = 0; inCol < 64; inCol++)
		{
			float inVal = nr5[inCol];
			float weightVal = GetWeight5(outRow, inCol);
			sum += inVal * weightVal;
		}

		nr6[outRow] = sum;
	}
}

void AddBiases0(inout float[64] nr1)
{
	for (uint i = 0; i < 64; i++)
	{
		nr1[i] += matBiases0[i];
	}
}

void AddBiases1(inout float[64] nr2)
{
	for (uint i = 0; i < 64; i++)
	{
		nr2[i] += matBiases1[i];
	}
}

void AddBiases2(inout float[64] nr3)
{
	for (uint i = 0; i < 64; i++)
	{
		nr3[i] += matBiases2[i];
	}
}

void AddBiases3(inout float[64] nr4)
{
	for (uint i = 0; i < 64; i++)
	{
		nr4[i] += matBiases3[i];
	}
}

void AddBiases4(inout float[64] nr5)
{
	for (uint i = 0; i < 64; i++)
	{
		nr5[i] += matBiases4[i];
	}
}

void AddBiases5(inout float[4] nr6)
{
	for (uint i = 0; i < 4; i++)
	{
		nr6[i] += matBiases5[i];
	}
}

void ActivateNr1(inout float[64] nr1)
{
	for (uint i = 0; i < 64; i++)
	{
		//nr1[i] = Sigmoid(nr1[i]);
		nr1[i] = Relu(nr1[i]);
	}
}

void ActivateNr2(inout float[64] nr2)
{
	for (uint i = 0; i < 64; i++)
	{
		//nr2[i] = Sigmoid(nr2[i]);
		nr2[i] = Relu(nr2[i]);
	}
}

void ActivateNr3(inout float[64] nr3)
{
	for (uint i = 0; i < 64; i++)
	{
		//nr3[i] = Sigmoid(nr3[i]);
		nr3[i] = Relu(nr3[i]);
	}
}

void ActivateNr4(inout float[64] nr4)
{
	for (uint i = 0; i < 64; i++)
	{
		//nr4[i] = Sigmoid(nr4[i]);
		nr4[i] = Relu(nr4[i]);
	}
}

void ActivateNr5(inout float[64] nr5)
{
	for (uint i = 0; i < 64; i++)
	{
		//nr5[i] = Sigmoid(nr5[i]);
		nr5[i] = Relu(nr5[i]);
	}
}

void ActivateNr6(inout float[4] nr6)
{
	for (uint i = 0; i < 4; i++)
	{
		//nr6[i] = Sigmoid(nr6[i]);
		nr6[i] = Relu(nr6[i]);
	}
}

vec4 Forward(vec3 ro, const vec3 rd)
{
	ro /= skySize.y;

	const float theta = atan(rd.y, rd.x);
	const float phi = atan(sqrt(rd.x * rd.x + rd.y * rd.y), rd.z);

	const float nr0[5] = float[]( ro.x, ro.y, ro.z, theta, phi );
	float nr1[64];
	float nr2[64];
	float nr3[64];
	float nr4[64];
	float nr5[64];
	float nr6[4];

	ApplyWeights0(nr0, nr1);
	//AddBiases0(nr1);
	ActivateNr1(nr1);
	
	ApplyWeights1(nr1, nr2);
	//AddBiases1(nr2);
	ActivateNr2(nr2);
	
	ApplyWeights2(nr2, nr3);
	//AddBiases2(nr3);
	ActivateNr3(nr3);
	
	ApplyWeights3(nr3, nr4);
	//AddBiases3(nr4);
	ActivateNr4(nr4);

	ApplyWeights4(nr4, nr5);
	//AddBiases4(nr5);
	ActivateNr5(nr5);
	
	ApplyWeights5(nr5, nr6);
	//AddBiases5(nr6);
	ActivateNr6(nr6);

	vec4 outputCol;
	outputCol.x = nr6[0];
	outputCol.y = nr6[1];
	outputCol.z = nr6[2];
	outputCol.w = nr6[3];

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
	const float g = volumeData.g;
	const float g2 = g * g;
	const float result = 0.5 * (1 - g2) / pow(1 + g2 - (2 * g * cos_theta), 1.5);
	return result;
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

// Path trace
float get_self_shadowing(vec3 pos)
{
	// Exit if not used
	if (SECONDARY_SAMPLE_COUNT == 0)
		return 1.0;

	// Find exit from current pos
	const vec3 exit = find_entry_exit(pos, -normalize(dir_light.dir))[1];

	// Generate secondary sample points using lerp factors
	const vec3 direction = exit - pos;
	vec3 secondary_sample_points[SECONDARY_SAMPLE_COUNT];
	for (int i = 0; i < SECONDARY_SAMPLE_COUNT; i++)
		secondary_sample_points[i] = pos + direction * exp(float(i - SECONDARY_SAMPLE_COUNT));

	// Calculate light reaching pos
	float transmittance = 1.0;
	const float sigma_e = SIGMA_E;
	for (int i = 0; i < SECONDARY_SAMPLE_COUNT; i++)
	{
		const vec3 sample_point = secondary_sample_points[i];
		const float density = getDensity(sample_point);
		if (density > 0.0)
		{
			const float sample_sigma_e = sigma_e * density;
			const float step_size = (i < SECONDARY_SAMPLE_COUNT - 1) ? 
				length(secondary_sample_points[i + 1] - secondary_sample_points[i]) : 
				1.0;
			const float t_r = exp(-sample_sigma_e * step_size);
			transmittance *= t_r;
		}
	}

	return transmittance;
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
	//float angle = RandFloat(PI);
	mat4 rotMat = rotationMatrix(orthoDir, angle);
	vec3 newRayDir = (rotMat * vec4(oldRayDir, 1.0)).xyz;

	// Rotate around oldRayDir
	angle = RandFloat(2.0 * PI);
	rotMat = rotationMatrix(oldRayDir, angle);
	newRayDir = (rotMat * vec4(newRayDir, 1.0)).xyz;

	return normalize(newRayDir);
}

vec3 TracePath3(vec3 rayOrigin)
{
	const vec3 rayDir = normalize(-dir_light.dir);
	
	float transmittance = 1.0;

	const vec3[2] entry_exit = find_entry_exit(rayOrigin, rayDir);
	const vec3 entry = entry_exit[0];
	const vec3 exit = entry_exit[1];

	vec3 samplePoints[SAMPLE_COUNT3];
	vec3 dir = exit - entry;
	for (int i = 0; i < SAMPLE_COUNT3; i++)
		samplePoints[i] = entry + dir * (float(i) / float(SAMPLE_COUNT3));

	const float stepSize = length(samplePoints[1] - samplePoints[0]);

	for (uint i = 0; i < SAMPLE_COUNT3; i++)
	{
		const vec3 samplePoint = samplePoints[i];
		const float density = getDensity(samplePoint);

		const float sampleSigmaE = density * SIGMA_E;

		if (density > 0.0)
		{
			transmittance *= exp(-sampleSigmaE * stepSize);
			if (transmittance < 0.01)
				break;
		}
	}

	return vec3(transmittance * dir_light.strength);
}

vec3 TracePath2(vec3 rayOrigin, vec3 rayDir)
{
	rayDir = normalize(rayDir);
	
	vec3 scatteredLight = vec3(0.0);
	float transmittance = 1.0;

	const vec3[2] entry_exit = find_entry_exit(rayOrigin, rayDir);
	const vec3 entry = entry_exit[0];
	const vec3 exit = entry_exit[1];

	vec3 samplePoints[SAMPLE_COUNT2];
	vec3 dir = exit - entry;
	for (int i = 0; i < SAMPLE_COUNT2; i++)
		samplePoints[i] = entry + dir * (float(i) / float(SAMPLE_COUNT2));

	const float stepSize = length(samplePoints[1] - samplePoints[0]);
	const vec3 step_offset = RandFloat(stepSize) * rayDir;

	for (uint i = 0; i < SAMPLE_COUNT2; i++)
	{
		const vec3 samplePoint = samplePoints[i] + step_offset;
		const float density = getDensity(samplePoint);

		if (density > 0.0)
		{
			const float sampleSigmaS = density * SIGMA_S;
			const float sampleSigmaE = density * SIGMA_E;

			const float phase = hg_phase_func(dot(dir_light.dir, -rayDir));
			//const vec3 incommingLight = vec3(get_self_shadowing(samplePoint));
			const vec3 incommingLight = TracePath3(samplePoint) * phase;

			const vec3 s = incommingLight * sampleSigmaS;
			const float t_r = exp(-sampleSigmaE * stepSize);
			const vec3 s_int = (s - (s * t_r)) / sampleSigmaE;

			scatteredLight += transmittance * s_int;
			transmittance *= t_r;
			
			// Early exit
			if (transmittance < 0.01)
				break;
		}
	}

	return scatteredLight;
}

vec3 TracePath1(vec3 rayOrigin, vec3 rayDir)
{
	rayDir = normalize(rayDir);
	
	vec3 scatteredLight = vec3(0.0);
	float transmittance = 1.0;

	const vec3[2] entry_exit = find_entry_exit(rayOrigin, rayDir);
	const vec3 entry = entry_exit[0];
	const vec3 exit = entry_exit[1];

	vec3 samplePoints[SAMPLE_COUNT1];
	vec3 dir = exit - entry;
	for (int i = 0; i < SAMPLE_COUNT1; i++)
		samplePoints[i] = entry + dir * (float(i) / float(SAMPLE_COUNT1));

	const float stepSize = length(samplePoints[1] - samplePoints[0]);
	const vec3 step_offset = RandFloat(stepSize) * rayDir;

	for (uint i = 0; i < SAMPLE_COUNT1; i++)
	{
		const vec3 samplePoint = samplePoints[i] + step_offset;
		const float density = getDensity(samplePoint);

		if (density > 0.0)
		{
			// Sigmas
			const float sampleSigmaS = density * SIGMA_S;
			const float sampleSigmaE = density * SIGMA_E;

			// Incoming light directly from sun
			const float sunPhase = hg_phase_func(dot(dir_light.dir, -rayDir));
			const vec3 sunLight = vec3(get_self_shadowing(samplePoint)) * sunPhase;

			// Incoming light from random dir
			vec3 randomDir = NewRayDir(rayDir);
			const float prob = 1.0 / (4.0 * PI * PI);
			const float randomPhase = hg_phase_func(dot(randomDir, -rayDir));
			const vec3 randomLight = TracePath2(samplePoint, randomDir) * randomPhase;

			// Combine incomming light
			const vec3 totalIncomingLight = (randomLight + sunLight) * 0.5;

			// Transmittance calculation
			const vec3 s = sampleSigmaS * totalIncomingLight;
			const float t_r = exp(-sampleSigmaE * stepSize);
			const vec3 s_int = (s - (s * t_r)) / sampleSigmaE;

			scatteredLight += transmittance * s_int;
			transmittance *= t_r;
			
			// Early exit
			if (transmittance < 0.01)
				break;
		}
	}

	return scatteredLight;
}

vec3 TracePath0(const vec3 rayOrigin, vec3 rayDir)
{
	rayDir = normalize(rayDir);
	
	vec3 scatteredLight = vec3(0.0);
	float transmittance = 1.0;

	const vec3[2] entry_exit = find_entry_exit(rayOrigin, rayDir);
	const vec3 entry = entry_exit[0];
	const vec3 exit = entry_exit[1];

	vec3 samplePoints[SAMPLE_COUNT0];
	vec3 dir = exit - entry;
	for (int i = 0; i < SAMPLE_COUNT0; i++)
		samplePoints[i] = entry + dir * (float(i) / float(SAMPLE_COUNT0));

	const float stepSize = length(samplePoints[1] - samplePoints[0]);
	const vec3 step_offset = RandFloat(stepSize) * rayDir;

	for (uint i = 0; i < SAMPLE_COUNT0; i++)
	{
		const vec3 samplePoint = samplePoints[i] + step_offset;
		const float density = getDensity(samplePoint);

		if (density > 0.0)
		{
			// Sigmas
			const float sampleSigmaS = density * SIGMA_S;
			const float sampleSigmaE = density * SIGMA_E;

			// Incoming light directly from sun
			const float sunPhase = hg_phase_func(dot(dir_light.dir, -rayDir));
			const vec3 sunLight = vec3(get_self_shadowing(samplePoint)) * sunPhase;

			// Incoming light from random dir
			vec3 randomDir = NewRayDir(rayDir);
			const float prob = 1.0 / (4.0 * PI * PI);
			const float randomPhase = hg_phase_func(dot(randomDir, -rayDir));
			const vec3 randomLight = TracePath1(samplePoint, randomDir) * randomPhase;
			//const vec3 randomLight = Forward(samplePoint, randomDir).xyz * randomPhase;
			// Combine incomming light
			const vec3 totalIncomingLight = (randomLight + sunLight) * 0.5;

			// Transmittance calculation
			const vec3 s = sampleSigmaS * totalIncomingLight;
			const float t_r = exp(-sampleSigmaE * stepSize);
			const vec3 s_int = (s - (s * t_r)) / sampleSigmaE;

			scatteredLight += transmittance * s_int;
			transmittance *= t_r;

			// Low transmittance early exit
			if (transmittance < 0.01)
				break;
		}
	}

	return scatteredLight;
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

		const float sampleSigmaE = density * SIGMA_E;
		const float t_r = exp(-sampleSigmaE * stepSize);
		transmittance *= t_r;
	}

	return transmittance;
}

#define TRUE_TRACE_SAMPLE_COUNT 32
vec3 TrueTracePath(const vec3 rayOrigin, const vec3 rayDir)
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
		if (RandFloat(1.0) > totalTermProb)
		{
			scatteredLight += transmittance * Forward(currentPoint, currentDir).xyz;
			return scatteredLight;
		}
		totalTermProb *= 0.9999;

		const float density = getDensity(currentPoint);

		if (density > 0.0)
		{
			// Sigmas
			const float sampleSigmaS = density * SIGMA_S;
			const float sampleSigmaE = density * SIGMA_E;

			// Incoming light directly from sun
			const float sunPhase = hg_phase_func(dot(dir_light.dir, -currentDir));
			const vec3 sunLight = vec3(get_self_shadowing(currentPoint)) * sunPhase;

			// Phase factor
			const float dirPhase = hg_phase_func(dot(currentDir, -lastDir));

			// Transmittance calculation
			const vec3 s = sampleSigmaS * sunLight * dirPhase;
			const float t_r = GetTransmittance(currentPoint, lastPoint, 16);
			const vec3 s_int = (s * (1.0 - t_r)) / sampleSigmaE; // TODO: sampleSigmeE not representative

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
		}

		// Generate new point
		currentDir = NewRayDir(currentDir);
		const vec3 exit = find_entry_exit(currentPoint, currentDir)[1];
		const float maxDistance = distance(exit, currentPoint);
		const float nextDistance = RandFloat(maxDistance);
		currentPoint = currentPoint + (currentDir * nextDistance);
	}

	return scatteredLight;
}

vec3 TracePath(const vec3 rayOrigin, const vec3 rayDir)
{
	if (volumeData.singleScatter != 1)
	{
		return TrueTracePath(rayOrigin, rayDir) * exp(volumeData.brightness);
	}

	return TracePath0(rayOrigin, rayDir) * exp(volumeData.brightness);
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

	if (sky_sdf(entry) > MAX_RAY_DISTANCE)
	{
		outColor = vec4(vec3(0.0), 1.0);
		return;
	}

	// Render
	float lowPassIndex = float(volumeData.lowPassIndex);
	float alpha = lowPassIndex / (lowPassIndex + 1.0);
	vec4 oldColor = texture(lowPassTex, fragUV);
	vec4 newColor = vec4(TracePath(ro, rd), 1.0);
	outColor = ((1.0 - alpha) * newColor) + (alpha * oldColor);

	//outColor = vec4(Forward(ro, rd).xyz, 1.0);
}
