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
#if !defined( OEM_BUILD ) && !defined( HLDEMO_BUILD )

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "effects.h"
#include "customentity.h"
#include "gamerules.h"

#define	EGON_PRIMARY_VOLUME		450
#define EGON_BEAM_SPRITE		"sprites/xbeam1.spr"
#define EGON_FLARE_SPRITE		"sprites/XSpark1.spr"
#define EGON_SOUND_OFF			"weapons/egon_off1.wav"
#define EGON_SOUND_RUN			"weapons/egon_run3.wav"
#define EGON_SOUND_STARTUP		"weapons/egon_windup2.wav"

#define EGON_SWITCH_NARROW_TIME			0.75			// Time it takes to switch fire modes
#define EGON_SWITCH_WIDE_TIME			1.5

enum egon_e {
	EGON_IDLE1 = 0,
	EGON_FIDGET1,
	EGON_ALTFIREON,
	EGON_ALTFIRECYCLE,
	EGON_ALTFIREOFF,
	EGON_FIRE1,
	EGON_FIRE2,
	EGON_FIRE3,
	EGON_FIRE4,
	EGON_DRAW,
	EGON_HOLSTER
};

LINK_ENTITY_TO_CLASS( weapon_egon, CEgon );

void CEgon::Spawn( )
{
	Precache( );
	m_iId = WEAPON_EGON;
	SET_MODEL(ENT(pev), "models/w_all_items5.mdl");
	pev->body = 8;

	m_iDefaultAmmo = 1;

	FallInit();// get ready to fall down.
}


void CEgon::Precache( void )
{
	PRECACHE_MODEL("models/v_egon.mdl");

	PRECACHE_SOUND("items/9mmclip1.wav");

	PRECACHE_SOUND( EGON_SOUND_OFF );
	PRECACHE_SOUND( EGON_SOUND_RUN );
	PRECACHE_SOUND( EGON_SOUND_STARTUP );

	PRECACHE_MODEL( EGON_BEAM_SPRITE );
	PRECACHE_MODEL( EGON_FLARE_SPRITE );

	PRECACHE_SOUND ("weapons/357_cock1.wav");

//	m_usEgonFire = PRECACHE_EVENT ( 1, "events/egon_fire.sc" );
//	m_usEgonStop = PRECACHE_EVENT ( 1, "events/egon_stop.sc" );
}


BOOL CEgon::Deploy( void )
{
	m_pPlayer->m_newcross_active = 1;
	m_deployed = FALSE;
	m_fireState = FIRE_OFF;
	m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] = (int)m_pPlayer->pev->health;

	return DefaultDeploy( "models/v_egon.mdl", 0, EGON_DRAW, "hive" );
}

int CEgon::AddToPlayer( CBasePlayer *pPlayer )
{
	if ( CBasePlayerWeapon::AddToPlayer( pPlayer ) )
	{
		MESSAGE_BEGIN( MSG_ONE, gmsgWeapPickup, NULL, pPlayer->pev );
			WRITE_BYTE( m_iId );
		MESSAGE_END();

		m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] = (int)m_pPlayer->pev->health;

		return TRUE;
	}
	return FALSE;
}



void CEgon::Holster( int skiplocal /* = 0 */ )
{
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;
	m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] = (int)m_pPlayer->pev->health;

	SendWeaponAnim( EGON_HOLSTER );

    if ( m_fireState != FIRE_OFF || m_pBeam ) EndAttack();
}

int CEgon::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "Egon_Book";
	p->iMaxAmmo1 = 600;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = WEAPON_NOCLIP;
	p->iSlot = 3;
	p->iPosition = 2;
	p->iId = m_iId = WEAPON_EGON;
	p->iFlags = 0;
	p->iWeight = EGON_WEIGHT;

	return 1;
}

#define EGON_PULSE_INTERVAL			0.05
#define EGON_DISCHARGE_INTERVAL		0.05

float CEgon::GetPulseInterval( void )
{
	return EGON_PULSE_INTERVAL;
}

float CEgon::GetDischargeInterval( void )
{
	return EGON_DISCHARGE_INTERVAL;
}

BOOL CEgon::HasAmmo( void )
{
	if ( m_pPlayer->m_rgAmmo[ m_iPrimaryAmmoType ] <= 1 )
		return FALSE;

	return TRUE;
}

void CEgon::UseAmmo( int count )
{
	if ( m_pPlayer->pev->health > 1){
	m_pPlayer->pev->health -= 1;
	}

	m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] = (int)m_pPlayer->pev->health;

//	if ( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] >= count )
//		m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] = count;
//	else
//		m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] = 0;
}


