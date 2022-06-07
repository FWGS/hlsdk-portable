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
#if !OEM_BUILD && !HLDEMO_BUILD

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"
#include "hornet.h"
#include "gamerules.h"
#include "effects.h"
#include "BMOD_messaging.h"
#include "decals.h"
#include "BMOD_zapgunrift.h"
#include "shake.h"
#include "squeakgrenade.h"

short iBSquidSpitSprite;

extern cvar_t bm_freezetime;
extern cvar_t bm_hornet_mod;

enum hgun_e
{
	HGUN_IDLE1 = 0,
	HGUN_FIDGETSWAY,
	HGUN_FIDGETSHAKE,
	HGUN_DOWN,
	HGUN_UP,
	HGUN_SHOOT
};

enum hgun_phase
{
	HGUN_IDLE = 0,
	HGUN_CHARGE,
	HGUN_ZAP,
	HGUN_ZAP_DONE,
	HGUN_SPIT
};

enum firemode_e
{
	FIREMODE_TRACK = 0,
	FIREMODE_FAST
};

//=========================================================
// Bullsquid's spit projectile
//=========================================================
class BMODSquidSpit : public CBaseEntity
{
public:
        void Spawn( void );

        static void Shoot( entvars_t *Owner, Vector vecStart, Vector vecVelocity );
        void Touch( CBaseEntity *pOther );
        void EXPORT Animate( void );

        static  TYPEDESCRIPTION m_SaveData[];

        int  m_maxFrame;
	entvars_t *pevOwner;

};

LINK_ENTITY_TO_CLASS( squidspit, BMODSquidSpit );

void BMODSquidSpit:: Spawn( void )
{
        pev->movetype = MOVETYPE_FLY;
        pev->classname = MAKE_STRING( "squidspit" );

        pev->solid = SOLID_BBOX;
        pev->rendermode = kRenderTransAlpha;
        pev->renderamt = 255;

        SET_MODEL( ENT( pev ), "sprites/bigspit.spr" );
        pev->frame = 0;
        pev->scale = 0.5;

        UTIL_SetSize( pev, Vector( -4, -4, -4 ), Vector( 4, 4, 4 ) );

        m_maxFrame = (float)MODEL_FRAMES( pev->modelindex ) - 1;

	MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
        	WRITE_BYTE( TE_BEAMFOLLOW );
	        WRITE_SHORT( entindex() ); // entity
	        WRITE_SHORT( g_sModelIndexLightning ); // model
	        WRITE_BYTE( 2 ); // life
	        WRITE_BYTE( 4 ); // width
	        WRITE_BYTE( 128 ); // r, g, b
	        WRITE_BYTE( 200 ); // r, g, b
	        WRITE_BYTE( 0 ); // r, g, b
	        WRITE_BYTE( 255 ); // brightness
        MESSAGE_END(); // move PHS/PVS data sending into here (SEND_ALL, SEND_PVS, SEND_PHS)
}

void BMODSquidSpit::Animate( void )
{
        pev->nextthink = gpGlobals->time + 0.1f;

        if( pev->frame++ )
        {
		if( pev->frame > m_maxFrame )
		{
			pev->frame = 0;
		}
	}
}

void BMODSquidSpit::Shoot( entvars_t *Owner, Vector vecStart, Vector vecVelocity )
{
	BMODSquidSpit *pSpit = GetClassPtr( (BMODSquidSpit *)NULL );
	pSpit->Spawn();

	UTIL_SetOrigin( pSpit->pev, vecStart );
	pSpit->pev->velocity = vecVelocity;
	pSpit->pev->owner = ENT( Owner );
	pSpit->pevOwner = Owner;

	pSpit->SetThink( &BMODSquidSpit::Animate );
	pSpit->pev->nextthink = gpGlobals->time + 0.1f;
}

void BMODSquidSpit::Touch( CBaseEntity *pOther )
{
	TraceResult tr;
	int iPitch;

	// splat sound
	iPitch = RANDOM_FLOAT( 90, 110 );

	EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, "bullchicken/bc_acid1.wav", 1, ATTN_NORM, 0, iPitch );

	switch( RANDOM_LONG( 0, 1 ) )
	{
	case 0:
		EMIT_SOUND_DYN( ENT( pev ), CHAN_WEAPON, "bullchicken/bc_spithit1.wav", 1, ATTN_NORM, 0, iPitch );
                break;
        case 1:
		EMIT_SOUND_DYN( ENT( pev ), CHAN_WEAPON, "bullchicken/bc_spithit2.wav", 1, ATTN_NORM, 0, iPitch );
		break;
	}
        
        // make a splat on the wall
        UTIL_TraceLine( pev->origin, pev->origin + pev->velocity, dont_ignore_monsters, ENT( pev ), &tr );
        UTIL_DecalTrace( &tr, DECAL_SPIT1 + RANDOM_LONG( 0, 1 ) );
	
        // make some flecks
        MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, tr.vecEndPos );
        	WRITE_BYTE( TE_SPRITE_SPRAY );
                WRITE_COORD( tr.vecEndPos.x );	// pos
                WRITE_COORD( tr.vecEndPos.y );
                WRITE_COORD( tr.vecEndPos.z );
                WRITE_COORD( tr.vecPlaneNormal.x );	// dir
                WRITE_COORD( tr.vecPlaneNormal.y );
                WRITE_COORD( tr.vecPlaneNormal.z );
                WRITE_SHORT( iBSquidSpitSprite );	// model
                WRITE_BYTE( 5 );			// count
                WRITE_BYTE( 30 );			// speed
                WRITE_BYTE( 80 );			// noise ( client will divide by 100 )
        MESSAGE_END();

        if( pOther->IsPlayer() )
	{
		ClearMultiDamage();
       	       	pOther->TraceAttack( pevOwner, 30, pev->origin + pev->velocity, &tr, DMG_GENERIC );
		ApplyMultiDamage( pev, pevOwner );
	}

	SetThink( &CBaseEntity::SUB_Remove );
	pev->nextthink = gpGlobals->time;
}

