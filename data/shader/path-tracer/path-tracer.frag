#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 pixelWorldPos;
layout(location = 1) in vec2 fragUV;

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

layout(location = 0) out vec4 outColor;
layout(location = 1) out vec4 outPos;
layout(location = 2) out vec4 outDir; // TODO: vec2

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

// --------------- Start: random ----------------

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

/*const uint64_t PCG32_DEFAULT_STATE  = 0x853c49e6748fea9bUL;
const uint64_t PCG32_DEFAULT_STREAM = 0xda3e39cb94b95bdbUL;
const uint64_t PCG32_MULT           = 0x5851f42d4c957f2dUL;

struct pcg32_t
{
	uint64_t state;
	uint64_t inc;
};

pcg32_t pcg32;

uint Pcg32NextUInt()
{
	uint64_t oldState = pcg32.state;
	pcg32.state = PCG32_MULT + pcg32.inc;
	uint xorShifted = uint(((oldState >> 18u) ^ oldState) >> 27u);
	uint rot = uint(oldState >> 59u);
	return (xorShifted >> rot) | (xorShifted << ((~rot + 1u) & 32));
}

uint Pcg32NextUInt(uint bound)
{
	uint threshold = (~bound + 1u) % bound;
	while (true)
	{
		uint r = Pcg32NextUInt();
		if (r >= threshold)
			return r % bound;
	}
}

float Pcg32NextFloat()
{
	uint u = (Pcg32NextUInt() >> 9) | 0x3f800000u;
	vec2 f = unpackHalf2x16(u);
	return rand(f);
}

void Pcg32Seed()
{
	uint64_t initstate = PCG32_DEFAULT_STATE;
	uint64_t initseq = PCG32_DEFAULT_STREAM;

	pcg32.state = packHalf2x16(vec2(myRand(), myRand()));
	//pcg32.state = 0U;
	pcg32.inc = (initseq << 1u) | 1u;
	Pcg32NextUInt();
	pcg32.state += initstate;
	Pcg32NextUInt();
}*/

float RandFloat(float maxVal)
{
	float f = myRand();
	return f * maxVal;
}

// --------------- End: random ----------------

// --------------- Start: util -----------------

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

// --------------- End: util -----------------

// --------------- Start: single scatter -----------------

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

// x-z = scattered light | z = transmittance
vec4 render_cloud(vec3 sample_points[SAMPLE_COUNT], vec3 out_dir, vec3 ro)
{	
	// out_dir should be normalized
	// SAMPLE_COUNT should be > 0

	vec3 scattered_light = vec3(0.0);
	float transmittance = 1.0;

	const float sigma_s = SIGMA_S;
	const float sigma_e = SIGMA_E;
	const float step_size = length(sample_points[1] - sample_points[0]);
	const vec3 step_offset = RandFloat(step_size) * out_dir;

	bool depth_stored = false;
	for (int i = 0; i < SAMPLE_COUNT; i++)
	{
		const vec3 sample_point = sample_points[i] + step_offset;
		const float density = getDensity(sample_point);

		if (density > 0.0)
		{
			const float sample_sigma_s = density * sigma_s;
			const float sample_sigma_e = density * sigma_e;

			const float phase_result = hg_phase_func(dot(dir_light.dir, out_dir));
			const float self_shadowing = get_self_shadowing(sample_point);

			const vec3 s = vec3(self_shadowing * phase_result) * sample_sigma_s * dir_light.strength;
			const float t_r = exp(-sample_sigma_e * step_size);
			const vec3 s_int = (s - (s * t_r)) / sample_sigma_e;

			scattered_light += transmittance * s_int;
			transmittance *= t_r;
			
			// Early exit
			if (transmittance < 0.01)
				break;
		}
	}

	return vec4(scattered_light, transmittance);
}

// --------------- End: util -----------------

// --------------- Start: Path trace -----------------

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
	float angle = RandFloat(PI);
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
			const vec3 incommingLight = vec3(get_self_shadowing(samplePoint));
			//const vec3 incommingLight = TracePath3(samplePoint);

			const vec3 s = incommingLight * phase * density * sampleSigmaS;
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
			const vec3 totalIncomingLight = randomLight + sunLight;

			// Transmittance calculation
			const vec3 s = sampleSigmaS * totalIncomingLight * density;// / prob;
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

			// Combine incomming light
			const vec3 totalIncomingLight = randomLight + sunLight;

			// Transmittance calculation
			const vec3 s = sampleSigmaS * totalIncomingLight * density;// / prob;
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

vec3 TracePath(const vec3 rayOrigin, const vec3 rayDir)
{
	return TracePath0(rayOrigin, rayDir) * exp(volumeData.brightness);
}

// --------------- End: path trace -----------------

// --------------- Main -----------------

void main()
{
	vec3 ro = camera.pos;
	const vec3 rd = normalize(pixelWorldPos - ro);

	outPos = vec4(ro, 1.0);
	outDir = vec4(rd.xy, 0.0, 1.0);

	const vec3[2] entry_exit = find_entry_exit(ro, rd);
	const vec3 entry = entry_exit[0];
	const vec3 exit = entry_exit[1];

	if (sky_sdf(entry) > MAX_RAY_DISTANCE)
	{
		outColor = vec4(vec3(0.0), 1.0);
		return;
	}

	vec3[SAMPLE_COUNT] samplePoints;
	gen_sample_points(entry, exit, samplePoints);

	if (volumeData.singleScatter > 0)
	{
		vec4 result = render_cloud(samplePoints, -rd, ro);
		outColor = vec4(result.xyz, 1.0 - result.w);
		return;
	}
	else
	{
		float lowPassIndex = float(volumeData.lowPassIndex);
		float alpha = lowPassIndex / (lowPassIndex + 1.0);
		vec4 oldColor = texture(lowPassTex, fragUV);
		vec4 newColor = vec4(TracePath(ro, rd), 1.0);
		outColor = ((1.0 - alpha) * newColor) + (alpha * oldColor);
		return;
	}
}
