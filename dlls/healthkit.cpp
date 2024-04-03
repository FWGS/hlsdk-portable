/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"
#include "items.h"
#include "gamerules.h"
#include "game.h"
#include "actanimating.h"
//#include "effects.h"

extern int gmsgItemPickup;

class CHealthKit : public CItem
{
	void Spawn( void );
	void Precache( void );
	BOOL MyTouch( CBasePlayer *pPlayer );
/*
	virtual int Save( CSave &save ); 
	virtual int Restore( CRestore &restore );
	static TYPEDESCRIPTION m_SaveData[];
*/
};

LINK_ENTITY_TO_CLASS( item_healthkit, CHealthKit )

/*
TYPEDESCRIPTION	CHealthKit::m_SaveData[] =
{

};

IMPLEMENT_SAVERESTORE( CHealthKit, CItem )
*/

void CHealthKit::Spawn( void )
{
	Precache();
	SET_MODEL( ENT( pev ), "models/w_medkit.mdl" );

	CItem::Spawn();
}

void CHealthKit::Precache( void )
{
	PRECACHE_MODEL( "models/w_medkit.mdl" );
	PRECACHE_SOUND( "items/smallmedkit1.wav" );
}

BOOL CHealthKit::MyTouch( CBasePlayer *pPlayer )
{
	if( pPlayer->pev->deadflag != DEAD_NO )
	{
		return FALSE;
	}

	if( pPlayer->TakeHealth( gSkillData.healthkitCapacity, DMG_GENERIC ) )
	{
		MESSAGE_BEGIN( MSG_ONE, gmsgItemPickup, NULL, pPlayer->pev );
			WRITE_STRING( STRING( pev->classname ) );
		MESSAGE_END();

		EMIT_SOUND( ENT( pPlayer->pev ), CHAN_ITEM, "items/smallmedkit1.wav", 1, ATTN_NORM );

		if( g_pGameRules->ItemShouldRespawn( this ) )
		{
			Respawn();
		}
		else
		{
			UTIL_Remove( this );	
		}

		return TRUE;
	}

	return FALSE;
}

//-------------------------------------------------------------
// Wall mounted health kit
//-------------------------------------------------------------
class CWallHealth : public CBaseToggle
{
public:
	void Spawn();
	void Precache( void );
	void EXPORT Off( void );
	void EXPORT Recharge( void );
	void KeyValue( KeyValueData *pkvd );
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	virtual int ObjectCaps( void ) { return ( CBaseToggle::ObjectCaps() | FCAP_CONTINUOUS_USE ) & ~FCAP_ACROSS_TRANSITION; }
	virtual int Save( CSave &save );
	virtual int Restore( CRestore &restore );

	static TYPEDESCRIPTION m_SaveData[];

	float m_flNextCharge; 
	int m_iReactivate ; // DeathMatch Delay until reactvated
	int m_iJuice;
	int m_iOn;			// 0 = off, 1 = startup, 2 = going
	float m_flSoundTime;
};

TYPEDESCRIPTION CWallHealth::m_SaveData[] =
{
	DEFINE_FIELD( CWallHealth, m_flNextCharge, FIELD_TIME ),
	DEFINE_FIELD( CWallHealth, m_iReactivate, FIELD_INTEGER ),
	DEFINE_FIELD( CWallHealth, m_iJuice, FIELD_INTEGER ),
	DEFINE_FIELD( CWallHealth, m_iOn, FIELD_INTEGER ),
	DEFINE_FIELD( CWallHealth, m_flSoundTime, FIELD_TIME ),
};

IMPLEMENT_SAVERESTORE( CWallHealth, CBaseToggle )

LINK_ENTITY_TO_CLASS( func_healthcharger, CWallHealth )

void CWallHealth::KeyValue( KeyValueData *pkvd )
{
	if( FStrEq(pkvd->szKeyName, "style" ) ||
		FStrEq( pkvd->szKeyName, "height" ) ||
		FStrEq( pkvd->szKeyName, "value1" ) ||
		FStrEq( pkvd->szKeyName, "value2" ) ||
		FStrEq( pkvd->szKeyName, "value3" ) )
	{
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "dmdelay" ) )
	{
		m_iReactivate = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
		CBaseToggle::KeyValue( pkvd );
}

