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
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"
#include "gamerules.h"

#ifndef CLIENT_DLL
#define BOLT_AIR_VELOCITY	4000 //2���ٷ���
#define BOLT_WATER_VELOCITY	2000

// UNDONE: Save/restore this?  Don't forget to set classname and LINK_ENTITY_TO_CLASS()
// 
// OVERLOADS SOME ENTVARS:
//
// speed - the ideal magnitude of my velocity
class CCrossbowBolt : public CBaseEntity
{
	void Spawn( void );
	void Precache( void );
	int  Classify ( void );
	void EXPORT BubbleThink( void );
	void EXPORT BoltTouch( CBaseEntity *pOther );
	void EXPORT ExplodeThink( void );

	int m_iTrail;

public:
	static CCrossbowBolt *BoltCreate( void );
};
LINK_ENTITY_TO_CLASS( crossbow_bolt, CCrossbowBolt );

CCrossbowBolt *CCrossbowBolt::BoltCreate( void )
{
	// Create a new entity with CCrossbowBolt private data
	CCrossbowBolt *pBolt = GetClassPtr( (CCrossbowBolt *)NULL );
	pBolt->pev->classname = MAKE_STRING("bolt");
	pBolt->Spawn();

	return pBolt;
}

void CCrossbowBolt::Spawn( )
{
	Precache( );
	pev->movetype = MOVETYPE_FLY;
	pev->solid = SOLID_BBOX;

	pev->gravity = 0;

	SET_MODEL(ENT(pev), "models/crossbow_bolt.mdl");

	UTIL_SetOrigin( pev, pev->origin );
	UTIL_SetSize(pev, Vector(0, 0, 0), Vector(0, 0, 0));

	SetTouch( &CCrossbowBolt::BoltTouch );
	SetThink( &CCrossbowBolt::BubbleThink );
	pev->nextthink = gpGlobals->time + 0.1;

	MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );

		WRITE_BYTE( TE_BEAMFOLLOW );
		WRITE_SHORT(entindex());	// entity
		WRITE_SHORT(m_iTrail );	// model
		WRITE_BYTE( 1 ); // life
		WRITE_BYTE( 1 );  // width
		WRITE_BYTE( 255 );   // r, g, b
		WRITE_BYTE( 224 );   // r, g, b
		WRITE_BYTE( 224 );   // r, g, b
		WRITE_BYTE( 255 );	// brightness

	MESSAGE_END();  // move PHS/PVS data sending into here (SEND_ALL, SEND_PVS, SEND_PHS)

	pev->dmg = 60;
}


void CCrossbowBolt::Precache( )
{
	PRECACHE_MODEL ("models/crossbow_bolt.mdl");
	PRECACHE_SOUND("weapons/xbolt_explode.wav");
	m_iTrail = PRECACHE_MODEL("sprites/streak.spr");
}


int	CCrossbowBolt :: Classify ( void )
{
	return	CLASS_NONE;
}

void CCrossbowBolt::BoltTouch( CBaseEntity *pOther )
{
	//SetTouch( NULL );
	SetThink( NULL );

	#ifndef CLIENT_DLL
		float flSpeed = pev->velocity.Length();

		if(pev->movetype == MOVETYPE_FLY){
			FX_Explosion( pev->origin, EXPLOSION_BOLT);

			

					TraceResult tr;
					UTIL_MakeVectors(pev->angles);
					Vector vecSrc	= Center();
					Vector vecEnd	= vecSrc + gpGlobals->v_forward * 30;
					UTIL_TraceLine( vecSrc, vecEnd, dont_ignore_monsters, ENT( pev ), &tr );

					CBaseEntity *pEntity = CBaseEntity::Instance(tr.pHit);

					int cteam = Classify();

					entvars_t	*pevOwner;
					pevOwner = VARS( pev->owner );
					if ( !FNullEnt(pevOwner) && (pevOwner->flags & FL_CLIENT) ){
					pev->dmg = 80;
					cteam = CLASS_PLAYER;
					}

					int tex = (int)TEXTURETYPE_Trace(&tr, vecSrc, vecEnd);
					int surface = (int)SURFACETYPE_Trace(&tr, vecSrc, vecEnd,cteam,1);
					FX_ImpBullet( tr.vecEndPos, tr.vecPlaneNormal, vecSrc, surface, BULLET_CROWBAR, (float)tex );

					if (pOther->pev->takedamage){
						ClearMultiDamage( );

						pOther->TraceAttack(pevOwner, pev->dmg, gpGlobals->v_forward, &tr, DMG_BULLET | DMG_NEVERGIB ); 
						ApplyMultiDamage( pev, pevOwner );

						UTIL_Remove( this );
						return;
					}
					else{
						if ( FClassnameIs( pOther->pev, "worldspawn" )
						|| FClassnameIs( pOther->pev, "func_wall" ))
						{
							UTIL_Sparks( pev->origin );
							// if what we hit is static architecture, can stay around for a while.
							Vector vecDir = pev->velocity.Normalize( );
							UTIL_SetOrigin( pev, pev->origin - vecDir * 12 );
							pev->angles = UTIL_VecToAngles( vecDir );
							pev->solid = SOLID_NOT;
							pev->movetype = MOVETYPE_FLY;
							pev->velocity = Vector( 0, 0, 0 );
							pev->avelocity.z = 0;
							pev->angles.z = RANDOM_LONG(0,360);
							pev->nextthink = gpGlobals->time + 10.0;
						}
						else{
							UTIL_Remove( this );
							return;
						}
					}
						

		}


		SetThink( &CCrossbowBolt::SUB_Remove );
		pev->nextthink = gpGlobals->time + 10.0;
	#endif

		

	//	pev->velocity = Vector( 0, 0, 0 );
	//	Killed( pev, GIB_NEVER );
}

