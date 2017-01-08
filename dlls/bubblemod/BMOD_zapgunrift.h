// ---------------------------------------------------------------
// Zap Rift Entity
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

#ifndef C_ZAP_RIFT
#define C_ZAP_RIFT

class CZapRift : public CPointEntity
{
public:
        void Spawn( void );
        void Precache( void );
        void EXPORT Animate( void );
		Vector FindBeam( void );

        int  m_maxFrame;
        CSprite                *m_pSprite;
        float m_fLifeSpan;
        float m_fNextElectrify;

        Vector m_vecZapDir;

};

class CZapBounce : public CPointEntity
{
public:
        void Spawn( void );
        void Precache( void );
        void EXPORT BounceThink( void );

		Vector	m_vecStart;
        Vector	m_vecDir;
		edict_t		*pentIgnore;
		BOOL	m_bFirstZap;
        float m_fDamage;
        int m_iBounce;
};

#endif