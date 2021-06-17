/***
*
*	Copyright (c) 1999, 2000 Valve LLC. All rights reserved.
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
#if !OEM_BUILD && !HLDEMO_BUILD

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
#include "shake.h"
#include "BMOD_messaging.h"

#define	EGON_PRIMARY_VOLUME		450
#define EGON_BEAM_SPRITE		"sprites/xbeam1.spr"
#define EGON_FLARE_SPRITE		"sprites/XSpark1.spr"
#define EGON_SOUND_OFF			"weapons/egon_off1.wav"
#define EGON_SOUND_RUN			"weapons/egon_run3.wav"
#define EGON_SOUND_STARTUP		"weapons/egon_windup2.wav"
#define BUBB_SOUND_OFF			"debris/flesh5.wav"
#define BUBB_SOUND_RUN			"debris/flesh5.wav"
#define BUBB_SOUND_STARTUP		"debris/flesh6.wav"
#define HEAL_SOUND_OFF		"debris/beamstart7.wav"

#define EGON_SWITCH_NARROW_TIME			0.75			// Time it takes to switch fire modes
#define EGON_SWITCH_WIDE_TIME			1.5

#define BUBBLE_HEAL_RADIUS 64
#define BUBBLE_HEAL_AMT 70

extern cvar_t bm_gluon_mod;
extern cvar_t bm_thrust;

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


class CEgon : public CBasePlayerWeapon
{
public:
	int		Save( CSave &save );
	int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	void Spawn( void );
	void Precache( void );
	int iItemSlot( void ) { return 4; }
	int GetItemInfo(ItemInfo *p);
	int AddToPlayer( CBasePlayer *pPlayer );

	BOOL Deploy( void );
	void Holster( int skiplocal = 0 );

	void CreateEffect( void );
	void UpdateEffect( const Vector &startPoint, const Vector &endPoint, float timeBlend );
	void DestroyEffect( void );

	void EndBubbleAttack( void );
	void EndAttack( void );
	void BubbleAttack( void );
	void Attack( void );
	void PrimaryAttack( void );
	void SecondaryAttack( void );
	void WeaponIdle( void );
	static int g_fireAnims1[];
	static int g_fireAnims2[];

	float m_flAmmoUseTime;// since we use < 1 point of ammo per update, we subtract ammo on a timer.

	float GetPulseInterval( void );
	float GetDischargeInterval( void );

	void Fire( const Vector &vecOrigSrc, const Vector &vecDir );
	void BubbleFire( const Vector &vecOrigSrc, const Vector &vecDir );
	void FireHeal( void );

	BOOL HasAmmo( void )
	{
		if (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
			return FALSE;
		return TRUE;
	}

	void UseAmmo( int count )
	{
		if ( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] >= count )
			m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] -= count;
		else
			m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] = 0;
	}

	enum EGON_FIRESTATE { FIRE_OFF, FIRE_CHARGE };
	enum EGON_FIREMODE { FIRE_NARROW, FIRE_WIDE};

private:
	float				m_shootTime;
	CBeam				*m_pBeam;
	CBeam				*m_pNoise;
	CSprite				*m_pSprite;
	EGON_FIRESTATE		m_fireState;
	EGON_FIREMODE		m_fireMode;
	float				m_shakeTime;
	BOOL				m_deployed;
	float				m_bubbletime;

	float				m_healAmmoUsed;
	float				m_healAmmoUseTime;

	unsigned short m_usEgonFire;
	unsigned short m_usEgonStop;

};

LINK_ENTITY_TO_CLASS( weapon_egon, CEgon );

int CEgon::g_fireAnims1[] = { EGON_FIRE1, EGON_FIRE2, EGON_FIRE3, EGON_FIRE4 };
int CEgon::g_fireAnims2[] = { EGON_ALTFIRECYCLE };


TYPEDESCRIPTION	CEgon::m_SaveData[] = 
{
	DEFINE_FIELD( CEgon, m_pBeam, FIELD_CLASSPTR ),
	DEFINE_FIELD( CEgon, m_pNoise, FIELD_CLASSPTR ),
	DEFINE_FIELD( CEgon, m_pSprite, FIELD_CLASSPTR ),
	DEFINE_FIELD( CEgon, m_shootTime, FIELD_TIME ),
	DEFINE_FIELD( CEgon, m_fireState, FIELD_INTEGER ),
	DEFINE_FIELD( CEgon, m_fireMode, FIELD_INTEGER ),
	DEFINE_FIELD( CEgon, m_shakeTime, FIELD_TIME ),
	DEFINE_FIELD( CEgon, m_flAmmoUseTime, FIELD_TIME ),
};
IMPLEMENT_SAVERESTORE( CEgon, CBasePlayerWeapon );


void CEgon::Spawn( )
{
	pev->classname = MAKE_STRING("weapon_egon");
	Precache( );
	m_iId = WEAPON_EGON;
	SET_MODEL(ENT(pev), "models/w_egon.mdl");

	m_iDefaultAmmo = EGON_DEFAULT_GIVE;

	FallInit();// get ready to fall down.
}


void CEgon::Precache( void )
{
	PRECACHE_MODEL("models/w_egon.mdl");
	PRECACHE_MODEL("models/v_egon.mdl");
	PRECACHE_MODEL("models/p_egon.mdl");

	PRECACHE_MODEL("models/w_9mmclip.mdl");
	PRECACHE_SOUND("items/9mmclip1.wav");

	PRECACHE_SOUND( EGON_SOUND_OFF );
	PRECACHE_SOUND( EGON_SOUND_RUN );
	PRECACHE_SOUND( EGON_SOUND_STARTUP );

	PRECACHE_SOUND( BUBB_SOUND_OFF );
	PRECACHE_SOUND( BUBB_SOUND_RUN );
	PRECACHE_SOUND( BUBB_SOUND_STARTUP );	
	PRECACHE_SOUND( HEAL_SOUND_OFF );

	PRECACHE_MODEL( EGON_BEAM_SPRITE );
	PRECACHE_MODEL( EGON_FLARE_SPRITE );

	PRECACHE_SOUND ("weapons/357_cock1.wav");

	m_usEgonFire = PRECACHE_EVENT ( 1, "events/egon_fire.sc" );
	m_usEgonStop = PRECACHE_EVENT ( 1, "events/egon_stop.sc" );

}


BOOL CEgon::Deploy( void )
{
	if (bm_gluon_mod.value) {
		PrintMessage( m_pPlayer, BMOD_CHAN_WEAPON, Vector (20,250,20), Vector (1, 4, 2), "BUBBLE GUN\nPRIMARY FIRE: Breathe bubbles underwater.\nSECONDARY FIRE: Hold down 5 seconds for area effect healing.");
	}

	m_deployed = FALSE;
	return DefaultDeploy( "models/v_egon.mdl", "models/p_egon.mdl", EGON_DRAW, "egon" );
}

int CEgon::AddToPlayer( CBasePlayer *pPlayer )
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



void CEgon::Holster( int skiplocal /* = 0 */ )
{
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5f;
	// m_flTimeWeaponIdle = gpGlobals->time + UTIL_RandomFloat ( 10, 15 );
	SendWeaponAnim( EGON_HOLSTER );

	if ( m_fireState != FIRE_OFF ) {
		if (bm_gluon_mod.value) 
			EndBubbleAttack();
		else
			EndAttack();
	}
}

