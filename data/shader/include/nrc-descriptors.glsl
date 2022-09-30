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
layout(std430, set = 3, binding = 0) buffer RenderNeurons
{
	float16_t renderNeurons[];
};

layout(std430, set = 3, binding = 1) buffer TrainNeurons
{
	float16_t trainNeurons[];
};

layout(std430, set = 3, binding = 2) buffer Weights
{
	float16_t weights[];
};

layout(std430, set = 3, binding = 3) buffer DeltaWeights
{
	float16_t deltaWeights[];
};

layout(std430, set = 3, binding = 4) buffer MomentumWeights
{
	float16_t momentumWeights[];
};

layout(std430, set = 3, binding = 5) buffer Biases
{
	float16_t biases[];
};

layout(std430, set = 3, binding = 6) buffer DeltaBiases
{
	float16_t deltaBiases[];
};

layout(std430, set = 3, binding = 7) buffer MomentumBiases
{
	float16_t momentumBiases[];
};

layout(std430, set = 3, binding = 8) buffer Mrhe
{
	float16_t mrhe[];
};

layout(std430, set = 3, binding = 9) buffer DeltaMrhe
{
	float16_t deltaMrhe[];
};

layout(std430, set = 3, binding = 10) buffer MrheRes
{
	uint mrheResolutions[];
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

layout(set = 6, binding = 1, rgba32f) uniform image2D nrcPrimaryRayColorImage;

layout(set = 6, binding = 2, rgba32f) uniform image2D nrcPrimaryRayInfoImage;

layout(set = 6, binding = 3, rgba32f) uniform image2D nrcNeuralRayOriginImage;

layout(set = 6, binding = 4, rgba32f) uniform image2D nrcNeuralRayDirImage;

layout(set = 6, binding = 5, rgba32f) uniform image2D nrcNeuralRayColorImage;

layout(set = 6, binding = 6, rgba32f) uniform image2D nrcNeuralRayTargetImage;
