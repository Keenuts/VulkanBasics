#include <vector>
#include <sstream>
#include <time.h>

#include "helpers.hh"
#include "stb_image.h"
#include "tiny_obj_loader.hh"
#include "vulkan.hh"
#include "vulkan_exception.hh"
#include "vulkan_render.hh"

#define MESH_PATH "assets/r5d4/model.obj"
#define MESH_DIFFUSE "assets/r5d4/tex_albedo.jpg"

#define SHADER_COUNT 2
#define FRAG_SHADER "assets/shaders/diffuse_frag.spv"
#define VERT_SHADER "assets/shaders/diffuse_vert.spv"

#define FRAMERATE 60.0f
#define CLOCKS_PER_FRAME ((long int)((1.0F / FRAMERATE) * CLOCKS_PER_SEC))
#define DEG2RAD (0.20943951023f)


static bool load_model(const char* path, model_t *model) {
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string err;
	
	bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &err, path);
	if (!ret)
		return false;
	
	std::vector<vertex_t> vertices;
	
	for (tinyobj::shape_t& s : shapes) {
		for (tinyobj::index_t& idx : s.mesh.indices) {
			vertex_t v;
			v.pos = {
				attrib.vertices[3 * idx.vertex_index + 0],
				attrib.vertices[3 * idx.vertex_index + 1],
				attrib.vertices[3 * idx.vertex_index + 2]
			};

			v.nrm = {
				attrib.normals[3 * idx.normal_index + 0],
				attrib.normals[3 * idx.normal_index + 1],
				attrib.normals[3 * idx.normal_index + 2]
			};

			v.uv = {
				attrib.texcoords[2 * idx.texcoord_index + 0],
				attrib.texcoords[2 * idx.texcoord_index + 1],
			};
			vertices.push_back(v);
		}
	}

	printf("[INFO] Loading model %s [%zu vertices]\n", path, vertices.size());

	model->count = vertices.size();

	model->vertices = new vertex_t[model->count];
	if (model->vertices == NULL)
		return false;

	for (uint64_t i = 0; i < model->count; i++)
		model->vertices[i] = vertices[i];

	return true;
}

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

// IMAGE CREATION
	texture_t texture = { 0 };
	stbi_uc *pixels = stbi_load(MESH_DIFFUSE, (int32_t*)&texture.width,
															(int32_t*)&texture.height, (int32_t*)&texture.channels,
															STBI_rgb_alpha);
	assert(pixels);

	vulkan_create_texture(&vulkan_info, &texture);
	vulkan_update_texture(&vulkan_info, &texture, pixels);

	stbi_image_free(pixels);

	printf("[INFO] Loading a texture %dx%d\n", texture.width, texture.height);
//END IMAGE

	const char *shaders_paths[SHADER_COUNT] = { VERT_SHADER, FRAG_SHADER };
	VkShaderStageFlagBits shaders_flags[SHADER_COUNT] = {
		VK_SHADER_STAGE_VERTEX_BIT, VK_SHADER_STAGE_FRAGMENT_BIT
	};

	vulkan_load_shaders(&vulkan_info, SHADER_COUNT, shaders_paths, shaders_flags);
	printf("[INFO] %d shaders loaded.\n", SHADER_COUNT);

	//=========== RENDERING SETUP
	try {

		vulkan_create_vertex_buffer(&vulkan_info, model.count * sizeof(vertex_t), &vulkan_info.vertex_buffer);
		vulkan_update_vertex_buffer(&vulkan_info, &vulkan_info.vertex_buffer, model.vertices, model.count);
		printf("[INFO] Vertex buffer created.\n");
		vulkan_create_rendering_pipeline(&vulkan_info);
		printf("[INFO] Done.\n");

	} catch (VkException e) {
		printf("Exception: %s\n", vktostring(e.what()));
		return 1;
	}
	//=========== RENDERING
	
	vulkan_frame_info_t frame_info = { 0 };
	frame_info.clear_color = { 0.1, 0.1, 0.1 };
	frame_info.vertex_count = vulkan_info.vertex_count;
	frame_info.vertex_buffer = vulkan_info.vertex_buffer;
	frame_info.command = vulkan_info.cmd_buffer;


	glm::vec3 camera = glm::vec3(3, 1, 3);
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
	vulkan_update_uniform_buffer(&vulkan_info, &scene);



	float delta_time = 1.0f / 1000.0;
	for (uint32_t i = 0; ; i++) {
		clock_t start_tick = clock();

		float angle = 50.0f;

		scene.model = glm::rotate(scene.model, angle * delta_time * DEG2RAD, glm::vec3(0,1,0));
		vulkan_update_uniform_buffer(&vulkan_info, &scene);

		render_create_cmd(&vulkan_info, &frame_info);
		render_submit(&vulkan_info, &frame_info);
		render_destroy(&vulkan_info, &frame_info);

		clock_t end_tick = clock();

		clock_t early_time = CLOCKS_PER_FRAME - (end_tick - start_tick);
		usleep(early_time);
	}

	vulkan_unload_shaders(&vulkan_info, SHADER_COUNT);
	vulkan_cleanup(&vulkan_info);
	return 0;
}
