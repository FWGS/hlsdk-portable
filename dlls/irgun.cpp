/****************************************************************************
*																			*
*			IRgun.cpp par Julien											*
*																			*
****************************************************************************/



#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "weapons.h"
#include "monsters.h"
#include "player.h"
#include "gamerules.h"
#include "../engine/shake.h"


enum irgun_e {
	IRGUN_FIDGET = 0,
	IRGUN_RELOAD_EMPTY,
	IRGUN_IDLE1,
	IRGUN_DRAW,
	IRGUN_FIRE1,
	IRGUN_IDLE_IR,
	IRGUN_FIRE_EMPTY,
	IRGUN_RELOAD
};

class CIRgun : public CBasePlayerWeapon
{
public:
	void Spawn( void );
	void Precache( void );
	int iItemSlot( void ) { return 2; }
	int GetItemInfo(ItemInfo *p);
	int AddToPlayer( CBasePlayer *pPlayer );
	void PrimaryAttack( void );
	void SecondaryAttack( void );
	BOOL Deploy( void );
	void Holster( int skiplocal = 0 );
	void Reload( void );
	void WeaponIdle( void );

private:
	unsigned short m_usFireIRgun;
	int m_iShell;

};
LINK_ENTITY_TO_CLASS( weapon_irgun, CIRgun );

int CIRgun::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "irgun";
	p->iMaxAmmo1 = IRGUN_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = IRGUN_MAX_CLIP;
	p->iFlags = 0;
	p->iSlot = 1;
	p->iPosition = 3;
	p->iId = m_iId = WEAPON_IRGUN;
	p->iWeight = IRGUN_WEIGHT;

	return 1;
}

int CIRgun::AddToPlayer( CBasePlayer *pPlayer )
{
	if ( CBasePlayerWeapon::AddToPlayer( pPlayer ) )
	{
		MESSAGE_BEGIN( MSG_ONE, gmsgWeapPickup, NULL, pPlayer->pev );
			WRITE_BYTE( m_iId );
		MESSAGE_END();

		m_pPlayer->TextAmmo( TA_IRGUN );

		return TRUE;
	}
	return FALSE;
}

void CIRgun::Spawn( )
{
	pev->classname = MAKE_STRING("weapon_irgun"); // hack to allow for old names
	Precache( );
	m_iId = WEAPON_IRGUN;
	SET_MODEL(ENT(pev), "models/w_irgun.mdl");

	m_iDefaultAmmo = IRGUN_DEFAULT_GIVE;

	FallInit();// get ready to fall down.
}


void CIRgun::Precache( void )
{
	PRECACHE_MODEL("models/v_irgun.mdl");
	PRECACHE_MODEL("models/w_irgun.mdl");
	PRECACHE_MODEL("models/p_357.mdl");

	PRECACHE_MODEL("models/w_irgunclip.mdl");
	PRECACHE_SOUND("items/9mmclip1.wav");              

	PRECACHE_SOUND ("weapons/357_reload1.wav");
	PRECACHE_SOUND ("weapons/357_cock1.wav");

//	PRECACHE_SOUND ("weapons/357_shot1.wav");
//	PRECACHE_SOUND ("weapons/357_shot2.wav");
	PRECACHE_SOUND ("hassault/hw_shoot1.wav");
	PRECACHE_SOUND ("hassault/hw_shoot2.wav");
	PRECACHE_SOUND ("hassault/hw_shoot3.wav");

	m_usFireIRgun = PRECACHE_EVENT( 1, "events/irgun.sc" );

	m_iShell = PRECACHE_MODEL ("models/beretta_shell.mdl");// brass shell

}

BOOL CIRgun::Deploy( )
{
	return DefaultDeploy( "models/v_irgun.mdl", "models/p_357.mdl", IRGUN_DRAW, "irgun" );
}


void CIRgun::Holster( int skiplocal /* = 0 */ )
{
	if ( m_pPlayer->bNvgOn == 1 )
		m_pPlayer->NVGTurnOff ();

	m_fInReload = FALSE;// cancel any reload in progress.

	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 1.0;
	m_flTimeWeaponIdle = gpGlobals->time + 10 + RANDOM_FLOAT ( 0, 5 );
}

