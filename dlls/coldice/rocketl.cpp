/*
	Copyright (c) 1999, Cold Ice Modification. 
	
	This code has been written by SlimShady ( darcuri@optonline.net )

    Use, distribution, and modification of this source code and/or resulting
    object code is restricted to non-commercial enhancements to products from
    Valve LLC.  All other use, distribution, or modification is prohibited
    without written permission from Valve LLC and from the Cold Ice team.

    Please if you use this code in any public form, please give us credit.

*/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"
#include "gamerules.h"


enum rpg_e {
	RPG_IDLE = 0,
	RPG_FIDGET,
	RPG_RELOAD,		// to reload
	RPG_FIRE2,		// to empty
	RPG_HOLSTER1,	// loaded
	RPG_DRAW1,		// loaded
	RPG_HOLSTER2,	// unloaded
	RPG_DRAW_UL,	// unloaded
	RPG_IDLE_UL,	// unloaded idle
	RPG_FIDGET_UL,	// unloaded fidget
};


class CLaserSpot : public CBaseEntity
{
	void Spawn( void );
	void Precache( void );

	int	ObjectCaps( void ) { return FCAP_DONT_SAVE; }

public:
	void Suspend( float flSuspendTime );
	void EXPORT Revive( void );
	
	static CLaserSpot *CreateSpot( void );
};
LINK_ENTITY_TO_CLASS( laser_spot, CLaserSpot );


class CRpg : public CBasePlayerWeapon
{
public:
	int		Save( CSave &save );
	int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	void Spawn( void );
	void Precache( void );
	void Reload( void );
	int iItemSlot( void ) { return 4; }
	int GetItemInfo(ItemInfo *p);
	int AddToPlayer( CBasePlayer *pPlayer );

	BOOL Deploy( void );
	BOOL CanHolster( void );
	void Holster( void );

	void PrimaryAttack( void );
	void SecondaryAttack( void );
	void WeaponIdle( void );

	void UpdateSpot( void );
	BOOL ShouldWeaponIdle( void ) { return TRUE; };

	CLaserSpot *m_pSpot;
	int m_fSpotActive;
	int m_cActiveRockets;

	int rocket_load;

};
LINK_ENTITY_TO_CLASS( weapon_rocketl, CRpg );
LINK_ENTITY_TO_CLASS ( weapon_rpg, CRpg );

TYPEDESCRIPTION	CRpg::m_SaveData[] = 
{
	DEFINE_FIELD( CRpg, m_fSpotActive, FIELD_INTEGER ),
	DEFINE_FIELD( CRpg, m_cActiveRockets, FIELD_INTEGER ),
};
IMPLEMENT_SAVERESTORE( CRpg, CBasePlayerWeapon );

//=========================================================
//=========================================================
CLaserSpot *CLaserSpot::CreateSpot( void )
{
	CLaserSpot *pSpot = GetClassPtr( (CLaserSpot *)NULL );
	pSpot->Spawn();

	pSpot->pev->classname = MAKE_STRING("laser_spot");

	return pSpot;
}

//=========================================================
//=========================================================
void CLaserSpot::Spawn( void )
{
	Precache( );
	pev->movetype = MOVETYPE_NONE;
	pev->solid = SOLID_NOT;

	pev->rendermode = kRenderGlow;
	pev->renderfx = kRenderFxNoDissipation;
	pev->renderamt = 255;

	SET_MODEL(ENT(pev), "sprites/laserdot.spr");
	UTIL_SetOrigin( pev, pev->origin );
};

//=========================================================
// Suspend- make the laser sight invisible. 
//=========================================================
void CLaserSpot::Suspend( float flSuspendTime )
{
	pev->effects |= EF_NODRAW;
	
	SetThink( Revive );
	pev->nextthink = gpGlobals->time + flSuspendTime;
}

//=========================================================
// Revive - bring a suspended laser sight back.
//=========================================================
void CLaserSpot::Revive( void )
{
	pev->effects &= ~EF_NODRAW;

	SetThink( NULL );
}

