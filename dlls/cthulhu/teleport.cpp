/***
*
*	Copyright (c) 1996-2001, Valve LLC. All rights reserved.
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
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"
#include "gamerules.h"
#include "soundent.h"

enum teleport_e {
	TELEPORT_IDLE1 = 0,
	TELEPORT_FIDGET1,
	TELEPORT_DRAW,
	TELEPORT_DROP
};

enum teleport_radio_e {
	TELEPORT_RADIO_IDLE1 = 0,
	TELEPORT_RADIO_FIDGET1,
	TELEPORT_RADIO_DRAW,
	TELEPORT_RADIO_FIRE,
	TELEPORT_RADIO_HOLSTER
};



class CTeleportCharge : public CGrenade
{
	void Spawn( void );
	void Precache( void );
	void BounceSound( void );

	void EXPORT TeleportSlide( CBaseEntity *pOther );
	void EXPORT TeleportThink( void );

public:
	void Destroy();
	void Deactivate( void );
};
LINK_ENTITY_TO_CLASS( monster_teleport, CTeleportCharge );

class CTeleport : public CBasePlayerWeapon
{
public:

#ifndef CLIENT_DLL
	int		Save( CSave &save );
	int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];
#endif


	void Spawn( void );
	void Precache( void );
	int iItemSlot( void ) { return 3; }
	int GetItemInfo(ItemInfo *p);
	int AddToPlayer( CBasePlayer *pPlayer );
	void PrimaryAttack( void );
	void SecondaryAttack( void );
	int AddDuplicate( CBasePlayerItem *pOriginal );
	BOOL CanDeploy( void );
	BOOL Deploy( void );
	BOOL IsUseable( void );
	
	void Holster( int skiplocal = 0 );
	void WeaponIdle( void );
	void Throw( void );
	
	virtual BOOL UseDecrement( void )
	{ 
#if defined( CLIENT_WEAPONS )
		return TRUE;
#else
		return FALSE;
#endif
	}
};


void CTeleportCharge::Destroy( void )
{
	MESSAGE_BEGIN( MSG_PAS, SVC_TEMPENTITY, pev->origin );
		WRITE_BYTE( TE_EXPLOSION );		// This makes a dynamic light and the explosion sprites/sound
		WRITE_COORD( pev->origin.x );	// Send to PAS because of the sound
		WRITE_COORD( pev->origin.y );
		WRITE_COORD( pev->origin.z );
		WRITE_SHORT( g_sModelIndexFireball );
		WRITE_BYTE( 5  ); 
		WRITE_BYTE( 15  ); // framerate
		WRITE_BYTE( TE_EXPLFLAG_NONE );
	MESSAGE_END();

	CSoundEnt::InsertSound ( bits_SOUND_COMBAT, pev->origin, SMALL_EXPLOSION_VOLUME, 3.0 );

	Deactivate();
}

//=========================================================
// Deactivate - do whatever it is we do to an orphaned 
// Teleport when we don't want it in the world anymore.
//=========================================================
void CTeleportCharge::Deactivate( void )
{
	pev->solid = SOLID_NOT;
	UTIL_Remove( this );
}


void CTeleportCharge :: Spawn( void )
{
	Precache( );
	// motor
	pev->movetype = MOVETYPE_BOUNCE;
	pev->solid = SOLID_BBOX;

	SET_MODEL(ENT(pev), "models/w_satchel.mdl");
	UTIL_SetSize(pev, Vector( -16, -16, -4), Vector(16, 16, 4));	// Old box -- size of headcrab monsters/players get blocked by this
	UTIL_SetOrigin( this, pev->origin );

	SetTouch( TeleportSlide );
	SetUse( DetonateUse );
	SetThink( TeleportThink );
	SetNextThink( 0.1 );

	pev->gravity = 0.5;
	pev->friction = 0.8;

	// hardcoded small value
	pev->dmg = 10;

	// ResetSequenceInfo( );
	pev->sequence = 1;
}


void CTeleportCharge::TeleportSlide( CBaseEntity *pOther )
{
	entvars_t	*pevOther = pOther->pev;

	// don't hit the guy that launched this grenade
	if ( pOther->edict() == pev->owner )
		return;

	// pev->avelocity = Vector (300, 300, 300);
	pev->gravity = 1;// normal gravity now

	// HACKHACK - On ground isn't always set, so look for ground underneath
	TraceResult tr;
	UTIL_TraceLine( pev->origin, pev->origin - Vector(0,0,10), ignore_monsters, edict(), &tr );

	if ( tr.flFraction < 1.0 )
	{
		// add a bit of static friction
		pev->velocity = pev->velocity * 0.95;
		pev->avelocity = pev->avelocity * 0.9;
		// play sliding sound, volume based on velocity
	}
	if ( !(pev->flags & FL_ONGROUND) && pev->velocity.Length2D() > 10 )
	{
		BounceSound();
	}
	StudioFrameAdvance( );
}


void CTeleportCharge :: TeleportThink( void )
{
	StudioFrameAdvance( );
	SetNextThink( 0.1 );

	if (!IsInWorld())
	{
		UTIL_Remove( this );
		return;
	}

	if (pev->waterlevel == 3 && pev->watertype != CONTENT_FOG)
	{
		pev->movetype = MOVETYPE_FLY;
		pev->velocity = pev->velocity * 0.8;
		pev->avelocity = pev->avelocity * 0.9;
		pev->velocity.z += 8;
	}
	else if (pev->waterlevel == 0 || pev->watertype == CONTENT_FOG)
	{
		pev->movetype = MOVETYPE_BOUNCE;
	}
	else
	{
		pev->velocity.z -= 8;
	}	
}

void CTeleportCharge :: Precache( void )
{
	PRECACHE_MODEL("models/grenade.mdl");
	PRECACHE_SOUND("weapons/g_bounce1.wav");
	PRECACHE_SOUND("weapons/g_bounce2.wav");
	PRECACHE_SOUND("weapons/g_bounce3.wav");
}

void CTeleportCharge :: BounceSound( void )
{
	switch ( RANDOM_LONG( 0, 2 ) )
	{
	case 0:	EMIT_SOUND(ENT(pev), CHAN_VOICE, "weapons/g_bounce1.wav", 1, ATTN_NORM);	break;
	case 1:	EMIT_SOUND(ENT(pev), CHAN_VOICE, "weapons/g_bounce2.wav", 1, ATTN_NORM);	break;
	case 2:	EMIT_SOUND(ENT(pev), CHAN_VOICE, "weapons/g_bounce3.wav", 1, ATTN_NORM);	break;
	}
}


LINK_ENTITY_TO_CLASS( weapon_teleport, CTeleport );


TYPEDESCRIPTION	CTeleport::m_SaveData[] = 
{
	DEFINE_FIELD( CTeleport, m_chargeReady, FIELD_INTEGER ),
};
IMPLEMENT_SAVERESTORE( CTeleport, CBasePlayerWeapon );

//=========================================================
// CALLED THROUGH the newly-touched weapon's instance. The existing player weapon is pOriginal
//=========================================================
int CTeleport::AddDuplicate( CBasePlayerItem *pOriginal )
{
	CTeleport *pTeleport;

#ifdef CLIENT_DLL
	if ( bIsMultiplayer() )
#else
	if ( g_pGameRules->IsMultiplayer() )
#endif
	{
		pTeleport = (CTeleport *)pOriginal;

		if ( pTeleport->m_chargeReady != 0 )
		{
			// player has some teleports deployed. Refuse to add more.
			return FALSE;
		}
	}

	return CBasePlayerWeapon::AddDuplicate ( pOriginal );
}

//=========================================================
//=========================================================
int CTeleport::AddToPlayer( CBasePlayer *pPlayer )
{
	int bResult = CBasePlayerItem::AddToPlayer( pPlayer );

	// cthulhu
	m_iPrimaryAmmoType = pPlayer->GetAmmoIndex( pszAmmo1() );

	pPlayer->pev->weapons |= (1<<m_iId);
	m_chargeReady = 0;// this teleport charge weapon now forgets that any teleports are deployed by it.

	if ( bResult )
	{
		return AddWeapon( );
	}
	return FALSE;
}

void CTeleport::Spawn( )
{
	Precache( );
	m_iId = WEAPON_TELEPORT;
	SET_MODEL(ENT(pev), "models/w_satchel.mdl");

	m_iDefaultAmmo = TELEPORT_DEFAULT_GIVE;
		
	FallInit();// get ready to fall down.
}


void CTeleport::Precache( void )
{
	PRECACHE_MODEL("models/v_satchel.mdl");
	PRECACHE_MODEL("models/v_satchel_radio.mdl");
	PRECACHE_MODEL("models/w_satchel.mdl");
	PRECACHE_MODEL("models/p_satchel.mdl");
	PRECACHE_MODEL("models/p_satchel_radio.mdl");

	UTIL_PrecacheOther( "monster_teleport" );
	UTIL_PrecacheOther( "env_warpball" );
}


int CTeleport::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "Teleport Pad";
	p->iMaxAmmo1 = TELEPORT_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = WEAPON_NOCLIP;
	p->iSlot = 3;
	p->iPosition = 3;
	p->iFlags = ITEM_FLAG_SELECTONEMPTY | ITEM_FLAG_LIMITINWORLD | ITEM_FLAG_EXHAUSTIBLE;
	p->iId = m_iId = WEAPON_TELEPORT;
	p->iWeight = TELEPORT_WEIGHT;

	return 1;
}

//=========================================================
//=========================================================
BOOL CTeleport::IsUseable( void )
{
	if ( m_pPlayer->m_rgAmmo[ PrimaryAmmoIndex() ] > 0 ) 
	{
		// player is carrying some teleports
		return TRUE;
	}

	if ( m_chargeReady != 0 )
	{
		// player isn't carrying any teleports, but has some out
		return TRUE;
	}

	return FALSE;
}

BOOL CTeleport::CanDeploy( void )
{
	if ( m_pPlayer->m_rgAmmo[ PrimaryAmmoIndex() ] > 0 ) 
	{
		// player is carrying some teleports
		return TRUE;
	}

	if ( m_chargeReady != 0 )
	{
		// player isn't carrying any teleports, but has some out
		return TRUE;
	}

	return FALSE;
}

BOOL CTeleport::Deploy( )
{

	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 1.0;
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );

	if ( m_chargeReady )
		return DefaultDeploy( "models/v_satchel_radio.mdl", "models/p_satchel_radio.mdl", TELEPORT_RADIO_DRAW, "hive" );
	else
		return DefaultDeploy( "models/v_satchel.mdl", "models/p_satchel.mdl", TELEPORT_DRAW, "trip" );

	
	return TRUE;
}


void CTeleport::Holster( int skiplocal /* = 0 */ )
{
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;
	
	if ( m_chargeReady )
	{
		SendWeaponAnim( TELEPORT_RADIO_HOLSTER );
	}
	else
	{
		SendWeaponAnim( TELEPORT_DROP );
	}
	EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "common/null.wav", 1.0, ATTN_NORM);

	if ( !m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] && !m_chargeReady )
	{
		m_pPlayer->pev->weapons &= ~(1<<WEAPON_TELEPORT);
		SetThink( DestroyItem );
		SetNextThink( 0.1 );
	}
}



