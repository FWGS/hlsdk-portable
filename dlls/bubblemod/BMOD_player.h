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

#ifndef BMOD_PLAYER_H
#define BMOD_PLAYER_H

#define OBS_CHASE_LOCKED		1
#define OBS_CHASE_FREE			2
#define OBS_ROAMING			3		
/*
typedef enum
{
	RUNE_NONE,
	RUNE_CROWBAR,
	RUNE_GRENADE,
	RUNE_357,
	RUNE_HEALTH,
	RUNE_BATTERY,
	RUNE_SHOTGUN,
} RUNE_FLAGS;
*/
#define RUNE_NONE		0
#define RUNE_CROWBAR	(1<<0)
#define RUNE_GRENADE	(1<<1)
#define RUNE_357		(1<<2)
#define RUNE_HEALTH		(1<<3)
#define RUNE_BATTERY	(1<<4)
#define RUNE_SHOTGUN	(1<<5)

#endif