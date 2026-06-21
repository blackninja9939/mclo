#pragma once

// Detects the target operating system from the predefined macros supplied by the compiler and defines exactly one
// top level MCLO_OS_* macro. On Apple platforms an additional macro distinguishing iOS from macOS is also defined.
// If the operating system cannot be determined a hard compile error is raised so unsupported targets fail loudly.

#if defined( _WIN32 )
/// @brief Defined when compiling for Windows.
#define MCLO_OS_WINDOWS
#elif defined( __linux__ )
/// @brief Defined when compiling for Linux.
#define MCLO_OS_LINUX
#elif defined( __ANDROID__ )
/// @brief Defined when compiling for Android.
#define MCLO_OS_ANDROID
#elif defined( __APPLE__ )
/// @brief Defined when compiling for an Apple operating system (iOS or macOS).
#define MCLO_OS_APPLE
#include <TargetConditionals.h>
#if TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR
/// @brief Defined when compiling for iOS, including the iOS simulator.
#define MCLO_OS_IOS
#elif TARGET_OS_MAC
/// @brief Defined when compiling for macOS.
#define MCLO_OS_MACOS
#endif
#else
#error Unable to detect operating system
#endif
