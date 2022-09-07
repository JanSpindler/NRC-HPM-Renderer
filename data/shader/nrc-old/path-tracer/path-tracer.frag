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

layout(set = 2, binding = 0) uniform dir_light_t
{
	vec3 color;
	float zenith;
	vec3 dir;
	float azimuth;
	float strength;
} dir_light;

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
	bool hasSelectedInternalPath = false;

	for (uint i = 0; i < TRUE_TRACE_SAMPLE_COUNT; i++)
	{
		if (!hasSelectedInternalPath && RandFloat(1.0) > totalTermProb)
		{
			// Reset lighting
			transmittance = 1.0;
			scatteredLight = vec3(0.0);

			// Encode new ray
			float theta = atan(currentDir.y, currentDir.x);
			float phi = atan(sqrt((currentDir.x * currentDir.x) + (currentDir.y * currentDir.y)), currentDir.z);

			outPos = vec4(currentPoint / skySize.y, 1.0);
			outDir = vec4(theta, phi, 0.0, 1.0);

			// Store decision
			hasSelectedInternalPath = true;
		}
		totalTermProb *= 0.8;

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
			const vec3 s = sampleSigmaS * sunLight;// * dirPhase;
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
		const float nextDistance = RandFloat(maxDistance);
		currentPoint = currentPoint + (currentDir * nextDistance);
	}

	return scatteredLight;
}

vec3 TracePath(const vec3 rayOrigin, const vec3 rayDir)
{
	return TrueTracePath(rayOrigin, rayDir) * exp(volumeData.brightness);
}

// --------------- End: path trace -----------------

void main()
{
	const vec3 ro = camera.pos;
	const vec3 rd = normalize(pixelWorldPos - ro);

	// uniform sample cosangle
	float theta = atan(rd.y, rd.x);
	float phi = atan(sqrt((rd.x * rd.x) + (rd.y * rd.y)), rd.z);

	outPos = vec4(ro / skySize.y, 1.0);
	outDir = vec4(theta, phi, 0.0, 1.0);

	// Check sdf collision
	const vec3[2] entry_exit = find_entry_exit(ro, rd);
	const vec3 entry = entry_exit[0];
	const vec3 exit = entry_exit[1];

	if (sky_sdf(entry) > MAX_RAY_DISTANCE)
	{
		outColor = vec4(vec3(0.0), 1.0);
		return;
	}

	// Trace path
	vec4 newColor = vec4(TracePath(ro, rd), 1.0);
	outColor = newColor;
}
