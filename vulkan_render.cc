#define VULKAN_HPP_NO_EXCEPTIONS

#include <cstdio>
#include <cstdlib>
#include <memory>
#include <unistd.h>
#include <vulkan/vulkan.hpp>

#include "vulkan.hh"
#include "vulkan_render.hh"
#include "vulkan_wrappers.hh"
#include "vulkan_exception.hh"
#include "types.hh"

static void render_begin_command(VkCommandBuffer *command) {
	VkCommandBufferBeginInfo cmd_info = {};
	cmd_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	cmd_info.pNext = NULL;
	cmd_info.flags = 0;
	cmd_info.pInheritanceInfo = NULL;

	VkResult res = vkBeginCommandBuffer(*command, &cmd_info);
	assert(res == VK_SUCCESS);
}

static void render_end_command(VkCommandBuffer *command) {
	vkEndCommandBuffer(*command);
}

//TODO: Delete
static void render_create_semaphore(vulkan_info_t *info, VkSemaphore *semaphore) {
	VkSemaphoreCreateInfo create_info;
	create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	create_info.pNext = NULL;
	create_info.flags = 0;
	//Seriously ?
	VkResult res = vkCreateSemaphore(info->device, &create_info, NULL, semaphore);
	assert(res == VK_SUCCESS);
}

void render_init_fences(vulkan_info_t *info) {
	uint32_t count = info->swapchain_images_count;

	info->semaphore_acquired = new VkSemaphore[count];
	info->semaphore_drawing = new VkSemaphore[count];
	info->semaphore_ownership = new VkSemaphore[count];
	info->fences = new VkFence[count];

	for(uint32_t i = 0; i < info->swapchain_images_count; i++) {
		VkFenceCreateInfo fence_info;
		fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fence_info.pNext = NULL;
		fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		VkResult res = vkCreateFence(info->device, &fence_info, NULL, &info->fences[i]);
		assert(res == VK_SUCCESS);
		res = vkCreateFence(info->device, &fence_info, NULL, &info->swapchain_buffers[i].fence);
		assert(res == VK_SUCCESS);

		render_create_semaphore(info, &info->semaphore_acquired[i]);
		render_create_semaphore(info, &info->semaphore_drawing[i]);
		render_create_semaphore(info, &info->semaphore_ownership[i]);
	}
}

