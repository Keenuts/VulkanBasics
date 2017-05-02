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

#include "stb_image.h"
#include "types.hh"
#include "window.hh"

#define CHECK_VK(res) 																			\
	if (res != VK_SUCCESS)   																	\
		return res;

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
	uint32_t shader_stages_count;
	VkFramebuffer *framebuffers;
	VkPipeline pipeline;
	VkViewport viewport;

	uint32_t vertex_count;
	data_buffer_t vertex_buffer;
	VkVertexInputBindingDescription vertex_binding;
	VkVertexInputAttributeDescription *vertex_attribute;
	VkRect2D scissor;
};

VkResult vulkan_initialize(vulkan_info_t *info);
VkResult vulkan_create_pipeline(vulkan_info_t *info);

VkResult vulkan_create_vertex_buffer(vulkan_info_t *i, uint32_t size, data_buffer_t *b);
VkResult vulkan_create_texture(vulkan_info_t *info, texture_t *tex);

VkResult vulkan_load_shaders(vulkan_info_t *info, uint32_t count,
														 const char **paths, VkShaderStageFlagBits *flags);

VkResult vulkan_update_texture(vulkan_info_t *info, texture_t *tex, stbi_uc* data);
VkResult vulkan_update_uniform_buffer(vulkan_info_t *info, scene_info_t *payload);
VkResult vulkan_update_vertex_buffer(vulkan_info_t *i, data_buffer_t *b,
																		 vertex_t *vtx, uint32_t count);

VkResult vulkan_begin_command_buffer(vulkan_info_t *info);
VkResult vulkan_render_frame(vulkan_info_t *info);

void vulkan_unload_shaders(vulkan_info_t *info, uint32_t count);
void vulkan_cleanup(vulkan_info_t *info);

//HELPERS
VkResult set_image_layout(VkCommandBuffer *cmd_buffer, VkImage image,
																 VkImageAspectFlags aspects,
																 VkImageLayout old_layout,
																 VkImageLayout new_layout);
bool find_memory_type_index(vulkan_info_t *info, uint32_t type,
																	 VkFlags flags, uint32_t *res);
