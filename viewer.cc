#define APP_SHORT_NAME "viewer"
#define STB_IMAGE_IMPLEMENTATION

#include <X11/Xutil.h>
#include <cassert>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <memory>
#include <time.h>
#include <unistd.h>

#include <vulkan/vk_sdk_platform.h>
#include <vulkan/vulkan.hpp>

#include "objloader.hh"
#include "stb_image.h"
#include "vulkan.hh"

#define MESH_PATH "assets/df9/model.obj"
#define MESH_DIFFUSE "assets/df9/tex_albedo.jpg"

#define SHADER_COUNT (2)
#define FRAG_SHADER "assets/shaders/diffuse_frag.spv"
#define VERT_SHADER "assets/shaders/diffuse_vert.spv"

#define CLOCKS_PER_TICK (CLOCKS_PER_SEC / 1000000.0f)
#define FRAMERATE (60.0f)
#define CLOCKS_PER_FRAME (CLOCKS_PER_SEC / FRAMERATE)
#define TICKS_PER_FRAME (CLOCKS_PER_TICK / CLOCKS_PER_FRAME)


int main(int argc, char** argv) {
	//=========== VULKAN INITIALIZATION
	vulkan_info_t vulkan_info = { 0 };
	vulkan_info.width = 500;
	vulkan_info.height = 500;

	VkResult res = vulkan_initialize(&vulkan_info);
	if (res != VK_SUCCESS) {
		printf("[Error] Failed initialization : %s\n", vktostring(res));
		return 1;
	}

	//=========== ASSETS LOADING
	model_t model = { 0 };
	if (!load_model(MESH_PATH, &model)) {
		printf("[ERROR] Unable to load a model\n");
		return 1;
	}

	texture_t texture = { 0 };
	stbi_uc *pixels = stbi_load(MESH_DIFFUSE, &texture.width,
															&texture.height, &texture.channels,
															STBI_rgb_alpha);
	if (!pixels)
		return 1;
	printf("[INFO] Loading a texture %dx%d\n", texture.width, texture.height);

	const char *shaders_paths[SHADER_COUNT] = { VERT_SHADER, FRAG_SHADER };
	VkShaderStageFlagBits shaders_flags[SHADER_COUNT] = {
		VK_SHADER_STAGE_VERTEX_BIT, VK_SHADER_STAGE_FRAGMENT_BIT
	};

	res = vulkan_load_shaders(&vulkan_info, SHADER_COUNT, shaders_paths, shaders_flags);
	CHECK_VK(res);
	printf("[INFO] %d shaders loaded.\n", SHADER_COUNT);

	//=========== RENDERING SETUP
	printf("[INFO] Creating vertex buffer.\n");
	res = vulkan_create_vertex_buffer(&vulkan_info, model.count * sizeof(vertex_t),
																		&vulkan_info.vertex_buffer);
	CHECK_VK(res);
	printf("[INFO] Setting vertex buffer data.\n");
	res = vulkan_update_vertex_buffer(&vulkan_info, &vulkan_info.vertex_buffer,
																		model.vertices, model.count);
	CHECK_VK(res);

	res = vulkan_create_pipeline(&vulkan_info);
	CHECK_VK(res);
	printf("[INFO] Done.\n");

	//=========== RENDERING
	clock_t last_tick = clock() / CLOCKS_PER_TICK;
	for (uint32_t i = 0; i < 1200; i++) {
		res = vulkan_begin_command_buffer(&vulkan_info);
		CHECK_VK(res);
		res = vulkan_render_frame(&vulkan_info);
		CHECK_VK(res);

		clock_t current_tick = clock() / CLOCKS_PER_TICK;
		clock_t early = (current_tick - last_tick - TICKS_PER_FRAME);
		if (early > 0) { usleep(early * 25); }
		last_tick = current_tick;
	}

	vulkan_unload_shaders(&vulkan_info, SHADER_COUNT);
	vulkan_cleanup(&vulkan_info);
	return 0;
}
