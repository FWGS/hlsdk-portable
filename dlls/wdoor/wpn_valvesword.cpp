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


#define	CROWBAR_BODYHIT_VOLUME 128
#define	CROWBAR_WALLHIT_VOLUME 512

LINK_ENTITY_TO_CLASS( weapon_valvesword, CFireAxe );



enum gauss_e {
	AXE_IDLE = 0,
	AXE_DRAW,
	AXE_HOLSTER,
	AXE_ATTACK1_HIT,
	AXE_ATTACK1,
    AXE_ATTACK2,
	AXE_ATTACK2_HIT,
    AXE_ATTACK3,
	AXE_ATTACK3_HIT,
	AXE_IDLE1,
	AXE_IDLE2,
	AXE_BIG_WIND,
	AXE_BIG_HIT,
    AXE_BIG_MISS,
	AXE_BIG_LOOP
};


void CFireAxe::Spawn( )
{
	Precache( );
	m_iId = WEAPON_FIREAXE;
	SET_MODEL(ENT(pev), "models/w_all_items5.mdl");
	pev->body = 7;
	m_iClip = -1;

	FallInit();// get ready to fall down.
}


void CFireAxe::Precache( void )
{
	PRECACHE_MODEL("models/v_valvesword.mdl");

	PRECACHE_SOUND("weapons/axe_swing.wav");
//	Bug Fix 3.0 �����������Ч����ȥ��
//	PRECACHE_SOUND("weapons/axe_hit.wav");
//	PRECACHE_SOUND("weapons/axe_hitbody.wav");

	PRECACHE_SOUND("rmxp/086-Action01.wav");
	PRECACHE_SOUND("mario/mario_skill_hit.wav");

	PRECACHE_SOUND("weapons/valvesword_hit1.wav");
	PRECACHE_SOUND("weapons/valvesword_hit2.wav");
	PRECACHE_SOUND("weapons/valvesword_hitwall1.wav");

	m_usCFireAxe = PRECACHE_EVENT ( 1, "events/valvesword.sc" );
}

int CFireAxe::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = NULL;
	p->iMaxAmmo1 = -1;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = WEAPON_NOCLIP;
	p->iSlot = 0;
	p->iPosition = 1;
	p->iId = WEAPON_FIREAXE;
	p->iWeight = KNIFE_WEIGHT;
	return 1;
}



BOOL CFireAxe::Deploy( )
{
	m_pPlayer->m_newcross_active = 1;
	m_pPlayer->m_newcross_ontarget = 0;
	m_pPlayer->m_flAxeCharge = 0;
	return DefaultDeploy( "models/v_valvesword.mdl", 0, AXE_DRAW, "crowbar" );
}

void CFireAxe::Holster( int skiplocal /* = 0 */ )
{
	m_pPlayer->m_flAxeCharge = 0;
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;
	SendWeaponAnim( AXE_HOLSTER );

	#ifndef CLIENT_DLL	
	FX_FireGun(m_pPlayer->pev->v_angle, m_pPlayer->entindex(), 200, 2, 58 );
	m_pPlayer->m_needlekilled_time = 0;
	#endif
}


void FindHullIntersection2( const Vector &vecSrc, TraceResult &tr, float *mins, float *maxs, edict_t *pEntity )
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


void CFireAxe::PrimaryAttack()
{
	if (! Swing( 1 ))
	{
		SetThink( &CFireAxe::SwingAgain );
		pev->nextthink = gpGlobals->time + 0.1;
	}
}


void CFireAxe::Smack( )
{
	DecalGunshot( &m_trHit, BULLET_PLAYER_CROWBAR );
}


void CFireAxe::SwingAgain( void )
{
	Swing( 0 );
}

int CFireAxe::AddToPlayer( CBasePlayer *pPlayer )
{
	if ( CBasePlayerWeapon::AddToPlayer( pPlayer ) )
	{
		MESSAGE_BEGIN( MSG_ONE, gmsgWeapPickup, NULL, pPlayer->pev );
			WRITE_BYTE( m_iId );
		MESSAGE_END();
		return TRUE;
	}
	return FALSE;
}

