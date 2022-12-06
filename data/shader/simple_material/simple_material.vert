#version 450
#extension GL_ARB_separate_shader_objects : enable

layout (location = 0) in vec3 pos;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 uv;

layout (set = 0, binding = 0) uniform model_uniforms_t
{
	mat4 model_mat;
} model_ubo;

layout (set = 1, binding = 0) uniform camera_uniforms_t
{
	mat4 proj_view_mat;
} camera_matrices;

layout (location = 0) out vec2 frag_uv;

void main()
{
	gl_Position = camera_matrices.proj_view_mat * model_ubo.model_mat * vec4(pos, 1.0);
	frag_uv = uv;
}