void CEgon::PrimaryAttack( void )
{//Blood Shock
	m_fireMode = FIRE_WIDE;
	if ( m_pPlayer->pev->waterlevel== 3 || !HasAmmo())
	{
	if ( m_fireState != FIRE_OFF || m_pBeam ) EndAttack();
		else
	  PlayEmptySound( );
	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.5;
	SendWeaponAnim(0);
	return;
	}

	UTIL_MakeVectors( m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle );
	Vector vecAiming = gpGlobals->v_forward;
	Vector vecSrc	 = m_pPlayer->GetGunPosition( );

	switch( m_fireState )
	{
		case FIRE_OFF:
		{
			if (!HasAmmo())
			{
				m_flNextPrimaryAttack = m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.5;
				PlayEmptySound( );
				return;
			}

			m_flAmmoUseTime = UTIL_WeaponTimeBase();

			#ifndef CLIENT_DLL
		   	FX_FireGun(m_pPlayer->pev->v_angle, m_pPlayer->entindex(), 200, 0, FIREGUN_EGON );

			if(m_pPlayer->m_darkposion > 0){
			m_pPlayer->m_darkposion = 0;//隐身取消
			}
			#endif

			m_shakeTime = 0;

			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.1;
			m_shootTime = gpGlobals->time + 2;

			SendWeaponAnim(3);

			if ( m_fireMode == FIRE_WIDE )
				EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/egon_windup2.wav", 0.9, ATTN_NORM, 0, 130 );
			else
				EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/egon_windup2.wav", 0.9, ATTN_NORM, 0, 100 );

			pev->dmgtime = UTIL_WeaponTimeBase() + EGON_PULSE_INTERVAL;
			m_fireState = FIRE_CHARGE;
		}
		break;

		case FIRE_CHARGE:
		{
			Fire( vecSrc, vecAiming );

			if ( m_shootTime != 0 && m_shootTime <= gpGlobals->time)
			{
				SendWeaponAnim(3);

				if ( m_fireMode == FIRE_WIDE )
					EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_STATIC, "weapons/egon_run3.wav", 0.9, ATTN_NORM, 0, 130 );
				else
					EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_STATIC, "weapons/egon_run3.wav", 0.9, ATTN_NORM, 0, 100 );

				m_shootTime = 0;
			}

			if (!HasAmmo())
			{
				EndAttack();
				m_fireState = FIRE_OFF;
				m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 1.0;
				m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 1.0;
			}

		}
		break;
	}
}

void CEgon::SecondaryAttack( void )
{//Armor Cover
	if(!m_pPlayer->m_skill_deathmatch){
	return;
	}

	m_fireMode = FIRE_NARROW;
	if ( m_pPlayer->pev->waterlevel== 3 || !HasAmmo() || m_pPlayer->m_skill_maxarmor == 0
	|| m_pPlayer->m_skill_maxarmor == m_pPlayer->pev->armorvalue)
	{
	if ( m_fireState != FIRE_OFF || m_pBeam ) EndAttack();
		else
	  PlayEmptySound( );
	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.5;
	SendWeaponAnim(0);
	return;
	}

	UTIL_MakeVectors( m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle );
	Vector vecAiming = gpGlobals->v_forward;
	Vector vecSrc	 = m_pPlayer->GetGunPosition( );

	switch( m_fireState )
	{
		case FIRE_OFF:
		{
			m_flAmmoUseTime = UTIL_WeaponTimeBase();

			#ifndef CLIENT_DLL
		   	FX_FireGun(m_pPlayer->pev->v_angle, m_pPlayer->entindex(), 200, 1, FIREGUN_EGON );

			if(m_pPlayer->m_darkposion > 0){
			m_pPlayer->m_darkposion = 0;//隐身取消
			}
			#endif

			m_shakeTime = 0;

			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.1;
			m_shootTime = gpGlobals->time + 2;

			SendWeaponAnim(11);

			if ( m_fireMode == FIRE_WIDE )
				EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/egon_windup2.wav", 0.9, ATTN_NORM, 0, 130 );
			else
				EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/egon_windup2.wav", 0.9, ATTN_NORM, 0, 100 );

			pev->dmgtime = UTIL_WeaponTimeBase() + EGON_PULSE_INTERVAL;
			m_fireState = FIRE_CHARGE;
		}
		break;

		case FIRE_CHARGE:
		{
			Fire( vecSrc, vecAiming );

			if ( m_shootTime != 0 && m_shootTime <= gpGlobals->time)
			{
				SendWeaponAnim(11);

				if ( m_fireMode == FIRE_WIDE )
					EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_STATIC, "weapons/egon_run3.wav", 0.9, ATTN_NORM, 0, 130 );
				else
					EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_STATIC, "weapons/egon_run3.wav", 0.9, ATTN_NORM, 0, 100 );

				m_shootTime = 0;
			}
		}
		break;
	}
}

