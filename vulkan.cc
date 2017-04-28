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


#include <glm/gtc/matrix_transform.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>
#include <vulkan/vk_sdk_platform.h>
#include <vulkan/vulkan.hpp>
#include <xcb/xcb.h>


#include "window.hh"
#include "vulkan.hh"


#define LOG(msg) printf("[INFO] %s\n", (msg))

#ifdef LOG_VERBOSE
#define CHECK_VK(res) 																			\
	if (res != VK_SUCCESS) { 																	\
		printf("[DEBUG] Vulkan failed : %s\n", vktostring(res));\
		return res;																						\
	}
#else

#define CHECK_VK(res) 																			\
	if (res != VK_SUCCESS)   																	\
		return res;
#endif

static VkResult vulkan_startup(vulkan_info_t *info) {
	//Initialize info.instance
	
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
	return VK_SUCCESS;
}

static VkResult vulkan_initialize_devices(vulkan_info_t *info) {
	//Initialize info.physical_device
	//					 info.memory_properties
	//					 info.device_properties

	uint32_t gpu_count;
	CHECK_VK(vkEnumeratePhysicalDevices(info->instance, &gpu_count, NULL));
	if (gpu_count == 0)
		return VK_INCOMPLETE;

	VkPhysicalDevice *phys_devices = new VkPhysicalDevice[gpu_count];
	CHECK_VK(vkEnumeratePhysicalDevices(info->instance, &gpu_count, phys_devices));
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
	return VK_SUCCESS;
}

static uint32_t get_queue_family_index(VkQueueFlagBits bits, uint32_t count,
																			 VkQueueFamilyProperties *props) {
	for (uint32_t i = 0; i < count ; i++) {
		if (props[i].queueFlags & bits)
			return i;
	}

	printf("[ERROR] Unable to queue type: %u\n", bits);
	return (uint32_t)-1;
}

static VkResult vulkan_create_command_pool(vulkan_info_t *info, uint32_t graphic_queue) {
	//Initialize info.cmd_pool

	VkCommandPoolCreateInfo cmd_pool_info = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
		.pNext = NULL,
		.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
		.queueFamilyIndex = graphic_queue,
	};

	CHECK_VK(vkCreateCommandPool(info->device, &cmd_pool_info, NULL, &info->cmd_pool));
	LOG("Command pool created.");
	return VK_SUCCESS;
}

static VkResult vulkan_create_device(vulkan_info_t *info,
												queue_creation_info_t *queue_info) {
	//initialize info.device
	vkGetPhysicalDeviceQueueFamilyProperties(info->physical_device,
																					 &queue_info->count, NULL);

	queue_info->family_props = new VkQueueFamilyProperties[queue_info->count];
	if (queue_info->family_props == NULL)
		return VK_ERROR_OUT_OF_HOST_MEMORY;

	vkGetPhysicalDeviceQueueFamilyProperties(info->physical_device,
																					 &queue_info->count,
																					 queue_info->family_props);

#ifdef LOG_VERBOSE
	uint32_t ext_nbr = 0;
	CHECK_VK(vkEnumerateDeviceExtensionProperties(info->physical_device, NULL,
																										  &ext_nbr, NULL));
	VkExtensionProperties *ext = new VkExtensionProperties[ext_nbr];
	CHECK_VK(vkEnumerateDeviceExtensionProperties(info->physical_device, NULL,
																								&ext_nbr, ext));
	
	printf("[DEBUG] Supported device extensions\n");
	for (uint32_t i = 0 ; i < ext_nbr ; i++ )
		printf("[DEBUG] \t\t- %s\n", ext[i].extensionName);
	delete ext;
#endif

	std::vector<const char*> device_extension_names = std::vector<const char*>();
	device_extension_names.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);


	uint32_t queue_id = get_queue_family_index(VK_QUEUE_GRAPHICS_BIT,
												queue_info->count, queue_info->family_props);
	if (queue_id == (uint32_t)-1)
		return VK_INCOMPLETE;

	float queue_priorities[1] = { 0.0f };
	VkDeviceQueueCreateInfo queue_creation_info {
		.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
		.pNext = NULL,
		.flags = 0,
		.queueFamilyIndex = queue_id,
		.queueCount = 1,
		.pQueuePriorities = queue_priorities,
	};

	VkDeviceCreateInfo create_info = {
		.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
		.pNext = NULL,
		.flags = 0,
		.queueCreateInfoCount = 1,
		.pQueueCreateInfos = &queue_creation_info,
		.enabledLayerCount = 0,
		.ppEnabledLayerNames = NULL,
		.enabledExtensionCount = static_cast<uint32_t>(device_extension_names.size()),
		.ppEnabledExtensionNames = device_extension_names.data(),
		.pEnabledFeatures = NULL,
	};

	CHECK_VK(vkCreateDevice(info->physical_device, &create_info, NULL, &info->device));
	return vulkan_create_command_pool(info, queue_id);
}

