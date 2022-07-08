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
#include "soundent.h"
#include "effects.h"
#include "rpg.h"
#include "shake.h"
 
extern int gmsgRpgViseur;
extern int gmsgRpgMenu;
extern int gmsgClientDecal;
extern int gEvilImpulse101;



#if !CLIENT_DLL

extern short g_sModelIndexBlastCircle;
extern void EnvSmokeCreate( const Vector &center, int m_iScale, float m_fFrameRate, int m_iTime, int m_iEndTime );

//------------------------------------------
//
// Sauvegardes
//
//------------------------------------------

// CRpg

LINK_ENTITY_TO_CLASS( weapon_rpg, CRpg );

TYPEDESCRIPTION	CRpg::m_SaveData[] = 
{
	DEFINE_FIELD( CRpg, m_cActiveRockets, FIELD_INTEGER ),
	DEFINE_FIELD( CRpg, m_pEntityTarget, FIELD_CLASSPTR ),
	DEFINE_FIELD( CRpg, m_pEntityLocked, FIELD_CLASSPTR ),
	DEFINE_FIELD( CRpg, m_flLockTime, FIELD_TIME ),
	DEFINE_FIELD( CRpg, m_iAmmoType, FIELD_INTEGER ),
	DEFINE_FIELD( CRpg, m_iAmmoRocket, FIELD_INTEGER ),
	DEFINE_FIELD( CRpg, m_iAmmoElectro, FIELD_INTEGER ),
	DEFINE_FIELD( CRpg, m_iAmmoNuclear, FIELD_INTEGER ),
	DEFINE_FIELD( CRpg, m_iMenuState, FIELD_INTEGER ),
	DEFINE_FIELD( CRpg, m_bLoaded, FIELD_BOOLEAN ),
	DEFINE_FIELD( CRpg, m_bRpgUpdate, FIELD_BOOLEAN ),
	DEFINE_FIELD( CRpg, m_flReloadTime, FIELD_FLOAT ),
	DEFINE_FIELD( CRpg, m_flLastBip, FIELD_TIME ),
};


// CRpgRocket

LINK_ENTITY_TO_CLASS( rpg_rocket, CRpgRocket );

TYPEDESCRIPTION	CRpgRocket::m_SaveData[] = 
{
	DEFINE_FIELD( CRpgRocket, m_flIgniteTime, FIELD_TIME ),
	DEFINE_FIELD( CRpgRocket, m_pTargetMonster, FIELD_CLASSPTR ),
	DEFINE_FIELD( CRpgRocket, m_iRocketType, FIELD_INTEGER ),
	DEFINE_FIELD( CRpgRocket, m_flDiskTime, FIELD_TIME ),
	DEFINE_FIELD( CRpgRocket, m_flLastRadius, FIELD_FLOAT ),

};
IMPLEMENT_SAVERESTORE( CRpgRocket, CGrenade );

//-------------------------------------------------
//------------------------------------------------
//
// CRpgRocket
//
//-------------------------------------------------
//-------------------------------------------------
 
 
//------------------------------------------------
//
// Creation de la roquette
//
//-------------------------------------------------
CRpgRocket *CRpgRocket::CreateRpgRocket( Vector vecOrigin, Vector vecAngles, CBaseEntity *pOwner, CRpg *pLauncher )
{
	CRpgRocket *pRocket = GetClassPtr( (CRpgRocket *)NULL );

	UTIL_SetOrigin( pRocket->pev, vecOrigin );
	pRocket->pev->angles = vecAngles;

	if ( pLauncher != NULL ) //modif de Julien
	{
		pRocket->m_pLauncher = pLauncher;// remember what RPG fired me. 
		pRocket->m_iRocketType = pLauncher->m_iAmmoType;
	}
	else
		pRocket->m_pLauncher = NULL;

	pRocket->Spawn();
	pRocket->SetTouch( &CRpgRocket::RocketTouch );

	if ( pOwner != NULL )
		pRocket->pev->owner = pOwner->edict();
	else
		pRocket->pev->owner = NULL;

	return pRocket;
}

//=========================================================
//=========================================================
void CRpgRocket::Spawn( void )
{
	Precache();

	pev->movetype = MOVETYPE_BOUNCE;
	pev->solid = SOLID_BBOX;

	// model

	if ( m_iRocketType == AMMO_ELECTRO )
		SET_MODEL(ENT(pev), "models/rpg_electrocket.mdl" );
	else if ( m_iRocketType == AMMO_NUCLEAR )
		SET_MODEL(ENT(pev), "models/rpg_nuclearrocket.mdl" );
	else
		SET_MODEL(ENT(pev), "models/rpgrocket.mdl");

	UTIL_SetSize( pev, Vector( 0, 0, 0 ), Vector( 0, 0, 0 ) );
	UTIL_SetOrigin( pev, pev->origin );

	pev->classname = MAKE_STRING( "rpg_rocket" );

	SetThink( &CRpgRocket::IgniteThink );
	pev->nextthink = gpGlobals->time + 0.1;		//modif de Julien : 0.4

	pev->angles.x -= 30.0f;
	UTIL_MakeVectors( pev->angles );
	pev->angles.x = -( pev->angles.x + 30.0f );

	pev->velocity = gpGlobals->v_forward * 250.0f;
	pev->gravity = 0.5f;


	pev->dmg = gSkillData.plrDmgRPG;

	pev->takedamage		= DAMAGE_NO;	// fix bug
}


void CRpgRocket :: Precache( void )
{
	PRECACHE_MODEL("models/rpgrocket.mdl");
	PRECACHE_MODEL("models/rpg_electrocket.mdl");
	PRECACHE_MODEL("models/rpg_nuclearrocket.mdl");

	PRECACHE_SOUND ("weapons/rocket1.wav");
	PRECACHE_SOUND ( "weapons/mortarhit.wav" );
	PRECACHE_SOUND ( "weapons/rpg_electrocket.wav" );

	m_iTrail = PRECACHE_MODEL("sprites/smoke.spr");
	m_iDiskTexture = PRECACHE_MODEL("sprites/rpg_disk.spr");
	m_sNuclearSprite = PRECACHE_MODEL("sprites/fexplo.spr");
}


