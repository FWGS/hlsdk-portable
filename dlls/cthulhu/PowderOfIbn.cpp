
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"

#include "PowderOfIbn.h"
#include "monster_PowderOfIbn.h"

LINK_ENTITY_TO_CLASS( weapon_powderofibn, CPowderOfIbn );


void CPowderOfIbn::Spawn( )
{
	Precache( );
	m_iId = WEAPON_POWDER_IBN;
	SET_MODEL(ENT(pev), "models/w_powderofibn.mdl");

	pev->dmg = 0;

	m_iDefaultAmmo = POWDER_IBN_DEFAULT_GIVE;

	FallInit();// get ready to fall down.
}

void CPowderOfIbn::Precache( void )
{
	PRECACHE_MODEL("models/w_powderofibn.mdl");
	PRECACHE_MODEL("models/v_powderofibn.mdl");
//	PRECACHE_MODEL("models/p_powderofibn.mdl");
}

int CPowderOfIbn::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "Powder Of Ibn";
	p->iMaxAmmo1 = POWDER_IBN_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = WEAPON_NOCLIP;
	p->iSlot = 4;
	p->iPosition = 3;
	p->iId = m_iId = WEAPON_POWDER_IBN;
	p->iWeight = POWDER_IBN_WEIGHT;
	p->iFlags = ITEM_FLAG_LIMITINWORLD | ITEM_FLAG_EXHAUSTIBLE;

	return 1;
}


BOOL CPowderOfIbn::Deploy( )
{
	m_flReleaseThrow = -1;
	return DefaultDeploy( "models/v_powderofibn.mdl", "", IBN_DRAW, "powderofibn" );
}

BOOL CPowderOfIbn::CanHolster( void )
{
	// can only holster powder when not lit!
	return ( m_flStartThrow == 0 );
}

void CPowderOfIbn::Holster( int skiplocal /* = 0 */ )
{
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;
	if (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType])
	{
		SendWeaponAnim( IBN_HOLSTER );
	}
	else
	{
		// no more powders!
		m_pPlayer->pev->weapons &= ~(1<<WEAPON_POWDER_IBN);
		SetThink( DestroyItem );
		SetNextThink( 0.1 );
	}

	EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "common/null.wav", 1.0, ATTN_NORM);
}

int CPowderOfIbn::AddToPlayer( CBasePlayer *pPlayer )
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

void CPowderOfIbn::PrimaryAttack()
{
	if (!m_flStartThrow && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] > 0)
	{
		m_flStartThrow = gpGlobals->time;
		m_flReleaseThrow = gpGlobals->time;

		SendWeaponAnim( IBN_THROW );
		m_flTimeWeaponIdle = gpGlobals->time + 0.5;
	}
}

void CPowderOfIbn::WeaponIdle( void )
{
	//if (m_flReleaseThrow == 0)
	//	m_flReleaseThrow = gpGlobals->time;

	if (m_flTimeWeaponIdle > gpGlobals->time)
		return;

	if (m_flStartThrow > 0)
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

		CMonsterPowderOfIbn::ShootContact( m_pPlayer->pev, vecSrc, vecThrow );

		// player "shoot" animation
		m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

		m_flStartThrow = 0;
		m_flNextPrimaryAttack = gpGlobals->time + 0.5;
		m_flTimeWeaponIdle = gpGlobals->time + 0.5;

		m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType]--;

		if ( !m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] )
		{
			// just threw last bottle of powder
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
		m_flReleaseThrow = -1;
		m_flTimeWeaponIdle = gpGlobals->time + RANDOM_FLOAT ( 10, 15 );

		if (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType])
		{
			SendWeaponAnim( IBN_DRAW );
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
			iAnim = IBN_IDLE;
			m_flTimeWeaponIdle = gpGlobals->time + RANDOM_FLOAT ( 10, 15 );// how long till we do this again.
		}
		else 
		{
			iAnim = IBN_FIDGET;
			m_flTimeWeaponIdle = gpGlobals->time + 75.0 / 30.0;
		}

		SendWeaponAnim( iAnim );
	}
}