int CEgon::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "uranium";
	p->iMaxAmmo1 = URANIUM_MAX_CARRY;
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


//#define EGON_PULSE_INTERVAL			0.25
//#define EGON_DISCHARGE_INTERVAL		0.5

#define EGON_PULSE_INTERVAL			0.1
#define EGON_DISCHARGE_INTERVAL		0.1

float CEgon::GetPulseInterval( void )
{
	if ( g_pGameRules->IsMultiplayer() )
	{
		return 0.1f;
	}

	return EGON_PULSE_INTERVAL;
}

float CEgon::GetDischargeInterval( void )
{
	if ( g_pGameRules->IsMultiplayer() )
	{
		return 0.1f;
	}

	return EGON_DISCHARGE_INTERVAL;
}

void CEgon::BubbleAttack( void )
{
	UTIL_MakeVectors( m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle );
	Vector vecAiming = gpGlobals->v_forward;
	Vector vecSrc	 = m_pPlayer->GetGunPosition( );

	switch( m_fireState )
	{
		case FIRE_OFF:
		{
			if ( !HasAmmo() )
			{
				m_flNextPrimaryAttack = gpGlobals->time + 0.25f;
				if (m_flNextSecondaryAttack <= gpGlobals->time)
					m_flNextSecondaryAttack = gpGlobals->time + 0.25f;
				PlayEmptySound( );
				return;
			}

			m_flAmmoUseTime = gpGlobals->time;// start using ammo ASAP.

			SendWeaponAnim( g_fireAnims1[ RANDOM_LONG(0,ARRAYSIZE(g_fireAnims1)-1) ] );
			m_shakeTime = 0;

			m_pPlayer->m_iWeaponVolume = EGON_PRIMARY_VOLUME;
			m_flTimeWeaponIdle = gpGlobals->time + 0.1f;
			// m_shootTime = gpGlobals->time + 2.0f;
			m_shootTime = gpGlobals->time + 0.2f;

			if ( m_fireMode == FIRE_WIDE )
			{
				EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_WEAPON, BUBB_SOUND_STARTUP, 0.98, ATTN_NORM, 0, 125 );
			}
			else
			{
				EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_WEAPON, BUBB_SOUND_STARTUP, 0.9, ATTN_NORM, 0, 80 );
				m_healAmmoUsed = 0;
				m_healAmmoUseTime = -1;
			}

			pev->dmgtime = gpGlobals->time + GetPulseInterval();
			m_fireState = FIRE_CHARGE;
		}
		break;

		case FIRE_CHARGE:
		{
			if ( m_fireMode == FIRE_WIDE )
				BubbleFire( vecSrc, vecAiming );
			else
				FireHeal();

			m_pPlayer->m_iWeaponVolume = EGON_PRIMARY_VOLUME;

			if ( m_shootTime != 0 && gpGlobals->time > m_shootTime )
			{
				if ( m_fireMode == FIRE_WIDE )
				{
					EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_STATIC, BUBB_SOUND_RUN, 0.98, ATTN_NORM, 0, 100 + RANDOM_LONG(0, 100) );
				}
				else
				{
					EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_STATIC, BUBB_SOUND_RUN, 0.9, ATTN_NORM, 0, 50 + (m_healAmmoUsed * 2) );
				}

				m_shootTime += (0.1f + RANDOM_FLOAT(0.0f, 0.1f));
			}
			if ( !HasAmmo() )
			{
				EndBubbleAttack();
				m_fireState = FIRE_OFF;
				m_flNextPrimaryAttack = gpGlobals->time + 1.0f;
				if (m_flNextSecondaryAttack <= gpGlobals->time)
					m_flNextSecondaryAttack = gpGlobals->time + 1.0f;
			}

		}
		break;
	}
}

