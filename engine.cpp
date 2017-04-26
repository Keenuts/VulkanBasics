#ifndef VK_USE_PLATFORM_XCB_KHR
#define VK_USE_PLATFORM_XCB_KHR
#endif

#define VULKAN_HPP_NO_EXCEPTIONS

#include <X11/Xutil.h>
//#include <xcb.h>

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



#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <vulkan/vulkan.hpp>
#include <vulkan/vk_sdk_platform.h>

#include "engine.hxx"
#include "window.hxx"

#define NUM_DESCRIPTORS 1
#define FENCE_TIMEOUT 100000000
#define CHECK(r) if ((r) != VK_SUCCESS) { return (r); }
#define LOG_INFO(msg) printf("[INFO] %s\n", (msg))
#define CHECK_COND(cond) if (!(cond)) { return VK_INCOMPLETE; }

Engine::Engine()
	: _width(500),	
	  _height(500), 
		_window(_width, _height),
 		_instance(NULL),
		_phys_devices(NULL),
		_device(0),
		_queue_family_count(0),
		_present_queue_family_index(0),
		_graphic_queue_family_index(0),
		_queue_props(NULL),
		_cmd_pool(0),
		_cmd_buffer(0)
{
	_instance_extension_names = std::vector<const char *>();
	_instance_validation_layers = std::vector<const char *>();
	_device_extension_names = std::vector<const char *>();
	_desc_layout = std::vector<VkDescriptorSetLayout>();
	_desc_set = std::vector<VkDescriptorSet>();
	_framebuffers = NULL;
}

Engine::~Engine()
{
}

VkResult Engine::init()
{
	VkResult res;

	res = initvk();
	CHECK(res);
	LOG_INFO("Vulkan initialized");
	res = getDevices();
	CHECK(res);
	res = getLogicalDevice();
	CHECK(res);
	LOG_INFO("Vulkan logical device created");

	res = createUniformBuffer();
	CHECK(res);
	LOG_INFO("Uniform buffer created");

	res = createCommandPool();
	CHECK(res);
	res = createCommandBuffer();
	CHECK(res);
	LOG_INFO("Command buffer created");

	res = createSwapchain();
	CHECK(res);
	LOG_INFO("Swapchain created");

	res = createDepthBuffer();
	CHECK(res);
	LOG_INFO("Depth buffer created");

	res = createPipelineLayout();
	CHECK(res);
	LOG_INFO("Pipeline layout created");

	res = createDescriptors();
	CHECK(res);
	LOG_INFO("Descriptors created");

	res = createRenderPass();
	CHECK(res);
	LOG_INFO("Render passes created");

	res = createShaders();
	CHECK(res);

	res = createFramebuffers();
	CHECK(res);
	res = BeginCommandBuffer();
	CHECK(res);
	res = createTriangle();
	CHECK(res);
	res = createPipeline();
	CHECK(res);
	LOG_INFO("Initialization done");
	
	return res;
}

VkResult Engine::initvk()
{
	VkApplicationInfo application_info = {
		.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
		.pNext = NULL,
		.pApplicationName = "Viewer",
		.applicationVersion = 0,
		.pEngineName = "toto",
		.engineVersion = 0,
		.apiVersion = VK_API_VERSION_1_0,
	};

	uint32_t ext_nbr = 0;
	VkResult res = vkEnumerateInstanceExtensionProperties(NULL, &ext_nbr, NULL);
	assert(res == VK_SUCCESS && "up");


	VkExtensionProperties *ext = (VkExtensionProperties*)malloc(
				sizeof(VkExtensionProperties) * ext_nbr);
	res = vkEnumerateInstanceExtensionProperties(NULL, &ext_nbr, ext);
	CHECK(res);

#ifdef LOG_VERBOSE
	printf("\tSupported instance extensions:\n");
	for (uint32_t i = 0 ; i < ext_nbr ; i++ )
		printf("\t\t%s\n", ext[i].extensionName);
#endif

	free(ext);


	uint32_t layer_nbr = 0;
	res = vkEnumerateInstanceLayerProperties(&layer_nbr, NULL);
	CHECK(res);

	VkLayerProperties *layers = (VkLayerProperties*)malloc(
				sizeof(VkLayerProperties) * layer_nbr);
	res = vkEnumerateInstanceLayerProperties(&layer_nbr, layers);
	CHECK(res);

#ifdef LOG_VERBOSE
	printf("\tSupported instance layers:\n");
	for (uint32_t i = 0 ; i < layer_nbr ; i++ )
		printf("\t\t%s\n", layers[i].layerName);
#endif
	free(layers);

	// Loading needed extensions
	_instance_extension_names.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
	_instance_extension_names.push_back(VK_KHR_XCB_SURFACE_EXTENSION_NAME);
	_instance_extension_names.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);

	// Validation layers loading
#ifdef LOG_VERBOSE
	_instance_validation_layers.push_back("VK_LAYER_LUNARG_standard_validation");
	_instance_validation_layers.push_back("VK_LAYER_LUNARG_swapchain");
	_instance_validation_layers.push_back("VK_LAYER_GOOGLE_unique_objects");