void CEgon::Fire( const Vector &vecOrigSrc, const Vector &vecDir )
{

	Vector vecDest = vecOrigSrc + vecDir * 3000;
	edict_t		*pentIgnore;
	TraceResult tr;

	pentIgnore = m_pPlayer->edict();
	Vector tmpSrc = vecOrigSrc + gpGlobals->v_up * -8 + gpGlobals->v_right * 3;

	// ALERT( at_console, "." );
	
	UTIL_TraceLine( vecOrigSrc, vecDest, dont_ignore_monsters, pentIgnore, &tr );

	if (tr.fAllSolid)
		return;

#ifndef CLIENT_DLL
	CBaseEntity *pEntity = CBaseEntity::Instance(tr.pHit);

	if (pEntity == NULL)
		return;


		if ( m_pSprite )
		{
			m_pSprite->pev->effects |= EF_NODRAW;
		}

	if (UTIL_PointContents(tr.vecEndPos) != CONTENTS_SKY)
	{
		if (tr.flFraction != 1.0){
			if(m_fireMode == FIRE_WIDE){
			FX_ImpBeam( tr.vecEndPos, tr.vecPlaneNormal, 1, IMPBEAM_EGON );
			}
		}
	}


#endif

	float timedist;

	switch ( m_fireMode )
	{
	case FIRE_NARROW:
#ifndef CLIENT_DLL
		if ( pev->dmgtime < gpGlobals->time )
		{
			if ( !m_pPlayer->IsAlive() ){
				return;
			}

			if (m_pPlayer->m_skill_maxarmor >= 100 && m_pPlayer->pev->armorvalue < m_pPlayer->m_skill_maxarmor)
			{
				if(m_pPlayer->m_skill_maxarmor >= 200){
				m_pPlayer->pev->armorvalue += 1;
				}
				else if(m_pPlayer->m_skill_maxarmor == 150){
				m_pPlayer->pev->armorvalue += 1.5;
				}
				else if(m_pPlayer->m_skill_maxarmor == 120){
				m_pPlayer->pev->armorvalue += 1.8;
				}
				else if(m_pPlayer->m_skill_maxarmor == 100){
				m_pPlayer->pev->armorvalue += 2;
				}
				if(m_pPlayer->pev->armorvalue > m_pPlayer->m_skill_maxarmor){
				m_pPlayer->pev->armorvalue = m_pPlayer->m_skill_maxarmor;
				}
				if ( gpGlobals->time >= m_flAmmoUseTime )
				{
				UseAmmo( 1 );
				m_flAmmoUseTime = gpGlobals->time + 0.05;
				}
			}
			// Wide mode uses 10 charges per second in single player
			
			pev->dmgtime = gpGlobals->time + GetDischargeInterval();
		}
#endif
		timedist = ( pev->dmgtime - gpGlobals->time ) / GetPulseInterval();
		break;
	
	case FIRE_WIDE:
#ifndef CLIENT_DLL
		if ( pev->dmgtime < gpGlobals->time )
		{
			// wide mode does damage to the ent, and radius damage
			ClearMultiDamage();
			if (pEntity->pev->takedamage)
			{
				pEntity->TraceAttack( m_pPlayer->pev, 10, vecDir, &tr, DMG_ENERGYBEAM);
			}
			ApplyMultiDamage(m_pPlayer->pev, m_pPlayer->pev);

			if ( !m_pPlayer->IsAlive() ){
				return;
			}

			// Wide mode uses 10 charges per second in single player
			if ( gpGlobals->time >= m_flAmmoUseTime )
			{
				UseAmmo( 1 );
				m_flAmmoUseTime = gpGlobals->time + 0.05;
			}

			pev->dmgtime = gpGlobals->time + GetDischargeInterval();
		}
#endif
		timedist = ( pev->dmgtime - gpGlobals->time ) / GetDischargeInterval();
		break;
	}

	if ( timedist < 0 )
		timedist = 0;
	else if ( timedist > 1 )
		timedist = 1;
	timedist = 1-timedist;

	UpdateEffect( tmpSrc, tr.vecEndPos, timedist );
}


void CEgon::UpdateEffect( const Vector &startPoint, const Vector &endPoint, float timeBlend )
{
	#ifndef CLIENT_DLL
	if ( !m_pBeam ) CreateEffect();

	m_pBeam->SetStartPos( endPoint );
	m_pBeam->SetBrightness( 255 - (timeBlend*180) );
	m_pBeam->SetWidth( 40 - (timeBlend*20) );

	if ( m_fireMode == FIRE_WIDE ){
		m_pBeam->SetColor( 60 + (25*timeBlend), 120 + (30*timeBlend), 64 + 80*fabs(sin(UTIL_WeaponTimeBase()*10)) );
	}
	else{
		m_pBeam->SetColor(0,0,0);
		m_pNoise->SetColor(0,0,0);
	}

	m_pNoise->SetStartPos( endPoint );
	#endif
}

