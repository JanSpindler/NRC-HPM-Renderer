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

// NN buffers
layout(std430, set = 3, binding = 0) buffer Neurons
{
	float neurons[];
};

layout(std430, set = 3, binding = 1) buffer Weights
{
	float weights[];
};

layout(std430, set = 3, binding = 2) buffer DeltaWeights
{
	float deltaWeights[];
};

layout(std430, set = 3, binding = 3) buffer MomentumWeights
{
	float momentumWeights[];
};

layout(std430, set = 3, binding = 4) buffer Biases
{
	float biases[];
};

layout(std430, set = 3, binding = 5) buffer DeltaBiases
{
	float deltaBiases[];
};

layout(std430, set = 3, binding = 6) buffer MomentumBiases
{
	float momentumBiases[];
};

layout(std430, set = 3, binding = 7) buffer Mrhe
{
	float mrhe[];
};

layout(std430, set = 3, binding = 8) buffer DeltaMrhe
{
	float deltaMrhe[];
};

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

layout(set = 6, binding = 0, rgba32f) uniform image2D nrcOutputImage;
