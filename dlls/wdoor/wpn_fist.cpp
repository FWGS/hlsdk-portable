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


#define	FIST_BODYHIT_VOLUME 128
#define	FIST_WALLHIT_VOLUME 512

LINK_ENTITY_TO_CLASS( weapon_fist, CFist );

enum fist_e {
	FIST_IDLE = 0,
	FIST_DRAW,
	FIST_HOLSTER,
	FIST_ATTACK1HIT,
	FIST_ATTACK1MISS,
	FIST_ATTACK2HIT,
	FIST_ATTACK2MISS,
	FIST_ATTACK3HIT,
	FIST_ATTACK3MISS,
	FIST_CLIMB,
	FIST_WRONGDOOR
};

int CFist::AddToPlayer( CBasePlayer *pPlayer )
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

void CFist::Spawn( )
{
	Precache( );
	m_iId = WEAPON_FIST;
	SET_MODEL(ENT(pev), "models/w_all_items3.mdl");
	pev->body = 5;
	m_iClip = -1;

	FallInit();// get ready to fall down.
}


void CFist::Precache( void )
{
	PRECACHE_MODEL("models/v_fist.mdl");

	PRECACHE_SOUND("newadd/fist_explode.wav");
	PRECACHE_SOUND("newadd/fist_hitbod1.wav");
	PRECACHE_SOUND("newadd/fist_hitwall.wav");
	PRECACHE_SOUND("zombie/claw_miss1.wav");
	PRECACHE_SOUND("zombie/claw_miss2.wav");
	PRECACHE_SOUND("weapons/gluongun_fire.wav");
	PRECACHE_SOUND("newadd/fist_hearvy_hit1.wav");//����ȭ

	m_usCFist = PRECACHE_EVENT ( 1, "events/fist.sc" );
}

int CFist::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = NULL;
	p->iMaxAmmo1 = -1;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = WEAPON_NOCLIP;
	p->iSlot = 0;
	p->iPosition = 0;
	p->iId = WEAPON_FIST;
	p->iWeight = KNIFE_WEIGHT;
	return 1;
}



BOOL CFist::Deploy( )
{
	m_pPlayer->m_newcross_active = 0;

	#ifndef CLIENT_DLL	
	FX_FireGun(m_pPlayer->pev->v_angle, m_pPlayer->entindex(), 200, 1, 59 );
	#endif

	return DefaultDeploy( "models/v_fist.mdl", 0, FIST_DRAW, "crowbar",0,514 );
}

void CFist::Holster( int skiplocal /* = 0 */ )
{
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;
	SendWeaponAnim( FIST_HOLSTER );

	#ifndef CLIENT_DLL	
	FX_FireGun(m_pPlayer->pev->v_angle, m_pPlayer->entindex(), 200, 1, 59 );
	m_pPlayer->m_wrongdoor_time = 0;
	#endif
}

void CFist::Reload( void )
{
	if(m_pPlayer->m_skill_wrongdoor && m_pPlayer->m_wrongdoor_cover_time > gpGlobals->time){
		char text[256];
		sprintf( text, "CD:%1.0fs\n",m_pPlayer->m_wrongdoor_cover_time - gpGlobals->time);
		UTIL_CenterPrintAll( text );
		return;
	}

	if(m_pPlayer->m_wrongdoor_time < 1 && m_pPlayer->m_skill_wrongdoor){
	m_pPlayer->m_newcross_active = 1;
	m_pPlayer->m_newcross_ontarget = 0;

	SendWeaponAnim( FIST_WRONGDOOR );
	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 5.0;
	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 5.0;
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 5.0;
	m_pPlayer->m_wrongdoor_time = gpGlobals->time + 5.0;
	#ifndef CLIENT_DLL	
	FX_FireGun(m_pPlayer->pev->v_angle, m_pPlayer->entindex(), 200, 0, 59 );

	if(m_pPlayer->m_darkposion > 0){
	m_pPlayer->m_darkposion = 0;//����ȡ��
	}
	#endif
	}

	return;
}

void FindHullIntersection3( const Vector &vecSrc, TraceResult &tr, float *mins, float *maxs, edict_t *pEntity )
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


void CFist::PrimaryAttack()
{
	if (! Swing( 1 ))
	{
		SetThink( &CFist::SwingAgain );
		pev->nextthink = gpGlobals->time + 0.1;
	}
}

