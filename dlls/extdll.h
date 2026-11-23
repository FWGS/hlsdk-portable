/***
*
*	Copyright (c) 1996-2001, Valve LLC. All rights reserved.
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
#ifndef EXTDLL_H
#define EXTDLL_H


//
// Global header file for extension DLLs
//

// Allow "DEBUG" in addition to default "_DEBUG"
#ifdef _DEBUG
#define DEBUG 1
#endif

// Silence certain warnings
#pragma warning(disable : 4244)		// int or float down-conversion
#pragma warning(disable : 4305)		// int or float data truncation
#pragma warning(disable : 4201)		// nameless struct/union
#pragma warning(disable : 4514)		// unreferenced inline function removed
#pragma warning(disable : 4100)		// unreferenced formal parameter

#include "archtypes.h"     // DAL

// Prevent tons of unused windows definitions
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOWINRES
#define NOSERVICE
#define NOMCX
#define NOIME
#include "winsani_in.h"
#include "windows.h"
#include "winsani_out.h"
#else // _WIN32
#define FALSE 0
#define TRUE (!FALSE)
typedef uint32 ULONG;
typedef unsigned char BYTE;
typedef int BOOL;
#if !defined( MAX_PATH )
#define MAX_PATH PATH_MAX
#endif
#include <limits.h>
#include <strings.h>
#define _stricmp strcasecmp
#include <stdarg.h>
#include <string.h> // memset 
#ifndef max
#define max(a,b)  (((a) > (b)) ? (a) : (b))
#define _vsnprintf(a,b,c,d) vsnprintf(a,b,c,d)
#endif
#endif //_WIN32

// Misc C-runtime library headers
#include "minmax.h"
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include "safe_snprintf.h"
#ifndef __restrict
#define	__restrict 
#endif
#if !HAVE_STRLCPY
extern "C" size_t strlcpy(char * __restrict dst, const char * __restrict src, size_t dsize);
#endif
#if !HAVE_STRLCAT
extern "C" size_t strlcat(char * __restrict dst, const char * __restrict src, size_t dsize);
#endif
// NOTE: Never use cmath header in HLSDK
// because it's required to use std namespace
// directly if you want to override math calls.
#include <math.h>

#if !defined(M_PI_F)
#define M_PI_F          (float)M_PI
#endif
#include "math.h"

// Header file containing definition of globalvars_t and entvars_t
// func_t and string_t come from const.h (string_t is signed there, MAKE_STRING
// may give negative offsets on 64-bit platforms)
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

#endif //EXTDLL_H
