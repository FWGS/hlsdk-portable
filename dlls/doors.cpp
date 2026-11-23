/***
*
*	Copyright (c) 1996-2001, Valve LLC. All rights reserved.
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

===== doors.cpp ========================================================

*/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "doors.h"

class CBaseDoor : public CBaseToggle
{
public:
	void Spawn() override;
	void Precache() override;
	void KeyValue( KeyValueData* data ) override;
	void Use( CBaseEntity* activator, CBaseEntity* caller, USE_TYPE useType, float value ) override;
	void Blocked( CBaseEntity* other ) override;
	int ObjectCaps() override { return ( pev->spawnflags & SF_DOOR_USE_ONLY ) ? FCAP_IMPULSE_USE : 0; }
	int Save( CSave& save ) override;
	int Restore( CRestore& restore ) override;
	void SetToggleState( int state ) override;
	virtual int PrecacheLockedSound();
	virtual int PrecacheUnlockedSound();
	virtual int PrecacheMoveSound();
	virtual int PrecacheStopSound();
	void EXPORT DoorTouch( CBaseEntity* other );

	// local functions
	int DoorActivate();
	void EXPORT DoorGoUp();
	void EXPORT DoorGoDown();
	void EXPORT DoorHitTop();
	void EXPORT DoorHitBottom();
	static TYPEDESCRIPTION m_SaveData[];

