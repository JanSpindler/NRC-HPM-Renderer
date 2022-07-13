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
	float sigmaS;
	float sigmaE;
	float brightness;
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

// Output
layout(location = 0) out vec4 outColor;

// Constants
const vec3 skySize = vec3(125.0, 85.0, 153.0) / 2.0;
const vec3 skyPos = vec3(0.0);

#define PI 3.14159265359

#define MAX_RAY_DISTANCE 100000.0
#define MIN_RAY_DISTANCE 0.125

#define SAMPLE_COUNT 40
#define SECONDARY_SAMPLE_COUNT 32

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

	debugPrintfEXT("%f, %f, %f\n", nr6[0], nr6[1], nr6[2]);

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
//	float g = volumeData.g;
//	float cosTheta;
//	if (abs(g) < 0.001)
//    {
//		cosTheta = 1 - 2 * RandFloat(1.0);
//	}
//	else
//	{
//		float sqrTerm = (1 - g * g) / (1 - g + (2 * g * RandFloat(1.0)));
//		cosTheta = (1 + (g * g) - (sqrTerm * sqrTerm)) / (2 * g);
//	}
//	float angle = acos(cosTheta);
	float angle = RandFloat(PI);
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

		const float sampleSigmaE = density * SIGMA_E;
		const float t_r = exp(-sampleSigmaE * stepSize);
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

	const float phase = hg_phase_func(dot(dir_light.dir, -dir));
	const vec3 dirLighting = vec3(get_self_shadowing(pos)) * dir_light.strength * phase;
	return dirLighting;
}

vec3 TracePointLight(const vec3 pos, const vec3 dir)
{
	if (pointLight.strength == 0.0)
	{
		return vec3(0.0);
	}

	const float transmittance = GetTransmittance(pointLight.pos, pos, 16);
	const float phase = hg_phase_func(dot(normalize(pointLight.pos - pos), -dir));
	const vec3 pointLighting = pointLight.color * pointLight.strength * transmittance * phase;
	return pointLighting;
}

vec3 SampleHdrEnvMap(const vec3 dir)
{
	// Assert: dir is normalized

	const vec2 invAtan = vec2(0.1591, 0.3183);

	vec2 uv = vec2(atan(dir.z, dir.x), asin(dir.y));
    uv *= invAtan;
    uv += 0.5;

	return texture(hdrEnvMap, uv).xyz;
}

vec3 SampleHdrEnvMap(uint sampleCount)
{
	return vec3(0.0);
}

vec3 TraceScene(const vec3 pos, const vec3 dir)
{
	const vec3 totalLight = TraceDirLight(pos, dir) + TracePointLight(pos, dir);
	return totalLight;
}

#define TRUE_TRACE_SAMPLE_COUNT 128
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
				totalTermProb *= 0.5;
			}

			// Sigmas
			const float sampleSigmaS = density * SIGMA_S;
			const float sampleSigmaE = density * SIGMA_E;

			// Get scene lighting
			const vec3 sceneLighting = TraceScene(currentPoint, currentDir);

			// Phase factor
			const float dirPhase = hg_phase_func(dot(currentDir, -lastDir));

			// Transmittance calculation
			const vec3 s = sampleSigmaS * sceneLighting * dirPhase; // Importance Sampling phase
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

			// Generate new direction
			currentDir = NewRayDir(currentDir);
		}

		// Generate new point
		const vec3 exit = find_entry_exit(currentPoint, currentDir)[1];
		const float maxDistance = distance(exit, currentPoint);
		const float nextDistance = maxDistance * exp(-RandFloat(1.0));//RandFloat(maxDistance);
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

	if (sky_sdf(entry) > MAX_RAY_DISTANCE)
	{
		//outColor = vec4(vec3(0.0), 1.0);
		outColor = vec4(SampleHdrEnvMap(rd), 1.0);
		return;
	}

	// Render
	const vec4 oldColor = texture(lowPassTex, fragUV);
	const vec4 traceResult = TracePath(ro, rd, volumeData.useNN == 1);
	const float transmittance = traceResult.w;

	if (transmittance == 1.0 && oldColor.w == 1.0)
	{
		outColor = vec4(SampleHdrEnvMap(rd), 1.0);
		return;
	}

	const float lowPassIndex = float(volumeData.lowPassIndex);
	const float alpha = lowPassIndex / (lowPassIndex + 1.0);
	outColor = ((1.0 - alpha) * traceResult) + (alpha * oldColor);
}