//------------------------------------------------
//
// Explosion de la roquette
//
//-------------------------------------------------


void CRpgRocket :: RocketTouch ( CBaseEntity *pOther ) //modif de Julien
{


	//modif de Julien
	if ( m_pTargetMonster != NULL && m_pLauncher && m_pLauncher->m_cActiveRockets == 1 )
	{
		if ( pOther == m_pTargetMonster )
			m_pLauncher->UpdateCrosshair ( RPG_TEXT_TOUCHE );
		else
			m_pLauncher->UpdateCrosshair ( RPG_TEXT_MANQUE );

		m_pLauncher->m_cActiveRockets = 0;
	}

	// enflamme le gaz

	if ( IsInGaz() == TRUE )
 	{
		edict_t *pFind = FIND_ENTITY_BY_CLASSNAME( NULL, "player" );
		CBaseEntity *pPlayer = CBaseEntity::Instance ( pFind );
		pPlayer->m_bFireInGaz = TRUE;
	}


	switch ( m_iRocketType )
	{
	case AMMO_ROCKET:
		{
			pev->origin = pev->origin - pev->velocity * 0.05;		// revient un peu en arriere pour ne pas exploser du sol
			ExplodeTouch( pOther );
			break;
		}

	case AMMO_ELECTRO:
		{
			// truc affreux pour creer une particule a travers ce message pour les decals

			MESSAGE_BEGIN( MSG_ALL, gmsgClientDecal );

				WRITE_COORD( pev->origin.x );			// xyz source
				WRITE_COORD( pev->origin.y );
				WRITE_COORD( pev->origin.z );
				WRITE_COORD( 0 );						// xyz norme
				WRITE_COORD( 0 );
				WRITE_COORD( 0 );
				WRITE_CHAR ( 'a' );						// type de texture
				WRITE_BYTE ( 4 );						//  4 == electro-rocket

			MESSAGE_END();

			SetThink ( &CRpgRocket::ElectroThink );
			pev->nextthink	= gpGlobals->time + 0.1;
			m_flDiskTime	= gpGlobals->time;
			m_flLastRadius	= 0.0;						// pour coller avec la dll client

			pev->effects = 0;

			UTIL_ScreenFadeAll ( Vector ( 215, 225, 255 ), 0.3, 0, 80, FFADE_IN );

			// fait du bruit pour r

			EMIT_SOUND(ENT(pev), CHAN_WEAPON, "weapons/rpg_electrocket.wav", 1, ATTN_NONE);
			

			break;
		}

	case AMMO_NUCLEAR:
		{
			// explosion

			MESSAGE_BEGIN( MSG_PAS, SVC_TEMPENTITY, pev->origin );
				WRITE_BYTE( TE_EXPLOSION );		// This makes a dynamic light and the explosion sprites/sound
				WRITE_COORD( pev->origin.x );	// Send to PAS because of the sound
				WRITE_COORD( pev->origin.y );
				WRITE_COORD( pev->origin.z + 200 );
				WRITE_SHORT( m_sNuclearSprite );
				WRITE_BYTE( 100  ); // scale * 10
				WRITE_BYTE( 10  ); // framerate
				WRITE_BYTE( TE_EXPLFLAG_NONE );
			MESSAGE_END();

			// cercles

			MESSAGE_BEGIN( MSG_PAS, SVC_TEMPENTITY, pev->origin );
				WRITE_BYTE( TE_BEAMCYLINDER );
				WRITE_COORD( pev->origin.x);
				WRITE_COORD( pev->origin.y);
				WRITE_COORD( pev->origin.z);
				WRITE_COORD( pev->origin.x);
				WRITE_COORD( pev->origin.y);
				WRITE_COORD( pev->origin.z + 600 * 5 ); // reach damage radius over .3 seconds
				WRITE_SHORT( g_sModelIndexBlastCircle );
				WRITE_BYTE( 0 ); // startframe
				WRITE_BYTE( 0 ); // framerate
				WRITE_BYTE( 3 ); // life
				WRITE_BYTE( 128 );  // width
				WRITE_BYTE( 0 );   // noise
				WRITE_BYTE( 255   );	// rgb
				WRITE_BYTE( 255 );
				WRITE_BYTE( 200  );
				WRITE_BYTE( 170 ); //brightness
				WRITE_BYTE( 0 );		// speed
			MESSAGE_END();
			MESSAGE_BEGIN( MSG_PAS, SVC_TEMPENTITY, pev->origin );
				WRITE_BYTE( TE_BEAMCYLINDER );
				WRITE_COORD( pev->origin.x);
				WRITE_COORD( pev->origin.y);
				WRITE_COORD( pev->origin.z + 32 );
				WRITE_COORD( pev->origin.x);
				WRITE_COORD( pev->origin.y);
				WRITE_COORD( pev->origin.z - 600 * 5 / 2 ); // reach damage radius over .3 seconds
				WRITE_SHORT( g_sModelIndexBlastCircle );
				WRITE_BYTE( 0 ); // startframe
				WRITE_BYTE( 0 ); // framerate
				WRITE_BYTE( 4 ); // life
				WRITE_BYTE( 48 );  // width
				WRITE_BYTE( 0 );   // noise
				WRITE_BYTE( 255   );	// rgb
				WRITE_BYTE( 255 );
				WRITE_BYTE( 200  );
				WRITE_BYTE( 170 ); //brightness
				WRITE_BYTE( 0 );		// speed
			MESSAGE_END();


			EMIT_SOUND(ENT(pev), CHAN_WEAPON, "weapons/mortarhit.wav", 1, ATTN_NONE);


			UTIL_ScreenFadeAll ( Vector ( 220, 220, 220 ), 0.2, 0.3, 130, FFADE_IN );
			UTIL_ScreenShakeAll ( pev->origin, 200, 100, 3 );

			CSoundEnt::InsertSound ( bits_SOUND_COMBAT, pev->origin, BIG_EXPLOSION_VOLUME, 3.0 );

			// dommages

			float flRadius = 1000;

			CBaseEntity *pEntity = NULL;
			float flDamage = 400;

			while ((pEntity = UTIL_FindEntityInSphere( pEntity, pev->origin, flRadius )) != NULL)
			{
				if ( pEntity->pev->takedamage != DAMAGE_NO )
				{
					Vector vecSpot = pEntity->Center();
					TraceResult tr;
					UTIL_TraceLine ( pev->origin, vecSpot, dont_ignore_monsters, ENT(pev), &tr );

					if ( tr.flFraction != 1.0 && tr.pHit != pEntity->edict() )
					{
						flDamage = flDamage * (pev->origin - vecSpot).Length() /  3 * flRadius;
					}

					pEntity->TakeDamage ( pev, pev, flDamage, DMG_RADIATION );
				}
			}

			SetThink ( &CBaseEntity::SUB_Remove );
			pev->nextthink = gpGlobals->time + 0.1;


			// d

			CBaseEntity *pTrigger = UTIL_FindEntityByClassname ( NULL, "trigger_nuclear" );

			while ( pTrigger != NULL )
			{

				Vector org = pev->origin, min = pTrigger->pev->absmin, max = pTrigger->pev->absmax;

				if (( org.x > min.x && org.x < max.x ) &&
					( org.y > min.y && org.y < max.y ) &&
					( org.z > min.z && org.z < max.z ) )
				{
					FireTargets ( STRING(pTrigger->pev->target), this, this, USE_ON, 0 );
				}

				pTrigger = UTIL_FindEntityByClassname ( pTrigger, "trigger_nuclear" );
			}



			break;
		}
 	}
 
	pev->movetype	= MOVETYPE_NOCLIP;			// bug de la trainee de fumee rebondissante
	pev->solid		= SOLID_NOT;
	pev->velocity	= Vector ( 0,0,0 );

 	STOP_SOUND( edict(), CHAN_VOICE, "weapons/rocket1.wav" );
 }