void CWallHealth::Spawn()
{
	Precache();

	pev->solid = SOLID_BSP;
	pev->movetype = MOVETYPE_PUSH;

	UTIL_SetOrigin( pev, pev->origin );		// set size and link into world
	UTIL_SetSize( pev, pev->mins, pev->maxs );
	SET_MODEL( ENT( pev ), STRING( pev->model ) );
	m_iJuice = (int)gSkillData.healthchargerCapacity;
	pev->frame = 0;
}

void CWallHealth::Precache()
{
	PRECACHE_SOUND( "items/medshot4.wav" );
	PRECACHE_SOUND( "items/medshotno1.wav" );
	PRECACHE_SOUND( "items/medcharge4.wav" );
}

void CWallHealth::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{ 
	// Make sure that we have a caller
	if( !pActivator )
		return;
	// if it's not a player, ignore
	if( !pActivator->IsPlayer() )
		return;

	// if there is no juice left, turn it off
	if( m_iJuice <= 0 )
	{
		pev->frame = 1;			
		Off();
	}

	// if the player doesn't have the suit, or there is no juice left, make the deny noise
	if( ( m_iJuice <= 0 ) || ( !( pActivator->pev->weapons & ( 1 << WEAPON_SUIT ) ) ) || ( ( chargerfix.value ) && ( pActivator->pev->health >= pActivator->pev->max_health ) ) )
	{
		if( m_flSoundTime <= gpGlobals->time )
		{
			m_flSoundTime = gpGlobals->time + 0.62f;
			EMIT_SOUND( ENT( pev ), CHAN_ITEM, "items/medshotno1.wav", 1.0, ATTN_NORM );
		}
		return;
	}

	pev->nextthink = pev->ltime + 0.25f;
	SetThink( &CWallHealth::Off );

	// Time to recharge yet?
	if( m_flNextCharge >= gpGlobals->time )
		return;

	// Play the on sound or the looping charging sound
	if( !m_iOn )
	{
		m_iOn++;
		EMIT_SOUND( ENT( pev ), CHAN_ITEM, "items/medshot4.wav", 1.0, ATTN_NORM );
		m_flSoundTime = 0.56f + gpGlobals->time;
	}
	if( ( m_iOn == 1 ) && ( m_flSoundTime <= gpGlobals->time ) )
	{
		m_iOn++;
		EMIT_SOUND( ENT( pev ), CHAN_STATIC, "items/medcharge4.wav", 1.0, ATTN_NORM );
	}

	// charge the player
	if( pActivator->TakeHealth( 1, DMG_GENERIC ) )
	{
		m_iJuice--;
	}

	// govern the rate of charge
	m_flNextCharge = gpGlobals->time + 0.1f;
}

void CWallHealth::Recharge( void )
{
	EMIT_SOUND( ENT( pev ), CHAN_ITEM, "items/medshot4.wav", 1.0, ATTN_NORM );
	m_iJuice = (int)gSkillData.healthchargerCapacity;
	pev->frame = 0;			
	SetThink( &CBaseEntity::SUB_DoNothing );
}

void CWallHealth::Off( void )
{
	// Stop looping sound.
	if( m_iOn > 1 )
		STOP_SOUND( ENT( pev ), CHAN_STATIC, "items/medcharge4.wav" );

	m_iOn = 0;

	if( ( !m_iJuice ) && ( ( m_iReactivate = (int)g_pGameRules->FlHealthChargerRechargeTime() ) > 0 ) )
	{
		pev->nextthink = pev->ltime + m_iReactivate;
		SetThink( &CWallHealth::Recharge );
	}
	else
		SetThink( &CBaseEntity::SUB_DoNothing );
}

//
//		NEW MODEL WALL HEALTH AS SEEN IN PLAYSTATION(R)2 VERSION OF HALF-LIFE
//

//-------------------------------------------------------------
// Wall mounted health kit
//-------------------------------------------------------------

