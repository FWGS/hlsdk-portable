/******************************************
*                                         *
*	Fichier m16 par Julien				  *
*                                         *
******************************************/
//10/03/01 création de l'arme
//11/03/01 force de recul et ralentissement / retrait du lance-grenades / ajout dans le hud


#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"
#include "soundent.h"
#include "gamerules.h"


enum m16_e
{
	M16_LONGIDLE = 0,
	M16_IDLE1,
	// modif de Julien 12/03/01
	M16_RELOAD,
	M16_DEPLOY,
	M16_FIRE1,
	M16_LAUNCH,
};


class CM16 : public CBasePlayerWeapon
{
public:
	void Spawn( void );
	void Precache( void );
	int iItemSlot( void ) { return 3; }
	int GetItemInfo(ItemInfo *p);
	int AddToPlayer( CBasePlayer *pPlayer );

	void PrimaryAttack( void );
	void SecondaryAttack( void );
	int SecondaryAmmoIndex( void );
	BOOL Deploy( void );
	void Reload( void );
	void WeaponIdle( void );
	float m_flNextAnimTime;
	int m_iShell;

private:
	unsigned short m_usM16;
};
LINK_ENTITY_TO_CLASS( weapon_m16, CM16 );


//=========================================================
//=========================================================
int CM16::SecondaryAmmoIndex( void )
{
	return m_iSecondaryAmmoType;
}

void CM16::Spawn( )
{
	pev->classname = MAKE_STRING("weapon_m16"); // hack to allow for old names
	Precache( );
	SET_MODEL(ENT(pev), "models/w_m16.mdl");
	m_iId = WEAPON_M16;

	m_iDefaultAmmo = M16_DEFAULT_GIVE;

	FallInit();// get ready to fall down.
}


void CM16::Precache( void )
{
	//modif de Julien 12/03/01
	PRECACHE_MODEL("models/v_m16.mdl");
	//
	PRECACHE_MODEL("models/w_m16.mdl");
	PRECACHE_MODEL("models/p_9mmAR.mdl");

	m_iShell = PRECACHE_MODEL ("models/m16_shell.mdl");// brass shellTE_MODEL

	PRECACHE_MODEL("models/grenade.mdl");

	PRECACHE_MODEL("models/w_m16clip.mdl");
	PRECACHE_SOUND("items/9mmclip1.wav");              

	PRECACHE_SOUND("items/clipinsert1.wav");
	PRECACHE_SOUND("items/cliprelease1.wav");

	PRECACHE_SOUND ("weapons/m16_shoot_1.wav");
	PRECACHE_SOUND ("weapons/m16_shoot_2.wav");
	PRECACHE_SOUND ("weapons/m16_reload.wav");

	PRECACHE_SOUND ("weapons/357_cock1.wav");

	PRECACHE_SOUND( "weapons/glauncher.wav" );
	PRECACHE_SOUND( "weapons/glauncher2.wav" );


	m_usM16 = PRECACHE_EVENT( 1, "events/m16.sc" );

}

int CM16::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "m16";
	p->iMaxAmmo1 = M16_MAX_CARRY;
	p->iMaxClip = M16_MAX_CLIP;
	p->pszAmmo2 = "ARgrenades";
	p->iMaxAmmo2 = M203_GRENADE_MAX_CARRY;
	p->iSlot = 2;
	p->iPosition = 3;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_M16;
	p->iWeight = M16_WEIGHT;

	return 1;
}

int CM16::AddToPlayer( CBasePlayer *pPlayer )
{
	if ( CBasePlayerWeapon::AddToPlayer( pPlayer ) )
	{
		MESSAGE_BEGIN( MSG_ONE, gmsgWeapPickup, NULL, pPlayer->pev );
			WRITE_BYTE( m_iId );
		MESSAGE_END();

		m_pPlayer->TextAmmo( TA_M16 );

		return TRUE;
	}
	return FALSE;
}

BOOL CM16::Deploy( )
{
	//modifié par Julien
	BOOL bResult = DefaultDeploy( "models/v_m16.mdl", "models/p_9mmAR.mdl", M16_DEPLOY, "m16" );
	
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 60 / 70.0;
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 60 / 70.0;
	return bResult;

}


void CM16::PrimaryAttack()
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
	
	PLAYBACK_EVENT( 0, m_pPlayer->edict(), m_usM16 );

	m_pPlayer->m_iWeaponVolume = NORMAL_GUN_VOLUME;
	m_pPlayer->Gunflash ();

	m_iClip--;

	// player "shoot" animation
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

	Vector vecSrc	 = m_pPlayer->GetGunPosition( );
	Vector vecAiming = m_pPlayer->GetAutoaimVector( AUTOAIM_5DEGREES );
	
	// single player spread
	m_pPlayer->FireBullets( 1, vecSrc, vecAiming, VECTOR_CONE_4DEGREES, 8192, BULLET_PLAYER_M16, 2 );

	if (!m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);


	m_flNextPrimaryAttack = m_flNextPrimaryAttack + 0.1;
	if (m_flNextPrimaryAttack < gpGlobals->time)
		m_flNextPrimaryAttack = gpGlobals->time + 0.1;
	
	
	m_flTimeWeaponIdle = gpGlobals->time + RANDOM_FLOAT ( 10, 15 );

}


