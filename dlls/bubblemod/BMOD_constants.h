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

#if defined(_WIN32) && defined(_MSC_VER)
#define BMOD_PLATFORM			"WIN32"
#elif defined(_WIN32) && defined(__MINGW32_)
#define BMOD_PLATFORM			"WIN32-MinGW"
#elif defined(__ANDROID__)
#define BMOD_PLATFORM			"Android"
#elif defined(__APPLE__) && ( defined(__arm__) || defined(__aarch64__) )
#define BMOD_PLATFORM			"ios"
#elif defined(__APPLE__)
#define BMOD_PLATFORM			"Apple"
#elif defined(__FreeBSD__)
#define BMOD_PLATFORM			"FreeBSD"
#elif defined(__NetBSD__)
#define BMOD_PLATFORM			"NetBSD"
#elif defined(__OpenBSD__)
#define BMOD_PLATFORM			"OpenBSD"
#else
#define BMOD_PLATFORM			"Linux"
#endif

#if defined(__amd64__) || defined(_M_X64)
#define BMOD_ARCH			"amd64"
#elif defined(__aarch64__)
#define BMOD_ARCH			"arm64"
#elif defined(__i386__) || defined(_X86_) || defined(_M_IX86)
#define BMOD_ARCH			"i386"
#elif defined(__arm__) || defined(_M_ARM)
#define BMOD_ARCH			"arm"
#else
#define BMOD_ARCH			"arch-unknown"
#endif

#endif
