/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   This source code contains proprietary and confidential information of
*   Valve LLC and its suppliers.  Access to this code is restricted to
*   persons who have executed a written SDK license with Valve.  Any access,
*   use or distribution of this code by or to any unlicensed person is illegal.
*
****/
//=========================================================
// Generic Monster - purely for scripted sequence work.
//=========================================================
#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"schedule.h"
#include	"animation.h"
#include	"weapons.h"
// For holograms, make them not solid so the player can walk through them
#define	SF_GENERICMONSTER_NOTSOLID					4 

//=========================================================
// Monster's Anim Events Go Here
//=========================================================

class Cjojo : public CBaseMonster
{
public:
	void RunAI( void );
	void Spawn( void );
	void Precache( void );
	void SetYawSpeed( void );
	void SetActivity ( Activity NewActivity );
	int  Classify ( void );
	void HandleAnimEvent( MonsterEvent_t *pEvent );
	int ISoundMask ( void );

	int m_iTrail;

	BOOL CheckRangeAttack1 ( float flDot, float flDist ) { return FALSE; }
	BOOL CheckRangeAttack2 ( float flDot, float flDist ) { return FALSE; }
};
LINK_ENTITY_TO_CLASS( monster_jojo, Cjojo );
LINK_ENTITY_TO_CLASS( monster_jojo_scientist, Cjojo );
LINK_ENTITY_TO_CLASS( monster_jojo_hgrunt, Cjojo );
LINK_ENTITY_TO_CLASS( monster_jojo_barney, Cjojo );
LINK_ENTITY_TO_CLASS( monster_jojo_gordon, Cjojo );
//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int	Cjojo :: Classify ( void )
{
	return	CLASS_PLAYER;
}

void Cjojo :: RunAI( void )
{
	CBaseMonster :: RunAI();
	if(pev->sequence == 2 && FClassnameIs( pev, "monster_jojo_gordon" ) ){
			MESSAGE_BEGIN( MSG_PAS, SVC_TEMPENTITY, pev->origin );
				WRITE_BYTE(  TE_IMPLOSION);
				WRITE_COORD( pev->origin.x );
				WRITE_COORD( pev->origin.y );
				WRITE_COORD( pev->origin.z + 66 );
				WRITE_BYTE( 90 );  // radius
				WRITE_BYTE( 30 ); // count
				WRITE_BYTE( 1 ); // life
			MESSAGE_END();

			pev->effects |= EF_MUZZLEFLASH;
	}

}

void Cjojo :: SetActivity ( Activity NewActivity )
{
	if(pev->weapons >= 1){
		int	iSequence = ACTIVITY_NOT_AVAILABLE;
		void *pmodel = GET_MODEL_PTR( ENT(pev) );
		
		iSequence = LookupActivity ( NewActivity );

		m_Activity = NewActivity; // Go ahead and set this so it doesn't keep trying when the anim is not present
		m_IdealActivity = m_Activity;
		// Set to the desired anim, or default anim if the desired is not present
		if ( iSequence > ACTIVITY_NOT_AVAILABLE )
		{
			if ( pev->sequence != iSequence || !m_fSequenceLoops )
			{
				pev->frame = 0;
			}

			pev->sequence		= iSequence;	// Set to the reset anim (if it's there)
			ResetSequenceInfo( );
			SetYawSpeed();
		}
	}
}


//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void Cjojo :: SetYawSpeed ( void )
{
	pev->yaw_speed = 90;
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void Cjojo :: HandleAnimEvent( MonsterEvent_t *pEvent )
{
	switch( pEvent->event )
	{
	case 1:
		{
		SetBodygroup( 3, 1);
		}
		break;

	case 2:
		{
		SetBodygroup( 4, 1);
		}
		break;

	case 3:
		{
		MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
		WRITE_BYTE( TE_BEAMFOLLOW );
		WRITE_SHORT( entindex() + 0x1000 * 3 );		// entity, attachment
		WRITE_SHORT(m_iTrail );	// model
		WRITE_BYTE( 3 ); // life
		WRITE_BYTE( 1 );  // width
		WRITE_BYTE( 255 );	// R
		WRITE_BYTE( 32 );	// G
		WRITE_BYTE( 32 );	// B
		WRITE_BYTE( 128 );	// brightness
	    MESSAGE_END();  // move PHS/PVS data sending into here (SEND_ALL, SEND_PVS, SEND_PHS)
		}
		break;

	case 4:
		{
		SetBodygroup( 2, 2);
		SetBodygroup( 3, 0);
		}
		break;
	
	case 5:
		{
		SetBodygroup( 2, 3);
		}
		break;

	case 6:
		{
		Vector vecArmPos,vecArmDir;
		GetAttachment( 0, vecArmPos, vecArmDir );
		UTIL_Sparks( vecArmPos );
		}
		break;

	case 7:
		{
		Vector vecArmPos,vecArmDir;
		GetBonePosition( 0, vecArmPos, vecArmDir );

		pev->effects |= EF_NODRAW;
		int teleb = PRECACHE_MODEL("sprites/b-tele1.spr");
		MESSAGE_BEGIN(MSG_BROADCAST, SVC_TEMPENTITY,vecArmPos);
		WRITE_BYTE(3);
		WRITE_COORD( vecArmPos.x );
		WRITE_COORD( vecArmPos.y );
		WRITE_COORD( vecArmPos.z);
		WRITE_SHORT(teleb);
		WRITE_BYTE(15);
		WRITE_BYTE(15);
		WRITE_BYTE(4);
		MESSAGE_END();
		}
		break;

	case 10:
		{ 
		pev->effects |= EF_MUZZLEFLASH;
		}
		break;

	default:
		CBaseMonster::HandleAnimEvent( pEvent );
		break;
	}
}

//=========================================================
// ISoundMask - generic monster can't hear.
//=========================================================
int Cjojo :: ISoundMask ( void )
{
	return	NULL;
}

//=========================================================
// Spawn
//=========================================================
void Cjojo :: Spawn()
{
	Precache();

	if ( FClassnameIs( pev, "monster_jojo_scientist" ) ){
	SET_MODEL(ENT(pev), "models/scientist_jojo.mdl");
	}
	else if ( FClassnameIs( pev, "monster_jojo_gordon" ) ){
	SET_MODEL(ENT(pev), "models/gordon_jojo.mdl");
	}
	else if ( FClassnameIs( pev, "monster_jojo_hgrunt" ) ){
	SET_MODEL(ENT(pev), "models/hgrunt_jojo.mdl");
	}
	else if ( FClassnameIs( pev, "monster_jojo_barney" ) ){
	SET_MODEL(ENT(pev), "models/barney_jojo.mdl");
	}

	UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	m_bloodColor		= BLOOD_COLOR_RED;
	pev->health			= 80;
	m_flFieldOfView		= 0.5;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState		= MONSTERSTATE_NONE;

	MonsterInit();
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void Cjojo :: Precache()
{
	PRECACHE_MODEL("models/barney_jojo.mdl");
	PRECACHE_MODEL("models/gordon_jojo.mdl");
	PRECACHE_MODEL("models/hgrunt_jojo.mdl");
	PRECACHE_MODEL("models/scientist_jojo.mdl");
	PRECACHE_MODEL ( "sprites/flare6.spr" );
	PRECACHE_MODEL("sprites/b-tele1.spr");
	m_iTrail = PRECACHE_MODEL("sprites/smoke.spr");
}	

//=========================================================
// AI Schedules Specific to this monster
//=========================================================
