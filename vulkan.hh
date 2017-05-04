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
	uint32_t graphic_queue_family_index;
	uint32_t present_queue_family_index;

	VkCommandPool cmd_pool;
	VkCommandBuffer cmd_buffer;

	VkFormat image_format;
	VkColorSpaceKHR color_space;

	VkSwapchainKHR swapchain;
	VkImage *swapchain_images;
	VkSemaphore *semaphore_acquired;
	VkSemaphore *semaphore_drawing;
	VkSemaphore *semaphore_ownership;
	VkFence *fences;
	uint32_t swapchain_images_count;

	image_buffer_t depth_buffer;
	data_buffer_t uniform_buffer;
	swapchain_buffer_t *swapchain_buffers;
	uint32_t current_buffer;
	uint32_t frame_index;

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
	
	VkImageView texture_view;
	VkSampler texture_sampler;
};

void vulkan_initialize(vulkan_info_t *info);
void vulkan_create_rendering_pipeline(vulkan_info_t *info);

void vulkan_create_texture(vulkan_info_t *info, texture_t *tex);
void vulkan_update_texture(vulkan_info_t *info, texture_t *tex, stbi_uc* data);

void vulkan_load_shaders(vulkan_info_t *info, uint32_t count,
														 const char **paths, VkShaderStageFlagBits *flags);

void vulkan_create_vertex_buffer(vulkan_info_t *i, uint32_t size, data_buffer_t *b);
void vulkan_update_vertex_buffer(vulkan_info_t *i, data_buffer_t *b, vertex_t *vtx, uint32_t count);

void vulkan_update_uniform_buffer(vulkan_info_t *info, scene_info_t *payload);

void vulkan_begin_command_buffer(vulkan_info_t *info);
void vulkan_render_frame(vulkan_info_t *info);

void vulkan_unload_shaders(vulkan_info_t *info, uint32_t count);
void vulkan_unload_texture(vulkan_info_t *info, texture_t *texture);
void vulkan_cleanup(vulkan_info_t *info);