void render_create_cmd(vulkan_info_t *info, vulkan_frame_info_t *frame, int i) {

	info->swapchain_buffers[i].command = create_command_buffer(info);

	VkCommandBuffer *command = &info->swapchain_buffers[i].command;
	render_begin_command(command);

	VkClearValue clear_values[2];
	clear_values[0].color = {
		frame->clear_color.x,
		frame->clear_color.y,
		frame->clear_color.z,
		1.0f
	};

	clear_values[1].depthStencil.depth = 1.0f;
	clear_values[1].depthStencil.stencil = 0;

	VkRenderPassBeginInfo pass_begin_info = { };
	pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	pass_begin_info.pNext = NULL;
	pass_begin_info.renderPass = info->render_pass;
	pass_begin_info.framebuffer = info->framebuffers[i];
	pass_begin_info.renderArea.offset.x = 0;
	pass_begin_info.renderArea.offset.y = 0;
	pass_begin_info.renderArea.extent.width = info->width;
	pass_begin_info.renderArea.extent.height = info->height;
	pass_begin_info.clearValueCount = 2;
	pass_begin_info.pClearValues = clear_values;

	vkCmdBeginRenderPass(*command, &pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
	vkCmdBindPipeline(*command, VK_PIPELINE_BIND_POINT_GRAPHICS, info->pipeline);
	vkCmdBindDescriptorSets(*command, VK_PIPELINE_BIND_POINT_GRAPHICS,
													info->pipeline_layout, 0, NUM_DESCRIPTORS,
													info->descriptor_sets, 0, NULL);

	const VkDeviceSize offsets[1] = { 0 };
	vkCmdBindVertexBuffers(*command, 0, 1, &frame->vertex_buffer.buffer, offsets);
	vkCmdSetViewport(*command, 0, 1, &info->viewport);
	vkCmdSetScissor(*command, 0, 1, &info->scissor);
	vkCmdDraw(*command, frame->vertex_count, 1, 0, 0);
	vkCmdEndRenderPass(*command);

	render_end_command(command);
}

void render_create_transition_commands(vulkan_info_t *info, uint32_t i) {
	VkResult res;

	VkCommandBufferBeginInfo command_info = { };
	command_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	command_info.pNext = NULL;
	command_info.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
	command_info.pInheritanceInfo = NULL;

	info->swapchain_buffers[i].present_transition_cmd = create_command_buffer(info);
	res = vkBeginCommandBuffer(info->swapchain_buffers[i].present_transition_cmd, &command_info);
	assert(res == VK_SUCCESS);

	VkImageMemoryBarrier barrier = { };
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.pNext = NULL;
	barrier.srcAccessMask = 0;
	barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	barrier.oldLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	barrier.srcQueueFamilyIndex = info->graphic_queue_family_index;
	barrier.dstQueueFamilyIndex = info->present_queue_family_index;
	barrier.image = info->swapchain_buffers[i].image;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;

	vkCmdPipelineBarrier(info->swapchain_buffers[i].present_transition_cmd,
											 VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
											 VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 0,
											 NULL, 0, NULL, 1, &barrier);
	res = vkEndCommandBuffer(info->swapchain_buffers[i].present_transition_cmd);
	assert(res == VK_SUCCESS);
}

void render_get_frame(vulkan_info_t *info) {
	vkWaitForFences(info->device, 1, &info->fences[info->frame_index], VK_TRUE, 1000000);
	vkResetFences(info->device, 1, &info->fences[info->frame_index]);

	VkResult res = vkAcquireNextImageKHR(info->device, info->swapchain, UINT64_MAX,
																			 info->semaphore_acquired[info->frame_index],
																			 info->fences[info->frame_index],
																			 &info->current_buffer);
	assert(res == VK_SUCCESS);
}

void render_submit(vulkan_info_t *info, vulkan_frame_info_t *frame) {
	VkResult res = VK_SUCCESS;

	VkPipelineStageFlags pipe_stage_flags = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

	VkSubmitInfo submit_info = {};
	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit_info.pNext = NULL;
	submit_info.waitSemaphoreCount = 1;
	submit_info.pWaitSemaphores = &info->semaphore_acquired[info->frame_index];
	submit_info.pWaitDstStageMask = &pipe_stage_flags;
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers = &info->swapchain_buffers[info->current_buffer].command;
	submit_info.signalSemaphoreCount = 1;
	submit_info.pSignalSemaphores = &info->semaphore_drawing[info->frame_index];

	vkWaitForFences(info->device, 1, &info->swapchain_buffers[info->current_buffer].fence, VK_TRUE, 1000000);
	vkResetFences(info->device, 1, &info->swapchain_buffers[info->current_buffer].fence);

	res = vkQueueSubmit(info->graphic_queue, 1, &submit_info, info->swapchain_buffers[info->current_buffer].fence);
	assert(res == VK_SUCCESS);

	VkPresentInfoKHR present;
	present.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	present.pNext = NULL;
	present.swapchainCount = 1;
	present.pSwapchains = &info->swapchain;
	present.pImageIndices = &info->current_buffer;
	present.waitSemaphoreCount = 1;
	present.pWaitSemaphores = &info->semaphore_drawing[info->frame_index];
	present.pResults = NULL;
	res = vkQueuePresentKHR(info->present_queue, &present);
	assert(res == VK_SUCCESS);

	info->frame_index = (info->frame_index + 1) % info->swapchain_images_count;
}

void render_destroy(vulkan_info_t *info, vulkan_frame_info_t *frame) {
	vkWaitForFences(info->device, info->swapchain_images_count, info->fences, VK_TRUE, UINT64_MAX);
	for (uint32_t i = 0; i < info->swapchain_images_count; i++) {
		vkDestroyFence(info->device, info->fences[i], NULL);
		vkDestroyFence(info->device, info->swapchain_buffers[i].fence, NULL);
		vkFreeCommandBuffers(info->device, info->cmd_pool, 1, &info->swapchain_buffers[i].command);

		vkDestroySemaphore(info->device, info->semaphore_acquired[i], NULL);
		vkDestroySemaphore(info->device, info->semaphore_drawing[i], NULL);
		vkDestroySemaphore(info->device, info->semaphore_ownership[i], NULL);
	}
}
