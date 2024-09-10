/**
 * Debug logging.
 * Copyright (C) 2024  dbstream
 */
#pragma once

#ifndef LGE_MODULE
#define LGE_MODULE "<no LGE_MODULE>"
#endif

namespace LGE {

extern bool bLoggingEnabled;

/**
 * Output a log message.
 *
 * @param origin message source. This should be LGE_MODULE.
 * @param fmt printf format string.
 *
 * @note It is strongly recommended to use Log instead of DebugPrint.
 */
void
DebugPrint (const char *origin, const char *fmt, ...);

/**
 * Output a message if logging is enabled.
 *
 * @param fmt printf format string.
 */
template <class ...Args>
static inline void
Log (const char *fmt, Args &&...args)
{
	if (!bLoggingEnabled)
		return;

	DebugPrint (LGE_MODULE, fmt, args...);
}

}
