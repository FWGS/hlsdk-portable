/***
*
*****CREATED BY SOLEXID
*******legendary BIG_COCK  
*ALL RIGHTS FUCKED
****/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"
#include "customweapons.h"

#define BIG_COCK_WEIGHT 21


enum glock_e {
	GLOCK_IDLE1 = 0,
	GLOCK_IDLE2,
	GLOCK_IDLE3,
	GLOCK_SHOOT,
	GLOCK_SHOOT_EMPTY,
	GLOCK_RELOAD,
	GLOCK_RELOAD_NOT_EMPTY,
	GLOCK_DRAW,
	GLOCK_HOLSTER,
	GLOCK_ADD_SILENCER
};

class CBig_Cock : public CBasePlayerWeapon
{
public:
	void Spawn( void );
	void Precache( void );
	int iItemSlot( void ) { return 3; }
	int GetItemInfo(ItemInfo *p);

	void PrimaryAttack( void );
	void SecondaryAttack( void );
	void GlockFire( float flSpread, float flCycleTime, BOOL fUseAutoAim );
	BOOL Deploy( void );
	void Reload( void );
	void WeaponIdle( void );

private:
	int m_iShell;
	

	unsigned short m_usFireGlock1;
	unsigned short m_usFireGlock2;
};
LINK_ENTITY_TO_CLASS(weapon_big_cock, CBig_Cock);
LINK_ENTITY_TO_CLASS(weapon_90mhandcock, CBig_Cock);


void CBig_Cock::Spawn()
{
	pev->classname = MAKE_STRING("weapon_big_cock"); // hack to allow for old names
	Precache( );
	m_iId = WEAPON_BIG_COCK;
	SET_MODEL(ENT(pev), "models/w_9mmcockgunn.mdl");

	m_iDefaultAmmo = GLOCK_DEFAULT_GIVE;

	FallInit();// get ready to fall down.
}


void CBig_Cock::Precache(void)
{
	PRECACHE_MODEL("models/v_9mmcockgunn.mdl");
	PRECACHE_MODEL("models/w_9mmcockgunn.mdl");
	PRECACHE_MODEL("models/p_9mmcockgunn.mdl");

	m_iShell = PRECACHE_MODEL ("models/shell.mdl");// brass shell

	PRECACHE_SOUND("items/9mmclip1.wav");
	PRECACHE_SOUND("items/9mmclip2.wav");

	PRECACHE_SOUND ("weapons/pl_gun1.wav");//silenced handgun
	PRECACHE_SOUND ("weapons/pl_gun2.wav");//silenced handgun
	PRECACHE_SOUND ("weapons/ck_gun3.wav");//handgun

	m_usFireGlock1 = PRECACHE_EVENT( 1, "events/glock1.sc" );
	m_usFireGlock2 = PRECACHE_EVENT( 1, "events/glock2.sc" );
}

int CBig_Cock::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "9mm";
	p->iMaxAmmo1 = _9MM_MAX_CARRY*5;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = 14;
	p->iSlot = 1;
	p->iPosition = 3;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_BIG_COCK;
	p->iWeight = BIG_COCK_WEIGHT;

	return 1;
}

BOOL CBig_Cock::Deploy()
{
	// pev->body = 1;
	return DefaultDeploy( "models/v_9mmcockgunn.mdl", "models/p_9mmcockgunn.mdl", GLOCK_DRAW, "onehanded" );
}

void CBig_Cock::SecondaryAttack(void)
{
	GlockFire( 0.02, 0.06, FALSE );
}

void CBig_Cock::PrimaryAttack(void)
{
	GlockFire( 0, 0.9, TRUE );
}

