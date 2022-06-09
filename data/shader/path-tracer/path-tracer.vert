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

layout(location = 0) out vec2 fragUV;

void main()
{
	int index = indices[gl_VertexIndex];
	gl_Position = vec4(pos[index], 0.0, 1.0);
	gl_Position.y *= -1.0;
	fragUV = uv[index];
}
