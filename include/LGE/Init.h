/**
 * Functions for engine setup and starting the event loop.
 * Copyright (C) 2024  dbstream
 */
#pragma once

namespace LGE {

class Application;

extern bool bIsProduction;
extern int gExitCode;

/**
 * Start the LGE engine.
 *
 * @param app Pointer to an Application instance.
 * @param argv nullptr or argument vector as passed to main.
 *
 * @return Status code that should be returned from main. This is equal to
 * gExitCode.
 */
int
LGEMain (Application &app, const char *const *argv);

}