void CFireAxe::Reload( void )
{
	if(m_pPlayer->m_skill_valvesword && m_pPlayer->m_swordrecover_time > gpGlobals->time){
		char text[256];
		sprintf( text, "CD:%1.0fs\n",m_pPlayer->m_swordrecover_time - gpGlobals->time);
		UTIL_CenterPrintAll( text );
		return;
	}

	//if(!(m_pPlayer->pev->flags & FL_ONGROUND) ){
	//return;	Bug Fix 3.0 - ȡ������վ������
	//}

	if(m_pPlayer->m_needlekilled_time < 1 && m_pPlayer->m_skill_valvesword){
	m_pPlayer->m_needlekilled_time = gpGlobals->time + 3.0;
	SendWeaponAnim( 15 );
	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 3.0;
	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 3.0;
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 3.0;

	#ifndef CLIENT_DLL	
	FX_FireGun(m_pPlayer->pev->v_angle, m_pPlayer->entindex(), 200, 0, 58 );

	if(m_pPlayer->m_darkposion > 0){
	m_pPlayer->m_darkposion = 0;//����ȡ��
	}
	#endif

	}
	return;
}


int CFireAxe::Swing( int fFirst )
{
	m_pPlayer->m_flAxeCharge = 0;
	int fDidHit = FALSE;

	TraceResult tr;

	UTIL_MakeVectors (m_pPlayer->pev->v_angle);
	Vector vecSrc	= m_pPlayer->GetGunPosition( );
	Vector vecEnd	= vecSrc + gpGlobals->v_forward * 56;

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
				FindHullIntersection2( vecSrc, tr, VEC_DUCK_HULL_MIN, VEC_DUCK_HULL_MAX, m_pPlayer->edict() );
			vecEnd = tr.vecEndPos;	// This is the point on the actual surface (the hull could have hit space)
		}
	}
#endif

	PLAYBACK_EVENT_FULL( FEV_NOTHOST, m_pPlayer->edict(), m_usCFireAxe, 
	0.0, (float *)&g_vecZero, (float *)&g_vecZero, 0, 0, 0,
	0.0, 0, 0.0 );


	if ( tr.flFraction >= 1.0 )
	{
		if (fFirst)
		{
			// miss
			m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.5;
			m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.5;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1.0;
			// player "shoot" animation
			m_pPlayer->SetAnimation( PLAYER_ATTACK1 );
		}
	}
	else
	{
		switch( ((m_iSwing++) % 2) + 1 )
		{
		case 0:
			SendWeaponAnim( AXE_ATTACK1 ); break;
		case 1:
			SendWeaponAnim( 6 ); break;
		case 2:
			SendWeaponAnim( 8 ); break;
		}

		// player "shoot" animation
		m_pPlayer->SetAnimation( PLAYER_ATTACK1 );
		
#ifndef CLIENT_DLL

		// hit
		fDidHit = TRUE;
		CBaseEntity *pEntity = CBaseEntity::Instance(tr.pHit);

		int tex = (int)TEXTURETYPE_Trace(&tr, vecSrc, vecEnd);
		int surface = (int)SURFACETYPE_Trace(&tr, vecSrc, vecEnd,m_pPlayer->Classify(),0);
		FX_ImpBullet( tr.vecEndPos, tr.vecPlaneNormal, vecSrc, surface, BULLET_CROWBAR, (float)tex );

		ClearMultiDamage( );
	
		pEntity->TraceAttack(m_pPlayer->pev, 15, gpGlobals->v_forward, &tr, DMG_CLUB ); 

		ApplyMultiDamage( m_pPlayer->pev, m_pPlayer->pev );

		// play thwack, smack, or dong sound
		float flVol = 1.0;
		int fHitWorld = TRUE;

		if (pEntity)
		{
			if ( pEntity->Classify() != CLASS_NONE && pEntity->Classify() != CLASS_MACHINE )
			{

				EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/valvesword_hit1.wav", 1, ATTN_NORM); 

				m_pPlayer->m_iWeaponVolume = CROWBAR_BODYHIT_VOLUME;

				flVol = 0.1;

				fHitWorld = FALSE;
			}
		}

		// play texture hit sound
		// UNDONE: Calculate the correct point of intersection when we hit with the hull instead of the line

		if (fHitWorld)
		{
			float fvolbar = TEXTURETYPE_PlaySound(&tr, vecSrc, vecSrc + (vecEnd-vecSrc)*2, BULLET_PLAYER_CROWBAR);

			if ( g_pGameRules->IsMultiplayer() )
			{
				// override the volume here, cause we don't play texture sounds in multiplayer, 
				// and fvolbar is going to be 0 from the above call.

				fvolbar = 1;
			}

			EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/valvesword_hitwall1.wav", fvolbar, ATTN_NORM, 0, 98 + RANDOM_LONG(0,3)); 


			// delay the decal a bit
			m_trHit = tr;
		}

		m_pPlayer->m_iWeaponVolume = flVol * CROWBAR_WALLHIT_VOLUME;
#endif
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.2;
		m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.2;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1.0;
		SetThink( &CFireAxe::Smack );
		pev->nextthink = UTIL_WeaponTimeBase() + 0.2;

		
	}
	return fDidHit;
}


