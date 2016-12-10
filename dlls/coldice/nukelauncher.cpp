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
#include "shake.h" 



enum nuke_e {
	NUKE_IDLE = 0,
	NUKE_FIDGET,
	NUKE_RELOAD,	// to reload
	NUKE_FIRE2,		// to empty
	NUKE_HOLSTER1,	// loaded
	NUKE_DRAW1,		// loaded
	NUKE_HOLSTER2,	// unloaded
	NUKE_DRAW_UL,	// unloaded
	NUKE_IDLE_UL,	// unloaded idle
	NUKE_FIDGET_UL,	// unloaded fidget
};


class CNuke : public CBasePlayerWeapon
{
public:

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
	BOOL ShouldWeaponIdle( void ) { return TRUE; };

};
LINK_ENTITY_TO_CLASS( weapon_nuke, CNuke );
//=========================================================
//=========================================================

class CNukeRocket : public CGrenade
{
public:
	int		Save( CSave &save );
	int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];
	void Spawn( void );
	void Precache( void );
	void EXPORT IgniteThink( void );	
	void EXPORT BlowMeUp( void );
	void EXPORT FollowThink( void );
	void EXPORT RocketTouch( CBaseEntity *pOther );
	static CNukeRocket *CreateNukeRocket( Vector vecOrigin, Vector vecAngles, CBaseEntity *pOwner, CNuke *pLauncher );

	int m_iSpriteTexture;
	int m_iExplode;
	int m_iBalls;

	int killit;

	int m_iTrail;
	float m_flIgniteTime;
	CNuke *m_pLauncher;
};
LINK_ENTITY_TO_CLASS( nuke, CNukeRocket );

TYPEDESCRIPTION	CNukeRocket::m_SaveData[] = 
{
	DEFINE_FIELD( CNukeRocket, m_flIgniteTime, FIELD_TIME ),
	DEFINE_FIELD( CNukeRocket, m_pLauncher, FIELD_CLASSPTR ),
};
IMPLEMENT_SAVERESTORE( CNukeRocket, CGrenade );

//=========================================================
//=========================================================
CNukeRocket *CNukeRocket::CreateNukeRocket( Vector vecOrigin, Vector vecAngles, CBaseEntity *pOwner, CNuke *pLauncher )
{
	CNukeRocket *pNuke = GetClassPtr( (CNukeRocket *)NULL );

	UTIL_SetOrigin( pNuke->pev, vecOrigin );
	pNuke->pev->angles = vecAngles;
	pNuke->Spawn();
	pNuke->SetTouch( CNukeRocket::RocketTouch );
	pNuke->m_pLauncher = pLauncher; 
	pNuke->pev->owner = pOwner->edict();

	return pNuke;
}

//=========================================================
//=========================================================
void CNukeRocket :: Spawn( void )
{
	Precache( );
	// motor
	pev->movetype = MOVETYPE_BOUNCE;
	pev->solid = SOLID_BBOX;

	SET_MODEL(ENT(pev), "models/nuke.mdl");
	UTIL_SetSize(pev, Vector( 0, 0, 0), Vector(0, 0, 0));
	UTIL_SetOrigin( pev, pev->origin );

	pev->classname = MAKE_STRING("nuke");

	SetThink( IgniteThink );
	SetTouch( ExplodeTouch );

	pev->angles.x -= 30;
	UTIL_MakeVectors( pev->angles );
	pev->angles.x = -(pev->angles.x + 30);

	pev->velocity = gpGlobals->v_forward * 250;
	pev->gravity = 0.5;

	pev->nextthink = gpGlobals->time + 0.4;

}

