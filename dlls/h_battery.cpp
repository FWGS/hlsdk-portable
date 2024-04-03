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
/*

===== h_battery.cpp ========================================================

  battery-related code

*/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "saverestore.h"
#include "skill.h"
#include "gamerules.h"
#include "weapons.h"
#include "game.h"
#include "actanimating.h"
#include "effects.h"

#define CHARGER_ACTIVE 0
#define CHARGER_EMPTY 1

class CRecharge : public CBaseToggle
{
public:
	void Spawn();
	void Precache( void );
	void EXPORT Off(void);
	void EXPORT Recharge(void);
	void KeyValue( KeyValueData *pkvd );
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	virtual int ObjectCaps( void ) { return ( CBaseToggle::ObjectCaps() | FCAP_CONTINUOUS_USE ) & ~FCAP_ACROSS_TRANSITION; }
	virtual int Save( CSave &save );
	virtual int Restore( CRestore &restore );

	static TYPEDESCRIPTION m_SaveData[];

	float m_flNextCharge; 
	int m_iReactivate; // DeathMatch Delay until reactvated
	int m_iJuice;
	int m_iOn;			// 0 = off, 1 = startup, 2 = going
	float m_flSoundTime;
};

TYPEDESCRIPTION CRecharge::m_SaveData[] =
{
	DEFINE_FIELD( CRecharge, m_flNextCharge, FIELD_TIME ),
	DEFINE_FIELD( CRecharge, m_iReactivate, FIELD_INTEGER ),
	DEFINE_FIELD( CRecharge, m_iJuice, FIELD_INTEGER ),
	DEFINE_FIELD( CRecharge, m_iOn, FIELD_INTEGER ),
	DEFINE_FIELD( CRecharge, m_flSoundTime, FIELD_TIME ),
};

IMPLEMENT_SAVERESTORE( CRecharge, CBaseToggle )

LINK_ENTITY_TO_CLASS( func_recharge, CRecharge )

