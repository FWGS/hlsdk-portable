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
#include "effects.h"
#include "gamerules.h"

enum kamakaze_e {
	SATCHEL_RADIO_IDLE1 = 0,
	SATCHEL_RADIO_FIDGET1,
	SATCHEL_RADIO_DRAW,
	SATCHEL_RADIO_FIRE,
	SATCHEL_RADIO_HOLSTER
};

class CKamikaze : public CBasePlayerWeapon
{
public:
	void Spawn( void );
	void Precache( void );
	int iItemSlot( void ) { return 5; }
	int GetItemInfo(ItemInfo *p);
	void PrimaryAttack( void );
	BOOL Deploy( void );
	void Holster( void );
	void WeaponIdle( void );
    int m_iExplode;
};
LINK_ENTITY_TO_CLASS( weapon_tnt, CKamikaze );
LINK_ENTITY_TO_CLASS( weapon_satchel, CKamikaze );


void CKamikaze::Spawn( )
{
	pev->classname = MAKE_STRING("weapon_tnt"); 
	Precache( );
	m_iId = WEAPON_TNT;
	SET_MODEL(ENT(pev), "models/wmodels/w_tnt.mdl");

	FallInit();

	m_iDefaultAmmo = TNT_DEFAULT_GIVE;
	 
}

void CKamikaze::Precache( void )
{
	PRECACHE_MODEL ("models/vmodels/v_kamikaze.mdl");
	PRECACHE_MODEL ("models/pmodels/p_tnt.mdl");
	PRECACHE_MODEL ("models/wmodels/w_tnt.mdl");
	PRECACHE_SOUND ("weapons/tnt1.wav");
	PRECACHE_SOUND ("items/tntvest.wav");

    m_iExplode = PRECACHE_MODEL( "sprites/fexplo.spr" );

}

int CKamikaze::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "vest";
	p->iMaxAmmo1 = TNT_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = WEAPON_NOCLIP;
	p->iSlot = 4;
	p->iPosition = 2;
	p->iId = m_iId = WEAPON_TNT;
	p->iWeight = TNT_WEIGHT;
	p->iFlags = 0;     

	return 1;
}

BOOL CKamikaze::Deploy( )
{
	EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "items/tntvest.wav", RANDOM_FLOAT(0.9, 1.0), ATTN_NORM);
	return DefaultDeploy( "models/vmodels/v_kamikaze.mdl", "models/pmodels/p_tnt.mdl", SATCHEL_RADIO_DRAW, "hive" );
}


void CKamikaze::Holster( )
{

	m_pPlayer->m_flNextAttack = gpGlobals->time + 0.5;

	if (!m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType])
	{
		m_pPlayer->pev->weapons &= ~(1<<WEAPON_TNT);
		SetThink( DestroyItem );
		
		pev->nextthink = gpGlobals->time + 0.1;
	}

	SendWeaponAnim( SATCHEL_RADIO_HOLSTER );
	EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "common/null.wav", 1.0, ATTN_NORM);
}

void CKamikaze::PrimaryAttack( void )
{
	if (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		return;

	EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_AUTO, "weapons/tnt1.wav", 1.0, ATTN_NORM);

    pev->nextthink = gpGlobals->time + 20.0;

	SendWeaponAnim( SATCHEL_RADIO_FIRE );

	Vector vecSpot = pev->origin;

	MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, vecSpot );
	WRITE_BYTE( TE_SPRITE );//additive sprite plays though once
	WRITE_COORD( vecSpot.x );//where to make the sprite appear on x axis
	WRITE_COORD( vecSpot.y );//where to make the sprite appear on y axis
	WRITE_COORD( vecSpot.z + 128 );//Creates sprite 128 units above model's center
	WRITE_SHORT( m_iExplode );//Name of the sprite to use, as defined at begining of tut
	WRITE_BYTE( 400 ); // scale in .1 units --by comparison the player is 72 units tall 
	WRITE_BYTE( 255 ); // brightness (this is as bright as it gets)
	MESSAGE_END();

	MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, vecSpot );
	WRITE_BYTE( TE_SMOKE );//alphablended sprite
	WRITE_COORD( vecSpot.x );
	WRITE_COORD( vecSpot.y );
	WRITE_COORD( vecSpot.z + 256 );
	WRITE_SHORT( g_sModelIndexSmoke );//This is Defined in weapons.cpp and weapons.h
	WRITE_BYTE( 125 ); //scale in .1 units
	WRITE_BYTE( 5 ); // framerate to playback sprite
	MESSAGE_END();

	EMIT_SOUND(ENT(pev), CHAN_STATIC, "weapons/mortarhit.wav", 1.0, 0.3);

	UTIL_ScreenShake( pev->origin, 50.0, 400.0, 1.0, 1080 );
	
	entvars_t	*pevOwner;
	
	if ( pev->owner )
		pevOwner = VARS( pev->owner );
	else
		pevOwner = NULL;

	pev->owner = NULL;

	RadiusDamage( pev->origin, pev, pevOwner, 400, 200, CLASS_NONE, DMG_BLAST | DMG_BURN ); 


	pev->effects |= EF_NODRAW;
	pev->velocity = g_vecZero;


    m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType]--;

			m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

			if (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] == 0)
				
				RetireWeapon();
				return;

	m_flNextPrimaryAttack = gpGlobals->time + 2.0;
	m_flTimeWeaponIdle = gpGlobals->time + RANDOM_FLOAT ( 10, 15 );
}
void CKamikaze::WeaponIdle( void )
{
	if (m_flTimeWeaponIdle > gpGlobals->time)
		return;

	if (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] > 0)
	{
		SendWeaponAnim( SATCHEL_RADIO_DRAW );
	}
	else
	{
		RetireWeapon(); 
		return;
	}

	int iAnim;
	float flRand = RANDOM_FLOAT(0, 1);
	if (flRand <= 0.25)
	{
		iAnim = SATCHEL_RADIO_IDLE1;
		m_flTimeWeaponIdle = gpGlobals->time + 90.0 / 30.0;
	}
	else if (flRand <= 0.75)
	{
		iAnim = SATCHEL_RADIO_IDLE1;
		m_flTimeWeaponIdle = gpGlobals->time + 60.0 / 30.0;
	}
	else
	{
		iAnim = SATCHEL_RADIO_IDLE1;
		m_flTimeWeaponIdle = gpGlobals->time + 100.0 / 30.0;
	}

	SendWeaponAnim( iAnim );
	
}




