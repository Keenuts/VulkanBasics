#version 400
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (binding = 1) uniform sampler2D albedo;

layout (location = 0) in vec4 in_color;
layout (location = 1) in vec3 in_normal;
layout (location = 2) in vec2 in_uv;

layout (location = 0) out vec4 out_color;

void main() {
	vec3 one = vec3(1);
	vec3 light_direction = normalize(vec3(0.3, 0.8, 0.1));

	out_color.rgb = in_color.rgb;

	vec2 uv = in_uv;
	uv.y = 1.0 - in_uv.y;

	out_color.rgb = texture(albedo, uv).rgb;
	out_color.rgb *= clamp(dot(in_normal.xyz, light_direction), 0.05, 1.0);
	out_color = clamp(out_color, 0.0, 1.0);
}
