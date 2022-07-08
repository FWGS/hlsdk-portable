//-------------------------------------------------
//-												---
//-			lflammes.cpp						---
//-												---
//-------------------------------------------------
//			par Julien		-----------------------
//-------------------------------------------------
//- code du lance flammes	-----------------------
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
#include "lflammes.h"
#include "decals.h"

extern int gmsgLFlammes;

enum lflammes_e 
{
	LFLAMMES_IDLE = 0,
	LFLAMMES_OPEN,
	LFLAMMES_CLOSE,
	LFLAMMES_FIRE,
	LFLAMMES_LONGIDLE,
	LFLAMMES_DEPLOY,
};



LINK_ENTITY_TO_CLASS( monster_flamme, CFlamme );


TYPEDESCRIPTION	CFlamme::m_SaveData[] = 
{
	DEFINE_FIELD( CFlamme, m_flBirthTime, FIELD_TIME ),
	DEFINE_FIELD( CFlamme, m_iMode, FIELD_INTEGER ),
	DEFINE_FIELD( CFlamme, m_flPlayerDmg, FIELD_FLOAT ),
	DEFINE_FIELD( CFlamme, m_bRestore, FIELD_INTEGER ),
	DEFINE_FIELD( CFlamme, m_flMonsterDamage, FIELD_FLOAT ),
};
//IMPLEMENT_SAVERESTORE( CFlamme, CPointEntity );



//----------------------------------------
// d


class CLFlammes : public CBasePlayerWeapon
{
public:
	void Spawn( void );
	void Precache( void );
	int iItemSlot( void ) { return 4; }
	int GetItemInfo(ItemInfo *p);
	int AddToPlayer( CBasePlayer *pPlayer );
	void PrimaryAttack( void );
	BOOL Deploy( void );
	void Holster( int skiplocal = 0 );
	void Reload( void );
	void WeaponIdle( void );

	virtual BOOL IsUseable ( void ) { return TRUE; };

	float m_flAttackReady;
	float m_flSoundStartTime;

	int		Save( CSave &save );
	int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];
};
LINK_ENTITY_TO_CLASS( weapon_lflammes, CLFlammes );


TYPEDESCRIPTION	CLFlammes::m_SaveData[] = 
{
	DEFINE_FIELD( CLFlammes, m_flAttackReady, FIELD_TIME ),
	DEFINE_FIELD( CLFlammes, m_flSoundStartTime, FIELD_TIME ),
};
IMPLEMENT_SAVERESTORE( CLFlammes, CBasePlayerWeapon );




//----------------------------------------
// spawn / pr

void CLFlammes::Spawn( )
{
	pev->classname = MAKE_STRING("weapon_lflammes");
	Precache( );
	m_iId = WEAPON_LFLAMMES;
	SET_MODEL(ENT(pev), "models/w_lflammes.mdl");

	m_iDefaultAmmo = LFLAMMES_DEFAULT_GIVE;
	m_flAttackReady = 0;
	m_flSoundStartTime = 0;

	pev->sequence = 1;
	pev->animtime = gpGlobals->time;
	pev->framerate = 10.0;

	FallInit();// get ready to fall down.
}


void CLFlammes::Precache( void )
{
	UTIL_PrecacheOther( "monster_flamme" );

	PRECACHE_MODEL("models/v_lflammes.mdl");
	PRECACHE_MODEL("models/w_lflammes.mdl");
	PRECACHE_MODEL("models/p_357.mdl");
	PRECACHE_MODEL("models/w_lflammesclip.mdl");

	PRECACHE_SOUND("items/9mmclip1.wav");
	PRECACHE_SOUND("garg/gar_flamerun1.wav" );
}


//----------------------------------------
// add / remove / bazar


int CLFlammes::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "oeufs";
	p->iMaxAmmo1 = LFLAMMES_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = WEAPON_NOCLIP;
	p->iFlags = ITEM_FLAG_NOAUTOSWITCHEMPTY | ITEM_FLAG_NOAUTORELOAD;
	p->iSlot = 3;
	p->iPosition = 4;
	p->iId = m_iId = WEAPON_LFLAMMES;
	p->iWeight = LFLAMMES_WEIGHT;

	return 1;
}