void CFist::SecondaryAttack()
{
	if (! Swing2( 1 ))
	{
		SetThink( &CFist::SwingAgain2 );
		pev->nextthink = gpGlobals->time + 0.1;
	}
}

void CFist::Smack( )
{
	DecalGunshot( &m_trHit, BULLET_PLAYER_CROWBAR );
}


void CFist::SwingAgain( void )
{
	Swing( 0 );
}

void CFist::SwingAgain2( void )
{
	Swing2( 0 );
}

int CFist::Swing2( int fFirst )
{
	int fDidHit = FALSE;
	int Climb = FALSE;

	int dist = 40;

	if(m_pPlayer->m_skill_locked && m_pPlayer->m_air_oxyan >= m_pPlayer->m_air_oxyan_max
	&& (m_pPlayer->pev->flags & FL_ONGROUND)){//����վ�ڵ������ʹ��!
	dist = 80;
	}

	TraceResult tr;

	UTIL_MakeVectors (m_pPlayer->pev->v_angle);
	Vector vecSrc	= m_pPlayer->GetGunPosition( );
	Vector vecEnd	= vecSrc + gpGlobals->v_forward * dist;

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
				FindHullIntersection3( vecSrc, tr, VEC_DUCK_HULL_MIN, VEC_DUCK_HULL_MAX, m_pPlayer->edict() );
			vecEnd = tr.vecEndPos;	// This is the point on the actual surface (the hull could have hit space)
		}
	}
#endif

	

