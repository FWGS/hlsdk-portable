//========= Copyright (c) 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#pragma once
#if !defined( IN_DEFSH )
#define IN_DEFSH

// up / down
#define	PITCH	0
// left / right
#define	YAW		1
// fall over
#define	ROLL	2 

#ifdef WINAPI_FAMILY
#if (!WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP) && WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP))
#define XASH_WINRT
#endif
#endif

#if defined(XASH_WINRT)
#define HSPRITE HSPRITE_win32
#include <windows.h>
#undef HSPRITE
#define GetCursorPos(x)
#define SetCursorPos(x,y)
#elif defined(_WIN32)
#define HSPRITE HSPRITE_win32
#include <windows.h>
#undef HSPRITE
#else
typedef struct point_s
{
	int x;
	int y;
} POINT;
#define GetCursorPos(x)
#define SetCursorPos(x,y)
#endif
#endif
