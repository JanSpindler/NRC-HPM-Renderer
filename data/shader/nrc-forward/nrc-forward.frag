#version 460
#extension GL_ARB_separate_shader_objects : enable

#include "common.glsl"

// Inputs
layout(location = 0) in vec3 pixelWorldPos;
layout(location = 1) in vec2 fragUV;

// Output
layout(location = 0) out vec4 outColor;

// Encode dir
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

void EncodeRay(vec3 pos, const vec3 dir)
{
	EncodePosMrhe(pos);
	EncodeDirOneBlob(dir);

	for (uint i = 0; i < 32; i++)
	{
		nr0[i] = mrheFeatures[i];
		nr0[i + 32] = oneBlobFeatures[i];
	}
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

vec3 SampleHdrEnvMap(const vec2 dir, const bool hpm)
{
	// Assert: dir is normalized
	const vec2 invAtan = vec2(0.1591, 0.3183);

	vec2 uv = dir;
    uv *= invAtan;
    uv += 0.5;

	const float strength = hpm ? hdrEnvMapData.hpmStrength : hdrEnvMapData.directStrength;
	return texture(hdrEnvMap, uv).xyz * strength;
}

vec3 SampleHdrEnvMap(const vec3 dir, const bool hpm)
{
	// Assert: dir is normalized
	vec2 phiTheta = vec2(atan(dir.z, dir.x), asin(dir.y));
	return SampleHdrEnvMap(phiTheta, hpm);
}

vec3 SampleHdrEnvMap(const vec3 pos, const vec3 dir, uint sampleCount)
{
	vec3 light = vec3(0.0);

	// Half ray importance sampled
	const uint halfSampleCount = sampleCount;//sampleCount / 2;
	for (uint i = 0; i < halfSampleCount; i++)
	{
		const vec3 randomDir = NewRayDir(dir);
		const float phase = 1.0;//hg_phase_func(dot(randomDir, -dir));
		const vec3 exit = find_entry_exit(pos, randomDir)[1];
		const float transmittance = GetTransmittance(pos, exit, 16);
		const vec3 sampleLight = SampleHdrEnvMap(randomDir, true) * phase * transmittance;

		light += sampleLight;
	}

	// Half env map importance sampled
	for (uint i = 0; i < sampleCount - halfSampleCount; i++)
	{
		const float thetaNorm = texture(hdrEnvMapInvCdfY, RandFloat(1.0)).x;
		const float phiNorm = texture(hdrEnvMapInvCdfX, vec2(RandFloat(1.0), thetaNorm)).x;

		//const float thetaNorm = 0.458;
		//const float phiNorm = 0.477;

		const vec3 randomDir = sin(thetaNorm * PI) * vec3(cos(phiNorm * 2.0 * PI), 1.0, sin(phiNorm * 2.0 * PI));

		const float phase = hg_phase_func(dot(randomDir, -dir));
		const vec3 exit = find_entry_exit(pos, randomDir)[1];
		const float transmittance = GetTransmittance(pos, exit, 16);
		const vec3 sampleLight = texture(hdrEnvMap, vec2(phiNorm, thetaNorm)).xyz * hdrEnvMapData.hpmStrength * phase * transmittance;

		light += sampleLight;
	}

	light /= float(sampleCount);

	return light;
}

vec3 TraceScene(const vec3 pos, const vec3 dir)
{
	const vec3 totalLight = TraceDirLight(pos, dir) + TracePointLight(pos, dir) + SampleHdrEnvMap(pos, dir, 4);
	return totalLight;
}

vec3 Forward(const vec3 pos, const vec3 dir)
{
	EncodeRay(pos, dir);
	Forward();
	return vec3(nr6[0], nr6[1], nr6[2]);
}

#define TRUE_TRACE_SAMPLE_COUNT 32
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
					if (volumeData.showNonNN == 0)
					{
						const float dirPhase = 1.0;//hg_phase_func(dot(currentDir, -lastDir));
						scatteredLight += transmittance * Forward(currentPoint, currentDir) * dirPhase;
					}
					return vec4(scatteredLight, transmittance);
				}
				totalTermProb *= 0.5;
			}

			// Get scene lighting
			const vec3 sceneLighting = TraceScene(currentPoint, currentDir);

			// Phase factor
			const float phase = 1.0; // Importance sampling

			// Transmittance calculation
			const vec3 s_int = density * sceneLighting * phase;
			const float t_r = GetTransmittance(currentPoint, lastPoint, 32);

			scatteredLight += transmittance * s_int;
			transmittance *= t_r;

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

vec4 TracePathMultiple(const vec3 rayOrigin, const vec3 rayDir, bool useNN)
{
	const uint spp = useNN ? volumeData.withNnSpp : volumeData.noNnSpp;
	vec4 average = vec4(0.0);
	for (uint i = 0; i < spp; i++)
	{
		average += TracePath(rayOrigin, rayDir, useNN);
	}
	average /= float(spp);
	return average;
}

// Main
void main()
{
	// Random
	preRand = volumeData.random.x * fragUV.x;
	prePreRand = volumeData.random.y * fragUV.y;

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
	const vec4 traceResult = TracePathMultiple(ro, rd, volumeData.useNN == 1);
	const float transmittance = traceResult.w;

	if (transmittance == 1.0)
	{
		outColor = vec4(envMapColor, 1.0);
		return;
	}

	//envMapColor = SampleHdrEnvMap(rd, false);
	//const float primaryRayTransmittance = GetTransmittance(entry, exit, 64);
	//outColor = vec4(traceResult.xyz + (envMapColor * primaryRayTransmittance), transmittance);
	outColor = traceResult;
}
