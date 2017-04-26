#version 400
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (std140, binding = 0) uniform _matrices {
        mat4 MVP;
} matrices;

layout (location = 0) in vec4 pos;
layout (location = 1) in vec4 color_in;
layout (location = 0) out vec4 color_out;

out gl_PerVertex {
        vec4 gl_Position;
};

void main() 
{
	color_out = color_in;
	gl_Position = matrices.MVP * pos;
}
