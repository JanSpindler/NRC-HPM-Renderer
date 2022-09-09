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

struct PathVertex
{
	float posX;
	float posY;
	float posZ;
	float dirX;
	float dirY;
	float dirZ;
};

layout(std430, set = 3, binding = 0) buffer PathReservoir
{
	PathVertex pathReservoir[];
};

layout(std430, set = 3, binding = 1) buffer OldViewProjMat
{
	mat4 oldViewProjMat[];
};

layout(std430, set = 3, binding = 2) buffer OldPathReservoirs
{
	PathVertex oldPathReservoirs[];
};

void StorePathVertex(const ivec2 imageCoord, const uint vertexIndex, const vec3 pos, const vec3 dir)
{
	const uint linearVertexIndex = ((imageCoord.y * RENDER_WIDTH) + imageCoord.x) * (PATH_VERTEX_COUNT) + vertexIndex;

	pathReservoir[linearVertexIndex].posX = pos.x;
	pathReservoir[linearVertexIndex].posY = pos.y;
	pathReservoir[linearVertexIndex].posZ = pos.z;

	pathReservoir[linearVertexIndex].dirX = dir.x;
	pathReservoir[linearVertexIndex].dirY = dir.y;
	pathReservoir[linearVertexIndex].dirZ = dir.z;
}

void LoadPathVertex(const ivec2 imageCoord, const uint vertexIndex, out vec3 pos, out vec3 dir)
{
	const uint linearVertexIndex = ((imageCoord.y * RENDER_WIDTH) + imageCoord.x) * (PATH_VERTEX_COUNT) + vertexIndex;
	
	pos.x = pathReservoir[linearVertexIndex].posX;
	pos.y = pathReservoir[linearVertexIndex].posY;
	pos.z = pathReservoir[linearVertexIndex].posZ;

	dir.x = pathReservoir[linearVertexIndex].dirX;
	dir.y = pathReservoir[linearVertexIndex].dirY;
	dir.z = pathReservoir[linearVertexIndex].dirZ;
}

void StoreOldPathVertex(const uint reservoirIndex, const ivec2 imageCoord, const uint vertexIndex, const vec3 pos, const vec3 dir)
{
	const uint linearVertexIndex = 
		(reservoirIndex * RESERVOIR_SIZE) +
		((imageCoord.y * RENDER_WIDTH) + imageCoord.x) * (PATH_VERTEX_COUNT) + 
		vertexIndex;

	pathReservoir[linearVertexIndex].posX = pos.x;
	pathReservoir[linearVertexIndex].posY = pos.y;
	pathReservoir[linearVertexIndex].posZ = pos.z;
	
	pathReservoir[linearVertexIndex].dirX = dir.x;
	pathReservoir[linearVertexIndex].dirY = dir.y;
	pathReservoir[linearVertexIndex].dirZ = dir.z;
}

void LoadOldPathVertex(const uint reservoirIndex, const ivec2 imageCoord, const uint vertexIndex, out vec3 pos, out vec3 dir)
{
	const uint linearVertexIndex = 
		(reservoirIndex * RESERVOIR_SIZE) +
		((imageCoord.y * RENDER_WIDTH) + imageCoord.x) * (PATH_VERTEX_COUNT) + 
		vertexIndex;
	
	pos.x = pathReservoir[linearVertexIndex].posX;
	pos.y = pathReservoir[linearVertexIndex].posY;
	pos.z = pathReservoir[linearVertexIndex].posZ;
	
	dir.x = pathReservoir[linearVertexIndex].dirX;
	dir.y = pathReservoir[linearVertexIndex].dirY;
	dir.z = pathReservoir[linearVertexIndex].dirZ;
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

layout(set = 6, binding = 3) uniform RendererUniformData
{
	uint frameCounter;
};