#ifndef CLIENT_DLL
		// hit
	int push_power = 600;

	if ( tr.flFraction < 1.0 )
	{
		fDidHit = TRUE;	
		CBaseEntity *pEntity = CBaseEntity::Instance(tr.pHit);
		
		if (pEntity)
		{

			if(m_pPlayer->m_fSecondWorld == TRUE){//����Ŀ���ؼ���
				if(pEntity->IsBSPModel()){
					if ( !(m_pPlayer->pev->flags & FL_ONGROUND) ) {//�����ڿ��в���ʹ��
						Vector vecSrc2	= tr.vecEndPos + Vector(0,0,20);
						Vector vecEnd2	= vecSrc2 - Vector(0,0,45) + gpGlobals->v_forward * 15;
						UTIL_TraceLine( vecSrc2, vecEnd2, dont_ignore_monsters, ENT( pev ), &tr );
						if ( (vecSrc2 - tr.vecEndPos).Length() >= 3 && tr.flFraction < 1.0 )
						{
							if(m_pPlayer->pev->origin.z < tr.vecEndPos.z + 10){
								if(m_pPlayer->m_flVelocityModifier2 == 1){
									Climb = TRUE;
									dist = 40;
									m_pPlayer->m_flVelocityModifier2 = -2;
									m_pPlayer->pev->punchangle.x += 10;
									UTIL_Sparks( tr.vecEndPos );
									m_pPlayer->m_vecClimb = tr.vecEndPos;
									EMIT_SOUND(ENT(pev), CHAN_WEAPON, "newadd/struggle_hit.wav", 1, ATTN_NORM);
									m_pPlayer->m_ClimbWallTime = gpGlobals->time;
								}
							}
						}
					}
				}
			}

			if(dist >= 80){//����
				if(m_pPlayer->m_darkposion > 0){
				m_pPlayer->m_darkposion = 0;//����ȡ��
				}

				m_pPlayer->m_air_oxyan = 1;
				m_pPlayer->m_air_oxyan_stop_time = gpGlobals->time + 2.0;
				FX_Explosion( tr.vecEndPos + tr.vecPlaneNormal * 8, 133 );
				m_pPlayer->pev->punchangle.x += -4;
				push_power = 2400;
				EMIT_SOUND(ENT(pev), CHAN_WEAPON, "newadd/fist_explode.wav", 1, ATTN_NORM);
				m_pPlayer->pev->velocity = gpGlobals->v_forward * -200 + m_pPlayer->pev->velocity;
				m_pPlayer->pev->velocity.z *= 0.5;
			}
			else{
				m_pPlayer->pev->velocity = gpGlobals->v_forward * -100 + m_pPlayer->pev->velocity;
				m_pPlayer->pev->velocity.z *= 0.5;
			}
			
			if ( pEntity->pev->movetype == MOVETYPE_PUSHSTEP){//���ƶ�����?
			pEntity->pev->velocity = (pEntity->pev->origin - pev->origin).Normalize() * push_power + m_pPlayer->pev->velocity;
			pEntity->pev->velocity.z = 0;
			}
			else if ( pEntity->Classify() != CLASS_NONE && pEntity->Classify() != CLASS_MACHINE
			&& pEntity->Classify() != CLASS_MACHINE_ASS && pEntity->Classify() != CLASS_MACHINE_BLACK)
			{
				if ( pEntity->pev->flags & (FL_MONSTER|FL_CLIENT) && !(pEntity->pev->spawnflags & SF_MONSTER_PREDISASTER) ){
					if(pEntity->pev->movetype != MOVETYPE_NOCLIP && pEntity->pev->takedamage){
						CBaseMonster *pEnemyMonster;
						pEnemyMonster = pEntity->MyMonsterPointer();
						if(pEnemyMonster){
							if(pEnemyMonster->m_MonsterState != MONSTERSTATE_SCRIPT 
							&& pEnemyMonster->m_IdealMonsterState != MONSTERSTATE_SCRIPT
							&& !pEnemyMonster->m_godmode){

								if ( !(pEntity->pev->flags & FL_ONGROUND) ) {
								push_power *= 0.3;
								}

								if(dist >= 80){//����
								
									if(pEntity->pev->gravity <= 1.5){
										pEntity->pev->velocity = (pEntity->pev->origin - pev->origin).Normalize() * push_power + m_pPlayer->pev->velocity;
										pEntity->pev->velocity.z = 0;
										pEnemyMonster->Freeze_Monster(10);
									}

									pEntity->TakeDamage ( m_pPlayer->pev, m_pPlayer->pev, 80, DMG_AIR | DMG_NEVERGIB);
								}
								else if(pEntity->pev->gravity <= 1){
								pEntity->TakeDamage ( m_pPlayer->pev, m_pPlayer->pev, 0, DMG_FALL | DMG_NEVERGIB);
								pEntity->pev->velocity = (pEntity->pev->origin - pev->origin).Normalize() * push_power + m_pPlayer->pev->velocity;
								pEntity->pev->velocity.z = 0;
								}

							}
						}
					}
				}
			}
		}
	}
	
	if(m_pPlayer->m_fSecondWorld == TRUE){//����Ŀ���ؼ���
		if ( !(m_pPlayer->pev->flags & FL_ONGROUND) ) {
			Vector vecEnd2	= tr.vecEndPos - Vector(0,0,48);
			UTIL_TraceLine( tr.vecEndPos, vecEnd2, dont_ignore_monsters, ENT( pev ), &tr );
			if ( tr.flFraction < 1.0 )
			{
				if(m_pPlayer->pev->origin.z < tr.vecEndPos.z + 10){
					if(m_pPlayer->m_flVelocityModifier2 == 1){
						Climb = TRUE;
						m_pPlayer->m_flVelocityModifier2 = -2;
						m_pPlayer->m_vecClimb = tr.vecEndPos;
						m_pPlayer->pev->punchangle.x += 10;
						UTIL_Sparks( tr.vecEndPos );
						EMIT_SOUND(ENT(pev), CHAN_WEAPON, "newadd/struggle_hit.wav", 1, ATTN_NORM);
						m_pPlayer->m_ClimbWallTime = gpGlobals->time;
					}
				}
			}
			if ( !Climb) {
			m_pPlayer->pev->velocity = m_pPlayer->pev->velocity + gpGlobals->v_forward * 50;
			}
		}
	}
	


#endif
	if(!Climb){
		SendWeaponAnim( FIST_ATTACK1HIT );
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.75;
		m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.75;
	}
	else{
		SendWeaponAnim( FIST_CLIMB );
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 2.5;
		m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 2.5;
	}
		SetThink( &CFist::Smack );
		pev->nextthink = UTIL_WeaponTimeBase() + 0.2;

	return fDidHit;
}

int CFist::Swing( int fFirst )
{
	int fDidHit = FALSE;

	int dist = 32;
	int dmg1 = 5;
	int dmg2 = 30;

	if(m_pPlayer->m_skill_wrongdoor){//�ϲ�һ��ǿ��ȭͷ
	dmg1 = 10;
	dmg2 = 60;
	}

	if(m_pPlayer->pev->velocity.Length() >= 650){//����ȭ
	dist = 64;
	}

	TraceResult tr;

	UTIL_MakeVectors (m_pPlayer->pev->v_angle);
	Vector vecSrc	= m_pPlayer->GetGunPosition( );
	Vector vecEnd	= vecSrc + gpGlobals->v_forward * dist;

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
				FindHullIntersection3( vecSrc, tr, VEC_DUCK_HULL_MIN, VEC_DUCK_HULL_MAX, m_pPlayer->edict() );
			vecEnd = tr.vecEndPos;	// This is the point on the actual surface (the hull could have hit space)
		}
	}
