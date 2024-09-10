/**
 * Debug logging.
 * Copyright (C) 2024  dbstream
 */
#define LGE_MODULE "LGELog"

#include <LGE/Log.h>
#include <stdio.h>
#include <stdarg.h>

#include <chrono>

namespace LGE {

bool bLoggingEnabled = true;

static double
get_print_time (void)
{
	/** wrap this function in a try-catch, as logging must never fail */
	try {
		using clock = std::chrono::high_resolution_clock;
		static clock::time_point t0 = clock::now ();
		clock::time_point t1 = clock::now ();

		std::chrono::duration<double> t = t1 - t0;
		return t.count ();
	} catch (...) {
		return 0.0;
	}
}

void
DebugPrint (const char *origin, const char *fmt, ...)
{
	// bLoggingEnabled is already checked by log, just print here.

	char logbuf[512];

	va_list args;
	va_start (args, fmt);
	vsnprintf (logbuf, sizeof (logbuf), fmt, args);
	va_end (args);

	// Make sure it is null-terminated.
	logbuf[sizeof (logbuf) - 1] = 0;
	printf ("%14.6f  %s: %s\n", get_print_time (), origin, logbuf);
}

}
