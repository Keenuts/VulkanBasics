#pragma once

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

#include <xcb/xcb.h>
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>

#include <xcb/xcb.h>

typedef struct XcbWindows
{
	xcb_connection_t *_connection;
	xcb_window_t _window;
	xcb_screen_t *_screen;
	xcb_atom_t _wm_protocols;
	xcb_atom_t _wm_delete_win;

  XcbWindows(uint32_t width, uint32_t height);
	~XcbWindows();

	VkResult initConnection();
	void createWindow(uint32_t width, uint32_t height);

} XcbWindow;
