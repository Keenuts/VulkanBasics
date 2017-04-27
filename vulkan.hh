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

#include "window.hxx"

#include <xcb/xcb.h>
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>

struct vulkan_info {
	uint32_t width;
	uint32_t height;
	
	VkInstance instance;
	VkPhysicalDevice physical_device;
	VkDevice device;
	
	VkQueue graphic_queue;
	VkQueue present_queue;

	VkCommandPool cmd_pool;
	VkCommandBuffer cmd_buffer;

	VkFormat image_format;
	VkColorSpaceKHR color_space;

	VkSwapchainKHR swapchain;
	VkImage *swapchain_images;

	Buffer depth_buffer;
	Buffer uniform_buffer;
	Buffer *swapchain_buffers;
	uint32_t current_buffer;

	VkDescriptorSetLayout *descriptor_layouts;
	VkDescriptorSet *descriptor_sets;
	VkDescriptorPool descriptor_pool;
	
	VkPipelineLayout pipeline_layout;
	VkRenderPass render_pass;
	VkPipelineSHaderStageCreateInfo *_shader_stages;
	VkFramebuffer *framebuffers;

	uint32_t vertex_count;
	Buffer vertex_buffer;
	VkVertexInputBindingDescription vertex_binding;
	VkVertexInputAttributeDescription *vertex_attribute;
	VkPipeline pipeline;
	VkViewport viewport;
	VkRect2D scissor;
}

struct scene_info {
	glm::mat4 clip;
	glm::mat4 model;
	glm::mat4 projection;

	glm::vec3 camera;
	glm::vec3 origin;
	glm::vec3 up;
}

struct object_info {
	glm::mat4 model;
	struct vertex *vertices;
}

struct vertex {
	float x, y, z, w;
	float r, g, b, a;
}