#include "BMOD_hornetgun.h"

LINK_ENTITY_TO_CLASS( weapon_hornetgun, CHgun )

//=========================================================
// ClearBeams - remove all beams
//=========================================================
void CHgun::ClearBeams()
{
	for( int i = 0; i < HGUN_MAX_BEAMS; i++ )
	{
                if( m_pBeam[i] )
                {
                        UTIL_Remove( m_pBeam[i] );
                        m_pBeam[i] = NULL;
                }
        }
        m_iBeams = 0;

        STOP_SOUND( ENT(m_pPlayer->pev), CHAN_WEAPON, "debris/zap4.wav" );
}

//=========================================================
// ArmBeam - small beam from arm to nearby geometry
//=========================================================

void CHgun::ArmBeam( Vector color )
{
        TraceResult tr;
        float flDist = 1.0;

	if( m_iBeams >= HGUN_MAX_BEAMS )
		return;

        // UTIL_MakeAimVectors( pev->angles );
        Vector vecSrc = m_pPlayer->GetGunPosition() + gpGlobals->v_forward * 16 + gpGlobals->v_right * 8 + gpGlobals->v_up * -12;

	for( int i = 0; i < 3; i++ )
	{
                Vector vecAim = gpGlobals->v_right * RANDOM_FLOAT( -1, 1 ) + gpGlobals->v_up * RANDOM_FLOAT( -1, 1 );
                TraceResult tr1;
                UTIL_TraceLine( vecSrc, vecSrc + vecAim * 512, dont_ignore_monsters, ENT( m_pPlayer->pev ), &tr1 );
                if( flDist > tr1.flFraction )
                {
                        tr = tr1;
                        flDist = tr.flFraction;
                }
        }

        // Couldn't find anything close enough
        if( flDist == 1.0f )
                return;

        // DecalGunshot( &tr, BULLET_PLAYER_CROWBAR );

        m_pBeam[m_iBeams] = CBeam::BeamCreate( "sprites/lgtning.spr", 30 );
        if( !m_pBeam[m_iBeams] )
                return;

        m_pBeam[m_iBeams]->PointEntInit( tr.vecEndPos, m_pPlayer->entindex() );
        m_pBeam[m_iBeams]->SetEndAttachment( 1 );
        // m_pBeam[m_iBeams]->SetColor( 96, 128, 16 );
        m_pBeam[m_iBeams]->SetColor( color.x, color.y, color.z );
        m_pBeam[m_iBeams]->SetBrightness( 64 );
        m_pBeam[m_iBeams]->SetNoise( 80 );
        m_pBeam[m_iBeams]->pev->spawnflags |= SF_BEAM_TEMPORARY;
        m_iBeams++;
}

//=========================================================
// ZapBeam - heavy damage directly forward
//=========================================================
void CHgun::ZapBeam( void )
{
        Vector vecSrc, vecAim, vecOrig;
        TraceResult tr;
        CBaseEntity *pEntity;

	/*
        if (m_iBeams >= HGUN_MAX_BEAMS)
                return;
	*/

	// UTIL_MakeVectors( m_pPlayer->pev->v_angle );
        vecOrig = m_pPlayer->GetGunPosition();
        vecSrc = m_pPlayer->GetGunPosition() + gpGlobals->v_forward * 16 + gpGlobals->v_right * 8 + gpGlobals->v_up * -12;
        vecAim = gpGlobals->v_forward;

        UTIL_TraceLine( vecOrig, vecOrig + vecAim * 2048, dont_ignore_monsters, ENT( m_pPlayer->pev ), &tr );

		for( int i = 0; i < 2; i++ )
		{
			MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
					WRITE_BYTE( TE_BEAMPOINTS );
					WRITE_COORD( vecSrc.x );
					WRITE_COORD( vecSrc.y );
					WRITE_COORD( vecSrc.z );
					WRITE_COORD( tr.vecEndPos.x );
					WRITE_COORD( tr.vecEndPos.y );
					WRITE_COORD( tr.vecEndPos.z );
					WRITE_SHORT( iZapBeamSpr );
					WRITE_BYTE( 0 ); // Starting frame
					WRITE_BYTE( 0  ); // framerate * 0.1
					WRITE_BYTE( 2 ); // life * 0.1
					WRITE_BYTE( 50 ); // width
					WRITE_BYTE( 20 ); // noise
					WRITE_BYTE( 180 ); // color r,g,b
					WRITE_BYTE( 255 ); // color r,g,b
					WRITE_BYTE( 96 ); // color r,g,b
					WRITE_BYTE( 255 ); // brightness
					WRITE_BYTE( 0 ); // scroll speed
			MESSAGE_END();
		}

        UTIL_DecalTrace( &tr, DECAL_SMALLSCORCH1 + RANDOM_LONG( 0, 2 ) );

	/*
        m_pBeam[m_iBeams] = CBeam::BeamCreate( "sprites/lgtning.spr", 50 );
        if( !m_pBeam[m_iBeams] )
                return;
        m_pBeam[m_iBeams]->PointEntInit( tr.vecEndPos, m_pPlayer->entindex() );
        m_pBeam[m_iBeams]->SetEndAttachment( 1 );
        m_pBeam[m_iBeams]->SetColor( 180, 255, 96 );
        m_pBeam[m_iBeams]->SetBrightness( 255 );
        m_pBeam[m_iBeams]->SetNoise( 20 );
        m_pBeam[m_iBeams]->pev->spawnflags |= SF_BEAM_TEMPORARY;
        m_iBeams++;
	*/

        pEntity = CBaseEntity::Instance(tr.pHit);
        if( pEntity != NULL && pEntity->pev->takedamage )
        {
			// pEntity->TakeDamage( pev, VARS (pev->owner), 45, DMG_SHOCK );
		ClearMultiDamage();
		entvars_t *Owner;
		Owner = VARS( pev->owner );
		pEntity->TraceAttack( Owner, 90, vecAim, &tr, DMG_SHOCK | DMG_ALWAYSGIB );
		ApplyMultiDamage( pev, Owner );
	        UTIL_ScreenFade( pEntity, Vector(180,255,96), 2, 0.5, 128, FFADE_IN ); 
        }

        UTIL_EmitAmbientSound( ENT( m_pPlayer->pev ), tr.vecEndPos, "weapons/electro4.wav", 0.5, ATTN_NORM, 0, RANDOM_LONG( 140, 160 ) );

	if( UTIL_PointContents( tr.vecEndPos ) == CONTENTS_WATER )
	{
		CBaseEntity *pRift = CBaseEntity::Create( "zaprift", tr.vecEndPos, UTIL_VecToAngles( tr.vecPlaneNormal ), m_pPlayer->edict() );
		// UTIL_EmitAmbientSound( ENT(m_pPlayer->pev), tr.vecEndPos, "weapons/electro4.wav", 50, ATTN_NORM, 0, 300 );
	}
}