void CEgon::PrimaryAttack( void )
{
	if (bm_gluon_mod.value) {
		m_fireMode = FIRE_WIDE;
		BubbleAttack();
	}
	else {
		m_fireMode = FIRE_WIDE;
		Attack();
	}
}

void CEgon::SecondaryAttack( void )
{
	if (bm_gluon_mod.value) {
		m_fireMode = FIRE_NARROW;
		BubbleAttack();
	}
	else {
	}
}

// ***********************************
// Eggplant's Bubble Gun
// ***********************************
void CEgon::BubbleFire( const Vector &vecOrigSrc, const Vector &vecDir )
{

	if (!(m_bubbletime <= gpGlobals->time))
		return;
	m_bubbletime = gpGlobals->time + 0.05f;

	UTIL_MakeVectors( m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle );
	m_pPlayer->pev->velocity = m_pPlayer->pev->velocity - gpGlobals->v_forward * bm_thrust.value;

    TraceResult tr;

	// Give player air!
	m_pPlayer->pev->air_finished = gpGlobals->time + 12;

	// First determine where the bubble will appear. It will appear in a random location
	// from the barrel of the gun to +128 units from the barrel. 
	Vector tmpSrcMin = vecOrigSrc + gpGlobals->v_up * -8 + gpGlobals->v_right * 3;
	Vector tmpSrcMax = tmpSrcMin + gpGlobals->v_forward * RANDOM_LONG(32, 128);

	// We dont want to be able to shoot through walls or people, so trace the line from the barrel to the
	// bubble point and make sure nothing is in the way.
	UTIL_TraceLine(tmpSrcMin, tmpSrcMax, dont_ignore_monsters, ENT(pev), &tr);
	if (tr.fStartSolid)
		return;
	Vector tmpSrc1 = tmpSrcMin + gpGlobals->v_forward * 32;
	Vector tmpSrc2 = tr.vecEndPos;

	// Now trace from the bubble point up 256 units to see where the bubble should pop.
	Vector vecDest = tmpSrc2 + Vector ( 0, 0, 1 ) * 256;
	UTIL_TraceLine(tmpSrc2, vecDest, ignore_monsters, ENT(pev), &tr);	
	float flHeight = tr.vecEndPos.z - tmpSrc2.z;

	// Tell the client about the new bubble. 
	// MESSAGE_BEGIN( MSG_PAS, SVC_TEMPENTITY, tmpSrc );
        //         WRITE_BYTE( TE_BUBBLES );
        //         WRITE_COORD( vecMin.x );  // mins
        //         WRITE_COORD( vecMin.y );
        //         WRITE_COORD( vecMin.z );
        //         WRITE_COORD( vecMax.x );  // maxz
        //         WRITE_COORD( vecMax.y );
        //         WRITE_COORD( vecMax.z );
        //         WRITE_COORD( flHeight );                        // height
        //         WRITE_SHORT( g_sModelIndexBubbles );
        //         WRITE_BYTE( 3 ); // count
        //         WRITE_COORD( 8 ); // speed
        // MESSAGE_END();
	MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
                WRITE_BYTE( TE_BUBBLETRAIL );
                WRITE_COORD( tmpSrc1.x );  // mins
                WRITE_COORD( tmpSrc1.y );
                WRITE_COORD( tmpSrc1.z );
                WRITE_COORD( tmpSrc2.x );    // maxz
                WRITE_COORD( tmpSrc2.y );
                WRITE_COORD( tmpSrc2.z );
                WRITE_COORD( flHeight );                        // height
                WRITE_SHORT( g_sModelIndexBubbles );
                WRITE_BYTE( 5 ); // count
                WRITE_COORD( 8 ); // speed
        MESSAGE_END();
}

