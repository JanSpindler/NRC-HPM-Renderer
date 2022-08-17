#version 460
#extension GL_ARB_separate_shader_objects : enable

#include "common.glsl"

// Inputs
layout(location = 0) in vec3 pixelWorldPos;
layout(location = 1) in vec2 fragUV;

// Output
layout(location = 0) out vec4 outColor;

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
