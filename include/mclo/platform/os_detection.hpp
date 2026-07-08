#pragma once

// Detects the target operating system from the predefined macros supplied by the compiler or platform SDK and defines
// the corresponding MCLO_OS_* macros. Detection is hierarchical: a platform family macro is defined for every member of
// that family, mirroring how MCLO_OS_APPLE is a superset of MCLO_OS_IOS and MCLO_OS_MACOS. For example MCLO_OS_WINDOWS
// is defined for both desktop Windows and Xbox GDK targets, and MCLO_OS_XBOX is further defined for each Xbox
// generation. If the operating system cannot be determined MCLO_OS_UNKNOWN is defined rather than raising a hard error,
// so the portable subset of the library still builds on unenumerated targets while OS specific headers fail loudly at
// their point of use.
//
// Only publicly available identifying macros are hardcoded here. Targets whose identity lives in NDA protected SDK
// headers, such as specific Nintendo hardware generations, are never named in this public header; define the
// corresponding MCLO_OS_* macro from your private platform toolchain instead and detection will honour it.

// clang-format off
#if defined( _WIN32 )
/// @brief Defined for the Windows platform family, covering both desktop Windows and Xbox GDK targets.
#define MCLO_OS_WINDOWS
#if defined( _GAMING_XBOX )
/// @brief Defined for the Xbox platform family (superset of the individual Xbox generations).
#define MCLO_OS_XBOX
#if defined( _GAMING_XBOX_SCARLETT )
/// @brief Defined when compiling for Xbox Series X|S (Scarlett).
#define MCLO_OS_XBOX_SCARLETT
#elif defined( _GAMING_XBOX_XBOXONE )
/// @brief Defined when compiling for Xbox One.
#define MCLO_OS_XBOX_ONE
#endif
#else
/// @brief Defined when compiling for desktop Windows (a non Xbox Windows target).
#define MCLO_OS_WINDOWS_PC
#endif
#elif defined( __PROSPERO__ ) || defined( __ORBIS__ )
/// @brief Defined for the Sony platform family (superset of the individual PlayStation consoles).
#define MCLO_OS_SONY
#if defined( __PROSPERO__ )
/// @brief Defined when compiling for PlayStation 5.
#define MCLO_OS_PS5
#elif defined( __ORBIS__ )
/// @brief Defined when compiling for PlayStation 4.
#define MCLO_OS_PS4
#endif
#elif defined( __SWITCH__ ) || defined( MCLO_OS_NINTENDO ) || defined( MCLO_OS_NINTENDO_SWITCH_1 ) || defined( MCLO_OS_NINTENDO_SWITCH_2 )
/// @brief Defined for the Nintendo platform family (superset of the individual Nintendo consoles).
#define MCLO_OS_NINTENDO
// Only the public homebrew macro __SWITCH__ is detected automatically. The Switch hardware generation cannot be
// determined from publicly available macros, so it is never detected here. If you know your exact target, define the
// appropriate generation macro yourself, typically from your private platform toolchain; the disabled blocks below
// only document the recognised names.
#if 0 // Define MCLO_OS_NINTENDO_SWITCH_2 yourself when targeting the Nintendo Switch 2.
/// @brief Defined when compiling for the Nintendo Switch 2.
#define MCLO_OS_NINTENDO_SWITCH_2
#endif
#if 0 // Define MCLO_OS_NINTENDO_SWITCH_1 yourself when targeting the original Nintendo Switch.
/// @brief Defined when compiling for the original Nintendo Switch.
#define MCLO_OS_NINTENDO_SWITCH_1
#endif
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
/// @brief Defined when the target operating system could not be determined.
#define MCLO_OS_UNKNOWN
#endif
// clang-format on
