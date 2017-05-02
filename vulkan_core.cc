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
#include <sys/stat.h>
#include <fcntl.h>


#include <glm/gtc/matrix_transform.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>
#include <vulkan/vk_sdk_platform.h>
#include <vulkan/vulkan.hpp>
#include <xcb/xcb.h>


#include "stb_image.h"
#include "vulkan.hh"
#include "window.hh"
#include "helpers.hh"
#include "vulkan_wrappers.hh"


#define LOG(msg) printf("[INFO] %s\n", (msg))

static VkResult vulkan_startup(vulkan_info_t *info) {
	//Initialize info.instance
	
	VkResult res;
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
	res = vkEnumerateInstanceExtensionProperties(NULL, &ext_nbr, NULL);
	CHECK_VK(res);


	VkExtensionProperties *ext = new VkExtensionProperties[ext_nbr];
	if (ext == NULL)
		return VK_ERROR_OUT_OF_HOST_MEMORY;
	res = vkEnumerateInstanceExtensionProperties(NULL, &ext_nbr, ext);
	CHECK_VK(res);

	printf("[DEBUG] Supported instance extensions:\n");
	for (uint32_t i = 0 ; i < ext_nbr ; i++ )
		printf("[DEBUG]\t\t- %s\n", ext[i].extensionName);
	delete[] ext;
#endif



#ifdef LOG_VERBOSE
	uint32_t layer_nbr = 0;
	res = vkEnumerateInstanceLayerProperties(&layer_nbr, NULL);
	CHECK_VK(res);

	VkLayerProperties *layers = new VkLayerProperties[layer_nbr];
	res = vkEnumerateInstanceLayerProperties(&layer_nbr, layers);
	CHECK_VK(res);

	printf("[DEBUG] Supported instance layers:\n");
	for (uint32_t i = 0 ; i < layer_nbr ; i++ )
		printf("[DEBUG]\t\t%s\n", layers[i].layerName);
	delete[] layers;
#endif

	std::vector<const char*> extension_names = std::vector<const char*>();
	std::vector<const char*> validation_layers = std::vector<const char*>();

	extension_names.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
	extension_names.push_back(VK_KHR_XCB_SURFACE_EXTENSION_NAME);
	extension_names.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);

#ifdef DEBUG
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

# ifdef LOG_VERBOSE
	printf("[DEBUG] Required layers:\n");
	for (const char* s : validation_layers)
		printf("[DEBUG]\t\t%s\n", s);
# endif
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

	res = vkCreateInstance(&create_info, NULL, &info->instance);
	if(res != VK_SUCCESS)
		return res;
	return VK_SUCCESS;
}

static VkResult vulkan_initialize_devices(vulkan_info_t *info) {
	//Initialize info.physical_device
	//					 info.memory_properties
	//					 info.device_properties
	VkResult res = VK_SUCCESS;

	uint32_t gpu_count;
	res = vkEnumeratePhysicalDevices(info->instance, &gpu_count, NULL);
	CHECK_VK(res);
	if (gpu_count == 0)
		return VK_INCOMPLETE;

	VkPhysicalDevice *phys_devices = new VkPhysicalDevice[gpu_count];
	res = vkEnumeratePhysicalDevices(info->instance, &gpu_count, phys_devices);
	CHECK_VK(res);
	info->physical_device = phys_devices[0];

#ifdef LOG_VERBOSE
	printf("[DEBUG] Devices: %u\n", gpu_count);
	for (uint32_t i = 0; i < gpu_count; i++) {
		VkPhysicalDeviceProperties deviceInfo;
		vkGetPhysicalDeviceProperties(phys_devices[i], &deviceInfo);
		printf("[DEBUG]\t\t[%u] %s\n", i, deviceInfo.deviceName);
	}
#endif

	delete[] phys_devices;
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
	VkResult res;

	VkCommandPoolCreateInfo cmd_pool_info = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
		.pNext = NULL,
		.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
		.queueFamilyIndex = graphic_queue,
	};

	res = vkCreateCommandPool(info->device, &cmd_pool_info, NULL, &info->cmd_pool);
	CHECK_VK(res);
	return VK_SUCCESS;
}

struct queue_creation_info_t {
	uint32_t count;
	uint32_t present_family_index;
	uint32_t graphic_family_index;

	VkQueueFamilyProperties *family_props;
};