void CRecharge::KeyValue( KeyValueData *pkvd )
{
	if( FStrEq( pkvd->szKeyName, "style" ) ||
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

void CRecharge::Spawn()
{
	Precache();

	pev->solid = SOLID_BSP;
	pev->movetype = MOVETYPE_PUSH;
	pev->skin		= CHARGER_ACTIVE;

	UTIL_SetOrigin( pev, pev->origin );		// set size and link into world
	UTIL_SetSize( pev, pev->mins, pev->maxs );
	SET_MODEL( ENT( pev ), STRING( pev->model ) );
	m_iJuice = (int)gSkillData.suitchargerCapacity;
	pev->frame = 0;			
}

void CRecharge::Precache()
{
	PRECACHE_SOUND( "items/suitcharge1.wav" );
	PRECACHE_SOUND( "items/suitchargeno1.wav" );
	PRECACHE_SOUND( "items/suitchargeok1.wav" );
}

void CRecharge::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
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
	if( ( m_iJuice <= 0 ) || ( !( pActivator->pev->weapons & ( 1 << WEAPON_SUIT ) ) ) || ( ( chargerfix.value ) && ( pActivator->pev->armorvalue == MAX_NORMAL_BATTERY ) ) )
	{
		if( m_flSoundTime <= gpGlobals->time )
		{
			m_flSoundTime = gpGlobals->time + 0.62f;
			EMIT_SOUND( ENT( pev ), CHAN_ITEM, "items/suitchargeno1.wav", 0.85, ATTN_NORM );
		}
		return;
	}

	pev->nextthink = pev->ltime + 0.25f;
	SetThink( &CRecharge::Off );

	// Time to recharge yet?
	if( m_flNextCharge >= gpGlobals->time )
		return;

	m_hActivator = pActivator;

	// Play the on sound or the looping charging sound
	if( !m_iOn )
	{
		m_iOn++;
		EMIT_SOUND( ENT( pev ), CHAN_ITEM, "items/suitchargeok1.wav", 0.85, ATTN_NORM );
		m_flSoundTime = 0.56f + gpGlobals->time;
	}

	if( ( m_iOn == 1 ) && ( m_flSoundTime <= gpGlobals->time ) )
	{
		m_iOn++;
		EMIT_SOUND( ENT( pev ), CHAN_STATIC, "items/suitcharge1.wav", 0.85, ATTN_NORM );
	}

	// charge the player
	if( m_hActivator->pev->armorvalue < 100 )
	{
		m_iJuice--;
		m_hActivator->pev->armorvalue += 1;

		if( m_hActivator->pev->armorvalue > 100 )
			m_hActivator->pev->armorvalue = 100;
	}

	// govern the rate of charge
	m_flNextCharge = gpGlobals->time + 0.1f;
}

void CRecharge::Recharge( void )
{
	m_iJuice = (int)gSkillData.suitchargerCapacity;
	pev->frame = 0;	
	SetThink( &CBaseEntity::SUB_DoNothing );
}

void CRecharge::Off( void )
{
	// Stop looping sound.
	if( m_iOn > 1 )
		STOP_SOUND( ENT( pev ), CHAN_STATIC, "items/suitcharge1.wav" );

	m_iOn = 0;

	if( ( !m_iJuice ) &&  ( ( m_iReactivate = (int)g_pGameRules->FlHEVChargerRechargeTime() ) > 0 ) )
	{
		pev->nextthink = pev->ltime + m_iReactivate;
		SetThink( &CRecharge::Recharge );
	}
	else
		SetThink( &CBaseEntity::SUB_DoNothing );
}

//
//		NEW MODEL WALL HEV CHARGER AS SEEN IN PLAYSTATION(R)2 VERSION OF HALF-LIFE
//

//-------------------------------------------------------------
// Wall mounted HEV charger
//-------------------------------------------------------------

#define	seqCharge_Still		0
#define	seqCharge_Deploy		1
#define seqCharge_RetractArm	2
#define seqCharge_GiveShot	3
#define	seqCharge_RetractShot 4
#define seqCharge_PrepShot	5
#define seqCharge_ShotIdle	6
#define seqCharge_Inactive    0

#define CHARGER_AWAKE_DISTANCE	64

class CMdlChargerGlass : public CActAnimating
{
public:
	void Spawn( );
	void Precache( void );
	static CMdlChargerGlass *CreateChargerGlass( edict_t *pOwner, const Vector &position );
};
LINK_ENTITY_TO_CLASS( item_rechargeglass, CMdlChargerGlass );

CMdlChargerGlass *CMdlChargerGlass::CreateChargerGlass( edict_t *pOwner, const Vector &position )
{
	CMdlChargerGlass *pGlass = GetClassPtr( (CMdlChargerGlass *)NULL );
	pGlass->pev->classname = MAKE_STRING( "item_rechargeglass" );
	pGlass->pev->origin = position;
	pGlass->pev->owner = pOwner;
    pGlass->Spawn();

	return pGlass;
}

void CMdlChargerGlass::Precache()
{
	PRECACHE_MODEL("models/hev_glass.mdl" );
}

void CMdlChargerGlass::Spawn()
{
	Precache( );

	pev->solid		= SOLID_NOT;
	pev->movetype	= MOVETYPE_NONE;

	UTIL_SetSize(pev, pev->mins, pev->maxs);
	SET_MODEL(ENT(pev), "models/hev_glass.mdl" );

	pev->rendermode = kRenderTransTexture;
	pev->renderamt = 192;
}

class CMdlCharger : public CActAnimating
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
	float   m_flDegree;
	bool	m_bStartUsing;
	int		m_iRotValue;

	CBaseEntity *pEntity;
	CMdlChargerGlass *pGlass;
	CBeam		*m_pBeam;
};

TYPEDESCRIPTION CMdlCharger::m_SaveData[] =
{
	DEFINE_FIELD( CMdlCharger, m_flNextCharge, FIELD_TIME),
	DEFINE_FIELD( CMdlCharger, m_flStopCharge, FIELD_TIME),
	DEFINE_FIELD( CMdlCharger, m_iReactivate, FIELD_INTEGER),
	DEFINE_FIELD( CMdlCharger, m_iJuice, FIELD_INTEGER),
	DEFINE_FIELD( CMdlCharger, m_iOn, FIELD_INTEGER),
	DEFINE_FIELD( CMdlCharger, m_iRotValue, FIELD_INTEGER ),
	DEFINE_FIELD( CMdlCharger, m_flSoundTime, FIELD_TIME),
	DEFINE_FIELD( CMdlCharger, m_flDegree, FIELD_TIME),
	DEFINE_FIELD( CMdlCharger, m_bStartUsing, FIELD_BOOLEAN),
	DEFINE_FIELD( CMdlCharger, pGlass, FIELD_CLASSPTR ),
	DEFINE_FIELD( CMdlCharger, pEntity, FIELD_CLASSPTR ),
};
//FIXED: caused bugs after load because of incorrectly specified derivied class
IMPLEMENT_SAVERESTORE( CMdlCharger, CActAnimating );

