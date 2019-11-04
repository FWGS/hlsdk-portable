
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"
#include "effects.h"


#include "dynamite.h"
#include "monster_dynamite.h"

//#define DYNAMITE_FUSE_SPRITE		"sprites/fire.spr"


LINK_ENTITY_TO_CLASS( weapon_dynamite, CDynamite );


void CDynamite::Spawn( )
{
	Precache( );
	m_iId = WEAPON_DYNAMITE;
	SET_MODEL(ENT(pev), "models/w_dynamite.mdl");

	pev->dmg = gSkillData.plrDmgDynamite;

	m_iDefaultAmmo = DYNAMITE_DEFAULT_GIVE;

	FallInit();// get ready to fall down.
}

void CDynamite::Precache( void )
{
	PRECACHE_MODEL("models/w_dynamite.mdl");
	PRECACHE_MODEL("models/v_dynamite.mdl");
//	PRECACHE_MODEL("models/p_dynamite.mdl");

//	PRECACHE_MODEL( DYNAMITE_FUSE_SPRITE );
}

int CDynamite::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "Dynamite";
	p->iMaxAmmo1 = DYNAMITE_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = WEAPON_NOCLIP;
	p->iSlot = 2;
	p->iPosition = 1;
	p->iId = m_iId = WEAPON_DYNAMITE;
	p->iWeight = DYNAMITE_WEIGHT;
	p->iFlags = ITEM_FLAG_LIMITINWORLD | ITEM_FLAG_EXHAUSTIBLE;

	return 1;
}


BOOL CDynamite::Deploy( )
{
	m_flReleaseThrow = -1;
	m_flFinishThrow = -1;
	return DefaultDeploy( "models/v_dynamite.mdl", "", DYNAMITE_DRAW, "dynamite" );
}

BOOL CDynamite::CanHolster( void )
{
	// can only holster dynamite when not lit!
	return ( m_flStartThrow == 0 );
}

void CDynamite::Holster( int skiplocal /* = 0 */ )
{
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;
	if (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType])
	{
		SendWeaponAnim( DYNAMITE_HOLSTER );
	}
	else
	{
		// no more dynamites!
		m_pPlayer->pev->weapons &= ~(1<<WEAPON_DYNAMITE);
		SetThink( DestroyItem );
		SetNextThink( 0.1 );
	}

	EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "common/null.wav", 1.0, ATTN_NORM);
}

int CDynamite::AddToPlayer( CBasePlayer *pPlayer )
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

void CDynamite::PrimaryAttack()
{
	if (!m_flStartThrow && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] > 0)
	{
		m_flStartThrow = gpGlobals->time;
		m_flReleaseThrow = gpGlobals->time;
		m_flFinishThrow = gpGlobals->time;

		SendWeaponAnim( DYNAMITE_LIGHT );

		m_flTimeWeaponIdle = gpGlobals->time + 1.5;
	}
}


void CDynamite::WeaponIdle( void )
{
	//if (m_flReleaseThrow == 0)
	//	m_flReleaseThrow = gpGlobals->time;
	//else if (m_flFinishThrow == 0)
	//	m_flFinishThrow = gpGlobals->time;

	if (m_flTimeWeaponIdle > gpGlobals->time)
		return;

	if (m_flStartThrow > 0)
	{
		// alway explode 5 seconds after the fuse was lit
		SendWeaponAnim( DYNAMITE_THROW );

		// player "shoot" animation
		m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

		m_flStartThrow = 0;
		m_flNextPrimaryAttack = gpGlobals->time + 1.0;
		m_flTimeWeaponIdle = gpGlobals->time + 0.5;

		return;
	}
	else if (m_flReleaseThrow > 0)
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

		float time = m_flReleaseThrow - gpGlobals->time + 5.0 - 0.5; // because it has been 0.5 seconds since we lit the fuse...
		if (time < 0)
			time = 0;

		CMonsterDynamite::Shoot( m_pPlayer->pev, vecSrc, vecThrow, time );

		m_flTimeWeaponIdle = gpGlobals->time + 0.5;

		m_flReleaseThrow = -1;

		m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType]--;

		if ( !m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] )
		{
			// just threw last stick of dynamite
			// set attack times in the future, and weapon idle in the future so we can see the whole throw
			// animation, weapon idle will automatically retire the weapon for us.
			m_flTimeWeaponIdle = m_flNextSecondaryAttack = m_flNextPrimaryAttack = gpGlobals->time + 1.0;// ensure that the animation can finish playing
		}
		return;
	}
	else if (m_flFinishThrow > 0)
	{
		// we've finished the throw, restart.
		m_flStartThrow = 0;
		m_flReleaseThrow = -1;
		m_flFinishThrow = -1;
		m_flTimeWeaponIdle = gpGlobals->time + RANDOM_FLOAT ( 10, 15 );

		if (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType])
		{
			SendWeaponAnim( DYNAMITE_DRAW );
		}
		else
		{
			RetireWeapon();
			return;
		}

		return;
	}

	if (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType])
	{
		int iAnim;
		float flRand = RANDOM_FLOAT(0, 1);
		if (flRand <= 0.75)
		{
			iAnim = DYNAMITE_IDLE1;
			m_flTimeWeaponIdle = gpGlobals->time + RANDOM_FLOAT ( 10, 15 );// how long till we do this again.
		}
		else 
		{
			iAnim = DYNAMITE_IDLE2;
			m_flTimeWeaponIdle = gpGlobals->time + 75.0 / 30.0;
		}

		SendWeaponAnim( iAnim );
	}
}




