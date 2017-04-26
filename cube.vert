#version 400
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (std140, binding = 0) uniform _matrices {
        mat4 MVP;
} matrices;

layout (location = 0) in vec4 pos;
layout (location = 1) in vec4 in_color;
layout (location = 0) out vec4 out_color;

out gl_PerVertex {
        vec4 gl_Position;
};

void main() 
{
	out_color = in_color;
	gl_Position = matrices.MVP * pos;
}