void CLaserSpot::Precache( void )
{
	PRECACHE_MODEL("sprites/laserdot.spr");
};





class CRpgRocket : public CGrenade
{
public:
	int		Save( CSave &save );
	int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];
	void Spawn( void );
	void Precache( void );
	void EXPORT FollowThink( void );
	void EXPORT IgniteThink( void );
	void EXPORT RocketTouch( CBaseEntity *pOther );
	static CRpgRocket *CreateRpgRocket( Vector vecOrigin, Vector vecAngles, CBaseEntity *pOwner, CRpg *pLauncher );

	int m_iTrail;
	float m_flIgniteTime;
	CRpg *m_pLauncher;// pointer back to the launcher that fired me. 
};
LINK_ENTITY_TO_CLASS( rocket, CRpgRocket );

TYPEDESCRIPTION	CRpgRocket::m_SaveData[] = 
{
	DEFINE_FIELD( CRpgRocket, m_flIgniteTime, FIELD_TIME ),
	DEFINE_FIELD( CRpgRocket, m_pLauncher, FIELD_CLASSPTR ),
};
IMPLEMENT_SAVERESTORE( CRpgRocket, CGrenade );

//=========================================================
//=========================================================
CRpgRocket *CRpgRocket::CreateRpgRocket( Vector vecOrigin, Vector vecAngles, CBaseEntity *pOwner, CRpg *pLauncher )
{
	CRpgRocket *pRocket = GetClassPtr( (CRpgRocket *)NULL );

	UTIL_SetOrigin( pRocket->pev, vecOrigin );
	pRocket->pev->angles = vecAngles;
	pRocket->Spawn();
	pRocket->SetTouch( CRpgRocket::RocketTouch );
	pRocket->m_pLauncher = pLauncher;// remember what RPG fired me. 
	pRocket->m_pLauncher->m_cActiveRockets++;// register this missile as active for the launcher
	pRocket->pev->owner = pOwner->edict();

	return pRocket;
}

//=========================================================
//=========================================================
void CRpgRocket :: Spawn( void )
{
	Precache( );
	// motor
	pev->movetype = MOVETYPE_BOUNCE;
	pev->solid = SOLID_BBOX;

	SET_MODEL(ENT(pev), "models/rpgrocket.mdl");
	UTIL_SetSize(pev, Vector( 0, 0, 0), Vector(0, 0, 0));
	UTIL_SetOrigin( pev, pev->origin );

	pev->classname = MAKE_STRING("rocket");

	SetThink( IgniteThink );
	SetTouch( ExplodeTouch );

	pev->angles.x -= 30;
	UTIL_MakeVectors( pev->angles );
	pev->angles.x = -(pev->angles.x + 30);

	pev->velocity = gpGlobals->v_forward * 250;
	pev->gravity = 0.5;

	pev->nextthink = gpGlobals->time + 0.4;

	pev->dmg = gSkillData.plrDmgRocket;
}

//=========================================================
//=========================================================
void CRpgRocket :: RocketTouch ( CBaseEntity *pOther )
{
	if ( m_pLauncher )
	{
		m_pLauncher->m_cActiveRockets--;
	}

	STOP_SOUND( edict(), CHAN_VOICE, "weapons/rocket1.wav" );
	ExplodeTouch( pOther );
}

//=========================================================
//=========================================================
void CRpgRocket :: Precache( void )
{
	PRECACHE_MODEL("models/rpgrocket.mdl");
	m_iTrail = PRECACHE_MODEL("sprites/smoke.spr");
	PRECACHE_SOUND ("weapons/rocket1.wav");
}