LINK_ENTITY_TO_CLASS(item_recharge, CMdlCharger);

void CMdlCharger::KeyValue( KeyValueData *pkvd )
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

void CMdlCharger::Spawn()
{
	Precache( );

	pev->solid		= SOLID_BBOX; //BBOX;
	pev->movetype	= MOVETYPE_NONE; //NONE; //PUSH;
	//     UTIL_SetSize( pev, Vector(-7,-6,-27), Vector(7,6,27) ); //size(-20 -6 -27,20 6 27) 

	UTIL_SetOrigin(pev, pev->origin);		// set size and link into world
	UTIL_SetSize(pev, pev->mins, pev->maxs);
	SET_MODEL(ENT(pev), "models/hev.mdl" );
	m_iJuice = gSkillData.suitchargerCapacity;
	pev->frame = 0;			
	InitBoneControllers();

	m_pBeam = CBeam::BeamCreate("sprites/lgtning.spr", 200);
	m_pBeam->EntsInit( entindex( ), entindex( ) );
	m_pBeam->SetStartAttachment( 3 );	// 2
	m_pBeam->SetEndAttachment( 4 );		// 3
	m_pBeam->SetColor( 0, 255, 0 );
	m_pBeam->SetNoise( 10 );
	m_pBeam->SetBrightness( 150 );
	m_pBeam->SetWidth( 5 );
	m_pBeam->SetScrollRate( 35 );

	pGlass = CMdlChargerGlass::CreateChargerGlass(edict(), pev->origin);
	pGlass->pev->angles = pev->angles;

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

	SetSequence( seqCharge_Inactive );

	SetThink(&CMdlCharger::ChargerThink);

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

void CMdlCharger::Precache()
{
	PRECACHE_SOUND("items/suitcharge1.wav");
	PRECACHE_SOUND("items/suitchargeno1.wav");
	PRECACHE_SOUND("items/suitchargeok1.wav");
	PRECACHE_MODEL("models/hev.mdl");
	UTIL_PrecacheOther("item_rechargeglass");
}

void CMdlCharger::UpdateFluidTank( void )
{
	// controllers 1 and 2 - 0..360
	m_flDegree += 7;
	if (m_flDegree > 360 )
		m_flDegree = m_flDegree - 360;

	SetBoneController( 1, m_flDegree );
	SetBoneController( 2, -m_flDegree );
}

void CMdlCharger::ChargerThink( void )
{
	//ALERT( at_console, "seq %d, on %d, juice %d\n", GetSequence(), m_iOn, m_iJuice );

	StudioFrameAdvance();
	pev->nextthink = gpGlobals->time + 0.1;

	float flDist;

	if (m_bStartUsing == true)
		if (m_flStopCharge >= gpGlobals->time)
		{
			SetSequence( seqCharge_RetractShot );
			//pFluidTank->SetSequence( seqCharge_ToRest );
			m_flStopCharge = 0;
		}

	switch( GetSequence() )
	{
		case seqCharge_Inactive: // inactive
		{
			if (pev->skin == CHARGER_EMPTY) // we're in "off" state
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
						SetSequence( seqCharge_Deploy );
						break;
					}
				}
			}
		}
		break;

	case seqCharge_Deploy: // arm awakens
		UpdateArm();
		if ( m_fSequenceFinished ) 
			SetSequence( seqCharge_PrepShot );
		break;

	case seqCharge_PrepShot: // we are waiting for player to use charger
		flDist = (  pev->origin - pEntity->pev->origin).Length();
		//ALERT( at_console, "dist = %f\n", flDist );

		if ( flDist > CHARGER_AWAKE_DISTANCE )
		{
			SetSequence( seqCharge_RetractArm );
			break;
		}
		UpdateArm();
		// can be interruped from USE function which activates seqCharge_GiveShot

		break;

	case seqCharge_RetractArm:
		if ( m_fSequenceFinished )
		{
			SetSequence( seqCharge_Inactive );
			pEntity = NULL;
		} else
			UpdateArm(); // possible fix???
		break;

	case seqCharge_RetractShot:	// stopped using
		m_bStartUsing = false;
		if ( m_fSequenceFinished ) 
			SetSequence( seqCharge_PrepShot );
		break;

	case seqCharge_GiveShot:	// started using
		m_bStartUsing = false;
		if ( m_fSequenceFinished )
		{
			m_bStartUsing = true;
			SetSequence( seqCharge_ShotIdle );
		}
		break;

	case seqCharge_ShotIdle:	// in use
		// give health here while USE is pressed
		break;

	default:
		break;
	}

}

