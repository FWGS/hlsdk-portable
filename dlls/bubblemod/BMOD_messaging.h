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

// This contains the messaging functions for the spectator messages
// rune, and weapon info messages.

#ifndef BMOD_MESSAGING
#define BMOD_MESSAGING

#include "extdll.h"
#include "util.h"
#include "cbase.h"

#define BMOD_CHAN_INFO		1
#define BMOD_CHAN_RUNE		2
#define BMOD_CHAN_COUNTDOWN	3
#define BMOD_CHAN_WEAPON		4

void PrintMessage( CBaseEntity *pEnt, int iChannel, Vector vecColor, Vector vecTime, const char *msg);

#endif