#define	seq_Still		0
#define	seq_Deploy		1
#define seq_RetractArm	2
#define seq_GiveShot	3
#define	seq_RetractShot 4
#define seq_PrepShot	5
#define seq_ShotIdle	6
#define seq_Inactive    7

#define seq_Slosh		1
#define seq_ToRest		2

#define CHARGER_AWAKE_DISTANCE	64

class CMdlWallHealthTank : public CActAnimating
{
public:
	void Spawn( );
	void Precache( void );
	void Update( int m_iJuice );
	void EXPORT TankThink( void );
	void StartUse( void );
	int		m_iJuice;
	static CMdlWallHealthTank *CreateFluidTank( edict_t *pOwner, const Vector &position );
};
LINK_ENTITY_TO_CLASS( item_healthchargertank, CMdlWallHealthTank );

CMdlWallHealthTank *CMdlWallHealthTank::CreateFluidTank( edict_t *pOwner, const Vector &position )
{
	CMdlWallHealthTank *pHealthTank = GetClassPtr( (CMdlWallHealthTank *)NULL );
	pHealthTank->pev->classname = MAKE_STRING( "item_healthchargertank" );
	pHealthTank->pev->origin = position;
	pHealthTank->pev->owner = pOwner;
    pHealthTank->Spawn();

	return pHealthTank;
}

void CMdlWallHealthTank::Precache()
{
	PRECACHE_MODEL("models/health_charger_both.mdl" );
}

void CMdlWallHealthTank::Spawn()
{
	Precache( );

	pev->solid		= SOLID_NOT;
	pev->movetype	= MOVETYPE_NONE;

	UTIL_SetSize(pev, pev->mins, pev->maxs);
	SET_MODEL(ENT(pev), "models/health_charger_both.mdl" );

	pev->rendermode = kRenderTransTexture;
	pev->renderamt = 192;
	SetBoneController( 0, 0 );	// -11 to 0 (empty to full)

	SetSequence( seq_Still );
	SetThink( TankThink );

	if (g_pGameRules->IsCoOp() )
	{
		CDecayRules *g_pDecayRules;
		g_pDecayRules = (CDecayRules*)g_pGameRules;

		if ( g_pDecayRules->m_bAlienMode == true )
		{
			SetThink( NULL );
			SetUse( NULL );
		}
	}

	pev->nextthink = gpGlobals->time + 0.1;
}

void CMdlWallHealthTank::Update(int m_iJuice)
{
	SetBoneController( 0, 	-(10 - (m_iJuice / gSkillData.healthchargerCapacity * 10)) );	// -11 to 0 (empty to full)
}

void CMdlWallHealthTank::StartUse()
{
	if (GetSequence() != seq_Slosh )
		SetSequence( seq_Slosh );
}

void CMdlWallHealthTank::TankThink()
{
	StudioFrameAdvance();
	pev->nextthink = gpGlobals->time + 0.1;

	switch( GetSequence() )
	{
	case seq_Still:	 // 0 - still
		break;
	case seq_Slosh:	 // 1 - slosh
		if ( m_fSequenceFinished )
			SetSequence( seq_Slosh );
		break;
	case seq_ToRest: // 2 - to rest
		if ( m_fSequenceFinished )
			SetSequence( seq_Still );
		break;
	default:
		break;
	}
}

#define HEALTHSTATION_FULL 0
#define HEALTHSTATION_EMPTY 1

class CMdlWallHealth : public CActAnimating
{
public:
	void Spawn( );
	void Precache( void );
	void EXPORT Off(void);
	void EXPORT Recharge(void);
	void EXPORT ChargerThink( void );
	void KeyValue( KeyValueData *pkvd );
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	virtual int	ObjectCaps( void ) { return (CActAnimating :: ObjectCaps() | FCAP_CONTINUOUS_USE) & ~FCAP_ACROSS_TRANSITION; }
	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	void UpdateArm();
	void UpdateFluidTank();
	void TurnOff();

	static	TYPEDESCRIPTION m_SaveData[];

