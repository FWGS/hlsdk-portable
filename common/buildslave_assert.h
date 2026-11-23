/*
buildslave_assert.h - stand-in for the header of the original CZDS build machine
The original builds pointed _wassert at the source path on the build machine.
Portable builds use the C runtime assert.
*/
#pragma once
#if !defined( BUILDSLAVE_ASSERT_H )
#define BUILDSLAVE_ASSERT_H

#include <assert.h>

#define BUILDSLAVE_PATH( path )	path

#if !defined( ASSERT_AT )
#define ASSERT_AT( expression, line )	assert( expression )
#endif

#endif // BUILDSLAVE_ASSERT_H
