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
#include	"animation.h"

// For holograms, make them not solid so the player can walk through them
#define	SF_GENERICMONSTER_NOTSOLID					4 

//=========================================================
// Monster's Anim Events Go Here
//=========================================================

class CVanlve : public CBaseMonster
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

	int TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType );

	void BarneyFirePistol( void );

	BOOL CheckRangeAttack1 ( float flDot, float flDist ) { return FALSE; }
	BOOL CheckRangeAttack2 ( float flDot, float flDist ) { return FALSE; }
};
LINK_ENTITY_TO_CLASS( monster_vanlve, CVanlve );
LINK_ENTITY_TO_CLASS( monster_vanlve_combat, CVanlve );
//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int	CVanlve :: Classify ( void )
{
	return	CLASS_PLAYER_ALLY;
}

void CVanlve :: RunAI( void )
{
	CBaseMonster :: RunAI();
}

void CVanlve :: SetActivity ( Activity NewActivity )
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
void CVanlve :: SetYawSpeed ( void )
{
	pev->yaw_speed = 90;
}

void CVanlve :: BarneyFirePistol ( void )
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
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CVanlve :: HandleAnimEvent( MonsterEvent_t *pEvent )
{
	switch( pEvent->event )
	{
	case 1:
		break;

	case 7:
	{
		EMIT_SOUND(ENT(pev), CHAN_WEAPON, "tank/tank_fire.wav", 1, 0.4);
		pev->effects = pev->effects | EF_MUZZLEFLASH;
	}
	break;

	case 8:
	{
		UTIL_SetSize(pev, Vector(-36,-36,0), Vector(36,36,72));

		pev->velocity.y = -3000;

		EMIT_SOUND_DYN( edict(), CHAN_WEAPON, "mario/mario_skill_hit.wav", 1.0, 0.3, 0, 100 );

		MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
		WRITE_BYTE( TE_BEAMFOLLOW );
		WRITE_SHORT( entindex() + 0x1000 * 2 );		// entity, attachment
		WRITE_SHORT(g_sModelIndexTrail );	// model
		WRITE_BYTE( 10 ); // life
		WRITE_BYTE( 7 );  // width
		WRITE_BYTE( 255 );	// R
		WRITE_BYTE( 32 );	// G
		WRITE_BYTE( 32 );	// B
		WRITE_BYTE( 192 );	// brightness
		MESSAGE_END();  // move PHS/PVS data sending into here (SEND_ALL, SEND_PVS, SEND_PHS)
	}
	break;

	case 9:
	{
		CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_gman_boss" );
		if ( pEntity )//¶ÔGmanÌØ¹¥
		{
			pEntity->pev->renderfx = kRenderFxExplode;
			pEntity->pev->rendercolor.x = 255;
			pEntity->pev->rendercolor.y = 255;
			pEntity->pev->rendercolor.z = 255;
			FX_Explosion(pEntity->Center(), 47 );
			EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, "rmxp/160-Skill04.wav", 1.0, 0.6, 0, 110);
			FX_Explosion( Center(), 126 );
			EMIT_SOUND_DYN ( ENT(pEntity->pev), CHAN_WEAPON,"newadd/fist_hearvy_hit2.wav", 1.0, 0.6, 0, 100 + RANDOM_LONG(-5,5) );
		}
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
int CVanlve :: ISoundMask ( void )
{
	return	NULL;
}

int CVanlve :: TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	if ( FClassnameIs ( pev, "monster_vanlve_combat" ) ){
	pev->sequence = LookupSequence( "float_smfinsh" );
	ResetSequenceInfo( );
	pev->frame = 0;
	return 0;
	}

	return CBaseMonster :: TakeDamage ( pevInflictor, pevAttacker, flDamage, bitsDamageType );
}

//=========================================================
// Spawn
//=========================================================
void CVanlve :: Spawn()
{
	Precache();

	SET_MODEL(ENT(pev), "models/biohomo_vanlve.mdl");

	UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	m_bloodColor		= BLOOD_COLOR_RED;
	pev->health			= 100;
	m_flFieldOfView		= 0.5;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState		= MONSTERSTATE_NONE;

	MonsterInit();

	if ( pev->spawnflags & SF_GENERICMONSTER_NOTSOLID )
	{
		pev->solid = SOLID_NOT;
		pev->takedamage = DAMAGE_NO;
	}

	SetBodygroup( 0, 1 );
	SetBodygroup( 2, 1 );

	if ( pev->spawnflags & SF_MONSTER_FADECORPSE )
	{
		pev->solid = SOLID_NOT;
		pev->takedamage = DAMAGE_NO;
		SetState( MONSTERSTATE_HUNT );
		SetBodygroup( 1, 1 );
		SetBits(pev->effects, EF_DIMLIGHT);
		pev->sequence = LookupSequence( "deadidle" );
		ResetSequenceInfo( );
		pev->frame = 0;
	}

	m_longming = 1;
	pev->spawnflags |= SF_MONSTER_PRISONER;

	if ( FClassnameIs ( pev, "monster_vanlve_combat" ) ){
	SetBodygroup( 1, 1 );
	SetBodygroup( 3, 1 );
	SetBodygroup( 4, 1 );
	pev->effects |= EF_DIMLIGHT;
	}
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CVanlve :: Precache()
{
	PRECACHE_MODEL("models/biohomo_vanlve.mdl");
	PRECACHE_SOUND( "buttons/button12.wav" );
	PRECACHE_SOUND ("tank/tank_fire.wav");
	PRECACHE_SOUND("rmxp/160-Skill04.wav");
	PRECACHE_SOUND("mario/mario_skill_hit.wav");
}	

//=========================================================
// AI Schedules Specific to this monster
//=========================================================
