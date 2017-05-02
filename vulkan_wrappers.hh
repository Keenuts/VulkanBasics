#pragma once

#include "vulkan.hh"

/* Command buffers */
VkCommandBuffer command_begin_disposable(vulkan_info_t *info);
void command_submit_disposable(vulkan_info_t *info, VkCommandBuffer cmd);

/* Images */
void image_layout_transition(vulkan_info_t *info, VkImage image, VkFormat format,
											 VkImageLayout old_layout, VkImageLayout new_layout);
void image_create(vulkan_info_t *info, uint32_t w, uint32_t h,
									VkImage *img, VkDeviceMemory *mem, VkDeviceSize *size,
									VkFormat format, VkImageUsageFlags usage);
void image_copy(vulkan_info_t *info, VkImage src, VkImage dst, uint32_t width,
								uint32_t height);
void image_view_create(vulkan_info_t *info, VkImage image, VkFormat format, VkImageView *view);
