/*
 *
 * Copyright (c) 1994
 * Hewlett-Packard Company
 *
 * Permission to use, copy, modify, distribute and sell this software
 * and its documentation for any purpose is hereby granted without fee,
 * provided that the above copyright notice appear in all copies and
 * that both that copyright notice and this permission notice appear
 * in supporting documentation.  Hewlett-Packard Company makes no
 * representations about the suitability of this software for any
 * purpose.  It is provided "as is" without express or implied warranty.
 *
 */

#ifndef DEFALLOC_H
#define DEFALLOC_H


template <class T>
inline void destroy(T* pointer)
{
    pointer->~T();
}

static inline void destroy(char*) {}
static inline void destroy(unsigned char*) {}
static inline void destroy(short*) {}
static inline void destroy(unsigned short*) {}
static inline void destroy(int*) {}
static inline void destroy(unsigned int*) {}
static inline void destroy(long*) {}
static inline void destroy(unsigned long*) {}
static inline void destroy(float*) {}
static inline void destroy(double*) {}
static inline void destroy(char**) {}
static inline void destroy(unsigned char**) {}
static inline void destroy(short**) {}
static inline void destroy(unsigned short**) {}
static inline void destroy(int**) {}
static inline void destroy(unsigned int**) {}
static inline void destroy(long**) {}
static inline void destroy(unsigned long**) {}
static inline void destroy(float**) {}
static inline void destroy(double**) {}


template <class T>
inline void destroy(T* beg, T* end)
{
    for ( ; beg != end; ++beg)
	beg->~T();
}

static inline void destroy(char*, char*) {}
static inline void destroy(unsigned char*, unsigned char*) {}
static inline void destroy(short*, short*) {}
static inline void destroy(unsigned short*, unsigned short*) {}
static inline void destroy(int*, int*) {}
static inline void destroy(unsigned int*, unsigned int*) {}
static inline void destroy(long*, long*) {}
static inline void destroy(unsigned long*, unsigned long*) {}
static inline void destroy(float*, float*) {}
static inline void destroy(double*, double*) {}
static inline void destroy(char**, char**) {}
static inline void destroy(unsigned char**, unsigned char**) {}
static inline void destroy(short**, short**) {}
static inline void destroy(unsigned short**, unsigned short**) {}
static inline void destroy(int**, int**) {}
static inline void destroy(unsigned int**, unsigned int**) {}
static inline void destroy(long**, long**) {}
static inline void destroy(unsigned long**, unsigned long**) {}
static inline void destroy(float**, float**) {}
static inline void destroy(double**, double**) {}

#ifdef __GNUG__
static inline void *operator new(size_t, void *place) { return place; }
static inline void *operator new[](size_t, void *place) { return place; }
#else
#include <new.h>
#endif

#endif
