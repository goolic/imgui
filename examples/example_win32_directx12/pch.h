//pch.h

#include <stdio.h>          // vsnprintf, sscanf, printf
#include <float.h>                  // FLT_MIN, FLT_MAX
#include <stdarg.h>                 // va_list, va_start, va_end
#include <string.h> 
#if defined(_MSC_VER) 
#include <tchar.h>
#endif
#include <math.h>
#include <ctype.h>          // toupper
#include <limits.h>         // INT_MIN, INT_MAX
#include <stdlib.h>         // NULL, malloc, free, atoi
#include "enum.h"

#if defined(_MSC_VER) && _MSC_VER <= 1500 // MSVC 2008 or earlier
#include <stddef.h>     // intptr_t
#else
#include <stdint.h>     // intptr_t
#endif

#if !defined(alloca)
#if defined(__GLIBC__) || defined(__sun) || defined(__APPLE__) || defined(__NEWLIB__)
#include <alloca.h>     // alloca (glibc uses <alloca.h>. Note that Cygwin may have _WIN32 defined, so the order matters here)
#elif defined(_WIN32)
#include <malloc.h>     // alloca
#if !defined(alloca)
#define alloca _alloca  // for clang with MS Codegen
#endif
#else
#include <stdlib.h>     // alloca
#endif
#endif

// #ifndef STB_SPRINTF_IMPLEMENTATION
// 	#define STB_SPRINTF_IMPLEMENTATION
#include "stb_sprintf.h"
// #endif

#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx12.h"


#ifdef _MSC_VER
	#pragma comment(lib, "d3dcompiler") // Automatically link with d3dcompiler.lib as we are using D3DCompile() below.
	#include <d3dcompiler.h>

	#include <d3d12.h>
	#include <dxgi1_4.h>

#endif



#ifndef WIN32_LEAN_AND_MEAN
	#define WIN32_LEAN_AND_MEAN
#endif

#ifdef _DEBUG
	#define DX12_ENABLE_DEBUG_LAYER
#endif

#ifdef DX12_ENABLE_DEBUG_LAYER
#include <dxgidebug.h>
#pragma comment(lib, "dxguid.lib")
#endif

#if defined(_MSC_VER)
	#ifndef IMGUI_IMPL_WIN32_DISABLE_GAMEPAD
	#include <XInput.h>
	#else
	#define IMGUI_IMPL_WIN32_DISABLE_LINKING_XINPUT
	#endif
	#if defined(_MSC_VER) && !defined(IMGUI_IMPL_WIN32_DISABLE_LINKING_XINPUT)
		#pragma comment(lib, "xinput")
		//#pragma comment(lib, "Xinput9_1_0")
	#endif
#endif


#if defined(_MSC_VER) && !defined(_CRT_SECURE_NO_WARNINGS)
#define _CRT_SECURE_NO_WARNINGS
#endif


#if defined(_WIN32) && defined(IMGUI_DISABLE_DEFAULT_FILE_FUNCTIONS) && defined(IMGUI_DISABLE_WIN32_DEFAULT_CLIPBOARD_FUNCTIONS) && defined(IMGUI_DISABLE_WIN32_DEFAULT_IME_FUNCTIONS) && !defined(IMGUI_DISABLE_WIN32_FUNCTIONS)
#define IMGUI_DISABLE_WIN32_FUNCTIONS
#endif
#if defined(_WIN32) && !defined(IMGUI_DISABLE_WIN32_FUNCTIONS)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef __MINGW32__
#include <Windows.h>        // _wfopen, OpenClipboard
#else
#include <windows.h>
#endif
#if defined(WINAPI_FAMILY) && (WINAPI_FAMILY == WINAPI_FAMILY_APP) // UWP doesn't have all Win32 functions
#define IMGUI_DISABLE_WIN32_DEFAULT_CLIPBOARD_FUNCTIONS
#define IMGUI_DISABLE_WIN32_DEFAULT_IME_FUNCTIONS
#endif
#endif

// [Apple] OS specific includes
#if defined(__APPLE__)
#include <TargetConditionals.h>
#endif



#undef NOMINMAX
#undef WIN32_LEAN_AND_MEAN