void CEgon::FireHeal( void ) {

	// Only do this every once in a while to save bandwidth.
	if (!(m_bubbletime <= gpGlobals->time))
                return;
        m_bubbletime = gpGlobals->time + 0.1f;

	Vector vecOrigSrc = m_pPlayer->GetGunPosition( );
	int bubbleRadius = 1;
	int numBubbles = 1;
	int bubbleSpeed = 8;

	if (m_healAmmoUseTime <= gpGlobals->time) {
		m_healAmmoUseTime = gpGlobals->time + 0.5f;
		UseAmmo(5);
		m_healAmmoUsed += 5;

		// Runes halve the ammo needed.
		int healTime = 50;
		if (m_pPlayer->m_RuneFlags == RUNE_BATTERY ||
			m_pPlayer->m_RuneFlags == RUNE_HEALTH) {
			healTime /= 2;
		}

		if (m_healAmmoUsed >= healTime) {
			// Heal all players within a radius.
			CBaseEntity *pEntity = NULL;

			// Find all the players inside the radius. Heal them, and make the screen flash.
			while ((pEntity = UTIL_FindEntityInSphere( pEntity, vecOrigSrc, BUBBLE_HEAL_RADIUS )) != NULL)
       		 	{
				if (pEntity->IsPlayer())
				{
					if (m_pPlayer->m_RuneFlags == RUNE_BATTERY)
					{
						pEntity->pev->armorvalue += BUBBLE_HEAL_AMT;
						if (pEntity->pev->armorvalue > 100)
							pEntity->pev->armorvalue = 100;
					}
					else
					{
						pEntity->TakeHealth(BUBBLE_HEAL_AMT, DMG_GENERIC);
					}
				}
					UTIL_ScreenFade( pEntity, Vector(0,128,255), 2, 0.5, 200, FFADE_IN );
			}

			// Make a big bubble cloud.
			bubbleRadius = BUBBLE_HEAL_RADIUS;
			numBubbles = 1000;
			bubbleSpeed = -8;

			// Turn off the bubble gun.
			EndBubbleAttack();

			// Make sure they can't do this again for a while.
			m_flNextSecondaryAttack = gpGlobals->time + 30;

			// Make a sound effect.
			EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_STATIC, HEAL_SOUND_OFF, 0.98, ATTN_NORM, 0, 100); 
		}
	}

	// Make bubble(s)
        TraceResult tr;
        Vector vecDest = vecOrigSrc + Vector ( 0, 0, 1 ) * 256;
        UTIL_TraceLine(vecOrigSrc, vecDest, ignore_monsters, ENT(pev), &tr);
        float flHeight = tr.vecEndPos.z - vecOrigSrc.z;

        // Tell the client about the new bubble.
        MESSAGE_BEGIN( MSG_PAS, SVC_TEMPENTITY, vecOrigSrc );
                WRITE_BYTE( TE_BUBBLES );
                WRITE_COORD( vecOrigSrc.x - bubbleRadius );  // mins
                WRITE_COORD( vecOrigSrc.y - bubbleRadius );
                WRITE_COORD( vecOrigSrc.z - bubbleRadius );
                WRITE_COORD( vecOrigSrc.x + bubbleRadius );  // maxz
                WRITE_COORD( vecOrigSrc.y + bubbleRadius );
                WRITE_COORD( vecOrigSrc.z + bubbleRadius );
                WRITE_COORD( flHeight );                        // height
                WRITE_SHORT( g_sModelIndexBubbles );
                WRITE_BYTE( numBubbles ); // count
                WRITE_COORD( bubbleSpeed ); // speed
        MESSAGE_END();
}