void CIRgun::PrimaryAttack()
{
	// don't fire underwater
	if (m_pPlayer->pev->waterlevel == 3)
	{
		PlayEmptySound( );
		m_flNextPrimaryAttack = gpGlobals->time + 0.15;
		return;
	}

	if (m_iClip <= 0)
	{
		if (!m_fFireOnEmpty)
			Reload( );
		else
		{
			EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/357_cock1.wav", 0.8, ATTN_NORM);
			m_flNextPrimaryAttack = gpGlobals->time + 0.15;
		}

		return;
	}

	m_iClip--;

//	PLAYBACK_EVENT( 0, m_pPlayer->edict(), m_usFireIRgun );	// le full permet de préciser le param1
	PLAYBACK_EVENT_FULL( 0, m_pPlayer->edict(), m_usFireIRgun, 0, (float *)&g_vecZero, (float *)&g_vecZero, 0.0, 0.0, 0, 0, ( m_iClip == 0 ) ? 1 : 0, 0 );

	m_pPlayer->m_iWeaponVolume = LOUD_GUN_VOLUME;
	m_pPlayer->Gunflash ();

	// player "shoot" animation
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );
	m_pPlayer->pev->effects = (int)(m_pPlayer->pev->effects) | EF_MUZZLEFLASH;

	UTIL_MakeVectors( m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle );

	Vector vecSrc	 = m_pPlayer->GetGunPosition( );
	Vector vecAiming = m_pPlayer->GetAutoaimVector( AUTOAIM_10DEGREES );
	m_pPlayer->FireBullets( 1, vecSrc, vecAiming, VECTOR_CONE_1DEGREES, 8192, BULLET_PLAYER_IRGUN, 0 );

	if (!m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);

	m_flNextPrimaryAttack = gpGlobals->time + 0.5;
	m_flTimeWeaponIdle = gpGlobals->time + RANDOM_FLOAT ( 10, 15 );
}



void CIRgun::SecondaryAttack( void )
{
	if ( m_pPlayer->bNvgOn == 0)
		m_pPlayer->NVGTurnOn ();

	else
		m_pPlayer->NVGTurnOff ();
		
	pev->nextthink = gpGlobals->time + 0.1;
	m_flNextSecondaryAttack = gpGlobals->time + 0.5;
}



void CIRgun::Reload( void )
{
	if (m_iClip == 0)
		DefaultReload( IRGUN_MAX_CLIP, IRGUN_RELOAD_EMPTY, 2.1 );
	else
		DefaultReload( IRGUN_MAX_CLIP, IRGUN_RELOAD, 2.1 );
}


void CIRgun::WeaponIdle( void )
{
	ResetEmptySound( );

	m_pPlayer->GetAutoaimVector( AUTOAIM_10DEGREES );

	if (m_flTimeWeaponIdle > gpGlobals->time)
		return;

	int iAnim;
	float flRand = RANDOM_FLOAT(0, 1);

	if (flRand <= 0.4 || m_pPlayer->bNvgOn == TRUE )
	{
		iAnim = IRGUN_IDLE_IR;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + RANDOM_FLOAT( 3, 6 );
	}
	else if (flRand <= 0.8)
	{
		iAnim = IRGUN_IDLE1;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 19 / 4.0;
	}
	else
	{
		iAnim = IRGUN_FIDGET;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 17 / 4.0;
	}



	SendWeaponAnim( iAnim );
}



class CIRgunAmmo : public CBasePlayerAmmo
{
	void Spawn( void )
	{ 
		Precache( );
		SET_MODEL(ENT(pev), "models/w_irgunclip.mdl");
		CBasePlayerAmmo::Spawn( );
	}
	void Precache( void )
	{
		PRECACHE_MODEL ("models/w_irgunclip.mdl");
		PRECACHE_SOUND("items/9mmclip1.wav");
	}
	BOOL AddAmmo( CBaseEntity *pOther ) 
	{ 
		if (pOther->GiveAmmo( AMMO_IRGUNCLIPGIVE, "irgun", IRGUN_MAX_CARRY ) != -1)
		{
			EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM);
			return TRUE;
		}
		return FALSE;
	}
};
LINK_ENTITY_TO_CLASS( ammo_irgun, CIRgunAmmo );

