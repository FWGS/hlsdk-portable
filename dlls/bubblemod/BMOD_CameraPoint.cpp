// ---------------------------------------------------------------
// Camera Point Entity
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
#include "gamerules.h"
#include "effects.h"
#include "BMOD_CameraPoint.h"
#include "shake.h"

LINK_ENTITY_TO_CLASS( campoint, CCamPoint );

void CCamPoint::Precache( void )
{
}

void CCamPoint::Spawn( void )
{
	pev->movetype = MOVETYPE_NOCLIP;
	pev->solid = SOLID_NOT;					// Remove model & collisions
	pev->renderamt = 0;						// The engine won't draw this model if this 
											// is set to 0 and blending is on
	pev->rendermode = kRenderTransTexture;

	SetThink( &CCamPoint::Think );
	pev->nextthink = gpGlobals->time + 0.1f;
}

void CCamPoint::Think( void )
{

	// If we have no owner, our owner is not in type mode,
	// our owner is not connected, our owner is invisible,
	// or our owner is a defuct player, remove ourselves.
	if( !m_pOwner
		|| !m_pOwner->m_bIsConnected 
		|| !m_pOwner->m_bTypeMode
		|| m_pOwner->pev->effects & EF_NODRAW
		|| ( ( STRING( m_pOwner->pev->netname ) )[0] == 0 )
		) {
		// UTIL_ClientPrintAll( HUD_PRINTTALK, "<SERVER> Camera point destroyed.\n");
		UTIL_Remove( this );
		pev->nextthink = gpGlobals->time + 0.1f;
		return;
	}

	pev->origin = m_pOwner->pev->origin + Vector(0,0,40), 
	pev->angles = Vector(m_pOwner->pev->angles.x + 22, 
						m_pOwner->pev->angles.y + 180, 
						0), 

	// Wait until it's time to think again.
	pev->nextthink = gpGlobals->time + 0.01f;
}
