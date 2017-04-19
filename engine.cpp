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


#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <vulkan/vulkan.hpp>
#include <vulkan/vk_sdk_platform.h>

#include "engine.hxx"
#include "window.hxx"

Engine::Engine()
	: _width(500),	
	  _height(500), 
		_window(_width, _height),
 		_instance(NULL),
		_phys_devices(NULL),
		_device(0),
		_queue_family_count(0),
		_present_queue_index(0),
		_graphic_queue_index(0),
		_queue_props(NULL),
		_cmd_pool(0),
		_cmd_buffer(0)
{
	_instance_extension_names = std::vector<const char *>();
	_device_extension_names = std::vector<const char *>();
	_desc_layout = std::vector<VkDescriptorSetLayout>();
	_desc_set = std::vector<VkDescriptorSet>();

/*
	_camera = vec3(3, 5, 0);
	_origin = vec3(0, 0, 0);
	_up = vec3(0, 1, 0);*/
}

Engine::~Engine()
{
}

VkResult Engine::init()
{
	VkResult res;
	
	//_window = XcbWindows(_width, _height);

	res = initvk();
	assert(res == VK_SUCCESS && "Unable to do initvk()");

	res = getDevices();
	assert(res == VK_SUCCESS && "Unable to do getDevices()");
		
	res = getLogicalDevice();
	assert(res == VK_SUCCESS && "Unable to do getLogicalDevice()");

	res = createCommandPool();
	assert(res == VK_SUCCESS && "Unable to do createCommandPool()");

	res = createCommandBuffer();
	assert(res == VK_SUCCESS && "Unable to do createCommandBuffer()");

	res = createSwapchain();
	assert(res == VK_SUCCESS && "Unable to do createSwapchain()");

	res = createDepthBuffer();
	assert(res == VK_SUCCESS && "Unable to do createDepthBuffer()");

	res = createPipeline();
	assert(res == VK_SUCCESS && "Unable to do createPipeline()");

	res = initDescriptors();
	assert(res == VK_SUCCESS && "Unable to do initDescriptors");

	printf("%s\n", vulkanErr(res));
	
	return res;
}