#endif
	
	VkInstanceCreateInfo create_info = {
		.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
		.pNext = NULL,
		.flags = 0,
		.pApplicationInfo = &application_info,
		.enabledLayerCount = static_cast<uint32_t>(_instance_validation_layers.size()),
		.ppEnabledLayerNames = _instance_validation_layers.data(),
		.enabledExtensionCount = static_cast<uint32_t>(_instance_extension_names.size()),
		.ppEnabledExtensionNames = _instance_extension_names.data(),
	};

	return vkCreateInstance(&create_info, NULL, &_instance);
}

VkResult Engine::getDevices()
{
	uint32_t gpu_count;
	VkResult res = vkEnumeratePhysicalDevices(_instance, &gpu_count, NULL);
#ifdef LOG_VERBOSE
	printf("\tDevices: %u\n", gpu_count);
#endif

	_phys_devices = new VkPhysicalDevice[gpu_count];
	res = vkEnumeratePhysicalDevices(_instance, &gpu_count, _phys_devices);

	for (uint32_t i = 0; i < gpu_count; i++) {
		VkPhysicalDeviceProperties deviceInfo;
		vkGetPhysicalDeviceProperties(_phys_devices[i], &deviceInfo);
#ifdef LOG_VERBOSE
		printf("\t\t[%u] %s\n", i, deviceInfo.deviceName);
#endif
	}

	vkGetPhysicalDeviceMemoryProperties(_phys_devices[0], &_memory_properties);
	vkGetPhysicalDeviceProperties(_phys_devices[0], &_device_properties);
	LOG_INFO(_device_properties.deviceName);
	return res;
}

VkResult Engine::getLogicalDevice()
{
	vkGetPhysicalDeviceQueueFamilyProperties(_phys_devices[0],
																					 &_queue_family_count,
																					 NULL);

	_queue_props = new VkQueueFamilyProperties[_queue_family_count];
	assert(_queue_props != NULL && "Unable to allocate queue props buffer.");

	vkGetPhysicalDeviceQueueFamilyProperties(_phys_devices[0],
																					 &_queue_family_count,
																					 _queue_props);

#ifdef LOG_VERBOSE
	printf("Families: %u\n", _queue_family_count);
#endif
	
	uint32_t ext_nbr = 0;
	VkResult res = vkEnumerateDeviceExtensionProperties(_phys_devices[0], NULL,
																										  &ext_nbr, NULL);
	CHECK(res);

	VkExtensionProperties *ext = (VkExtensionProperties*)malloc(
				sizeof(VkExtensionProperties) * ext_nbr);
	res = vkEnumerateDeviceExtensionProperties(_phys_devices[0], NULL, &ext_nbr, ext);
	CHECK(res);
	
#ifdef LOG_VERBOSE
	printf("\tSupported device extensions\n");
	for (uint32_t i = 0 ; i < ext_nbr ; i++ )
		printf("\t\t- %s\n", ext[i].extensionName);
#endif

	free(ext);

	_device_extension_names.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

	float queue_priorities[1] = { 0.0f };
	VkDeviceQueueCreateInfo queue_info {
		.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
		.pNext = NULL,
		.flags = 0,
		.queueFamilyIndex = getQueueFamilyIndex(VK_QUEUE_GRAPHICS_BIT),
		.queueCount = 1,
		.pQueuePriorities = queue_priorities,
	};

	VkDeviceCreateInfo create_info = {
		.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
		.pNext = NULL,
		.flags = 0,
		.queueCreateInfoCount = 1,
		.pQueueCreateInfos = &queue_info,
		.enabledLayerCount = 0,
		.ppEnabledLayerNames = NULL,
		.enabledExtensionCount = static_cast<uint32_t>(_device_extension_names.size()),
		.ppEnabledExtensionNames = _device_extension_names.data(),
		.pEnabledFeatures = NULL,
	};

	return vkCreateDevice(_phys_devices[0], &create_info, NULL, &_device);
}


uint32_t Engine::getQueueFamilyIndex(VkQueueFlagBits bits)
{
	for (uint32_t i = 0; i < _queue_family_count ; i++)
		if (_queue_props[i].queueFlags & bits)
			return i;

	assert(0 && "Unable to find a queue type");
	return 0;
}

VkResult Engine::createCommandPool()
{
	VkCommandPoolCreateInfo cmd_pool_info = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
		.pNext = NULL,
		.flags = 0,
		.queueFamilyIndex = getQueueFamilyIndex(VK_QUEUE_GRAPHICS_BIT),
	};

	return vkCreateCommandPool(_device,
																		 &cmd_pool_info,
																		 NULL,
																		 &_cmd_pool);
}

VkResult Engine::createCommandBuffer()
{
	VkCommandBufferAllocateInfo cmd = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.pNext = NULL,
		.commandPool = _cmd_pool,
		.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		.commandBufferCount = 1,
	};

	return vkAllocateCommandBuffers(_device, &cmd, &_cmd_buffer);
}