//=========================================================
// Freeze Ray - 
//=========================================================
void CHgun::FreezeRay( void )
{
	Vector vecSrc, vecAim, vecOrig;
	TraceResult tr;
	CBaseEntity *pEntity;

	if( ( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] < 12 ) || m_pPlayer->pev->waterlevel == 3 )
	{
		return;
	}

	vecOrig = m_pPlayer->GetGunPosition();
	vecSrc = m_pPlayer->GetGunPosition() + gpGlobals->v_forward * 16 + gpGlobals->v_right * 8 + gpGlobals->v_up * -12;
	vecAim = gpGlobals->v_forward;
	UTIL_TraceLine( vecOrig, vecOrig + vecAim * 2048, dont_ignore_monsters, ENT( m_pPlayer->pev ), &tr );

	MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
		WRITE_BYTE( TE_BEAMPOINTS );
		WRITE_COORD( vecSrc.x );
		WRITE_COORD( vecSrc.y );
		WRITE_COORD( vecSrc.z );
		WRITE_COORD( tr.vecEndPos.x );
		WRITE_COORD( tr.vecEndPos.y );
		WRITE_COORD( tr.vecEndPos.z );
		WRITE_SHORT( iZapBeamSpr );
		WRITE_BYTE( 0 ); // Starting frame
		WRITE_BYTE( 0 ); // framerate * 0.1
		WRITE_BYTE( 3 ); // life * 0.1
		WRITE_BYTE( 200 ); // width
		WRITE_BYTE( 2 ); // noise
		WRITE_BYTE( 220 ); // color r,g,b
		WRITE_BYTE( 220 ); // color r,g,b
		WRITE_BYTE( 255 ); // color r,g,b
		WRITE_BYTE( 255 ); // brightness
		WRITE_BYTE( 0 ); // scroll speed
	MESSAGE_END();

	UTIL_DecalTrace( &tr, DECAL_MOMMASPLAT );

	MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
		WRITE_BYTE( TE_SPRITETRAIL );// TE_RAILTRAIL);
		WRITE_COORD( vecSrc.x );
		WRITE_COORD( vecSrc.y );
		WRITE_COORD( vecSrc.z );
		WRITE_COORD( tr.vecEndPos.x );
		WRITE_COORD( tr.vecEndPos.y );
		WRITE_COORD( tr.vecEndPos.z );
		WRITE_SHORT( m_sGlowSpr );		// model
		WRITE_BYTE( 20 );				// count
		WRITE_BYTE( 10 );				// life * 10
		WRITE_BYTE( RANDOM_LONG( 1, 2 ) );				// size * 10
		WRITE_BYTE( 10 );				// amplitude * 0.1
		WRITE_BYTE( 0 );				// speed * 100
	MESSAGE_END();

	pEntity = CBaseEntity::Instance( tr.pHit );
	if( pEntity != NULL && pEntity->pev->takedamage && pEntity->IsPlayer() )
	{
		CBasePlayer *pPlayer = (CBasePlayer *)pEntity;
		
		UTIL_ScreenFade( pPlayer, Vector( 0, 0, 255 ), 2.0, 1.0, 128, FFADE_IN ); 

		pPlayer->pev->rendermode = kRenderNormal;
		pPlayer->pev->renderfx = kRenderFxGlowShell;

		pPlayer->pev->rendercolor.x = 240;  // red
		pPlayer->pev->rendercolor.y = 240;  // green
		pPlayer->pev->rendercolor.z = 255; // blue

		pPlayer->pev->renderamt = 60;  // glow shell distance from entity

		float freezetime = Q_max (0.6f, bm_freezetime.value);

		// freeze the player and set the "unfreeze" time...
		pPlayer->EnableControl(FALSE);
		pPlayer->m_vFreezeAngle = pPlayer->pev->v_angle;
		// pPlayer->pev->movetype = MOVETYPE_TOSS;

		pPlayer->m_flFreezeTime = gpGlobals->time + freezetime;
		//RuneMsg( pPlayer, "YOU ARE FROZEN!!!", Vector(100,100,255), freezetime - .5);
		PrintMessage( pPlayer, BMOD_CHAN_RUNE, Vector(100,100,255), Vector (.1, freezetime - 0.5f, .1), "YOU ARE FROZEN!!!");

	}

	UTIL_EmitAmbientSound( ENT(m_pPlayer->pev), tr.vecEndPos, "weapons/electro4.wav", 0.5, ATTN_NORM, 0, RANDOM_LONG( 140, 160 ));

	m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] = 0;
	m_flNextPrimaryAttack = gpGlobals->time + 0.25f;

	m_flRechargeTime = gpGlobals->time + 0.5f;

	m_pPlayer->m_iWeaponVolume = NORMAL_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = DIM_GUN_FLASH;

	SendWeaponAnim( HGUN_SHOOT );

	// player "shoot" animation
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

	m_flTimeWeaponIdle = gpGlobals->time + RANDOM_FLOAT( 10, 15 );
	m_pPlayer->pev->punchangle.x = RANDOM_FLOAT( 0, 2 );
}