void CEgon::UpdateEffect( const Vector &startPoint, const Vector &endPoint, float timeBlend )
{
	if ( !m_pBeam )
	{
		CreateEffect();
	}

	m_pBeam->SetStartPos( endPoint );
	m_pBeam->SetBrightness( 255 - (timeBlend*180) );
	m_pBeam->SetWidth( 40 - (timeBlend*20) );

	if ( m_fireMode == FIRE_WIDE )
		m_pBeam->SetColor( 30 + (25*timeBlend), 30 + (30*timeBlend), 64 + 80*fabs(sin(gpGlobals->time*10)) );
	else
		m_pBeam->SetColor( 60 + (25*timeBlend), 120 + (30*timeBlend), 64 + 80*fabs(sin(gpGlobals->time*10)) );


	UTIL_SetOrigin( m_pSprite->pev, endPoint );
	m_pSprite->pev->frame += 8 * gpGlobals->frametime;
	if ( m_pSprite->pev->frame > m_pSprite->Frames() )
		m_pSprite->pev->frame = 0;

	m_pNoise->SetStartPos( endPoint );
}


void CEgon::CreateEffect( void )
{
	DestroyEffect();

	m_pBeam = CBeam::BeamCreate( EGON_BEAM_SPRITE, 40 );
	m_pBeam->PointEntInit( pev->origin, m_pPlayer->entindex() );
	m_pBeam->SetFlags( BEAM_FSINE );
	m_pBeam->SetEndAttachment( 1 );
	m_pBeam->pev->spawnflags |= SF_BEAM_TEMPORARY;	// Flag these to be destroyed on save/restore or level transition

	m_pNoise = CBeam::BeamCreate( EGON_BEAM_SPRITE, 55 );
	m_pNoise->PointEntInit( pev->origin, m_pPlayer->entindex() );
	m_pNoise->SetScrollRate( 25 );
	m_pNoise->SetBrightness( 100 );
	m_pNoise->SetEndAttachment( 1 );
	m_pNoise->pev->spawnflags |= SF_BEAM_TEMPORARY;

	m_pSprite = CSprite::SpriteCreate( EGON_FLARE_SPRITE, pev->origin, FALSE );
	m_pSprite->pev->scale = 1.0;
	m_pSprite->SetTransparency( kRenderGlow, 255, 255, 255, 255, kRenderFxNoDissipation );
	m_pSprite->pev->spawnflags |= SF_SPRITE_TEMPORARY;

	if ( m_fireMode == FIRE_WIDE )
	{
		m_pBeam->SetScrollRate( 50 );
		m_pBeam->SetNoise( 20 );
		m_pNoise->SetColor( 50, 50, 255 );
		m_pNoise->SetNoise( 8 );
	}
	else
	{
		m_pBeam->SetScrollRate( 110 );
		m_pBeam->SetNoise( 5 );
		m_pNoise->SetColor( 80, 120, 255 );
		m_pNoise->SetNoise( 2 );
	}
}


