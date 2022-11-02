layout(set = 0, binding = 0) uniform camMat_t
{
	mat4 projView;
	mat4 invProjView;
	mat4 prevProjView;
} camMat;

layout(set = 0, binding = 1) uniform camera_t
{
	vec3 pos;
} camera;

layout(set = 1, binding = 0) uniform sampler3D densityTex;

layout(set = 2, binding = 0) uniform dir_light_t
{
	vec3 color;
	float zenith;
	vec3 dir;
	float azimuth;
	float strength;
} dir_light;

layout(set = 3, binding = 0) uniform PointLight
{
	vec3 pos;
	float strength;
	vec3 color;
} pointLight;

layout(set = 4, binding = 0) uniform sampler2D hdrEnvMap;

layout(set = 4, binding = 1) uniform sampler2D hdrEnvMapInvCdfX;

layout(set = 4, binding = 2) uniform sampler1D hdrEnvMapInvCdfY;

layout(set = 5, binding = 0, rgba32f) uniform image2D outputImage;

layout(set = 5, binding = 1, rgba32f) uniform image2D infoImage;

layout(set = 5, binding = 2) uniform Renderer
{
	vec4 random;
	float blendFactor;
};