int CFireAxe::Swing_big( int fFirst )
{
	int fDidHit = FALSE;
	int dmg_speed = 0;

	TraceResult tr;

	UTIL_MakeVectors (m_pPlayer->pev->v_angle);
	Vector vecSrc	= m_pPlayer->GetGunPosition( );
	Vector vecEnd	= vecSrc + gpGlobals->v_forward * 128;

	UTIL_TraceLine( vecSrc, vecEnd, dont_ignore_monsters, ENT( m_pPlayer->pev ), &tr );

#ifndef CLIENT_DLL
	if( (m_pPlayer->pev->flags & FL_ONGROUND) ){ 
		m_pPlayer->pev->velocity = m_pPlayer->pev->velocity + gpGlobals->v_forward * 400;
		m_pPlayer->pev->velocity.z = 0;
	}
	else{
			if(m_pPlayer->pev->velocity.Length() >= 650){
			vecEnd	= vecSrc + gpGlobals->v_forward * 256;
			dmg_speed = 100;//�����ٽ�!
			m_pPlayer->pev->velocity = m_pPlayer->pev->velocity + gpGlobals->v_forward * 300;
			}
			else{
			m_pPlayer->pev->velocity = m_pPlayer->pev->velocity + gpGlobals->v_forward * 100;
			}
	}

	if ( tr.flFraction >= 1.0 )
	{
		UTIL_TraceHull( vecSrc, vecEnd, dont_ignore_monsters, head_hull, ENT( m_pPlayer->pev ), &tr );
		if ( tr.flFraction < 1.0 )
		{
			// Calculate the point of intersection of the line (or hull) and the object we hit
			// This is and approximation of the "best" intersection
			CBaseEntity *pHit = CBaseEntity::Instance( tr.pHit );
			if ( !pHit || pHit->IsBSPModel() )
				FindHullIntersection2( vecSrc, tr, VEC_DUCK_HULL_MIN, VEC_DUCK_HULL_MAX, m_pPlayer->edict() );
			vecEnd = tr.vecEndPos;	// This is the point on the actual surface (the hull could have hit space)
		}
	}
#endif

//	PLAYBACK_EVENT_FULL( FEV_NOTHOST, m_pPlayer->edict(), m_usCFireAxe2, 
//	0.0, (float *)&g_vecZero, (float *)&g_vecZero, 0, 0, 0,
//	0.0, 0, 0.0 );


	if ( tr.flFraction >= 1.0 )
	{
	SendWeaponAnim( AXE_BIG_MISS ); 
	EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/axe_swing.wav", 1, ATTN_NORM); 

		if (fFirst)
		{
			// miss
			m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 1.0;
			m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 1.0;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1.0;
			// player "shoot" animation
			m_pPlayer->SetAnimation( PLAYER_ATTACK1 );
		}
	}
	else
	{
		

		SendWeaponAnim( AXE_BIG_HIT ); 
        //DecalGunshot( &m_trHit, BULLET_PLAYER_CROWBAR );
		// player "shoot" animation
		m_pPlayer->SetAnimation( PLAYER_ATTACK1 );
		
#ifndef CLIENT_DLL
		//m_pPlayer->pev->punchangle.x += 10;

		// hit
		fDidHit = TRUE;
		CBaseEntity *pEntity = CBaseEntity::Instance(tr.pHit);

		int tex = (int)TEXTURETYPE_Trace(&tr, vecSrc, vecEnd);
		int surface = (int)SURFACETYPE_Trace(&tr, vecSrc, vecEnd,m_pPlayer->Classify(),0);
		int dmg = 165 + m_pPlayer->m_flAxeCharge * 15 + dmg_speed;//180-300�ӿ�����

		FX_ImpBullet( tr.vecEndPos, tr.vecPlaneNormal, vecSrc, surface, BULLET_CROWBAR, (float)tex );

		ClearMultiDamage( );

		if(m_pPlayer->m_darkposion > 0){
		m_pPlayer->m_darkposion = 0;//����ȡ��
		//Ǳ��+�����˺�Ч��׷��!
		dmg = 600;//���賤��������900������˺�!
		FX_Explosion( tr.vecEndPos+(tr.vecPlaneNormal * 8), 47);
		EMIT_SOUND(ENT(pev), CHAN_WEAPON, "mario/mario_skill_hit.wav", 1, ATTN_NORM); 
		}

		pEntity->TraceAttack(m_pPlayer->pev, dmg, gpGlobals->v_forward, &tr, DMG_CLUB ); 

		ApplyMultiDamage( m_pPlayer->pev, m_pPlayer->pev );

		// play thwack, smack, or dong sound
		float flVol = 1.0;
		int fHitWorld = TRUE;

		EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/valvesword_hit2.wav", 1, ATTN_NORM); 

		FX_Explosion( tr.vecEndPos+(tr.vecPlaneNormal * 12), EXPLOSION_LIGHTSABER );

		if (pEntity)
		{
			if ( pEntity->Classify() != CLASS_NONE && pEntity->Classify() != CLASS_MACHINE )
			{
				m_pPlayer->m_iWeaponVolume = CROWBAR_BODYHIT_VOLUME;
/*				if ( !pEntity->IsAlive() )
					  return TRUE;
				else*/
					  flVol = 0.1;

				fHitWorld = FALSE;
			}
		}

		// play texture hit sound
		// UNDONE: Calculate the correct point of intersection when we hit with the hull instead of the line

		if (fHitWorld)
		{
			float fvolbar = TEXTURETYPE_PlaySound(&tr, vecSrc, vecSrc + (vecEnd-vecSrc)*2, BULLET_PLAYER_CROWBAR);

			if ( g_pGameRules->IsMultiplayer() )
			{
				// override the volume here, cause we don't play texture sounds in multiplayer, 
				// and fvolbar is going to be 0 from the above call.

				fvolbar = 1;
			}

			// delay the decal a bit
			m_trHit = tr;
		}

		m_pPlayer->m_iWeaponVolume = flVol * CROWBAR_WALLHIT_VOLUME;
#endif
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 1.2;
		m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 1.2;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1.2;
		SetThink( &CFireAxe::Smack );
		pev->nextthink = UTIL_WeaponTimeBase() + 0.2;

		
	}
	return fDidHit;
}

