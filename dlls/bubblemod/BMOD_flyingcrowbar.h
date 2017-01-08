// ---------------------------------------------------------------
// Flying Crowbar Entity. Ver 1.0 as seen in Lambda BubbleMod  
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
// Flying Crowbar Entity

#ifndef C_FLYING_CROWBAR
#define C_FLYING_CROWBAR

class CFlyingCrowbar : public CBaseEntity
{    
public: 

   void Spawn( void ); 
   void Precache( void );
   void EXPORT BubbleThink( void );
   void EXPORT SpinTouch( CBaseEntity *pOther );
   CBasePlayer	*m_pPlayer;

private:

   EHANDLE m_hOwner;        // Original owner is stored here so we can
                            // allow the crowbar to hit the user.
};

#endif


 



 