//=========================================================
//=========================================================
void CNukeRocket :: RocketTouch ( CBaseEntity *pOther )
{
	STOP_SOUND( edict(), CHAN_VOICE, "weapons/rocket1.wav" );
//	ExplodeTouch( pOther );

	//initialize a vector that finds the center of the RPG models hitbox
	Vector vecSpot = pev->origin + (pev->mins + pev->maxs) * 0.5;

	killit++;
 
	if ( killit == 1 )	
	{
		// blast circle "The Infamous Disc of Death"
		MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, pev->origin );
		WRITE_BYTE( TE_BEAMCYLINDER );
		WRITE_COORD( pev->origin.x);//center of effect on x axis
		WRITE_COORD( pev->origin.y);//center of effect on y axis
		WRITE_COORD( pev->origin.z);//center of effect on z axis
		WRITE_COORD( pev->origin.x);//axis of effect on x axis
		WRITE_COORD( pev->origin.y);//axis of effect on y axis
		WRITE_COORD( pev->origin.z + 300 ); // z axis and Radius of effect
		WRITE_SHORT( m_iSpriteTexture );//Name of the sprite to use, as defined at begining of tut
		WRITE_BYTE( 0 ); // startframe
		WRITE_BYTE( 0 ); //framerate in 0.1's
		WRITE_BYTE( 40 ); //Life in 0.1's
		WRITE_BYTE( 302 ); //Line Width in .1 units
		WRITE_BYTE( 0 ); //Noise Amplitude in 0.01's
		WRITE_BYTE( 255 ); // Red Color Value
		WRITE_BYTE( 255 ); // Green Color Value
		WRITE_BYTE( 192 ); // Blue Color Value
		WRITE_BYTE( 128 ); // brightness
		WRITE_BYTE( 0 ); // speed
		MESSAGE_END();


		pev->effects |= EF_NODRAW;//stop showing the model!!
		pev->velocity = g_vecZero;//set velocity to "0"

		UTIL_ScreenShake( pev->origin, 500.0, 400.0, 6.0, 1800 );
	
		entvars_t	*pevOwner;
	
		if ( pev->owner )
			pevOwner = VARS( pev->owner );
		else
			pevOwner = NULL;

		pev->owner = NULL; // can't traceline attack owner if this is set

		/*Big Damage 
		The first parameter is the center of where the damage radiates from. The second is the inflictor
		( or what caused it ). The third is the attacker (or what caused this ). The fourth parameter is the
		damage to inflict, the fifth is the class to ignore ( or what entity not apply damage to ). The final
		parameter is the damage type--(with multiple damage types delineated by a | ).*/
		::RadiusDamage( pev->origin, pev, pevOwner, 400, 1000, CLASS_NONE, DMG_BLAST | DMG_BURN | DMG_ALWAYSGIB ); 

		EMIT_SOUND(ENT(pev), CHAN_STATIC, "weapons/nuke1.wav", 1.0, 0.3);


		MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, pev->origin );
		WRITE_BYTE( TE_SPRITE );//additive sprite plays though once
		WRITE_COORD( pev->origin.x );//where to make the sprite appear on x axis
		WRITE_COORD( pev->origin.y );//where to make the sprite appear on y axis
		WRITE_COORD( pev->origin.z + 128 );//Creates sprite 128 units above model's center
		WRITE_SHORT( m_iExplode );//Name of the sprite to use, as defined at begining of tut
		WRITE_BYTE( 400 ); // scale in .1 units --by comparison the player is 72 units tall 
		WRITE_BYTE( 255 ); // brightness (this is as bright as it gets)
		MESSAGE_END();

	
		UTIL_Remove( this );

	}
	
		killit = 0;	
}

//=========================================================
//=========================================================
void CNukeRocket :: Precache( void )
{
	PRECACHE_MODEL("models/nuke.mdl");
	m_iTrail = PRECACHE_MODEL("sprites/blueflare1.spr");
	PRECACHE_SOUND ("weapons/rocket1.wav");

	PRECACHE_SOUND("weapons/mortarhit.wav");//precache the Big Booming Sound
	m_iSpriteTexture = PRECACHE_MODEL( "sprites/white.spr" );//precache the sprite for disc
	m_iBalls = PRECACHE_MODEL("sprites/blueflare1.spr");
	m_iExplode = PRECACHE_MODEL( "sprites/fexplo.spr" );//precache the sprite for explosion
}


void CNukeRocket :: IgniteThink( void  )
{

	pev->movetype = MOVETYPE_FLY;
	pev->effects |= EF_LIGHT;

	EMIT_SOUND( ENT(pev), CHAN_VOICE, "weapons/rocket1.wav", 1, 0.5 );

	
	MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );

		WRITE_BYTE( TE_BEAMFOLLOW );
		WRITE_SHORT(entindex());	// entity
		WRITE_SHORT(m_iTrail );	// model
		WRITE_BYTE( 10 ); // life
		WRITE_BYTE( 9 );  // width
		WRITE_BYTE( 224 );   // r, g, b
		WRITE_BYTE( 224 );   // r, g, b
		WRITE_BYTE( 255 );   // r, g, b
		WRITE_BYTE( 128 );	// brightness

	MESSAGE_END(); 



	m_flIgniteTime = gpGlobals->time;

	SetThink( FollowThink );
	pev->nextthink = gpGlobals->time + 0.1;
}


void CNukeRocket :: FollowThink( void  )
{
	CBaseEntity *pOther = NULL;
	Vector vecTarget;
	Vector vecDir;
	float flMax;
	TraceResult tr;

	UTIL_MakeAimVectors( pev->angles );

	vecTarget = gpGlobals->v_forward;
	flMax = 4096;

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




void CNuke::Reload( void )
{
	int iResult;

	if ( m_iClip == 1 )
	{
		return;
	}
	
	m_flNextPrimaryAttack = gpGlobals->time + 0.5;


	if (m_iClip == 0)
	{
		iResult = DefaultReload( NUKE_MAX_CLIP, NUKE_RELOAD, 2 );
	}

	if (iResult)
	{
		m_flTimeWeaponIdle = gpGlobals->time + RANDOM_FLOAT ( 10, 15 );
	}
}

void CNuke::Spawn( )
{
	pev->classname = MAKE_STRING("weapon_nuke"); 
	Precache( );
	m_iId = WEAPON_NUKE;

	SET_MODEL(ENT(pev), "models/wmodels/w_nuke.mdl");

	m_iDefaultAmmo = NUKE_DEFAULT_GIVE;

	FallInit();
}


void CNuke::Precache( void )
{
	PRECACHE_MODEL("models/wmodels/w_nuke.mdl");
	PRECACHE_MODEL("models/vmodels/v_nuke.mdl");
	PRECACHE_MODEL("models/pmodels/p_nuke.mdl");

	PRECACHE_SOUND("items/9mmclip1.wav");

	UTIL_PrecacheOther( "nuke" );

	PRECACHE_SOUND("weapons/rocketfire1.wav");
	PRECACHE_SOUND("weapons/glauncher.wav"); 
	PRECACHE_SOUND("weapons/nuke1.wav"); 
}


int CNuke::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "nuke";
	p->iMaxAmmo1 = NUKE_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = NUKE_MAX_CLIP;
	p->iSlot = 3;
	p->iPosition = 4;
	p->iId = m_iId = WEAPON_NUKE;
	p->iFlags = 0;
	p->iWeight = NUKE_WEIGHT;

	return 1;
}