void CBig_Cock::GlockFire(float flSpread, float flCycleTime, BOOL fUseAutoAim)
{
	if (m_iClip <= 0)
	{
		if (m_fFireOnEmpty)
		{
			PlayEmptySound();
			m_flNextPrimaryAttack = gpGlobals->time + 0.4;
		}

		return;
	}

	m_iClip--;

	m_pPlayer->pev->effects = (int)(m_pPlayer->pev->effects) | EF_MUZZLEFLASH;

#if defined ( OLD_WEAPONS )
	if (m_iClip != 0)
		SendWeaponAnim( GLOCK_SHOOT );
	else
		SendWeaponAnim( GLOCK_SHOOT_EMPTY );
#endif

	if ( fUseAutoAim )
	{
		PLAYBACK_EVENT_FULL( 0, m_pPlayer->edict(), m_usFireGlock1, 0.0, (float *)&g_vecZero, (float *)&g_vecZero, 0.0, 0.0, 0, 0, ( m_iClip == 0 ) ? 1 : 0, 0 );
	}
	else
	{
		PLAYBACK_EVENT_FULL( 0, m_pPlayer->edict(), m_usFireGlock2, 0.0, (float *)&g_vecZero, (float *)&g_vecZero, 0.0, 0.0, 0, 0, ( m_iClip == 0 ) ? 1 : 0, 0 );
	}

	// player "shoot" animation
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

#if defined ( OLD_WEAPONS )
	UTIL_MakeVectors( m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle );
		
	Vector	vecShellVelocity = m_pPlayer->pev->velocity 
							 + gpGlobals->v_right * RANDOM_FLOAT(50,70) 
							 + gpGlobals->v_up * RANDOM_FLOAT(100,150) 
							 + gpGlobals->v_forward * 25;
	EjectBrass ( pev->origin + m_pPlayer->pev->view_ofs + gpGlobals->v_up * -12 + gpGlobals->v_forward * 32 + gpGlobals->v_right * 6 , vecShellVelocity, pev->angles.y, m_iShell, TE_BOUNCE_SHELL ); 
#endif

	// silenced
	if (pev->body == 1)
	{
		m_pPlayer->m_iWeaponVolume = QUIET_GUN_VOLUME;
		m_pPlayer->m_iWeaponFlash = DIM_GUN_FLASH;
#if defined ( OLD_WEAPONS )
		switch(RANDOM_LONG(0,1))
		{
		case 0:
			EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/pl_gun1.wav", RANDOM_FLOAT(0.9, 1.0), ATTN_NORM);
			break;
		case 1:
			EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/pl_gun2.wav", RANDOM_FLOAT(0.9, 1.0), ATTN_NORM);
			break;
		}
#endif
	}
	else
	{
		// non-silenced
		m_pPlayer->m_iWeaponVolume = BIG_EXPLOSION_VOLUME;
		m_pPlayer->m_iWeaponFlash = NORMAL_GUN_FLASH;
#if defined ( OLD_WEAPONS )
		EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/ck_gun3.wav", RANDOM_FLOAT(0.92, 1.0), ATTN_NORM, 0, 98 + RANDOM_LONG(0,3));
#endif
	}

	Vector vecSrc	 = m_pPlayer->GetGunPosition( );
	Vector vecAiming;
	
	if ( fUseAutoAim )
	{
		vecAiming = m_pPlayer->GetAutoaimVector( AUTOAIM_10DEGREES );
	}
	else
	{
		vecAiming = gpGlobals->v_forward;
	}

	TraceResult tr;
	if (flSpread > 0){
		m_pPlayer->FireBullets(1, vecSrc, vecAiming, Vector(flSpread, flSpread, flSpread), 8192, BULLET_PLAYER_357, 5, 35);
		m_pPlayer->pev->velocity = m_pPlayer->pev->velocity - gpGlobals->v_forward * 25 * 5;
		
	}
	else{
		m_pPlayer->FireBullets(1, vecSrc, vecAiming, Vector(flSpread, flSpread, flSpread), 8192, BULLET_PLAYER_357, 5, 60);
		m_pPlayer->pev->velocity = m_pPlayer->pev->velocity - gpGlobals->v_forward * 100 * 5;

		//ApplyMultiDamage(m_pPlayer->pev, m_pPlayer->pev);
	}


	m_flNextPrimaryAttack = m_flNextSecondaryAttack = gpGlobals->time + flCycleTime;

	if (!m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);

	m_flTimeWeaponIdle = gpGlobals->time + RANDOM_FLOAT ( 10, 15 );

#if defined ( OLD_WEAPONS )
	m_pPlayer->pev->punchangle.x -= 2;
#endif
}


void CBig_Cock::Reload(void)
{
	int iResult;

	if (m_iClip == 0)
		iResult = DefaultReload( 17, GLOCK_RELOAD, 1.5 );
	else
		iResult = DefaultReload( 18, GLOCK_RELOAD_NOT_EMPTY, 1.5 );

	if (iResult)
	{
		m_flTimeWeaponIdle = gpGlobals->time + RANDOM_FLOAT ( 10, 15 );
	}
}



void CBig_Cock::WeaponIdle(void)
{
	ResetEmptySound( );

	m_pPlayer->GetAutoaimVector( AUTOAIM_10DEGREES );

	if (m_flTimeWeaponIdle > gpGlobals->time)
		return;

	// only idle if the slid isn't back
	if (m_iClip != 0)
	{
		int iAnim;
		float flRand = RANDOM_FLOAT(0, 1);
		if (flRand <= 0.3 + 0 * 0.75)
		{
			iAnim = GLOCK_IDLE3;
			m_flTimeWeaponIdle = gpGlobals->time + 49.0 / 16;
		}
		else if (flRand <= 0.6 + 0 * 0.875)
		{
			iAnim = GLOCK_IDLE1;
			m_flTimeWeaponIdle = gpGlobals->time + 60.0 / 16.0;
		}
		else
		{
			iAnim = GLOCK_IDLE2;
			m_flTimeWeaponIdle = gpGlobals->time + 40.0 / 16.0;
		}
		SendWeaponAnim( iAnim );
	}
}








class CGlockAmmo : public CBasePlayerAmmo
{
	void Spawn( void )
	{ 
		Precache( );
		SET_MODEL(ENT(pev), "models/w_9mmclip.mdl");
		CBasePlayerAmmo::Spawn( );
	}
	void Precache( void )
	{
		PRECACHE_MODEL ("models/w_9mmclip.mdl");
		PRECACHE_SOUND("items/9mmclip1.wav");
	}
	BOOL AddAmmo( CBaseEntity *pOther ) 
	{ 
		if (pOther->GiveAmmo( AMMO_GLOCKCLIP_GIVE, "9mm", _9MM_MAX_CARRY*5 ) != -1)
		{
			EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM);
			return TRUE;
		}
		return FALSE;
	}
};
LINK_ENTITY_TO_CLASS( ammo_cockclip, CGlockAmmo );
LINK_ENTITY_TO_CLASS( ammo_9mcocks, CGlockAmmo );