static VkResult vulkan_create_device(vulkan_info_t *info,
												queue_creation_info_t *queue_info) {
	//initialize info.device
	VkResult res;

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
	res = vkEnumerateDeviceExtensionProperties(info->physical_device, NULL,
																										  &ext_nbr, NULL);
	CHECK_VK(res);
	VkExtensionProperties *ext = new VkExtensionProperties[ext_nbr];
	res = vkEnumerateDeviceExtensionProperties(info->physical_device, NULL,
																								&ext_nbr, ext);
	CHECK_VK(res);
	
	printf("[DEBUG] Supported device extensions\n");
	for (uint32_t i = 0 ; i < ext_nbr ; i++ )
		printf("[DEBUG] \t\t- %s\n", ext[i].extensionName);
	delete[] ext;
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

	res = vkCreateDevice(info->physical_device, &create_info, NULL, &info->device);
	CHECK_VK(res);
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

static VkResult vulkan_create_KHR_surface(vulkan_info_t *info) {
	VkXcbSurfaceCreateInfoKHR create_info = {
		.sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR,
		.pNext = NULL,
		.flags = 0,
		.connection = info->window.connection,
		.window = info->window.window
	};

	return vkCreateXcbSurfaceKHR(info->instance, &create_info, NULL, &info->surface);
}

template <typename VTYPE, VkResult(*F)(VkPhysicalDevice, VkSurfaceKHR, uint32_t*, VTYPE*)>
auto vkGetter(uint32_t *count, VTYPE **array, VkPhysicalDevice dev, VkSurfaceKHR surface) {
	VkResult res = F(dev, surface, count, NULL);
	if (res != VK_SUCCESS)
		return res;
	if (*count <= 0)
		return VK_INCOMPLETE;

	*array = new VTYPE[*count];
	if (!*array)
		return VK_ERROR_OUT_OF_HOST_MEMORY;

	res = F(dev, surface, count, *array);
	if (res != VK_SUCCESS)
		delete[] *array;
	return res;
}

static VkResult vulkan_set_extents(vulkan_info_t *info, VkSurfaceCapabilitiesKHR *caps) {
	float ratio = (float)info->height / (float)info->width;
	info->width = clamp(info->width, caps->minImageExtent.width,
								 caps->maxImageExtent.width);
	info->height = (float)(info->width * ratio);
	info->height = clamp(info->height, caps->minImageExtent.height,
								 caps->maxImageExtent.height);

#ifdef LOG_VERBOSE
	printf("[DEBUG] Min extents are %ux%u\n",
				 caps->minImageExtent.width, caps->minImageExtent.height);
	printf("[DEBUG] Max extents are %ux%u\n",
				 caps->maxImageExtent.width, caps->maxImageExtent.height);
#endif
	printf("[INFO] Extents set to %ux%u\n", info->width, info->height);

	return VK_SUCCESS;
}

static VkResult vulkan_create_swapchain(vulkan_info_t *info) {
	VkResult res;
	uint32_t nbr_formats;
	uint32_t nbr_modes;
	VkSurfaceFormatKHR *supported_formats = NULL;
	VkPresentModeKHR *supported_modes = NULL;
  VkSurfaceCapabilitiesKHR capabilities;

	VkPresentModeKHR present_mode = VK_PRESENT_MODE_FIFO_KHR;
	VkCompositeAlphaFlagBitsKHR composite_alpha_bits = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	VkSwapchainCreateInfoKHR swapchain_info = { };

	res = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(info->physical_device,
																								info->surface, &capabilities);
	CHECK_VK(res);
	res = vulkan_set_extents(info, &capabilities);
	CHECK_VK(res);

	res = vkGetter<VkSurfaceFormatKHR, vkGetPhysicalDeviceSurfaceFormatsKHR>
			 (&nbr_formats, &supported_formats, info->physical_device, info->surface);

	if (res != VK_SUCCESS)
		goto ERROR_FORMATS;

	res = vkGetter<VkPresentModeKHR, vkGetPhysicalDeviceSurfacePresentModesKHR>
			 (&nbr_modes, &supported_modes, info->physical_device, info->surface);
	if (res != VK_SUCCESS)
		goto ERROR_PRESENT;

	if ((nbr_formats == 1 && supported_formats[0].format == VK_FORMAT_UNDEFINED)
		|| nbr_formats == 0) {
		res = VK_INCOMPLETE;
		goto ERROR;
	}

#ifdef LOG_VERBOSE
	printf("[DEBUG] Supported formats KHR: ");
	for (uint32_t i = 0; i < nbr_formats; i++)
		printf("%u, ", (uint32_t)supported_formats[i].format);
	printf("\n");
	printf("[DEBUG] Selected format: %u\n", info->image_format);
#endif

	for (uint32_t i = 0; i < nbr_modes; i++) {
		if (supported_modes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
			present_mode = VK_PRESENT_MODE_MAILBOX_KHR;
			break;
		}
		else if (supported_modes[i] == VK_PRESENT_MODE_IMMEDIATE_KHR)
			present_mode = VK_PRESENT_MODE_IMMEDIATE_KHR;
	}

	info->color_space = supported_formats[0].colorSpace;
	info->image_format = supported_formats[0].format;


	swapchain_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapchain_info.pNext = NULL;
	swapchain_info.flags = 0;
	swapchain_info.surface = info->surface;
	swapchain_info.minImageCount = capabilities.minImageCount;
	swapchain_info.imageFormat = info->image_format;
	swapchain_info.imageColorSpace = info->color_space;
	swapchain_info.imageExtent.width = info->width;
	swapchain_info.imageExtent.height = info->height;
	swapchain_info.imageArrayLayers = 1;
	swapchain_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	swapchain_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	swapchain_info.queueFamilyIndexCount = 0;
	swapchain_info.pQueueFamilyIndices = NULL;
	swapchain_info.preTransform = capabilities.currentTransform;
	swapchain_info.compositeAlpha = composite_alpha_bits;
	swapchain_info.presentMode = present_mode;
	swapchain_info.clipped = VK_FALSE;
	swapchain_info.oldSwapchain = VK_NULL_HANDLE;
	
	
	res = vkCreateSwapchainKHR(info->device, &swapchain_info, NULL, &info->swapchain);

ERROR:
	delete[] supported_modes;
ERROR_PRESENT:
	delete[] supported_formats;
ERROR_FORMATS:
	return res;
}

static VkResult vulkan_create_command_buffer(vulkan_info_t *info)
{
	VkCommandBufferAllocateInfo cmd = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.pNext = NULL,
		.commandPool = info->cmd_pool,
		.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		.commandBufferCount = 1,
	};

	return vkAllocateCommandBuffers(info->device, &cmd, &info->cmd_buffer);
}

