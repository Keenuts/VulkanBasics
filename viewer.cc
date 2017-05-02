#include <time.h>

#include "helpers.hh"
#include "objloader.hh"
#include "stb_image.h"
#include "vulkan.hh"
#include "vulkan_render.hh"

#define MESH_PATH "assets/df9/model.obj"
#define MESH_DIFFUSE "assets/df9/tex_albedo.jpg"

#define SHADER_COUNT (2)
#define FRAG_SHADER "assets/shaders/diffuse_frag.spv"
#define VERT_SHADER "assets/shaders/diffuse_vert.spv"

#define FRAMERATE (60.0f)
#define CLOCKS_PER_FRAME ((long int)((1.0F / FRAMERATE) * CLOCKS_PER_SEC))

//This main is used as a draft, don't worry
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

// IMAGE CREATION
	texture_t texture = { 0 };
	stbi_uc *pixels = stbi_load(MESH_DIFFUSE, (int32_t*)&texture.width,
															(int32_t*)&texture.height, (int32_t*)&texture.channels,
															STBI_rgb_alpha);
	if (!pixels)
		return 1;
	res = vulkan_create_texture(&vulkan_info, &texture);
	if (res != VK_SUCCESS) {
		printf("[ERROR] Unable to create a texture\n");
		return 1;
	}
	res = vulkan_update_texture(&vulkan_info, &texture, pixels);
	if (res != VK_SUCCESS) {
		printf("[ERROR] Unable to update a texture\n");
		return 1;
	}
	stbi_image_free(pixels);

//END IMAGE
	printf("[INFO] Loading a texture %dx%d\n", texture.width, texture.height);

	const char *shaders_paths[SHADER_COUNT] = { VERT_SHADER, FRAG_SHADER };
	VkShaderStageFlagBits shaders_flags[SHADER_COUNT] = {
		VK_SHADER_STAGE_VERTEX_BIT, VK_SHADER_STAGE_FRAGMENT_BIT
	};

	res = vulkan_load_shaders(&vulkan_info, SHADER_COUNT, shaders_paths, shaders_flags);
	if (res != VK_SUCCESS) {
		printf("[ERROR] Unable to load a shader\n");
		return 1;
	}
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
	
	vulkan_frame_info_t frame_info = { 0 };
	frame_info.clear_color = { 0.1, 0.1, 0.1 };
	frame_info.vertex_count = vulkan_info.vertex_count;
	frame_info.vertex_buffer = vulkan_info.vertex_buffer;
	frame_info.command = vulkan_info.cmd_buffer;


	glm::vec3 camera = glm::vec3(3, 5, 10);
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

	res = vulkan_update_uniform_buffer(&vulkan_info, &scene);
	if (res != VK_SUCCESS) {
		printf("[ERROR] Unable to update uniform buffer.\n");
		return 1;
	}
	printf("[INFO] Starting render loop.\n");

	float delta_time = 1.0f / 1000.0;
	for (uint32_t i = 0; i < 300; i++) {
		clock_t start_tick = clock();

		float angle = 50.0f;
#define DEG2RAD (0.20943951023f)

		scene.model = glm::rotate(scene.model, angle * delta_time * DEG2RAD, glm::vec3(0,1,0));
		vulkan_update_uniform_buffer(&vulkan_info, &scene);

		res = render_create_cmd(&vulkan_info, &frame_info);
		CHECK_VK(res);
		res = render_submit(&vulkan_info, &frame_info);
		CHECK_VK(res);
		render_destroy(&vulkan_info, &frame_info);

		clock_t end_tick = clock();

		clock_t early_time = CLOCKS_PER_FRAME - (end_tick - start_tick);
		usleep(early_time);
	}

	vulkan_unload_shaders(&vulkan_info, SHADER_COUNT);
	vulkan_cleanup(&vulkan_info);
	return 0;
}