#endif
	

	PLAYBACK_EVENT_FULL( FEV_NOTHOST, m_pPlayer->edict(), m_usCFist, 
	0.0, (float *)&g_vecZero, (float *)&g_vecZero, 0, 0, 0,
	0.0, 0, 0.0 );

	if ( tr.flFraction >= 1.0 )
	{
		if (fFirst)
		{
			// miss
			m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.5;
			m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.5;

			// player "shoot" animation
			m_pPlayer->SetAnimation( PLAYER_ATTACK1 );
		}
	}
	else
	{
		switch( ((m_iSwing++) % 2) )
		{
		case 0:
			SendWeaponAnim( FIST_ATTACK2HIT ); break;
		case 1:
			SendWeaponAnim( FIST_ATTACK3HIT ); break;
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


		/*
		if(m_pPlayer->m_skill_locked && m_pPlayer->m_air_oxyan >= m_pPlayer->m_air_oxyan_max){
		pEntity->TraceAttack(m_pPlayer->pev, 50, gpGlobals->v_forward, &tr, DMG_BLAST); 
		m_pPlayer->m_air_oxyan = 1;
		m_pPlayer->m_air_oxyan_stop_time = gpGlobals->time + 2.0;
		FX_Explosion( tr.vecEndPos + tr.vecPlaneNormal * 4, 133);
		m_pPlayer->pev->velocity = gpGlobals->v_forward * -500 + m_pPlayer->pev->velocity;
		m_pPlayer->pev->velocity.z *= 0.5;
		m_pPlayer->pev->punchangle.x += -4;
		m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.75;
		}
		else{
		*/
		if(dist == 64 && m_pPlayer->m_air_oxyan > 300){//Bug Fix 3.0 ���ؼ��ܱ���ȭ��
		m_pPlayer->m_air_oxyan -= 300;
		pEntity->TraceAttack(m_pPlayer->pev, dmg2, gpGlobals->v_forward, &tr, DMG_BLAST); 
		m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.75;
		EMIT_SOUND(ENT(pev), CHAN_WEAPON, "newadd/fist_hearvy_hit1.wav", 1, ATTN_NORM);
		FX_Explosion( tr.vecEndPos + tr.vecPlaneNormal * 4, 136);
		}
		else{
		pEntity->TraceAttack(m_pPlayer->pev, dmg1, gpGlobals->v_forward, &tr, DMG_CLUB); 
		}
		//}


		ApplyMultiDamage( m_pPlayer->pev, m_pPlayer->pev );

		// play thwack, smack, or dong sound
		float flVol = 1.0;
		int fHitWorld = TRUE;

		if (pEntity)
		{
			if ( pEntity->Classify() != CLASS_NONE && pEntity->Classify() != CLASS_MACHINE
				&& pEntity->Classify() != CLASS_MACHINE_ASS && pEntity->Classify() != CLASS_MACHINE_BLACK)
			{
	
				EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_ITEM, "newadd/fist_hitbod1.wav", 1, ATTN_NORM);

				m_pPlayer->m_iWeaponVolume = FIST_BODYHIT_VOLUME;

			    flVol = 0.1;

				fHitWorld = FALSE;
			}
		}

		// play texture hit sound
		// UNDONE: Calculate the correct point of intersection when we hit with the hull instead of the line

		if (fHitWorld)
		{
			float fvolbar = TEXTURETYPE_PlaySound(&tr, vecSrc, vecSrc + (vecEnd-vecSrc)*2, BULLET_PLAYER_CROWBAR);

			EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_ITEM, "newadd/fist_hitwall.wav", fvolbar, ATTN_NORM, 0, 98 + RANDOM_LONG(0,3)); 

			// delay the decal a bit
			m_trHit = tr;
		}

		m_pPlayer->m_iWeaponVolume = flVol * FIST_WALLHIT_VOLUME;
#endif

		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.25;
		m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.25;

		SetThink( &CFist::Smack );
		pev->nextthink = UTIL_WeaponTimeBase() + 0.2;

		
	}
	return fDidHit;
}



