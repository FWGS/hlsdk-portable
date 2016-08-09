//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#if !defined( IN_DEFSH )
#define IN_DEFSH
#pragma once

// up / down
#define	PITCH	0
// left / right
#define	YAW		1
// fall over
#define	ROLL	2 

#ifdef _WIN32
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
