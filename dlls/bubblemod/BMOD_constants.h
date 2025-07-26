// ---------------------------------------------------------------
// BubbleMod
// 
// AUTHOR
//        Tyler Lund <halflife@bubblemod.org>
//
// LICENSE                                                            
//                                                                    
//        Permission is granted to anyone to use this  software  for  
//        any purpose on any computer system, and to redistribute it  
//        in any way, subject to the following restrictions:          
//                                                                    
//        1. The author is not responsible for the  consequences  of  
//           use of this software, no matter how awful, even if they  
//           arise from defects in it.                                
//        2. The origin of this software must not be misrepresented,  
//           either by explicit claim or by omission.                 
//        3. Altered  versions  must  be plainly marked as such, and  
//           must  not  be  misrepresented  (by  explicit  claim  or  
//           omission) as being the original software.                
//        3a. It would be nice if I got  a  copy  of  your  improved  
//            version  sent to halflife@bubblemod.org. 
//        4. This notice must not be removed or altered.              
//                                                                    
// ---------------------------------------------------------------
// Global BubbleMod Constants

#ifndef BMOD_CONSTANTS_H
#define BMOD_CONSTANTS_H
#include "build.h"
// ---------------------------------------------------------------
// Branch customizations
// Information in the next three constants will be used for servers
// running your mod on the BubbleWatch pages on bubblemod.org. Fill
// these in with the information you would like to appear on that page.
// 
// If you do NOT want servers running your branch to appear on the 
// BubbleWatch page, simply set BMOD_VERSION below to "", or run your
// server with no master server reporting.

// Fill this in with the title of your bubblemod branch
#define BMOD_BRANCH_NAME	"Custom Bubblemod"

// The version of your branch
#define BMOD_BRANCH_VERSION	"2.2.3"

// The URL of the website describing your branch
#define BMOD_BRANCH_URL		"http://www.bubblemod.org"

// End branch customizations
// ---------------------------------------------------------------

#define BMOD_VERSION			"2.2.3a"

#if XASH_WIN32
#define BMOD_PLATFORM			"Win32"
#elif XASH_DOS4GW
#define BMOD_PLATFORM			"DOS"
#elif XASH_ANDROID
#define BMOD_PLATFORM			"Android"
#elif XASH_IOS
#define BMOD_PLATFORM			"iOS"
#elif XASH_APPLE
#define BMOD_PLATFORM			"Apple"
#elif XASH_FREEBSD
#define BMOD_PLATFORM			"FreeBSD"
#elif XASH_NETBSD
#define BMOD_PLATFORM			"NetBSD"
#elif XASH_OPENBSD
#define BMOD_PLATFORM			"OpenBSD"
#elif XASH_LINUX
#define BMOD_PLATFORM			"Linux"
#elif XASH_HAIKU
#define BMOD_PLATFORM			"Haiku"
#elif XASH_SERENITY
#define BMOD_PLATFORM			"SerenityOS"
#elif XASH_IRIX
#define BMOD_PLATFORM			"IRIX"
#elif XASH_NSWITCH
#define BMOD_PLATFORM			"NSwitch"
#elif XASH_PSVITA
#define BMOD_PLATFORM			"PSVita"
#elif XASH_WASI
#define BMOD_PLATFORM			"WASI"
#elif XASH_SUNOS
#define BMOD_PLATFORM			"SunOS(Solaris)"
#elif XASH_EMSCRIPTEN
#define BMOD_PLATFORM			"Emscripten"
#else
#define BMOD_PLATFORM			"os-unknown"
#endif 

#if XASH_AMD64
#define BMOD_ARCH			"amd64"
#elif XASH_ARM >= 8
#define BMOD_ARCH			"arm64"
#elif XASH_X86
#define BMOD_ARCH			"i386"
#elif XASH_ARM
#define BMOD_ARCH			"arm"
#elif XASH_RISCV 
#if XASH_64BIT
#define BMOD_ARCH			"riscv64"
else
#define BMOD_ARCH			"riscv32"
#endif
#elif XASH_MIPS
#define BMOD_ARCH			"mips"
#elif XASH_E2K
#define BMOD_ARCH			"elbrus"
#elif XASH_PPC
#if XASH_64BIT
#define BMOD_ARCH			"powerpc64"
#else
#define BMOD_ARCH			"powerpc"
#endif
#elif XASH_WASM
#if XASH_64BIT
#define BMOD_ARCH			"wasm64"
#else
#define BMOD_ARCH			"wasm"
#endif
#else
#define BMOD_ARCH			"arch-unknown"
#endif

#endif
