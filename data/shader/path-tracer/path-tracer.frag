#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 pixelWorldPos;

layout(set = 0, binding = 1) uniform camera_t
{
	vec3 pos;
} camera;

layout(set = 1, binding = 0) uniform sampler3D densityTex;

layout(set = 2, binding = 0) uniform dir_light_t // TODO: raname to sun_t
{
	vec3 color;
	float zenith;
	vec3 dir;
	float azimuth;
} dir_light;

layout(location = 0) out vec4 outColor;

const vec3 skySize = vec3(125.0, 85.0, 153.0) / 2.0;
const vec3 skyPos = vec3(0.0);

#define PI 3.14159265359

#define MAX_RAY_DISTANCE 100000.0
#define MIN_RAY_DISTANCE 0.125
#define DEPTH_MAX_TRANSMITTANCE 0.3

#define MIN_HEIGHT (skyPos.y - (skySize.y / 2))
#define MAX_HEIGHT (MIN_HEIGHT + skySize.y)

#define SAMPLE_COUNT 40
#define SECONDARY_SAMPLE_COUNT 8

#define SIGMA_E 0.5
#define SIGMA_S 0.5
#define G 0.95

float rand(vec2 co)
{
	return fract(sin(dot(co, vec2(12.9898, 78.233))) * 43758.5453);
}

float sky_sdf(vec3 pos)
{
	vec3 d = abs(pos - skyPos) - skySize / 2;
	return length(max(d, 0)) + min(max(d.x, max(d.y, d.z)), 0);
}

vec3[2] find_entry_exit(vec3 ro, vec3 rd)
{
	// rd should be normalized

	float dist;
	do
	{
		dist = sky_sdf(ro);
		ro  += dist * rd;
	} while (dist > MIN_RAY_DISTANCE && dist < MAX_RAY_DISTANCE);
	vec3 entry = ro;

	ro += rd * length(2 * skySize);//ro += rd * MAX_RAY_DISTANCE * 2;
	rd *= -1.0;
	do
	{
		dist = sky_sdf(ro);
		ro += dist * rd;
	} while (dist > MIN_RAY_DISTANCE && dist < MAX_RAY_DISTANCE);
	vec3 exit = ro;
	
	return vec3[2]( entry, exit );
}

void gen_sample_points(vec3 start_pos, vec3 end_pos, out vec3 samples[SAMPLE_COUNT])
{
	vec3 dir = end_pos - start_pos;
	for (int i = 0; i < SAMPLE_COUNT; i++)
		samples[i] = start_pos + dir * (float(i) / float(SAMPLE_COUNT));
}

vec3 get_sky_uvw(vec3 pos)
{
	return ((pos - skyPos) / skySize) + vec3(0.5);
}

float getDensity(vec3 pos)
{
	return texture(densityTex, get_sky_uvw(pos)).x;
}

float hg_phase_func(float cos_theta, float g)
{
	float g2 = g * g;
	float result = 0.5 * (1 - g2) / pow(1 + g2 - (2 * g * cos_theta), 1.5);
	return result;
}

float get_self_shadowing(vec3 pos)
{
	// Exit if not used
	if (SECONDARY_SAMPLE_COUNT == 0)
		return 1.0;

	// Find exit from current pos
	const vec3 exit = find_entry_exit(pos, -normalize(dir_light.dir))[1];

	// Generate secondary sample points using lerp factors
	const vec3 direction = exit - pos;
	vec3 secondary_sample_points[SECONDARY_SAMPLE_COUNT];
	for (int i = 0; i < SECONDARY_SAMPLE_COUNT; i++)
		secondary_sample_points[i] = pos + direction * exp(float(i - SECONDARY_SAMPLE_COUNT));

	// Calculate light reaching pos
	float transmittance = 1.0;
	const float sigma_e = SIGMA_E;
	for (int i = 0; i < SECONDARY_SAMPLE_COUNT; i++)
	{
		const vec3 sample_point = secondary_sample_points[i];
		const float density = getDensity(sample_point);
		if (density > 0.0)
		{
			const float sample_sigma_e = sigma_e * density;
			const float step_size = (i < SECONDARY_SAMPLE_COUNT - 1) ? 
				length(secondary_sample_points[i + 1] - secondary_sample_points[i]) : 
				1.0;
			const float t_r = exp(-sample_sigma_e * step_size);
			transmittance *= t_r;
		}
	}

	return transmittance;
}