void CTeleport::PrimaryAttack()
{
	switch (m_chargeReady)
	{
	case 0:
		{
		Throw( );
		}
		break;
	case 1:
		{
			SendWeaponAnim( TELEPORT_RADIO_FIRE );

			Vector vOldOrigin;
			edict_t *pPlayer = m_pPlayer->edict( );

			// get the teleport pad
			CBaseEntity *pTeleport = NULL;

			while ((pTeleport = UTIL_FindEntityInSphere( pTeleport, m_pPlayer->pev->origin, 4096 )) != NULL)
			{
				if (FClassnameIs( pTeleport->pev, "monster_teleport"))
				{
					if (pTeleport->pev->owner == pPlayer)
					{
						// get its position
						vOldOrigin = m_pPlayer->pev->origin;

						// move our position (if possible)
						m_pPlayer->pev->origin = pTeleport->pev->origin;
						m_pPlayer->pev->origin.z += 40;
						if (!WALK_MOVE ( ENT(m_pPlayer->pev), 0,1, WALKMOVE_NORMAL ))
						{
							m_pPlayer->pev->origin = vOldOrigin;
						}
						else
						{
							CEnvWarpBall* pWarpIn = (CEnvWarpBall*)CBaseEntity::Create("env_warpball",m_pPlayer->pev->origin,Vector(0,0,0),pPlayer);
							pWarpIn->pev->frags = 8;	// num of beams
							pWarpIn->pev->health = 128;	// max length of beam
							pWarpIn->Use(NULL,NULL,USE_TOGGLE,0);

							CEnvWarpBall* pWarpOut = (CEnvWarpBall*)CBaseEntity::Create("env_warpball",vOldOrigin,Vector(0,0,0),pPlayer);
							pWarpOut->pev->frags = 8;	// num of beams
							pWarpOut->pev->health = 128;	// max length of beam
							pWarpOut->Use(NULL,NULL,USE_TOGGLE,0);
						}

						break;
					}
				}
			}
			m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.5;
			m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.5;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.5;
		}
		break;

	case 2:
		// we're reloading, don't allow fire
		{
		}
		break;
	}
}