//=========================================================
// BeamGlow - brighten all beams
//=========================================================
void CHgun::BeamGlow()
{
        int b = m_iBeams * 32;
        if( b > 255 )
                b = 255;

        for( int i = 0; i < m_iBeams; i++ )
        {
                if( m_pBeam[i]->GetBrightness() != 255 )
                {
                        m_pBeam[i]->SetBrightness( b );
                }
        }
}

BOOL CHgun::IsUseable( void )
{
	return TRUE;
}

void CHgun::Spawn()
{
	Precache();
	m_iId = WEAPON_HORNETGUN;
	SET_MODEL( ENT( pev ), "models/w_hgun.mdl" );

	m_iDefaultAmmo = HIVEHAND_DEFAULT_GIVE;
	m_iFirePhase = 0;
	m_iFireMode = 0;

	FallInit();// get ready to fall down.
}

void CHgun::Precache( void )
{
	PRECACHE_MODEL( "models/v_hgun.mdl" );
	PRECACHE_MODEL( "models/w_hgun.mdl" );
	PRECACHE_MODEL( "models/p_hgun.mdl" );

	m_usHornetFire = PRECACHE_EVENT( 1, "events/firehornet.sc" );

	UTIL_PrecacheOther( "hornet" );
	UTIL_PrecacheOther( "zaprift" );
	UTIL_PrecacheOther( "zapbounce" );
	PRECACHE_SOUND( "debris/zap4.wav" );
	PRECACHE_SOUND( "weapons/electro4.wav" );
	PRECACHE_SOUND( "hassault/hw_shoot1.wav" );
	iZapBeamSpr = PRECACHE_MODEL( "sprites/lgtning.spr" );

	PRECACHE_MODEL( "sprites/bigspit.spr" );
	iBSquidSpitSprite = PRECACHE_MODEL( "sprites/tinyspit.spr" );// client side spittle.
	PRECACHE_SOUND( "bullchicken/bc_acid1.wav" );
	PRECACHE_SOUND( "bullchicken/bc_spithit1.wav" );
	PRECACHE_SOUND( "bullchicken/bc_spithit2.wav" );
	PRECACHE_SOUND( "bullchicken/bc_attack2.wav" );
	PRECACHE_SOUND( "bullchicken/bc_attack3.wav" );
	PRECACHE_SOUND( "leech/leech_bite1.wav" );

	m_sGlowSpr = PRECACHE_MODEL( "sprites/glow04.spr" );
}

int CHgun::AddToPlayer( CBasePlayer *pPlayer )
{
	if( CBasePlayerWeapon::AddToPlayer( pPlayer ) )
	{
#if !CLIENT_DLL
		if( g_pGameRules->IsMultiplayer() )
		{
			// in multiplayer, all hivehands come full. 
			pPlayer->m_rgAmmo[PrimaryAmmoIndex()] = 12;
		}
#endif
		MESSAGE_BEGIN( MSG_ONE, gmsgWeapPickup, NULL, pPlayer->pev );
			WRITE_BYTE( m_iId );
		MESSAGE_END();
		return TRUE;
	}
	return FALSE;
}

int CHgun::GetItemInfo( ItemInfo *p )
{
	p->pszName = STRING( pev->classname );
	p->pszAmmo1 = "Hornets";
	p->iMaxAmmo1 = m_iMaxammo;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = WEAPON_NOCLIP;
	p->iSlot = 3;
	p->iPosition = 3;
	p->iId = m_iId = WEAPON_HORNETGUN;
	p->iFlags = ITEM_FLAG_NOAUTOSWITCHEMPTY | ITEM_FLAG_NOAUTORELOAD;
	p->iWeight = HORNETGUN_WEIGHT;

	return 1;
}

BOOL CHgun::Deploy()
{
	// BMOD Edit - Modified hornet message
	if( bm_hornet_mod.value )
		PrintMessage( m_pPlayer, BMOD_CHAN_WEAPON, Vector( 20, 250, 20 ), Vector( 1, 4, 2 ), "HORNET GUN\nACID SPIT - LIGHTNING - FREEZE RAY - SNARKS\nSECONDARY FIRE: Switches modes." );

	return DefaultDeploy( "models/v_hgun.mdl", "models/p_hgun.mdl", HGUN_UP, "hive" );
}