	float m_flNextCharge, m_flStopCharge; 
	int		m_iReactivate ; // DeathMatch Delay until reactvated
	int		m_iJuice;
	int		m_iOn;			// 0 = off, 1 = startup, 2 = going
	float   m_flSoundTime;
	bool	m_bStartUsing;
	int		m_iRotValue;

	CBaseEntity *pEntity;
	CMdlWallHealthTank *pFluidTank;
};

TYPEDESCRIPTION CMdlWallHealth::m_SaveData[] =
{
	DEFINE_FIELD( CMdlWallHealth, m_flNextCharge, FIELD_TIME),
	DEFINE_FIELD( CMdlWallHealth, m_flStopCharge, FIELD_TIME),
	DEFINE_FIELD( CMdlWallHealth, m_iReactivate, FIELD_INTEGER),
	DEFINE_FIELD( CMdlWallHealth, m_iJuice, FIELD_INTEGER),
	DEFINE_FIELD( CMdlWallHealth, m_iOn, FIELD_INTEGER),
	DEFINE_FIELD( CMdlWallHealth, m_iRotValue, FIELD_INTEGER ),
	DEFINE_FIELD( CMdlWallHealth, m_flSoundTime, FIELD_TIME),
	DEFINE_FIELD( CMdlWallHealth, m_bStartUsing, FIELD_BOOLEAN),
	DEFINE_FIELD( CMdlWallHealth, pFluidTank, FIELD_CLASSPTR ),
	DEFINE_FIELD( CMdlWallHealth, pEntity, FIELD_CLASSPTR ),
};
//FIXED: caused bugs after load because of incorrectly specified derivied class
IMPLEMENT_SAVERESTORE( CMdlWallHealth, CActAnimating );

LINK_ENTITY_TO_CLASS(item_healthcharger, CMdlWallHealth);

void CMdlWallHealth::KeyValue( KeyValueData *pkvd )
{
	if (	FStrEq(pkvd->szKeyName, "style") ||
				FStrEq(pkvd->szKeyName, "height") ||
				FStrEq(pkvd->szKeyName, "value1") ||
				FStrEq(pkvd->szKeyName, "value2") ||
				FStrEq(pkvd->szKeyName, "value3"))
	{
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "dmdelay"))
	{
		m_iReactivate = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CActAnimating::KeyValue( pkvd );
}

void CMdlWallHealth::Spawn()
{
	Precache( );

	pev->solid		= SOLID_BBOX;
	pev->movetype	= MOVETYPE_NONE; //PUSH;
	//     UTIL_SetSize( pev, Vector(-7,-6,-27), Vector(7,6,27) ); //size(-20 -6 -27,20 6 27) 

	UTIL_SetOrigin(pev, pev->origin);		// set size and link into world
	UTIL_SetSize(pev, pev->mins, pev->maxs);
	SET_MODEL(ENT(pev), "models/health_charger_body.mdl" );
	m_iJuice = gSkillData.healthchargerCapacity;
	pev->frame = 0;			
	pev->skin = HEALTHSTATION_FULL;
	InitBoneControllers();

	pFluidTank = CMdlWallHealthTank::CreateFluidTank( edict(), pev->origin);
	//pFluidTank->pev->origin = pev->origin;
	pFluidTank->pev->angles = pev->angles;

	int YVal = pev->angles.y;
	switch (YVal)
	{
	case 180:
		m_iRotValue = 0;
		break;
	case 270:
		m_iRotValue = -90;
		break;
	case 0:
		m_iRotValue = -180;
		break;
	case 90:
		m_iRotValue = -270;
		break;
	}
	
	// controller is -90...90

	// -90...90 - ang 180 use 0
	// 0..180 -   ang 270 use -90
	// 90..270 -  ang 0 use -180
	// 180..360 - ang 90 use -270

	SetSequence( seq_Inactive );
	SetThink(ChargerThink);
	pev->nextthink = gpGlobals->time + 0.1;
}

