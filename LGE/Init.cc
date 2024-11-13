/**
 * Game engine initialization and event loop.
 * Copyright (C) 2024  dbstream
 */
#define LGE_MODULE "LGEInit"

#include <LGE/Application.h>
#include <LGE/DebugUI.h>
#include <LGE/Init.h>
#include <LGE/Log.h>
#include <LGE/Vulkan.h>
#include <LGE/Window.h>

#include <VKFW/vkfw.h>
#include <string.h>

#include <stdexcept>

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

static void
event_handler (VKFWevent *e, void *user);

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

	::vkfwSetEventHandler (event_handler, nullptr);

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

static bool event_handler_has_thrown = false;

static void
event_handler (VKFWevent *e, void *user)
{
	(void) user;
	try {
		if (gApplication->KeepRunning ())
			gApplication->HandleEvent (*e);
	} catch (const std::exception &e) {
		event_handler_has_thrown = true;
		bLoggingEnabled = true;
		Log ("Caught an exception in an event handler: %s", e.what ());
	}
}

template <void (*initializer) (void), void (*terminator) (void)>
struct InitializeSystem {
	InitializeSystem (void)
	{
		initializer ();
	}

	~InitializeSystem (void)
	{
		terminator ();
	}
};

static void
run_event_loop (void)
{
	VkResult result;
	uint64_t prevFrameTime = vkfwGetTime ();

	InitializeSystem<DebugUIInit, DebugUITerminate> debug_ui;

	while (gApplication->KeepRunning ()) {
		if (gWindow && gWindow->IsVsyncSwapchain ())
			result = ::vkfwDispatchEvents (VKFW_EVENT_MODE_POLL, 0);
		else
			result = ::vkfwDispatchEvents (VKFW_EVENT_MODE_DEADLINE, prevFrameTime + 16666);

		if (result != VK_SUCCESS) {
			Log ("vkfwDispatchEvents returned %s", VulkanTypeToString (result));
			throw std::runtime_error ("vkfwDispatchEvents failed");
		}

		if (event_handler_has_thrown) {
			throw std::runtime_error ("An event handler threw an exception");
		}

		prevFrameTime = vkfwGetTime ();
		gApplication->Render ();
	}

	result = ::vkDeviceWaitIdle (gVkDevice);
	if (result != VK_SUCCESS)
		Log ("warning: vkDeviceWaitIdle returned %s", VulkanTypeToString (result));
}

}