void CHgun::Holster( int skiplocal /* = 0 */ )
{
	ClearBeams();
	m_iFirePhase = HGUN_IDLE;

	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5f;
	SendWeaponAnim( HGUN_DOWN );

	//!!!HACKHACK - can't select hornetgun if it's empty! no way to get ammo for it, either.
	if( !m_pPlayer->m_rgAmmo[PrimaryAmmoIndex()] )
	{
		m_pPlayer->m_rgAmmo[PrimaryAmmoIndex()] = 1;
	}
}

void CHgun::SecondaryAttack()
{
	if( !bm_hornet_mod.value )
	{
		OldSecondaryAttack();
		return;
	}

	m_iFireMode = ( m_iFireMode + 1 ) % 5;

	switch( m_iFireMode )
	{
	case 0:
		PrintMessage( m_pPlayer, BMOD_CHAN_WEAPON, Vector( 20, 250, 20 ), Vector( 1, 4, 2 ), "\n\nAcid Spit  Mode - 3 per shot");
		break;
	case 1:
		PrintMessage( m_pPlayer, BMOD_CHAN_WEAPON, Vector( 20, 250, 20 ), Vector( 1, 4, 2 ), "\n\nLightning  Mode - 4 per shot");
		break;
	case 2:
		PrintMessage( m_pPlayer, BMOD_CHAN_WEAPON, Vector( 20, 250, 20 ), Vector( 1, 4, 2 ), "\n\nMultizap  Mode - 12 per shot");
		break;
	case 3:
		PrintMessage( m_pPlayer, BMOD_CHAN_WEAPON, Vector( 20, 250, 20 ), Vector( 1, 4, 2 ), "\n\nFreeze Ray Mode - 12 per shot");
		break;
	case 4:
		PrintMessage( m_pPlayer, BMOD_CHAN_WEAPON, Vector( 20, 250, 20 ), Vector( 1, 4, 2 ), "\n\nSnark Launcher Mode - 1 Snark per shot");
		break;
	}

	EMIT_SOUND( ENT( m_pPlayer->pev ), CHAN_WEAPON, "leech/leech_bite1.wav", 1, ATTN_NORM );
	m_flNextSecondaryAttack = gpGlobals->time + 0.5f;
}

void CHgun::ZapGun()
{
	if( m_iFirePhase == HGUN_IDLE )
		Reload();

	if( ( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] < 4 ) || ( m_fNextPhaseTime > gpGlobals->time ) )
	{
		return;
	}

	if( m_pPlayer->pev->waterlevel > 0 )
	{

		// m_pPlayer->TakeDamage( VARS( eoNullEntity ), VARS( eoNullEntity ), 50, DMG_SHOCK );
		UTIL_ScreenFade( this, Vector( 180, 255, 96 ), 2, 0.5, 128, FFADE_IN ); 

		CBaseEntity *pRift = CBaseEntity::Create( "zaprift", pev->origin, pev->angles, ENT( pev ) );

		m_pPlayer->TakeDamage( m_pPlayer->pev, m_pPlayer->pev, 100, DMG_SHOCK );
		EMIT_SOUND_DYN( ENT( m_pPlayer->pev ), CHAN_WEAPON, "debris/zap4.wav", 1, ATTN_NORM, 0, 100 + m_iBeams * 10 );
		m_flNextPrimaryAttack = m_flNextPrimaryAttack + 1.5f;
		return;
	}

	switch( m_iFirePhase )
        {
                case HGUN_IDLE:
                        m_flTimeWeaponIdle = gpGlobals->time;
			m_iFirePhase = HGUN_CHARGE;
			m_fNextPhaseTime = 0;
			m_pPlayer->m_iWeaponVolume = QUIET_GUN_VOLUME;
			m_pPlayer->m_iWeaponFlash = DIM_GUN_FLASH;
                case HGUN_CHARGE:
                        if( m_fNextPhaseTime < gpGlobals->time )
                        {
                                if( m_iBeams < HGUN_MAX_BEAMS )
                                {
                                        ArmBeam( Vector( 96, 128, 16 ) );
                                        BeamGlow();
                                        m_fNextPhaseTime = gpGlobals->time + HGUN_CHARGE_TIME;
                                        EMIT_SOUND_DYN( ENT( m_pPlayer->pev ), CHAN_WEAPON, "debris/zap4.wav", 1, ATTN_NORM, 0,100 + m_iBeams * 10 );
                                }
                                else
				{
                                        m_iFirePhase = HGUN_ZAP;
                                        m_fNextPhaseTime = gpGlobals->time + 0.1f;
				}
			}
			break;
		case HGUN_ZAP:
			ClearBeams();
			ZapBeam();
			m_iFirePhase = HGUN_ZAP_DONE;
			m_fNextPhaseTime = gpGlobals->time + HGUN_ZAP_TIME;
			EMIT_SOUND_DYN( ENT(m_pPlayer->pev), CHAN_WEAPON, "hassault/hw_shoot1.wav", 1, ATTN_NORM, 0, RANDOM_LONG( 130, 160 ) );
			SendWeaponAnim( HGUN_SHOOT );

			// player "shoot" animation
			m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

			m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] -= 4;
			m_flRechargeTime = gpGlobals->time + 0.5f;
			m_pPlayer->pev->punchangle.x = RANDOM_FLOAT( 0, 2 );
			break;
                case HGUN_ZAP_DONE:
			ClearBeams();
			m_flNextPrimaryAttack = gpGlobals->time + 0.25f;
			m_flTimeWeaponIdle = gpGlobals->time + 0.1f;
			break;
	}
}