	BYTE m_bHealthValue; // some doors are medi-kit doors, they give players health
	BYTE m_bMoveSnd;			// sound a door makes while moving
	BYTE m_bStopSnd;			// sound a door makes when it stops
	locksound_t m_ls;			// door lock sounds
	BYTE m_bLockedSound;		// ordinals from entity selection
	BYTE m_bLockedSentence;
	BYTE m_bUnlockedSound;
	BYTE m_bUnlockedSentence;
	int m_sGlobalStateName;
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

TYPEDESCRIPTION CBaseDoor::m_SaveData[] =
{
	DEFINE_FIELD( CBaseDoor, m_bHealthValue, FIELD_CHARACTER ),
	DEFINE_FIELD( CBaseDoor, m_bMoveSnd, FIELD_CHARACTER ),
	DEFINE_FIELD( CBaseDoor, m_bStopSnd, FIELD_CHARACTER ),
	DEFINE_FIELD( CBaseDoor, m_bLockedSound, FIELD_CHARACTER ),
	DEFINE_FIELD( CBaseDoor, m_bLockedSentence, FIELD_CHARACTER ),
	DEFINE_FIELD( CBaseDoor, m_bUnlockedSound, FIELD_CHARACTER ),
	DEFINE_FIELD( CBaseDoor, m_bUnlockedSentence, FIELD_CHARACTER ),
	DEFINE_FIELD( CBaseDoor, m_sGlobalStateName, FIELD_STRING ),
};

IMPLEMENT_SAVERESTORE( CBaseDoor, CBaseToggle );

/*QUAKED func_door (0 .5 .8) ? START_OPEN x DOOR_DONT_LINK TOGGLE
if two doors touch, they are assumed to be connected and operate as a unit.

TOGGLE causes the door to wait in both the start and end states for a trigger event.

START_OPEN causes the door to move to its destination when spawned, and operate in reverse.
It is used to temporarily or permanently close off an area when triggered (not usefull for
touch or takedamage doors).

"angle"         determines the opening direction
"targetname"	if set, no touch field will be spawned and a remote button or trigger
				field activates the door.
"health"        if set, door must be shot open
"speed"         movement speed (100 default)
"wait"          wait before returning (3 default, -1 = never return)
"lip"           lip remaining at end of move (8 default)
"dmg"           damage to inflict when blocked (2 default)
"sounds"
0)      no sound
1)      stone
2)      base
3)      stone chain
4)      screechy metal
*/

LINK_ENTITY_TO_CLASS( func_door, CBaseDoor );
//
// func_water - same as a door. 
//
LINK_ENTITY_TO_CLASS( func_water, CBaseDoor );

void PlayLockSounds( entvars_t* pev, locksound_t* sounds, int locked, int button )
{
	float soundWait = button ? 0.5f : 3.0f;
	if ( locked )
	{
		BOOL playSound = sounds->sLockedSound && gpGlobals->time > sounds->flwaitSound;
		BOOL playSentence = sounds->sLockedSentence && !sounds->bEOFLocked && gpGlobals->time > sounds->flwaitSentence;
		if ( playSound )
		{
			EMIT_SOUND( ENT( pev ), CHAN_ITEM, STRING( sounds->sLockedSound ), playSentence ? 0.25f : 1.0f, ATTN_NORM );
			sounds->flwaitSound = gpGlobals->time + soundWait;
		}

		// if there is a sentence, we've not played all in list, and we've debounced, play sound
		if ( playSentence )
		{
			int previous = sounds->iLockedSentence;
			sounds->iLockedSentence = SENTENCEG_PlaySequentialSz( ENT( pev ), STRING( sounds->sLockedSentence ),
				0.85f, ATTN_NORM, 0, PITCH_NORM, previous, FALSE );
			sounds->iUnlockedSentence = 0;
			sounds->bEOFLocked = previous == sounds->iLockedSentence;
			sounds->flwaitSentence = gpGlobals->time + 6;
		}
	}
	else
	{
		BOOL playSound = sounds->sUnlockedSound && gpGlobals->time > sounds->flwaitSound;
		BOOL playSentence = sounds->sUnlockedSentence && !sounds->bEOFUnlocked && gpGlobals->time > sounds->flwaitSentence;
		if ( playSound )
		{
			EMIT_SOUND( ENT( pev ), CHAN_ITEM, STRING( sounds->sUnlockedSound ), playSentence ? 0.25f : 1.0f, ATTN_NORM );
			sounds->flwaitSound = gpGlobals->time + soundWait;
		}

		// play next 'door unlocked' sentence in group
		if ( playSentence )
		{
			int previous = sounds->iUnlockedSentence;
			sounds->iUnlockedSentence = SENTENCEG_PlaySequentialSz( ENT( pev ), STRING( sounds->sUnlockedSentence ),
				0.85f, ATTN_NORM, 0, PITCH_NORM, previous, FALSE );
			sounds->iLockedSentence = 0;
			sounds->bEOFUnlocked = previous == sounds->iUnlockedSentence;
			sounds->flwaitSentence = gpGlobals->time + 6;
		}
	}
}

//
// Cache user-entity-field values until spawn is called.
//

void CBaseDoor::KeyValue( KeyValueData* data )
{
	if ( FStrEq( data->szKeyName, "skin" ) ) //skin is used for content type
	{
		pev->skin = atof( data->szValue );
	}
	else if ( FStrEq( data->szKeyName, "movesnd" ) )
	{
		m_bMoveSnd = (BYTE)(int)atof( data->szValue );
	}
	else if ( FStrEq( data->szKeyName, "stopsnd" ) )
	{
		m_bStopSnd = (BYTE)(int)atof( data->szValue );
	}
	else if ( FStrEq( data->szKeyName, "healthvalue" ) )
	{
		m_bHealthValue = (BYTE)(int)atof( data->szValue );
	}
	else if ( FStrEq( data->szKeyName, "locked_sound" ) )
	{
		m_bLockedSound = (BYTE)(int)atof( data->szValue );
	}
	else if ( FStrEq( data->szKeyName, "locked_sentence" ) )
	{
		m_bLockedSentence = (BYTE)(int)atof( data->szValue );
	}
	else if ( FStrEq( data->szKeyName, "unlocked_sound" ) )
	{
		m_bUnlockedSound = (BYTE)(int)atof( data->szValue );
	}
	else if ( FStrEq( data->szKeyName, "unlocked_sentence" ) )
	{
		m_bUnlockedSentence = (BYTE)(int)atof( data->szValue );
	}
	else if ( FStrEq( data->szKeyName, "WaveHeight" ) )
	{
		pev->scale = atof( data->szValue ) * 0.125;
	}
	else if ( FStrEq( data->szKeyName, "globalstate" ) )
	{
		m_sGlobalStateName = ALLOC_STRING( data->szValue );
	}
	else
	{
		CBaseToggle::KeyValue( data );
		return;
	}
	data->fHandled = TRUE;
}

void CBaseDoor::Spawn()
{
	Precache();
	SetMovedir( pev );
	if ( pev->skin == 0 )
	{ //normal door
		pev->solid = ( pev->spawnflags & SF_DOOR_PASSABLE ) ? SOLID_NOT : SOLID_BSP;
	}
	else
	{ // special contents
		pev->solid = SOLID_NOT;
		pev->spawnflags |= SF_DOOR_SILENT;
	}
	pev->movetype = MOVETYPE_PUSH;
	UTIL_SetOrigin( pev, pev->origin );
	SET_MODEL( edict(), STRING( pev->model ) );
	if ( pev->speed == 0 )
	{
		pev->speed = 100;
	}
	m_vecPosition1 = pev->origin;
	m_vecPosition2 = m_vecPosition1 + pev->movedir *
		( fabs( pev->movedir.x * ( pev->size.x - 2 ) ) + fabs( pev->movedir.y * ( pev->size.y - 2 ) ) +
		fabs( pev->movedir.z * ( pev->size.z - 2 ) ) - m_flLip );
	if ( pev->spawnflags & SF_DOOR_START_OPEN )
	{	// swap pos1 and pos2, put door at pos2
		UTIL_SetOrigin( pev, m_vecPosition2 );
		m_vecPosition2 = m_vecPosition1;
		m_vecPosition1 = pev->origin;
	}
	m_toggle_state = TS_AT_BOTTOM;
	SetTouch( ( pev->spawnflags & SF_DOOR_USE_ONLY ) ? NULL : &CBaseDoor::DoorTouch );
}

void CBaseDoor::SetToggleState( int state )
{
	UTIL_SetOrigin( pev, state == TS_AT_TOP ? m_vecPosition2 : m_vecPosition1 );
}

int CBaseDoor::PrecacheMoveSound()
{
	static const char* sounds[] =
	{
		"doors/door_pneu_open1.wav", "doors/door_wood_slide1.wav", "doors/door_metal_slide1.wav",
		"doors/door_metal_slide2.wav", "doors/door_metal_open1.wav", "doors/door_water_rise1.wav",
		"doors/door_vault.wav", "doors/door_metal_open2.wav", "doors/door_wood_open1.wav",
		"doors/door_wood_open2.wav", "doors/door_elec_gate.wav"
	};
	const char* sound = m_bMoveSnd >= 1 && m_bMoveSnd <= ARRAYSIZE( sounds ) ? sounds[m_bMoveSnd - 1] : "common/null.wav";
	PRECACHE_SOUND( (char*)sound );
	return ALLOC_STRING( sound );
}

int CBaseDoor::PrecacheStopSound()
{
	static const char* sounds[] =
	{
		"doors/doorstop1.wav", "doors/doorstop2.wav", "doors/doorstop3.wav", "doors/doorstop4.wav",
		"doors/doorstop5.wav", "doors/doorstop6.wav", "doors/doorstop7.wav", "doors/doorstop8.wav",
		"common/null.wav", "common/null.wav", "doors/door_elec_gatestop.wav"
	};
	const char* sound = m_bStopSnd >= 1 && m_bStopSnd <= ARRAYSIZE( sounds ) ? sounds[m_bStopSnd - 1] : "common/null.wav";
	PRECACHE_SOUND( (char*)sound );
	return ALLOC_STRING( sound );
}

int CBaseDoor::PrecacheLockedSound()
{
	static const char* sounds[] =
	{
		"doors/door_wood_knob.wav", "doors/door_wood_locked1.wav", "doors/door_wood_shake1.wav",
		"doors/door_metal_latch.wav", "doors/door_metal_shake1.wav", "doors/door_metal_shake2.wav",
		"doors/door_metal_locked1.wav", "doors/door_metal_hit.wav", "doors/door_pneu_locked.wav", "doors/door_pneu_buzz.wav"
	};
	const char* sound = m_bLockedSound >= 1 && m_bLockedSound <= ARRAYSIZE( sounds ) ? sounds[m_bLockedSound - 1] : "common/null.wav";
	PRECACHE_SOUND( (char*)sound );
	return ALLOC_STRING( sound );
}

int CBaseDoor::PrecacheUnlockedSound()
{
	static const char* sounds[] =
	{
		"buttons/button1.wav", "buttons/button3.wav", "buttons/button4.wav", "buttons/button5.wav", "buttons/button6.wav",
		"buttons/button7.wav", "buttons/button8.wav", "buttons/button9.wav", "buttons/button10.wav", "buttons/latchunlocked1.wav"
	};
	const char* sound = m_bUnlockedSound >= 1 && m_bUnlockedSound <= ARRAYSIZE( sounds ) ? sounds[m_bUnlockedSound - 1] : "common/null.wav";
	PRECACHE_SOUND( (char*)sound );
	return ALLOC_STRING( sound );
}

void CBaseDoor::Precache()
{
	pev->noise1 = PrecacheMoveSound();
	pev->noise2 = PrecacheStopSound();
	m_ls.sLockedSound = PrecacheLockedSound();
	m_ls.sUnlockedSentence = PrecacheUnlockedSound();

	// get sentence group names, for doors which are directly 'touched' to open

	switch ( m_bLockedSentence )
	{
	case 1: m_ls.sLockedSentence = ALLOC_STRING( "NA" ); break; // access denied
	case 2: m_ls.sLockedSentence = ALLOC_STRING( "ND" ); break; // security lockout
	case 3: m_ls.sLockedSentence = ALLOC_STRING( "NF" ); break; // blast door
	case 4: m_ls.sLockedSentence = ALLOC_STRING( "NFIRE" ); break; // fire door
	case 5: m_ls.sLockedSentence = ALLOC_STRING( "NCHEM" ); break; // chemical door
	case 6: m_ls.sLockedSentence = ALLOC_STRING( "NRAD" ); break; // radiation door
	case 7: m_ls.sLockedSentence = ALLOC_STRING( "NCON" ); break; // gen containment
	case 8: m_ls.sLockedSentence = ALLOC_STRING( "NH" ); break; // maintenance door
	case 9: m_ls.sLockedSentence = ALLOC_STRING( "NG" ); break; // broken door
	default: m_ls.sLockedSentence = 0; break;
	}
	switch ( m_bUnlockedSentence )
	{
	case 1: m_ls.sUnlockedSentence = ALLOC_STRING( "EA" ); break; // access granted
	case 2: m_ls.sUnlockedSentence = ALLOC_STRING( "ED" ); break; // security door
	case 3: m_ls.sUnlockedSentence = ALLOC_STRING( "EF" ); break; // blast door
	case 4: m_ls.sUnlockedSentence = ALLOC_STRING( "EFIRE" ); break; // fire door
	case 5: m_ls.sUnlockedSentence = ALLOC_STRING( "ECHEM" ); break; // chemical door
	case 6: m_ls.sUnlockedSentence = ALLOC_STRING( "ERAD" ); break; // radiation door
	case 7: m_ls.sUnlockedSentence = ALLOC_STRING( "ECON" ); break; // gen containment
	case 8: m_ls.sUnlockedSentence = ALLOC_STRING( "EH" ); break; // maintenance door
	default: m_ls.sUnlockedSentence = 0; break;
	}
}

//
// Doors not tied to anything (e.g. button, another door) can be touched, to make them activate.
//
void CBaseDoor::DoorTouch( CBaseEntity* other )
{
	if ( !FClassnameIs( other->pev, "player" ) )
	{
		return;
	}
	if ( m_sMaster && !UTIL_IsMasterTriggered( m_sMaster, other ) )
	{
		PlayLockSounds( pev, &m_ls, TRUE, FALSE );
	}
	if ( m_sGlobalStateName && gGlobalState.EntityGetState( m_sGlobalStateName ) != GLOBAL_ON )
	{
		PlayLockSounds( pev, &m_ls, TRUE, FALSE );
		return;
	}

	// If door is somebody's target, then touching does nothing.
	// You have to activate the owner (e.g. button).

	if ( !FStringNull( pev->targetname ) )
	{
		// play locked sound
		PlayLockSounds( pev, &m_ls, TRUE, FALSE );
		return;
	}
	m_hActivator = other; // remember who activated the door
	if ( DoorActivate() )
	{
		SetTouch( NULL );
	}
}


//
// Used by SUB_UseTargets, when a door is the target of a button.
//
void CBaseDoor::Use( CBaseEntity* activator, CBaseEntity*, USE_TYPE, float )
{
	m_hActivator = activator;
	if ( m_toggle_state == TS_AT_BOTTOM || ( ( pev->spawnflags & SF_DOOR_NO_AUTO_RETURN ) && m_toggle_state == TS_AT_TOP ) )
	{
		DoorActivate();
	}
}

//
// Causes the door to "do its thing", i.e. start moving, and cascade activation.
//
int CBaseDoor::DoorActivate()
{
	if ( !UTIL_IsMasterTriggered( m_sMaster, m_hActivator ) )
	{
		return 0;
	}
	if ( ( pev->spawnflags & SF_DOOR_NO_AUTO_RETURN ) && m_toggle_state == TS_AT_TOP )
	{ // door should close
		DoorGoDown();
	}
	else
	{ // door should open
		if ( m_hActivator && m_hActivator->IsPlayer() )
		{ // give health if player opened the door (medikit)
		// VARS( m_eoActivator )->health += m_bHealthValue;

			m_hActivator->TakeHealth( m_bHealthValue, DMG_GENERIC );
		}

		// play door unlock sounds
		PlayLockSounds( pev, &m_ls, FALSE, FALSE );
		DoorGoUp();
	}
	return 1;
}

void CBaseDoor::DoorGoUp()
{
	if ( !( pev->spawnflags & SF_DOOR_SILENT ) )
	{
		EMIT_SOUND( edict(), CHAN_STATIC, STRING( pev->noise1 ), 1, ATTN_NORM );
	}
	m_toggle_state = TS_GOING_UP;
	SetMoveDone( &CBaseDoor::DoorHitTop );
	if ( FClassnameIs( pev, "func_door_rotating" ) )		// !!! BUGBUG Triggered doors don't work with this yet
	{
		float sign = 1;
		if ( m_hActivator )
		{
			entvars_t* activator = m_hActivator->pev;
			if ( !( pev->spawnflags & SF_DOOR_ONEWAY ) && pev->movedir.y )
			{
				Vector delta = activator->origin - pev->origin;
				Vector angles = activator->angles;
				angles.x = 0;
				angles.z = 0;
				UTIL_MakeVectors( angles );
				UTIL_MakeVectors( activator->angles );
				Vector next = activator->origin + gpGlobals->v_forward * 10 - pev->origin;
				if ( delta.x * next.y - delta.y * next.x < 0 )
				{
					sign = -1;
				}
			}
		}
		AngularMove( m_vecAngle2 * sign, pev->speed );
	}
	else
	{
		LinearMove( m_vecPosition2, pev->speed );
	}
}


//
// The door has reached the "up" position.  Either go back down, or wait for another activation.
//
void CBaseDoor::DoorHitTop()
{
	if ( !( pev->spawnflags & SF_DOOR_SILENT ) )
	{
		STOP_SOUND( edict(), CHAN_STATIC, STRING( pev->noise1 ) );
		EMIT_SOUND( edict(), CHAN_STATIC, STRING( pev->noise2 ), 1, ATTN_NORM );
	}
	m_toggle_state = TS_AT_TOP;
	if ( pev->spawnflags & SF_DOOR_NO_AUTO_RETURN )
	{
		if ( !( pev->spawnflags & SF_DOOR_USE_ONLY ) )
		{
			SetTouch( &CBaseDoor::DoorTouch );
		}
	}
	else
	{
		// In flWait seconds, DoorGoDown will fire, unless wait is -1, then door stays open
		pev->nextthink = pev->ltime + m_flWait;
		SetThink( &CBaseDoor::DoorGoDown );
		if ( m_flWait == -1 )
		{
			pev->nextthink = -1;
		}
	}

	// Fire the close target (if startopen is set, then "top" is closed) - netname is the close target
	if ( pev->netname && ( pev->spawnflags & SF_DOOR_START_OPEN ) )
	{
		FireTargets( STRING( pev->netname ), m_hActivator, this, USE_TOGGLE, 0 );
	}
	SUB_UseTargets( m_hActivator, USE_TOGGLE, 0 );
}


//
// Starts the door going to its "down" position (simply ToggleData->vecPosition1).
//
void CBaseDoor::DoorGoDown()
{
	if ( !( pev->spawnflags & SF_DOOR_SILENT ) )
	{
		EMIT_SOUND( edict(), CHAN_STATIC, STRING( pev->noise1 ), 1, ATTN_NORM );
	}
	m_toggle_state = TS_GOING_DOWN;
	SetMoveDone( &CBaseDoor::DoorHitBottom );
	if ( FClassnameIs( pev, "func_door_rotating" ) ) //rotating door
	{
		AngularMove( m_vecAngle1, pev->speed );
	}
	else
	{
		LinearMove( m_vecPosition1, pev->speed );
	}
}

void CBaseDoor::DoorHitBottom()
{
	if ( !( pev->spawnflags & SF_DOOR_SILENT ) )
	{
		STOP_SOUND( edict(), CHAN_STATIC, STRING( pev->noise1 ) );
		EMIT_SOUND( edict(), CHAN_STATIC, STRING( pev->noise2 ), 1, ATTN_NORM );
	}
	m_toggle_state = TS_AT_BOTTOM;
	SetTouch( ( pev->spawnflags & SF_DOOR_USE_ONLY ) ? NULL : &CBaseDoor::DoorTouch );
	SUB_UseTargets( m_hActivator, USE_TOGGLE, 0 );

	// Fire the close target (if startopen is set, then "top" is closed) - netname is the close target
	if ( pev->netname && !( pev->spawnflags & SF_DOOR_START_OPEN ) )
	{
		FireTargets( STRING( pev->netname ), m_hActivator, this, USE_TOGGLE, 0 );
	}
}

void CBaseDoor::Blocked( CBaseEntity* other )
{


	// Hurt the blocker a little.
	if ( pev->dmg )
	{
		other->TakeDamage( pev, pev, pev->dmg, DMG_CRUSH );
	}

	// if a door has a negative wait, it would never come back if blocked,
	// so let it just squash the object to death real fast

	if ( m_flWait >= 0 )
	{
		if ( m_toggle_state == TS_GOING_DOWN )
		{
			DoorGoUp();
		}
		else
		{
			DoorGoDown();
		}
	}

	// Block all door pieces with the same targetname here.
	if ( FStringNull( pev->targetname ) )
	{
		return;
	}
	edict_t* target = NULL;
	while ( !FNullEnt( target = FIND_ENTITY_BY_TARGETNAME( target, STRING( pev->targetname ) ) ) )
	{
		if ( VARS( target ) == pev || !( FClassnameIs( target, "func_door" ) || FClassnameIs( target, "func_door_rotating" ) ) )
		{
			continue;
		}
		CBaseDoor* door = GetClassPtr( (CBaseDoor*)VARS( target ) );
		if ( door->m_flWait < 0 )
		{
			continue;
		}
		if ( door->pev->velocity == pev->velocity && door->pev->avelocity == pev->velocity )
		{
			// this is the most hacked, evil, bastardized thing I've ever seen. kjb
			if ( FClassnameIs( target, "func_door" ) )
			{
				door->pev->origin = pev->origin;
				door->pev->velocity = g_vecZero;
			}
			else
			{ // set angles to realign rotating doors
				door->pev->angles = pev->angles;
				door->pev->avelocity = g_vecZero;
			}
		}
		if ( door->m_toggle_state == TS_GOING_DOWN )
		{
			door->DoorGoUp();
		}
		else
		{
			door->DoorGoDown();
		}
	}
}


/*QUAKED FuncRotDoorSpawn (0 .5 .8) ? START_OPEN REVERSE  
DOOR_DONT_LINK TOGGLE X_AXIS Y_AXIS
if two doors touch, they are assumed to be connected and operate as  
a unit.

TOGGLE causes the door to wait in both the start and end states for  
a trigger event.

START_OPEN causes the door to move to its destination when spawned,  
and operate in reverse.  It is used to temporarily or permanently  
close off an area when triggered (not usefull for touch or  
takedamage doors).

You need to have an origin brush as part of this entity.  The  
center of that brush will be
the point around which it is rotated. It will rotate around the Z  
axis by default.  You can
check either the X_AXIS or Y_AXIS box to change that.

"distance" is how many degrees the door will be rotated.
"speed" determines how fast the door moves; default value is 100.

REVERSE will cause the door to rotate in the opposite direction.

"angle"		determines the opening direction
"targetname" if set, no touch field will be spawned and a remote  
button or trigger field activates the door.
"health"	if set, door must be shot open
"speed"		movement speed (100 default)
"wait"		wait before returning (3 default, -1 = never return)
"dmg"		damage to inflict when blocked (2 default)
"sounds"
0)	no sound
1)	stone
2)	base
3)	stone chain
4)	screechy metal
*/
class CRotDoor : public CBaseDoor
{
public:
	void Spawn() override;
	void SetToggleState( int state ) override;
};

LINK_ENTITY_TO_CLASS( func_door_rotating, CRotDoor );

void CRotDoor::Spawn()
{
	Precache();
	AxisDir( pev );
	if ( pev->spawnflags & SF_DOOR_ROTATE_BACKWARDS )
	{
		pev->movedir = pev->movedir * -1;
	}

	//m_flWait			= 2; who the hell did this? (sjb)
	m_vecAngle1 = pev->angles;
	m_vecAngle2 = pev->angles + pev->movedir * m_flMoveDistance;
	pev->solid = ( pev->spawnflags & SF_DOOR_PASSABLE ) ? SOLID_NOT : SOLID_BSP;
	pev->movetype = MOVETYPE_PUSH;
	UTIL_SetOrigin( pev, pev->origin );
	SET_MODEL( edict(), STRING( pev->model ) );
	if ( pev->speed == 0 )
	{
		pev->speed = 100;
	}
	if ( pev->spawnflags & SF_DOOR_START_OPEN )
	{
		pev->angles = m_vecAngle2;
		m_vecAngle2 = m_vecAngle1;
		pev->movedir = pev->movedir * -1;
	}
	m_toggle_state = TS_AT_BOTTOM;
	SetTouch( ( pev->spawnflags & SF_DOOR_USE_ONLY ) ? NULL : &CBaseDoor::DoorTouch );
}

void CRotDoor::SetToggleState( int state )
{
	pev->angles = state == TS_AT_TOP ? m_vecAngle2 : m_vecAngle1;
	UTIL_SetOrigin( pev, pev->origin );
}

class CMomentaryDoor : public CBaseToggle
{
public:
	void Spawn() override;
	void Precache() override;
	void KeyValue( KeyValueData* data ) override;
	void Use( CBaseEntity* activator, CBaseEntity* caller, USE_TYPE useType, float value ) override;
	int ObjectCaps() override { return 0; }
	int Save( CSave& save ) override;
	int Restore( CRestore& restore ) override;
	static TYPEDESCRIPTION m_SaveData[];
	BYTE m_bMoveSnd;
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

LINK_ENTITY_TO_CLASS( momentary_door, CMomentaryDoor );

TYPEDESCRIPTION CMomentaryDoor::m_SaveData[] =
{
	DEFINE_FIELD( CMomentaryDoor, m_bMoveSnd, FIELD_CHARACTER ),
};

IMPLEMENT_SAVERESTORE( CMomentaryDoor, CBaseToggle );

void CMomentaryDoor::Spawn()
{
	SetMovedir( pev );
	pev->solid = SOLID_BSP;
	pev->movetype = MOVETYPE_PUSH;
	UTIL_SetOrigin( pev, pev->origin );
	SET_MODEL( edict(), STRING( pev->model ) );
	if ( pev->speed == 0 )
	{
		pev->speed = 100;
	}
	if ( pev->dmg == 0 )
	{
		pev->dmg = 2;
	}
	m_vecPosition1 = pev->origin;
	m_vecPosition2 = m_vecPosition1 + pev->movedir *
		( fabs( pev->movedir.x * ( pev->size.x - 2 ) ) + fabs( pev->movedir.y * ( pev->size.y - 2 ) ) +
		fabs( pev->movedir.z * ( pev->size.z - 2 ) ) - m_flLip );
	if ( pev->spawnflags & SF_DOOR_START_OPEN )
	{	// swap pos1 and pos2, put door at pos2
		UTIL_SetOrigin( pev, m_vecPosition2 );
		m_vecPosition2 = m_vecPosition1;
		m_vecPosition1 = pev->origin;
	}
	SetTouch( NULL );
	Precache();
}

void CMomentaryDoor::Precache()
{
	const char* sound = NULL;
	switch ( m_bMoveSnd )
	{
	case 1: sound = "doors/doormove1.wav"; break;
	case 2: sound = "doors/doormove2.wav"; break;
	case 3: sound = "doors/doormove3.wav"; break;
	case 4: sound = "doors/doormove4.wav"; break;
	case 5: sound = "doors/doormove5.wav"; break;
	case 6: sound = "doors/doormove6.wav"; break;
	case 7: sound = "doors/doormove7.wav"; break;
	case 8: sound = "doors/doormove8.wav"; break;
	}
	if ( sound )
	{
		PRECACHE_SOUND( (char*)sound );
		pev->noise1 = ALLOC_STRING( sound );
	}
	else
	{
		pev->noise1 = ALLOC_STRING( "common/null.wav" );
	}
}

void CMomentaryDoor::KeyValue( KeyValueData* data )
{
	if ( FStrEq( data->szKeyName, "movesnd" ) )
	{
		m_bMoveSnd = (BYTE)(int)atof( data->szValue );
		data->fHandled = TRUE;
	}
	else if ( FStrEq( data->szKeyName, "stopsnd" ) || FStrEq( data->szKeyName, "healthvalue" ) )
	{
//		m_bHealthValue = atof(pkvd->szValue);
		data->fHandled = TRUE;
	}
	else
	{
		CBaseToggle::KeyValue( data );
	}
}

void CMomentaryDoor::Use( CBaseEntity*, CBaseEntity*, USE_TYPE useType, float value )
{
	if ( useType != USE_SET )		// Momentary buttons will pass down a float in here
	{
		return;
	}
	if ( value > 1 )
	{
		value = 1;
	}
	Vector destination = m_vecPosition1 + ( m_vecPosition2 - m_vecPosition1 ) * value;
	float speed = ( destination - pev->origin ).Length() * 10;
	if ( speed == 0 )
	{
		return;
	}

	// This entity only thinks when it moves, so if it's thinking, it's in the process of moving
	// play the sound when it starts moving (not yet thinking)
	if ( pev->nextthink < pev->ltime || pev->nextthink == 0 )
	{
		EMIT_SOUND( edict(), CHAN_STATIC, STRING( pev->noise1 ), 1, ATTN_NORM );
	}
	LinearMove( destination, speed );
}
