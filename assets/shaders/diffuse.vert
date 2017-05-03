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

layout (location = 0) out vec4 out_color;
layout (location = 1) out vec3 out_normal;
layout (location = 2) out vec2 out_uv;

out gl_PerVertex {
        vec4 gl_Position;
};

void main() 
{
	mat4 MVP = udata.clip * udata.projection * udata.view * udata.model;

	out_color = vec4(1.0, 1.0, 1.0, 1.0);
	out_uv = uv;
	out_normal = (udata.model * normal.xyzz).xyz;
	gl_Position = MVP * vec4(position, 1.0);
}