void CHgun::MultiZapGun()
{
	CZapBounce *pRift = NULL;

	if( m_iFirePhase == HGUN_IDLE )
		Reload();

	if( ( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] < 12 ) || ( m_fNextPhaseTime > gpGlobals->time ) )
	{
		return;
	}

	if( m_pPlayer->pev->waterlevel > 0 )
	{
		UTIL_ScreenFade( this, Vector( 180, 255, 96 ), 2, 0.5, 128, FFADE_IN ); 
		m_pPlayer->TakeDamage( m_pPlayer->pev, m_pPlayer->pev, 100, DMG_SHOCK );
		EMIT_SOUND_DYN( ENT( m_pPlayer->pev ), CHAN_WEAPON, "debris/zap4.wav", 1, ATTN_NORM, 0, 100 + m_iBeams * 10 );
		m_flNextPrimaryAttack = m_flNextPrimaryAttack + 1.5f;
		return;
	}

	switch( m_iFirePhase )
        {
                case HGUN_IDLE:
                        m_flTimeWeaponIdle = gpGlobals->time;
			m_iFirePhase = HGUN_CHARGE;
			m_fNextPhaseTime = 0;
			m_pPlayer->m_iWeaponVolume = QUIET_GUN_VOLUME;
			m_pPlayer->m_iWeaponFlash = DIM_GUN_FLASH;
                case HGUN_CHARGE:
                        if( m_fNextPhaseTime < gpGlobals->time )
                        {
				if( m_iBeams < HGUN_MAX_BEAMS / 2 )
				{
					ArmBeam( Vector( 16, 96, 128 ) );
					BeamGlow();
					m_fNextPhaseTime = gpGlobals->time + HGUN_CHARGE_TIME;
					EMIT_SOUND_DYN( ENT(m_pPlayer->pev), CHAN_WEAPON, "debris/zap4.wav", 1, ATTN_NORM, 0,100 + m_iBeams * 10 );
				}
				else
					m_iFirePhase = HGUN_ZAP;
					m_fNextPhaseTime = gpGlobals->time + 0.1f;
			}
			break;
                case HGUN_ZAP:
                        ClearBeams();
			pRift = (CZapBounce*)CBaseEntity::Create( "zapbounce", 
							m_pPlayer->GetGunPosition( ) + gpGlobals->v_forward * 16 + gpGlobals->v_right * 8 + gpGlobals->v_up * -12, 
							gpGlobals->v_forward, 
							m_pPlayer->edict() );
			if( pRift )
			{
				pRift->m_fDamage = 90;
				pRift->m_iBounce = 5;
				pRift->pentIgnore = ENT( m_pPlayer->pev );
				pRift->m_bFirstZap = TRUE;
				pRift->BounceThink();
			}
                        m_iFirePhase = HGUN_ZAP_DONE;
                        m_fNextPhaseTime = gpGlobals->time + HGUN_ZAP_TIME;
                        EMIT_SOUND_DYN( ENT( m_pPlayer->pev ), CHAN_WEAPON, "hassault/hw_shoot1.wav", 1, ATTN_NORM, 0, RANDOM_LONG( 130, 160 ) );
                        SendWeaponAnim( HGUN_SHOOT );
                        // player "shoot" animation
                        m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

                        m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] -= 12;
                        m_flRechargeTime = gpGlobals->time + 0.5f;
			m_pPlayer->pev->punchangle.x = RANDOM_FLOAT( 0, 2 );
                        break;
                case HGUN_ZAP_DONE:
                        ClearBeams();
                        m_flNextPrimaryAttack = gpGlobals->time + 0.25f;
                        m_flTimeWeaponIdle = gpGlobals->time + 0.1f;
                        break;
        }
}

void CHgun::PrimaryAttack( void )
{
	if( !bm_hornet_mod.value )
	{
		OldPrimaryAttack();
		return;
	}

	Reload();

	switch( m_iFireMode )
	{
	case 0:
		SquidSpit();
		break;
	case 1:
		ZapGun();
		break;
	case 2:
		MultiZapGun();
		break;
	case 3:
		FreezeRay();
		break;
	case 4:
		LaunchSnark();
		break;
	}
}

