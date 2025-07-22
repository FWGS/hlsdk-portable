#pragma once
#if !defined(EXPORTDEF_H)
#define EXPORTDEF_H
#if XASH_WIN32 || __CYGWIN__
	#if __GNUC__
		#define EXPORT __attribute__ ((dllexport))
	#else
		#define EXPORT __declspec(dllexport) // Note: actually gcc seems to also supports this syntax.
	#endif
#else
	#if __GNUC__ >= 4
		#if defined(__EMSCRIPTEN__)
			#include <emscripten.h>
			#define EXPORT EMSCRIPTEN_KEEPALIVE
		#else
			#define EXPORT __attribute__ ((visibility ("default")))
		#endif
	#else
		#define EXPORT
	#endif
#endif
#define DLLEXPORT EXPORT
#define _DLLEXPORT EXPORT
#endif // EXPORTDEF_H
