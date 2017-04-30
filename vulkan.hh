#pragma once

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

#include <xcb/xcb.h>
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>

#include "types.hh"
#include "window.hh"

#define CHECK_VK(res) 																			\
	if (res != VK_SUCCESS)   																	\
		return res;

struct swapchain_buffer_t {
	VkImage image;
	VkImageView view;
	VkFramebuffer framebuffer;
};

struct data_buffer_t {
	VkDescriptorBufferInfo descriptor;
	VkDeviceMemory memory;
	VkBuffer buffer;
};

struct image_buffer_t {
	VkFormat format;
	VkImage image;
	VkDeviceMemory memory;
	VkImageView view;
};

struct queue_creation_info_t {
	uint32_t count;
	uint32_t present_family_index;
	uint32_t graphic_family_index;

	VkQueueFamilyProperties *family_props;
};

#define NUM_DESCRIPTORS (1)

struct vulkan_info_t {
	uint32_t width;
	uint32_t height;
	
	VkInstance instance;
	VkPhysicalDevice physical_device;
	VkPhysicalDeviceMemoryProperties memory_properties;
	VkPhysicalDeviceProperties device_properties;
	VkDevice device;

	window_t window;
	VkSurfaceKHR surface;
	
	VkQueue graphic_queue;
	VkQueue present_queue;

	VkCommandPool cmd_pool;
	VkCommandBuffer cmd_buffer;

	VkFormat image_format;
	VkColorSpaceKHR color_space;

	VkSwapchainKHR swapchain;
	VkImage *swapchain_images;
	uint32_t swapchain_images_count;

	image_buffer_t depth_buffer;
	data_buffer_t uniform_buffer;
	swapchain_buffer_t *swapchain_buffers;
	uint32_t current_buffer;

	VkDescriptorSetLayout *descriptor_layouts;
	VkDescriptorSet *descriptor_sets;
	VkDescriptorPool descriptor_pool;
	
	VkPipelineLayout pipeline_layout;
	VkRenderPass render_pass;
	VkPipelineShaderStageCreateInfo *shader_stages;
	VkFramebuffer *framebuffers;
	VkPipeline pipeline;
	VkViewport viewport;

	uint32_t vertex_count;
	data_buffer_t vertex_buffer;
	VkVertexInputBindingDescription vertex_binding;
	VkVertexInputAttributeDescription *vertex_attribute;
	VkRect2D scissor;
};

__attribute__((__used__))
static const char* vktostring(VkResult res)
{
	switch (res) {
#define CASE_STR(N) case N: return #N

		CASE_STR(VK_SUCCESS);
		CASE_STR(VK_NOT_READY);
		CASE_STR(VK_TIMEOUT);
		CASE_STR(VK_EVENT_SET);
		CASE_STR(VK_EVENT_RESET);
		CASE_STR(VK_INCOMPLETE);
		CASE_STR(VK_SUBOPTIMAL_KHR);
		CASE_STR(VK_ERROR_OUT_OF_HOST_MEMORY);
		CASE_STR(VK_ERROR_OUT_OF_DEVICE_MEMORY);
		CASE_STR(VK_ERROR_INITIALIZATION_FAILED);
		CASE_STR(VK_ERROR_MEMORY_MAP_FAILED);
		CASE_STR(VK_ERROR_DEVICE_LOST);
		CASE_STR(VK_ERROR_EXTENSION_NOT_PRESENT);
		CASE_STR(VK_ERROR_FEATURE_NOT_PRESENT);
		CASE_STR(VK_ERROR_LAYER_NOT_PRESENT);
		CASE_STR(VK_ERROR_INCOMPATIBLE_DRIVER);
		CASE_STR(VK_ERROR_TOO_MANY_OBJECTS);
		CASE_STR(VK_ERROR_FORMAT_NOT_SUPPORTED);
		CASE_STR(VK_ERROR_SURFACE_LOST_KHR);
		CASE_STR(VK_ERROR_OUT_OF_DATE_KHR);
		CASE_STR(VK_ERROR_INCOMPATIBLE_DISPLAY_KHR);
		CASE_STR(VK_ERROR_NATIVE_WINDOW_IN_USE_KHR);
		CASE_STR(VK_ERROR_VALIDATION_FAILED_EXT);
		default:
			return "VK_???";
	}
#undef CASE_STR
}

VkResult vulkan_initialize(vulkan_info_t *info);
VkResult vulkan_create_vertex_buffer(vulkan_info_t *info, uint32_t size,
																						data_buffer_t *buffer);

VkResult vulkan_begin_command_buffer(vulkan_info_t *info);
VkResult vulkan_update_vertex_buffer(vulkan_info_t *info, data_buffer_t *buffer,
																		 vertex_t *vertices, uint32_t count);

VkResult vulkan_render_frame(vulkan_info_t *info);
void vulkan_cleanup(vulkan_info_t *info);
