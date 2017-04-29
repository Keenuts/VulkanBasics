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

#define CHECK_VK(res) 																			\
	if (res != VK_SUCCESS)   																	\
		return res;

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

static uint32_t clamp(uint32_t value, uint32_t min, uint32_t max) {
	return value < min ? min : (value > max ? max : value);
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

static VkResult set_image_layout(VkCommandBuffer *cmd_buffer, VkImage image,
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
	res = vkGetSwapchainImagesKHR(info->device, info->swapchain, &image_count,
																														images.data());
	CHECK_VK(res);

	info->swapchain_buffers = new swapchain_buffer_t[image_count];
	for (uint32_t i = 0; i < image_count ; i++) {
		info->swapchain_buffers[i].image = images[i];
		set_image_layout(&info->cmd_buffer, images[i], VK_IMAGE_ASPECT_COLOR_BIT,
									 	 VK_IMAGE_LAYOUT_UNDEFINED,
									 	 VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	}

	res = VK_SUCCESS;
	for (uint32_t i = 0; i < image_count ; i++)
	{
		VkImageViewCreateInfo color_image_view = {};
		color_image_view.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		color_image_view.pNext = NULL;
		color_image_view.flags = 0;
		color_image_view.image = info->swapchain_buffers[i].image;
		color_image_view.viewType = VK_IMAGE_VIEW_TYPE_2D;
		color_image_view.format = info->image_format; 
		color_image_view.components.r = VK_COMPONENT_SWIZZLE_R;
		color_image_view.components.g = VK_COMPONENT_SWIZZLE_G;
		color_image_view.components.b = VK_COMPONENT_SWIZZLE_B;
		color_image_view.components.a = VK_COMPONENT_SWIZZLE_A;
		color_image_view.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		color_image_view.subresourceRange.baseMipLevel = 0;
		color_image_view.subresourceRange.levelCount = 1;
		color_image_view.subresourceRange.baseArrayLayer = 0;
		color_image_view.subresourceRange.layerCount = 1;

		res = vkCreateImageView(info->device, &color_image_view, NULL, &info->swapchain_buffers[i].view);
		if (res != VK_SUCCESS)
			break;
	}

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

static bool find_memory_type_index(vulkan_info_t *info, uint32_t type,
																	 VkFlags flags, uint32_t *res) {
	for (uint32_t i = 0; i < info->memory_properties.memoryTypeCount; i++) {
		if ((type & 1) == 1) {
			if ((info->memory_properties.memoryTypes[i].propertyFlags & flags) == flags) {
				*res = i;
#ifdef LOG_VERBOSE
				printf("[DEBUG] Found compatible memory type.\n");
#endif
				return true;
			}
		}
		type >>= 1;
	}
	return false;
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
	return vkAllocateMemory(info->device, &alloc_info, NULL, &info->uniform_buffer.memory);
}

static VkResult initialize_uniform_buffer(vulkan_info_t *info) {
	VkResult res = VK_SUCCESS;

	glm::mat4 projection_matrix = glm::perspective(glm::radians(45.0f), 1.0f, 0.1f, 100.0f);
	glm::vec3 camera = glm::vec3(0, 0, 5);
	glm::vec3 origin = glm::vec3(0, 0, 0);
	glm::vec3 up = glm::vec3(0, 1, 0);

	glm::mat4 view_matrix = glm::lookAt(camera, origin, up);
	glm::mat4 model_matrix = glm::mat4(1.0f);

	glm::mat4 clip_matrix = glm::mat4(
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, -1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 0.5f, 0.0f,
		0.0f, 0.0f, 0.5f, 1.0f);
	
	glm::mat4 payload = clip_matrix * projection_matrix * view_matrix * model_matrix;
	
	VkMemoryRequirements mem_reqs;
	vkGetBufferMemoryRequirements(info->device, info->uniform_buffer.buffer, &mem_reqs);

	uint8_t *p_data = NULL;
	res = vkMapMemory(info->device, info->uniform_buffer.memory, 0, mem_reqs.size, 0,
										(void**)&p_data);
	CHECK_VK(res);

	memcpy(p_data, &payload, sizeof(payload));
	vkUnmapMemory(info->device, info->uniform_buffer.memory);
	res = vkBindBufferMemory(info->device, info->uniform_buffer.buffer,
													 info->uniform_buffer.memory, 0);
	CHECK_VK(res);

	info->uniform_buffer.descriptor.buffer = info->uniform_buffer.buffer;
	info->uniform_buffer.descriptor.offset = 0;
  info->uniform_buffer.descriptor.range = sizeof(payload); 
	return VK_SUCCESS;	
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
	res = vulkan_create_swapchain(info);
	CHECK_VK(res);
	res = vulkan_create_command_buffer(info);
	CHECK_VK(res);
	LOG("Command buffer initialized.");
	res = vulkan_initialize_swachain_images(info);
	CHECK_VK(res);
	LOG("Swapchain initialized");
	res = vulkan_create_depth_buffer(info);
	CHECK_VK(res);
	res = vulkan_create_uniform_buffer(info, sizeof(glm::mat4));
	CHECK_VK(res);
	//TODO: Later, let this take better arguments, think of update routine, etc
	res = initialize_uniform_buffer(info);
	LOG("Uniform buffer initialized");
	CHECK_VK(res);


	
	delete[] queue_info.family_props;

	LOG("Vulkan initialized.");
	return VK_SUCCESS;
}

static void vulkan_destroy_image_buffer(VkDevice device, image_buffer_t buffer) {
	vkDestroyImageView(device, buffer.view, NULL);
	vkDestroyImage(device, buffer.image, NULL);
	vkFreeMemory(device, buffer.memory, NULL);
}

void vulkan_cleanup(vulkan_info_t *info) {
	vulkan_destroy_image_buffer(info->device, info->depth_buffer);	

	for (uint32_t i = 0; i < info->swapchain_images_count; i++)
		vkDestroyImageView(info->device, info->swapchain_buffers[i].view, NULL);
	vkDestroySwapchainKHR(info->device, info->swapchain, NULL);
	delete[] info->swapchain_buffers;

	vkFreeCommandBuffers(info->device, info->cmd_pool, 1, &info->cmd_buffer);
	vkDestroyCommandPool(info->device, info->cmd_pool, NULL);
	vkDeviceWaitIdle(info->device);
	vkDestroyDevice(info->device, NULL);
	vkDestroySurfaceKHR(info->instance, info->surface, NULL);
	destroy_window(&info->window);
	vkDestroyInstance(info->instance, NULL);
}