int CLFlammes::AddToPlayer( CBasePlayer *pPlayer )
{
	if ( CBasePlayerWeapon::AddToPlayer( pPlayer ) )
	{
		MESSAGE_BEGIN( MSG_ONE, gmsgWeapPickup, NULL, pPlayer->pev );
			WRITE_BYTE( m_iId );
		MESSAGE_END();

		m_pPlayer->TextAmmo( TA_LFLAMMES );

		return TRUE;
	}
	return FALSE;
}

BOOL CLFlammes::Deploy( )
{
	BOOL bResult = DefaultDeploy( "models/v_lflammes.mdl", "models/p_357.mdl", LFLAMMES_DEPLOY, "lflammes" );

	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.9;
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1.5;
	return bResult;
}


void CLFlammes::Holster( int skiplocal  )
{
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 1.0;
	m_flTimeWeaponIdle = gpGlobals->time + 10 + RANDOM_FLOAT ( 0, 5 );
}



//----------------------------------------
// attaque



void CLFlammes::PrimaryAttack()
{
	// don't fire underwater
	if (m_pPlayer->pev->waterlevel == 3)
	{
		m_flNextPrimaryAttack = gpGlobals->time + 0.15;
		return;
	}

	// enflamme le gaz

	if ( m_pPlayer->IsInGaz() == TRUE )
		m_pPlayer->m_bFireInGaz = TRUE;


	if (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 )
		return;

	// lancement de l'attaque

	if ( m_flAttackReady == 0 )
	{
		SendWeaponAnim( LFLAMMES_OPEN );
		m_flAttackReady = gpGlobals->time + 0.25;
	}

	// lancement de l'anim shoot

	else if ( m_flAttackReady < gpGlobals->time && m_flAttackReady != -1 )
	{
		m_flAttackReady = -1;
		SendWeaponAnim( LFLAMMES_FIRE );

		// son
		m_flSoundStartTime = gpGlobals->time;
		EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, "garg/gar_flamerun1.wav", 0.2, ATTN_NORM, 0, 100 );
	}

	// tir

	if ( m_flAttackReady == -1 )
	{
		m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType]--;

		if (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
			m_pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);

		UTIL_MakeVectors ( m_pPlayer->pev->v_angle );

		CFlamme *pFlamme = CFlamme::CreateFlamme(
			m_pPlayer->GetGunPosition() + gpGlobals->v_forward*15 + gpGlobals->v_right*7 - gpGlobals->v_up * 6,
			m_pPlayer->pev->v_angle 
			);

		pFlamme->pev->velocity = pFlamme->pev->velocity + m_pPlayer->pev->velocity;

		// son
		float delta = gpGlobals->time - m_flSoundStartTime;
		float flVol = delta < 1 ? 0.2 + delta*0.6 : 0.8;

		EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, "garg/gar_flamerun1.wav", flVol, ATTN_NORM, SND_CHANGE_VOL, 100 );

	}

	m_flTimeWeaponIdle = gpGlobals->time + 0.1;
	m_flNextPrimaryAttack = gpGlobals->time + 0.02;


}



//----------------------------------------
// reload



void CLFlammes::Reload( void )
{
	return;
}

//----------------------------------------
// weaponidle


void CLFlammes::WeaponIdle( void )
{
	ResetEmptySound( );

	m_pPlayer->GetAutoaimVector( AUTOAIM_10DEGREES );

	if (m_flTimeWeaponIdle > gpGlobals->time)
		return;

	// anim fermeture

	if ( m_flAttackReady == -1 )
	{
		SendWeaponAnim( LFLAMMES_CLOSE );
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.5;
		m_flAttackReady = 0;

		// son
		STOP_SOUND ( edict(), CHAN_WEAPON, "garg/gar_flamerun1.wav" );
	}
	else
	{
		m_flAttackReady = 0;

		switch ( RANDOM_LONG(0,1) )
		{
		case 1:
		default:
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + RANDOM_FLOAT( 5, 10 );
			SendWeaponAnim( LFLAMMES_IDLE );
			break;
		case 0:
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + RANDOM_FLOAT( 10,12 );
			SendWeaponAnim( LFLAMMES_LONGIDLE );
			break;
		}
	}
}


//----------------------------------------
// munitions