VkResult Engine::initvk()
{
	VkApplicationInfo application_info = {
		.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
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
	std::cout << "Supported instance extensions" << std::endl;

	VkExtensionProperties *ext = (VkExtensionProperties*)malloc(
				sizeof(VkExtensionProperties) * ext_nbr);
	res = vkEnumerateInstanceExtensionProperties(NULL, &ext_nbr, ext);
	assert(res == VK_SUCCESS && "vkinit: unable to enumerate extensions.");

	for (uint32_t i = 0 ; i < ext_nbr ; i++ )
		std::cout << "\t" << ext[i].extensionName << std::endl;
	free(ext);

	// Loading needed extensions
	_instance_extension_names.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
	_instance_extension_names.push_back(VK_KHR_XCB_SURFACE_EXTENSION_NAME);
	_instance_extension_names.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
	
	VkInstanceCreateInfo create_info = {
		.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
		.pNext = NULL,
		.flags = 0,
		.pApplicationInfo = &application_info,
		.enabledLayerCount = 0,
		.ppEnabledLayerNames = NULL,
		.enabledExtensionCount = static_cast<uint32_t>(_instance_extension_names.size()),
		.ppEnabledExtensionNames = _instance_extension_names.data(),
	};

	return vkCreateInstance(&create_info, NULL, &_instance);
}

VkResult Engine::getDevices()
{
	uint32_t gpu_count;
	VkResult res = vkEnumeratePhysicalDevices(_instance, &gpu_count, NULL);
	std::cout << "Devices: " << gpu_count << std::endl;

	_phys_devices = new VkPhysicalDevice[gpu_count];
	res = vkEnumeratePhysicalDevices(_instance, &gpu_count, _phys_devices);
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

	std::cout << "Families: " << _queue_family_count << std::endl;
	
	uint32_t ext_nbr = 0;
	VkResult res = vkEnumerateDeviceExtensionProperties(_phys_devices[0], NULL, &ext_nbr, NULL);
	assert(res == VK_SUCCESS && "up");
	std::cout << "Supported device extensions" << std::endl;

	VkExtensionProperties *ext = (VkExtensionProperties*)malloc(
				sizeof(VkExtensionProperties) * ext_nbr);
	res = vkEnumerateDeviceExtensionProperties(_phys_devices[0], NULL, &ext_nbr, ext);
	assert(res == VK_SUCCESS && "getLogicalDevice: unable to enumerate extensions.");
	
	for (uint32_t i = 0 ; i < ext_nbr ; i++ )
		std::cout << "\t" << ext[i].extensionName << std::endl;
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
	assert(1 && "Unable to find a queue type");
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

	VkResult res = vkCreateCommandPool(_device,
																		 &cmd_pool_info,
																		 NULL,
																		 &_cmd_pool);
	return res;
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
	_present_queue_index = UINT32_MAX;
	_graphic_queue_index = UINT32_MAX;

	VkBool32 *pSupportPresent = (VkBool32*)malloc(sizeof(VkBool32)
																								* _queue_family_count);

	for (uint32_t i = 0; i < _queue_family_count ; i++)
		vkGetPhysicalDeviceSurfaceSupportKHR(_phys_devices[0],
		 																		 i, _surface, &pSupportPresent[i]);

	for (uint32_t i = 0; i < _queue_family_count ; i++) {

		if (_queue_props[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
			_graphic_queue_index = i;

		if (pSupportPresent[i] == VK_TRUE)
			_present_queue_index = i;
		if (_present_queue_index != UINT32_MAX && _graphic_queue_index != UINT32_MAX)
			break;
	}

	if (_present_queue_index == UINT32_MAX || _graphic_queue_index == UINT32_MAX) {
		std::cout << "[Error] No present queue found." << std::endl;
		return VK_INCOMPLETE;
	}
	return VK_SUCCESS;
}

VkResult Engine::getSupportedFormats()
{
	uint32_t count_formats = 0;
	VkResult res = vkGetPhysicalDeviceSurfaceFormatsKHR(_phys_devices[0], _surface,
																											&count_formats, NULL);
	assert(res == VK_SUCCESS);
	
	VkSurfaceFormatKHR *supported = static_cast<VkSurfaceFormatKHR*>(
													 malloc(sizeof(VkSurfaceFormatKHR) * count_formats));
	res = vkGetPhysicalDeviceSurfaceFormatsKHR(_phys_devices[0], _surface,
																						 &count_formats, NULL);
	assert(res == VK_SUCCESS);

	_format = VK_FORMAT_B8G8R8A8_UNORM;
	_color_space = supported[0].colorSpace;

	return VK_SUCCESS;
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
	assert(res == VK_SUCCESS);

	res = createQueues();
	assert(res == VK_SUCCESS);
		
	res = getSupportedFormats();
	assert(res == VK_SUCCESS);

	// Getting surface capabilities
  VkSurfaceCapabilitiesKHR capabilities;
	res = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(_phys_devices[0], _surface, &capabilities);
	assert(res == VK_SUCCESS);
	
	// Getting present modes
	uint32_t present_count = 0;
	res = vkGetPhysicalDeviceSurfacePresentModesKHR(_phys_devices[0], _surface,
																									&present_count, NULL);
	assert(res == VK_SUCCESS);
	VkPresentModeKHR *modes = static_cast<VkPresentModeKHR*>(
												    malloc(sizeof(VkPresentModeKHR) * present_count));
	res = vkGetPhysicalDeviceSurfacePresentModesKHR(_phys_devices[0], _surface,
																									&present_count, modes);
	assert(res == VK_SUCCESS);


	VkSwapchainCreateInfoKHR swapchain_info = { };
	swapchain_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapchain_info.pNext = NULL;
	swapchain_info.flags = 0;
	swapchain_info.surface = _surface;
	swapchain_info.minImageCount = capabilities.minImageCount;
	swapchain_info.imageFormat = _format;
	swapchain_info.imageColorSpace = _color_space;

	if ( capabilities.currentExtent.width == UINT32_MAX || capabilities.currentExtent.height == UINT32_MAX )
		swapchain_info.imageExtent = capabilities.maxImageExtent;
	else {
		swapchain_info.imageExtent.width = _width;
		swapchain_info.imageExtent.height = _height;
	}
	std::cout << "Setting extents to " << swapchain_info.imageExtent.width 
						<< "x" << swapchain_info.imageExtent.height << std::endl;
	
	swapchain_info.imageArrayLayers = 1;
	swapchain_info.imageUsage = 0;
	swapchain_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	swapchain_info.queueFamilyIndexCount = 0;
	swapchain_info.pQueueFamilyIndices = NULL;
	swapchain_info.preTransform = capabilities.currentTransform;

	VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;
	for (uint32_t i = 0; i < present_count; i++) {
		if (modes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
			presentMode = VK_PRESENT_MODE_MAILBOX_KHR;
			break;
		}
		else if (modes[i] == VK_PRESENT_MODE_IMMEDIATE_KHR)
			presentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
	}
	swapchain_info.presentMode = presentMode;
	
	
	res = vkCreateSwapchainKHR(_device, &swapchain_info, NULL, &_swapchain);
	assert(res == VK_SUCCESS);

	uint32_t image_count = 0;
	res = vkGetSwapchainImagesKHR(_device, _swapchain, &image_count, NULL);
	assert(res == VK_SUCCESS && image_count > 0);

	_images = std::vector<VkImage>(image_count);
	_buffers = std::vector<SwapchainBuffer>(image_count);

	res = vkGetSwapchainImagesKHR(_device, _swapchain, &image_count, _images.data());
	assert(res == VK_SUCCESS);

	VkImage *swapchains_images = static_cast<VkImage*>(malloc(sizeof(VkImage) * image_count));
	res = vkGetSwapchainImagesKHR(_device, _swapchain, &image_count, swapchains_images);
	
	for (uint32_t i = 0; i < image_count ; i++)
		_buffers[i].image = swapchains_images[i];
	free(swapchains_images);


	for (uint32_t i = 0; i < image_count ; i++)
	{
		VkImageViewCreateInfo color_image_view = {};
		color_image_view.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		color_image_view.pNext = NULL;
		color_image_view.flags = 0;
		color_image_view.image = _buffers[i].image;
		color_image_view.viewType = VK_IMAGE_VIEW_TYPE_2D;
		color_image_view.format = _format; 
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
		assert(res == VK_SUCCESS);
	}

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
	assert(res == VK_SUCCESS);

	vkGetImageMemoryRequirements(_device, _depth.image, &mem_reqs);

	alloc_info.allocationSize = mem_reqs.size;
	res = vkAllocateMemory(_device, &alloc_info, NULL, &_depth.mem);
	assert(res == VK_SUCCESS);

	res = vkBindImageMemory(_device, _depth.image, _depth.mem, 0);
	assert(res == VK_SUCCESS);

	view_info.image = _depth.image;
	res = vkCreateImageView(_device, &view_info, NULL, &_depth.view);
	assert(res == VK_SUCCESS);

	return VK_SUCCESS;
}

VkResult Engine::createUniformBuffer()
{
	_projection_matrix = glm::perspective(glm::radians(45.0f), 1.0f, 0.1f, 100.0f);
	_view_matrix = glm::lookAt(_camera, _origin, _up);
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
	assert(res == VK_SUCCESS);
	
	VkMemoryRequirements mem_reqs;
	vkGetBufferMemoryRequirements(_device, _uniform_data.buff, &mem_reqs);

	VkMemoryAllocateInfo alloc_info = {};
	alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	alloc_info.pNext = NULL;
	alloc_info.memoryTypeIndex = 0;
	alloc_info.allocationSize = mem_reqs.size;
	
	res = vkAllocateMemory(_device, &alloc_info, NULL, &(_uniform_data.mem));
	assert(res == VK_SUCCESS);

	uint8_t *pData = NULL;
	res = vkMapMemory(_device, _uniform_data.mem, 0, mem_reqs.size, 0, (void**)&pData);
	assert(res == VK_SUCCESS);

	memcpy(pData, &_MVP, sizeof(_MVP));
	vkUnmapMemory(_device, _uniform_data.mem);
	res = vkBindBufferMemory(_device, _uniform_data.buff, _uniform_data.mem, 0);
	assert(res == VK_SUCCESS);

	_uniform_data.buffer_info.buffer = _uniform_data.buff;
	_uniform_data.buffer_info.offset = 0;
  _uniform_data.buffer_info.range = sizeof(_MVP);
  
	return VK_SUCCESS;	
}

#define NUM_DESCRIPTORS 1

VkResult Engine::createPipeline()
{
	// This command buffer will be used on the vertex stage
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

	_desc_layout.resize(1);
	VkResult res = vkCreateDescriptorSetLayout(_device, &descriptor_layout, NULL, _desc_layout.data());
	assert(res == VK_SUCCESS);
	
	VkPipelineLayoutCreateInfo pipeline_layout = {};
	pipeline_layout.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipeline_layout.pNext = NULL;
	pipeline_layout.flags = 0;
	pipeline_layout.setLayoutCount = NUM_DESCRIPTORS;	
	pipeline_layout.pSetLayouts = _desc_layout.data();

	res = vkCreatePipelineLayout(_device, &pipeline_layout, NULL, &_pipeline_layout);
	assert(res == VK_SUCCESS);

	return VK_SUCCESS;
	
}

#define NUM_DESCRIPTOR_SETS 1

VkResult Engine::initDescriptors()
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
	alloc_info[0].descriptorSetCount = NUM_DESCRIPTOR_SETS;
	alloc_info[0].pSetLayouts = _desc_layout.data();
	
	_desc_set.resize(NUM_DESCRIPTOR_SETS);
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

void Engine::run()
{
}

void Engine::cleanup()
{
}
