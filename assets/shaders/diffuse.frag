#version 400
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable
layout (location = 0) in vec4 in_color;
layout (location = 1) in vec3 in_normal;

layout (location = 0) out vec4 out_color;

void main() {
	vec3 one = vec3(1);
	vec3 light_direction = normalize(vec3(0.3, 0.8, 0.1));

	out_color.rgb = vec3(0.5,0.49, 0.5);
	out_color.rgb *= clamp(dot(in_normal.xyz, light_direction), 0.0, 1.0);
	out_color = clamp(out_color, 0.0, 1.0);
}
