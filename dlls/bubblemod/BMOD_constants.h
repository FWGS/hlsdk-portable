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
#define BMOD_BRANCH_VERSION	"1.0.0"

// The URL of the website describing your branch
#define BMOD_BRANCH_URL		"http://www.bubblemod.org"

// End branch customizations
// ---------------------------------------------------------------

#define BMOD_VERSION			"2.2.3a"

#ifdef _WIN32
#define BMOD_PLATFORM			"WIN32"
#else
#define BMOD_PLATFORM			"Linux"
#endif

#endif