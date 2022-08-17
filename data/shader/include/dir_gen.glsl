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
