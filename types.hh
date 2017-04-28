#pragma once

#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>

typedef struct scene_info {
	glm::mat4 clip;
	glm::mat4 model;
	glm::mat4 projection;

	glm::vec3 camera;
	glm::vec3 origin;
	glm::vec3 up;
} scene_info_t;

typedef struct v2 {
	float x, y;
} v2_t;

typedef struct v3 : v2 {
	float z;
} v3_t;

typedef struct vertex {
	v3_t pos;
	v3_t nrm;
	v2_t uv;
} vertex_t;

typedef struct model {
	vertex_t *vertices;
	uint32_t count;
} model_t;

typedef struct object_info {
	glm::mat4 mat;
	model_t model;
} object_info_t;