VkResult Engine::createQueues()
{
	_present_queue_family_index = UINT32_MAX;
	_graphic_queue_family_index = UINT32_MAX;

	VkBool32 *pSupportPresent = (VkBool32*)malloc(sizeof(VkBool32)
																								* _queue_family_count);

	for (uint32_t i = 0; i < _queue_family_count ; i++)
		vkGetPhysicalDeviceSurfaceSupportKHR(_phys_devices[0],
		 																		 i, _surface, &pSupportPresent[i]);

	for (uint32_t i = 0; i < _queue_family_count ; i++) {

		if (_queue_props[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
			_graphic_queue_family_index = i;

		if (pSupportPresent[i] == VK_TRUE)
			_present_queue_family_index = i;
		if (_present_queue_family_index != UINT32_MAX
		 && _graphic_queue_family_index != UINT32_MAX)
			break;
	}

	if (_present_queue_family_index == UINT32_MAX
	 || _graphic_queue_family_index == UINT32_MAX) {
		std::cout << "[Error] No present queue found." << std::endl;
		return VK_INCOMPLETE;
	}

	vkGetDeviceQueue(_device, _graphic_queue_family_index, 0, &_graphic_queue);
	vkGetDeviceQueue(_device, _present_queue_family_index, 0, &_present_queue);
	return VK_SUCCESS;
}

static uint32_t clamp(uint32_t value, uint32_t min, uint32_t max) {
	return value < min ? min : (value > max ? max : value);
}

VkResult Engine::createSwapchain()
{
	VkXcbSurfaceCreateInfoKHR create_info = {
		.sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR,
		.pNext = NULL,
		.flags = 0,
		.connection = _window._connection,
		.window = _window._window
	};

	VkResult res = vkCreateXcbSurfaceKHR(_instance, &create_info, NULL, &_surface);
	CHECK(res);

	res = createQueues();
	CHECK(res);

	//Getting supported formats
	//And the color space
	uint32_t supported_formats_nbr = 0;
	res = vkGetPhysicalDeviceSurfaceFormatsKHR(_phys_devices[0], _surface,
																						 &supported_formats_nbr, NULL);
	CHECK(res);
	CHECK_COND(supported_formats_nbr > 0);

	VkSurfaceFormatKHR *supported_formats = static_cast<VkSurfaceFormatKHR*>(
										malloc(sizeof(VkSurfaceFormatKHR) * supported_formats_nbr));
	res = vkGetPhysicalDeviceSurfaceFormatsKHR(_phys_devices[0], _surface,
																		&supported_formats_nbr, supported_formats);
	CHECK(res);

	_color_space = supported_formats[0].colorSpace;
	free(supported_formats);

	// Getting surface capabilities
  VkSurfaceCapabilitiesKHR capabilities;
	res = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(_phys_devices[0], _surface,
																									&capabilities);
	CHECK(res);

	// Getting present modes
	uint32_t present_count = 0;
	res = vkGetPhysicalDeviceSurfacePresentModesKHR(_phys_devices[0], _surface,
																									&present_count, NULL);
	CHECK(res);

	VkPresentModeKHR *modes = static_cast<VkPresentModeKHR*>(
												    malloc(sizeof(VkPresentModeKHR) * present_count));
	res = vkGetPhysicalDeviceSurfacePresentModesKHR(_phys_devices[0], _surface,
																									&present_count, modes);
	CHECK(res);



	_width = clamp(_width, capabilities.minImageExtent.width,
								 capabilities.maxImageExtent.width);
	_height = clamp(_height, capabilities.minImageExtent.height,
								 capabilities.maxImageExtent.height);

#ifdef LOG_VERBOSE
	printf("[INFO] Extents set to %ux%u\n", _width, _height);
#endif

	// Getting Available surface formats
	uint32_t nbrAvailableFormats = 0;
	res = vkGetPhysicalDeviceSurfaceFormatsKHR(_phys_devices[0], _surface,
																						 &nbrAvailableFormats, NULL);
	CHECK(res);
	VkSurfaceFormatKHR* surfaceFormats = (VkSurfaceFormatKHR*)
											malloc(nbrAvailableFormats * sizeof(VkSurfaceFormatKHR));
	res = vkGetPhysicalDeviceSurfaceFormatsKHR(_phys_devices[0], _surface,
																						 &nbrAvailableFormats, surfaceFormats);
	CHECK(res);

#ifdef LOG_VERBOSE
	printf("[DEBUG] Supported formats KHR: ");
	for (uint32_t i = 0; i < nbrAvailableFormats; i++)
		printf("%u, ", (uint32_t)surfaceFormats[i].format);
	printf("\n");
#endif

	if ((nbrAvailableFormats == 1
		&& surfaceFormats[0].format == VK_FORMAT_UNDEFINED)
		|| nbrAvailableFormats == 0)
		return VK_INCOMPLETE;

	_image_format = surfaceFormats[0].format;
	free(surfaceFormats);

#ifdef LOG_VERBOSE
	printf("[DEBUG] Selected format: %u\n", _image_format);
#endif

	//Select presentation mode
	VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;
	for (uint32_t i = 0; i < present_count; i++) {
		if (modes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
			presentMode = VK_PRESENT_MODE_MAILBOX_KHR;
			break;
		}
		else if (modes[i] == VK_PRESENT_MODE_IMMEDIATE_KHR)
			presentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
	}

	//Find Composite mode
	VkCompositeAlphaFlagBitsKHR composite_alpha_bits = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;



	VkSwapchainCreateInfoKHR swapchain_info = { };

	swapchain_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapchain_info.pNext = NULL;
	swapchain_info.flags = 0;
	swapchain_info.surface = _surface;
	swapchain_info.minImageCount = capabilities.minImageCount;
	swapchain_info.imageFormat = _image_format;
	swapchain_info.imageColorSpace = _color_space;
	swapchain_info.imageExtent.width = _width;
	swapchain_info.imageExtent.height = _height;
	swapchain_info.imageArrayLayers = 1;
	swapchain_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	swapchain_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	swapchain_info.queueFamilyIndexCount = 0;
	swapchain_info.pQueueFamilyIndices = NULL;
	swapchain_info.preTransform = capabilities.currentTransform;
	swapchain_info.compositeAlpha = composite_alpha_bits;
	swapchain_info.presentMode = presentMode;
	swapchain_info.clipped = VK_FALSE;
	swapchain_info.oldSwapchain = VK_NULL_HANDLE;
	
	
	res = vkCreateSwapchainKHR(_device, &swapchain_info, NULL, &_swapchain);
	CHECK(res);

	_swapchain_image_count = 0;
	res = vkGetSwapchainImagesKHR(_device, _swapchain, &_swapchain_image_count, NULL);
	CHECK(res);
	CHECK_COND(_swapchain_image_count > 0);

	std::vector<VkImage> images = std::vector<VkImage>(_swapchain_image_count);
	_buffers = std::vector<SwapchainBuffer>(_swapchain_image_count);

	res = vkGetSwapchainImagesKHR(_device, _swapchain, &_swapchain_image_count,
																images.data());
	CHECK(res);
	
	for (uint32_t i = 0; i < _swapchain_image_count ; i++) {
		_buffers[i].image = images[i];
		setImageLayout(_cmd_buffer, images[i], VK_IMAGE_ASPECT_COLOR_BIT,
									 VK_IMAGE_LAYOUT_UNDEFINED,
									 VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	}


	for (uint32_t i = 0; i < _swapchain_image_count ; i++)
	{
		VkImageViewCreateInfo color_image_view = {};
		color_image_view.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		color_image_view.pNext = NULL;
		color_image_view.flags = 0;
		color_image_view.image = _buffers[i].image;
		color_image_view.viewType = VK_IMAGE_VIEW_TYPE_2D;
		color_image_view.format = _image_format; 
		color_image_view.components.r = VK_COMPONENT_SWIZZLE_R;
		color_image_view.components.g = VK_COMPONENT_SWIZZLE_G;
		color_image_view.components.b = VK_COMPONENT_SWIZZLE_B;
		color_image_view.components.a = VK_COMPONENT_SWIZZLE_A;
		color_image_view.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		color_image_view.subresourceRange.baseMipLevel = 0;
		color_image_view.subresourceRange.levelCount = 1;
		color_image_view.subresourceRange.baseArrayLayer = 0;
		color_image_view.subresourceRange.layerCount = 1;

		res = vkCreateImageView(_device, &color_image_view, NULL, &_buffers[i].view);
		CHECK(res);
	}
	_current_buffer = 0;

	return VK_SUCCESS;
}

VkResult Engine::setImageLayout(VkCommandBuffer cmdBuffer, VkImage image,
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

	vkCmdPipelineBarrier(_cmd_buffer, srcFlags, dstFlags, 0, 0, 
											 NULL, 0, NULL, 1, &barrier);
	return VK_SUCCESS;
}

VkResult Engine::createDepthBuffer()
{
	VkImageCreateInfo image_info = {};
	const VkFormat depth_format = VK_FORMAT_D16_UNORM;

	image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	image_info.pNext = NULL;
	image_info.flags = 0;
	image_info.imageType = VK_IMAGE_TYPE_2D;
	image_info.format = depth_format;
	image_info.extent.width = _width;
	image_info.extent.height = _height;
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
	_depth.format = depth_format;
	VkResult res = vkCreateImage(_device, &image_info, NULL, &_depth.image);
	CHECK(res);

	vkGetImageMemoryRequirements(_device, _depth.image, &mem_reqs);

	alloc_info.allocationSize = mem_reqs.size;
	res = vkAllocateMemory(_device, &alloc_info, NULL, &_depth.mem);
	CHECK(res);

	res = vkBindImageMemory(_device, _depth.image, _depth.mem, 0);
	CHECK(res);

	view_info.image = _depth.image;
	res = vkCreateImageView(_device, &view_info, NULL, &_depth.view);
	CHECK(res);

	return VK_SUCCESS;
}

VkResult Engine::findMemoryTypeIndex(uint32_t type, VkFlags flags, uint32_t *res) {
	for (uint32_t i = 0; i < _memory_properties.memoryTypeCount; i++) {
		if ((type & 1) == 1) {
			if ((_memory_properties.memoryTypes[i].propertyFlags & flags) == flags) {
				*res = i;
#ifdef LOG_VERBOSE
				printf("[DEBUG] Found compatible memory type.\n");
#endif
				return VK_SUCCESS;
			}
		}
		type >>= 1;
	}
	return VK_INCOMPLETE;
}

VkResult Engine::createUniformBuffer() {
	_projection_matrix = glm::perspective(glm::radians(45.0f), 1.0f, 0.1f, 100.0f);
	
	glm::vec3 camera = glm::vec3(-5, 3, -10);
	glm::vec3 origin = glm::vec3(0, 0, 0);
	glm::vec3 up = glm::vec3(0, -1, 0);

	_view_matrix = glm::lookAt(camera, origin, up);
	_model_matrix = glm::mat4(1.0f);

	//Vulkan and GL have different orientations (to. ~ Android tex2D)
	_clip_matrix = glm::mat4(
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, -1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 0.5f, 0.0f,
		0.0f, 0.0f, 0.5f, 1.0f);
	
	_MVP = _clip_matrix * _projection_matrix * _view_matrix * _model_matrix;
	
	VkBufferCreateInfo buf_info = {};
	buf_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	buf_info.pNext = NULL;
	buf_info.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
	buf_info.size = sizeof(_MVP);
	buf_info.queueFamilyIndexCount = 0;
	buf_info.pQueueFamilyIndices = NULL;
	buf_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	buf_info.flags = 0;

	VkResult res = vkCreateBuffer(_device, &buf_info, NULL, &_uniform_data.buff);
	CHECK(res);
	
	VkMemoryRequirements mem_reqs;
	vkGetBufferMemoryRequirements(_device, _uniform_data.buff, &mem_reqs);

	VkMemoryAllocateInfo alloc_info = {};
	alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	alloc_info.pNext = NULL;
	alloc_info.memoryTypeIndex = 0;
	
	alloc_info.allocationSize = mem_reqs.size;
	res = findMemoryTypeIndex(mem_reqs.memoryTypeBits,
														VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
															| VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
														&alloc_info.memoryTypeIndex);
	CHECK(res);
	
	res = vkAllocateMemory(_device, &alloc_info, NULL, &(_uniform_data.mem));
	CHECK(res);

	uint8_t *pData = NULL;
	res = vkMapMemory(_device, _uniform_data.mem, 0, mem_reqs.size, 0, (void**)&pData);
	CHECK(res);

	memcpy(pData, &_MVP, sizeof(_MVP));
	vkUnmapMemory(_device, _uniform_data.mem);
	res = vkBindBufferMemory(_device, _uniform_data.buff, _uniform_data.mem, 0);
	CHECK(res);

	_uniform_data.buffer_info.buffer = _uniform_data.buff;
	_uniform_data.buffer_info.offset = 0;
  _uniform_data.buffer_info.range = sizeof(_MVP);
  
	return VK_SUCCESS;	
}

VkResult Engine::createPipelineLayout()
{
	// This command buffer will be used on the vertex stage
	// Used to send data like mv mat
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

	_desc_layout.resize(NUM_DESCRIPTORS);
	VkResult res = vkCreateDescriptorSetLayout(_device, &descriptor_layout, NULL, _desc_layout.data());
	assert(res == VK_SUCCESS);
	
	VkPipelineLayoutCreateInfo pipeline_layout = {};
	pipeline_layout.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipeline_layout.pNext = NULL;
	pipeline_layout.flags = 0;
	pipeline_layout.pushConstantRangeCount = 0;
	pipeline_layout.pPushConstantRanges = NULL;
	pipeline_layout.setLayoutCount = NUM_DESCRIPTORS;	
	pipeline_layout.pSetLayouts = _desc_layout.data();

	res = vkCreatePipelineLayout(_device, &pipeline_layout, NULL, &_pipeline_layout);
	assert(res == VK_SUCCESS);

	return VK_SUCCESS;
	
}

VkResult Engine::createDescriptors()
{
	VkDescriptorPoolSize type_count[1];
	type_count[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	type_count[0].descriptorCount = 1;

	VkDescriptorPoolCreateInfo pool_info = {};
	pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	pool_info.pNext = NULL;
	pool_info.maxSets = 1;
	pool_info.poolSizeCount = 1;
	pool_info.pPoolSizes = type_count;

	VkResult res = vkCreateDescriptorPool(_device, &pool_info, NULL, &_desc_pool);
	assert(res == VK_SUCCESS);

	VkDescriptorSetAllocateInfo alloc_info[1];
	alloc_info[0].sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	alloc_info[0].pNext = NULL;
	alloc_info[0].descriptorPool = _desc_pool;
	alloc_info[0].descriptorSetCount = NUM_DESCRIPTORS;
	alloc_info[0].pSetLayouts = _desc_layout.data();
	
	_desc_set.resize(NUM_DESCRIPTORS);
	res = vkAllocateDescriptorSets(_device, alloc_info, _desc_set.data());
	assert(res == VK_SUCCESS);

	VkWriteDescriptorSet writes[1];

	writes[0] = {};
	writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writes[0].pNext = NULL;
	writes[0].dstSet = _desc_set[0];
	writes[0].descriptorCount = 1;
	writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	writes[0].pBufferInfo = &_uniform_data.buffer_info;
	writes[0].dstArrayElement = 0;
	writes[0].dstBinding = 0;

	vkUpdateDescriptorSets(_device, 1, writes, 0, NULL);

	return VK_SUCCESS;
}

VkResult Engine::createRenderPass()
{
	VkAttachmentDescription attachments[2];
	attachments[0].flags = 0;
	attachments[0].format = _image_format;
	attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
	attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attachments[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	attachments[1].flags = 0;
	attachments[1].format = _depth.format;
	attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
	attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
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

	return vkCreateRenderPass(_device, &renderPassInfo, NULL, &_render_pass);
	//TODO destroy the renderpass
}

static int loadShader(const char* path, VkShaderStageFlagBits stageFlagBits,
										  std::vector<unsigned int>& output) {
	int fd = open(path, O_RDONLY);
	if (fd < 0)
		return 1;

#ifdef LOG_VERBOSE
	printf("[INFO] Loading shader: %s\n", path);
#endif

	uint64_t length = lseek(fd, 0, SEEK_END);
	lseek(fd, 0, SEEK_SET);

	output.resize(length / sizeof(unsigned int));
	printf("\tReading %zu bytes (%zu uint).\n", length, output.size());
	uint32_t ret = read(fd, output.data(), length);
	close(fd);

	return ret < 0;
}


VkResult Engine::createShaders() {

	std::vector<unsigned int> vertShader = std::vector<unsigned int>();
	std::vector<unsigned int> fragShader = std::vector<unsigned int>();

	if (loadShader("cube_vert.spv", VK_SHADER_STAGE_VERTEX_BIT, vertShader))
		return VK_INCOMPLETE;
	if (loadShader("cube_frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT, fragShader))
		return VK_INCOMPLETE;

	//TODO: Do this in a more clever way

	_shader_stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	_shader_stages[0].pNext = NULL;
	_shader_stages[0].flags = 0;
	_shader_stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
	_shader_stages[0].pName = "main";
	_shader_stages[0].pSpecializationInfo = NULL;

	_shader_stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	_shader_stages[1].pNext = NULL;
	_shader_stages[1].flags = 0;
	_shader_stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	_shader_stages[1].pName = "main";
	_shader_stages[1].pSpecializationInfo = NULL;

	VkShaderModuleCreateInfo vertCreateInfo = {};
	vertCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	vertCreateInfo.pNext = NULL;
	vertCreateInfo.flags = 0;
	vertCreateInfo.codeSize = vertShader.size() * sizeof(unsigned int);
	vertCreateInfo.pCode = vertShader.data();

	VkResult res = vkCreateShaderModule(_device, &vertCreateInfo, NULL, &_shader_stages[0].module);
	CHECK(res)

	VkShaderModuleCreateInfo fragCreateInfo = {};
	fragCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	fragCreateInfo.pNext = NULL;
	fragCreateInfo.flags = 0;
	fragCreateInfo.codeSize = fragShader.size() * sizeof(unsigned int);
	fragCreateInfo.pCode = fragShader.data();

	res = vkCreateShaderModule(_device, &fragCreateInfo, NULL, &_shader_stages[1].module);
	return res;
}

VkResult Engine::createFramebuffers() {
    VkImageView attachments[2];
    attachments[1] = _depth.view;

    VkFramebufferCreateInfo framebufferCreationInfo = {};
    framebufferCreationInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferCreationInfo.pNext = NULL;
		framebufferCreationInfo.flags = 0;
    framebufferCreationInfo.renderPass = _render_pass;
    framebufferCreationInfo.attachmentCount = 2;
    framebufferCreationInfo.pAttachments = attachments;
    framebufferCreationInfo.width = _width;
    framebufferCreationInfo.height = _height;
    framebufferCreationInfo.layers = 1;

    uint32_t i = 0;
    _framebuffers = (VkFramebuffer *)malloc(_swapchain_image_count
																						* sizeof(VkFramebuffer));
		assert(_framebuffers != NULL);

		VkResult res = VK_SUCCESS;
    for (i = 0; i < _swapchain_image_count; i++) {
        attachments[0] = _buffers[i].view;
        res = vkCreateFramebuffer(_device, &framebufferCreationInfo, NULL,
																	&_framebuffers[i]);
				CHECK(res);
    }
		return VK_SUCCESS;
}

struct Vertex {
    float posX, posY, posZ, posW;  // Position data
    float r, g, b, a;              // Color
};
#define XYZ1(_x_, _y_, _z_) (_x_), (_y_), (_z_), 1.f


static const Vertex cube[] = {
    // red face
    {XYZ1(-1, -1, 1), XYZ1(1.f, 0.f, 0.f)},
    {XYZ1(-1, 1, 1), XYZ1(1.f, 0.f, 0.f)},
    {XYZ1(1, -1, 1), XYZ1(1.f, 0.f, 0.f)},
    {XYZ1(1, -1, 1), XYZ1(1.f, 0.f, 0.f)},
    {XYZ1(-1, 1, 1), XYZ1(1.f, 0.f, 0.f)},
    {XYZ1(1, 1, 1), XYZ1(1.f, 0.f, 0.f)},
    // green face
    {XYZ1(-1, -1, -1), XYZ1(0.f, 1.f, 0.f)},
    {XYZ1(1, -1, -1), XYZ1(0.f, 1.f, 0.f)},
    {XYZ1(-1, 1, -1), XYZ1(0.f, 1.f, 0.f)},
    {XYZ1(-1, 1, -1), XYZ1(0.f, 1.f, 0.f)},
    {XYZ1(1, -1, -1), XYZ1(0.f, 1.f, 0.f)},
    {XYZ1(1, 1, -1), XYZ1(0.f, 1.f, 0.f)},
    // blue face
    {XYZ1(-1, 1, 1), XYZ1(0.f, 0.f, 1.f)},
    {XYZ1(-1, -1, 1), XYZ1(0.f, 0.f, 1.f)},
    {XYZ1(-1, 1, -1), XYZ1(0.f, 0.f, 1.f)},
    {XYZ1(-1, 1, -1), XYZ1(0.f, 0.f, 1.f)},
    {XYZ1(-1, -1, 1), XYZ1(0.f, 0.f, 1.f)},
    {XYZ1(-1, -1, -1), XYZ1(0.f, 0.f, 1.f)},
    // yellow face
    {XYZ1(1, 1, 1), XYZ1(1.f, 1.f, 0.f)},
    {XYZ1(1, 1, -1), XYZ1(1.f, 1.f, 0.f)},
    {XYZ1(1, -1, 1), XYZ1(1.f, 1.f, 0.f)},
    {XYZ1(1, -1, 1), XYZ1(1.f, 1.f, 0.f)},
    {XYZ1(1, 1, -1), XYZ1(1.f, 1.f, 0.f)},
    {XYZ1(1, -1, -1), XYZ1(1.f, 1.f, 0.f)},
    // magenta face
    {XYZ1(1, 1, 1), XYZ1(1.f, 0.f, 1.f)},
    {XYZ1(-1, 1, 1), XYZ1(1.f, 0.f, 1.f)},
    {XYZ1(1, 1, -1), XYZ1(1.f, 0.f, 1.f)},
    {XYZ1(1, 1, -1), XYZ1(1.f, 0.f, 1.f)},
    {XYZ1(-1, 1, 1), XYZ1(1.f, 0.f, 1.f)},
    {XYZ1(-1, 1, -1), XYZ1(1.f, 0.f, 1.f)},
    // cyan face
    {XYZ1(1, -1, 1), XYZ1(0.f, 1.f, 1.f)},
    {XYZ1(1, -1, -1), XYZ1(0.f, 1.f, 1.f)},
    {XYZ1(-1, -1, 1), XYZ1(0.f, 1.f, 1.f)},
    {XYZ1(-1, -1, 1), XYZ1(0.f, 1.f, 1.f)},
    {XYZ1(1, -1, -1), XYZ1(0.f, 1.f, 1.f)},
    {XYZ1(-1, -1, -1), XYZ1(0.f, 1.f, 1.f)},
};

VkResult Engine::createVertexBuffer(uint32_t size, uniformBuffer *buffer) {

	if (buffer == NULL)
		return VK_INCOMPLETE;

	VkBufferCreateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	info.pNext = NULL;
	info.flags = 0; 
	info.size = size;
	info.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
	info.sharingMode = VK_SHARING_MODE_EXCLUSIVE; 
	info.queueFamilyIndexCount = 0;
	info.pQueueFamilyIndices = NULL;

	VkResult res = vkCreateBuffer(_device, &info, NULL, &(buffer->buff));
	CHECK(res);

	VkMemoryRequirements requirements;
	vkGetBufferMemoryRequirements(_device, buffer->buff, &requirements);

	VkMemoryAllocateInfo allocation_info = {};
	allocation_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocation_info.pNext = NULL;
	allocation_info.allocationSize = size;
	allocation_info.memoryTypeIndex = 0;
	res = findMemoryTypeIndex(requirements.memoryTypeBits,
														VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
															| VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
														&allocation_info.memoryTypeIndex);
	//TODO: Update memoryTypeIndex

	res = vkAllocateMemory(_device, &allocation_info, NULL, &(buffer->mem));
	CHECK(res);

	return VK_SUCCESS;
}

VkResult Engine::BeginCommandBuffer() {
	VkCommandBufferBeginInfo cmdInfo = {};
	cmdInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	cmdInfo.pNext = NULL;
	cmdInfo.flags = 0;
	cmdInfo.pInheritanceInfo = NULL;
	VkResult res = vkBeginCommandBuffer(_cmd_buffer, &cmdInfo);
	CHECK(res);

	return res;
}

VkResult Engine::createTriangle() {

	VkResult res = createVertexBuffer(sizeof(cube), &_vertex_buffer);
	CHECK(res);

	void *ptr = NULL;
	res = vkMapMemory(_device, _vertex_buffer.mem, 0, sizeof(cube), 0, &ptr);
	CHECK(res);
	memcpy(ptr, cube, sizeof(cube));
	vkUnmapMemory(_device, _vertex_buffer.mem);

	res = vkBindBufferMemory(_device, _vertex_buffer.buff, _vertex_buffer.mem, 0);
	CHECK(res);

	_vtx_binding.binding = 0;
	_vtx_binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
	_vtx_binding.stride = sizeof(cube[0]);
	
	_vtx_attribute[0].location = 0;
	_vtx_attribute[0].binding = 0;
	_vtx_attribute[0].format = VK_FORMAT_R32G32B32A32_SFLOAT;
	_vtx_attribute[0].offset = 0;
	_vtx_attribute[1].location = 0;
	_vtx_attribute[1].binding = 0;
	_vtx_attribute[1].format = VK_FORMAT_R32G32B32A32_SFLOAT;
	_vtx_attribute[1].offset = 16; //Stored as RGBA

	return VK_SUCCESS;
}

VkResult Engine::createPipeline() {

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
	vtx_input.pVertexBindingDescriptions = &_vtx_binding;
	vtx_input.vertexAttributeDescriptionCount = 2;
	vtx_input.pVertexAttributeDescriptions = _vtx_attribute;

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
	rasterization_state.cullMode = VK_CULL_MODE_BACK_BIT;
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
	pipeline.stageCount = 2;
	pipeline.pStages = _shader_stages;
	pipeline.pVertexInputState = &vtx_input;
	pipeline.pInputAssemblyState = &input_assembly;
	pipeline.pTessellationState = NULL;
	pipeline.pViewportState = &viewport_state;
	pipeline.pRasterizationState = &rasterization_state;
	pipeline.pMultisampleState = &multisample;
	pipeline.pDepthStencilState = &depth_stencil;
	pipeline.pColorBlendState = &color_blend;
	pipeline.pDynamicState = &dynamic_state;
	pipeline.layout = _pipeline_layout;
	pipeline.renderPass = _render_pass;
	pipeline.subpass = 0;
	pipeline.basePipelineHandle = VK_NULL_HANDLE;
	pipeline.basePipelineIndex = 0;

	return vkCreateGraphicsPipelines(_device, VK_NULL_HANDLE,
																	 1, &pipeline, NULL, &_pipeline);
}

VkResult Engine::run()
{
	
	VkClearValue clear_values[2];
	clear_values[0].color = {0.2f, 0.2f, 0.2f, 0.2f };
	clear_values[1].depthStencil.depth = 1.0f;
	clear_values[1].depthStencil.stencil = 0;

	VkSemaphore semaphore;

	VkSemaphoreCreateInfo semaphore_create_info;
	semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	semaphore_create_info.pNext = NULL;
	semaphore_create_info.flags = 0;
	//Seriously ?
	
	VkResult res = vkCreateSemaphore(_device, &semaphore_create_info, NULL,
																	 &semaphore);
	CHECK(res);

	res = vkAcquireNextImageKHR(_device, _swapchain, UINT64_MAX, semaphore,
															VK_NULL_HANDLE, &_current_buffer);
	CHECK(res);
	
	setImageLayout(_cmd_buffer, _buffers[_current_buffer].image, VK_IMAGE_ASPECT_COLOR_BIT,
								 VK_IMAGE_LAYOUT_UNDEFINED,
								 VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

	VkRenderPassBeginInfo passBeginInfo = {};
	passBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	passBeginInfo.pNext = NULL;
	passBeginInfo.renderPass = _render_pass;
	passBeginInfo.framebuffer = _framebuffers[_current_buffer];
	passBeginInfo.renderArea.offset.x = 0;
	passBeginInfo.renderArea.offset.y = 0;
	passBeginInfo.renderArea.extent.width = _width;
	passBeginInfo.renderArea.extent.height = _height;
	passBeginInfo.clearValueCount = 2;
	passBeginInfo.pClearValues = clear_values;
	
	vkCmdBeginRenderPass(_cmd_buffer, &passBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
	vkCmdBindPipeline(_cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _pipeline);
	vkCmdBindDescriptorSets(_cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
													_pipeline_layout, 0, NUM_DESCRIPTORS,
													_desc_set.data(), 0, NULL);

	const VkDeviceSize offsets[1] = { 0 };
	vkCmdBindVertexBuffers(_cmd_buffer, 0, 1, &_vertex_buffer.buff, offsets);
	
	_viewport.x = 0;
	_viewport.y = 0;
	_viewport.width = _width;
	_viewport.height = _height;
	_viewport.minDepth = 0.0f;
	_viewport.maxDepth = 1.0f;
	vkCmdSetViewport(_cmd_buffer, 0, 1, &_viewport);

	_scissor.extent.width = _width;
	_scissor.extent.height = _height;
	_scissor.offset.x = 0;
	_scissor.offset.y = 0;
	vkCmdSetScissor(_cmd_buffer, 0, 1, &_scissor);

	vkCmdDraw(_cmd_buffer, 12 * 3, 1, 0, 0);
	vkCmdEndRenderPass(_cmd_buffer);

	res = vkEndCommandBuffer(_cmd_buffer);
	CHECK(res);
	const VkCommandBuffer command_buffers[] = { _cmd_buffer };

	VkFenceCreateInfo fence_info;
	VkFence draw_fence;
	fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fence_info.pNext = NULL;
	fence_info.flags = 0;
	vkCreateFence(_device, &fence_info, NULL, &draw_fence);

	VkPipelineStageFlags pipe_stage_flags = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	VkSubmitInfo submit_info[1] = {};
	submit_info[0].sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit_info[0].pNext = NULL;
	submit_info[0].waitSemaphoreCount = 1;
	submit_info[0].pWaitSemaphores = &semaphore;
	submit_info[0].pWaitDstStageMask = &pipe_stage_flags;
	submit_info[0].commandBufferCount = 1;
	submit_info[0].pCommandBuffers = command_buffers;
	submit_info[0].signalSemaphoreCount = 0;
	submit_info[0].pSignalSemaphores = NULL;

	res = vkQueueSubmit(_graphic_queue, 1, submit_info, draw_fence);
	CHECK(res);

	VkPresentInfoKHR present;
	present.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	present.pNext = NULL;
	present.swapchainCount = 1;
	present.pSwapchains = &_swapchain;
	present.pImageIndices = &_current_buffer;
	present.pWaitSemaphores = NULL;
	present.waitSemaphoreCount = 0;
	present.pResults = NULL;

	do {
			res = vkWaitForFences(_device, 1, &draw_fence, VK_TRUE, FENCE_TIMEOUT);
	} while (res == VK_TIMEOUT);
	CHECK(res);

	return vkQueuePresentKHR(_present_queue, &present);
}

void Engine::cleanup()
{
}
