#include "vulkan_wrappers.hh"
#include "vulkan_exception.hh"

bool find_memory_type_index(vulkan_info_t *info, uint32_t type, VkFlags flags, uint32_t *res) {
	for (uint32_t i = 0; i < info->memory_properties.memoryTypeCount; i++) {
		if ((type & 1) == 1) {
			if ((info->memory_properties.memoryTypes[i].propertyFlags & flags) == flags) {
				*res = i;
				return true;
			}
		}
		type >>= 1;
	}
	assert(0 && "Unable to find the proper memory type");
}

uint32_t get_queue_family_index(VkQueueFlagBits bits, uint32_t count, VkQueueFamilyProperties *props) {
	for (uint32_t i = 0; i < count ; i++) {
		if (props[i].queueFlags & bits)
			return i;
	}
	assert(0 && "Unable to find the proper queue family.");
	return (uint32_t)-1;
}

VkCommandBuffer command_begin_disposable(vulkan_info_t *info) {
	VkCommandBufferAllocateInfo alloc_info = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.pNext = NULL,
		.commandPool = info->cmd_pool,
		.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		.commandBufferCount = 1,
	};

	VkResult res = VK_SUCCESS;
	VkCommandBuffer command;

	res = vkAllocateCommandBuffers(info->device, &alloc_info, &command);
	if (res != VK_SUCCESS)
		throw VkException(res);
	
	VkCommandBufferBeginInfo begin_info = {};
	begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	begin_info.pNext = NULL;
	begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	begin_info.pInheritanceInfo = NULL;

	res = vkBeginCommandBuffer(command, &begin_info);
	assert(res == VK_SUCCESS);
	return command;
}

void command_submit_disposable(vulkan_info_t *info, VkCommandBuffer command) {
	VkResult res = VK_SUCCESS;
	res = vkEndCommandBuffer(command);
	assert(res == VK_SUCCESS);

	VkSubmitInfo submission = { };
	submission.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submission.commandBufferCount = 1;
	submission.pCommandBuffers = &command;

	res = vkQueueSubmit(info->graphic_queue, 1, &submission, VK_NULL_HANDLE);
	assert(res == VK_SUCCESS);

	res = vkQueueWaitIdle(info->graphic_queue);
	assert(res == VK_SUCCESS);

	vkFreeCommandBuffers(info->device, info->cmd_pool, 1, &command);
}

void image_create(vulkan_info_t *info, uint32_t w, uint32_t h,
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
	assert(res == VK_SUCCESS && "Unable to create Image");

	VkMemoryRequirements mem_reqs;
	vkGetImageMemoryRequirements(info->device, *img, &mem_reqs);

	VkMemoryAllocateInfo alloc_info = {};
	alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	alloc_info.pNext = NULL;
	alloc_info.memoryTypeIndex = 0;
	alloc_info.allocationSize = mem_reqs.size;
	*size = mem_reqs.size;
	bool success = find_memory_type_index(info, mem_reqs.memoryTypeBits,
										VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
										&alloc_info.memoryTypeIndex );

	assert(success);
	res = vkAllocateMemory(info->device, &alloc_info, NULL, mem);
	if (res != VK_SUCCESS)
		throw VkException(res);

	res = vkBindImageMemory(info->device, *img, *mem, 0);
	assert(res == VK_SUCCESS && "Cannot bind memory");
}

static VkAccessFlags get_access_flag_from_old_layout(VkImageLayout layout) {
	switch (layout) {
		case VK_IMAGE_LAYOUT_PREINITIALIZED:
			return VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
		case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
			return VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
			return VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
			return VK_ACCESS_TRANSFER_READ_BIT;
		case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
			return VK_ACCESS_SHADER_READ_BIT;
		case VK_IMAGE_LAYOUT_UNDEFINED:
			return 0;
		default:
			assert(0 && "Unhandled old layout transition");
			break;
	}
	assert(0 && "Unhandled layout transition");
	return VK_ACCESS_MEMORY_READ_BIT;
}

static VkAccessFlags get_access_flag_from_new_layout(VkImageLayout layout) {
	switch(layout) {
		case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
			return VK_ACCESS_TRANSFER_WRITE_BIT;
		case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
			return VK_ACCESS_TRANSFER_READ_BIT;
		case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
			return VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
			return VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
			return VK_ACCESS_SHADER_READ_BIT;
		default:
			assert(0 && "Unhandled new layout transition");
			break;
	}
	assert(0 && "Unhandled layout transition");
	return VK_ACCESS_MEMORY_READ_BIT;
}

void image_layout_transition(vulkan_info_t *info, VkImage image, VkImageLayout old_layout, VkImageLayout new_layout) {
	VkCommandBuffer command = command_begin_disposable(info);

	VkImageMemoryBarrier barrier = { };
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.pNext = NULL;
	barrier.srcAccessMask = get_access_flag_from_old_layout(old_layout);
	barrier.dstAccessMask = get_access_flag_from_new_layout(new_layout);
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

	command_submit_disposable(info, command);
}

void image_copy(vulkan_info_t *info, VkImage src, VkImage dst, uint32_t width,
								uint32_t height) {
	VkCommandBuffer command = command_begin_disposable(info);

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
	command_submit_disposable(info, command);
}

void image_view_create(vulkan_info_t *info, VkImage image, VkFormat format, VkImageView *view) {
	VkImageViewCreateInfo create_info = { };
	create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	create_info.pNext = NULL;
	create_info.flags = 0;
	create_info.image = image;
	create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
	create_info.format = format; 
	create_info.components.r = VK_COMPONENT_SWIZZLE_R;
	create_info.components.g = VK_COMPONENT_SWIZZLE_G;
	create_info.components.b = VK_COMPONENT_SWIZZLE_B;
	create_info.components.a = VK_COMPONENT_SWIZZLE_A;
	create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	create_info.subresourceRange.baseMipLevel = 0;
	create_info.subresourceRange.levelCount = 1;
	create_info.subresourceRange.baseArrayLayer = 0;
	create_info.subresourceRange.layerCount = 1;

	VkResult res = vkCreateImageView(info->device, &create_info, NULL, view);
	if (res != VK_SUCCESS)
		throw VkException(res);
}
