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
#include	"soundent.h"
// For holograms, make them not solid so the player can walk through them
#define	SF_GENERICMONSTER_NOTSOLID					4 

//=========================================================
// Monster's Anim Events Go Here
//=========================================================

class CBiohomoVan : public CBaseMonster
{
public:
	void RunAI( void );
	void Spawn( void );
	void Precache( void );
	void SetYawSpeed( void );
	void SetActivity ( Activity NewActivity );
	int  Classify ( void );

	void BarneyFirePistol( void );

	void HandleAnimEvent( MonsterEvent_t *pEvent );
	int ISoundMask ( void );

	int m_iTrail;

	BOOL CheckRangeAttack1 ( float flDot, float flDist );
	BOOL CheckRangeAttack2 ( float flDot, float flDist ) { return FALSE; }
};
LINK_ENTITY_TO_CLASS( biohomo_markwolf, CBiohomoVan );
LINK_ENTITY_TO_CLASS( biohomo_markwolf2, CBiohomoVan );
LINK_ENTITY_TO_CLASS( biohomo_van, CBiohomoVan );
LINK_ENTITY_TO_CLASS( biohomo_van2, CBiohomoVan );
LINK_ENTITY_TO_CLASS( biohomo_takeboy, CBiohomoVan );
//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int	CBiohomoVan :: Classify ( void )
{
	return	CLASS_PLAYER_ALLY;
}

void CBiohomoVan :: RunAI( void )
{
	CBaseMonster :: RunAI();
	if(pev->sequence == LookupActivity ( ACT_WALK )){
	m_flGroundSpeed = 45;
	}

}


//=========================================================
// CheckRangeAttack1
//=========================================================
BOOL CBiohomoVan :: CheckRangeAttack1 ( float flDot, float flDist )
{
	if ( pev->weapons == 1 ){
		return FALSE;
	}

	if (flDot >= 0.5 )
	{
		return TRUE;
	}

	return FALSE;
}

void CBiohomoVan :: SetActivity ( Activity NewActivity )
{
	int	iSequence = ACTIVITY_NOT_AVAILABLE;
	void *pmodel = GET_MODEL_PTR( ENT(pev) );

	switch ( NewActivity)
	{
	case ACT_RUN:
		// grunt is either shooting standing or shooting crouched
		iSequence = LookupActivity ( NewActivity );
		break;
	case ACT_WALK:
		// grunt is either shooting standing or shooting crouched
		iSequence = LookupActivity ( NewActivity );
		break;
	case ACT_IDLE:
		// grunt is either shooting standing or shooting crouched
		iSequence = LookupActivity ( NewActivity );
		break;
	default:
		iSequence = LookupActivity ( NewActivity );
		break;
	}
	
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
	else
	{
		// Not available try to get default anim
		ALERT ( at_console, "%s has no sequence for act:%d\n", STRING(pev->classname), NewActivity );
	//	pev->sequence		= 0;	// Set to the reset anim (if it's there)
	}
}


//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CBiohomoVan :: SetYawSpeed ( void )
{
	if(m_MonsterState == MONSTERSTATE_COMBAT){
	pev->yaw_speed = 180;
	}
	else{
	pev->yaw_speed = 120;
	}
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CBiohomoVan :: HandleAnimEvent( MonsterEvent_t *pEvent )
{
	switch( pEvent->event )
	{
	case 1:
		{
			UTIL_Sparks( pev->origin + Vector(0,0,60) );
			EMIT_SOUND(ENT(pev), CHAN_WEAPON, "newadd/struggle_hit.wav", 1, ATTN_NORM);
		}
		break;

	case 2:
		{
			pev->body = 1;
		}
		break;

	case 3:
		{
			UTIL_Ricochet( pev->origin + Vector(0,0,48),1 );
		}
		break;

	case 4:
		{
			StopAnimation();
		}
		break;
	case 5:
		{
			EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "!VAN_14", 1, ATTN_NORM, 0, 100 );
		}
		break;

	case 6:
		{
			FireTargets( "door1", this, this, USE_TOGGLE, 0 );
		}
		break;

	case 7:
		{
			BarneyFirePistol();
		}
		break;

	case 8:
		m_cAmmoLoaded = m_cClipSize;
		break;

	case 9:
		pev->movetype = MOVETYPE_FLY;
		pev->velocity.x = -40;
		break;

	case 10:
		pev->velocity.x = 0;
		pev->movetype = MOVETYPE_STEP;
		break;

	default:
		CBaseMonster::HandleAnimEvent( pEvent );
		break;
	}
}

void CBiohomoVan :: BarneyFirePistol ( void )
{
	if ( m_cAmmoLoaded <= 0){
	return;
	}

	Vector vecShootOrigin;

	UTIL_MakeVectors(pev->angles);

	vecShootOrigin = pev->origin + Vector( 0, 0, 60 );
	
	Vector vecShootDir = ShootAtEnemy( vecShootOrigin );

	pev->effects = EF_MUZZLEFLASH;

	int pitchShift = RANDOM_LONG( 0, 20 );
	
	// Only shift about half the time
	if ( pitchShift > 10 )
		pitchShift = 0;
	else
		pitchShift -= 5;
	

	int iBulletType;
	iBulletType = BULLET_30mm;

	FireBullets(1, vecShootOrigin, vecShootDir, Vector(0.01,0.01,0.01), 2048, iBulletType,1);

	EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, "weapons/de_shot1.wav", 1, 0.7, 0, 100 + pitchShift );

	CSoundEnt::InsertSound ( bits_SOUND_COMBAT, pev->origin, 384, 0.3 );

	m_cAmmoLoaded--;
}

//=========================================================
// ISoundMask - generic monster can't hear.
//=========================================================
int CBiohomoVan :: ISoundMask ( void )
{
	return	NULL;
}

//=========================================================
// Spawn
//=========================================================
void CBiohomoVan :: Spawn()
{
	Precache();

	if ( FClassnameIs ( pev, "biohomo_van" ) || FClassnameIs ( pev, "biohomo_van2" ) ){
	SET_MODEL(ENT(pev), "models/biohomo_vanlve.mdl");
	}
	else if ( FClassnameIs ( pev, "biohomo_markwolf" ) || FClassnameIs ( pev, "biohomo_markwolf2" ) ){
	SET_MODEL(ENT(pev), "models/biohomo_markwolf.mdl");
	}
	else if ( FClassnameIs ( pev, "biohomo_takeboy" ) ){
	SET_MODEL(ENT(pev), "models/biohomo_takeboy.mdl");
	}

	UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	m_bloodColor		= BLOOD_COLOR_RED;
	pev->health			= 100;
	m_flFieldOfView		= 0.5;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState		= MONSTERSTATE_NONE;

	m_cClipSize			= 7;
	m_cAmmoLoaded		= m_cClipSize;

	MonsterInit();

	m_aimenemy_mod = 1;
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CBiohomoVan :: Precache()
{
	PRECACHE_MODEL("models/biohomo_vanlve.mdl");
	PRECACHE_MODEL("models/biohomo_takeboy.mdl");
	PRECACHE_MODEL("models/biohomo_markwolf.mdl");
	m_iTrail = PRECACHE_MODEL("sprites/streak.spr");
}	

//=========================================================
// AI Schedules Specific to this monster
//=========================================================
