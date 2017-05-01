#pragma once

#include <vulkan/vulkan.hpp>

#include "types.hh"
#include "vulkan.hh"

struct vulkan_frame_info_t {
	v3_t clear_color;
	uint32_t vertex_count;
	data_buffer_t vertex_buffer;

	VkSemaphore semaphore;
	VkCommandBuffer command;
	//Vertex bindings are globals for now
	//Shaders are loaded upfront
};

VkResult render_create_cmd(vulkan_info_t *info, vulkan_frame_info_t *frame);
VkResult render_submit(vulkan_info_t *info, vulkan_frame_info_t *frame);
void render_destroy(vulkan_info_t *info, vulkan_frame_info_t *frame);