void CHgun::SquidSpit( void )
{
	if( ( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] < 3 ) || ( m_iFirePhase != HGUN_IDLE ) )
	{
		return;
	}

	Vector  vecSrc, vecDir, vecOrig;
	int iSpittle, iSpitSpeed;
	TraceResult tr;

	// UTIL_MakeVectors( m_pPlayer->pev->v_angle );

        // Vector anglesAim = m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle;
        // UTIL_MakeVectors( anglesAim );

	vecOrig = m_pPlayer->GetGunPosition();
	vecSrc = m_pPlayer->GetGunPosition() + gpGlobals->v_forward * 16 + gpGlobals->v_right * 8 + gpGlobals->v_up * -12;
	// vecDir = m_pPlayer->pev->v_angle;
	// vecDir = m_pPlayer->GetAutoaimVector( AUTOAIM_5DEGREES );
	vecDir = gpGlobals->v_forward;
        UTIL_TraceLine( vecOrig, vecOrig + vecDir * 2048, dont_ignore_monsters, ENT( m_pPlayer->pev ), &tr );
	vecDir = ( tr.vecEndPos - vecSrc ).Normalize();

	if( m_pPlayer->pev->waterlevel == 3 )
	{
		iSpittle = 15;	
		iSpitSpeed = 120;
	}
	else
	{
		// CSquidSpit::Shoot( m_pPlayer->edict(), vecSrc, vecDir * 900 );
		// CBaseEntity *pSpit = CBaseEntity::Create( "squidspit", vecSrc, vecDir , pev->owner );
		BMODSquidSpit::Shoot( m_pPlayer->pev, vecSrc, vecDir * 1100 );
		iSpittle = 5;
		iSpitSpeed = 80;
	}

	m_flNextPrimaryAttack = gpGlobals->time + 0.25f;

	// spew the spittle temporary ents.
	MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, vecSrc );
		WRITE_BYTE( TE_SPRITE_SPRAY );
		WRITE_COORD( vecSrc.x );	// pos
		WRITE_COORD( vecSrc.y );
		WRITE_COORD( vecSrc.z );
		WRITE_COORD( vecDir.x );	// dir
		WRITE_COORD( vecDir.y );
		WRITE_COORD( vecDir.z );
		WRITE_SHORT( iBSquidSpitSprite );	// model
		WRITE_BYTE( iSpittle );			// count
		WRITE_BYTE( iSpitSpeed );		// speed
		WRITE_BYTE( 25 );		// noise ( client will divide by 100 )
	MESSAGE_END();

        switch( RANDOM_LONG( 0, 1 ) )
        {
        case 0:
                EMIT_SOUND( ENT( m_pPlayer->pev ), CHAN_WEAPON, "bullchicken/bc_attack2.wav", 1, ATTN_NORM );
                break;
        case 1:
                EMIT_SOUND( ENT( m_pPlayer->pev ), CHAN_WEAPON, "bullchicken/bc_attack3.wav", 1, ATTN_NORM );
                break;
        }

	m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] -= 3;	
	m_flRechargeTime = gpGlobals->time + 0.5f;

	m_pPlayer->m_iWeaponVolume = NORMAL_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = DIM_GUN_FLASH;

	SendWeaponAnim( HGUN_SHOOT );

	// player "shoot" animation
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

	m_flTimeWeaponIdle = gpGlobals->time + RANDOM_FLOAT ( 10, 15 );
	m_pPlayer->pev->punchangle.x = RANDOM_FLOAT( 0, 2 );
}

void CHgun::LaunchSnark( void )
{
	if( ( m_pPlayer->m_rgAmmo[m_pPlayer->GetAmmoIndex( "Snarks" )] < 1 ) || ( m_iFirePhase != HGUN_IDLE ) )
	{
		return;
	}

	Vector  vecSrc, vecDir, vecOrig;
	int iSpittle, iSpitSpeed;
	TraceResult tr;

	vecOrig = m_pPlayer->GetGunPosition();
	vecSrc = m_pPlayer->GetGunPosition() + gpGlobals->v_forward * 16 + gpGlobals->v_right * 8 + gpGlobals->v_up * -12;
	// vecDir = m_pPlayer->pev->v_angle;
	// vecDir = m_pPlayer->GetAutoaimVector( AUTOAIM_5DEGREES );
	vecDir = gpGlobals->v_forward;
	UTIL_TraceLine( vecSrc + gpGlobals->v_forward * 20, vecSrc + gpGlobals->v_forward * 40, dont_ignore_monsters, NULL, &tr );
	vecDir = ( tr.vecEndPos - vecSrc ).Normalize();

	if( m_pPlayer->pev->waterlevel == 3 )
	{
		iSpittle = 15;	
		iSpitSpeed = 120;
	}
	else
	{
		CBaseEntity *pSqueak = CBaseEntity::Create( "monster_snark", tr.vecEndPos, m_pPlayer->pev->v_angle, m_pPlayer->edict() );
		pSqueak->pev->velocity = gpGlobals->v_forward * 1500 + m_pPlayer->pev->velocity;
		( (CSqueakGrenade*)pSqueak )->m_bWasLaunched = TRUE;
		EMIT_SOUND_DYN( ENT( pSqueak->pev ), CHAN_VOICE, "squeek/sqk_hunt2.wav", 1, ATTN_NORM, 0, 105 );

		iSpittle = 5;
		iSpitSpeed = 80;
	}

	m_flNextPrimaryAttack = gpGlobals->time + 0.4f;

	// spew the spittle temporary ents.
	MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, vecSrc );
		WRITE_BYTE( TE_SPRITE_SPRAY );
		WRITE_COORD( vecSrc.x );		// pos
		WRITE_COORD( vecSrc.y );
		WRITE_COORD( vecSrc.z );
		WRITE_COORD( vecDir.x );     // dir
		WRITE_COORD( vecDir.y );
		WRITE_COORD( vecDir.z );
		WRITE_SHORT( iBSquidSpitSprite );	// model
		WRITE_BYTE( iSpittle );		// count
		WRITE_BYTE( iSpitSpeed );	// speed
		WRITE_BYTE( 25 );		// noise ( client will divide by 100 )
	MESSAGE_END();

	switch( RANDOM_LONG( 0, 1 ) )
	{
	case 0:
		EMIT_SOUND( ENT( m_pPlayer->pev ), CHAN_WEAPON, "bullchicken/bc_attack2.wav", 1, ATTN_NORM );
        	break;
	case 1:
        	EMIT_SOUND( ENT( m_pPlayer->pev ), CHAN_WEAPON, "bullchicken/bc_attack3.wav", 1, ATTN_NORM );
        	break;
	}

	m_pPlayer->m_rgAmmo[m_pPlayer->GetAmmoIndex( "Snarks" )] -= 1;	
	m_flRechargeTime = gpGlobals->time + 0.5f;

	m_pPlayer->m_iWeaponVolume = NORMAL_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = DIM_GUN_FLASH;

	SendWeaponAnim( HGUN_SHOOT );

	// player "shoot" animation
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

	m_flTimeWeaponIdle = gpGlobals->time + RANDOM_FLOAT( 10, 15 );
	m_pPlayer->pev->punchangle.x = RANDOM_FLOAT( 0, 2 );
}