class CLFLammesAmmo : public CBasePlayerAmmo
{
	void Spawn( void )
	{ 
		Precache( );
		SET_MODEL(ENT(pev), "models/w_lflammesclip.mdl");

		pev->sequence = 1;
		pev->animtime = gpGlobals->time;
		pev->framerate = 1.0;

		CBasePlayerAmmo::Spawn( );
	}
	void Precache( void )
	{
		PRECACHE_MODEL ("models/w_lflammesclip.mdl");
		PRECACHE_SOUND("debris/flesh6.wav");
	}
	BOOL AddAmmo( CBaseEntity *pOther ) 
	{

		// seulement autoris

		if ( pOther->IsPlayer() == false )
			return FALSE;

		CBasePlayer *pPlayer = (CBasePlayer *) pOther;		
		CBasePlayerItem *pItem;
		CLFlammes *pLF = NULL;
		int i, fin = 0;

		for ( i = 0 ; i < MAX_ITEM_TYPES ; i++ )
		{
			pItem = pPlayer->m_rgpPlayerItems[ i ];
			
			while (pItem)
			{
				if ( !strcmp( "weapon_lflammes", STRING( pItem->pev->classname ) ) )
				{
					fin = 1;
					pLF = (CLFlammes*)pItem->GetWeaponPtr();
				}
				pItem = pItem->m_pNext;

				if ( fin )
					break;
			}

			if ( fin )
				break;
		}

		if ( pLF == NULL )
			return FALSE;

		// code traditionnel

		if (pOther->GiveAmmo( AMMO_LFLAMMESCLIPGIVE, "oeufs", LFLAMMES_MAX_CARRY ) != -1)
		{
			EMIT_SOUND(ENT(pev), CHAN_ITEM, "debris/flesh6.wav", 1, ATTN_NORM);
			return TRUE;
		}
		return FALSE;
	}
};
LINK_ENTITY_TO_CLASS( ammo_lflammes, CLFLammesAmmo );





//---------------------------------------------------------
// flammes



CFlamme *CFlamme :: CreateFlamme( Vector vecOrigin, Vector vecAngles, int imode )
{
	CFlamme *pFlamme = GetClassPtr( (CFlamme *)NULL );

	UTIL_SetOrigin( pFlamme->pev, vecOrigin );
	pFlamme->pev->angles = vecAngles;

	pFlamme->m_iMode = imode;
	pFlamme->Spawn();

	CBaseEntity *pPlayer = NULL;
	pPlayer = UTIL_FindEntityByClassname( NULL, "player" );

	MESSAGE_BEGIN( MSG_ONE, gmsgLFlammes, NULL, pPlayer->pev );

		WRITE_BYTE	( imode );				// mode
		WRITE_COORD	( ENTINDEX(pFlamme->edict()) );		// idx
		WRITE_COORD	( pFlamme->m_flBirthTime );			// age

		if ( pFlamme->m_iMode == FLAMME_ATTACHEE )
		{
			// pour les flammes attach
			WRITE_COORD	( vecOrigin.x );			// offset
			WRITE_COORD	( vecOrigin.y );			// offset
			WRITE_COORD	( vecOrigin.z );			// offset
		}
		else
		{
			WRITE_COORD	( 0.0f );			// offset
			WRITE_COORD	( 0.0f );			// offset
			WRITE_COORD	( 0.0f );			// offset
		}

	MESSAGE_END();


	return pFlamme;
}


void CFlamme :: Spawn( void )
{
	Precache( );

	pev->movetype = MOVETYPE_BOUNCEMISSILE;
//	pev->movetype = MOVETYPE_FLY;
//	pev->solid = SOLID_SLIDEBOX;
	pev->solid = SOLID_TRIGGER;

	UTIL_SetSize(pev, Vector( 0, 0, 0), Vector(0, 0, 0));
	UTIL_SetOrigin( pev, pev->origin );

	SET_MODEL(ENT(pev), "models/tank_cam.mdl");				// model
//	SET_MODEL(ENT(pev), "models/can.mdl" );				// model
	pev->classname = MAKE_STRING("monster_flamme");

	UTIL_MakeVectors ( pev->angles );

	pev->velocity	= gpGlobals->v_forward.Normalize() * 350;

	m_flBirthTime	= gpGlobals->time;
	m_flPlayerDmg	= 0;
	m_bRestore		= FALSE;

	SetThink ( &CFlamme::FlameThink );
	SetTouch ( &CFlamme::FlameTouch );
	pev->nextthink = m_flBirthTime + 0.1;


	// son
	EMIT_SOUND_DYN(ENT(pev), CHAN_STREAM, "ambience/burning1.wav", 0.8, ATTN_NORM, 0, 100 );

}