void CEgon::CreateEffect( void )
{
	#ifndef CLIENT_DLL
	DestroyEffect();

	m_pBeam = CBeam::BeamCreate( "sprites/rings_all.spr", 35 );
	m_pBeam->PointEntInit( pev->origin, m_pPlayer->entindex() );
	m_pBeam->SetFlags( BEAM_FSINE );
	m_pBeam->SetFrame( 6 );
	m_pBeam->SetEndAttachment( 1 );
	m_pBeam->pev->spawnflags |= SF_BEAM_TEMPORARY;// Flag these to be destroyed on save/restore or level transition

	m_pNoise = CBeam::BeamCreate( "sprites/rings_all.spr", 35 );
	m_pNoise->PointEntInit( pev->origin, m_pPlayer->entindex() );
	m_pNoise->SetFrame( 14 );
	m_pNoise->SetScrollRate( 25 );
	m_pNoise->SetBrightness( 250 );
	m_pNoise->SetEndAttachment( 1 );
	m_pNoise->pev->spawnflags |= SF_BEAM_TEMPORARY;


		m_pBeam->SetScrollRate( 120 );
		m_pBeam->SetNoise( 6 );
		m_pNoise->SetColor( 80, 120, 255 );
		m_pNoise->SetNoise( 2 );

	#endif
}


void CEgon::DestroyEffect( void )
{
		#ifndef CLIENT_DLL
	if ( m_pBeam )
	{
		UTIL_Remove( m_pBeam );
		m_pBeam = NULL;
	}
	if ( m_pNoise )
	{
		UTIL_Remove( m_pNoise );
		m_pNoise = NULL;
	}
	#endif
}



void CEgon::WeaponIdle( void )
{
	ResetEmptySound( );

	m_pPlayer->GetAutoaimVector( AUTOAIM_5DEGREES );
	m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] = (int)m_pPlayer->pev->health;

	if ( m_flTimeWeaponIdle > UTIL_WeaponTimeBase() )
	return;

	if ( m_fireState != FIRE_OFF )
	EndAttack();

	int iAnim;
	switch ( RANDOM_LONG( 0, 1 ) )
	{
	case 0:	
		iAnim = 0;	
		break;
	
	default:
	case 1:
		iAnim = 1;
		break;
	}

	SendWeaponAnim( iAnim );
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + RANDOM_FLOAT(10,15);

	m_deployed = TRUE;
}



void CEgon::EndAttack( void )
{
	#ifndef CLIENT_DLL
	FX_FireGun(m_pPlayer->pev->v_angle, m_pPlayer->entindex(), 200, 0, FIREGUN_REMOVE );
	#endif
	STOP_SOUND(ENT(m_pPlayer->pev), CHAN_STATIC, "weapons/egon_run3.wav" );
	EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/egon_off1.wav", 0.9, ATTN_NORM, 0, 100); 

	bool bMakeNoise = false;
		
	if ( m_fireState != FIRE_OFF ) //Checking the button just in case!.
		 bMakeNoise = true;

	//PLAYBACK_EVENT_FULL( FEV_GLOBAL | FEV_RELIABLE, m_pPlayer->edict(), m_usEgonStop, 0, (float *)&m_pPlayer->pev->origin, (float *)&m_pPlayer->pev->angles, 0.0, 0.0, bMakeNoise, 0, 0, 0 );

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 2.0;
	m_flNextPrimaryAttack = m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.5;

	m_fireState = FIRE_OFF;

	DestroyEffect();
}



class CEgonAmmo : public CBasePlayerAmmo
{
	void Spawn( void )
	{ 
		Precache( );
		SET_MODEL(ENT(pev), "models/w_all_items1.mdl");
		pev->body = 4;
		CBasePlayerAmmo::Spawn( );
	}
	void Precache( void )
	{
		PRECACHE_SOUND("items/9mmclip1.wav");
	}
	BOOL AddAmmo( CBaseEntity *pOther ) 
	{ 
		if (pOther->GiveAmmo( AMMO_URANIUMBOX_GIVE, "uranium", URANIUM_MAX_CARRY ) != -1)
		{
			EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM);
			return TRUE;
		}
		return FALSE;
	}
};
LINK_ENTITY_TO_CLASS( ammo_egonclip, CEgonAmmo );

#endif