void CTeleport::SecondaryAttack( void )
{
	switch (m_chargeReady)
	{
	case 0:
		{
			// we don't do anything if we are ready to throw
		}
		break;
	case 1:
		{
		SendWeaponAnim( TELEPORT_RADIO_FIRE );

		edict_t *pPlayer = m_pPlayer->edict( );

		CBaseEntity *pTeleport = NULL;

		while ((pTeleport = UTIL_FindEntityInSphere( pTeleport, m_pPlayer->pev->origin, 4096 )) != NULL)
		{
			if (FClassnameIs( pTeleport->pev, "monster_teleport"))
			{
				if (pTeleport->pev->owner == pPlayer)
				{
					//pTeleport->Use( m_pPlayer, m_pPlayer, USE_ON, 0 );
					((CTeleportCharge*)pTeleport)->Destroy();
					m_chargeReady = 2;
				}
			}
		}

		m_chargeReady = 2;
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.5;
		m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.5;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.5;
		break;
		}

	case 2:
		// we're reloading, don't allow fire
		{
		}
		break;
	}
}


void CTeleport::Throw( void )
{
	if ( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] )
	{
		Vector vecSrc = m_pPlayer->pev->origin;

		Vector vecThrow = gpGlobals->v_forward * 274 + m_pPlayer->pev->velocity;

#ifndef CLIENT_DLL
		CBaseEntity *pTeleport = Create( "monster_teleport", vecSrc, Vector( 0, 0, 0), m_pPlayer->edict() );
		pTeleport->pev->velocity = vecThrow;
		pTeleport->pev->avelocity.y = 400;

		m_pPlayer->pev->viewmodel = MAKE_STRING("models/v_satchel_radio.mdl");
		m_pPlayer->pev->weaponmodel = MAKE_STRING("models/p_satchel_radio.mdl");
#else
		LoadVModel ( "models/v_satchel_radio.mdl", m_pPlayer );
#endif

		SendWeaponAnim( TELEPORT_RADIO_DRAW );

		// player "shoot" animation
		m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

		m_chargeReady = 1;
		
		m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType]--;

		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 1.0;
		m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.5;
	}
}