void CFlamme :: Precache( void )
{
	PRECACHE_MODEL("models/tank_cam.mdl" );
	PRECACHE_MODEL("models/can.mdl" );
	PRECACHE_MODEL("sprites/lflammes02.spr" );
	PRECACHE_MODEL("sprites/tank_smokeball.spr" );
	PRECACHE_SOUND("ambience/burning1.wav" );
}


BOOL CFlamme :: CanCatchMonster ( CBaseMonster * pMonster )
{
	if ( pMonster == NULL )
		return FALSE;

	if ( pMonster->pev->deadflag == DEAD_DEAD || pMonster->pev->deadflag == DEAD_DYING )
		return FALSE;
	
	if ( pMonster->m_MonsterState == MONSTERSTATE_HUNT  || pMonster->m_MonsterState == MONSTERSTATE_PLAYDEAD || pMonster->m_MonsterState == MONSTERSTATE_DEAD )
		return FALSE;


	if ( !(
		FClassnameIs ( pMonster->pev, "monster_human_grunt" )		||
		FClassnameIs ( pMonster->pev, "monster_sniper" )			||
		FClassnameIs ( pMonster->pev, "monster_rpg_grunt" )			||
		FClassnameIs ( pMonster->pev, "monster_human_assassin" )	||

		FClassnameIs ( pMonster->pev, "monster_scientist" )			||
		FClassnameIs ( pMonster->pev, "monster_barney" )			||

		FClassnameIs ( pMonster->pev, "player" )					||

		FClassnameIs ( pMonster->pev, "monster_headcrab" )			||
		FClassnameIs ( pMonster->pev, "monster_alien_slave" )		||
		FClassnameIs ( pMonster->pev, "monster_bullchicken" )		||
		FClassnameIs ( pMonster->pev, "monster_barnacle" )			||
		FClassnameIs ( pMonster->pev, "monster_houndeye" )			||
		FClassnameIs ( pMonster->pev, "monster_flybee" )			||
		FClassnameIs ( pMonster->pev, "monster_luciole" )			||
		FClassnameIs ( pMonster->pev, "monster_alien_controller" )
		))		
		return FALSE;



	return TRUE;
}


float CFlamme :: FlameDamageMonster ( CBaseMonster *pMonster )
{

	if ( FClassnameIs ( pMonster->pev, "monster_luciole" ) )
		return 100;

	// temps mis 

	float flAgonie = RANDOM_FLOAT ( 4, 6 );

	// dommages par dixi

	float flDamage = ( pMonster->pev->max_health / flAgonie ) * 0.1;

	return flDamage;
}




