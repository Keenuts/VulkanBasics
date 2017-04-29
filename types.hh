#pragma once

#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>

struct scene_info_t {
	glm::mat4 clip;
	glm::mat4 model;
	glm::mat4 projection;

	glm::vec3 camera;
	glm::vec3 origin;
	glm::vec3 up;
};

struct v2_t {
	float x, y;
};

struct v3_t : v2_t {
	float z;
};

struct vertex_t {
	v3_t pos;
	v3_t nrm;
	v2_t uv;
};

struct model_t {
	vertex_t *vertices;
	uint32_t count;
};

struct object_info_t {
	glm::mat4 mat;
	model_t model;
};
