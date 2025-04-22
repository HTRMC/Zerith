// windows_wrapper.h
#pragma once

// Only define NOMINMAX if it hasn't been defined already
#if !defined(NOMINMAX)
#define NOMINMAX
#endif

#include <windows.h>

#undef ERROR
#undef WARN