int CRpgRocket :: TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{	return 1;	}

void CRpgRocket :: Killed( entvars_t *pevAttacker, int iGib )
{	};


void CRpgRocket :: ElectroThink ( void )
{
	if ( m_flLastRadius > ELECTRO_DISK_MAX )
	{
		SetThink ( &CBaseEntity::SUB_Remove );
		pev->nextthink = gpGlobals->time + 0.1;
		return;
	}

	pev->nextthink = gpGlobals->time + 0.1;

	float flRadius = ( gpGlobals->time - m_flDiskTime ) * ELECTRO_DISK_SPEED + 1;	// +1 pour coller avec la dll client

	CBaseEntity *pEntity = NULL;
	float flDamage = 100;

	while ((pEntity = UTIL_FindEntityInSphere( pEntity, pev->origin, flRadius )) != NULL)
	{
		if ( pEntity->pev->takedamage != DAMAGE_NO )
		{
			Vector2D vecSpot = ( pev->origin - pEntity->Center() ).Make2D();

			if ( vecSpot.Length() > m_flLastRadius &&
				 (
				 ( pEntity->pev->solid == SOLID_BSP && pEntity->pev->mins.z <= pev->origin.z && pEntity->pev->maxs.z >= pev->origin.z )
				 ||
				 ( pEntity->pev->solid != SOLID_BSP && pEntity->pev->absmin.z <= pev->origin.z && pEntity->pev->absmax.z >= pev->origin.z )
				 )
			   )

			{
				pEntity->TakeDamage ( pev, pev, flDamage, DMG_SHOCK );

				if ( pEntity->IsPlayer () )
				{
					short sens = RANDOM_LONG ( 0, 1 );
					sens = sens == 0 ? -1 : 1;
					pEntity->pev->punchangle.y = 25 * sens;
					pEntity->pev->punchangle.x = 25 * sens;
				}
			}	
		}
	}

	m_flLastRadius = flRadius;
}

//------------------------------------------------
//
// Mouvement de la roquette
//
//-------------------------------------------------


void CRpgRocket::IgniteThink( void )
{

	pev->movetype = MOVETYPE_FLY;

	if ( m_iRocketType == AMMO_ROCKET )
		pev->effects |= EF_LIGHT;


	// make rocket sound
	EMIT_SOUND( ENT( pev ), CHAN_VOICE, "weapons/rocket1.wav", 1, 0.5f );

	if ( m_iRocketType == AMMO_ROCKET || m_iRocketType == AMMO_ELECTRO )
	{
		// rocket trail
		MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
			WRITE_BYTE( TE_BEAMFOLLOW );
			WRITE_SHORT(entindex());	// entity
			WRITE_SHORT(m_iTrail );	// model
			WRITE_BYTE( 40 ); // life
			WRITE_BYTE( 5 );  // width
			WRITE_BYTE( 224 );   // r, g, b
			WRITE_BYTE( 224 );   // r, g, b
			WRITE_BYTE( 255 );   // r, g, b
 
			if ( m_iRocketType == AMMO_ROCKET )
				WRITE_BYTE( 255 );	// brightness
			else
				WRITE_BYTE( 100 );	// brightness
 
		MESSAGE_END();  // move PHS/PVS data sending into here (SEND_ALL, SEND_PVS, SEND_PHS)
	}


	m_flIgniteTime = gpGlobals->time;

	// set to follow laser spot
	SetThink( &CRpgRocket::FollowThink );
	pev->nextthink = gpGlobals->time + 0.1f;
}

