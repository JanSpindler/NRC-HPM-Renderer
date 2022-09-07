#version 460
#extension GL_ARB_separate_shader_objects : enable

vec2 pos[4] = vec2[](
	vec2(-1.0, -1.0),
	vec2(-1.0, 1.0),
	vec2(1.0, 1.0),
	vec2(1.0, -1.0));

vec2 uv[4] = vec2[] (
	vec2(0.0, 0.0),
	vec2(0.0, 1.0),
	vec2(1.0, 1.0),
	vec2(1.0, 0.0)
);

int indices[6] = int[] ( 0, 1, 2, 2, 3, 0 );

layout(set = 0, binding = 0) uniform camMat_t
{
	mat4 projView;
	mat4 oldProjView;
} camMat;

layout(location = 0) out vec3 pixelWorldPos;
layout(location = 1) out vec2 fragUV;

void main()
{
	int index = indices[gl_VertexIndex];
	gl_Position = vec4(pos[index], 0.0, 1.0);

	vec4 screenCoord = gl_Position;
	screenCoord.y *= -1.0;

	vec4 worldPos = inverse(camMat.projView) * screenCoord;
	pixelWorldPos = worldPos.xyz / worldPos.w;

	fragUV = uv[index];
}