void CMdlCharger::UpdateArm()
{
	if ( FNullEnt( pEntity->pev ) )
		return;

	Vector vecTarget = (pev->origin - pEntity->pev->origin).Normalize( );	// TODO: CRASH HERE, because pEntity is NULL (0x00)
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
	SetBoneController(3, angles.y + m_iRotValue); //- 90); // +25 to the right
}

void CMdlCharger::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{ 
	// Make sure that we have a caller
	if (!pActivator)
		return;

	// if it's not a player, ignore
	if ( !pActivator->IsPlayer() )
		return;

	if (pev->skin == CHARGER_EMPTY) // already off
		return;

	// if there is no juice left, turn it off
	if (m_iJuice <= 0)
	{
		TurnOff();
		return;
	}

	// if the player doesn't have the suit, or there is no juice left, make the deny noise
	if ((m_iJuice <= 0) || (!(pActivator->pev->weapons & (1<<WEAPON_SUIT))))
	{
		if (m_flSoundTime <= gpGlobals->time)
		{
			m_flSoundTime = gpGlobals->time + 0.62;
			EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/suitchargeno1.wav", 0.85, ATTN_NORM );
		}
		return;
	}

	//pFluidTank->StartUse();
	pev->nextthink = gpGlobals->time + 0.25; // pev->ltime
	SetThink(&CMdlCharger::Off);

	// start the give shot sequence and do not use until it's finished
	if (GetSequence() != seqCharge_GiveShot)
		if ( m_bStartUsing == false )
		{
			//if ( GetSequence() != seqCharge_PrepShot )
			//	return; // can't start using until arm fully deployed from charger

			SetSequence( seqCharge_GiveShot );
			return;
		}

	// Time to recharge yet?
	if (m_flNextCharge >= gpGlobals->time)
		return;

	// Play the on sound or the looping charging sound
	if (!m_iOn)
	{
		m_iOn++;
		EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/suitchargeno1.wav", 0.85, ATTN_NORM );
		m_flSoundTime = 0.56 + gpGlobals->time;
	}
	if ((m_iOn == 1) && (m_flSoundTime <= gpGlobals->time))
	{
		m_iOn++;
		EMIT_SOUND(ENT(pev), CHAN_STATIC, "items/suitcharge1.wav", 0.85, ATTN_NORM );
	}

	// charge the player
	if (pActivator->pev->armorvalue < 100)
	{
		m_iJuice--;
		pActivator->pev->armorvalue += 1;
		//pFluidTank->StartUse();

		if (pActivator->pev->armorvalue > 100)
			pActivator->pev->armorvalue = 100;
	}

	UpdateFluidTank();

	// govern the rate of charge
	m_flNextCharge = gpGlobals->time + 0.1;
}

void CMdlCharger::Recharge(void)
{
	//if (pev->skin == CHARGER_EMPTY)
	//	return;

	//EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/medshot4.wav", 1.0, ATTN_NORM );
	m_iJuice = gSkillData.suitchargerCapacity;
	pev->skin = CHARGER_ACTIVE;	// set the active skin			
	SetThink( &CMdlCharger::ChargerThink );
}

void CMdlCharger::Off(void)
{
	m_flStopCharge = gpGlobals->time + 0.25;
	SetThink( &CMdlCharger::ChargerThink );
	pev->nextthink = gpGlobals->time + 0.01;

	// Stop looping sound.
	if (m_iOn > 1)
		STOP_SOUND( ENT(pev), CHAN_STATIC, "items/suitcharge1.wav" );

	m_iOn = 0;

	if ((!m_iJuice) &&  ( ( m_iReactivate = g_pGameRules->FlHEVChargerRechargeTime() ) > 0) )
	{
		pev->nextthink = pev->ltime + m_iReactivate;
		SetThink( &CMdlCharger::Recharge );
	}
	//else
	//	SetThink( &CMdlCharger::SUB_DoNothing );

	SetSequence( seqCharge_RetractShot );
}

void CMdlCharger::TurnOff(void)
{
	pev->skin = CHARGER_EMPTY;
	UTIL_Remove( m_pBeam );
	m_pBeam = NULL;
	SetUse( NULL );
	SetSequence( seqCharge_RetractArm );
}