void CRpgRocket::FollowThink( void )
{
	CBaseEntity *pOther = NULL;
	Vector vecTarget;
	Vector vecDir;
	Vector angDir;
	TraceResult tr;

	UTIL_MakeAimVectors( pev->angles );

	vecTarget = gpGlobals->v_forward;

	
	if ( m_pTargetMonster != NULL )
	{
		pOther = m_pTargetMonster;	
		UTIL_TraceLine ( pev->origin, pOther->Center(), dont_ignore_monsters, ENT(pev), &tr );

		if( tr.flFraction >= 0.9f )
		{
			vecDir = ( pOther->Center() - pev->origin ).Normalize();
			angDir = UTIL_VecToAngles( vecDir );


			if ( (CBaseEntity::Instance(pev->owner)) ->IsPlayer() )
				vecTarget = vecDir/* * 100*/;

			else
			{
				float flAngDistX = UTIL_AngleDiff ( angDir.x, pev->angles.x );
				float flAngDistY = UTIL_AngleDiff ( angDir.y, pev->angles.y );

				if ( fabs(flAngDistX) < 45 && fabs(flAngDistY) < 45 )
				{

					if ( fabs(flAngDistX) <= 9 && fabs(flAngDistY) <= 9 )
					{
						vecTarget = vecDir;
					}
					else
					{
						angDir = pev->angles;
						int iSens = flAngDistX > 0 ? 1 : -1;
						angDir.x = angDir.x + flAngDistX / 2 * iSens;
						iSens = flAngDistY > 0 ? 1 : -1;
						angDir.y = angDir.y + flAngDistY / 2 * iSens;

						UTIL_MakeVectors( angDir );
						vecTarget = gpGlobals->v_forward;
					}
				}
				else
				{
					m_pTargetMonster = NULL;
				}
			}
		}
	}

	pev->angles = UTIL_VecToAngles( vecTarget );

	// this acceleration and turning math is totally wrong, but it seems to respond well so don't change it.
	float flSpeed = pev->velocity.Length();
	if( gpGlobals->time - m_flIgniteTime < 1.0f )
	{
		pev->velocity = pev->velocity * 0.2f + vecTarget * ( flSpeed * 0.8f + 400.0f );
		if( pev->waterlevel == 3 )
		{
			// go slow underwater
			if( pev->velocity.Length() > 300.0f )
			{
				pev->velocity = pev->velocity.Normalize() * 300.0f;
			}
			UTIL_BubbleTrail( pev->origin - pev->velocity * 0.1f, pev->origin, 4 );
		} 
		else 
		{
			if( pev->velocity.Length() > 2000.0f )
			{
				pev->velocity = pev->velocity.Normalize() * 2000.0f;
			}
		}
	}
	else
	{
		if( pev->effects & EF_LIGHT )
		{
			pev->effects = 0;
			STOP_SOUND( ENT( pev ), CHAN_VOICE, "weapons/rocket1.wav" );
		}
		pev->velocity = pev->velocity * 0.2f + vecTarget * flSpeed * 0.798f;
		if( pev->waterlevel == 0 && pev->velocity.Length() < 1500.0f )
		{
			//if( CRpg *pLauncher = (CRpg*)( (CBaseEntity*)( m_hLauncher ) ) )
			if ( m_pLauncher ) //modif de Julien o para Julien, LOL :D
			{
				// my launcher is still around, tell it I'm dead.
				m_pLauncher->m_cActiveRockets--; //lacked m_ before pLauncher
			}
			Detonate();
		}
	}

	//modif de JUlien

	if ( CBaseEntity::Instance( pev->owner )->IsPlayer() )
		CSoundEnt::InsertSound ( bits_SOUND_DANGER, pev->origin, pev->velocity.Length( ), 0.1 );

	pev->nextthink = gpGlobals->time + 0.1f;
}
#endif


//-------------------------------------------------
//-------------------------------------------------
//------------------------------------------------
//
// CRpg
//
//-------------------------------------------------
//-------------------------------------------------
//-------------------------------------------------
 
 
 

void CRpg::Spawn( )
{
	Precache( );
	m_iId = WEAPON_RPG;
	SET_MODEL(ENT(pev), "models/w_rpg.mdl");
 

	m_iClip = 1;				// empeche le declenchement du rechargement par la classe mere
 

	m_iAmmoType = AMMO_ROCKET;
	m_bLoaded = TRUE;			//pas de rechargement la premiere fois
	m_flReloadTime = -1;		// -1 = pas en cours de rechargement
	pev->body = RPG_WEAPON_ROCKET;
 

	m_iMenuState = 0;
	m_iMenuState |= ( RPG_MENU_ROCKET_SELECTED | RPG_MENU_ROCKET_EMPTY | RPG_MENU_ELECTRO_EMPTY | RPG_MENU_NUCLEAR_EMPTY );
 
	m_iDefaultAmmo = RPG_DEFAULT_GIVE;
 
	m_flLastBip = 0;

 
 	FallInit();// get ready to fall down.
}

void CRpg::Precache( void )
{
	PRECACHE_MODEL( "models/w_rpg.mdl" );
	PRECACHE_MODEL( "models/v_rpg.mdl" );
	PRECACHE_MODEL( "models/p_rpg.mdl" );

	PRECACHE_SOUND( "items/9mmclip1.wav" );

	PRECACHE_SOUND("sentences/hev_aim_on.wav");
	PRECACHE_SOUND("sentences/hev_aim_off.wav");
	PRECACHE_SOUND( "weapons/rocketfire1.wav" );
	PRECACHE_SOUND( "weapons/glauncher.wav" ); // alternative fire sound
	PRECACHE_SOUND("weapons/rpg_lock03.wav");

	UTIL_PrecacheOther( "rpg_rocket" );

//	m_usRpg = PRECACHE_EVENT( 1, "events/rpg.sc" ); //modif de Julien, wasn't in Julien's code, commented out (never understood those sc events anyway)
//	This event was used in primary attack code, but it got removed anyway.
}

// /give weapon_rpg

int CRpg::GetItemInfo( ItemInfo *p )
{
	p->pszName = STRING( pev->classname );
	p->pszAmmo1 = NULL; //modif de Julien
	p->iMaxAmmo1 = ROCKET_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = RPG_MAX_CLIP;
	p->iSlot = 3;
	p->iPosition = 0;
	p->iId = m_iId = WEAPON_RPG;
	p->iFlags = 0;
	p->iWeight = RPG_WEIGHT;

	return 1;
}