void CHgun::OldPrimaryAttack()
{
	Reload();

	if( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 )
	{
		return;
	}
#if !CLIENT_DLL
	UTIL_MakeVectors( m_pPlayer->pev->v_angle );

	CBaseEntity *pHornet = CBaseEntity::Create( "hornet", m_pPlayer->GetGunPosition() + gpGlobals->v_forward * 16.0f + gpGlobals->v_right * 8.0f + gpGlobals->v_up * -12.0f, m_pPlayer->pev->v_angle, m_pPlayer->edict() );
	pHornet->pev->velocity = gpGlobals->v_forward * 300.0f;

	m_flRechargeTime = gpGlobals->time + 0.5f;
#endif
	m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType]--;
	
	m_pPlayer->m_iWeaponVolume = QUIET_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = DIM_GUN_FLASH;

	int flags;
#if CLIENT_WEAPONS
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif
	PLAYBACK_EVENT_FULL( flags, m_pPlayer->edict(), m_usHornetFire, 0.0f, g_vecZero, g_vecZero, 0.0f, 0.0f, 0, 0, 0, 0 );

	// player "shoot" animation
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

	m_flNextPrimaryAttack = m_flNextPrimaryAttack + 0.25f;

	if( m_flNextPrimaryAttack < UTIL_WeaponTimeBase() )
	{
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.25f;
	}

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
}

void CHgun::OldSecondaryAttack( void )
{
	Reload();

	if( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 )
	{
		return;
	}

	//Wouldn't be a bad idea to completely predict these, since they fly so fast...
#if !CLIENT_DLL
	CBaseEntity *pHornet;
	Vector vecSrc;

	UTIL_MakeVectors( m_pPlayer->pev->v_angle );

	vecSrc = m_pPlayer->GetGunPosition() + gpGlobals->v_forward * 16.0f + gpGlobals->v_right * 8.0f + gpGlobals->v_up * -12.0f;

	m_iFirePhase++;
	switch( m_iFirePhase )
	{
	case 1:
		vecSrc = vecSrc + gpGlobals->v_up * 8.0f;
		break;
	case 2:
		vecSrc = vecSrc + gpGlobals->v_up * 8.0f;
		vecSrc = vecSrc + gpGlobals->v_right * 8.0f;
		break;
	case 3:
		vecSrc = vecSrc + gpGlobals->v_right * 8.0f;
		break;
	case 4:
		vecSrc = vecSrc + gpGlobals->v_up * -8.0f;
		vecSrc = vecSrc + gpGlobals->v_right * 8.0f;
		break;
	case 5:
		vecSrc = vecSrc + gpGlobals->v_up * -8.0f;
		break;
	case 6:
		vecSrc = vecSrc + gpGlobals->v_up * -8.0f;
		vecSrc = vecSrc + gpGlobals->v_right * -8.0f;
		break;
	case 7:
		vecSrc = vecSrc + gpGlobals->v_right * -8.0f;
		break;
	case 8:
		vecSrc = vecSrc + gpGlobals->v_up * 8.0f;
		vecSrc = vecSrc + gpGlobals->v_right * -8.0f;
		m_iFirePhase = 0;
		break;
	}

	pHornet = CBaseEntity::Create( "hornet", vecSrc, m_pPlayer->pev->v_angle, m_pPlayer->edict() );
	pHornet->pev->velocity = gpGlobals->v_forward * 1200.0f;
	pHornet->pev->angles = UTIL_VecToAngles( pHornet->pev->velocity );

	pHornet->SetThink( &CHornet::StartDart );

	m_flRechargeTime = gpGlobals->time + 0.5f;
#endif
	int flags;
#if CLIENT_WEAPONS
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif
	PLAYBACK_EVENT_FULL( flags, m_pPlayer->edict(), m_usHornetFire, 0.0f, g_vecZero, g_vecZero, 0.0f, 0.0f, 0, 0, 0, 0 );

	m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType]--;
	m_pPlayer->m_iWeaponVolume = NORMAL_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = DIM_GUN_FLASH;

	// player "shoot" animation
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

	m_flNextPrimaryAttack = m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.1f;
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10.0f, 15.0f );
}

void CHgun::Reload( void )
{
	m_iMaxammo = (bm_hornet_mod.value) ? 12 : 8;
	if( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] >= m_iMaxammo )
		return;

	while( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] < m_iMaxammo && m_flRechargeTime < gpGlobals->time )
	{
		m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType]++;
		m_flRechargeTime += 0.5f;
	}
}

void CHgun::WeaponIdle( void )
{
	Reload();
	m_iFirePhase = HGUN_IDLE;
	if( m_iBeams )
		ClearBeams();

	if( m_flTimeWeaponIdle > UTIL_WeaponTimeBase() )
		return;

	int iAnim;
	float flRand = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 0.0f, 1.0f );
	if( flRand <= 0.75f )
	{
		iAnim = HGUN_IDLE1;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 30.0f / 16.0f * 2.0f;
	}
	else if( flRand <= 0.875f )
	{
		iAnim = HGUN_FIDGETSWAY;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 40.0f / 16.0f;
	}
	else
	{
		iAnim = HGUN_FIDGETSHAKE;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 35.0f / 16.0f;
	}
	SendWeaponAnim( iAnim );
}
#endif
