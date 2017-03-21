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

#include <xcb/xcb.h>

XcbWindow::XcbWindows(uint32_t width, uint32_t height)
{
	initConnection();
	createWindow(width, height);
}

XcbWindow::~XcbWindows()
{
}
 
VkResult XcbWindow::initConnection()
{
	int scr = 0;
	_connection = xcb_connect(NULL, &scr);
	if (!_connection || xcb_connection_has_error(_connection)) {
		std::cout << "Unable to connect to the X server." << std::endl;
		return VK_INCOMPLETE;
	}

	const xcb_setup_t *setup = xcb_get_setup(_connection);
	xcb_screen_iterator_t iter = xcb_setup_roots_iterator(setup);
	while (scr-- > 0)
		xcb_screen_next(&iter);
	_screen = iter.data;

	return VK_SUCCESS;
}

void XcbWindow::createWindow(uint32_t height, uint32_t width)
{
	_window = xcb_generate_id(_connection);
	uint32_t value_mask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
	uint32_t value_list[32] = { _screen->black_pixel, 0 };

	std::cout << "Creating a window " << width << "x" << height << std::endl;

	xcb_create_window(_connection,
										XCB_COPY_FROM_PARENT,
										_window,
										_screen->root,
										0,
										0,
										width,
										height,
										0,
										XCB_WINDOW_CLASS_INPUT_OUTPUT,
										_screen->root_visual,
										value_mask,
										value_list);

	xcb_intern_atom_cookie_t wm_delete_cookie = xcb_intern_atom(
    _connection, 0, strlen("WM_DELETE_WINDOW"), "WM_DELETE_WINDOW");
	xcb_intern_atom_cookie_t wm_protocols_cookie =
		xcb_intern_atom(_connection, 0, strlen("WM_PROTOCOLS"), "WM_PROTOCOLS");
	xcb_intern_atom_reply_t *wm_delete_reply =
		xcb_intern_atom_reply(_connection, wm_delete_cookie, NULL);
	xcb_intern_atom_reply_t *wm_protocols_reply =
		xcb_intern_atom_reply(_connection, wm_protocols_cookie, NULL);
	_wm_delete_win = wm_delete_reply->atom;
	_wm_protocols = wm_protocols_reply->atom;
	
   xcb_map_window(_connection, _window);
	 xcb_flush(_connection);

   const uint32_t coords[] = {100, 100};
   xcb_configure_window(_connection, _window,

                          XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y, coords);
}