// x-z = scattered light | z = transmittance
vec4 render_cloud(vec3 sample_points[SAMPLE_COUNT], vec3 out_dir, vec3 ro)
{	
	// out_dir should be normalized
	// SAMPLE_COUNT should be > 0

	vec3 scattered_light = vec3(0.0);
	float transmittance = 1.0;

	const float sigma_s = SIGMA_S;
	const float sigma_e = SIGMA_E;
	const float step_size = length(sample_points[1] - sample_points[0]);
	const vec3 step_offset = 
		rand(vec2(out_dir.x * out_dir.y, out_dir.z)) * 
		out_dir * 
		step_size;

	bool depth_stored = false;
	for (int i = 0; i < SAMPLE_COUNT; i++)
	{
		const vec3 sample_point = sample_points[i] + step_offset;
		const float density = getDensity(sample_point);

		if (density > 0.0)
		{
			const float sample_sigma_s = sigma_s * density;
			const float sample_sigma_e = sigma_e * density;

			const vec3 ambient_color = normalize(vec3(1.0, 1.0, 1.0));

			const float phase_result = hg_phase_func(dot(dir_light.dir, out_dir), G);
			const float self_shadowing = get_self_shadowing(sample_point);

			const vec3 s = (vec3(self_shadowing * phase_result) + ambient_color) * sample_sigma_s;
			const float t_r = exp(-sample_sigma_e * step_size);
			const vec3 s_int = (s - (s * t_r)) / sample_sigma_e;

			scattered_light += transmittance * s_int;
			transmittance *= t_r;
			
			// Early exit
			if (transmittance < 0.01)
				break;
		}
	}

	return vec4(scattered_light, transmittance);
}

void main()
{
	vec3 ro = camera.pos;
	const vec3 rd = normalize(pixelWorldPos - ro);

	const vec3[2] entry_exit = find_entry_exit(ro, rd);
	const vec3 entry = entry_exit[0];
	const vec3 exit = entry_exit[1];

	if (sky_sdf(entry) > MAX_RAY_DISTANCE)
	{
		outColor = vec4(0.0);
		return;
	}

	vec3[SAMPLE_COUNT] samplePoints;
	gen_sample_points(entry, exit, samplePoints);

	vec4 result = render_cloud(samplePoints, -rd, ro);
	outColor = vec4(result.xyz, 1.0 - result.w);
}