int CRpg::AddToPlayer( CBasePlayer *pPlayer )
{
	if( CBasePlayerWeapon::AddToPlayer( pPlayer ) )
	{
		MESSAGE_BEGIN( MSG_ONE, gmsgWeapPickup, NULL, pPlayer->pev );
			WRITE_BYTE( m_iId );
		MESSAGE_END();

		m_pPlayer->TextAmmo( TA_RPG );

		// ajoute la premi

		AddAmmo ( this, AMMO_ROCKET, 1 );

		return TRUE;
	}
	return FALSE;
}

//------------------------------------------
//
// Draw / Holster
//
//------------------------------------------



BOOL CRpg::Deploy()
{
	// d
	UpdateMenu ();
 
	PlayStateSound ();

	if ( m_iAmmoType == AMMO_ROCKET )
		UpdateCrosshair ( RPG_CROSSHAIR_EMPTY );
	else
		UpdateCrosshair ( RPG_CROSSHAIR_NORMAL );

	m_flReloadTime = -1;		// -1 = pas en cours de rechargement

	// animation

	BOOL bResult = DefaultDeploy( "models/v_rpg.mdl", "models/p_rpg.mdl", RPG_DRAW, "rpg" );

	m_flNextPrimaryAttack = gpGlobals->time + 8 / 5.0;
	m_flTimeWeaponIdle = gpGlobals->time + 8 / 5.0;

	return bResult;
}

 BOOL CRpg::CanHolster( void )
 {
	if ( m_cActiveRockets )
 		return FALSE;

 	return TRUE;
 }

void CRpg::Holster( int skiplocal )
{
	m_fInReload = FALSE;// cancel any reload in progress.

	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5f;
	m_flTimeWeaponIdle = gpGlobals->time + RANDOM_FLOAT ( 10, 15 );

	// fermeture du hud rpg

	m_iMenuState |= RPG_CLOSE;
	UpdateMenu ();
	m_iMenuState &= ~RPG_CLOSE;

}

//------------------------------------------
//
// Tirs principal et secondaire
//
//------------------------------------------

void CRpg::PrimaryAttack()
{
	// confirmation de la selection

	if ( m_iMenuState & RPG_MENU_ACTIVE )
	{
		m_iMenuState &= ~RPG_MENU_ACTIVE;
		UpdateMenu();

		if ( m_iMenuState & RPG_MENU_ROCKET_SELECTED )
		{
			if ( m_iAmmoType != AMMO_ROCKET )
			{
				m_bLoaded = FALSE;
				m_flTimeWeaponIdle = gpGlobals->time - 0.1;
			}
			m_iAmmoType = AMMO_ROCKET;
		}
		else if ( m_iMenuState & RPG_MENU_ELECTRO_SELECTED )
		{
			if ( m_iAmmoType != AMMO_ELECTRO )
			{
				m_bLoaded = FALSE;
				m_flTimeWeaponIdle = gpGlobals->time - 0.1;
			}
			m_iAmmoType = AMMO_ELECTRO;
		}
		else if ( m_iMenuState & RPG_MENU_NUCLEAR_SELECTED )
		{
			if ( m_iAmmoType != AMMO_NUCLEAR )
			{
				m_bLoaded = FALSE;
				m_flTimeWeaponIdle = gpGlobals->time - 0.1;
			}
			m_iAmmoType = AMMO_NUCLEAR;
		}

		// bon viseur !

		if ( m_iAmmoType == AMMO_ROCKET )
			UpdateCrosshair ( RPG_CROSSHAIR_EMPTY );
		else
			UpdateCrosshair ( RPG_CROSSHAIR_NORMAL );

				
		m_flNextPrimaryAttack = gpGlobals->time + 0.3;
	}

	// tir

	else if ( m_bLoaded == TRUE &&
			   ((m_iAmmoType == AMMO_ROCKET		&& m_iAmmoRocket	!= 0) ||
				(m_iAmmoType == AMMO_ELECTRO	&& m_iAmmoElectro	!= 0) ||
				(m_iAmmoType == AMMO_NUCLEAR	&& m_iAmmoNuclear	!= 0)  )
			)
	{

		// flash et son

		m_pPlayer->m_iWeaponVolume = LOUD_GUN_VOLUME;
		m_pPlayer->m_iWeaponFlash = BRIGHT_GUN_FLASH;
		m_pPlayer->pev->punchangle.x -= 5;
		EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/rocketfire1.wav", 0.9, ATTN_NORM );
		EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/glauncher.wav", 0.7, ATTN_NORM );

		// bodygroup
		pev->body = RPG_WEAPON_EMPTY;

#if !CLIENT_DLL
		// animations
 
		// player "shoot" animation
		SendWeaponAnim( RPG_FIRE );
		m_pPlayer->SetAnimation( PLAYER_ATTACK1 );
		// roquette

		UTIL_MakeVectors( m_pPlayer->pev->v_angle );
		Vector vecSrc = m_pPlayer->GetGunPosition() + gpGlobals->v_forward * 16.0f + gpGlobals->v_right * 8.0f + gpGlobals->v_up * -8.0f;

		pRocket = CRpgRocket::CreateRpgRocket( vecSrc, m_pPlayer->pev->v_angle, m_pPlayer, this );

		if ( (m_iAmmoType == AMMO_ROCKET)  && ( m_pEntityLocked != NULL ) )
		{
			pRocket->m_pTargetMonster = m_pEntityLocked;
			m_cActiveRockets = 1;
		}
		else
		{
			pRocket->m_pTargetMonster = NULL;
			m_cActiveRockets = 0;			// m_cActiveRockets empeche le rechargement
		}

		UTIL_MakeVectors( m_pPlayer->pev->v_angle );// RpgRocket::Create stomps on globals, so remake.
		pRocket->pev->velocity = pRocket->pev->velocity + gpGlobals->v_forward * DotProduct( m_pPlayer->pev->velocity, gpGlobals->v_forward );
#endif

		// munitions

		m_bLoaded = FALSE;

		if ( m_iAmmoType == AMMO_ROCKET )		// A FAIRE  : continuer de remplacer les masks par iammotype
		{
			m_iAmmoRocket --;

			if ( m_iAmmoRocket == 0 )
				m_iMenuState |= RPG_MENU_ROCKET_EMPTY;
		}
		else if ( m_iAmmoType == AMMO_ELECTRO )
		{
			m_iAmmoElectro --;

			if ( m_iAmmoElectro == 0 )
				m_iMenuState |= RPG_MENU_ELECTRO_EMPTY;
		}
		else if ( m_iAmmoType == AMMO_NUCLEAR )
		{
			m_iAmmoNuclear --;

			if ( m_iAmmoNuclear == 0 )
				m_iMenuState |= RPG_MENU_NUCLEAR_EMPTY;
		}

		// mise a jour du hud

		UpdateMenu();

		m_pEntityTarget = m_pEntityLocked = NULL;
		m_flLockTime = 0;
		
		// prochaine attaque

		m_flNextPrimaryAttack = GetNextAttackDelay( 1.5f );
		m_flTimeWeaponIdle = gpGlobals->time + 1.5f;		// weaponidle () declenche le rechargement

		ResetEmptySound();
	}
	else
	{
		PlayEmptySound();
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.2f;
	}
}

