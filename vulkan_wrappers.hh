#pragma once

#include "vulkan.hh"

VkCommandBuffer begin_disposable_command(vulkan_info_t *info);
void submit_disposable_command(vulkan_info_t *info, VkCommandBuffer cmd);
void layout_transition(vulkan_info_t *info, VkImage image, VkFormat format,
											 VkImageLayout old_layout, VkImageLayout new_layout);
void create_image(vulkan_info_t *info, uint32_t w, uint32_t h,
									VkImage *img, VkDeviceMemory *mem, VkDeviceSize *size,
									VkFormat format, VkImageUsageFlags usage);
void copy_image(vulkan_info_t *info, VkImage src, VkImage dst, uint32_t width,
								uint32_t height);
