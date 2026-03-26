#ifndef BYTESWAP_H
#define BYTESWAP_H

#include <stdint.h>
#include "build.h"

#define Swap32( x ) (((uint32_t)((( x ) & 255 ) << 24 )) + ((uint32_t)(((( x ) >> 8 ) & 255 ) << 16 )) + ((uint32_t)((( x ) >> 16 ) & 255 ) << 8 ) + ((( x ) >> 24 ) & 255 ))
#define Swap16( x ) ((uint16_t)((((uint16_t)( x ) >> 8 ) & 255 ) + (((uint16_t)( x ) & 255 ) << 8 )))

static inline uint32_t FloatAsUint( float v )
{
	uint32_t u;
	memcpy( &u, &v, sizeof(u) );
	return u;
}

static inline float UintAsFloat( uint32_t u )
{
	float v;
	memcpy( &v, &u, sizeof(v) );
	return v;
}

static inline float SwapFloat( float bf )
{
	uint32_t bi = FloatAsUint( bf );
	uint32_t li = Swap32( bi );
	return UintAsFloat( li );
}

template<typename T>
static inline T Byteswap( T n )
{
	switch ( sizeof(n) )
	{
		case 2: return Swap16( n );
		case 4: return Swap32( n );
	}
}

template<>
inline float Byteswap( float n )
{
	return SwapFloat( n );
}

template<typename T>
inline T UByteswap( T& n )
{
	T u;
	memcpy(&u, &n, sizeof(u) );
	switch ( sizeof(u) )
	{
		case 2: return Swap16( u );
		case 4: return Swap32( u );
	}
}

template<>
inline float UByteswap( float& n )
{
	float u;
	memcpy(&u, &n, sizeof(u) );
	return SwapFloat( u );
}


template<typename T>
inline void UByteswapSW( T& n )
{
	T u = UByteswap( n );
	memcpy(&n, &u, sizeof(u) );
}

#ifdef XASH_BIG_ENDIAN
	#define LittleToHost( x )   Byteswap( x )
	#define LittleToHostSW( x ) ( x = Byteswap( x ) )
	#define BigToHost( x ) ( x )
	#define BigToHostSW( x )
	// Unaligned data macros
	#define ULittleToHost( x )   UByteswap( x )
	#define ULittleToHostSW( x ) UByteswapSW( x )
#else
	#define LittleToHost( x ) ( x )
	#define LittleToHostSW( x )
	#define BigToHost( x )   Byteswap( x )
	#define BigToHostSW( x ) ( x = Byteswap( x ) )
	// As-is there are no little endian platforms that need these
	#define ULittleToHost( x )   LittleToHost( x )
	#define ULittleToHostSW( x ) LittleToHostSW( x )
#endif

#endif
