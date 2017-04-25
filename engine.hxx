#pragma once

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

#include "window.hxx"

#include <xcb/xcb.h>
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>


static const char *vulkanErr(VkResult res)
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

typedef struct SwapchainBuffer {
	VkImage image;
	VkImageView view;
	VkFramebuffer frameBuffer;
} SwapchainBuffer;

typedef struct DepthMap {
	VkFormat format;
	VkImage image;
	VkDeviceMemory mem;
	VkImageView view;
} DepthMap;

typedef struct {
	VkBuffer buff;
	VkDeviceMemory mem;
	VkDescriptorBufferInfo buffer_info;
} uniformBuffer;

class Engine
{
	private:
		uint32_t _width, _height;
		XcbWindows _window;
		VkInstance _instance;

		std::vector<const char *> _instance_extension_names;
		std::vector<const char *> _instance_validation_layers;
		std::vector<const char *> _device_extension_names;

		VkPhysicalDevice *_phys_devices;
		VkPhysicalDeviceMemoryProperties _memory_properties;
		VkDevice _device;

		uint32_t _queue_family_count;
		uint32_t _present_queue_family_index;
		uint32_t _graphic_queue_family_index;

		VkQueueFamilyProperties *_queue_props;
		VkQueue _graphic_queue;
		VkQueue _present_queue;

		VkCommandPool _cmd_pool;
		VkCommandBuffer _cmd_buffer;

		VkSurfaceKHR _surface;
		VkFormat _image_format;
		VkColorSpaceKHR _color_space;

		uint32_t _swapchain_image_count;
		VkSwapchainKHR _swapchain;

		DepthMap _depth;
		uniformBuffer _uniform_data;
		VkPipelineLayout _pipeline_layout;

		std::vector<SwapchainBuffer> _buffers;
		std::vector<VkDescriptorSetLayout> _desc_layout;
		std::vector<VkDescriptorSet> _desc_set;

		VkDescriptorPool _desc_pool;

		uint32_t _current_buffer;
		VkRenderPass _render_pass;
		VkPipelineShaderStageCreateInfo _shader_stages[2];
		VkFramebuffer* _framebuffers;
		uniformBuffer _vertexBuffer;
		VkVertexInputBindingDescription _vtxBinding;
		VkVertexInputAttributeDescription _vtxAttribute[2];
		VkPipeline _pipeline;

		VkViewport _viewport;
		VkRect2D _scissor;


		//Matrices
		glm::mat4 _clip_matrix;
		glm::mat4 _model_matrix;
		glm::mat4 _view_matrix;
		glm::mat4 _projection_matrix;
		glm::mat4 _MVP;

		glm::vec3 _camera;
		glm::vec3 _origin;
		glm::vec3 _up;


		std::vector<VkImage> _images;

		VkResult initvk();
		VkResult initConnection();
		void createWindow();

		VkResult getDevices();
		VkResult getLogicalDevice();

		uint32_t getQueueFamilyIndex(VkQueueFlagBits bits);
		VkResult findMemoryTypeIndex(uint32_t type, VkFlags flags, uint32_t *res);
		VkResult getSupportedFormats();

		VkResult createQueues();
		VkResult createCommandPool();
		VkResult createCommandBuffer();
		VkResult createSwapchain();
		VkResult createDepthBuffer();
		VkResult createUniformBuffer();
		VkResult createPipelineLayout();
		VkResult createDescriptors();
		VkResult createRenderPass();
		VkResult createShaders();
		VkResult createFramebuffers();

		VkResult createVertexBuffer(uint32_t size, uniformBuffer *buffer);
		VkResult BeginCommandBuffer();
		VkResult createTriangle();
		VkResult createPipeline();


	public:
		Engine();
		~Engine();

		VkResult init();
		VkResult run();
		void cleanup();
};
