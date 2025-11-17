#pragma once

#if defined( _WIN32 )
#define MCLO_OS_WINDOWS
#elif defined( __linux__ )
#define MCLO_OS_LINUX
#elif defined( __ANDROID__ )
#define MCLO_OS_ANDROID
#elif defined( __APPLE__ )
#define MCLO_OS_APPLE
#include <TargetConditionals.h>
#if TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR
#define MCLO_OS_IOS
#elif TARGET_OS_MAC
#define MCLO_OS_MACOS
#endif
#else
#error Unable to detect operating system
#endif
