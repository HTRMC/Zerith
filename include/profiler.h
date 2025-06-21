#pragma once

#ifdef TRACY_ENABLE
#include <tracy/Tracy.hpp>

#define PROFILE_FUNCTION() ZoneScoped
#define PROFILE_SCOPE(name) ZoneScopedN(name)
#define PROFILE_FRAME(name) FrameMarkNamed(name)
#define PROFILE_FRAME_START(name) FrameMarkStart(name)
#define PROFILE_FRAME_END(name) FrameMarkEnd(name)
#define PROFILE_ALLOC(ptr, size) TracyAlloc(ptr, size)
#define PROFILE_FREE(ptr) TracyFree(ptr)
#define PROFILE_MESSAGE(text, size) TracyMessage(text, size)
#define PROFILE_MESSAGE_L(text) TracyMessageL(text)
#define PROFILE_VALUE(name, value) TracyPlot(name, value)

#else

#define PROFILE_FUNCTION()
#define PROFILE_SCOPE(name)
#define PROFILE_FRAME(name)
#define PROFILE_FRAME_START(name)
#define PROFILE_FRAME_END(name)
#define PROFILE_ALLOC(ptr, size)
#define PROFILE_FREE(ptr)
#define PROFILE_MESSAGE(text, size)
#define PROFILE_MESSAGE_L(text)
#define PROFILE_VALUE(name, value)

#endif