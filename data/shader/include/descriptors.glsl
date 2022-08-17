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
layout(std430, set = 3, binding = 0) buffer Weights0
{
	float matWeights0[64]; // 64 x 64
};

layout(std430, set = 3, binding = 1) buffer Weights1
{
	float matWeights1[4096]; // 64 x 64
};

layout(std430, set = 3, binding = 2) buffer Weights2
{
	float matWeights2[4096]; // 64 x 64
};

layout(std430, set = 3, binding = 3) buffer Weights3
{
	float matWeights3[4096]; // 64 x 64
};

layout(std430, set = 3, binding = 4) buffer Weights4
{
	float matWeights4[4096]; // 64 x 64
};

layout(std430, set = 3, binding = 5) buffer Weights5
{
	float matWeights5[192]; // 64 x 3
};

layout(std430, set = 3, binding = 6) buffer DeltaWeights0
{
	float matDeltaWeights0[4096]; // 64 x 64
};

layout(std430, set = 3, binding = 7) buffer DeltaWeights1
{
	float matDeltaWeights1[4096]; // 64 x 64
};

layout(std430, set = 3, binding = 8) buffer DeltaWeights2
{
	float matDeltaWeights2[4096]; // 64 x 64
};

layout(std430, set = 3, binding = 9) buffer DeltaWeights3
{
	float matDeltaWeights3[4096]; // 64 x 64
};

layout(std430, set = 3, binding = 10) buffer DeltaWeights4
{
	float matDeltaWeights4[4096]; // 64 x 64
};

layout(std430, set = 3, binding = 11) buffer DeltaWeights5
{
	float matDeltaWeights5[192]; // 64 x 3
};

layout(std430, set = 3, binding = 12) buffer Momentum1Weights0
{
	float matMomentum1Weights0[4096]; // 64 x 64
};

layout(std430, set = 3, binding = 13) buffer Momentum1Weights1
{
	float matMomentum1Weights1[4096]; // 64 x 64
};

layout(std430, set = 3, binding = 14) buffer Momentum1Weights2
{
	float matMomentum1Weights2[4096]; // 64 x 64
};

layout(std430, set = 3, binding = 15) buffer Momentum1Weights3
{
	float matMomentum1Weights3[4096]; // 64 x 64
};

layout(std430, set = 3, binding = 16) buffer Momentum1Weights4
{
	float matMomentum1Weights4[4096]; // 64 x 64
};

layout(std430, set = 3, binding = 17) buffer Momentum1Weights5
{
	float matMomentum1Weights5[192]; // 64 x 3
};

layout(std430, set = 3, binding = 18) buffer Biases0
{
	float matBiases0[64];
};

layout(std430, set = 3, binding = 19) buffer Biases1
{
	float matBiases1[64];
};

layout(std430, set = 3, binding = 20) buffer Biases2
{
	float matBiases2[64];
};

layout(std430, set = 3, binding = 21) buffer Biases3
{
	float matBiases3[64];
};

layout(std430, set = 3, binding = 22) buffer Biases4
{
	float matBiases4[64];
};

layout(std430, set = 3, binding = 23) buffer Biases5
{
	float matBiases5[3];
};

layout(std430, set = 3, binding = 24) buffer DeltaBiases0
{
	float matDeltaBiases0[64];
};

layout(std430, set = 3, binding = 25) buffer DeltaBiases1
{
	float matDeltaBiases1[64];
};

layout(std430, set = 3, binding = 26) buffer DeltaBiases2
{
	float matDeltaBiases2[64];
};

layout(std430, set = 3, binding = 27) buffer DeltaBiases3
{
	float matDeltaBiases3[64];
};

layout(std430, set = 3, binding = 28) buffer DeltaBiases4
{
	float matDeltaBiases4[64];
};

layout(std430, set = 3, binding = 29) buffer DeltaBiases5
{
	float matDeltaBiases5[3];
};

layout(std430, set = 3, binding = 30) buffer Momentum1Biases0
{
	float matMomentum1Biases0[64];
};

layout(std430, set = 3, binding = 31) buffer Momentum1Biases1
{
	float matMomentum1Biases1[64];
};

layout(std430, set = 3, binding = 32) buffer Momentum1Biases2
{
	float matMomentum1Biases2[64];
};

layout(std430, set = 3, binding = 33) buffer Momentum1Biases3
{
	float matMomentum1Biases3[64];
};

layout(std430, set = 3, binding = 34) buffer Momentum1Biases4
{
	float matMomentum1Biases4[64];
};

layout(std430, set = 3, binding = 35) buffer Momentum1Biases5
{
	float matMomentum1Biases5[3];
};

layout(set = 3, binding = 36) uniform NrcConfig
{
	float learningRate;
	float weightDecay;
	float beta1;
} nrcConfig;

layout(std430, set = 3, binding = 37) buffer NrcStats
{
	float mseLoss;
} nrcStats;

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

layout(set = 6, binding = 0) uniform MrheData
{
	float learningRate;
	float weightDecay;
	uint levelCount;
	uint hashTableSize;
	uint featureCount;
	uint minRes;
	uint maxRes;
	uint resolutions[16];
} mrhe;

layout(std430, set = 6, binding = 1) buffer MRHashTable
{
	float mrHashTable[];
};

layout(std430, set = 6, binding = 2) buffer MRDeltaHashTable
{
	float mrDeltaHashTable[];
};