void CRpgRocket :: IgniteThink( void  )
{

	pev->movetype = MOVETYPE_FLY;
	pev->effects |= EF_LIGHT;

	// make rocket sound
	EMIT_SOUND( ENT(pev), CHAN_VOICE, "weapons/rocket1.wav", 1, 0.5 );

	// rocket trail
	MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );

		WRITE_BYTE( TE_BEAMFOLLOW );
		WRITE_SHORT(entindex());	// entity
		WRITE_SHORT(m_iTrail );	// model
		WRITE_BYTE( 5 ); // life
		WRITE_BYTE( 5 );  // width
		WRITE_BYTE( 224 );   // r, g, b
		WRITE_BYTE( 224 );   // r, g, b
		WRITE_BYTE( 255 );   // r, g, b
		WRITE_BYTE( 255 );	// brightness

	MESSAGE_END();  // move PHS/PVS data sending into here (SEND_ALL, SEND_PVS, SEND_PHS)

	m_flIgniteTime = gpGlobals->time;

	SetThink( FollowThink );
	pev->nextthink = gpGlobals->time + 0.1;
}


void CRpgRocket :: FollowThink( void  )
{
	CBaseEntity *pOther = NULL;
	Vector vecTarget;
	Vector vecDir;
	float flDist, flMax, flDot;
	TraceResult tr;

	UTIL_MakeAimVectors( pev->angles );

	vecTarget = gpGlobals->v_forward;
	flMax = 4096;
	
	// Examine all entities within a reasonable radius
	while ((pOther = UTIL_FindEntityByClassname( pOther, "laser_spot" )) != NULL)
	{
		UTIL_TraceLine ( pev->origin, pOther->pev->origin, dont_ignore_monsters, ENT(pev), &tr );
		// ALERT( at_console, "%f\n", tr.flFraction );
		if (tr.flFraction >= 0.90)
		{
			vecDir = pOther->pev->origin - pev->origin;
			flDist = vecDir.Length( );
			vecDir = vecDir.Normalize( );
			flDot = DotProduct( gpGlobals->v_forward, vecDir );
			if ((flDot > 0) && (flDist * (1 - flDot) < flMax))
			{
				flMax = flDist * (1 - flDot);
				vecTarget = vecDir;
			}
		}
	}

	pev->angles = UTIL_VecToAngles( vecTarget );

	// this acceleration and turning math is totally wrong, but it seems to respond well so don't change it.
	float flSpeed = pev->velocity.Length();
	if (gpGlobals->time - m_flIgniteTime < 1.0)
	{
		pev->velocity = pev->velocity * 0.2 + vecTarget * (flSpeed * 0.8 + 400);
		if (pev->waterlevel == 3)
		{
			// go slow underwater
			if (pev->velocity.Length() > 300)
			{
				pev->velocity = pev->velocity.Normalize() * 300;
			}
			UTIL_BubbleTrail( pev->origin - pev->velocity * 0.1, pev->origin, 4 );
		} 
		else 
		{
			if (pev->velocity.Length() > 2000)
			{
				pev->velocity = pev->velocity.Normalize() * 2000;
			}
		}
	}
	else
	{
		if (pev->effects & EF_LIGHT)
		{
			pev->effects = 0;
			STOP_SOUND( ENT(pev), CHAN_VOICE, "weapons/rocket1.wav" );
		}
		pev->velocity = pev->velocity * 0.2 + vecTarget * flSpeed * 0.798;
		if (pev->waterlevel == 0 && pev->velocity.Length() < 1500)
		{
			Detonate( );
		}
	}
	// ALERT( at_console, "%.0f\n", flSpeed );

	pev->nextthink = gpGlobals->time + 0.1;
}








void CRpg::Reload( void )
{
	int iResult;

	if ( m_iClip == 1 )
	{
		// don't bother with any of this if don't need to reload.
		return;
	}
	
	m_flNextPrimaryAttack = gpGlobals->time + 0.5;

	if ( m_cActiveRockets && m_fSpotActive )
	{
		// no reloading when there are active missiles tracking the designator.
		// ward off future autoreload attempts by setting next attack time into the future for a bit. 
		return;
	}

	if (m_pSpot && m_fSpotActive)
	{
		m_pSpot->Suspend( 2.1 );
		m_flNextSecondaryAttack = gpGlobals->time + 2.1;
	}

	if (m_iClip == 0)
	{
		iResult = DefaultReload( ROCKETL_MAX_CLIP, RPG_RELOAD, 2 );
	}

	if (iResult)
	{
		m_flTimeWeaponIdle = gpGlobals->time + RANDOM_FLOAT ( 10, 15 );
	}
}

