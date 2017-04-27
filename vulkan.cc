#ifndef VK_USE_PLATFORM_XCB_KHR
#define VK_USE_PLATFORM_XCB_KHR
#endif

#define VULKAN_HPP_NO_EXCEPTIONS

#include <X11/Xutil.h>

#include <cassert>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <memory>
#include <unistd.h>


#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <vulkan/vulkan.hpp>
#include <vulkan/vk_sdk_platform.h>

#include "window.hxx"

#include <xcb/xcb.h>
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>

#include "vulkan.hh"

#define LOG(msg) printf("[INFO] %s\n", (msg))
#define CHECK(cond) if (!(cond)) { return false; }

#ifdef LOG_VERBOSE
#define CHECK_VK(res) 																			\
	if (res != VK_SUCCESS) { 																	\
		printf("[DEBUG] Vulkan failed : %s\n", vktostring(res));\
		return false;																						\
	}
#else

#define CHECK_VK(res) 																			\
	if (res != VK_SUCCESS)   																	\
		return false;
#endif

bool vulkan_startup(vulkan_info_t *info) {
	VkApplicationInfo application_info = {
		.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
		.pNext = NULL,
		.pApplicationName = "VulkanIsLove",
		.applicationVersion = 0,
		.pEngineName = "NoName",
		.engineVersion = 1,
		.apiVersion = VK_API_VERSION_1_0,
	};

#ifdef LOG_VERBOSE
	uint32_t ext_nbr = 0;
	CHECK_VK(vkEnumerateInstanceExtensionProperties(NULL, &ext_nbr, NULL));

	VkExtensionProperties *ext = new VkExtensionProperties[ext_nbr];
	CHECK_VK(vkEnumerateInstanceExtensionProperties(NULL, &ext_nbr, ext));

	printf("[DEBUG] Supported instance extensions:\n");
	for (uint32_t i = 0 ; i < ext_nbr ; i++ )
		printf("[DEBUG]\t\t- %s\n", ext[i].extensionName);
	delete ext;
#endif



#ifdef LOG_VERBOSE
	uint32_t layer_nbr = 0;
	CHECK_VK(vkEnumerateInstanceLayerProperties(&layer_nbr, NULL));

	VkLayerProperties *layers = new VkLayerProperties[layer_nbr];
	CHECK_VK(vkEnumerateInstanceLayerProperties(&layer_nbr, layers));

	printf("[DEBUG] Supported instance layers:\n");
	for (uint32_t i = 0 ; i < layer_nbr ; i++ )
		printf("[DEBUG]\t\t%s\n", layers[i].layerName);
	delete layers;
#endif

	std::vector<const char*> extension_names = std::vector<const char*>();
	std::vector<const char*> validation_layers = std::vector<const char*>();

	extension_names.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
	extension_names.push_back(VK_KHR_XCB_SURFACE_EXTENSION_NAME);
	extension_names.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);

#ifdef LOG_VERBOSE
	validation_layers.push_back("VK_LAYER_LUNARG_standard_validation");
	validation_layers.push_back("VK_LAYER_LUNARG_swapchain");
	validation_layers.push_back("VK_LAYER_LUNARG_monitor");
	validation_layers.push_back("VK_LAYER_LUNARG_core_validation");
	//validation_layers.push_back("VK_LAYER_LUNARG_api_dump");
	validation_layers.push_back("VK_LAYER_LUNARG_screenshot");
	validation_layers.push_back("VK_LAYER_LUNARG_object_tracker");
	validation_layers.push_back("VK_LAYER_LUNARG_parameter_validation");
	//validation_layers.push_back("VK_LAYER_RENDERDOC_Capture");
	validation_layers.push_back("VK_LAYER_GOOGLE_unique_objects");
	validation_layers.push_back("VK_LAYER_GOOGLE_threading");

	printf("[DEBUG] Required layers:\n");
	for (const char* s : validation_layers)
		printf("[DEBUG]\t\t%s\n", s);
#endif

	VkInstanceCreateInfo create_info = {
		.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
		.pNext = NULL,
		.flags = 0,
		.pApplicationInfo = &application_info,
		.enabledLayerCount = static_cast<uint32_t>(validation_layers.size()),
		.ppEnabledLayerNames = validation_layers.data(),
		.enabledExtensionCount = static_cast<uint32_t>(extension_names.size()),
		.ppEnabledExtensionNames = extension_names.data(),
	};

	CHECK_VK(vkCreateInstance(&create_info, NULL, &info->instance));
	LOG("Instance created");
	return true;
}

bool vulkan_initialize_devices(vulkan_info *info) {
	uint32_t gpu_count;
	VkResult res = vkEnumeratePhysicalDevices(info->instance, &gpu_count, NULL);
	CHECK(gpu_count > 0);

	VkPhysicalDevice *phys_devices = new VkPhysicalDevice[gpu_count];
	res = vkEnumeratePhysicalDevices(info->instance, &gpu_count, phys_devices);
	info->physical_device = phys_devices[0];

#ifdef LOG_VERBOSE
	printf("[DEBUG] Devices: %u\n", gpu_count);
	for (uint32_t i = 0; i < gpu_count; i++) {
		VkPhysicalDeviceProperties deviceInfo;
		vkGetPhysicalDeviceProperties(phys_devices[i], &deviceInfo);
		printf("[DEBUG]\t\t[%u] %s\n", i, deviceInfo.deviceName);
	}
#endif


	delete phys_devices;
	vkGetPhysicalDeviceMemoryProperties(info->physical_device, &info->memory_properties);
	vkGetPhysicalDeviceProperties(info->physical_device, &info->device_properties);
	printf("[INFO] Selecting device: %s\n", info->device_properties.deviceName); 
	return res;
}


bool vulkan_initialize(vulkan_info_t *info) {
	LOG("Initializing Vulkan...");
	
	CHECK(vulkan_startup(info));
	CHECK(vulkan_initialize_devices(info));

	LOG("Vulkan initialized.");
	return true;
}