static VkResult vulkan_create_queues(vulkan_info_t *info, queue_creation_info_t *queues) {
	queues->present_family_index = UINT32_MAX;
	queues->graphic_family_index = UINT32_MAX;

	VkBool32 *support_present = new VkBool32[queues->count];

	for (uint32_t i = 0; i < queues->count; i++) {
		vkGetPhysicalDeviceSurfaceSupportKHR(info->physical_device,
				i, info->surface, &support_present[i]);
	}

	for (uint32_t i = 0; i < queues->count ; i++) {

		if (queues->family_props[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
			queues->graphic_family_index = i;

		if (support_present[i] == VK_TRUE)
			queues->present_family_index = i;
		if (queues->present_family_index != UINT32_MAX
		 && queues->graphic_family_index != UINT32_MAX)
			break;
	}

	if (queues->present_family_index == UINT32_MAX
	 || queues->graphic_family_index == UINT32_MAX) {
		std::cout << "[Error] No present queue found." << std::endl;
		return VK_INCOMPLETE;
	}

	vkGetDeviceQueue(info->device, queues->graphic_family_index, 0, &info->graphic_queue);
	vkGetDeviceQueue(info->device, queues->present_family_index, 0, &info->present_queue);
	return VK_SUCCESS;
}

VkResult vulkan_create_KHR_surface(vulkan_info_t *info) {
	VkXcbSurfaceCreateInfoKHR create_info = {
		.sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR,
		.pNext = NULL,
		.flags = 0,
		.connection = info->window.connection,
		.window = info->window.window
	};

	return vkCreateXcbSurfaceKHR(info->instance, &create_info, NULL, &info->surface);
}

#define VK_CREATE_GET_ARRAY_INFO(FuncName, VkFunc, VkType) 										 \
VkResult FuncName(vulkan_info_t *info, VkType **s, uint32_t *nbr) {						 \
	*nbr = 0;																																		 \
	VkResult res = VkFunc(info->physical_device, info->surface, nbr, NULL);			 \
	if (res != VK_SUCCESS)																											 \
		return res;																																 \
	if (*nbr <= 0)																															 \
		return VK_INCOMPLETE;																											 \
	*s = new VkType[*nbr];																											 \
	if (*s == NULL)																															 \
		return VK_INCOMPLETE;																											 \
	res = VkFunc(info->physical_device, info->surface, nbr, *s);								 \
	if (res != VK_SUCCESS) {																										 \
		delete *s;																																 \
		return res;																																 \
	}																																						 \
	return VK_SUCCESS;																													 \
}

VK_CREATE_GET_ARRAY_INFO(vulkan_get_supported_formats, vkGetPhysicalDeviceSurfaceFormatsKHR, VkSurfaceFormatKHR)
VK_CREATE_GET_ARRAY_INFO(vulkan_get_supported_modes, vkGetPhysicalDeviceSurfacePresentModesKHR, VkPresentModeKHR)

static uint32_t clamp(uint32_t value, uint32_t min, uint32_t max) {
	return value < min ? min : (value > max ? max : value);
}

VkResult vulkan_set_extents(vulkan_info_t *info) {
  VkSurfaceCapabilitiesKHR capabilities;
	CHECK_VK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(info->physical_device,
																								info->surface, &capabilities));
	float ratio = (float)info->height / (float)info->width;
	info->width = clamp(info->width, capabilities.minImageExtent.width,
								 capabilities.maxImageExtent.width);
	info->height = (float)(info->width * ratio);
	info->height = clamp(info->height, capabilities.minImageExtent.height,
								 capabilities.maxImageExtent.height);

#ifdef LOG_VERBOSE
	printf("[DEBUG] Min extents are %ux%u\n",
				 capabilities.minImageExtent.width, capabilities.minImageExtent.height);
	printf("[DEBUG] Max extents are %ux%u\n",
				 capabilities.maxImageExtent.width, capabilities.maxImageExtent.height);
#endif
	printf("[INFO] Extents set to %ux%u\n", info->width, info->height);

	return VK_SUCCESS;
}

VkResult vulkan_create_swapchain(vulkan_info_t *info) {
	//Getting supported formats
	//And the color space
	
	uint32_t nbr_format;
	uint32_t nbr_modes;
	VkSurfaceFormatKHR *supported_formats = NULL;
	VkPresentModeKHR *supported_modes = NULL;

	VkResult res = vulkan_get_supported_formats(info, &supported_formats, &nbr_format));
	if (res != VK_SUCCESS)
		goto ERROR_FORMATS;
	res = vulkan_get_supported_modes(info, &supported_modes, &nbr_modes);
	if (res != VK_SUCCESS)
		goto ERROR_PRESENT;

	info->color_space = supported_formats[0].colorSpace;


	delete supported_modes;
ERROR_PRESENT:
	delete supported_formats;
ERROR_FORMATS:
	return res;
}



VkResult vulkan_initialize(vulkan_info_t *info) {
	LOG("Initializing Vulkan...");
	
	queue_creation_info_t queue_info = { 0 };

	if (!create_window(&info->window, info->width, info->height))
		return VK_INCOMPLETE;

	CHECK_VK(vulkan_startup(info));
	CHECK_VK(vulkan_initialize_devices(info));
	CHECK_VK(vulkan_create_device(info, &queue_info));
	CHECK_VK(vulkan_create_KHR_surface(info));
	CHECK_VK(vulkan_create_queues(info, &queue_info));
	CHECK_VK(vulkan_create_swapchain(info));
	
	delete queue_info.family_props;

	LOG("Vulkan initialized.");
	return VK_SUCCESS;
}
