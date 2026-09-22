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

class CSicker623 : public CBaseMonster
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

//	CSprite		*m_pEyeGlow;		// Glow around the eyes
	
//	virtual int		Save( CSave &save );
//	virtual int		Restore( CRestore &restore );
	
//	static	TYPEDESCRIPTION m_SaveData[];

	BOOL CheckRangeAttack1 ( float flDot, float flDist ) { return FALSE; }
	BOOL CheckRangeAttack2 ( float flDot, float flDist ) { return FALSE; }
};
LINK_ENTITY_TO_CLASS( monster_sicker_623, CSicker623 );
LINK_ENTITY_TO_CLASS( monster_sicker_zz, CSicker623 );
/*
TYPEDESCRIPTION	CSicker623::m_SaveData[] = 
{
	DEFINE_FIELD( CSicker623, m_pEyeGlow, FIELD_CLASSPTR ),
};

IMPLEMENT_SAVERESTORE( CSicker623, CBaseMonster );
*/
//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int	CSicker623 :: Classify ( void )
{
	return	CLASS_PLAYER;
}

void CSicker623 :: RunAI( void )
{
	if (pev->velocity != g_vecZero){
	pev->velocity = g_vecZero;
	}

	if(!FNullEnt( FIND_CLIENT_IN_PVS( edict() ) ) && FClassnameIs( pev, "monster_sicker_623")){

		if(m_bloodColor != BLOOD_COLOR_RED){
		m_bloodColor = BLOOD_COLOR_RED;
		pev->solid	= SOLID_SLIDEBOX;
		}

		if(pev->frags <= 30){
			pev->frags += 1;
			pev->health = pev->max_health;

			if(pev->sequence != LookupActivity ( ACT_SLEEP )){
			SetActivity ( ACT_SLEEP );
			}
		}
		else if (pev->weapons == 0 ){
			pev->weapons = 1;
			pev->frame = 0;
			pev->framerate = 0;
			FireTargets( "sicker_golden_key", this, this, USE_TOGGLE, 0 );
		}
	}
	else if(m_bloodColor != DONT_BLEED){
		m_bloodColor = DONT_BLEED;
		pev->solid	= SOLID_NOT;
	}

	CBaseMonster :: RunAI();
}

void CSicker623 :: SetActivity ( Activity NewActivity )
{
	int	iSequence = ACTIVITY_NOT_AVAILABLE;
	void *pmodel = GET_MODEL_PTR( ENT(pev) );

	switch ( NewActivity)
	{
	case ACT_RUN:
		iSequence = LookupActivity ( NewActivity );
		break;
	case ACT_WALK:
		iSequence = LookupActivity ( NewActivity );
		break;
	case ACT_IDLE:
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
void CSicker623 :: SetYawSpeed ( void )
{
	pev->yaw_speed = 90;
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CSicker623 :: HandleAnimEvent( MonsterEvent_t *pEvent )
{
	switch( pEvent->event )
	{
	case 1:
		{
		//	m_pEyeGlow = CSprite::SpriteCreate( "sprites/glow01.spr", pev->origin, FALSE );
		//	m_pEyeGlow->SetTransparency( kRenderGlow, 255, 255, 128, 192, kRenderFxNoDissipation );
		//	m_pEyeGlow->SetAttachment( edict(), 1 );
		//	m_pEyeGlow->pev->scale = 0.5;
		}
		break;

	default:
		CBaseMonster::HandleAnimEvent( pEvent );
		break;
	}
}

void CSicker623 :: BarneyFirePistol ( void )
{
	Vector vecShootOrigin;

	UTIL_MakeVectors(pev->angles);

	vecShootOrigin = pev->origin + Vector( 0, 0, 65 );
	
	Vector vecShootDir = ShootAtEnemy( vecShootOrigin );

	pev->effects = EF_MUZZLEFLASH;

	int pitchShift = RANDOM_LONG( 0, 20 );
	
	// Only shift about half the time
	if ( pitchShift > 10 )
		pitchShift = 0;
	else
		pitchShift -= 5;
	

	int iBulletType;
	iBulletType = BULLET_12MM;

	FireBullets(1, vecShootOrigin, vecShootDir, Vector(0.0,0.0,0.0), 2048, iBulletType,1);

	EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, "weapons/pl_gun3.wav", 1, ATTN_NORM, 0, 100 + pitchShift );

	CSoundEnt::InsertSound ( bits_SOUND_COMBAT, pev->origin, 384, 0.3 );
}

//=========================================================
// ISoundMask - generic monster can't hear.
//=========================================================
int CSicker623 :: ISoundMask ( void )
{
	return	NULL;
}

//=========================================================
// Spawn
//=========================================================
void CSicker623 :: Spawn()
{
	Precache();

	SET_MODEL(ENT(pev), "models/sicker623.mdl");

	UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	m_bloodColor		= BLOOD_COLOR_RED;
	pev->health			= 2000;
	m_flFieldOfView		= -1.0;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState		= MONSTERSTATE_NONE;

	MonsterInit();

	SetBits(pev->effects, EF_DIMLIGHT);
	m_longming = 1;
	pev->movetype = MOVETYPE_NOCLIP;
	m_groundElev = TRUE;

	if( FClassnameIs( pev, "monster_sicker_zz") ){
	SET_MODEL(ENT(pev), "models/god623.mdl");
	pev->rendermode  = kRenderTransTexture;
	pev->renderfx	 = kRenderFxHologram;
	pev->renderamt	 = 255;
	}
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CSicker623 :: Precache()
{
	PRECACHE_MODEL("models/sicker623.mdl");
	PRECACHE_MODEL("models/zdeadeye.mdl");
	PRECACHE_MODEL("models/god623.mdl");
}	