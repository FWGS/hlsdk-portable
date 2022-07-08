/******************************************
*                                         *
*	Fichier fsniper, par Julien			  *
*                                         *
*	code du fusil de snipe				  *
*										  *
******************************************/




#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"
#include "soundent.h"
#include "gamerules.h"

enum fsniper_e
{
	FSNIPER_LONGIDLE = 0,
	FSNIPER_IDLE1,
	FSNIPER_RELOAD,
	FSNIPER_DEPLOY,
	FSNIPER_FIRE1,
	FSNIPER_FIDGET,
	FSNIPER_ZOOM,
};


class CFSniper : public CBasePlayerWeapon
{
public:
	void Spawn( void );
	void Precache( void );
	int AddToPlayer( CBasePlayer *pPlayer );

	void PrimaryAttack( void );
	void SecondaryAttack( void );
	void Holster( int skiplocal = 0 );
	void Reload( void );
	void WeaponIdle( void );
	BOOL Deploy( void );

	int GetItemInfo(ItemInfo *p);
	int iItemSlot( void ) { return 4; }	//position de l'arme dans le hud

	float m_flNextAnimTime;
	int m_iShell;

private:
	unsigned short m_usFSniper;		//pour pr
};
LINK_ENTITY_TO_CLASS( weapon_fsniper, CFSniper );



//============================================
//
//============================================

void CFSniper::Spawn( )
{
	pev->classname = MAKE_STRING("weapon_fsniper");
	Precache();
	SET_MODEL(ENT(pev), "models/w_fsniper.mdl");
	m_iId = WEAPON_FSNIPER;

	m_iDefaultAmmo = FSNIPER_DEFAULT_GIVE;

	FallInit();// get ready to fall down.
}


void CFSniper::Precache( void )
{
	PRECACHE_MODEL("models/v_fsniper.mdl");
	PRECACHE_MODEL("models/w_fsniper.mdl");
	PRECACHE_MODEL("models/p_9mmAR.mdl");

	m_iShell = PRECACHE_MODEL ("models/sniper_shell.mdl");	//model de la cartouche, 

	PRECACHE_MODEL("models/w_fsniperclip.mdl");
	PRECACHE_SOUND("items/9mmclip1.wav");              

	PRECACHE_SOUND("items/clipinsert1.wav");
	PRECACHE_SOUND("items/cliprelease1.wav");

	PRECACHE_SOUND ("weapons/fsniper_shoot1.wav");
	PRECACHE_SOUND ("weapons/fsniper_eject1.wav");
	PRECACHE_SOUND ("weapons/fsniper_eject2.wav");
	PRECACHE_SOUND ("weapons/fsniper_lens_open.wav");
	PRECACHE_SOUND ("weapons/357_cock1.wav");

	m_usFSniper = PRECACHE_EVENT( 1, "events/fsniper.sc" );	//pr
}

int CFSniper::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "fsniper";
	p->iMaxAmmo1 = FSNIPER_MAX_CARRY;
	p->iMaxClip = FSNIPER_MAX_CLIP;
	p->iSlot = 3;
	p->iPosition = 1;
	p->iId = m_iId = WEAPON_FSNIPER;
	p->iWeight = FSNIPER_WEIGHT;
	p->iFlags = 0;

	return 1;
}

int CFSniper::AddToPlayer( CBasePlayer *pPlayer )
{
	if ( CBasePlayerWeapon::AddToPlayer( pPlayer ) )
	{
		MESSAGE_BEGIN( MSG_ONE, gmsgWeapPickup, NULL, pPlayer->pev );
			WRITE_BYTE( m_iId );
		MESSAGE_END();

		m_pPlayer->TextAmmo( TA_SNIPER );

		return TRUE;
	}
	return FALSE;
}

BOOL CFSniper::Deploy( )
{
	BOOL bResult = DefaultDeploy( "models/v_fsniper.mdl", "models/p_9mmAR.mdl", FSNIPER_DEPLOY, "fsniper" );

	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 36 / 16.0;
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 36 / 16.0;
	return bResult;
}


