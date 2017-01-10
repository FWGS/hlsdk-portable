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

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"
#include "effects.h"
#include "gamerules.h"
#include "BMOD_boxmarker.h"

LINK_ENTITY_TO_CLASS( boxmarker, CBoxMarker );

void CBoxMarker :: Spawn( void )
{

	pev->classname = MAKE_STRING("boxmarker");

	pev->movetype = MOVETYPE_FLY;
	pev->solid = SOLID_NOT;

	SetThink( &CBoxMarker::PowerupThink );
	pev->nextthink = gpGlobals->time + RANDOM_FLOAT(.1, 1);

	m_vecExtents = Vector (16, 16, 16);
	m_beamsFired = 0;
}

void CBoxMarker :: PowerupThink( void )
{
	Vector vecStart, vecEnd;

	pev->nextthink = gpGlobals->time + RANDOM_FLOAT(.1, .5);

	switch (m_beamsFired)
	{
	case 0:
		vecStart = pev->origin + Vector(m_vecExtents.x *  1 , m_vecExtents.y *  1, m_vecExtents.z *  1);
		vecEnd   = pev->origin + Vector(m_vecExtents.x * -1 , m_vecExtents.y *  1, m_vecExtents.z *  1);
		break;
	case 1:
		vecStart = pev->origin + Vector(m_vecExtents.x * -1 , m_vecExtents.y *  1, m_vecExtents.z *  1);
		vecEnd   = pev->origin + Vector(m_vecExtents.x * -1 , m_vecExtents.y * -1, m_vecExtents.z *  1);
		break;
	case 2:
		vecStart = pev->origin + Vector(m_vecExtents.x * -1 , m_vecExtents.y * -1, m_vecExtents.z *  1);
		vecEnd   = pev->origin + Vector(m_vecExtents.x *  1 , m_vecExtents.y * -1, m_vecExtents.z *  1);
		break;
	case 3:
		vecStart = pev->origin + Vector(m_vecExtents.x *  1 , m_vecExtents.y * -1, m_vecExtents.z *  1);
		vecEnd   = pev->origin + Vector(m_vecExtents.x *  1 , m_vecExtents.y *  1, m_vecExtents.z *  1);
		break;
	case 4:
		vecStart = pev->origin + Vector(m_vecExtents.x *  1 , m_vecExtents.y *  1, m_vecExtents.z *  1);
		vecEnd   = pev->origin + Vector(m_vecExtents.x *  1 , m_vecExtents.y *  1, m_vecExtents.z * -1);
		break;
	case 5:
		vecStart = pev->origin + Vector(m_vecExtents.x * -1 , m_vecExtents.y *  1, m_vecExtents.z *  1);
		vecEnd   = pev->origin + Vector(m_vecExtents.x * -1 , m_vecExtents.y *  1, m_vecExtents.z * -1);
		break;
	case 6:
		vecStart = pev->origin + Vector(m_vecExtents.x * -1 , m_vecExtents.y * -1, m_vecExtents.z *  1);
		vecEnd   = pev->origin + Vector(m_vecExtents.x * -1 , m_vecExtents.y * -1, m_vecExtents.z * -1);
		break;
	case 7:
		vecStart = pev->origin + Vector(m_vecExtents.x *  1 , m_vecExtents.y * -1, m_vecExtents.z *  1);
		vecEnd   = pev->origin + Vector(m_vecExtents.x *  1 , m_vecExtents.y * -1, m_vecExtents.z * -1);
		break;
	case 8:
		vecStart = pev->origin + Vector(m_vecExtents.x *  1 , m_vecExtents.y *  1, m_vecExtents.z * -1);
		vecEnd   = pev->origin + Vector(m_vecExtents.x * -1 , m_vecExtents.y *  1, m_vecExtents.z * -1);
		break;
	case 9:
		vecStart = pev->origin + Vector(m_vecExtents.x * -1 , m_vecExtents.y *  1, m_vecExtents.z * -1);
		vecEnd   = pev->origin + Vector(m_vecExtents.x * -1 , m_vecExtents.y * -1, m_vecExtents.z * -1);
		break;
	case 10:
		vecStart = pev->origin + Vector(m_vecExtents.x * -1 , m_vecExtents.y * -1, m_vecExtents.z * -1);
		vecEnd   = pev->origin + Vector(m_vecExtents.x *  1 , m_vecExtents.y * -1, m_vecExtents.z * -1);
		break;
	case 11:
		vecStart = pev->origin + Vector(m_vecExtents.x *  1 , m_vecExtents.y * -1, m_vecExtents.z * -1);
		vecEnd   = pev->origin + Vector(m_vecExtents.x *  1 , m_vecExtents.y *  1, m_vecExtents.z * -1);
		SetThink(NULL);
		break;
	}
	
	m_pBeam[m_beamsFired] = CBeam::BeamCreate( g_pModelNameLaser, 10 );
	m_pBeam[m_beamsFired]->PointsInit( vecStart, vecEnd );
	m_pBeam[m_beamsFired]->SetColor( 0, 255, 0 );
	m_pBeam[m_beamsFired]->SetScrollRate( 255 );
	m_pBeam[m_beamsFired]->SetBrightness( 64 );

	m_beamsFired++;

}