void CFireAxe::SecondaryAttack( void )
{
	if(m_pPlayer->m_flAxeCharge == 0){
	m_pPlayer->m_flAxeCharge = 1;
	SendWeaponAnim( AXE_BIG_WIND );
	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 1;
	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 1;
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1;
	return;
	}
	else if(m_pPlayer->m_flAxeCharge >= 1){
		m_pPlayer->m_flAxeCharge += 1;
		if(m_pPlayer->m_flAxeCharge > 9){
		m_pPlayer->m_flAxeCharge = 9;
		}
		SendWeaponAnim( AXE_BIG_LOOP );
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.1;
		m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.1;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.1;
	}
}


void CFireAxe::WeaponIdle( void )
{
	if ( m_flTimeWeaponIdle > UTIL_WeaponTimeBase() ){
		return;
	}

	if(m_pPlayer->m_flAxeCharge >= 1){
				
				Swing_big( 0 );
				m_pPlayer->m_flAxeCharge = 0;
				m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 1.0;
				m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 1.0;
				m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1.0;// idle pretty soon after shooting.

				return;
	}

	int iAnim;
	iAnim = AXE_IDLE;	
	SendWeaponAnim( iAnim );

	m_flTimeWeaponIdle = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 4, 5 ); // how long till we do this again.
}