void CRpg::Spawn( )
{
	pev->classname = MAKE_STRING("weapon_rocketl"); 
	Precache( );
	m_iId = WEAPON_ROCKETL;

	SET_MODEL(ENT(pev), "models/wmodels/w_rocketl.mdl");
	m_fSpotActive = 1;

	m_iDefaultAmmo = ROCKETL_DEFAULT_GIVE;

	FallInit();
}


void CRpg::Precache( void )
{
	PRECACHE_MODEL("models/wmodels/w_rocketl.mdl");
	PRECACHE_MODEL("models/vmodels/v_rocketl.mdl");
	PRECACHE_MODEL("models/pmodels/p_rocketl.mdl");

	UTIL_PrecacheOther( "laser_spot" );
	UTIL_PrecacheOther( "rocket" );

	PRECACHE_SOUND("weapons/rocketfire1.wav");
	PRECACHE_SOUND("weapons/glauncher.wav");
	PRECACHE_SOUND("weapons/rocketalert.wav");
	PRECACHE_SOUND("weapons/rocketload.wav");
}


int CRpg::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "rocket";
	p->iMaxAmmo1 = ROCKETL_MAX_CARRY;
	p->pszAmmo2 = "heli-rockets";
	p->iMaxAmmo2 = HELIROCKET_MAX_CARRY;
	p->iMaxClip = ROCKETL_MAX_CLIP;
	p->iSlot = 3;
	p->iPosition = 1;
	p->iId = m_iId = WEAPON_ROCKETL;
	p->iFlags = 0;
	p->iWeight = ROCKETL_WEIGHT;

	return 1;
}

int CRpg::AddToPlayer( CBasePlayer *pPlayer )
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

BOOL CRpg::Deploy( )
{
	if ( m_iClip == 0 )
	{
		return DefaultDeploy( "models/vmodels/v_rocketl.mdl", "models/pmodels/p_rocketl.mdl", RPG_DRAW_UL, "egon" );
	}

	return DefaultDeploy( "models/vmodels/v_rocketl.mdl", "models/pmodels/p_rocketl.mdl", RPG_DRAW1, "egon" );
}


BOOL CRpg::CanHolster( void )
{
	if ( m_fSpotActive && m_cActiveRockets )
	{
		// can't put away while guiding a missile.
		return FALSE;
	}

	return TRUE;
}

void CRpg::Holster( )
{
	m_fInReload = FALSE;

	rocket_load = 0;

	m_pPlayer->m_flNextAttack = gpGlobals->time + 0.5;
	SendWeaponAnim( RPG_HOLSTER1 );
	
	if (m_pSpot)
	{
		m_pSpot->Killed( NULL, GIB_NEVER );
		m_pSpot = NULL;
	}
}



void CRpg::PrimaryAttack()
{

	if (m_iClip)
	{
		m_pPlayer->m_iWeaponVolume = LOUD_GUN_VOLUME;
		m_pPlayer->m_iWeaponFlash = BRIGHT_GUN_FLASH;

		SendWeaponAnim( RPG_FIRE2 );

		// player "shoot" animation
		m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

		UTIL_MakeVectors( m_pPlayer->pev->v_angle );
		Vector vecSrc = m_pPlayer->GetGunPosition( ) + gpGlobals->v_forward * 16 + gpGlobals->v_right * 8 + gpGlobals->v_up * -8;
		
		CRpgRocket *pRocket = CRpgRocket::CreateRpgRocket( vecSrc, m_pPlayer->pev->v_angle, m_pPlayer, this );

		UTIL_MakeVectors( m_pPlayer->pev->v_angle );// RpgRocket::Create stomps on globals, so remake.
		pRocket->pev->velocity = pRocket->pev->velocity + gpGlobals->v_forward * DotProduct( m_pPlayer->pev->velocity, gpGlobals->v_forward );

		
		EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/rocketfire1.wav", 0.9, ATTN_NORM );
		EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/glauncher.wav", 0.7, ATTN_NORM );

		m_iClip--; 
			
		if( m_pPlayer->m_iPlayerRune == RUNE_ROCKETARENA )
		{
			m_flNextPrimaryAttack = gpGlobals->time + .5;
		}
		else
		{
			m_flNextPrimaryAttack = gpGlobals->time + 1.5;
		}

			m_flTimeWeaponIdle = gpGlobals->time + 1.5;
			m_pPlayer->pev->punchangle.x -= 5;
	}
	else
	{
		PlayEmptySound( );
		m_flNextPrimaryAttack = gpGlobals->time + .5;
	}
	
	UpdateSpot( );
}