int CNuke::AddToPlayer( CBasePlayer *pPlayer )
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

BOOL CNuke::Deploy( )
{
	if ( m_iClip == 0 )
	{
		return DefaultDeploy( "models/vmodels/v_nuke.mdl", "models/pmodels/p_nuke.mdl", NUKE_DRAW_UL, "egon" );
	}

	return DefaultDeploy( "models/vmodels/v_nuke.mdl", "models/pmodels/p_nuke.mdl", NUKE_DRAW1, "egon" );
}


BOOL CNuke::CanHolster( void )
{
	return TRUE;
}

void CNuke::Holster( )
{
	m_fInReload = FALSE;

	m_pPlayer->m_flNextAttack = gpGlobals->time + 0.5;

	SendWeaponAnim( NUKE_HOLSTER1 );
}



void CNuke::PrimaryAttack()
{
	if (m_iClip)
	{
		m_pPlayer->m_iWeaponVolume = LOUD_GUN_VOLUME;
		m_pPlayer->m_iWeaponFlash = BRIGHT_GUN_FLASH;

		SendWeaponAnim( NUKE_FIRE2 );

		m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

		UTIL_MakeVectors( m_pPlayer->pev->v_angle );
		Vector vecSrc = m_pPlayer->GetGunPosition( ) + gpGlobals->v_forward * 16 + gpGlobals->v_right * 8 + gpGlobals->v_up * -8;
		
		CNukeRocket * pNuke = CNukeRocket::CreateNukeRocket( vecSrc, m_pPlayer->pev->v_angle, m_pPlayer, this );

		UTIL_MakeVectors( m_pPlayer->pev->v_angle );
		pNuke->pev->velocity = pNuke->pev->velocity + gpGlobals->v_forward * DotProduct( m_pPlayer->pev->velocity, gpGlobals->v_forward );
		
		EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/rocketfire1.wav", 0.9, ATTN_NORM );
		EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/glauncher.wav", 0.7, ATTN_NORM );

		m_iClip--; 

		m_flNextPrimaryAttack = gpGlobals->time + 1.5;
		m_flTimeWeaponIdle = gpGlobals->time + 1.5;
		m_pPlayer->pev->punchangle.x -= 5;
	}
	else
	{
		PlayEmptySound( );
		m_flNextPrimaryAttack = gpGlobals->time + .15;
	}

}


void CNuke::SecondaryAttack()
{
	
}


void CNuke::WeaponIdle( void )
{

	ResetEmptySound( );

	if (m_flTimeWeaponIdle > gpGlobals->time)
		return;

	if (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType])
	{
		int iAnim;
		float flRand = RANDOM_FLOAT(0, 1);
		if (flRand <= 0.75 )
		{
			if ( m_iClip == 0 )
				iAnim = NUKE_IDLE_UL;
			else
				iAnim = NUKE_IDLE;

			m_flTimeWeaponIdle = gpGlobals->time + 90.0 / 15.0;
		}
		else
		{
			if ( m_iClip == 0 )
				iAnim = NUKE_FIDGET_UL;
			else
				iAnim = NUKE_FIDGET;

			m_flTimeWeaponIdle = gpGlobals->time + 3.0;
		}

		SendWeaponAnim( iAnim );
	}
	else
	{
		m_flTimeWeaponIdle = gpGlobals->time + 1;
	}
}





class CNukeAmmo : public CBasePlayerAmmo
{
	void Spawn( void )
	{ 
		Precache( );
		SET_MODEL(ENT(pev), "models/w_rpgammo.mdl");
		CBasePlayerAmmo::Spawn( );
	}
	void Precache( void )
	{
		PRECACHE_MODEL ("models/w_rpgammo.mdl");
		PRECACHE_SOUND("items/9mmclip1.wav");
	}
	BOOL AddAmmo( CBaseEntity *pOther ) 
	{ 
		int iGive;
		iGive = AMMO_NUKECLIP_GIVE;
	
		if (pOther->GiveAmmo( iGive, "nuke", NUKE_MAX_CARRY ) != -1)
		{
			EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM);
			return TRUE;
		}
		return FALSE;
	}
};
LINK_ENTITY_TO_CLASS( ammo_nukeclip, CNukeAmmo );
