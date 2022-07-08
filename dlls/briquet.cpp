//-------------------------------------------------
//-												---
//-			briquet.cpp							---
//-												---
//-------------------------------------------------
//			par Julien		-----------------------
//-------------------------------------------------
//- code du briquet servant de lampe torche		---
//-------------------------------------------------



//----------------------------------------
// inclusions

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "weapons.h"
#include "monsters.h"
#include "player.h"
#include "gamerules.h"
#include "effects.h"
#include "customentity.h"

extern int gmsgBriquetSwitch;

enum briquet_e 
{
	BRIQUET_IDLE = 0,
	BRIQUET_DRAW,
	BRIQUET_ALLUME_ESSAIE,
	BRIQUET_ALLUME,
	BRIQUET_ALLUME_IDLE,
	BRIQUET_ETEINT,
};

#define BRIQUET_IDLE_TIME					19 / 10.0
#define BRIQUET_DRAW_TIME					9 / 20.0
#define BRIQUET_ALLUME_ESSAIE_TIME			39 / 60.0
#define BRIQUET_ALLUME_TIME					9 / 2.0
#define BRIQUET_ALLUME_IDLE_TIME			14 / 30.0

#define BRIQUET_SPRITE						"sprites/briquet.spr"
#define BRIQUET_ETINCELLES_SPRITE			"sprites/richo1.spr"

//----------------------------------------
// d


class CBriquet : public CBasePlayerWeapon
{
public:
	void Spawn( void );
	void Precache( void );

	int iItemSlot( void ) { return 1; }
	int GetItemInfo(ItemInfo *p);

	int AddToPlayer( CBasePlayer *pPlayer );
	BOOL Deploy( void );
	void Holster( int skiplocal = 0 );

	void PrimaryAttack( void );
	void Reload( void );
	void WeaponIdle( void );

	BOOL ShouldWeaponIdle( void ) { return TRUE; };

	int		m_bActif;
	float	m_flNextLight;
	BOOL	m_bTransition;

	virtual int	Save	( CSave &save );
	virtual int	Restore	( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

};
LINK_ENTITY_TO_CLASS( weapon_briquet, CBriquet );

TYPEDESCRIPTION CBriquet::m_SaveData[] =
{
	DEFINE_FIELD( CBriquet, m_bActif, FIELD_INTEGER ),
	DEFINE_FIELD( CBriquet, m_flNextLight, FIELD_TIME ),
	DEFINE_FIELD( CBriquet, m_bTransition, FIELD_BOOLEAN ),
};

// IMPLEMENT_SAVERESTORE( CBriquet, CBasePlayerWeapon );



//----------------------------------------
// spawn / pr

void CBriquet::Spawn( )
{
	pev->classname = MAKE_STRING("weapon_briquet");
	Precache( );
	m_iId = WEAPON_BRIQUET;
	SET_MODEL(ENT(pev), "models/w_briquet.mdl");
	m_iClip = -1;

	m_bTransition = FALSE;

	FallInit();// get ready to fall down.
}


void CBriquet::Precache( void )
{
	PRECACHE_MODEL("models/v_briquet.mdl");
	PRECACHE_MODEL("models/w_briquet.mdl");
	PRECACHE_MODEL("models/p_crowbar.mdl");
	PRECACHE_MODEL(BRIQUET_SPRITE);
	PRECACHE_MODEL(BRIQUET_ETINCELLES_SPRITE);

	PRECACHE_SOUND("items/9mmclip1.wav");     
}


//----------------------------------------
// add / remove / bazar


int CBriquet::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = NULL;
	p->iMaxAmmo1 = -1;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = WEAPON_NOCLIP;
	p->iFlags = 0;
	p->iSlot = 0;
	p->iPosition = 1;
	p->iId = m_iId = WEAPON_BRIQUET;
	p->iWeight = BRIQUET_WEIGHT;

	return 1;
}


int CBriquet::AddToPlayer( CBasePlayer *pPlayer )
{
	if ( CBasePlayerWeapon::AddToPlayer( pPlayer ) )
	{
		MESSAGE_BEGIN( MSG_ONE, gmsgWeapPickup, NULL, pPlayer->pev );
			WRITE_BYTE( m_iId );
		MESSAGE_END();

//		m_pPlayer->TextAmmo( TA_BRIQUET );

		return TRUE;
	}
	return FALSE;
}

BOOL CBriquet::Deploy( )
{
	BOOL bResult = DefaultDeploy( "models/v_briquet.mdl", "models/p_crowbar.mdl", BRIQUET_DRAW, "briquet" );

	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + BRIQUET_DRAW_TIME;
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1.5;

	m_bActif = 0;

	return bResult;
}


void CBriquet::Holster( int skiplocal  )
{
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 1.0;
	m_flTimeWeaponIdle = gpGlobals->time + 10 + RANDOM_FLOAT ( 0, 5 );

	// 
	MESSAGE_BEGIN( MSG_ONE, gmsgBriquetSwitch, NULL, m_pPlayer->pev );
		WRITE_BYTE ( 0 );						// 0 == off, 1 == on
	MESSAGE_END();

}



//----------------------------------------
// attaque