void CRpg::SecondaryAttack()
{
	// menu

	if ( !(m_iMenuState & RPG_MENU_ACTIVE) )
		m_iMenuState |= RPG_MENU_ACTIVE;

	else if ( m_iMenuState & RPG_MENU_ROCKET_SELECTED )
	{
		m_iMenuState &= ~RPG_MENU_ROCKET_SELECTED;

		if ( !(m_iMenuState & RPG_MENU_ELECTRO_EMPTY) )
			m_iMenuState |= RPG_MENU_ELECTRO_SELECTED;

		else if ( !(m_iMenuState & RPG_MENU_NUCLEAR_EMPTY) )
			m_iMenuState |= RPG_MENU_NUCLEAR_SELECTED;

		else
			m_iMenuState |= RPG_MENU_ROCKET_SELECTED;
	}


	else if ( m_iMenuState & RPG_MENU_ELECTRO_SELECTED )
	{
		m_iMenuState &= ~RPG_MENU_ELECTRO_SELECTED;

		if ( !(m_iMenuState & RPG_MENU_NUCLEAR_EMPTY) )
			m_iMenuState |= RPG_MENU_NUCLEAR_SELECTED;

		else if ( !(m_iMenuState & RPG_MENU_ROCKET_EMPTY) )
			m_iMenuState |= RPG_MENU_ROCKET_SELECTED;

		else
			m_iMenuState |= RPG_MENU_ELECTRO_SELECTED;
	}
	else if ( m_iMenuState & RPG_MENU_NUCLEAR_SELECTED )
	{
		m_iMenuState &= ~RPG_MENU_NUCLEAR_SELECTED;

		if ( !(m_iMenuState & RPG_MENU_ROCKET_EMPTY) )
			m_iMenuState |= RPG_MENU_ROCKET_SELECTED;

		else if ( !(m_iMenuState & RPG_MENU_ELECTRO_EMPTY) )
			m_iMenuState |= RPG_MENU_ELECTRO_SELECTED;

		else
			m_iMenuState |= RPG_MENU_NUCLEAR_SELECTED;
	}

	UpdateMenu ();
	
	PlayStateSound ();

	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.2f;
}

//------------------------------------------
//
// Rechargement
//
//------------------------------------------



void CRpg::Reload( void )
{
	if ( m_cActiveRockets < 0 || m_flTimeWeaponIdle > gpGlobals->time ) // weaponidle () declenche le rechargement
		return;

	if (	(m_iAmmoType == AMMO_ROCKET		&& m_iAmmoRocket	== 0) ||
			(m_iAmmoType == AMMO_ELECTRO	&& m_iAmmoElectro	== 0) ||
			(m_iAmmoType == AMMO_NUCLEAR	&& m_iAmmoNuclear	== 0)  
		)
			return;

	// bodygroup
	// en cas de changement de munition, le bodygroup n'a pas 

	pev->body = RPG_WEAPON_EMPTY;

	// animations

	int iAnim;
	switch ( m_iAmmoType )
	{
	case AMMO_ROCKET:
		iAnim = RPG_RELOAD_ROCKET;
		m_flReloadTime = gpGlobals->time + 2;
		break;

	case AMMO_ELECTRO:
		iAnim = RPG_RELOAD_ELECTRO;
		m_flReloadTime = gpGlobals->time + 4;
		break;

	case AMMO_NUCLEAR:
		iAnim = RPG_RELOAD_NUCLEAR;
		m_flReloadTime = gpGlobals->time + 6;
		break;
	}

	SendWeaponAnim( iAnim );

	// validation du rechargement

	m_flTimeWeaponIdle = gpGlobals->time + 10;
	m_flNextPrimaryAttack = gpGlobals->time + 10;

}