void CFSniper::PrimaryAttack()
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
		PlayEmptySound();
		m_flNextPrimaryAttack = gpGlobals->time + 0.15;
		return;
	}
	
	PLAYBACK_EVENT( 0, m_pPlayer->edict(), m_usFSniper );
	//avec un d
	PLAYBACK_EVENT_FULL ( 0, m_pPlayer->edict(), m_usFSniper, 27 / 25.0, (float *)&g_vecZero, (float *)&g_vecZero, 0, 0, 0, 0, 1, 0 );


	m_pPlayer->m_iWeaponVolume = NORMAL_GUN_VOLUME;
	m_pPlayer->Gunflash ();

	m_iClip--;

	// player "shoot" animation
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

	Vector vecSrc	 = m_pPlayer->GetGunPosition( );
	Vector vecAiming = m_pPlayer->GetAutoaimVector( AUTOAIM_8DEGREES );
	
	m_pPlayer->FireBullets( 1, vecSrc, vecAiming, Vector ( 0, 0, 0 )/*VECTOR_CONE_1DEGREES*/, 8192, BULLET_PLAYER_SNIPER, 1 );

	
	if (!m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);


	m_flNextPrimaryAttack = gpGlobals->time + 52 / 25.0;
	
	
	m_flTimeWeaponIdle = gpGlobals->time + RANDOM_FLOAT ( 10, 15 );

}


void CFSniper::SecondaryAttack(void)
{
	if ( m_pPlayer->pev->fov == 0 ) //This needed m_pPlayer->pev->fov instead of m_pPlayer->m_iFOV. modif de Roy
	{
		m_pPlayer->pev->fov = m_pPlayer->m_iFOV = 45 ; //This needed m_pPlayer->pev->fov to be added.
	}

	if ( m_flNextSecondaryAttack + 0.2 < gpGlobals->time )
	{
		m_pPlayer->pev->fov = m_pPlayer->m_iFOV = 0; //And this.
		pev->nextthink = UTIL_WeaponTimeBase() + 0.3f;
		m_flNextSecondaryAttack = gpGlobals->time + 0.3f;
		return;
	}

	else if ( m_pPlayer->pev->fov > 10)
		m_pPlayer->pev->fov = m_pPlayer->m_iFOV -= 1.5; //And this.

	pev->nextthink = UTIL_WeaponTimeBase() + 0.03f;
	m_flNextSecondaryAttack = gpGlobals->time + 0.03f;
}

void CFSniper :: Holster( int skiplocal /* = 0 */ )
{
	m_fInReload = FALSE;// cancel any reload in progress.

	if ( m_pPlayer->pev->fov !=0 )
		m_pPlayer->pev->fov = m_pPlayer->m_iFOV = 0;
}




void CFSniper::Reload( void )
{
	m_pPlayer->pev->fov = m_pPlayer->m_iFOV = 0; //champ de vision normal pendant le rechargement

	DefaultReload( FSNIPER_MAX_CLIP, FSNIPER_RELOAD, 69 / 19.0 );
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 69 / 19.0;

}



void CFSniper::WeaponIdle( void )
{

	ResetEmptySound( );

	m_pPlayer->GetAutoaimVector( AUTOAIM_5DEGREES );

	if (m_flTimeWeaponIdle > gpGlobals->time)
		return;

	int iAnim;
	switch ( RANDOM_LONG( 0, 5 ) )
	{
	case 0:
	case 1:
	case 2:
		iAnim = FSNIPER_LONGIDLE;
		m_flTimeWeaponIdle = gpGlobals->time + 25 / 4.0;
		break;
	
	default:
	case 3:
	case 4:
		iAnim = FSNIPER_IDLE1;
		m_flTimeWeaponIdle = gpGlobals->time + 24 / 9.0;
		break;

	case 5:
		iAnim = FSNIPER_FIDGET;
		m_flTimeWeaponIdle = gpGlobals->time + 50 / 13.0;
		break;
	}

	SendWeaponAnim( iAnim );

//	m_flTimeWeaponIdle = gpGlobals->time + RANDOM_FLOAT ( 10, 15 );// how long till we do this again.
}



class CFSniperAmmoClip : public CBasePlayerAmmo
{
	void Spawn( void )
	{ 
		Precache( );
		SET_MODEL(ENT(pev), "models/w_fsniperclip.mdl");
		CBasePlayerAmmo::Spawn( );
	}
	void Precache( void )
	{
		PRECACHE_MODEL ("models/w_fsniperclip.mdl");
		PRECACHE_SOUND("items/9mmclip1.wav");
	}
	BOOL AddAmmo( CBaseEntity *pOther ) 
	{ 
		int bResult = (pOther->GiveAmmo( AMMO_FSNIPERCLIP_GIVE, "9mm", _9MM_MAX_CARRY) != -1);
		if (bResult)
		{
			EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM);
		}
		return bResult;
	}
};
LINK_ENTITY_TO_CLASS( ammo_fsniper, CFSniperAmmoClip );