void CRpg::SecondaryAttack()
{
	if ( g_pGameRules->IsRocketArena() == 0 )
	{
		m_fSpotActive = ! m_fSpotActive;

		if (!m_fSpotActive && m_pSpot)
		{
			m_pSpot->Killed( NULL, GIB_NORMAL );
			m_pSpot = NULL;
		}

		m_flNextSecondaryAttack = gpGlobals->time + 0.2;
	}
	else
	{
		if (m_pPlayer->m_rgAmmo[m_iSecondaryAmmoType])
		{
			rocket_load++;

			if ( rocket_load == 1 )
			{
				EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/rocketload.wav", 0.9, ATTN_NORM );
				m_flNextSecondaryAttack = gpGlobals->time + 1.0;
			}
			if ( rocket_load == 2 )
			{
				EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/rocketload.wav", 0.9, ATTN_NORM );
				m_flNextSecondaryAttack = gpGlobals->time + 1.0;
			}
			if ( rocket_load == 3 )
			{
				EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/rocketload.wav", 0.9, ATTN_NORM );
				m_flNextSecondaryAttack = gpGlobals->time + 1.0;
			}
			if ( rocket_load == 4 )
			{
				EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_BODY, "weapons/rocketalert.wav", 0.9, ATTN_NORM );
			
				m_pPlayer->m_iWeaponVolume = LOUD_GUN_VOLUME;
				m_pPlayer->m_iWeaponFlash = BRIGHT_GUN_FLASH;

				SendWeaponAnim( RPG_FIRE2 );

				// player "shoot" animation
				m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

				UTIL_MakeVectors( m_pPlayer->pev->v_angle );
			
				Vector vecSrc1 = m_pPlayer->GetGunPosition( ) + gpGlobals->v_forward * 16 + gpGlobals->v_right * -8 + gpGlobals->v_up * -8;
				Vector vecSrc2 = m_pPlayer->GetGunPosition( ) + gpGlobals->v_forward * 16 + gpGlobals->v_right * 0 + gpGlobals->v_up * -8;
				Vector vecSrc3 = m_pPlayer->GetGunPosition( ) + gpGlobals->v_forward * 16 + gpGlobals->v_right * 8 + gpGlobals->v_up * -8;
			
				CRpgRocket *pRocket = CRpgRocket::CreateRpgRocket( vecSrc1, m_pPlayer->pev->v_angle, m_pPlayer, this );
									  CRpgRocket::CreateRpgRocket( vecSrc2, m_pPlayer->pev->v_angle, m_pPlayer, this );
				                      CRpgRocket::CreateRpgRocket( vecSrc3, m_pPlayer->pev->v_angle, m_pPlayer, this );
			
				UTIL_MakeVectors( m_pPlayer->pev->v_angle );
				pRocket->pev->velocity = pRocket->pev->velocity + gpGlobals->v_forward * DotProduct( m_pPlayer->pev->velocity, gpGlobals->v_forward );

		
				EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/rocketfire1.wav", 0.9, ATTN_NORM );
				EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/glauncher.wav", 0.7, ATTN_NORM );

				m_pPlayer->m_rgAmmo[m_iSecondaryAmmoType]--;

				rocket_load = 0;
		
				m_flNextSecondaryAttack = gpGlobals->time + 1.5;
				m_flTimeWeaponIdle = gpGlobals->time + 1.5;
				m_pPlayer->pev->punchangle.x -= 5;
			}
		}
	}

}


