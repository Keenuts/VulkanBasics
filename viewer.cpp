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

#include <vulkan/vulkan.hpp>
#include <vulkan/vk_sdk_platform.h>

#include "vulkan.hh"
#include "objloader.hh"


int main(int argc, char** argv) {
	vulkan_info_t vulkan_info;

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
	return 0;
}