void CRpg::WeaponIdle( void )
{
	// rafraichissement des donnees client

	if ( m_bRpgUpdate == 1 )
	{
		UpdateMenu ();

		if ( m_iAmmoType == AMMO_ROCKET )
			UpdateCrosshair ( RPG_CROSSHAIR_EMPTY );
		else
			UpdateCrosshair ( RPG_CROSSHAIR_NORMAL );

		m_flTimeWeaponIdle = gpGlobals->time - 0.1;		// pour charger une anim et changer le bodygroup
		m_bRpgUpdate = 0;		// weaponidle, ca rafraichit, et c'est d
	}

	// viseur

	if ( (m_iAmmoType == AMMO_ROCKET) && ( m_cActiveRockets == 0 ) )
		UpdateEntityTarget ();

	//ALARM! DIFFERENT CODE DETECTED!
 	ResetEmptySound( );
 
	// rechargement

	if ( m_flTimeWeaponIdle < gpGlobals->time && m_bLoaded == FALSE && m_flReloadTime == -1 )	// -1 = pas en cours de rechargement
	{
		Reload ();
		return;			// pour ne pas lancer d anim idle
	}

	else if ( m_bLoaded == FALSE && m_flReloadTime != -1 && m_flReloadTime < gpGlobals->time )
	{
		m_bLoaded				= TRUE;
		m_flReloadTime			= -1;
		m_flNextPrimaryAttack	= gpGlobals->time;
		m_flTimeWeaponIdle		= gpGlobals->time;
	}

	
	// ajustement du bodygroup

	if ( m_flTimeWeaponIdle <= gpGlobals->time )
	{
		int ibody;
		switch ( m_iAmmoType )
		{
		case AMMO_ROCKET:
			ibody = RPG_WEAPON_ROCKET; break;
		case AMMO_ELECTRO:
			ibody = RPG_WEAPON_ELECTRO; break;
		case AMMO_NUCLEAR:
			ibody = RPG_WEAPON_NUCLEAR; break;
		}
		if ( m_bLoaded == FALSE )
			ibody = RPG_WEAPON_EMPTY;
		
		pev->body = ibody;
	}



	// animations idle

 	if (m_flTimeWeaponIdle > gpGlobals->time)
 		return;
 
	int iAnim;
	switch ( RANDOM_LONG( 0, 6 ) )
 	{
	case 0:
		iAnim = RPG_FIDGET;
		m_flTimeWeaponIdle = gpGlobals->time + 15 / 4.0;
		break;
	
	default:
		iAnim = RPG_IDLE;
		m_flTimeWeaponIdle = gpGlobals->time + 15 / 3.0;
		break;
	}
	SendWeaponAnim( iAnim );
}




void CRpg::UpdateEntityTarget( void )
{
	if ( m_iMenuState & RPG_MENU_ACTIVE )
		return;			// le texte s'affiche et disparait selon la selection

	UTIL_MakeVectors( m_pPlayer->pev->v_angle );
	Vector vecSrc = m_pPlayer->GetGunPosition( );
	Vector vecAiming = gpGlobals->v_forward;

	TraceResult tr;
	UTIL_TraceLine ( vecSrc, vecSrc + vecAiming * 8192, dont_ignore_monsters, ENT(m_pPlayer->pev), &tr );		//tracage du vecteur


	if ( tr.pHit && ( CBaseEntity :: Instance ( tr.pHit )) -> MyMonsterPointer() != NULL )
	{

		// si une entite est touchee et 
		//que l'entit


		if ( m_pEntityTarget == NULL || ( m_pEntityTarget != NULL && ( m_pEntityTarget != (CBaseEntity::Instance(tr.pHit)) ) ) )
 		{
 
			// si il n'y avait pas de cible, on en met une . On met en memoire l heure de reperage de la cible
			// une cible, mais pas la meme qu avant, ...

			m_pEntityTarget = CBaseEntity::Instance ( tr.pHit );
			m_pEntityLocked = NULL;
			m_flLockTime = gpGlobals->time;
			UpdateCrosshair ( RPG_CROSSHAIR_PROCESS );

			
			// son
			EMIT_SOUND ( ENT(pev), CHAN_BODY, "weapons/rpg_lock03.wav", 0.5, ATTN_NORM );

 		}

		else if ( ( m_pEntityTarget != NULL ) && ( m_pEntityTarget == (CBaseEntity::Instance(tr.pHit))) && ( gpGlobals->time - m_flLockTime > 1 ) )
 		{
			// la meme cible pendant 1 sec , on verrouille
 

			m_pEntityLocked = m_pEntityTarget;
			UpdateCrosshair ( RPG_CROSSHAIR_LOCKED );
 
			// son
			if ( m_flLastBip <= 0 || gpGlobals->time - m_flLastBip >= 0.1 )
			{
				EMIT_SOUND ( ENT(pev), CHAN_BODY, "weapons/rpg_lock03.wav", 0.5, ATTN_NORM );
				m_flLastBip = gpGlobals->time;
			}
		}
		
 	}

 	else
 	{

		//si rien n est touche

		UpdateCrosshair ( RPG_CROSSHAIR_EMPTY );
		m_pEntityTarget = m_pEntityLocked = NULL;
		m_flLockTime = 0;
 	}

}


void CRpg :: PlayStateSound ( void )
{
/*
	if ( m_iAmmoType == AMMO_ROCKET )
		EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "sentences/hev_aim_on.wav", 0.9, ATTN_NORM );

	else
		EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "sentences/hev_aim_off.wav", 0.9, ATTN_NORM );
*/
}

//------------------------------------------
//
// messages au client
//
//------------------------------------------

void CRpg :: UpdateCrosshair ( int crosshair )
{

	MESSAGE_BEGIN( MSG_ONE, gmsgRpgViseur, NULL, m_pPlayer->pev );
		WRITE_BYTE( crosshair );
	MESSAGE_END();
}


void CRpg :: UpdateMenu ( void )
{
	MESSAGE_BEGIN( MSG_ONE, gmsgRpgMenu, NULL, m_pPlayer->pev );
		WRITE_BYTE( m_iMenuState );
		WRITE_BYTE( m_iAmmoRocket );
		WRITE_BYTE( m_iAmmoElectro );
		WRITE_BYTE( m_iAmmoNuclear );
	MESSAGE_END();

}
 
 
//------------------------------------------
//
// munitions
//
//------------------------------------------

void CRpg ::  ItemTouch( CBaseEntity *pOther )
 {
	if ( pOther->IsPlayer() == FALSE )
 	{
		return;
	}

	CBasePlayer *pPlayer = (CBasePlayer*)pOther;
	CBasePlayerItem *pItem;
	CRpg *pRpg = NULL;

	for ( int i = 0 ; i < MAX_ITEM_TYPES ; i++ )
	{
		pItem = pPlayer->m_rgpPlayerItems[ i ];
		
		while (pItem)
 		{
			if ( !strcmp( "weapon_rpg", STRING( pItem->pev->classname ) ) )
			{
				pRpg = (CRpg*)pItem->GetWeaponPtr();
				break;
			}
			pItem = pItem->m_pNext;
 		}
	}
 
	if ( pRpg == NULL )
		return;
 
	if ( pRpg->AddAmmo ( (CBasePlayerWeapon *) pRpg, AMMO_ROCKET, 1 ) )
	{
		EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM);
		UTIL_Remove( this );
	}
}
 