void CRpg::WeaponIdle( void )
{
	UpdateSpot( );

	ResetEmptySound( );

	if (m_flTimeWeaponIdle > gpGlobals->time)
		return;

	if (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType])
	{
		int iAnim;
		float flRand = RANDOM_FLOAT(0, 1);
		
		if (flRand <= 0.75 || m_fSpotActive)
		{
			if ( m_iClip == 0 )
				iAnim = RPG_IDLE_UL;
			else
				iAnim = RPG_IDLE;

			m_flTimeWeaponIdle = gpGlobals->time + 90.0 / 15.0;
		}
		else
		{
			if ( m_iClip == 0 )
				iAnim = RPG_FIDGET_UL;
			else
				iAnim = RPG_FIDGET;

			m_flTimeWeaponIdle = gpGlobals->time + 3.0;
		}

		SendWeaponAnim( iAnim );
	}
	else
	{
		m_flTimeWeaponIdle = gpGlobals->time + 1;
	}
}



void CRpg::UpdateSpot( void )
{
	if (m_fSpotActive)
	{
		if (!m_pSpot)
		{
			m_pSpot = CLaserSpot::CreateSpot();
		}

		UTIL_MakeVectors( m_pPlayer->pev->v_angle );
		Vector vecSrc = m_pPlayer->GetGunPosition( );;
		Vector vecAiming = gpGlobals->v_forward;

		TraceResult tr;
		UTIL_TraceLine ( vecSrc, vecSrc + vecAiming * 8192, dont_ignore_monsters, ENT(m_pPlayer->pev), &tr );
		
		/*
		ALERT( "%f %f\n", gpGlobals->v_forward.y, vecAiming.y );
		
		char * a = gpGlobals->v_forward.y * vecAiming.y + gpGlobals->v_forward.x * vecAiming.x;
		m_pPlayer->pev->punchangle.y = acos( a ) * (180 / M_PI);

		ShowMenu (m_pPlayer, 0x0, 2, 0, a );
			
	
		ALERT( at_console, "%f\n", a );
		*/

		UTIL_SetOrigin( m_pSpot->pev, tr.vecEndPos );
	}
}


class CRocketAmmo : public CBasePlayerAmmo
{
	void Spawn( void )
	{ 
		Precache( );
		SET_MODEL(ENT(pev), "models/ammo/w_rocketl.mdl");
		CBasePlayerAmmo::Spawn( );
	}
	void Precache( void )
	{
		PRECACHE_MODEL ("models/ammo/w_rocketl.mdl");
		PRECACHE_SOUND("items/9mmclip1.wav");
	}
	BOOL AddAmmo( CBaseEntity *pOther ) 
	{ 
		int iGive;

		iGive = AMMO_ROCKET_GIVE;

		if (pOther->GiveAmmo( iGive, "rocket", ROCKETL_MAX_CARRY ) != -1)
		{
			EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM);
			return TRUE;
		}
		return FALSE;
	}
};
LINK_ENTITY_TO_CLASS( ammo_rocket, CRocketAmmo );

class CHeliRocketAmmo : public CBasePlayerAmmo
{
	void Spawn( void )
	{ 
		Precache( );
		SET_MODEL(ENT(pev), "models/ammo/w_rocketl.mdl");
		CBasePlayerAmmo::Spawn( );
	}
	void Precache( void )
	{
		PRECACHE_MODEL ("models/ammo/w_rocketl.mdl");
		PRECACHE_SOUND("items/9mmclip1.wav");
	}
	BOOL AddAmmo( CBaseEntity *pOther ) 
	{ 

		if (pOther->GiveAmmo( HELIROCKET_MAX_CARRY, "heli-rockets", HELIROCKET_MAX_CARRY ) != -1)
		{
			EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM);
			return TRUE;
		}
		return FALSE;
	}
};
LINK_ENTITY_TO_CLASS( ammo_helirockets, CHeliRocketAmmo );