//TODO: move to an helper file ?
VkResult set_image_layout(VkCommandBuffer *cmd_buffer, VkImage image,
																 VkImageAspectFlags aspects,
																 VkImageLayout old_layout,
																 VkImageLayout new_layout) {

	VkImageMemoryBarrier barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.pNext = NULL;
	barrier.oldLayout = old_layout;
	barrier.newLayout = new_layout;
	barrier.image = image;
	barrier.subresourceRange.aspectMask = aspects;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.layerCount = 1;

	switch (old_layout) {
		case VK_IMAGE_LAYOUT_PREINITIALIZED:
			barrier.srcAccessMask =
					VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
			break;
		case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
			barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			break;
		case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
			barrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
			break;
		case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
			break;
		case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
			barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
			break;
		default:
			return VK_INCOMPLETE;
	}

	switch (new_layout) {
		case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			break;
		case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
			barrier.srcAccessMask |= VK_ACCESS_TRANSFER_READ_BIT;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
			break;
		case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
			barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
			break;
		case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
			barrier.dstAccessMask |=
					VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
			break;
		case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
			barrier.srcAccessMask =
					VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
			break;
		default:
			return VK_INCOMPLETE;
	}
	
	VkPipelineStageFlagBits srcFlags = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
	VkPipelineStageFlagBits dstFlags = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;

	vkCmdPipelineBarrier(*cmd_buffer, srcFlags, dstFlags, 0, 0, 
											 NULL, 0, NULL, 1, &barrier);
	return VK_SUCCESS;
}

static VkResult vulkan_initialize_swachain_images(vulkan_info_t *info) {
	VkResult res;
	uint32_t image_count = 0;
	res = vkGetSwapchainImagesKHR(info->device, info->swapchain, &image_count, NULL);
	CHECK_VK(res);
	if (image_count == 0)
		return VK_INCOMPLETE;

	std::vector<VkImage> images = std::vector<VkImage>(image_count);
	res = vkGetSwapchainImagesKHR(info->device, info->swapchain, &image_count, images.data());
	CHECK_VK(res);

	info->swapchain_buffers = new swapchain_buffer_t[image_count];
	for (uint32_t i = 0; i < image_count ; i++) {
		info->swapchain_buffers[i].image = images[i];
		set_image_layout(
			&info->cmd_buffer,
			images[i],
			VK_IMAGE_ASPECT_COLOR_BIT,
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
		);
	}

	for (uint32_t i = 0; i < image_count ; i++)
		image_view_create(info, info->swapchain_buffers[i].image, info->image_format, &info->swapchain_buffers[i].view);

	info->current_buffer = 0;
	info->swapchain_images_count = image_count;
	return res;
}

static VkResult vulkan_create_depth_buffer(vulkan_info_t *info) {
	VkResult res = VK_SUCCESS;

	VkImageCreateInfo image_info = {};
	const VkFormat depth_format = VK_FORMAT_D16_UNORM;

	image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	image_info.pNext = NULL;
	image_info.flags = 0;
	image_info.imageType = VK_IMAGE_TYPE_2D;
	image_info.format = depth_format;
	image_info.extent.width = info->width;
	image_info.extent.height = info->height;
	image_info.extent.depth = 1;
	image_info.mipLevels = 1;
	image_info.arrayLayers = 1;
	image_info.samples = VK_SAMPLE_COUNT_1_BIT;
	image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
	image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	image_info.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
	image_info.queueFamilyIndexCount = 0;
	image_info.pQueueFamilyIndices = NULL;
	image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	VkMemoryAllocateInfo alloc_info = {};
	alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	alloc_info.pNext = NULL;
	alloc_info.allocationSize = 0;
	alloc_info.memoryTypeIndex = 0;

	VkImageViewCreateInfo view_info = {};
	view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	view_info.pNext = NULL;
	view_info.flags = 0;
	view_info.image = VK_NULL_HANDLE;
	view_info.format = depth_format;
	view_info.components.r = VK_COMPONENT_SWIZZLE_R;
	view_info.components.g = VK_COMPONENT_SWIZZLE_G;
	view_info.components.b = VK_COMPONENT_SWIZZLE_B;
	view_info.components.a = VK_COMPONENT_SWIZZLE_A;
	view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
	view_info.subresourceRange.baseMipLevel = 0;
	view_info.subresourceRange.levelCount = 1;
	view_info.subresourceRange.baseArrayLayer = 0;
	view_info.subresourceRange.layerCount = 1;
	view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;

	VkMemoryRequirements mem_reqs;
	info->depth_buffer.format = depth_format;
	res = vkCreateImage(info->device, &image_info, NULL, &info->depth_buffer.image);
	CHECK_VK(res);

	vkGetImageMemoryRequirements(info->device, info->depth_buffer.image, &mem_reqs);

	alloc_info.allocationSize = mem_reqs.size;
	res = vkAllocateMemory(info->device, &alloc_info, NULL,
												 &info->depth_buffer.memory);
	CHECK_VK(res);

	res = vkBindImageMemory(info->device, info->depth_buffer.image,
													info->depth_buffer.memory, 0);
	CHECK_VK(res);

	view_info.image = info->depth_buffer.image;
	res = vkCreateImageView(info->device, &view_info, NULL, &info->depth_buffer.view);
	CHECK_VK(res);

	return res;
}

