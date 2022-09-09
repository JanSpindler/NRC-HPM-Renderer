layout(set = 0, binding = 0) uniform camMat_t
{
	mat4 projView;
	mat4 invProjView;
} camMat;

layout(set = 0, binding = 1) uniform camera_t
{
	vec3 pos;
} camera;

layout(set = 1, binding = 0) uniform sampler3D densityTex;

layout(set = 1, binding = 1) uniform volumeData_t
{
	vec4 random;
	uint useNN;
	uint showNonNN;
	float densityFactor;
	float g;
	int noNnSpp;
	int withNnSpp;
} volumeData;

layout(set = 2, binding = 0) uniform dir_light_t
{
	vec3 color;
	float zenith;
	vec3 dir;
	float azimuth;
	float strength;
} dir_light;

struct Vec3f
{
	float x;
	float y;
	float z;
};

layout(std430, set = 3, binding = 0) buffer PathReservoir
{
	Vec3f pathReservoir[];
};

layout(std430, set = 3, binding = 1) buffer OldViewProjMat
{
	mat4 oldViewProjMat[];
};

layout(std430, set = 3, binding = 2) buffer OldPathReservoirs
{
	Vec3f oldPathReservoirs[];
};

void StorePathVertex(const ivec2 imageCoord, const uint vertexIndex, const vec3 vertex)
{
	const uint linearVertexIndex = ((imageCoord.y * RENDER_WIDTH) + imageCoord.x) * (PATH_VERTEX_COUNT) + vertexIndex;
	
	Vec3f vertexFormatted;
	vertexFormatted.x = vertex.x;
	vertexFormatted.y = vertex.y;
	vertexFormatted.z = vertex.z;

	pathReservoir[linearVertexIndex] = vertexFormatted;
}

vec3 LoadPathVertex(const ivec2 imageCoord, const uint vertexIndex)
{
	const uint linearVertexIndex = ((imageCoord.y * RENDER_WIDTH) + imageCoord.x) * (PATH_VERTEX_COUNT) + vertexIndex;
	
	vec3 vertex;
	Vec3f vertexFormatted = pathReservoir[linearVertexIndex];
	vertex.x = vertexFormatted.x;
	vertex.y = vertexFormatted.y;
	vertex.z = vertexFormatted.z;

	return vertex;
}

void StoreOldPathVertex(const uint reservoirIndex, const ivec2 imageCoord, const uint vertexIndex, const vec3 vertex)
{
	const uint linearVertexIndex = 
		(reservoirIndex * RESERVOIR_SIZE) +
		((imageCoord.y * RENDER_WIDTH) + imageCoord.x) * (PATH_VERTEX_COUNT) + 
		vertexIndex;

	Vec3f vertexFormatted;
	vertexFormatted.x = vertex.x;
	vertexFormatted.y = vertex.y;
	vertexFormatted.z = vertex.z;

	oldPathReservoirs[linearVertexIndex] = vertexFormatted;
}

vec3 LoadOldPathVertex(const uint reservoirIndex, const ivec2 imageCoord, const uint vertexIndex)
{
	const uint linearVertexIndex = 
		(reservoirIndex * RESERVOIR_SIZE) +
		((imageCoord.y * RENDER_WIDTH) + imageCoord.x) * (PATH_VERTEX_COUNT) + 
		vertexIndex;
	
	vec3 vertex;
	Vec3f vertexFormatted = oldPathReservoirs[linearVertexIndex];
	vertex.x = vertexFormatted.x;
	vertex.y = vertexFormatted.y;
	vertex.z = vertexFormatted.z;

	return vertex;
}

layout(set = 4, binding = 0) uniform PointLight
{
	vec3 pos;
	float strength;
	vec3 color;
} pointLight;

layout(set = 5, binding = 0) uniform sampler2D hdrEnvMap;

layout(set = 5, binding = 1) uniform sampler2D hdrEnvMapInvCdfX;

layout(set = 5, binding = 2) uniform sampler1D hdrEnvMapInvCdfY;

layout(set = 5, binding = 3) uniform HdrEnvMapData
{
	float directStrength;
	float hpmStrength;
} hdrEnvMapData;

layout(set = 6, binding = 0, rgba32f) uniform image2D outputImage;

layout(set = 6, binding = 1, rgba32f) uniform image2D pixelInfoImage;

layout(set = 6, binding = 2, rgba32f) uniform image2D restirStatsImage;