void CCrossbowBolt::BubbleThink( void )
{
	pev->nextthink = gpGlobals->time + 0.05;

	if (pev->waterlevel == 0)
		return;

	UTIL_BubbleTrail( pev->origin - pev->velocity * 0.1, pev->origin, 1 );
}

void CCrossbowBolt::ExplodeThink( void )
{
	UTIL_Remove(this);
}
#endif

enum crossbow_e {
	CROSSBOW_IDLE1 = 0,	// full
	CROSSBOW_IDLE2,		// empty
	CROSSBOW_FIDGET1,	// full
	CROSSBOW_FIDGET2,	// empty
	CROSSBOW_FIRE1,		// full
	CROSSBOW_FIRE2,		// reload
	CROSSBOW_FIRE3,		// empty
	CROSSBOW_RELOAD,	// from empty
	CROSSBOW_DRAW1,		// full
	CROSSBOW_DRAW2,		// empty
	CROSSBOW_HOLSTER1,	// full
	CROSSBOW_HOLSTER2,	// empty
};

LINK_ENTITY_TO_CLASS( weapon_crossbow, CCrossbow );

void CCrossbow::Spawn( )
{
	Precache( );
	m_iId = WEAPON_CROSSBOW;
	SET_MODEL(ENT(pev), "models/w_all_items2.mdl");
	pev->body = 4;

	m_iDefaultAmmo = CROSSBOW_DEFAULT_GIVE;

	FallInit();// get ready to fall down.
}

int CCrossbow::AddToPlayer( CBasePlayer *pPlayer )
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

void CCrossbow::Precache( void )
{
	PRECACHE_MODEL("models/v_crossbow.mdl");

	PRECACHE_SOUND("weapons/xbow_fire1.wav");
	PRECACHE_SOUND("weapons/xbow_reload1.wav");

	UTIL_PrecacheOther( "crossbow_bolt" );

	m_usCrossbow = PRECACHE_EVENT( 1, "events/crossbow1.sc" );
}


int CCrossbow::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "bolts";
	p->iMaxAmmo1 = BOLT_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = CROSSBOW_MAX_CLIP;
	p->iSlot = 2;
	p->iPosition = 2;
	p->iId = WEAPON_CROSSBOW;
	p->iFlags = 0;
	p->iWeight = CROSSBOW_WEIGHT;
	return 1;
}


BOOL CCrossbow::Deploy( )
{
	m_pPlayer->m_newcross_active = 0;
	if (m_iClip)
		return DefaultDeploy( "models/v_crossbow.mdl", 0, CROSSBOW_DRAW1, "bow" );
	return DefaultDeploy( "models/v_crossbow.mdl", 0, CROSSBOW_DRAW2, "bow" );
}

void CCrossbow::Holster( int skiplocal /* = 0 */ )
{
	m_fInReload = FALSE;// cancel any reload in progress.

	if ( m_fInZoom )
	{
		m_pPlayer->pev->fov = m_pPlayer->m_iFOV = 0; // 0 means reset to default fov
		m_fInZoom = 0;
		m_pPlayer->m_newcross_active = 0;
	}

	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;
	if (m_iClip)
		SendWeaponAnim( CROSSBOW_HOLSTER1 );
	else
		SendWeaponAnim( CROSSBOW_HOLSTER2 );
}

void CCrossbow::PrimaryAttack( void )
{
	FireBolt();
}

// this function only gets called in multiplayer
void CCrossbow::FireSniperBolt()
{
	return;
}