void EXPORT CFlamme :: FlameThink ( void )
{

	//------------------------------------------
	// flamme libre
	//------------------------------------------

	CBaseEntity *pPlayer = NULL;
	pPlayer = UTIL_FindEntityByClassname( NULL, "player" );

	// restoration apr

	if ( m_bRestore == TRUE )
	{
		if ( m_iMode == FLAMME_LIBRE )
		{
			MESSAGE_BEGIN( MSG_ONE, gmsgLFlammes, NULL, pPlayer->pev );

				WRITE_BYTE	( m_iMode );				// mode
				WRITE_COORD	( ENTINDEX(edict()) );		// idx
				WRITE_COORD	( m_flBirthTime );			// age

				WRITE_COORD	( 0.0f );			// offset
				WRITE_COORD	( 0.0f );			// offset
				WRITE_COORD	( 0.0f );			// offset

			MESSAGE_END();

		}
		else if ( m_iMode == FLAMME_ATTACHEE )
		{
			CBaseEntity *pEntityMonster = CBaseEntity::Instance ( pev->aiment );

			if ( pEntityMonster != NULL )
			{
				CBaseMonster *pMonster = pEntityMonster->MyMonsterPointer();

				Vector vecOrigin = pMonster->Center()-pMonster->pev->origin;

				MESSAGE_BEGIN( MSG_ONE, gmsgLFlammes, NULL, pPlayer->pev );

					WRITE_BYTE	( m_iMode );				// mode
					WRITE_COORD	( ENTINDEX(edict()) );		// idx
					WRITE_COORD	( m_flBirthTime );			// age

					WRITE_COORD	( vecOrigin.x );			// offset
					WRITE_COORD	( vecOrigin.y );			// offset
					WRITE_COORD	( vecOrigin.z );			// offset

				MESSAGE_END();
			}
		}
		m_bRestore = FALSE;
	}



	if ( m_iMode == FLAMME_LIBRE )
	{

		// vitesse

		if ( gpGlobals->time - m_flBirthTime > 1 )
			pev->velocity = pev->velocity.Normalize() * Q_min ( 50, pev->velocity.Length() );

		// destruction

		if ( gpGlobals->time - m_flBirthTime > 3 || pev->waterlevel > 0 )
		{
			MESSAGE_BEGIN( MSG_ONE, gmsgLFlammes, NULL, pPlayer->pev );

				WRITE_BYTE	( DETRUIT_FLAMME );			// mode
				WRITE_COORD	( ENTINDEX(edict()) );		// idx

			MESSAGE_END();

			pev->nextthink = gpGlobals->time + 0.1;
			SetThink ( &CBaseEntity::SUB_Remove );

			// son
			STOP_SOUND(ENT(pev), CHAN_STREAM, "ambience/burning1.wav" );

		}
	}


	//------------------------------------------
	// d
	//------------------------------------------
	
	pev->nextthink = gpGlobals->time + 0.1;

	CBaseEntity *pEntity = NULL;
	pEntity = UTIL_FindEntityInSphere( NULL, pev->origin, gpGlobals->time-m_flBirthTime < 1 ? FLAMME_RADIUS_SMALL : FLAMME_RADIUS_BIG );
	
	while ( pEntity != NULL)
	{
		CBaseMonster *pMonster = pEntity->MyMonsterPointer( );

		if ( CanCatchMonster ( pMonster ) == FALSE )
		{
			pEntity = UTIL_FindEntityInSphere( pEntity, pev->origin, gpGlobals->time-m_flBirthTime < 1 ? FLAMME_RADIUS_SMALL : FLAMME_RADIUS_BIG );
			continue;
		}

		pMonster->pev->renderfx = kRenderFxGlowShell;
		pMonster->pev->rendercolor = Vector ( 255, 110, 15 );

		pMonster->SetState ( MONSTERSTATE_HUNT );
		pMonster->SetConditions(
			bits_COND_LIGHT_DAMAGE |
			bits_COND_HEAVY_DAMAGE |
			bits_COND_HEAR_SOUND |
			bits_COND_ENEMY_DEAD );


		// flamme ma

		CFlamme *pFlamme = CFlamme :: CreateFlamme ( pMonster->Center()-pMonster->pev->origin, Vector(0,0,0), FLAMME_ATTACHEE );
		pFlamme->pev->movetype = MOVETYPE_FOLLOW;
		pFlamme->pev->aiment = pMonster->edict();

		// pr

		pFlamme->m_flMonsterDamage = FlameDamageMonster ( pMonster );

		// fin de la boucle

		pEntity = UTIL_FindEntityInSphere( pEntity, pev->origin, gpGlobals->time-m_flBirthTime < 1 ? FLAMME_RADIUS_SMALL : FLAMME_RADIUS_BIG );
	}

	// lumi

	float ratio = 1;

	if ( gpGlobals->time - m_flBirthTime < 0.5 )
		ratio = (gpGlobals->time - m_flBirthTime) / 0.5;

	if ( gpGlobals->time - m_flBirthTime > 2.5 && m_iMode == FLAMME_LIBRE )
		ratio = Q_min( 0, 1 - (gpGlobals->time - m_flBirthTime - 2.5));

	MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, pev->origin );
		WRITE_BYTE(TE_DLIGHT);
		WRITE_COORD(pev->origin.x);	// X
		WRITE_COORD(pev->origin.y);	// Y
		WRITE_COORD(pev->origin.z);	// Z
		WRITE_BYTE( 10 * RANDOM_FLOAT(0.8, 1.2) * ratio );		// radius * 0.1

		WRITE_BYTE( RANDOM_FLOAT ( 240, 255 ) );		// r
		WRITE_BYTE( RANDOM_FLOAT ( 180, 213 ) );		// g
		WRITE_BYTE( RANDOM_FLOAT ( 96,  110 ) );		// b

		WRITE_BYTE( 3 );		// time * 10
		WRITE_BYTE( 0 );		// decay * 0.1
	MESSAGE_END( );


	// son
	float delta = gpGlobals->time - m_flBirthTime;
	float flVol = delta < 3 ? ( delta > 2 ? 0.4 + 0.6 * (1 - (delta-2)) : 1 ) : 0;

	EMIT_SOUND_DYN(ENT(pev), CHAN_STREAM, "ambience/burning1.wav", flVol, ATTN_NORM, SND_CHANGE_VOL, 100 );


	if ( m_iMode == FLAMME_LIBRE )
		return;



	//------------------------------------------
	// routine de contr
	//------------------------------------------


	CBaseEntity *pEntityMonster = CBaseEntity::Instance ( pev->aiment );

	CBaseMonster *pMonster;

	if ( pEntityMonster != NULL )
		pMonster = pEntityMonster->MyMonsterPointer();


	// entit

	if ( pEntityMonster == NULL || pMonster == NULL || !UTIL_IsValidEntity(pMonster->edict()) || pMonster->pev->effects & EF_NODRAW )
	{
		MESSAGE_BEGIN( MSG_ONE, gmsgLFlammes, NULL, pPlayer->pev );

			WRITE_BYTE	( DETRUIT_FLAMME );			// mode
			WRITE_COORD	( ENTINDEX(edict()) );		// idx

		MESSAGE_END();

		pev->nextthink = gpGlobals->time + 0.1;
		SetThink ( &CBaseEntity::SUB_Remove );
	}

	// dommages

	else if ( pMonster->pev->deadflag != DEAD_DEAD && pMonster->pev->deadflag != DEAD_DYING && !pMonster->IsPlayer() )
	{
		pMonster->TakeDamage ( pev, pev, m_flMonsterDamage, DMG_BURN | DMG_NEVERGIB );
	}

	// joueur

	else if ( pMonster->IsPlayer() )
	{
		// fini

		if ( gpGlobals->time - m_flBirthTime > PLAYER_BURN_TIME || pMonster->pev->waterlevel >= 2 )
		{
			pMonster->SetState ( MONSTERSTATE_NONE );

			MESSAGE_BEGIN( MSG_ONE, gmsgLFlammes, NULL, pPlayer->pev );

				WRITE_BYTE	( DETRUIT_FLAMME );			// mode
				WRITE_COORD	( ENTINDEX(edict()) );		// idx

			MESSAGE_END();

			pev->nextthink = gpGlobals->time + 0.1;
			SetThink ( &CBaseEntity::SUB_Remove );
			return;
		}

		// dommages

		else 
		{
			m_flPlayerDmg += FLAMME_DAMAGE_PLAYER*0.1/PLAYER_BURN_TIME;

			if ( (int)m_flPlayerDmg > 1 )
			{
				pMonster->TakeDamage ( pev, pev, (int)m_flPlayerDmg, DMG_BURN );
				m_flPlayerDmg -= (int)m_flPlayerDmg;
			}
		}
	}

	// mort

	else if ( pMonster->pev->deadflag == DEAD_DEAD )
	{
		MESSAGE_BEGIN( MSG_ONE, gmsgLFlammes, NULL, pPlayer->pev );

			WRITE_BYTE	( FLAMME_DEAD );			// mode
			WRITE_COORD	( ENTINDEX(edict()) );		// idx

		MESSAGE_END();

		
		pev->nextthink = gpGlobals->time + 0.1;
		SetThink ( &CBaseEntity::SUB_Remove );
	}



}

void CFlamme :: FlameTouch( CBaseEntity *pOther )
{
	TraceResult trace = UTIL_GetGlobalTrace( );

	if ( pOther && pOther->IsBSPModel() )
	{
		pev->velocity = pev->velocity.Normalize() * Q_min ( 30, pev->velocity.Length() );
		UTIL_DecalTrace( &trace, DECAL_SCORCH1 + RANDOM_LONG(0,1) );
	}
}



int CFlamme :: Save( CSave &save )
{
	if ( !CPointEntity::Save(save) )
		return 0;

	return save.WriteFields( "CFlamme", this, m_SaveData, ARRAYSIZE(m_SaveData) );
}

int CFlamme :: Restore( CRestore &restore )		// s execute lors du chargement rapide
{
	if ( !CPointEntity::Restore(restore) )
		return 0;

	int status = restore.ReadFields( "CFlamme", this, m_SaveData, ARRAYSIZE(m_SaveData) );

	//------------
	
	m_bRestore = TRUE;

	//------------
	
	return status;
}
