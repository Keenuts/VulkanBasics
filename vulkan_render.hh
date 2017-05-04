#pragma once

#include <vulkan/vulkan.hpp>

#include "types.hh"
#include "vulkan.hh"

struct vulkan_frame_info_t {
	v3_t clear_color;
	uint32_t vertex_count;
	data_buffer_t vertex_buffer;

	VkCommandBuffer command;
	//Vertex bindings are globals for now
	//Shaders are loaded upfront
};

void render_init_fences(vulkan_info_t *info);
void render_create_transition_commands(vulkan_info_t *info, uint32_t i);
void render_create_cmd(vulkan_info_t *info, vulkan_frame_info_t *frame, int i);
void render_get_frame(vulkan_info_t *info);
void render_submit(vulkan_info_t *info, vulkan_frame_info_t *frame);
void render_destroy(vulkan_info_t *info, vulkan_frame_info_t *frame);
