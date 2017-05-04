#include <vector>
#include <sstream>
#include <chrono>
#include <inttypes.h>

#include "helpers.hh"
#include "stb_image.h"
#include "vulkan.hh"
#include "vulkan_exception.hh"
#include "vulkan_render.hh"
#include "vulkan_wrappers.hh"
#include "assets_loader.hh"

#define MESH_PATH "assets/r5d4/model.obj"
#define MESH_DIFFUSE "assets/r5d4/tex_albedo.jpg"

#define SHADER_COUNT 2
#define FRAG_SHADER "assets/shaders/diffuse_frag.spv"
#define VERT_SHADER "assets/shaders/diffuse_vert.spv"

#define LIMIT_FRAMERATE 1
#define FRAMERATE 120.0f
#define CLOCKS_PER_FRAME ((long int)((1.0F / FRAMERATE) * CLOCKS_PER_SEC))
#define DEG2RAD (0.20943951023f)

//This main is used as a draft, don't worry
int main(int argc, char** argv) {

//=========== VULKAN INITIALIZATION

	vulkan_info_t vulkan_info = { 0 };
	vulkan_info.width = 500;
	vulkan_info.height = 500;
	try {
	vulkan_initialize(&vulkan_info);
	} catch (VkException e) {
		printf("Exception: %s\n", vktostring(e.what()));
		return 1;
	}

//=========== ASSETS LOADING
	
	model_t model = { 0 };
	bool success = load_model(MESH_PATH, &model);
	assert(success);

	texture_t texture = { 0 };
	stbi_uc *pixels = stbi_load(MESH_DIFFUSE, (int32_t*)&texture.width, (int32_t*)&texture.height,
															(int32_t*)&texture.channels, STBI_rgb_alpha);
	assert(pixels);
	printf("[INFO] Loading a texture %dx%d\n", texture.width, texture.height);

	glm::vec3 camera = glm::vec3(2, 0.4f, 2);
	glm::vec3 origin = glm::vec3(0, 0, 0);
	glm::vec3 up = glm::vec3(0, 1, 0);

	scene_info_t scene = { };
	scene.clip = glm::mat4(
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, -1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 0.5f, 0.0f,
		0.0f, 0.0f, 0.5f, 1.0f
	);

	scene.model = glm::mat4(1.0f);
	scene.view = glm::lookAt(camera, origin, up);
	scene.projection = glm::perspective(glm::radians(45.0f), 1.0f, 0.1f, 1000.0f);

//============ INIT RENDERING
	vulkan_frame_info_t frame_info = { 0 };
	try {
		VkCommandBuffer command = command_begin_disposable(&vulkan_info);
		vulkan_create_texture(&vulkan_info, &texture);
		vulkan_update_texture(&vulkan_info, &texture, pixels);

		const char *shaders_paths[SHADER_COUNT] = { VERT_SHADER, FRAG_SHADER };
		VkShaderStageFlagBits shaders_flags[SHADER_COUNT] = { VK_SHADER_STAGE_VERTEX_BIT, VK_SHADER_STAGE_FRAGMENT_BIT };
		vulkan_load_shaders(&vulkan_info, SHADER_COUNT, shaders_paths, shaders_flags);
		printf("[INFO] %d shaders loaded.\n", SHADER_COUNT);

		vulkan_create_vertex_buffer(&vulkan_info, model.count * sizeof(vertex_t), &vulkan_info.vertex_buffer);
		vulkan_update_vertex_buffer(&vulkan_info, &vulkan_info.vertex_buffer, model.vertices, model.count);

		vulkan_create_rendering_pipeline(&vulkan_info);
		render_init_fences(&vulkan_info);

		vulkan_update_uniform_buffer(&vulkan_info, &scene);

		frame_info.clear_color = { 0.0, 0.0, 0.0 };
		frame_info.vertex_count = vulkan_info.vertex_count;
		frame_info.vertex_buffer = vulkan_info.vertex_buffer;
		frame_info.command = vulkan_info.cmd_buffer;

		for (uint32_t i = 0; i < vulkan_info.swapchain_images_count; i++) {
			render_create_cmd(&vulkan_info, &frame_info, i);
			render_create_transition_commands(&vulkan_info, i);
		}

		command_submit_disposable(&vulkan_info, command);

	} catch (VkException e) {
		printf("Exception: %s\n", vktostring(e.what()));
		return 1;
	}

	//=========== RENDERING

	float delta_time = 1.0f / 1000.0;
	auto start_time = std::chrono::steady_clock::now();
	uint64_t frame_count = 0;
	printf("FPS:\n");

	for (uint32_t i = 0; ; i++) {

		render_get_frame(&vulkan_info);

		clock_t frame_start = clock();
		float angle = 50.0f;
		scene.model = glm::rotate(scene.model, angle * delta_time * DEG2RAD, glm::vec3(0,1,0));
		vulkan_update_uniform_buffer(&vulkan_info, &scene);

		render_submit(&vulkan_info, &frame_info);

		clock_t frame_end = clock();
		clock_t early_time = CLOCKS_PER_FRAME - (frame_end - frame_start);

#if LIMIT_FRAMERATE
		usleep(early_time);
#else
		(void)early_time;
#endif

		frame_count++;

		auto cur_time = std::chrono::steady_clock::now();
		auto diff = cur_time - start_time;

		if (std::chrono::duration_cast<std::chrono::milliseconds>(diff).count() > 1000.0f) {
			printf("\b\rFPS: %zu     \n", frame_count);
			frame_count = 0;
			start_time = cur_time;
		}
	}

	printf("\n\n\n");
	vkDeviceWaitIdle(vulkan_info.device);
	render_destroy(&vulkan_info, &frame_info);

	vulkan_unload_shaders(&vulkan_info, SHADER_COUNT);
	vulkan_unload_texture(&vulkan_info, &texture);
	vulkan_cleanup(&vulkan_info);
	stbi_image_free(pixels);
	return 0;
}