void CEgon::DestroyEffect( void )
{
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
	if ( m_pSprite )
	{
		if ( m_fireMode == FIRE_WIDE )
			m_pSprite->Expand( 10, 500 );
		else
			UTIL_Remove( m_pSprite );
		m_pSprite = NULL;
	}
}


void CEgon::WeaponIdle( void )
{
	ResetEmptySound( );

	if ( m_flTimeWeaponIdle > gpGlobals->time )
		return;

	if ( m_fireState != FIRE_OFF ) {
		if (bm_gluon_mod.value) 
			EndBubbleAttack();
		else
			EndAttack();
	}

	int iAnim;

	float flRand = RANDOM_FLOAT(0,1);

	if ( flRand <= 0.5f )
	{
		iAnim = EGON_IDLE1;
		m_flTimeWeaponIdle = gpGlobals->time + RANDOM_FLOAT(10,15);
	}
	else 
	{
		iAnim = EGON_FIDGET1;
		m_flTimeWeaponIdle = gpGlobals->time + 3;
	}

	SendWeaponAnim( iAnim );
	m_deployed = TRUE;
}

void CEgon::EndBubbleAttack( void )
{
	STOP_SOUND( ENT(m_pPlayer->pev), CHAN_STATIC, BUBB_SOUND_RUN );
	EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_WEAPON, BUBB_SOUND_OFF, 0.98, ATTN_NORM, 0, 100); 
	m_fireState = FIRE_OFF;
	m_flTimeWeaponIdle = gpGlobals->time + 2.0f;
	m_flNextPrimaryAttack = gpGlobals->time + 0.5f;
	// m_flNextSecondaryAttack = gpGlobals->time + 30.0f;
	DestroyEffect();
}

void CEgon::EndAttack( void )
{
	STOP_SOUND( ENT(m_pPlayer->pev), CHAN_STATIC, EGON_SOUND_RUN );
	EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_WEAPON, EGON_SOUND_OFF, 0.98, ATTN_NORM, 0, 100); 
	m_fireState = FIRE_OFF;
	m_flTimeWeaponIdle = gpGlobals->time + 2.0f;
	m_flNextPrimaryAttack = gpGlobals->time + 0.5f;
	// m_flNextSecondaryAttack = gpGlobals->time + 30.0f;
	DestroyEffect();
}

