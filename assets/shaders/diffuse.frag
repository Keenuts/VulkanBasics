#version 400
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable
layout (location = 0) in vec4 color;
layout (location = 1) in vec3 normal;

layout (location = 0) out vec4 out_color;

void main() {
   out_color = color * dot(normal, normalize(vec3(0.2, 0.9, 0.1)));
}