static VkResult vulkan_create_uniform_buffer(vulkan_info_t *info, uint32_t size) {
	VkResult res = VK_SUCCESS;

	VkBufferCreateInfo buf_info = {};
	buf_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	buf_info.pNext = NULL;
	buf_info.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
	buf_info.size = size;
	buf_info.queueFamilyIndexCount = 0;
	buf_info.pQueueFamilyIndices = NULL;
	buf_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	buf_info.flags = 0;

	res = vkCreateBuffer(info->device, &buf_info, NULL, &info->uniform_buffer.buffer);
	CHECK_VK(res);
	
	VkMemoryRequirements mem_reqs;
	vkGetBufferMemoryRequirements(info->device, info->uniform_buffer.buffer, &mem_reqs);

	VkMemoryAllocateInfo alloc_info = {};
	alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	alloc_info.pNext = NULL;
	alloc_info.memoryTypeIndex = 0;
	
	alloc_info.allocationSize = mem_reqs.size;
	bool success = find_memory_type_index(info, mem_reqs.memoryTypeBits,
																				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
																				 | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
																				&alloc_info.memoryTypeIndex);
	if (!success)
		return VK_INCOMPLETE;

	res = vkAllocateMemory(info->device, &alloc_info, NULL, &info->uniform_buffer.memory);
	CHECK_VK(res);

	info->uniform_buffer.descriptor.buffer = info->uniform_buffer.buffer;
	info->uniform_buffer.descriptor.offset = 0;
  info->uniform_buffer.descriptor.range = size; 
	res = vkBindBufferMemory(info->device, info->uniform_buffer.buffer,
													 	info->uniform_buffer.memory, 0);
	return res;
}

VkResult vulkan_update_uniform_buffer(vulkan_info_t *info, scene_info_t *payload) {
	VkResult res = VK_SUCCESS;

	void *data = NULL;
	res = vkMapMemory(info->device, info->uniform_buffer.memory, 0, sizeof(payload), 0,
										(void**)&data);
	CHECK_VK(res);
	memcpy(data, payload, info->uniform_buffer.descriptor.range);
	vkUnmapMemory(info->device, info->uniform_buffer.memory);
	return res;
}

static VkResult vulkan_create_pipeline_layout(vulkan_info_t *info) {
	VkResult res = VK_SUCCESS;

	VkDescriptorSetLayoutBinding layout_binding = {};
	layout_binding.binding = 0;
	layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	layout_binding.descriptorCount = 1;
	layout_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	layout_binding.pImmutableSamplers = NULL;


	VkDescriptorSetLayoutCreateInfo descriptor_layout = {};
	descriptor_layout.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	descriptor_layout.pNext = NULL;
	descriptor_layout.bindingCount = 1;
	descriptor_layout.pBindings = &layout_binding;

	info->descriptor_layouts = new VkDescriptorSetLayout[NUM_DESCRIPTORS];
	res = vkCreateDescriptorSetLayout(info->device, &descriptor_layout, NULL,
																		info->descriptor_layouts);
	CHECK_VK(res);
	
	VkPipelineLayoutCreateInfo pipeline_layout = {};
	pipeline_layout.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipeline_layout.pNext = NULL;
	pipeline_layout.flags = 0;
	pipeline_layout.pushConstantRangeCount = 0;
	pipeline_layout.pPushConstantRanges = NULL;
	pipeline_layout.setLayoutCount = NUM_DESCRIPTORS;	
	pipeline_layout.pSetLayouts = info->descriptor_layouts;

	res = vkCreatePipelineLayout(info->device, &pipeline_layout, NULL,
															 &info->pipeline_layout);
	CHECK_VK(res);

	return res;
}

