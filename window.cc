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

#include "window.hh"

static bool initialize_connection(window_t *window)
{
	int scr = 0;
	window->connection = xcb_connect(NULL, &scr);
	if (!window->connection || xcb_connection_has_error(window->connection)) {
		fprintf(stderr, "[ERROR] %s\n", "Unable to connect to the X server.");
		return false;
	}

	const xcb_setup_t *setup = xcb_get_setup(window->connection);
	xcb_screen_iterator_t iter = xcb_setup_roots_iterator(setup);
	while (scr-- > 0)
		xcb_screen_next(&iter);
	window->screen = iter.data;

	return true;
}

bool initialize_window(window_t *window, uint32_t height, uint32_t width)
{
	window->window = xcb_generate_id(window->connection);
	uint32_t value_mask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
	uint32_t value_list[32] = { window->screen->black_pixel, 0 };

	printf("[INFO] Creating a window %ux%u\n", width, height);

	xcb_create_window(window->connection,
										XCB_COPY_FROM_PARENT,
										window->window,
										window->screen->root,
										0,
										0,
										width,
										height,
										0,
										XCB_WINDOW_CLASS_INPUT_OUTPUT,
										window->screen->root_visual,
										value_mask,
										value_list);

	xcb_intern_atom_cookie_t wm_delete_cookie = xcb_intern_atom(
    window->connection, 0, strlen("WM_DELETE_WINDOW"), "WM_DELETE_WINDOW");
	xcb_intern_atom_cookie_t wm_protocols_cookie =
		xcb_intern_atom(window->connection, 0, strlen("WM_PROTOCOLS"), "WM_PROTOCOLS");
	xcb_intern_atom_reply_t *wm_delete_reply =
		xcb_intern_atom_reply(window->connection, wm_delete_cookie, NULL);
	xcb_intern_atom_reply_t *wm_protocols_reply =
		xcb_intern_atom_reply(window->connection, wm_protocols_cookie, NULL);
	window->wm_delete_win = wm_delete_reply->atom;
	window->wm_protocols = wm_protocols_reply->atom;
	
   xcb_map_window(window->connection, window->window);
	 xcb_flush(window->connection);

   const uint32_t coords[] = {0, 0};
   xcb_configure_window(window->connection, window->window,
                          XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y, coords);
	return true;
}

bool create_window(window_t *window, uint32_t width, uint32_t height) {
	if (!initialize_connection(window))
		return false;
	return initialize_window(window, width, height);
}