/*#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_EXT_debug_printf : enable

layout(location = 0) in vec3 pixel_world_pos;

layout(constant_id = 0) const int SAMPLE_COUNT = 16;
layout(constant_id = 1) const int SECONDARY_SAMPLE_COUNT = 0;

layout(set = 0, binding = 0) uniform camera_matrices_t
{
	mat4 proj_view;
	mat4 old_proj_view;
} camera_matrices;

layout(set = 0, binding = 1) uniform camera_t
{
	vec3 pos;
} camera;

layout(set = 1, binding = 0) uniform dir_light_t // TODO: raname to sun_t
{
	vec3 color;
	float zenith;
	vec3 dir;
	float azimuth;
} dir_light;

layout(set = 2, binding = 0) uniform cloud_data_t
{
	vec3 sky_size;
	vec3 sky_pos;
	float shape_scale;
	float detail_threshold;
	float detail_factor;
	float detail_scale;
	float height_gradient_min_val;
	float height_gradient_max_val;
	float g;
	float ambient_gradient_min_val;
	float ambient_gradient_max_val;
	float jitter_strength;
	float sigma_s;
	float sigma_e;
	float temporal_upsampling;
	uint ltee;
	uint htee;
} cloud_data;

layout(set = 2, binding = 1) uniform sampler3D cloud_shape_tex;
layout(set = 2, binding = 2) uniform sampler3D cloud_detail_tex;
layout(set = 2, binding = 3) uniform sampler2D weather_tex;

layout(set = 3, binding = 0) uniform sampler2D old_tex;

layout(set = 3, binding = 1) uniform random_t
{
	float value;
} random;

layout(location = 0) out vec4 out_color;

#define PI 3.14159265359

#define MAX_SECONDARY_SAMPLE_COUNT 8
#define MAX_RAY_DISTANCE 100000.0
#define MIN_RAY_DISTANCE 0.125
#define DEPTH_MAX_TRANSMITTANCE 0.3

#define MIN_HEIGHT (cloud_data.sky_pos.y - (cloud_data.sky_size.y / 2))
#define MAX_HEIGHT (MIN_HEIGHT + cloud_data.sky_size.y)

struct cloud_result_t
{
	vec3 light;
	float transmittance;
	float depth;
};

float rand(vec2 co)
{
	return fract(sin(dot(co, vec2(12.9898, 78.233))) * 43758.5453);
}

float sky_sdf(vec3 pos)
{
	vec3 sky_size = cloud_data.sky_size;
	vec3 d = abs(pos - cloud_data.sky_pos) - sky_size / 2;
	return length(max(d, 0)) + min(max(d.x, max(d.y, d.z)), 0);
}

vec3[2] find_entry_exit(vec3 ro, vec3 rd)
{
	// rd should be normalized

	float dist;
	do
	{
		dist = sky_sdf(ro);
		ro  += dist * rd;
	} while (dist > MIN_RAY_DISTANCE && dist < MAX_RAY_DISTANCE);
	vec3 entry = ro;

	ro += rd * length(2 * cloud_data.sky_size);//ro += rd * MAX_RAY_DISTANCE * 2;
	rd *= -1.0;
	do
	{
		dist = sky_sdf(ro);
		ro += dist * rd;
	} while (dist > MIN_RAY_DISTANCE && dist < MAX_RAY_DISTANCE);
	vec3 exit = ro;
	
	return vec3[2]( entry, exit );
}

void gen_sample_points(vec3 start_pos, vec3 end_pos, out vec3 samples[SAMPLE_COUNT])
{
	vec3 dir = end_pos - start_pos;
	for (int i = 0; i < SAMPLE_COUNT; i++)
			samples[i] = start_pos + dir * (float(i) / float(SAMPLE_COUNT));
}

vec3 get_sky_uvw(vec3 pos)
{
	return ((pos - cloud_data.sky_pos) / cloud_data.sky_size) + vec3(0.5);
}

float get_cloud_shape(vec3 pos)
{
	vec3 tex_sample_pos = get_sky_uvw(pos);
	tex_sample_pos *= cloud_data.sky_size; // real size
	tex_sample_pos /= vec3(128.0, 32.0, 128.0); // shape texture size
	tex_sample_pos *= cloud_data.shape_scale;


	vec4 shape = texture(cloud_shape_tex, tex_sample_pos);
	float result = shape.x * (shape.y + shape.z + shape.w);
	return result;
}

float get_cloud_detail(vec3 pos)
{
	vec3 tex_sample_pos = get_sky_uvw(pos);
	tex_sample_pos *= cloud_data.sky_size;
	tex_sample_pos /= cloud_data.sky_size.y;
	tex_sample_pos *= cloud_data.detail_scale;

	vec3 detail = texture(cloud_shape_tex, tex_sample_pos).xyz;
	float result = detail.x + detail.y + detail.z;
	result /= 3.0;
	return result;
}

vec3 get_weather(vec3 pos)
{
	return texture(weather_tex, get_sky_uvw(pos).xz).xyz;
}

float get_height_signal(float height, float altitude, vec3 pos)
{
	float real_altitude = pos.y;
	
	float r1 = real_altitude - altitude;
	float r2 = real_altitude - altitude - height;
	float s = -4.0 / (height * height);

	float result = r1 * r2 * s;

	return result;
}

float get_height_gradient(float height, float altitude, vec3 pos)
{
	float real_altitude = pos.y;
	
	float x = (real_altitude - altitude) / height;
	x = clamp(x, 0.0, 1.0);
	float result = cloud_data.height_gradient_min_val + (cloud_data.height_gradient_max_val - cloud_data.height_gradient_min_val) * x;

	return result;
}

// Henyey-Greenstein
float hg_phase_func(float cos_theta, float g)
{
	float g2 = g * g;
	float result = 0.5 * (1 - g2) / pow(1 + g2 - (2 * g * cos_theta), 1.5);
	return result;
}

float get_ambient_gradient(const float height)
{
	const float x = (height - MIN_HEIGHT) / (MAX_HEIGHT - MIN_HEIGHT);
	return cloud_data.ambient_gradient_min_val + x * (cloud_data.ambient_gradient_max_val - cloud_data.ambient_gradient_min_val);
}

float get_density(vec3 pos)
{
	vec3 weather = get_weather(pos);
	float density = weather.r;
	float height = weather.g * cloud_data.sky_size.y;
	float altitude = MIN_HEIGHT + weather.b * cloud_data.sky_size.y;

	density *= get_height_signal(height, altitude, pos);
	density *= get_cloud_shape(pos);
	if (density < cloud_data.detail_threshold)
		density -= get_cloud_detail(pos) * cloud_data.detail_factor;
	density *= get_height_gradient(height, altitude, pos);

	return clamp(density, 0.0, 1.0);
}

float get_self_shadowing(vec3 pos)
{
	// Exit if not used
	if (SECONDARY_SAMPLE_COUNT == 0)
		return 1.0;

	// Find exit from current pos
	const vec3 exit = find_entry_exit(pos, -normalize(dir_light.dir))[1];

	// Generate secondary sample points using lerp factors
	const vec3 direction = exit - pos;
	vec3 secondary_sample_points[MAX_SECONDARY_SAMPLE_COUNT];
	for (int i = 0; i < SECONDARY_SAMPLE_COUNT; i++)
		secondary_sample_points[i] = pos + direction * exp(float(i - SECONDARY_SAMPLE_COUNT));

	// Calculate light reaching pos
	float transmittance = 1.0;
	const float sigma_e = cloud_data.sigma_e;
	for (int i = 0; i < SECONDARY_SAMPLE_COUNT; i++)
	{
		const vec3 sample_point = secondary_sample_points[i];
		const float density = get_density(sample_point);
		if (density > 0.0)
		{
			const float sample_sigma_e = sigma_e * density;
			const float step_size = (i < SECONDARY_SAMPLE_COUNT - 1) ? 
				length(secondary_sample_points[i + 1] - secondary_sample_points[i]) : 
				1.0;
			const float t_r = exp(-sample_sigma_e * step_size);
			transmittance *= t_r;
		}
	}

	return transmittance;
}

cloud_result_t render_cloud(vec3 sample_points[SAMPLE_COUNT], vec3 out_dir, vec3 ro)
{	
	// out_dir should be normalized
	// SAMPLE_COUNT should be > 0

	cloud_result_t result;
	result.light = vec3(0.0);
	result.transmittance = 1.0;
	result.depth = 1.0;

	const float sigma_s = cloud_data.sigma_s;
	const float sigma_e = cloud_data.sigma_e;
	const float step_size = length(sample_points[1] - sample_points[0]);
	const vec3 step_offset = 
		rand(vec2(out_dir.x * out_dir.y, out_dir.z /** random.value*//*)) * 
		out_dir * 
		step_size * 
		cloud_data.jitter_strength;

	bool depth_stored = false;
	for (int i = 0; i < SAMPLE_COUNT; i++)
	{
		const vec3 sample_point = sample_points[i] + step_offset;
		const float density = get_density(sample_point);
		if (density > 0.0)
		{
			const float sample_sigma_s = sigma_s * density;
			const float sample_sigma_e = sigma_e * density;

			const vec3 ambient_color = normalize(vec3(1.0, 1.0, 1.0));
			const vec3 ambient = get_ambient_gradient(sample_point.y) * ambient_color;

			const float phase_result = hg_phase_func(dot(dir_light.dir, out_dir), cloud_data.g);
			const float self_shadowing = get_self_shadowing(sample_point);

			const vec3 s = (vec3(self_shadowing * phase_result) + ambient) * sample_sigma_s;
			const float t_r = exp(-sample_sigma_e * step_size);
			const vec3 s_int = (s - (s * t_r)) / sample_sigma_e;

			result.light += result.transmittance * s_int;
			result.transmittance *= t_r;
			
			if (!depth_stored && result.transmittance < DEPTH_MAX_TRANSMITTANCE)
				result.depth = length(sample_point - ro);

			// Early exit
			if (cloud_data.ltee != 0 && result.transmittance < 0.01)
				break;
		}
	}

	return result;
}

void main()
{
	vec3 ro = camera.pos;
	const vec3 rd = normalize(pixel_world_pos - ro);

	const vec3[2] entry_exit = find_entry_exit(ro, rd);
	const vec3 entry = entry_exit[0];
	const vec3 exit = entry_exit[1];

	if (sky_sdf(entry) > MAX_RAY_DISTANCE)
	{
		out_color = vec4(0.0);
		gl_FragDepth = 1.0;
		return;
	}

	// Sample old color
	/*vec4 old_coord = camera_matrices.old_proj_view * vec4(entry, 1.0);
	vec2 old_uv = old_coord.xy / old_coord.w;
	old_uv.y *= -1.0;
	old_uv += vec2(1.0);
	old_uv /= 2.0;
	vec4 old_color = texture(old_tex, old_uv);

	// High transmittance early exit
	bool in_uv_range = clamp(old_uv.x, 0.0, 1.0) == old_uv.x && clamp(old_uv.y, 0.0, 1.0) == old_uv.y;
	if (cloud_data.htee != 0 && in_uv_range && old_color.w == 0.0)
	{
		out_color = vec4(0.0, 0.0, 0.0, 1.0); // w value = 0?
		gl_FragDepth = 1.0;
		return;
	}*/

	// Calc new color
/*	vec3 sample_points[SAMPLE_COUNT];
	gen_sample_points(entry, exit, sample_points);
	cloud_result_t result = render_cloud(sample_points, -rd, ro);
	out_color = vec4(result.light, 1.0 - result.transmittance);
	gl_FragDepth = 1.0;//result.depth;

	// Blend
	//if (in_uv_range)
		//out_color = out_color * (1.0 - cloud_data.temporal_upsampling) + old_color * cloud_data.temporal_upsampling;
}
*/
