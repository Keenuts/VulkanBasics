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


int main(int argc, char** argv) {
	vulkan_info_t vulkan_info;

	if (!vulkan_initialize(&vulkan_info))
		return 1;

	return 0;
}