void CM16::SecondaryAttack(void)
{

	// don't fire underwater
	if (m_pPlayer->pev->waterlevel == 3)
	{
		PlayEmptySound( );
		m_flNextPrimaryAttack = gpGlobals->time + 0.15;
		return;
	}

	if (m_pPlayer->m_rgAmmo[m_iSecondaryAmmoType] == 0)
	{
		PlayEmptySound( );
		return;
	}

	m_pPlayer->m_iWeaponVolume = NORMAL_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = BRIGHT_GUN_FLASH;

	m_pPlayer->m_iExtraSoundTypes = bits_SOUND_DANGER;
	m_pPlayer->m_flStopExtraSoundTime = gpGlobals->time + 0.2;
			
	m_pPlayer->m_rgAmmo[m_iSecondaryAmmoType]--;

	SendWeaponAnim( M16_LAUNCH );

	// player "shoot" animation
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

	if ( RANDOM_LONG(0,1) )
		EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/glauncher.wav", 0.8, ATTN_NORM);
	else
		EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/glauncher2.wav", 0.8, ATTN_NORM);
 
	UTIL_MakeVectors( m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle );

	// we don't add in player velocity anymore.
	CGrenade::ShootContact( m_pPlayer->pev, 
							m_pPlayer->pev->origin + m_pPlayer->pev->view_ofs + gpGlobals->v_forward * 16, 
							gpGlobals->v_forward * 800 );
	

	m_flNextPrimaryAttack = gpGlobals->time + 1;
	m_flNextSecondaryAttack = gpGlobals->time + 1;
	m_flTimeWeaponIdle = gpGlobals->time + 5;// idle pretty soon after shooting.

	if (!m_pPlayer->m_rgAmmo[m_iSecondaryAmmoType])
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);

	m_pPlayer->pev->punchangle.x -= 10;

}


void CM16::Reload( void )
{
//	EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/m16_reload.wav", 0.8, ATTN_NORM);
	DefaultReload( M16_MAX_CLIP, M16_RELOAD, 93 / 32.0 );
}



void CM16::WeaponIdle( void )
{
	ResetEmptySound( );

	m_pPlayer->GetAutoaimVector( AUTOAIM_5DEGREES );

	if (m_flTimeWeaponIdle > gpGlobals->time)
		return;

	int iAnim;
	switch ( RANDOM_LONG( 0, 2 ) )
	{
	case 0:
	case 1:
		iAnim = M16_LONGIDLE;
		m_flTimeWeaponIdle = gpGlobals->time + 70 / 14.0;
		break;
	
	default:
	case 2:
		iAnim = M16_IDLE1;
		m_flTimeWeaponIdle = gpGlobals->time + 62 / 8.0;
		break;
	}

	SendWeaponAnim( iAnim );

//	m_flTimeWeaponIdle = gpGlobals->time + RANDOM_FLOAT ( 10, 15 );// how long till we do this again.
}



class CM16AmmoClip : public CBasePlayerAmmo
{
	void Spawn( void )
	{ 
		Precache( );
		SET_MODEL(ENT(pev), "models/w_m16clip.mdl");
		CBasePlayerAmmo::Spawn( );
	}
	void Precache( void )
	{
		PRECACHE_MODEL ("models/w_m16clip.mdl");
		PRECACHE_SOUND("items/9mmclip1.wav");
	}
	BOOL AddAmmo( CBaseEntity *pOther ) 
	{ 
		int bResult = (pOther->GiveAmmo( AMMO_M16CLIP_GIVE, "m16", M16_MAX_CARRY) != -1);
		if (bResult)
		{
			EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM);
		}
		return bResult;
	}
};
LINK_ENTITY_TO_CLASS( ammo_m16, CM16AmmoClip );


class CMP5AmmoGrenade : public CBasePlayerAmmo
{
	void Spawn( void )
	{ 
		Precache( );
		SET_MODEL(ENT(pev), "models/w_ARgrenade.mdl");
		CBasePlayerAmmo::Spawn( );
	}
	void Precache( void )
	{
		PRECACHE_MODEL ("models/w_ARgrenade.mdl");
		PRECACHE_SOUND("items/9mmclip1.wav");
	}
	BOOL AddAmmo( CBaseEntity *pOther ) 
	{ 
		int bResult = (pOther->GiveAmmo( AMMO_M203BOX_GIVE, "ARgrenades", M203_GRENADE_MAX_CARRY ) != -1);

		if (bResult)
		{
			EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM);
		}
		return bResult;
	}
};
LINK_ENTITY_TO_CLASS( ammo_mp5grenades, CMP5AmmoGrenade );
LINK_ENTITY_TO_CLASS( ammo_ARgrenades, CMP5AmmoGrenade );