void CCrossbow::FireBolt()
{
	TraceResult tr;

	if (m_iClip == 0)
	{
		PlayEmptySound( );
		return;
	}

	m_pPlayer->m_iWeaponVolume = QUIET_GUN_VOLUME;

	m_iClip--;

	int flags;
#if defined( CLIENT_WEAPONS )
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif

	PLAYBACK_EVENT_FULL( flags, m_pPlayer->edict(), m_usCrossbow, 0.0, (float *)&g_vecZero, (float *)&g_vecZero, 0, 0, m_iClip, m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType], 0, 0 );

	// player "shoot" animation
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

	Vector anglesAim = m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle;
	UTIL_MakeVectors( anglesAim );
	
	anglesAim.x		= -anglesAim.x;
	Vector vecSrc	 = m_pPlayer->GetGunPosition( ) + gpGlobals->v_forward * 16;
	Vector vecDir	 = gpGlobals->v_forward;

#ifndef CLIENT_DLL
	CCrossbowBolt *pBolt = CCrossbowBolt::BoltCreate();
	pBolt->pev->origin = vecSrc;
	pBolt->pev->angles = anglesAim;
	pBolt->pev->owner = m_pPlayer->edict();

	if (m_pPlayer->pev->waterlevel == 3)
	{
		pBolt->pev->velocity = vecDir * BOLT_WATER_VELOCITY;
		pBolt->pev->speed = BOLT_WATER_VELOCITY;
	}
	else
	{
		pBolt->pev->velocity = vecDir * BOLT_AIR_VELOCITY;
		pBolt->pev->speed = BOLT_AIR_VELOCITY;
	}
	pBolt->pev->avelocity.z = 10;

	if(m_pPlayer->m_darkposion > 0){
	m_pPlayer->m_darkposion = 0;//����ȡ��
	}
#endif

	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 1.0;

	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 1.0;

	if (m_iClip != 0)
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 5.0;
	else
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.5;
}


void CCrossbow::SecondaryAttack()
{
	if ( m_pPlayer->pev->fov != 0 )
	{
		m_pPlayer->pev->fov = m_pPlayer->m_iFOV = 0; // 0 means reset to default fov
		m_fInZoom = 0;
		m_pPlayer->m_newcross_active = 0;
	}
	else if ( m_pPlayer->pev->fov != 20 )
	{
		EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/sniper_zoom.wav", 1, ATTN_NORM);
		m_pPlayer->pev->fov = m_pPlayer->m_iFOV = 20;
		m_fInZoom = 1;
		m_pPlayer->m_newcross_active = 1;
	}
	
	pev->nextthink = UTIL_WeaponTimeBase() + 0.1;
	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 1.0;
}


void CCrossbow::Reload( void )
{
	if ( m_pPlayer->ammo_bolts <= 0 )
		return;

	if ( m_pPlayer->pev->fov != 0 )
	{
		m_pPlayer->pev->fov = m_pPlayer->m_iFOV = 0; // 0 means reset to default fov
		m_fInZoom = 0;
	}

	if ( DefaultReload( 5, CROSSBOW_RELOAD, 4.5 ) )
	{
		EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/xbow_reload1.wav", RANDOM_FLOAT(0.95, 1.0), ATTN_NORM, 0, 93 + RANDOM_LONG(0,0xF));
	}
}


void CCrossbow::WeaponIdle( void )
{
	m_pPlayer->GetAutoaimVector( AUTOAIM_2DEGREES );  // get the autoaim vector but ignore it;  used for autoaim crosshair in DM

	ResetEmptySound( );
	
	if ( m_flTimeWeaponIdle < UTIL_WeaponTimeBase() )
	{
		float flRand = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 0, 1 );
		if (flRand <= 0.75)
		{
			if (m_iClip)
			{
				SendWeaponAnim( CROSSBOW_IDLE1 );
			}
			else
			{
				SendWeaponAnim( CROSSBOW_IDLE2 );
			}
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
		}
		else
		{
			if (m_iClip)
			{
				SendWeaponAnim( CROSSBOW_FIDGET1 );
				m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 90.0 / 30.0;
			}
			else
			{
				SendWeaponAnim( CROSSBOW_FIDGET2 );
				m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 80.0 / 30.0;
			}
		}
	}
}



class CCrossbowAmmo : public CBasePlayerAmmo
{
	void Spawn( void )
	{ 
		Precache( );
		SET_MODEL(ENT(pev), "models/w_all_items1.mdl");
		pev->body = 10;
		CBasePlayerAmmo::Spawn( );
	}
	void Precache( void )
	{
		PRECACHE_MODEL ("models/w_crossbow_clip.mdl");
		PRECACHE_SOUND("items/9mmclip1.wav");
	}
	BOOL AddAmmo( CBaseEntity *pOther ) 
	{ 
		if (pOther->GiveAmmo( AMMO_CROSSBOWCLIP_GIVE, "bolts", BOLT_MAX_CARRY ) != -1)
		{
			EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM);
			return TRUE;
		}
		return FALSE;
	}
};
LINK_ENTITY_TO_CLASS( ammo_crossbow, CCrossbowAmmo );



#endif