int CRpg::ExtractAmmo( CBasePlayerWeapon *pWeapon )
{
//	AddAmmo ( pWeapon, m_iAmmoType, 1 );
	return 1;
}

int CRpg::ExtractClipAmmo( CBasePlayerWeapon *pWeapon )
{
//	AddAmmo ( pWeapon, m_iAmmoType, 1 );
	return 1;
}

BOOL CRpg ::AddAmmo ( CBasePlayerWeapon *pWeapon, int iAmmotype, int iNombre )
{
	int *pType;
	CRpg *pRpg = (CRpg*) pWeapon;

	switch ( iAmmotype )
	{
	default:
	case AMMO_ROCKET:
		pType = &pRpg->m_iAmmoRocket;
		pRpg->m_iMenuState &= ~RPG_MENU_ROCKET_EMPTY;
		break;

	case AMMO_ELECTRO:
		pType = &pRpg->m_iAmmoElectro;
		pRpg->m_iMenuState &= ~RPG_MENU_ELECTRO_EMPTY;
		break;

	case AMMO_NUCLEAR:
		pType = &pRpg->m_iAmmoNuclear;
		pRpg->m_iMenuState &= ~RPG_MENU_NUCLEAR_EMPTY;
		break;

 	}

	if ( pType[0] == RPG_MAX_AMMO )
		return FALSE;

	pType[0] += iNombre;

	if ( m_pPlayer->m_pActiveItem == pWeapon )
	{
		pRpg->UpdateMenu();
	}

	return TRUE;
 }
 
int CRpg :: GiveAmmo( int iAmount, char *szName, int iMax )
{
//	AddAmmo ( this, m_iAmmoType, 1 );
	return 1;
}

class CRpgAmmo : public CBasePlayerAmmo
{
	void Spawn( void )
	{
		Precache();

		if ( FClassnameIs( pev, "ammo_rpgclip" ) )
			SET_MODEL(ENT(pev), "models/w_rpgclip.mdl");
		else if ( FClassnameIs( pev, "ammo_rpgelectroclip" ) )
			SET_MODEL(ENT(pev), "models/w_rpgelectroclip.mdl");
		else if ( FClassnameIs( pev, "ammo_rpgnuclearclip" ) )
			SET_MODEL(ENT(pev), "models/w_rpgnuclearclip.mdl");

		CBasePlayerAmmo::Spawn();
	}
	void Precache( void )
	{
		PRECACHE_MODEL ("models/w_rpgclip.mdl");
		PRECACHE_MODEL ("models/w_rpgelectroclip.mdl");
		PRECACHE_MODEL ("models/w_rpgnuclearclip.mdl");

		PRECACHE_SOUND( "items/9mmclip1.wav" );
	}
 	BOOL AddAmmo( CBaseEntity *pOther ) 
	{
		if ( pOther->IsPlayer() == 0 )
			return FALSE;
 
		CBasePlayer *pPlayer = (CBasePlayer *) pOther;

		
		CBasePlayerItem *pItem;
		CRpg *pRpg = NULL;
		int i;
		int fin = 0;

		for ( i = 0 ; i < MAX_ITEM_TYPES ; i++ )
 		{
			pItem = pPlayer->m_rgpPlayerItems[ i ];
			
			while (pItem)
			{
				if ( !strcmp( "weapon_rpg"/*&pszItemName*/, STRING( pItem->pev->classname ) ) )
				{
					fin = 1;
					pRpg = (CRpg*)pItem->GetWeaponPtr();
				}
				pItem = pItem->m_pNext;

				if ( fin )
					break;
			}

			if ( fin )
				break;
 		}

		if ( pRpg == NULL )
			return FALSE;

		int iAmmotype = 0;

		if ( FClassnameIs( pev, "ammo_rpgclip" ) )
			iAmmotype = AMMO_ROCKET;
		else if ( FClassnameIs( pev, "ammo_rpgelectroclip" ) )
			iAmmotype = AMMO_ELECTRO;
		else if ( FClassnameIs( pev, "ammo_rpgnuclearclip" ) )
			iAmmotype = AMMO_NUCLEAR;
 		else
			return FALSE;
 
		if ( pRpg->AddAmmo ( (CBasePlayerWeapon *) pRpg, iAmmotype, 1 ) == TRUE )
 		{
			// accept

			switch ( iAmmotype )
			{
			default:
			case AMMO_ROCKET:
				break;
			case AMMO_ELECTRO:
				pRpg->m_pPlayer->TextAmmo( TA_ELECTROROCKET ); break;
			case AMMO_NUCLEAR:
				pRpg->m_pPlayer->TextAmmo( TA_NUCLEARROCKET ); break;
			}

 			EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM);
 			return TRUE;
 		}

		else
			return FALSE;
 	}
};

LINK_ENTITY_TO_CLASS( ammo_rpgclip, CRpgAmmo )
LINK_ENTITY_TO_CLASS( ammo_rpgelectroclip, CRpgAmmo );
LINK_ENTITY_TO_CLASS( ammo_rpgnuclearclip, CRpgAmmo );


//------------------------------------------
//
// sauvegarde et restauration
//
//------------------------------------------

#if !CLIENT_DLL
int CRpg::Save( CSave &save )
{
	if ( !CBasePlayerWeapon::Save(save) )
		return 0;

	return save.WriteFields( "CRpg", this, m_SaveData, ARRAYSIZE(m_SaveData) );
}

int CRpg::Restore( CRestore &restore )
{
	if ( !CBasePlayerWeapon::Restore(restore) )
		return 0;

	int status = restore.ReadFields( "CRpg", this, m_SaveData, ARRAYSIZE(m_SaveData) );

	//-----------------------

	m_bRpgUpdate = 1;		// force le rafraichissement des donnees client

	//----------------------
	return status;
}
#endif