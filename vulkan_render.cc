#define VULKAN_HPP_NO_EXCEPTIONS

#include <cstdio>
#include <cstdlib>
#include <memory>
#include <unistd.h>
#include <vulkan/vulkan.hpp>

#include "vulkan.hh"
#include "vulkan_render.hh"
#include "types.hh"

VkResult render_submit(vulkan_info_t *info, vulkan_frame_info_t *frame) {

	VkFenceCreateInfo fence_info;
	VkFence draw_fence;
	fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fence_info.pNext = NULL;
	fence_info.flags = 0;
	vkCreateFence(info->device, &fence_info, NULL, &draw_fence);

	VkPipelineStageFlags pipe_stage_flags = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	VkSubmitInfo submit_info[1] = {};
	submit_info[0].sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit_info[0].pNext = NULL;
	submit_info[0].waitSemaphoreCount = 1;
	submit_info[0].pWaitSemaphores = &frame->semaphore;
	submit_info[0].pWaitDstStageMask = &pipe_stage_flags;
	submit_info[0].commandBufferCount = 1;
	submit_info[0].pCommandBuffers = &frame->command;
	submit_info[0].signalSemaphoreCount = 0;
	submit_info[0].pSignalSemaphores = NULL;

	VkResult res = vkQueueSubmit(info->graphic_queue, 1, submit_info, draw_fence);
	CHECK_VK(res);

	VkPresentInfoKHR present;
	present.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	present.pNext = NULL;
	present.swapchainCount = 1;
	present.pSwapchains = &info->swapchain;
	present.pImageIndices = &info->current_buffer;
	present.pWaitSemaphores = NULL;
	present.waitSemaphoreCount = 0;
	present.pResults = NULL;

	do {
#define FENCE_TIMEOUT 100000000
			res = vkWaitForFences(info->device, 1, &draw_fence, VK_TRUE, FENCE_TIMEOUT);
	} while (res == VK_TIMEOUT);
	CHECK_VK(res);

	res = vkQueuePresentKHR(info->present_queue, &present);
	CHECK_VK(res);

	vkDestroyFence(info->device, draw_fence, NULL);
	return VK_SUCCESS;
}

static VkResult render_begin_command(VkCommandBuffer *command) {
	VkCommandBufferBeginInfo cmd_info = {};
	cmd_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	cmd_info.pNext = NULL;
	cmd_info.flags = 0;
	cmd_info.pInheritanceInfo = NULL;
	return vkBeginCommandBuffer(*command, &cmd_info);
}

static VkResult render_end_command(VkCommandBuffer *command) {
	return vkEndCommandBuffer(*command);
}

static VkResult render_create_semaphore(vulkan_info_t *info, VkSemaphore *semaphore) {
	VkSemaphoreCreateInfo create_info;
	create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	create_info.pNext = NULL;
	create_info.flags = 0;
	//Seriously ?
	return vkCreateSemaphore(info->device, &create_info, NULL, semaphore);
}

VkResult render_create_cmd(vulkan_info_t *info, vulkan_frame_info_t *frame) {
	render_begin_command(&frame->command);
	render_create_semaphore(info, &frame->semaphore);

	VkResult res = vkAcquireNextImageKHR(info->device, info->swapchain, UINT64_MAX,
																			 frame->semaphore, VK_NULL_HANDLE,
																			 &info->current_buffer);
	CHECK_VK(res);
	res = set_image_layout(&frame->command,
								 info->swapchain_buffers[info->current_buffer].image,
								 VK_IMAGE_ASPECT_COLOR_BIT,
								 VK_IMAGE_LAYOUT_UNDEFINED,
								 VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	//CHECK_VK(res);

	VkClearValue clear_values[2];
	clear_values[0].color = {
		frame->clear_color.x, frame->clear_color.y, frame->clear_color.z, 1.0
	};
	clear_values[1].depthStencil.depth = 1.0f;
	clear_values[1].depthStencil.stencil = 0;

	VkRenderPassBeginInfo pass_begin_info = {};
	pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	pass_begin_info.pNext = NULL;
	pass_begin_info.renderPass = info->render_pass;
	pass_begin_info.framebuffer = info->framebuffers[info->current_buffer];
	pass_begin_info.renderArea.offset.x = 0;
	pass_begin_info.renderArea.offset.y = 0;
	pass_begin_info.renderArea.extent.width = info->width;
	pass_begin_info.renderArea.extent.height = info->height;
	pass_begin_info.clearValueCount = 2;
	pass_begin_info.pClearValues = clear_values;

	vkCmdBeginRenderPass(frame->command, &pass_begin_info,
											 VK_SUBPASS_CONTENTS_INLINE);
	vkCmdBindPipeline(frame->command, VK_PIPELINE_BIND_POINT_GRAPHICS,
										info->pipeline);
	vkCmdBindDescriptorSets(frame->command, VK_PIPELINE_BIND_POINT_GRAPHICS,
													info->pipeline_layout, 0, NUM_DESCRIPTORS,
													info->descriptor_sets, 0, NULL);

	const VkDeviceSize offsets[1] = { 0 };
	vkCmdBindVertexBuffers(frame->command, 0, 1, &frame->vertex_buffer.buffer, offsets);
	
	vkCmdSetViewport(frame->command, 0, 1, &info->viewport);
	vkCmdSetScissor(frame->command, 0, 1, &info->scissor);

	vkCmdDraw(frame->command, frame->vertex_count, 1, 0, 0);
	vkCmdEndRenderPass(frame->command);
	return render_end_command(&frame->command);
}

void render_destroy(vulkan_info_t *info, vulkan_frame_info_t *frame) {
	vkDestroySemaphore(info->device, frame->semaphore, NULL);
}
