/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/
#pragma once
#ifndef EXTDLL_H
#define EXTDLL_H

//
// Global header file for extension DLLs
//

// Allow "DEBUG" in addition to default "_DEBUG"
#if _DEBUG
#define DEBUG 1
#endif

// Silence certain warnings
#if _MSC_VER
#pragma warning(disable : 4244)		// int or float down-conversion
#pragma warning(disable : 4305)		// int or float data truncation
#pragma warning(disable : 4201)		// nameless struct/union
#pragma warning(disable : 4514)		// unreferenced inline function removed
#pragma warning(disable : 4100)		// unreferenced formal parameter
#endif

// Prevent tons of unused windows definitions
#if _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOWINRES
#define NOSERVICE
#define NOMCX
#define NOIME
#define HSPRITE HSPRITE_win32
#include <windows.h>
#undef HSPRITE
#else // _WIN32
#ifndef FALSE
#define FALSE 0
#endif
#ifndef TRUE
#define TRUE (!FALSE)
#endif
#include <limits.h>
#include <stdarg.h>
typedef unsigned int ULONG;
typedef unsigned char BYTE;
typedef int BOOL;
#define MAX_PATH PATH_MAX
#ifndef PATH_MAX
#define PATH_MAX 4096
#endif
#endif //_WIN32

// Misc C-runtime library headers
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#if HAVE_CMATH
#include <cmath>
#else
#include <math.h>
#endif

#ifndef M_PI_F
#define M_PI_F          (float)M_PI
#endif

#if defined(__LP64__) || defined(__LLP64__) || defined(_WIN64) || (defined(__x86_64__) && !defined(__ILP32__) ) || defined(_M_X64) || defined(__ia64) || defined (_M_IA64) || defined(__aarch64__) || defined(__powerpc64__)
  #define XASH_64BIT
#endif

// Header file containing definition of globalvars_t and entvars_t
typedef unsigned int func_t;
typedef int string_t;				// from engine's pr_comp.h;
typedef float vec_t;				// needed before including progdefs.h

// Vector class
#include "vector.h"

// Defining it as a (bogus) struct helps enforce type-checking
#define vec3_t Vector

// Shared engine/DLL constants
#include "const.h"
#include "progdefs.h"
#include "edict.h"

// Shared header describing protocol between engine and DLLs
#include "eiface.h"

// Shared header between the client DLL and the game DLLs
#include "cdll_dll.h"
#ifndef Q_min
#define Q_min(a,b)  (((a) < (b)) ? (a) : (b))
#endif
#ifndef Q_max
#define Q_max(a,b)  (((a) > (b)) ? (a) : (b))
#endif

#endif //EXTDLL_H
