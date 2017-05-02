#include "vulkan_wrappers.hh"

bool find_memory_type_index(vulkan_info_t *info, uint32_t type,
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

VkCommandBuffer begin_disposable_command(vulkan_info_t *info) {
	VkCommandBufferAllocateInfo alloc_info = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.pNext = NULL,
		.commandPool = info->cmd_pool,
		.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		.commandBufferCount = 1,
	};

	VkResult res = VK_SUCCESS;
	(void)res;
	VkCommandBuffer command;

	res = vkAllocateCommandBuffers(info->device, &alloc_info, &command);
	//TODO: throw exception
	
	VkCommandBufferBeginInfo begin_info = {};
	begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	begin_info.pNext = NULL;
	begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	begin_info.pInheritanceInfo = NULL;

	res = vkBeginCommandBuffer(command, &begin_info);
	//TODO: throw exception
	return command;
}

void submit_disposable_command(vulkan_info_t *info, VkCommandBuffer command) {
	VkResult res = VK_SUCCESS;
	(void)res;

	res = vkEndCommandBuffer(command);
	//TODO: throw exception

	VkSubmitInfo submission = { };
	submission.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submission.commandBufferCount = 1;
	submission.pCommandBuffers = &command;

	res = vkQueueSubmit(info->graphic_queue, 1, &submission, VK_NULL_HANDLE);
	//TODO: throw exception

	res = vkQueueWaitIdle(info->graphic_queue);
	//TODO: throw exception

	vkFreeCommandBuffers(info->device, info->cmd_pool, 1, &command);
}

void layout_transition(vulkan_info_t *info, VkImage image, VkFormat format,
											 VkImageLayout old_layout, VkImageLayout new_layout) {
	VkCommandBuffer command = begin_disposable_command(info);

	VkImageMemoryBarrier barrier = { };
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.pNext = NULL;
	barrier.srcAccessMask = 0;
	barrier.dstAccessMask = 0;
	barrier.oldLayout = old_layout;
	barrier.newLayout = new_layout;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = image;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;

	vkCmdPipelineBarrier( command,
		VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
		VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
		0, 0, nullptr, 0, nullptr, 1, &barrier);

	submit_disposable_command(info, command);
}

void create_image(vulkan_info_t *info, uint32_t w, uint32_t h,
									VkImage *img, VkDeviceMemory *mem, VkDeviceSize *size,
									VkFormat format, VkImageUsageFlags usage) {
	VkResult res = VK_SUCCESS;
	(void)res;

	VkImageCreateInfo image_info = {};
	image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	image_info.pNext = NULL;
	image_info.flags = 0;
	image_info.imageType = VK_IMAGE_TYPE_2D;
	image_info.extent.width = w;
	image_info.extent.height = h;
	image_info.extent.depth = 1;
	image_info.mipLevels = 1;
	image_info.arrayLayers = 1;
	image_info.format = format;
	image_info.tiling = VK_IMAGE_TILING_LINEAR;
	image_info.initialLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;
	image_info.usage = usage;
	image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	image_info.samples = VK_SAMPLE_COUNT_1_BIT;

	res = vkCreateImage(info->device, &image_info, NULL, img);
	//TODO: throw exception

	VkMemoryRequirements mem_reqs;
	vkGetImageMemoryRequirements(info->device, *img, &mem_reqs);

	VkMemoryAllocateInfo alloc_info = {};
	alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	alloc_info.pNext = NULL;
	alloc_info.memoryTypeIndex = 0;
	alloc_info.allocationSize = mem_reqs.size;
	*size = mem_reqs.size;
	bool success = find_memory_type_index(info, mem_reqs.memoryTypeBits,
																				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
																				 | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
																				&alloc_info.memoryTypeIndex);
	(void)success;
	//TODO: throw exception

	res = vkAllocateMemory(info->device, &alloc_info, NULL, mem);
	//TODO: throw exception

	res = vkBindImageMemory(info->device, *img, *mem, 0);
	//TODO: throw exception
}

void copy_image(vulkan_info_t *info, VkImage src, VkImage dst, uint32_t width,
								uint32_t height) {
	VkCommandBuffer command = begin_disposable_command(info);

	VkImageSubresourceLayers subResource = { };
	subResource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	subResource.baseArrayLayer = 0;
	subResource.mipLevel = 0;
	subResource.layerCount = 1;

	VkImageCopy region = { };
	region.srcSubresource = subResource;
	region.dstSubresource = subResource;
	region.srcOffset = {0, 0, 0};
	region.dstOffset = {0, 0, 0};
	region.extent.width = width;
	region.extent.height = height;
	region.extent.depth = 1;

	vkCmdCopyImage(command, src, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
								 dst, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
	submit_disposable_command(info, command);
}