void CTeleport::WeaponIdle( void )
{
	if ( m_flTimeWeaponIdle > UTIL_WeaponTimeBase() )
		return;

	switch( m_chargeReady )
	{
	case 0:
		SendWeaponAnim( TELEPORT_FIDGET1 );
		// use tripmine animations
		strcpy( m_pPlayer->m_szAnimExtention, "trip" );
		break;
	case 1:
		SendWeaponAnim( TELEPORT_RADIO_FIDGET1 );
		// use hivehand animations
		strcpy( m_pPlayer->m_szAnimExtention, "hive" );
		break;
	case 2:
		if ( !m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] )
		{
			m_chargeReady = 0;
			RetireWeapon();
			return;
		}

#ifndef CLIENT_DLL
		m_pPlayer->pev->viewmodel = MAKE_STRING("models/v_satchel.mdl");
		m_pPlayer->pev->weaponmodel = MAKE_STRING("models/p_satchel.mdl");
#else
		LoadVModel ( "models/v_satchel.mdl", m_pPlayer );
#endif

		SendWeaponAnim( TELEPORT_DRAW );

		// use tripmine animations
		strcpy( m_pPlayer->m_szAnimExtention, "trip" );

		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.5;
		m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.5;
		m_chargeReady = 0;
		break;
	}
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );// how long till we do this again.
}

//=========================================================
// DeactivateTeleports - removes all Teleports owned by
// the provided player. Should only be used upon death.
//
// Made this global on purpose.
//=========================================================
void DeactivateTeleports( CBasePlayer *pOwner )
{
	edict_t *pFind; 

	pFind = FIND_ENTITY_BY_CLASSNAME( NULL, "monster_teleport" );

	while ( !FNullEnt( pFind ) )
	{
		CBaseEntity *pEnt = CBaseEntity::Instance( pFind );
		CTeleportCharge *pTeleport = (CTeleportCharge *)pEnt;

		if ( pTeleport )
		{
			if ( pTeleport->pev->owner == pOwner->edict() )
			{
				pTeleport->Deactivate();
			}
		}

		pFind = FIND_ENTITY_BY_CLASSNAME( pFind, "monster_teleport" );
	}
}

#endif