void CEgon::Attack( void )
{
	// don't fire underwater
	if ( m_pPlayer->pev->waterlevel == 3 )
	{
		
		if ( m_fireState != FIRE_OFF || m_pBeam )
		{
			EndAttack();
		}
		else
		{
			PlayEmptySound( );
		}
		return;
	}

	UTIL_MakeVectors( m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle );
	Vector vecAiming = gpGlobals->v_forward;
	Vector vecSrc	 = m_pPlayer->GetGunPosition( );

	int flags;
#if CLIENT_WEAPONS
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif

	switch( m_fireState )
	{
		case FIRE_OFF:
		{
			if ( !HasAmmo() )
			{
				m_flNextPrimaryAttack = m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.25f;
				PlayEmptySound( );
				return;
			}

			m_flAmmoUseTime = gpGlobals->time;// start using ammo ASAP.

			PLAYBACK_EVENT_FULL( flags, m_pPlayer->edict(), m_usEgonFire, 0.0, (float *)&g_vecZero, (float *)&g_vecZero, 0.0, 0.0, m_fireState, m_fireMode, 1, 0 );
						
			m_shakeTime = 0;

			m_pPlayer->m_iWeaponVolume = EGON_PRIMARY_VOLUME;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.1f;
			pev->fuser1	= UTIL_WeaponTimeBase() + 2.0f;
			m_shootTime = gpGlobals->time + 2.0f;

			if ( m_fireMode == FIRE_WIDE )
			{
				EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_WEAPON, EGON_SOUND_STARTUP, 0.98, ATTN_NORM, 0, 125 );
			}
			else
			{
				EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_WEAPON, EGON_SOUND_STARTUP, 0.9, ATTN_NORM, 0, 80 );
			}

			pev->dmgtime = gpGlobals->time + GetPulseInterval();
			m_fireState = FIRE_CHARGE;
		}
		break;

		case FIRE_CHARGE:
		{
			Fire( vecSrc, vecAiming );

			m_pPlayer->m_iWeaponVolume = EGON_PRIMARY_VOLUME;

			if ( m_shootTime != 0 && gpGlobals->time > m_shootTime )
			{
				EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_STATIC, EGON_SOUND_RUN, 0.98, ATTN_NORM, 0, 125 );
				m_shootTime = 0;
			}			

			//if ( pev->fuser1 <= UTIL_WeaponTimeBase() )
			//{
		//		PLAYBACK_EVENT_FULL( flags, m_pPlayer->edict(), m_usEgonFire, 0, (float *)&g_vecZero, (float *)&g_vecZero, 0.0, 0.0, m_fireState, m_fireMode, 0, 0 );
		//		pev->fuser1 = 1000;
		//	}

			if ( !HasAmmo() )
			{
				EndAttack();
				m_flNextPrimaryAttack = m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 1.0f;
			}

		}
		break;
	}
}

