#ifndef EXPORTDEF_H
#define EXPORTDEF_H
#if defined _WIN32 || defined __CYGWIN__
	#ifdef __GNUC__
		#define EXPORT __attribute__ ((dllexport))
	#else
		#define EXPORT __declspec(dllexport) // Note: actually gcc seems to also supports this syntax.
		//#define EXPORT __stdcall
	#endif
#else
  #if __GNUC__ >= 4
	#define EXPORT __attribute__ ((visibility ("default")))
   #else
	#define EXPORT
  #endif
#endif

#ifdef DLLEXPORT
#undef DLLEXPORT
#endif
#ifdef _DLLEXPORT
#undef DLLEXPORT
#endif

#define DLLEXPORT EXPORT

#define _DLLEXPORT EXPORT

#endif // EXPORTDEF_H
