/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"
#include "gamerules.h"


#define	BEAMKATANA_BODYHIT_VOLUME 128
#define	BEAMKATANA_WALLHIT_VOLUME 512

LINK_ENTITY_TO_CLASS( weapon_beamkatana, CBeamKatana );



enum gauss_e {
	BEAMKATANA_IDLE = 0,
	BEAMKATANA_DRAW,
	BEAMKATANA_HOLSTER,
	BEAMKATANA_ATTACK1HIT,
	BEAMKATANA_ATTACK1MISS,
	BEAMKATANA_ATTACK2MISS,
	BEAMKATANA_ATTACK2HIT,
	BEAMKATANA_ATTACK3MISS,
	BEAMKATANA_ATTACK3HIT
};


void CBeamKatana::Spawn( )
{
	Precache( );
	m_iId = WEAPON_BEAMKATANA;
	SET_MODEL(ENT(pev), "models/w_beamkatana.mdl");
	m_iClip = -1;

	FallInit();// get ready to fall down.
}


void CBeamKatana::Precache( void )
{
	PRECACHE_MODEL("models/v_beamkatana.mdl");
	PRECACHE_MODEL("models/w_beamkatana.mdl");
	PRECACHE_MODEL("models/p_beamkatana.mdl");
	PRECACHE_SOUND("weapons/beamkatana_hit1.wav");
	PRECACHE_SOUND("weapons/beamkatana_hit2.wav");
	PRECACHE_SOUND("weapons/beamkatana_hitbod1.wav");
	PRECACHE_SOUND("weapons/beamkatana_hitbod2.wav");
	PRECACHE_SOUND("weapons/beamkatana_hitbod3.wav");
	PRECACHE_SOUND("weapons/beamkatana_miss1.wav");
	PRECACHE_SOUND("weapons/beamkatana_out.wav");
	PRECACHE_SOUND("taunts/bk_1.wav");
	//PRECACHE_SOUND("taunts/bk_2.wav");
	PRECACHE_SOUND("taunts/bk_3.wav");
	PRECACHE_SOUND("taunts/bk_4.wav");
	PRECACHE_SOUND("taunts/bk_5.wav");
	PRECACHE_SOUND("taunts/bk_6.wav");

	m_usBeamKatana = PRECACHE_EVENT ( 1, "events/beamkatana.sc" );
}

int CBeamKatana::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = NULL;
	p->iMaxAmmo1 = -1;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = WEAPON_NOCLIP;
	p->iSlot = 0;
	p->iPosition = 1;
	p->iId = WEAPON_BEAMKATANA;
	p->iWeight = BEAMKATANA_WEIGHT;
	return 1;
}



BOOL CBeamKatana::Deploy( )
{
	return DefaultDeploy( "models/v_beamkatana.mdl", "models/p_beamkatana.mdl", BEAMKATANA_DRAW, "crowbar" );
	EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/beamkatana_out.wav", 1, ATTN_NORM, 0, 98 + RANDOM_LONG(0,3)); 					
}

void CBeamKatana::Holster( int skiplocal /* = 0 */ )
{
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;
	SendWeaponAnim( BEAMKATANA_HOLSTER );
}


void FindBullIntersection( const Vector &vecSrc, TraceResult &tr, float *mins, float *maxs, edict_t *pEntity )
{
	int			i, j, k;
	float		distance;
	float		*minmaxs[2] = {mins, maxs};
	TraceResult tmpTrace;
	Vector		vecHullEnd = tr.vecEndPos;
	Vector		vecEnd;

	distance = 1e6f;

	vecHullEnd = vecSrc + ((vecHullEnd - vecSrc)*2);
	UTIL_TraceLine( vecSrc, vecHullEnd, dont_ignore_monsters, pEntity, &tmpTrace );
	if ( tmpTrace.flFraction < 1.0 )
	{
		tr = tmpTrace;
		return;
	}

	for ( i = 0; i < 2; i++ )
	{
		for ( j = 0; j < 2; j++ )
		{
			for ( k = 0; k < 2; k++ )
			{
				vecEnd.x = vecHullEnd.x + minmaxs[i][0];
				vecEnd.y = vecHullEnd.y + minmaxs[j][1];
				vecEnd.z = vecHullEnd.z + minmaxs[k][2];

				UTIL_TraceLine( vecSrc, vecEnd, dont_ignore_monsters, pEntity, &tmpTrace );
				if ( tmpTrace.flFraction < 1.0 )
				{
					float thisDistance = (tmpTrace.vecEndPos - vecSrc).Length();
					if ( thisDistance < distance )
					{
						tr = tmpTrace;
						distance = thisDistance;
					}
				}
			}
		}
	}
}


void CBeamKatana::PrimaryAttack()
{
	if (! Swing( 1 ))
	{
		SetThink( &CBeamKatana::SwingAgain );
		pev->nextthink = gpGlobals->time + 0.1;
	}
}

void CBeamKatana::SecondaryAttack()
{
Fuckhead();
}

void CBeamKatana::Fuckhead()
{
				switch( RANDOM_LONG(0,4) )
				{
				case 0:
					EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_ITEM, "taunts/bk_1.wav", 1, ATTN_NORM); break;
				case 1:
					EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_ITEM, "taunts/bk_3.wav", 1, ATTN_NORM); break;
				case 2:
					EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_ITEM, "taunts/bk_4.wav", 1, ATTN_NORM); break;
				case 3:
					EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_ITEM, "taunts/bk_5.wav", 1, ATTN_NORM); break;
				case 4:
					EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_ITEM, "taunts/bk_6.wav", 1, ATTN_NORM); break;
				}

	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 2;	

}