void CMdlWallHealth::Precache()
{
	PRECACHE_SOUND("items/medshot4.wav");
	PRECACHE_SOUND("items/medshotno1.wav");
	PRECACHE_SOUND("items/medcharge4.wav");
	PRECACHE_MODEL("models/health_charger_body.mdl" );
	UTIL_PrecacheOther("item_healthchargertank");
}

void CMdlWallHealth::UpdateFluidTank( void )
{
	pFluidTank->Update( m_iJuice );
/*
	-11 - 0
	0	- 100

	m_iJuice	- X
	gSkillData.healthchargerCapacity - 100

	25 - 50 (?)
	50 - 100
	25:50*100

	m_iJuice div gSkillData.healthchargerCapacity * 100
	-(10 - (m_iJuice div gSkillData.healthchargerCapacity * 10))
*/
}

void CMdlWallHealth::ChargerThink( void )
{
	//ALERT( at_console, "WallHealth thinks!\n" );
    //ALERT( at_console, "next = %f  prev = %f\n", m_flNextCharge, m_flStopCharge );
	//ALERT( at_console, "seq %d, on %d, juice %d\n", GetSequence(), m_iOn, m_iJuice );

	StudioFrameAdvance();
	pev->nextthink = gpGlobals->time + 0.1;

	Vector posGun, angGun;
	Vector vecTarget, vecOut, angles;
	float flDist;

	if (m_bStartUsing == true)
		if (m_flStopCharge >= gpGlobals->time)
		{
			SetSequence( seq_RetractShot );
			pFluidTank->SetSequence( seq_ToRest );
			m_flStopCharge = 0;
		}

	switch( GetSequence() )
	{
	case seq_Inactive: // inactive
		{
			if (pev->skin == HEALTHSTATION_EMPTY) // we're in "off" state
			{
				SetThink( NULL );
				return;
			}

			// iterate on all entities in the vicinity.
			CBaseEntity *pTmpEntity = NULL;
			while ((pTmpEntity = UTIL_FindEntityInSphere( pTmpEntity, pev->origin, CHARGER_AWAKE_DISTANCE )) != NULL)
			{
				if (pTmpEntity->IsPlayer())
				{
					pEntity = pTmpEntity;
					if (( pev->origin - pEntity->pev->origin).Length() <= CHARGER_AWAKE_DISTANCE )
					{
						SetSequence( seq_Deploy );
						break;
					}
				}
			}
		}
		break;

	case seq_Deploy: // arm awakens
		UpdateArm();
		if ( m_fSequenceFinished ) 
			SetSequence( seq_PrepShot );
		break;

	case seq_PrepShot: // we are waiting for player to use charger
		flDist = (  pev->origin - pEntity->pev->origin).Length(); // CRASH IS HERE!!!!!!
		//ALERT( at_console, "dist = %f\n", flDist );

		if ( flDist > CHARGER_AWAKE_DISTANCE )
		{
			SetSequence( seq_RetractArm );
			break;
		}
		UpdateArm();
		// can be interruped from USE function which activates seq_GiveShot

		break;

	case seq_RetractArm:	
		if ( m_fSequenceFinished )
		{
			SetSequence( seq_Inactive );
			pEntity = NULL;
		} else
			UpdateArm(); // possible fix???
		break;

	case seq_RetractShot:	// stopped using
		m_bStartUsing = false;
		if ( m_fSequenceFinished ) 
			SetSequence( seq_PrepShot );
		break;

	case seq_GiveShot:	// started using
		m_bStartUsing = false;
		if ( m_fSequenceFinished )
		{
			m_bStartUsing = true;
			SetSequence( seq_ShotIdle );
		}
		break;

	case seq_ShotIdle:	// in use
		// give health here while USE is pressed
		break;

	default:
		break;
	}

}

