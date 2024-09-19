/**
 * Game engine initialization and event loop.
 * Copyright (C) 2024  dbstream
 */
#define LGE_MODULE "LGEInit"

#include <LGE/Application.h>
#include <LGE/Init.h>
#include <LGE/Log.h>
#include <LGE/Vulkan.h>
#include <LGE/Window.h>

#include <VKFW/vkfw.h>
#include <string.h>

#include <exception>

namespace LGE {

bool bIsProduction = false;
int gExitCode = 0;

static void
run_event_loop (void);

static void
setup_opts_for_prod (void)
{
	// Do not disable logging until we are past initialization.
	bIsProduction = true;
}

int
LGEMain (Application &app, const char *const *argv)
{
	if (argv) {
		// skip argv[0]
		if (*argv)
			argv++;

		if (*argv) {
			if (!::strcmp (*argv, "prod"))
				setup_opts_for_prod ();
		}
	}

	gApplication = &app;

	Log ("Application is %s", app.GetUserFriendlyName ());

	if (!bIsProduction)
		::vkfwEnableDebugLogging (VKFW_LOG_ALL);

	if (::vkfwInit () != VK_SUCCESS) {
		Log ("Failed to initialize VKFW");
		gExitCode = 1;
		return gExitCode;
	}

	if (!InitializeVulkan ()) {
		Log ("Failed to initialize Vulkan");
		::vkfwTerminate ();
		gExitCode = 1;
		return gExitCode;
	}

	try {
		gWindow = new Window;
	} catch (const std::exception &e) {
		Log ("Failed to create game window: %s", e.what ());
		TerminateVulkan ();
		::vkfwTerminate ();
		gExitCode = 1;
		return gExitCode;
	}

	// If we are in production, disable logging now.
	Log ("Initialization was successful");
	if (bIsProduction)
		bLoggingEnabled = false;

	try {
		gApplication->CheckRequirements ();
		run_event_loop ();
	} catch (const std::exception &e) {
		bLoggingEnabled = true;
		Log ("Caught an exception during the event loop: %s", e.what ());
		gExitCode = 1;
	}

	::vkDeviceWaitIdle (gVkDevice);
	gApplication->Cleanup ();

	delete gWindow;
	gWindow = nullptr;
	gApplication = nullptr;

	TerminateVulkan ();
	::vkfwTerminate ();
	Log ("Exit code is %d", gExitCode);
	return gExitCode;
}

static void
run_event_loop (void)
{
	VKFWevent e;

	uint64_t prevFrameTime = vkfwGetTime ();

	auto get_event = [&](void) -> bool {
		VkResult result;

		/**
		 * If we have a VSync swapchain, acquire will block, thus
		 * avoiding CPU burn. Otherwise, we have to block ourselves.
		 */

		if (gWindow && gWindow->IsVsyncSwapchain ()) {
			if ((result = ::vkfwGetNextEvent (&e)) == VK_SUCCESS)
				return true;
			Log ("vkfwGetNextEvent returned %s", VulkanTypeToString (result));
			return false;
		}

		// 16666 means "wait 16.6 milliseconds", thus giving us 60fps.
		if ((result = ::vkfwWaitNextEventUntil (&e, prevFrameTime + 16666)) == VK_SUCCESS)
			return true;
		Log ("vkfwWaitNextEvent returned %s", VulkanTypeToString (result));
		return false;
	};

	while (gApplication->KeepRunning ()) {

		// 1. pump events
		do {
			if (!get_event ()) {
				gExitCode = 1;
				return;
			}

			// do not dispatch NONE and NULL events
			if (e.type != VKFW_EVENT_NONE && e.type != VKFW_EVENT_NULL) {
				gApplication->HandleEvent (e);

				// HandleEvent can change KeepRunning
				if (!gApplication->KeepRunning ())
					break;
			}
		} while (e.type != VKFW_EVENT_NONE);

		prevFrameTime = vkfwGetTime ();

		// 2. render
		gApplication->Render ();
	}
}

}
