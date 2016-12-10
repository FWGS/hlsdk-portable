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


#define	CLUSTERGRENADE_PRIMARY_VOLUME		450

enum cluster_e {
/*
	HANDGRENADE_IDLE = 0,
	HANDGRENADE_FIDGET,
	HANDGRENADE_PINPULL,
	HANDGRENADE_THROW1,	
	HANDGRENADE_THROW2,	
	HANDGRENADE_THROW3,	
	HANDGRENADE_HOLSTER,
	HANDGRENADE_DRAW
*/
};


class CClusterGrenade : public CBasePlayerWeapon
{
public:
	void Spawn( void );
	void Precache( void );
	int iItemSlot( void ) { return 5; }
	int GetItemInfo(ItemInfo *p);

	void PrimaryAttack( void );
	BOOL Deploy( void );
	BOOL CanHolster( void );
	void Holster( void );
	void WeaponIdle( void );
	float m_flStartThrow;
	float m_flReleaseThrow;
};
LINK_ENTITY_TO_CLASS( weapon_clustergrenade, CClusterGrenade );
LINK_ENTITY_TO_CLASS( weapon_handgrenade, CClusterGrenade );

void CClusterGrenade::Spawn( )
{
	pev->classname = MAKE_STRING( "weapon_clustergrenade" ); 

	Precache( );
	m_iId = WEAPON_CLUSTERGRENADE;
	SET_MODEL(ENT(pev), "models/clustergrenade.mdl");

	pev->dmg = gSkillData.plrDmgClusterGrenade;

	m_iDefaultAmmo = CLUSTERGRENADE_DEFAULT_GIVE;

	FallInit();
}


void CClusterGrenade::Precache( void )
{
	PRECACHE_MODEL("models/w_grenade.mdl");
	PRECACHE_MODEL("models/vmodels/v_chumtoad.mdl");
	PRECACHE_MODEL("models/p_grenade.mdl");

	PRECACHE_SOUND("weapons/pinpull.wav");
}

int CClusterGrenade::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "Cluster Grenade";
	p->iMaxAmmo1 = CLUSTERGRENADE_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = WEAPON_NOCLIP;
	p->iSlot = 4;
	p->iPosition = 0;
	p->iId = m_iId = WEAPON_CLUSTERGRENADE;
	p->iWeight = CLUSTERGRENADE_WEIGHT;
	p->iFlags = ITEM_FLAG_LIMITINWORLD | ITEM_FLAG_EXHAUSTIBLE;

	return 1;
}


BOOL CClusterGrenade::Deploy( )
{
	m_flReleaseThrow = -1;
	return DefaultDeploy( "models/vmodels/v_chumtoad.mdl", "models/p_grenade.mdl", NULL, "crowbar" );
}

BOOL CClusterGrenade::CanHolster( void )
{
	return ( m_flStartThrow == 0 );
}

void CClusterGrenade::Holster( )
{
	m_pPlayer->m_flNextAttack = gpGlobals->time + 0.5;
	
	if (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType])
	{
		EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/pinpull.wav", 1.0, ATTN_NORM);
		//SendWeaponAnim( HANDGRENADE_HOLSTER );
	}
	else
	{
		m_pPlayer->pev->weapons &= ~(1<<WEAPON_CLUSTERGRENADE);
		SetThink( DestroyItem );
		pev->nextthink = gpGlobals->time + 0.1;
	}

}

void CClusterGrenade::PrimaryAttack()
{
	if (!m_flStartThrow && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] > 0)
	{
		m_flStartThrow = gpGlobals->time;
		m_flReleaseThrow = 0;

		//SendWeaponAnim( HANDGRENADE_PINPULL );
		m_flTimeWeaponIdle = gpGlobals->time + 0.5;
	}
}


void CClusterGrenade::WeaponIdle( void )
{
	if (m_flReleaseThrow == 0)
		m_flReleaseThrow = gpGlobals->time;

	if (m_flTimeWeaponIdle > gpGlobals->time)
		return;

	if (m_flStartThrow)
	{
		Vector angThrow = m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle;

		if (angThrow.x < 0)
			angThrow.x = -10 + angThrow.x * ((90 - 10) / 90.0);
		else
			angThrow.x = -10 + angThrow.x * ((90 + 10) / 90.0);

		float flVel = (90 - angThrow.x) * 4;
		if (flVel > 500)
			flVel = 500;

		UTIL_MakeVectors( angThrow );

		Vector vecSrc = m_pPlayer->pev->origin + m_pPlayer->pev->view_ofs + gpGlobals->v_forward * 16;

		Vector vecThrow = gpGlobals->v_forward * flVel + m_pPlayer->pev->velocity;

		// alway explode 3 seconds after the pin was pulled
		float time = m_flStartThrow - gpGlobals->time + 3.0;
		if (time < 0)
			time = 0;

		CGrenade::ShootClusterGrenade( m_pPlayer->pev, vecSrc, vecThrow, time );

		// player "shoot" animation
		m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

		m_flStartThrow = 0;
		m_flNextPrimaryAttack = gpGlobals->time + 0.5;
		m_flTimeWeaponIdle = gpGlobals->time + 0.5;

		m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType]--;

		if ( !m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] )
		{
			// just threw last grenade
			// set attack times in the future, and weapon idle in the future so we can see the whole throw
			// animation, weapon idle will automatically retire the weapon for us.
			m_flTimeWeaponIdle = m_flNextSecondaryAttack = m_flNextPrimaryAttack = gpGlobals->time + 0.5;// ensure that the animation can finish playing
		}
		return;
	}
	else if (m_flReleaseThrow > 0)
	{
		// we've finished the throw, restart.
		m_flStartThrow = 0;

		if (!m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType])
		{
			RetireWeapon();
			return;
		}

		m_flTimeWeaponIdle = gpGlobals->time + RANDOM_FLOAT ( 10, 15 );
		m_flReleaseThrow = -1;
		return;
	}

	if (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType])
	{
		//int iAnim;
		float flRand = RANDOM_FLOAT(0, 1);
		if (flRand <= 0.75)
		{
			//iAnim = HANDGRENADE_IDLE;
			m_flTimeWeaponIdle = gpGlobals->time + RANDOM_FLOAT ( 10, 15 );// how long till we do this again.
		}
		else 
		{
			//iAnim = HANDGRENADE_FIDGET;
			m_flTimeWeaponIdle = gpGlobals->time + 75.0 / 30.0;
		}

		//SendWeaponAnim( iAnim );
	}
}