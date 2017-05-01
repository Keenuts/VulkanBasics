#version 400
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (std140, binding = 0) uniform _matrices {
        mat4 clip;
        mat4 model;
        mat4 view;
        mat4 projection;
} udata;

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 uv;

layout (location = 0) out vec4 color_out;
layout (location = 1) out vec3 normal_out;

out gl_PerVertex {
        vec4 gl_Position;
};

void main() 
{
	mat4 MVP = udata.clip * udata.projection * udata.view * udata.model;

	color_out = vec4(0.8, 0.9, 0.9, 1.0);
	normal_out = udata.model * normal;
	gl_Position = MVP * vec4(position, 1.0);
}
