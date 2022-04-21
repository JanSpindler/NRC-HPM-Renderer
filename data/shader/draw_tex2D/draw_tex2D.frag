#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec2 frag_uv;

layout(set = 0, binding = 0) uniform sampler2D tex;

layout(location = 0) out vec4 out_color;

void main()
{
	vec4 val = texture(tex, frag_uv);
	out_color = val;
}
