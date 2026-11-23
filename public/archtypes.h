/*
archtypes.h - word size dependent definitions, portable stand-in for the Valve SDK header
the original CZDS code includes it from util.h
*/
#pragma once
#if !defined( ARCHTYPES_H )
#define ARCHTYPES_H

#include <stddef.h>
#include <limits.h>
#include <stdint.h>

// the fixed size types of public/steam/steamtypes.h the original code uses
typedef int8_t	int8;
typedef uint8_t	uint8;
typedef int16_t	int16;
typedef uint16_t	uint16;
typedef int32_t	int32;
typedef uint32_t	uint32;
typedef int64_t	int64;
typedef uint64_t	uint64;

#if !defined( _WIN32 )
#include <sys/types.h>
#include <sys/stat.h>

#if !defined( MAX_PATH )
#define MAX_PATH	PATH_MAX
#endif
#endif // !_WIN32

#include "minmax.h"

#endif // ARCHTYPES_H