void CMdlWallHealth::UpdateArm()
{
	Vector vecTarget = (pev->origin - pEntity->pev->origin).Normalize( );
	Vector angles = UTIL_VecToAngles (vecTarget);
	//ALERT( at_console, "angles = %f, %f, %f - %f\n", angles.x, angles.y, angles.z, angles.y / 180 );
	if (angles.y > 180)
		angles.y = angles.y - 360;
	if (angles.y < -180)
		angles.y = angles.y + 360;

	// 180 - 100
	//  90 - X   (50)
	// 50 = 90 * 100 / 180
	// percent = desired degree * 100 / 180
	// percent = desired degree / 180

	// controller is -90...90
	// -90...90 - 180 use 0
	// 0..180 - 270 use -90
	// 90..270 - 0 use -180
	// 180..360(0) - 90 use 

	// pos = (end - start) * scalar + start
	// pos = 180 * scalar - 90
	SetBoneController(0, angles.y + m_iRotValue); //- 90); // +25 to the right
}

void CMdlWallHealth::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{ 
	// Make sure that we have a caller
	if (!pActivator)
		return;

	// if it's not a player, ignore
	if ( !pActivator->IsPlayer() )
		return;

	if (pev->skin == HEALTHSTATION_EMPTY) // already off
		return;

	// if there is no juice left, turn it off
	if (m_iJuice <= 0)
	{
		TurnOff();			// do we need it there or in Off() ?
		return;
	}

	// if the player doesn't have the suit, or there is no juice left, make the deny noise
	if ((m_iJuice <= 0) || (!(pActivator->pev->weapons & (1<<WEAPON_SUIT))))
	{
		if (m_flSoundTime <= gpGlobals->time)
		{
			m_flSoundTime = gpGlobals->time + 0.62;
			EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/medshotno1.wav", 1.0, ATTN_NORM );
		}
		return;
	}

	pFluidTank->StartUse();
	pev->nextthink = gpGlobals->time + 0.25; // pev->ltime
	SetThink(Off);

	// start the give shot sequence and do not use until it's finished
	if (GetSequence() != seq_GiveShot)
		if ( m_bStartUsing == false )
		{
			SetSequence( seq_GiveShot );
			return;
		}

	// Time to recharge yet?
	if (m_flNextCharge >= gpGlobals->time)
		return;

	// Play the on sound or the looping charging sound
	if (!m_iOn)
	{
		m_iOn++;
		EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/medshot4.wav", 1.0, ATTN_NORM );
		m_flSoundTime = 0.56 + gpGlobals->time;
	}
	if ((m_iOn == 1) && (m_flSoundTime <= gpGlobals->time))
	{
		m_iOn++;
		EMIT_SOUND(ENT(pev), CHAN_STATIC, "items/medcharge4.wav", 1.0, ATTN_NORM );
	}

	// charge the player
	if ( pActivator->TakeHealth( 1, DMG_GENERIC ) )
	{
		m_iJuice--;
		//pFluidTank->StartUse();
	}

	UpdateFluidTank();

	// govern the rate of charge
	m_flNextCharge = gpGlobals->time + 0.1;
}

void CMdlWallHealth::Recharge(void)
{
	//if (pev->skin == HEALTHSTATION_EMPTY)
	//	return;

	EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/medshot4.wav", 1.0, ATTN_NORM );
	m_iJuice = gSkillData.healthchargerCapacity;
	pev->skin = HEALTHSTATION_FULL;	// set the active skin			
	SetThink( ChargerThink );
}

void CMdlWallHealth::Off(void)
{
	m_flStopCharge = gpGlobals->time + 0.25;
	SetThink( ChargerThink );
	pev->nextthink = gpGlobals->time + 0.01;

	// Stop looping sound.
	if (m_iOn > 1)
		STOP_SOUND( ENT(pev), CHAN_STATIC, "items/medcharge4.wav" );

	m_iOn = 0;

	if ((!m_iJuice) &&  ( ( m_iReactivate = g_pGameRules->FlHealthChargerRechargeTime() ) > 0) )
	{
		pev->nextthink = pev->ltime + m_iReactivate;
		SetThink(Recharge);
	}
	//else
	//	SetThink( SUB_DoNothing );
}

void CMdlWallHealth::TurnOff( void )
{
	pev->skin = HEALTHSTATION_EMPTY;
	pFluidTank->SetSequence( seq_ToRest );
	SetUse( NULL );
	SetSequence( seq_RetractArm );
}