static VkResult vulkan_create_descriptor_pool(vulkan_info_t *info) {
	VkDescriptorPoolSize type_count[1];
	type_count[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	type_count[0].descriptorCount = 1;

	VkDescriptorPoolCreateInfo pool_info = {};
	pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	pool_info.pNext = NULL;
	pool_info.maxSets = 1;
	pool_info.poolSizeCount = 1;
	pool_info.pPoolSizes = type_count;

	return vkCreateDescriptorPool(info->device, &pool_info, NULL,
																&info->descriptor_pool);
}

static VkResult vulkan_create_descriptors(vulkan_info_t *info) {
	VkResult res = VK_SUCCESS;

	VkDescriptorSetAllocateInfo alloc_info[1];
	alloc_info[0].sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	alloc_info[0].pNext = NULL;
	alloc_info[0].descriptorPool = info->descriptor_pool;
	alloc_info[0].descriptorSetCount = NUM_DESCRIPTORS;
	alloc_info[0].pSetLayouts = info->descriptor_layouts;
	
	info->descriptor_sets = new VkDescriptorSet[NUM_DESCRIPTORS];
	res = vkAllocateDescriptorSets(info->device, alloc_info, info->descriptor_sets);
	CHECK_VK(res);

	VkWriteDescriptorSet writes[1];

	writes[0] = {};
	writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writes[0].pNext = NULL;
	writes[0].dstSet = info->descriptor_sets[0];
	writes[0].descriptorCount = 1;
	writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	writes[0].pBufferInfo = &info->uniform_buffer.descriptor;
	writes[0].dstArrayElement = 0;
	writes[0].dstBinding = 0;

	vkUpdateDescriptorSets(info->device, 1, writes, 0, NULL);

	return VK_SUCCESS;
}

VkResult vulkan_create_render_pass(vulkan_info_t *info) {
	VkAttachmentDescription attachments[2];
	attachments[0].flags = 0;
	attachments[0].format = info->image_format;
	attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
	attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attachments[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	attachments[1].flags = 0;
	attachments[1].format = info->depth_buffer.format;
	attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
	attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference color_reference = {};
	color_reference.attachment = 0;
	color_reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depth_reference = {};
	depth_reference.attachment = 1;
	depth_reference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass = {};
	subpass.flags = 0;
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS; 
	subpass.inputAttachmentCount = 0;
	subpass.pInputAttachments = NULL;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &color_reference;
	subpass.pResolveAttachments = NULL;
	subpass.pDepthStencilAttachment = &depth_reference;
	subpass.preserveAttachmentCount = 0;
	subpass.pPreserveAttachments = NULL;

	VkRenderPassCreateInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.pNext = NULL;
	renderPassInfo.flags = 0;
	renderPassInfo.attachmentCount = 2;
	renderPassInfo.pAttachments = attachments;
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;
	renderPassInfo.dependencyCount = 0;
	renderPassInfo.pDependencies = NULL;

	return vkCreateRenderPass(info->device, &renderPassInfo, NULL, &info->render_pass);
}

struct shader_t {
	uint32_t *bytes;
	uint64_t length;
};

static bool load_shader(const char* path, shader_t *shader) {
	int fd = open(path, O_RDONLY);
	if (fd < 0)
		return false;

#ifdef LOG_VERBOSE
	printf("[INFO] Loading shader: %s\n", path);
#endif

	uint64_t length = lseek(fd, 0, SEEK_END);
	lseek(fd, 0, SEEK_SET);

	uint32_t *bytes = new uint32_t[length / sizeof(uint32_t)];

	uint64_t ret = read(fd, bytes, length);
	close(fd);

	shader->length = length / sizeof(uint32_t);
	shader->bytes = bytes;

	return ret > 0;
}

VkResult vulkan_load_shaders(vulkan_info_t *info, uint32_t count,
														 const char **paths, VkShaderStageFlagBits *flags) {
	VkResult res = VK_SUCCESS;
	info->shader_stages = new VkPipelineShaderStageCreateInfo[count];
	if (info->shader_stages == NULL)
		return VK_ERROR_OUT_OF_HOST_MEMORY;

	for (uint32_t i = 0; i < count; i++) {
		shader_t shader = { 0 };
		if (!load_shader(paths[i], &shader))
			return VK_INCOMPLETE;

		info->shader_stages[i].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		info->shader_stages[i].pNext = NULL;
		info->shader_stages[i].flags = 0;
		info->shader_stages[i].stage = flags[i];
		info->shader_stages[i].pName = "main";
		info->shader_stages[i].pSpecializationInfo = NULL;

		VkShaderModuleCreateInfo module_info = {};
		module_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		module_info.pNext = NULL;
		module_info.flags = 0;
		module_info.codeSize = shader.length * sizeof(uint32_t);
		module_info.pCode = shader.bytes;

		res = vkCreateShaderModule(info->device, &module_info, NULL,
															 &info->shader_stages[i].module);
		delete[] shader.bytes;
		CHECK_VK(res);
	}
	info->shader_stages_count = count;
	return res;
}

static VkResult vulkan_create_framebuffers(vulkan_info_t *info) {
	VkResult res = VK_SUCCESS;
	VkImageView attachments[2];
	attachments[1] = info->depth_buffer.view;

	VkFramebufferCreateInfo framebuffer_info = {};
	framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	framebuffer_info.pNext = NULL;
	framebuffer_info.flags = 0;
	framebuffer_info.renderPass = info->render_pass;
	framebuffer_info.attachmentCount = 2;
	framebuffer_info.pAttachments = attachments;
	framebuffer_info.width = info->width;
	framebuffer_info.height = info->height;
	framebuffer_info.layers = 1;

	uint32_t i = 0;
	info->framebuffers = new VkFramebuffer[info->swapchain_images_count];
	if (info->framebuffers == NULL)
		return VK_ERROR_OUT_OF_HOST_MEMORY;

	for (i = 0; i < info->swapchain_images_count; i++) {
			attachments[0] = info->swapchain_buffers[i].view;
			res = vkCreateFramebuffer(info->device, &framebuffer_info, NULL,
																&info->framebuffers[i]);
			CHECK_VK(res);
	}

	return VK_SUCCESS;
}

static VkResult vulkan_create_vertex_bindings(vulkan_info_t *info) {
	info->vertex_binding.binding = 0;
	info->vertex_binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
	info->vertex_binding.stride = sizeof(vertex_t);
	
	info->vertex_attribute = new VkVertexInputAttributeDescription[3];
	if (info->vertex_attribute == NULL)
		return VK_ERROR_OUT_OF_HOST_MEMORY;

	//POSITION
	info->vertex_attribute[0].location = 0;
	info->vertex_attribute[0].binding = 0;
	info->vertex_attribute[0].format = VK_FORMAT_R32G32B32_SFLOAT;
	info->vertex_attribute[0].offset = 0;
	//NORMAL
	info->vertex_attribute[1].location = 1;
	info->vertex_attribute[1].binding = 0;
	info->vertex_attribute[1].format = VK_FORMAT_R32G32B32_SFLOAT;
	info->vertex_attribute[1].offset = 4 * 3; //Stored as RGBA
	//UV
	info->vertex_attribute[2].location = 2;
	info->vertex_attribute[2].binding = 0;
	info->vertex_attribute[2].format = VK_FORMAT_R32G32_SFLOAT;
	info->vertex_attribute[2].offset = 4 * 3 + 4 * 3; //Stored as RGBA

	return VK_SUCCESS;
}

VkResult vulkan_create_vertex_buffer(vulkan_info_t *info, uint32_t size,
																						data_buffer_t *buffer) {
	VkResult res = VK_SUCCESS;

	VkBufferCreateInfo buffer_info = {};
	buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	buffer_info.pNext = NULL;
	buffer_info.flags = 0; 
	buffer_info.size = size;
	buffer_info.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
	buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE; 
	buffer_info.queueFamilyIndexCount = 0;
	buffer_info.pQueueFamilyIndices = NULL;

	res = vkCreateBuffer(info->device, &buffer_info, NULL, &buffer->buffer);
	CHECK_VK(res);

	VkMemoryRequirements requirements;
	vkGetBufferMemoryRequirements(info->device, buffer->buffer, &requirements);

	VkMemoryAllocateInfo allocation_info = {};
	allocation_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocation_info.pNext = NULL;
	allocation_info.allocationSize = size;
	allocation_info.memoryTypeIndex = 0;
	bool success = find_memory_type_index(info, requirements.memoryTypeBits,
																				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
																					| VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
																				&allocation_info.memoryTypeIndex);
	buffer->descriptor.range = size;
	if (!success)
		return VK_INCOMPLETE;
	return vkAllocateMemory(info->device, &allocation_info, NULL, &buffer->memory);
}

VkResult vulkan_update_vertex_buffer(vulkan_info_t *info, data_buffer_t *buffer,
																		 vertex_t *vertices, uint32_t count) {
	void *ptr = NULL;
	uint32_t data_size = count * sizeof(vertex_t);
	VkResult res = VK_SUCCESS;

	if (buffer->descriptor.range < data_size) {
		LOG("Tried to write more bytes than there is in a buffer");
		return VK_INCOMPLETE;
	}

	res = vkMapMemory(info->device, info->vertex_buffer.memory, 0, data_size, 0, &ptr);
	CHECK_VK(res);

	memcpy(ptr, vertices, data_size);
	vkUnmapMemory(info->device, buffer->memory);

	res = vkBindBufferMemory(info->device, buffer->buffer, buffer->memory, 0);
	CHECK_VK(res);

	info->vertex_count = count;
	return VK_SUCCESS;
}

__attribute__((__used__))
static VkResult vulkan_create_simple_vertex_buffer(vulkan_info_t *info) {
	VkResult res = VK_SUCCESS;

	vertex_t triangle[] = {
		{ { -1, 0, 0 }, { 0, 0, 1 }, { 1, 1 } },
		{ { 0, 1, 0 }, { 0, 0, 1 }, { 1, 0 } },
		{ { 1, 0, 0 }, { 0, 0, 1 }, { 0, 1 } },
	};

	uint32_t buffer_size = sizeof(triangle);
	info->vertex_count += 3;

	res = vulkan_create_vertex_buffer(info, buffer_size , &info->vertex_buffer);
	CHECK_VK(res);
	res = vulkan_update_vertex_buffer(info, &info->vertex_buffer, triangle, 3);
	return res;
}

void vulkan_setup_viewport(vulkan_info_t *info) {
	info->viewport.x = 0;
	info->viewport.y = 0;
	info->viewport.width = info->width;
	info->viewport.height = info->height;
	info->viewport.minDepth = 0.0f;
	info->viewport.maxDepth = 1.0f;
}

void vulkan_setup_scissor(vulkan_info_t *info) {
	info->scissor.extent.width = info->width;
	info->scissor.extent.height = info->height;
	info->scissor.offset.x = 0;
	info->scissor.offset.y = 0;
}

VkResult vulkan_create_pipeline(vulkan_info_t *info) {
	VkDynamicState dynamic_state_enable[VK_DYNAMIC_STATE_RANGE_SIZE];
	memset(dynamic_state_enable, 0, sizeof dynamic_state_enable);

	VkPipelineDynamicStateCreateInfo dynamic_state = {};
	dynamic_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamic_state.pNext = NULL;
	dynamic_state.flags = 0;
	dynamic_state.dynamicStateCount = 0;
	dynamic_state.pDynamicStates = dynamic_state_enable;

	VkPipelineVertexInputStateCreateInfo vtx_input;
	vtx_input.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vtx_input.pNext = NULL;
	vtx_input.flags = 0;
	vtx_input.vertexBindingDescriptionCount = 1;
	vtx_input.pVertexBindingDescriptions = &info->vertex_binding;
	vtx_input.vertexAttributeDescriptionCount = 3;
	vtx_input.pVertexAttributeDescriptions = info->vertex_attribute;

	VkPipelineInputAssemblyStateCreateInfo input_assembly;
	input_assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	input_assembly.pNext = NULL;
	input_assembly.flags = 0;
	input_assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	input_assembly.primitiveRestartEnable = VK_FALSE;

	VkPipelineRasterizationStateCreateInfo rasterization_state;
	rasterization_state.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterization_state.pNext = NULL;
	rasterization_state.flags = 0;
	rasterization_state.depthClampEnable = VK_FALSE;
	rasterization_state.rasterizerDiscardEnable = VK_FALSE;
	rasterization_state.polygonMode = VK_POLYGON_MODE_FILL;
	rasterization_state.cullMode = VK_CULL_MODE_FRONT_BIT; //_NONE
	rasterization_state.frontFace = VK_FRONT_FACE_CLOCKWISE;
	rasterization_state.depthBiasEnable = VK_FALSE;
	rasterization_state.depthBiasConstantFactor = 0;
	rasterization_state.depthBiasClamp = 0;
	rasterization_state.depthBiasSlopeFactor = 0;
	rasterization_state.lineWidth = 1.0f;

	VkPipelineColorBlendAttachmentState attachment_state[1];
	attachment_state[0].blendEnable = VK_FALSE;
	attachment_state[0].srcColorBlendFactor = VK_BLEND_FACTOR_ZERO;
	attachment_state[0].dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
	attachment_state[0].colorBlendOp = VK_BLEND_OP_ADD;
	attachment_state[0].srcAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	attachment_state[0].dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	attachment_state[0].alphaBlendOp = VK_BLEND_OP_ADD;
	attachment_state[0].colorWriteMask = 0xf;

	VkPipelineColorBlendStateCreateInfo color_blend;
	color_blend.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	color_blend.pNext = NULL;
	color_blend.flags = 0;
	color_blend.logicOpEnable = VK_FALSE;
	color_blend.logicOp = VK_LOGIC_OP_NO_OP;
	color_blend.attachmentCount = 1;
	color_blend.pAttachments = attachment_state;

	color_blend.blendConstants[0] = 1.0f;
	color_blend.blendConstants[1] = 1.0f;
	color_blend.blendConstants[2] = 1.0f;
	color_blend.blendConstants[3] = 1.0f;


	VkPipelineViewportStateCreateInfo viewport_state = {};
	viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewport_state.pNext = NULL;
	viewport_state.flags = 0;
	viewport_state.viewportCount = 1;
	viewport_state.pViewports = NULL;
	viewport_state.scissorCount = 1;
	viewport_state.pScissors = NULL;

	dynamic_state_enable[dynamic_state.dynamicStateCount++] = VK_DYNAMIC_STATE_SCISSOR;
	dynamic_state_enable[dynamic_state.dynamicStateCount++] = VK_DYNAMIC_STATE_VIEWPORT;

	VkPipelineDepthStencilStateCreateInfo depth_stencil;
	depth_stencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depth_stencil.pNext = NULL;
	depth_stencil.flags = 0;
	depth_stencil.depthTestEnable = VK_TRUE;
	depth_stencil.depthWriteEnable = VK_TRUE;
	depth_stencil.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
	depth_stencil.depthBoundsTestEnable = VK_FALSE;
	depth_stencil.stencilTestEnable = VK_FALSE;
	depth_stencil.back.failOp = VK_STENCIL_OP_KEEP;
	depth_stencil.back.passOp = VK_STENCIL_OP_KEEP;
	depth_stencil.back.compareOp = VK_COMPARE_OP_ALWAYS;
	depth_stencil.back.compareMask = 0;
	depth_stencil.back.reference = 0;
	depth_stencil.back.depthFailOp = VK_STENCIL_OP_KEEP;
	depth_stencil.back.writeMask = 0;
	depth_stencil.front = depth_stencil.back;
	depth_stencil.minDepthBounds = 0;
	depth_stencil.maxDepthBounds = 0;

	VkPipelineMultisampleStateCreateInfo multisample;
	multisample.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisample.pNext = NULL;
	multisample.flags = 0;
	multisample.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multisample.sampleShadingEnable = VK_FALSE;
	multisample.minSampleShading = 0.0;
	multisample.pSampleMask = NULL;
	multisample.alphaToCoverageEnable = VK_FALSE;
	multisample.alphaToOneEnable = VK_FALSE;

	VkGraphicsPipelineCreateInfo pipeline;
	pipeline.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipeline.pNext = NULL;
	pipeline.flags = 0;
	pipeline.stageCount = info->shader_stages_count;
	pipeline.pStages = info->shader_stages;
	pipeline.pVertexInputState = &vtx_input;
	pipeline.pInputAssemblyState = &input_assembly;
	pipeline.pTessellationState = NULL;
	pipeline.pViewportState = &viewport_state;
	pipeline.pRasterizationState = &rasterization_state;
	pipeline.pMultisampleState = &multisample;
	pipeline.pDepthStencilState = &depth_stencil;
	pipeline.pColorBlendState = &color_blend;
	pipeline.pDynamicState = &dynamic_state;
	pipeline.layout = info->pipeline_layout;
	pipeline.renderPass = info->render_pass;
	pipeline.subpass = 0;
	pipeline.basePipelineHandle = VK_NULL_HANDLE;
	pipeline.basePipelineIndex = 0;

	return vkCreateGraphicsPipelines(info->device, VK_NULL_HANDLE,
																	 1, &pipeline, NULL, &info->pipeline);
}

VkResult vulkan_initialize(vulkan_info_t *info) {
	LOG("Initializing Vulkan...");
	
	queue_creation_info_t queue_info = { 0 };
	VkResult res;

	if (!create_window(&info->window, info->width, info->height))
		return VK_INCOMPLETE;

	res = vulkan_startup(info);
	CHECK_VK(res);
	LOG("Vulkan initialized.");
	res = vulkan_initialize_devices(info);
	CHECK_VK(res);
	res = vulkan_create_device(info, &queue_info);
	CHECK_VK(res);
	LOG("Logical device created initialized.");
	res = vulkan_create_KHR_surface(info);
	CHECK_VK(res);
	res = vulkan_create_queues(info, &queue_info);
	CHECK_VK(res);
	res = vulkan_create_command_buffer(info);
	CHECK_VK(res);
	LOG("Command buffer initialized.");
	res = vulkan_create_swapchain(info);
	CHECK_VK(res);
	res = vulkan_initialize_swachain_images(info);
	CHECK_VK(res);
	LOG("Swapchain initialized");
	res = vulkan_create_depth_buffer(info);
	CHECK_VK(res);
	LOG("Depth buffer initialized");
	res = vulkan_create_descriptor_pool(info);
	CHECK_VK(res);
	LOG("Descriptor pool initialized");

	//END STATIC ZONE

	res = vulkan_create_uniform_buffer(info, sizeof(scene_info_t));
	LOG("Uniform buffer initialized");
	CHECK_VK(res);

	res = vulkan_create_pipeline_layout(info);
	CHECK_VK(res);
	LOG("Pipeline layout initialized.");
	res = vulkan_create_descriptors(info);
	CHECK_VK(res);
	LOG("Descriptors initialized.");
	res = vulkan_create_render_pass(info);
	CHECK_VK(res);
	LOG("Render pass created.");

	vulkan_setup_scissor(info);
	vulkan_setup_viewport(info);

	//SHADERS OLD POSITION
	
	res = vulkan_create_framebuffers(info);
	CHECK_VK(res);
	LOG("Framebuffers initialized.");
	res = vulkan_create_vertex_bindings(info);
	CHECK_VK(res);
	LOG("Vertex bindings done.");
	
	delete[] queue_info.family_props;

	LOG("Vulkan initialized.");
	return VK_SUCCESS;
}

VkResult vulkan_create_texture(vulkan_info_t *info, texture_t *tex) {
	image_create(info, tex->width, tex->height, &tex->storage_image,
										 &tex->storage_memory, &tex->size,
										 VK_FORMAT_R8G8B8A8_UNORM,
										 VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
	image_create(info, tex->width, tex->height, &tex->texture_image,
										 &tex->texture_memory, &tex->size,
										 VK_FORMAT_R8G8B8A8_UNORM,
										 VK_IMAGE_USAGE_TRANSFER_DST_BIT |
										 VK_IMAGE_USAGE_SAMPLED_BIT);
	return VK_SUCCESS;
}

VkResult vulkan_update_texture(vulkan_info_t *info, texture_t *tex, stbi_uc* data) {
	VkResult res = VK_SUCCESS;
	void *ptr = NULL;
	res = vkMapMemory(info->device, tex->storage_memory, 0, tex->size, 0, &ptr);
	CHECK_VK(res);

	VkImageSubresource subresource = {};
	subresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	subresource.mipLevel = 0;
	subresource.arrayLayer = 0;

	VkSubresourceLayout layout;
	vkGetImageSubresourceLayout(info->device, tex->storage_image, &subresource, &layout);

	if (layout.rowPitch == tex->width * 4)
		memcpy(ptr, data, tex->size);
	else {
		uint8_t *bytes = reinterpret_cast<uint8_t*>(data);
		for (uint32_t y = 0; y < tex->height; y++)
			memcpy(&bytes[y * layout.rowPitch], &data[y * tex->width * 4], tex->width * 4);
	}

	vkUnmapMemory(info->device, tex->storage_memory);

	image_layout_transition(info, tex->storage_image,
		VK_FORMAT_R8G8B8A8_UNORM,
		VK_IMAGE_LAYOUT_PREINITIALIZED,
		VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
	);

	image_layout_transition(info, tex->texture_image,
		VK_FORMAT_R8G8B8A8_UNORM,
		VK_IMAGE_LAYOUT_PREINITIALIZED,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
	);

	image_copy(info, tex->storage_image, tex->texture_image, tex->width, tex->height);
	return res;
}


//===== CLEAN FUNCTIONS

static void vulkan_destroy_framebuffers(VkDevice d, VkFramebuffer *b, uint32_t c) {
	for (uint32_t i = 0; i < c; i++)
		vkDestroyFramebuffer(d, b[i], NULL);
	delete[] b;
}

static void vulkan_destroy_data_buffer(VkDevice device, data_buffer_t buffer) {
	vkDestroyBuffer(device, buffer.buffer, NULL);
	vkFreeMemory(device, buffer.memory, NULL);
}

static void vulkan_destroy_image_buffer(VkDevice device, image_buffer_t buffer) {
	vkDestroyImageView(device, buffer.view, NULL);
	vkDestroyImage(device, buffer.image, NULL);
	vkFreeMemory(device, buffer.memory, NULL);
}

void vulkan_unload_shaders(vulkan_info_t *info, uint32_t count) {
	for (uint32_t i = 0; i < count; i++)
		vkDestroyShaderModule(info->device, info->shader_stages[i].module, NULL);
	delete[] info->shader_stages;
}

void vulkan_cleanup(vulkan_info_t *info) {
	vkDestroyPipeline(info->device, info->pipeline, NULL);
	vulkan_destroy_data_buffer(info->device, info->vertex_buffer);
	vulkan_destroy_framebuffers(info->device, info->framebuffers,
															info->swapchain_images_count);


	vkDestroyRenderPass(info->device, info->render_pass, NULL);
	vkDestroyDescriptorPool(info->device, info->descriptor_pool, NULL);
	vkDestroyPipelineLayout(info->device, info->pipeline_layout, NULL);

	for (uint32_t i = 0; i < NUM_DESCRIPTORS; i++)
		vkDestroyDescriptorSetLayout(info->device, info->descriptor_layouts[i], NULL);
	delete[] info->descriptor_layouts;

	vulkan_destroy_data_buffer(info->device, info->uniform_buffer);
	vulkan_destroy_image_buffer(info->device, info->depth_buffer);	

	for (uint32_t i = 0; i < info->swapchain_images_count; i++)
		vkDestroyImageView(info->device, info->swapchain_buffers[i].view, NULL);
	delete[] info->swapchain_buffers;

	vkDestroySwapchainKHR(info->device, info->swapchain, NULL);
	vkFreeCommandBuffers(info->device, info->cmd_pool, 1, &info->cmd_buffer);
	vkDestroyCommandPool(info->device, info->cmd_pool, NULL);
	vkDeviceWaitIdle(info->device);
	vkDestroyDevice(info->device, NULL);
	vkDestroySurfaceKHR(info->instance, info->surface, NULL);
	destroy_window(&info->window);
	vkDestroyInstance(info->instance, NULL);
}