void CEgon::Fire( const Vector &vecOrigSrc, const Vector &vecDir )
{
	Vector vecDest = vecOrigSrc + vecDir * 2048;
	edict_t		*pentIgnore;
	TraceResult tr;

	pentIgnore = m_pPlayer->edict();
	Vector tmpSrc = vecOrigSrc + gpGlobals->v_up * -8 + gpGlobals->v_right * 3;

	// ALERT( at_console, "." );
	
	UTIL_TraceLine( vecOrigSrc, vecDest, dont_ignore_monsters, pentIgnore, &tr );

	if (tr.fAllSolid)
		return;

#if !CLIENT_DLL
	CBaseEntity *pEntity = CBaseEntity::Instance(tr.pHit);

	if (pEntity == NULL)
		return;

	if ( g_pGameRules->IsMultiplayer() )
	{
		if ( m_pSprite && pEntity->pev->takedamage )
		{
			m_pSprite->pev->effects &= ~EF_NODRAW;
		}
		else if ( m_pSprite )
		{
			m_pSprite->pev->effects |= EF_NODRAW;
		}
	}


#endif

	float timedist;

	switch ( m_fireMode )
	{
	case FIRE_NARROW:
#if !CLIENT_DLL
		if ( pev->dmgtime < gpGlobals->time )
		{
			// Narrow mode only does damage to the entity it hits
			ClearMultiDamage();
			if (pEntity->pev->takedamage)
			{
				pEntity->TraceAttack( m_pPlayer->pev, gSkillData.plrDmgEgonNarrow, vecDir, &tr, DMG_ENERGYBEAM );
			}
			ApplyMultiDamage(m_pPlayer->pev, m_pPlayer->pev);

			if ( g_pGameRules->IsMultiplayer() )
			{
				// multiplayer uses 1 ammo every 1/10th second
				if ( gpGlobals->time >= m_flAmmoUseTime )
				{
					UseAmmo( 1 );
					m_flAmmoUseTime = gpGlobals->time + 0.1f;
				}
			}
			else
			{
				// single player, use 3 ammo/second
				if ( gpGlobals->time >= m_flAmmoUseTime )
				{
					UseAmmo( 1 );
					m_flAmmoUseTime = gpGlobals->time + 0.166f;
				}
			}

			pev->dmgtime = gpGlobals->time + GetPulseInterval();
		}
#endif
		timedist = ( pev->dmgtime - gpGlobals->time ) / GetPulseInterval();
		break;
	
	case FIRE_WIDE:
#if !CLIENT_DLL
		if ( pev->dmgtime < gpGlobals->time )
		{
			// wide mode does damage to the ent, and radius damage
			ClearMultiDamage();
			if (pEntity->pev->takedamage)
			{
				pEntity->TraceAttack( m_pPlayer->pev, gSkillData.plrDmgEgonWide, vecDir, &tr, DMG_ENERGYBEAM | DMG_ALWAYSGIB);
			}
			ApplyMultiDamage(m_pPlayer->pev, m_pPlayer->pev);

			if ( g_pGameRules->IsMultiplayer() )
			{
				// radius damage a little more potent in multiplayer.
				::RadiusDamage( tr.vecEndPos, pev, m_pPlayer->pev, gSkillData.plrDmgEgonWide/4, 128, CLASS_NONE, DMG_ENERGYBEAM | DMG_BLAST | DMG_ALWAYSGIB );
			}

			if ( !m_pPlayer->IsAlive() )
				return;

			if ( g_pGameRules->IsMultiplayer() )
			{
				//multiplayer uses 5 ammo/second
				if ( gpGlobals->time >= m_flAmmoUseTime )
				{
					UseAmmo( 1 );
					m_flAmmoUseTime = gpGlobals->time + 0.2f;
				}
			}
			else
			{
				// Wide mode uses 10 charges per second in single player
				if ( gpGlobals->time >= m_flAmmoUseTime )
				{
					UseAmmo( 1 );
					m_flAmmoUseTime = gpGlobals->time + 0.1f;
				}
			}

			pev->dmgtime = gpGlobals->time + GetDischargeInterval();
			if ( m_shakeTime < gpGlobals->time )
			{
				UTIL_ScreenShake( tr.vecEndPos, 5.0, 150.0, 0.75, 250.0 );
				m_shakeTime = gpGlobals->time + 1.5f;
			}
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


class CEgonAmmo : public CBasePlayerAmmo
{
	void Spawn( void )
	{ 
		pev->classname = MAKE_STRING("ammo_egonclip");
		Precache( );
		SET_MODEL(ENT(pev), "models/w_chainammo.mdl");
		CBasePlayerAmmo::Spawn( );
	}
	void Precache( void )
	{
		PRECACHE_MODEL ("models/w_chainammo.mdl");
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
