#define APP_SHORT_NAME "viewer"

#include <X11/Xutil.h>
#include <cassert>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <memory>
#include <unistd.h>
#include <time.h>

#include <vulkan/vulkan.hpp>
#include <vulkan/vk_sdk_platform.h>

#include "vulkan.hh"
#include "objloader.hh"


int main(int argc, char** argv) {
	vulkan_info_t vulkan_info = { 0 };
	vulkan_info.width = 500;
	vulkan_info.height = 500;

	model_t model = { 0 };
	if (!load_model("mesh.obj", &model)) {
		printf("[ERROR] Unable to load a model\n");
		return 1;
	}

	VkResult res = vulkan_initialize(&vulkan_info);
	if (res != VK_SUCCESS) {
		printf("[Error] Failed initialization : %s\n", vktostring(res));
		return 1;
	}


#define CLOCKS_PER_TICK (CLOCKS_PER_SEC / 1000000.0f)
#define FRAMERATE (60.0f)
#define CLOCKS_PER_FRAME (CLOCKS_PER_SEC / FRAMERATE)
#define TICKS_PER_FRAME (CLOCKS_PER_TICK / CLOCKS_PER_FRAME)

	clock_t last_tick = clock() / CLOCKS_PER_TICK;

	for (uint32_t i = 0; i < 500; i++) {
		res = vulkan_begin_command_buffer(&vulkan_info);
		res = vulkan_render_frame(&vulkan_info);

		clock_t current_tick = clock() / CLOCKS_PER_TICK;
		clock_t early = (current_tick - last_tick - TICKS_PER_FRAME);
		if (early > 0)
			usleep(early * 25);
		//while (current_tick - last_tick < CLOCKS_PER_FRAME) {
		//	current_tick = clock() / CLOCKS_PER_TICK;
		//	}
		last_tick = current_tick;

	}

	vulkan_cleanup(&vulkan_info);
	return 0;
}
