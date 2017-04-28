#pragma once

#include <X11/Xutil.h>

#include <cassert>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <memory>
#include <unistd.h>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>
#include <vulkan/vk_sdk_platform.h>
#include <vulkan/vulkan.hpp>
#include <xcb/xcb.h>

struct window_t
{
	xcb_connection_t *connection;
	xcb_window_t window;
	xcb_screen_t *screen;
	xcb_atom_t wm_protocols;
	xcb_atom_t wm_delete_win;
};

bool create_window(window_t *window, uint32_t width, uint32_t height);
