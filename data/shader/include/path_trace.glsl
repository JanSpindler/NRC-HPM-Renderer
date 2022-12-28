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

float RatioTrack(const vec3 start, const vec3 end)
{
	const float invMaxDensity = 1.0 / VOLUME_DENSITY_FACTOR;
	//const float invMaxDensity = 1.0;

	const vec3 dir = normalize(end - start);
	const float tMax = distance(end, start);
	float transmittance = 1.0;
	float t = 0.0;
	
	for (uint i = 0; i < 128; i++)
	{
		t -= log(1.0 - RandFloat(1.0)) * invMaxDensity;
		if (t >= tMax) { break; }
		const vec3 nextSamplePoint = start + (t * dir);
		transmittance *= 1.0 - (getDensity(nextSamplePoint) * invMaxDensity);
	}
	
	return transmittance;
}

vec3 TraceDirLight(const vec3 pos, const vec3 dir)
{
	if (dir_light.strength == 0.0)
	{
		return vec3(0.0);
	}

	const float transmittance = RatioTrack(pos, find_entry_exit(pos, -normalize(dir_light.dir))[1]);
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

	const float transmittance = RatioTrack(pointLight.pos, pos);
	const float phase = hg_phase_func(dot(normalize(pointLight.pos - pos), -dir));
	const vec3 pointLighting = pointLight.color * pointLight.strength * transmittance * phase;
	return pointLighting;
}

vec3 SampleHdrEnvMap(const vec2 dir)
{
	const vec2 invAtan = vec2(0.1591, 0.3183);

	vec2 uv = dir;
    uv *= invAtan;
    uv += 0.5;

	return texture(hdrEnvMap, uv).xyz * HDR_ENV_MAP_STRENGTH;
}

vec3 SampleHdrEnvMap(const vec3 dir)
{
	vec2 phiTheta = vec2(atan(dir.z, dir.x), asin(dir.y));
	return SampleHdrEnvMap(phiTheta);
}

vec3 SampleHdrEnvMap(const vec3 pos, const vec3 dir, uint sampleCount)
{
	if (HDR_ENV_MAP_STRENGTH == 0.0)
	{
		return vec3(0.0);
	}

	vec3 light = vec3(0.0);

	for (uint i = 0; i < sampleCount; i++)
	{
		const vec3 randomDir = NewRayDir(dir, false);
		const float phase = hg_phase_func(dot(randomDir, -dir));
		const vec3 exit = find_entry_exit(pos, randomDir)[1];
		//const float transmittance = GetTransmittance(pos, exit, 16);
		const float transmittance = RatioTrack(pos, exit);
		const vec3 sampleLight = SampleHdrEnvMap(randomDir) * phase * transmittance;

		light += sampleLight;
	}

	// Half env map importance sampled
//	for (uint i = 0; i < sampleCount - halfSampleCount; i++)
//	{
//		const float thetaNorm = texture(hdrEnvMapInvCdfY, RandFloat(1.0)).x;
//		const float phiNorm = texture(hdrEnvMapInvCdfX, vec2(RandFloat(1.0), thetaNorm)).x;
//
//		//const float thetaNorm = 0.458;
//		//const float phiNorm = 0.477;
//
//		const vec3 randomDir = sin(thetaNorm * PI) * vec3(cos(phiNorm * 2.0 * PI), 1.0, sin(phiNorm * 2.0 * PI));
//
//		const float phase = hg_phase_func(dot(randomDir, -dir));
//		const vec3 exit = find_entry_exit(pos, randomDir)[1];
//		const float transmittance = GetTransmittance(pos, exit, 16);
//		const vec3 sampleLight = texture(hdrEnvMap, vec2(phiNorm, thetaNorm)).xyz * hdrEnvMapData.hpmStrength * phase * transmittance;
//
//		light += sampleLight;
//	}

	light /= float(sampleCount);

	return light;
}

vec3 TraceScene(const vec3 pos, const vec3 dir)
{
	const vec3 totalLight = TraceDirLight(pos, dir) + TracePointLight(pos, dir) + SampleHdrEnvMap(pos, dir, 1);
	return totalLight;
}

vec3 TraceScene(const vec3 pos, const vec3 dir, const vec3 hdrEnvMapUniformDir)
{
	const vec3 exit = find_entry_exit(pos, hdrEnvMapUniformDir)[1];
	const float hdrEnvMapTransmittance = GetTransmittance(pos, exit, 16);
	const float hdrEnvMapPhase = hg_phase_func(dot(-dir, hdrEnvMapUniformDir));
	const vec3 hdrEnvMapLight = SampleHdrEnvMap(hdrEnvMapUniformDir) * hdrEnvMapTransmittance * hdrEnvMapPhase;

	const vec3 totalLight = TraceDirLight(pos, dir) + TracePointLight(pos, dir) + hdrEnvMapLight;
	return totalLight;
}

vec3 DeltaTrack(const vec3 rayOrigin, const vec3 rayDir, out bool volumeExit)
{
	volumeExit = false;

	const float invMaxDensity = 1.0 / VOLUME_DENSITY_FACTOR;
	//const float invMaxDensity = 1.0;

	const vec3 exit = find_entry_exit(rayOrigin, rayDir)[1];
	const float tMax = distance(exit, rayOrigin);
	float t = 0.0;

	for (uint i = 0; i < 128; i++)
	{
		t -= log(1.0 - RandFloat(1.0)) * invMaxDensity;
		if (t >= tMax)
		{
			volumeExit = true;
			break; 
		}
		const vec3 nextSamplePoint = rayOrigin + (t * rayDir);
		if (getDensity(nextSamplePoint) * invMaxDensity > RandFloat(1.0)) { return nextSamplePoint; }
	}

	return rayOrigin + (RandFloat(tMax) * rayDir);
}