void CBeamKatana::Smack( )
{
	DecalGunshot( &m_trHit, BULLET_PLAYER_BEAMKATANA );
}


void CBeamKatana::SwingAgain( void )
{
	Swing( 0 );
}


int CBeamKatana::Swing( int fFirst )
{
	int fDidHit = FALSE;

	TraceResult tr;

	UTIL_MakeVectors (m_pPlayer->pev->v_angle);
	Vector vecSrc	= m_pPlayer->GetGunPosition( );
	Vector vecEnd	= vecSrc + gpGlobals->v_forward * 32;

	UTIL_TraceLine( vecSrc, vecEnd, dont_ignore_monsters, ENT( m_pPlayer->pev ), &tr );

#ifndef CLIENT_DLL
	if ( tr.flFraction >= 1.0 )
	{
		UTIL_TraceHull( vecSrc, vecEnd, dont_ignore_monsters, head_hull, ENT( m_pPlayer->pev ), &tr );
		if ( tr.flFraction < 1.0 )
		{
			// Calculate the point of intersection of the line (or hull) and the object we hit
			// This is and approximation of the "best" intersection
			CBaseEntity *pHit = CBaseEntity::Instance( tr.pHit );
			if ( !pHit || pHit->IsBSPModel() )
				FindBullIntersection( vecSrc, tr, VEC_DUCK_HULL_MIN, VEC_DUCK_HULL_MAX, m_pPlayer->edict() );
			vecEnd = tr.vecEndPos;	// This is the point on the actual surface (the hull could have hit space)
		}
	}
#endif

	PLAYBACK_EVENT_FULL( FEV_NOTHOST, m_pPlayer->edict(), m_usBeamKatana, 
	0.0, (float *)&g_vecZero, (float *)&g_vecZero, 0, 0, 0,
	0.0, 0, 0.0 );


	if ( tr.flFraction >= 1.0 )
	{
		if (fFirst)
		{
			// miss
			m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.5;
			
			// player "shoot" animation
			m_pPlayer->SetAnimation( PLAYER_ATTACK1 );
		}
	}
	else
	{
		switch( ((m_iSwing++) % 2) + 1 )
		{
		case 0:
			SendWeaponAnim( BEAMKATANA_ATTACK1HIT ); break;
		case 1:
			SendWeaponAnim( BEAMKATANA_ATTACK2HIT ); break;
		case 2:
			SendWeaponAnim( BEAMKATANA_ATTACK3HIT ); break;
		}

		// player "shoot" animation
		m_pPlayer->SetAnimation( PLAYER_ATTACK1 );
		
#ifndef CLIENT_DLL

		// hit
		fDidHit = TRUE;
		CBaseEntity *pEntity = CBaseEntity::Instance(tr.pHit);

		ClearMultiDamage( );

		if ( (m_flNextPrimaryAttack + 1 < UTIL_WeaponTimeBase() ) || g_pGameRules->IsMultiplayer() )
		{
			// first swing does full damage
			pEntity->TraceAttack(m_pPlayer->pev, gSkillData.plrDmgBeamKatana, gpGlobals->v_forward, &tr, DMG_CLUB ); 
		}
		else
		{
			// subsequent swings do half
			pEntity->TraceAttack(m_pPlayer->pev, gSkillData.plrDmgBeamKatana / 2, gpGlobals->v_forward, &tr, DMG_CLUB ); 
		}	
		ApplyMultiDamage( m_pPlayer->pev, m_pPlayer->pev );

		// play thwack, smack, or dong sound
		float flVol = 1.0;
		int fHitWorld = TRUE;

		if (pEntity)
		{
			if ( pEntity->Classify() != CLASS_NONE && pEntity->Classify() != CLASS_MACHINE )
			{
				// play thwack or smack sound
				switch( RANDOM_LONG(0,2) )
				{
				case 0:
					EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/beamkatana_hitbod1.wav", 1, ATTN_NORM); break;
				case 1:
					EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/beamkatana_hitbod2.wav", 1, ATTN_NORM); break;
				case 2:
					EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/beamkatana_hitbod3.wav", 1, ATTN_NORM); break;
				}
				m_pPlayer->m_iWeaponVolume = BEAMKATANA_BODYHIT_VOLUME;
				if ( !pEntity->IsAlive() )
					  return TRUE;
				else
					  flVol = 0.1;

				fHitWorld = FALSE;
			}
		}

		// play texture hit sound
		// UNDONE: Calculate the correct point of intersection when we hit with the hull instead of the line

		if (fHitWorld)
		{
			float fvolbar = TEXTURETYPE_PlaySound(&tr, vecSrc, vecSrc + (vecEnd-vecSrc)*2, BULLET_PLAYER_BEAMKATANA);

			if ( g_pGameRules->IsMultiplayer() )
			{
				// override the volume here, cause we don't play texture sounds in multiplayer, 
				// and fvolbar is going to be 0 from the above call.

				fvolbar = 1;
			}

			// also play crowbar strike
			switch( RANDOM_LONG(0,1) )
			{
			case 0:
				EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/beamkatana_hit1.wav", fvolbar, ATTN_NORM, 0, 98 + RANDOM_LONG(0,3)); 
				break;
			case 1:
				EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/beamkatana_hit2.wav", fvolbar, ATTN_NORM, 0, 98 + RANDOM_LONG(0,3)); 
				break;
			}

			// delay the decal a bit
			m_trHit = tr;
		}

		m_pPlayer->m_iWeaponVolume = flVol * BEAMKATANA_WALLHIT_VOLUME;
#endif
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.25;
		
		SetThink( &CBeamKatana::Smack );
		pev->nextthink = UTIL_WeaponTimeBase() + 0.2;

		
	}
	return fDidHit;
}



