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

#include "engine.hxx"

// Allow a maximum of two outstanding presentation operations.
#define FRAME_LAG 2
#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))

#define ERR_EXIT(err_msg, err_class) \
	do {                             	 \
		printf("%s\n", err_msg);         \
		fflush(stdout);                  \
		exit(1);                         \
	} while (0)

struct texture_object {
	vk::Sampler sampler;

	vk::Image image;
	vk::ImageLayout imageLayout{vk::ImageLayout::eUndefined};
	vk::MemoryAllocateInfo mem_alloc;
	vk::DeviceMemory mem;
	vk::ImageView view;

	int32_t tex_width{0};
	int32_t tex_height{0};
};

int err_code = 0;

int main(int argc, char** argv) {
	Engine viewer;

	VkResult res = viewer.init();
	printf("[INFO] Init done: %s\n", vulkanErr(res));

	viewer.run();
	for (uint64_t i = 0; i < 0xFFFFFFF; i++)
	{}

	viewer.cleanup();
	
	return err_code;
}
