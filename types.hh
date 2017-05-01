#pragma once

#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>
#include <vulkan/vulkan.hpp>

struct scene_info_t {
	glm::mat4 clip;
	glm::mat4 model;
	glm::mat4 view;
	glm::mat4 projection;
};

struct v2_t {
	float x, y;
};

struct v3_t {
	float x, y, z;
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

struct texture_t {
	uint32_t width;
	uint32_t height;
	uint32_t channels;
	VkDeviceSize size;
	VkImage image;
	VkDeviceMemory memory;
};

//unused
struct object_info_t {
	glm::mat4 mat;
	model_t model;
};

struct swapchain_buffer_t {
	VkImage image;
	VkImageView view;
	VkFramebuffer framebuffer;
};

struct data_buffer_t {
	VkDescriptorBufferInfo descriptor;
	VkDeviceMemory memory;
	VkBuffer buffer;
};

struct image_buffer_t {
	VkFormat format;
	VkImage image;
	VkDeviceMemory memory;
	VkImageView view;
};