void CBriquet::PrimaryAttack()
{
	// sous l'eau
	if (m_pPlayer->pev->waterlevel >= 2)
	{
		if ( m_bActif == 1 )
		{
			// 
			MESSAGE_BEGIN( MSG_ONE, gmsgBriquetSwitch, NULL, m_pPlayer->pev );
				WRITE_BYTE ( 0 );						// 0 == off, 1 == on
			MESSAGE_END();

			SendWeaponAnim( BRIQUET_ETEINT );

			m_bActif = 0;
			m_flTimeWeaponIdle = gpGlobals->time + 1;
			m_flNextPrimaryAttack = gpGlobals->time + 0.3;
		}
		else
		{
			PlayEmptySound( );
			m_flNextPrimaryAttack = gpGlobals->time + 0.2;
		}
		return;
	}

	// enflamme le gaz

	if ( m_pPlayer->IsInGaz() == TRUE )
			m_pPlayer->m_bFireInGaz = TRUE;


	// d

	if ( m_bActif == 1 )
	{
		// 
		MESSAGE_BEGIN( MSG_ONE, gmsgBriquetSwitch, NULL, m_pPlayer->pev );
			WRITE_BYTE ( 0 );						// 0 == off, 1 == on
		MESSAGE_END();

		SendWeaponAnim( BRIQUET_ETEINT );

		m_bActif = 0;
		m_flTimeWeaponIdle = gpGlobals->time + 1;
		m_flNextPrimaryAttack = gpGlobals->time + 0.3;


		return;
	}

	// 33% de chances de l'allumer

	if ( RANDOM_FLOAT(0,1) < 0.33 )
	{
		SendWeaponAnim( BRIQUET_ALLUME );
		m_flTimeWeaponIdle = gpGlobals->time + BRIQUET_ALLUME_TIME;
		m_flNextPrimaryAttack = gpGlobals->time + 1;

		m_bActif = 1;

		MESSAGE_BEGIN( MSG_ONE, gmsgBriquetSwitch, NULL, m_pPlayer->pev );
			WRITE_BYTE ( 1 );						// 0 == off, 1 == on
		MESSAGE_END();


	}

	else
	{
		SendWeaponAnim( BRIQUET_ALLUME_ESSAIE );
		m_flTimeWeaponIdle = gpGlobals->time + BRIQUET_ALLUME_ESSAIE_TIME;

		CSprite *pEtincelle = CSprite :: SpriteCreate (	BRIQUET_ETINCELLES_SPRITE, Vector (0,0,0), FALSE );

		pEtincelle->SetAttachment ( m_pPlayer->edict(), 1 );
		pEtincelle->SetTransparency( kRenderTransAdd, 255, 255, 255, 255, kRenderFxNoDissipation );
		pEtincelle->SetScale( 0.05 );
		pEtincelle->SetThink ( &CBaseEntity::SUB_Remove );
		pEtincelle->pev->nextthink = gpGlobals->time + 0.05;

		m_flNextPrimaryAttack = gpGlobals->time + 0.3;
	}


}



//----------------------------------------
// reload



void CBriquet::Reload( void )
{
	return;
}

//----------------------------------------
// weaponidle


void CBriquet::WeaponIdle( void )
{
	ResetEmptySound( );

	// restoration de la flamme 

	if ( m_bTransition == TRUE )
	{
		MESSAGE_BEGIN( MSG_ONE, gmsgBriquetSwitch, NULL, m_pPlayer->pev );
		WRITE_BYTE ( m_bActif == 1 ? 1 : 0 );
		MESSAGE_END();

		m_bTransition = FALSE;
	}

	m_pPlayer->GetAutoaimVector( AUTOAIM_10DEGREES );

	// lumi

	if ( m_bActif == 1 && gpGlobals->time > m_flNextLight )
	{
		m_flNextLight = UTIL_WeaponTimeBase() + 0.15;

		Vector vecSrc = m_pPlayer->Center ();

		MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, vecSrc );
			WRITE_BYTE(TE_DLIGHT);
			WRITE_COORD(vecSrc.x);	// X
			WRITE_COORD(vecSrc.y);	// Y
			WRITE_COORD(vecSrc.z);	// Z
			WRITE_BYTE( 15 * RANDOM_FLOAT(0.8, 1.2) );		// radius * 0.1
			WRITE_BYTE( 255 );		// r
			WRITE_BYTE( 180 );		// g
			WRITE_BYTE( 96 );		// b
			WRITE_BYTE( 3 );		// time * 10
			WRITE_BYTE( 0 );		// decay * 0.1
		MESSAGE_END( );

	}

	// eau

	if ( m_pPlayer->pev->waterlevel >= 2 && m_bActif == 1)
		PrimaryAttack();


	if (m_flTimeWeaponIdle > gpGlobals->time)
		return;

	

	int iAnim;

	if ( m_bActif == 1 )
		iAnim = BRIQUET_ALLUME_IDLE;

	else
		iAnim = BRIQUET_IDLE;

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + RANDOM_FLOAT( 5, 12 );


	SendWeaponAnim( iAnim );
}


//--------------------------------------------------------------------------
// restoration de la flamme 

int CBriquet::Save( CSave &save )
{
	if ( !CBasePlayerWeapon::Save(save) )
		return 0;

	return save.WriteFields( "CBriquet", this, m_SaveData, ARRAYSIZE(m_SaveData) );
}

int CBriquet::Restore( CRestore &restore )		// s execute lors du chargement rapide
{
	if ( !CBasePlayerWeapon::Restore(restore) )
		return 0;

	int status = restore.ReadFields( "CBriquet", this, m_SaveData, ARRAYSIZE(m_SaveData) );

	//-----------------------

	m_bTransition = TRUE;
	
	return status;
}

