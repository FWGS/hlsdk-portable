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

===== doors.cpp ========================================================

*/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "doors.h"
#include "game.h"
#include "weapons.h"
#include "player.h"
#include "shake.h"

extern void SetMovedir( entvars_t *ev );

extern DLL_GLOBAL int		g_restore_fix;

#define noiseMoving noise1
#define noiseArrived noise2

class CBaseDoor : public CBaseToggle
{
public:
	void Spawn( void );
	void Precache( void );
	virtual void KeyValue( KeyValueData *pkvd );
	virtual void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	virtual void Blocked( CBaseEntity *pOther );
	virtual int TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType );
	void TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType );

	virtual int ObjectCaps( void ) 
	{ 
		if( pev->spawnflags & SF_ITEM_USE_ONLY )
			return ( CBaseToggle::ObjectCaps() & ~FCAP_ACROSS_TRANSITION ) | FCAP_IMPULSE_USE;
		else
			return (CBaseToggle::ObjectCaps() & ~FCAP_ACROSS_TRANSITION);
	};
	virtual int Save( CSave &save );
	virtual int Restore( CRestore &restore );
	void EXPORT		Die( void );
	static TYPEDESCRIPTION m_SaveData[];

	virtual void SetToggleState( int state );

	int pGibName;
	int pGibName2;
	int pGibName3;
	int pGibName4;

	// used to selectivly override defaults
	void EXPORT DoorTouch( CBaseEntity *pOther );

	// local functions
	int DoorActivate();
	void EXPORT DoorGoUp( void );
	void EXPORT DoorGoDown( void );
	void EXPORT DoorHitTop( void );
	void EXPORT DoorHitBottom( void );

	BYTE m_bHealthValue;// some doors are medi-kit doors, they give players health

	BYTE m_bMoveSnd;			// sound a door makes while moving
	BYTE m_bStopSnd;			// sound a door makes when it stops

	locksound_t m_ls;			// door lock sounds

	BYTE m_bLockedSound;		// ordinals from entity selection
	BYTE m_bLockedSentence;	
	BYTE m_bUnlockedSound;	
	BYTE m_bUnlockedSentence;
};

TYPEDESCRIPTION	CBaseDoor::m_SaveData[] =
{
	DEFINE_FIELD( CBaseDoor, m_bHealthValue, FIELD_CHARACTER ),
	DEFINE_FIELD( CBaseDoor, m_bMoveSnd, FIELD_CHARACTER ),
	DEFINE_FIELD( CBaseDoor, m_bStopSnd, FIELD_CHARACTER ),

	DEFINE_FIELD( CBaseDoor, m_bLockedSound, FIELD_CHARACTER ),
	DEFINE_FIELD( CBaseDoor, m_bLockedSentence, FIELD_CHARACTER ),
	DEFINE_FIELD( CBaseDoor, m_bUnlockedSound, FIELD_CHARACTER ),
	DEFINE_FIELD( CBaseDoor, m_bUnlockedSentence, FIELD_CHARACTER ),
};

IMPLEMENT_SAVERESTORE( CBaseDoor, CBaseToggle )

#define DOOR_SENTENCEWAIT	6.0f
#define DOOR_SOUNDWAIT		3.0f
#define BUTTON_SOUNDWAIT	0.5f

// play door or button locked or unlocked sounds. 
// pass in pointer to valid locksound struct. 
// if flocked is true, play 'door is locked' sound,
// otherwise play 'door is unlocked' sound
// NOTE: this routine is shared by doors and buttons

void PlayLockSounds( entvars_t *pev, locksound_t *pls, int flocked, int fbutton )
{
	// LOCKED SOUND

	// CONSIDER: consolidate the locksound_t struct (all entries are duplicates for lock/unlock)
	// CONSIDER: and condense this code.
	float flsoundwait;

	if( fbutton )
		flsoundwait = BUTTON_SOUNDWAIT;
	else
		flsoundwait = DOOR_SOUNDWAIT;

	if( flocked )
	{
		int fplaysound = ( pls->sLockedSound && gpGlobals->time > pls->flwaitSound );
		int fplaysentence = ( pls->sLockedSentence && !pls->bEOFLocked && gpGlobals->time > pls->flwaitSentence );
		float fvol;

		if( fplaysound && fplaysentence )
			fvol = 0.25f;
		else
			fvol = 1.0f;

		// if there is a locked sound, and we've debounced, play sound
		if( fplaysound )
		{
			// play 'door locked' sound
			EMIT_SOUND( ENT( pev ), CHAN_ITEM, STRING( pls->sLockedSound ), fvol, ATTN_NORM );
			pls->flwaitSound = gpGlobals->time + flsoundwait;
		}

		// if there is a sentence, we've not played all in list, and we've debounced, play sound
		if( fplaysentence )
		{
			// play next 'door locked' sentence in group
			int iprev = pls->iLockedSentence;

			pls->iLockedSentence = SENTENCEG_PlaySequentialSz( ENT( pev ), STRING( pls->sLockedSentence ),
					  0.85f, ATTN_NORM, 0, 100, pls->iLockedSentence, FALSE );
			pls->iUnlockedSentence = 0;

			// make sure we don't keep calling last sentence in list
			pls->bEOFLocked = ( iprev == pls->iLockedSentence );
	
			pls->flwaitSentence = gpGlobals->time + DOOR_SENTENCEWAIT;
		}
	}
	else
	{
		// UNLOCKED SOUND

		int fplaysound = ( pls->sUnlockedSound && gpGlobals->time > pls->flwaitSound );
		int fplaysentence = ( pls->sUnlockedSentence && !pls->bEOFUnlocked && gpGlobals->time > pls->flwaitSentence );
		float fvol;

		// if playing both sentence and sound, lower sound volume so we hear sentence
		if( fplaysound && fplaysentence )
			fvol = 0.25f;
		else
			fvol = 1.0f;

		// play 'door unlocked' sound if set
		if( fplaysound )
		{
			EMIT_SOUND( ENT( pev ), CHAN_ITEM, STRING( pls->sUnlockedSound ), fvol, ATTN_NORM );
			pls->flwaitSound = gpGlobals->time + flsoundwait;
		}

		// play next 'door unlocked' sentence in group
		if( fplaysentence )
		{
			int iprev = pls->iUnlockedSentence;

			pls->iUnlockedSentence = SENTENCEG_PlaySequentialSz( ENT( pev ), STRING( pls->sUnlockedSentence ),
					  0.85f, ATTN_NORM, 0, 100, pls->iUnlockedSentence, FALSE );
			pls->iLockedSentence = 0;

			// make sure we don't keep calling last sentence in list
			pls->bEOFUnlocked = ( iprev == pls->iUnlockedSentence );
			pls->flwaitSentence = gpGlobals->time + DOOR_SENTENCEWAIT;
		}
	}
}

//
// Cache user-entity-field values until spawn is called.
//
void CBaseDoor::KeyValue( KeyValueData *pkvd )
{
	if( FStrEq( pkvd->szKeyName, "skin" ) )//skin is used for content type
	{
		pev->skin = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "movesnd" ) )
	{
		m_bMoveSnd = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "stopsnd" ) )
	{
		m_bStopSnd = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "healthvalue" ) )
	{
		m_bHealthValue = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "locked_sound" ) )
	{
		m_bLockedSound = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "locked_sentence" ) )
	{
		m_bLockedSentence = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "unlocked_sound" ) )
	{
		m_bUnlockedSound = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "unlocked_sentence" ) )
	{
		m_bUnlockedSentence = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "WaveHeight" ) )
	{
		pev->scale = atof( pkvd->szValue ) * ( 1.0f / 8.0f );
		pkvd->fHandled = TRUE;
	}
	else
		CBaseToggle::KeyValue( pkvd );
}

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

LINK_ENTITY_TO_CLASS( func_door, CBaseDoor )
LINK_ENTITY_TO_CLASS( func_door_breaker, CBaseDoor );
LINK_ENTITY_TO_CLASS( func_door_toggle, CBaseDoor );
//
// func_water - same as a door. 
//
LINK_ENTITY_TO_CLASS( func_water, CBaseDoor )

void CBaseDoor::TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType )
{
	if ( (FClassnameIs( pev, "func_door_breaker" ) || FClassnameIs ( pev, "func_door_rotating_breaker" ) ) && pev->armortype != 3 )
	{
		if ( pev->dmgtime != gpGlobals->time)
		{
			pev->dmgtime = gpGlobals->time;
			UTIL_WhiteSparks( ptr->vecEndPos, ptr->vecPlaneNormal, 9, 5, 5, 100 );//puntos
		}
	}
	CBaseDelay::TraceAttack( pevAttacker, flDamage, vecDir, ptr, bitsDamageType );
}

void CBaseDoor::Die( void )
{
	Vector vecSpot;// shard origin
	Vector vecVelocity;// shard velocity
	CBaseEntity *pEntity = NULL;
	char cFlag = 0;
	int pitch;
	float fvol;
	
	pitch = 95 + RANDOM_LONG(0,29);

	if (pitch > 97 && pitch < 103)
		pitch = 100;

	// The more negative pev->health, the louder
	// the sound should be.

	fvol = 1.0;

	if(pev->armortype == 3)
	{
		switch ( RANDOM_LONG(0,1) )
		{
		case 0:	
			EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "debris/bustglass1.wav", fvol, ATTN_NORM, 0, pitch);	
			break;
		case 1:	
			EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "debris/bustglass2.wav", fvol, ATTN_NORM, 0, pitch);	
			break;
		}
		cFlag = BREAK_GLASS;
	}
	else if(pev->armortype == 2)
	{
		switch ( RANDOM_LONG(0,1) )
		{
		case 0:	
			EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "debris/bustflesh1.wav", fvol, ATTN_NORM, 0, pitch);	
			break;
		case 1:	
			EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "debris/bustflesh2.wav", fvol, ATTN_NORM, 0, pitch);	
			break;
		}
		cFlag = BREAK_WOOD;
	}
	else if(pev->armortype == 1)
	{
		switch ( RANDOM_LONG(0,1) )
		{
		case 0:	
			EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "debris/bustcrate1.wav", fvol, ATTN_NORM, 0, pitch);	
			break;
		case 1:	
			EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "debris/bustcrate2.wav", fvol, ATTN_NORM, 0, pitch);	
			break;
		}
		cFlag = BREAK_WOOD;
	}
	else
	{
		switch ( RANDOM_LONG(0,1) )
		{
		case 0:	
			EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "debris/bustmetal1.wav", fvol, ATTN_NORM, 0, pitch);	
			break;
		case 1:	
			EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "debris/bustmetal2.wav", fvol, ATTN_NORM, 0, pitch);	
			break;
		}
		cFlag = BREAK_METAL;
	}
    
	vecVelocity.x = 0;
	vecVelocity.y = 0;
	vecVelocity.z = 0;

	vecSpot = pev->origin + (pev->mins + pev->maxs) * 0.5;

	MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, vecSpot );
		WRITE_BYTE( TE_BREAKMODEL);

		// position
		WRITE_COORD( vecSpot.x );
		WRITE_COORD( vecSpot.y );
		WRITE_COORD( vecSpot.z );

		// size
		WRITE_COORD( pev->size.x);
		WRITE_COORD( pev->size.y);
		WRITE_COORD( pev->size.z);

		// velocity
		WRITE_COORD( vecVelocity.x ); 
		WRITE_COORD( vecVelocity.y );
		WRITE_COORD( vecVelocity.z );

		// randomization
		WRITE_BYTE( 10 ); 

		// Model
		if(pev->armortype == 3)
		{
			WRITE_SHORT( pGibName4 );	//model id#
		}
		else if(pev->armortype == 2)
		{
			WRITE_SHORT( pGibName3 );	//model id#
		}
		else if(pev->armortype == 1)
		{
			WRITE_SHORT( pGibName2 );	//model id#
		}
		else
		{
			WRITE_SHORT( pGibName );	//model id#
		}

		// # of shards
		WRITE_BYTE( 0 );	// let client decide

		// duration
		WRITE_BYTE( 100 );// 10.0 seconds

		// flags
		WRITE_BYTE( cFlag );
	MESSAGE_END();

	float size = pev->size.x;
	if ( size < pev->size.y )
		size = pev->size.y;
	if ( size < pev->size.z )
		size = pev->size.z;

	// !!! HACK  This should work!
	// Build a box above the entity that looks like an 8 pixel high sheet
	Vector mins = pev->absmin;
	Vector maxs = pev->absmax;
	mins.z = pev->absmax.z;

	if(pev->armortype == 2){
	SpawnBlood(vecSpot, BLOOD_COLOR_RED, 200);
	FX_Explosion( vecSpot, 236 );
	}

	if(pev->armortype == 0 && pev->frags != 6)
	{
		FX_Explosion( vecSpot, EXPLOSION_TRIPMINE );
		FX_Trail( vecSpot, entindex(), (UTIL_PointContents(pev->origin) == CONTENT_WATER)?PROJ_M203_DETONATE_WATER:PROJ_M203_DETONATE );
		::RadiusDamage2( vecSpot, pev, pev, 150, 400, CLASS_NONE, DMG_BLAST);

		MESSAGE_BEGIN( MSG_PAS, SVC_TEMPENTITY, vecSpot );
			WRITE_BYTE( TE_EXPLOSION);
			WRITE_COORD( vecSpot.x );
			WRITE_COORD( vecSpot.y );
			WRITE_COORD( vecSpot.z );
			WRITE_SHORT( g_sModelIndexFireball );
			WRITE_BYTE( 0 ); // no sprite
			WRITE_BYTE( 15  ); // framerate
			WRITE_BYTE( TE_EXPLFLAG_NONE );
		MESSAGE_END();
		
		maxs.z += 8;
	}
}

int CBaseDoor :: TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType )
{
	if(bitsDamageType == DMG_OPEN_DOOR)
	{
		if (m_toggle_state == TS_AT_BOTTOM || FBitSet(pev->spawnflags, SF_DOOR_NO_AUTO_RETURN) && m_toggle_state == TS_AT_TOP)
		{
			if (FStringNull ( pev->targetname ) && pevAttacker )
			{
				CBaseEntity *pEntity = GetClassPtr((CBaseEntity *)pevAttacker);
				m_hActivator = pEntity;
				DoorActivate();
				return 0;
			}
		}
	}
	
	if(pev->takedamage == DAMAGE_NO)
		return 0;
	
	if(!FClassnameIs ( pev, "func_door_breaker" ) && !FClassnameIs ( pev, "func_door_rotating_breaker" ) )
		return 0;
	
	Vector	vecTemp;

	// if Attacker == Inflictor, the attack was a melee or other instant-hit attack.
	// (that is, no actual entity projectile was involved in the attack so use the shooter's origin). 
	if ( pevAttacker == pevInflictor )	
	{
		vecTemp = pevInflictor->origin - ( pev->absmin + ( pev->size * 0.5 ) );
	}
	else
	// an actual missile was involved.
	{
		vecTemp = pevInflictor->origin - ( pev->absmin + ( pev->size * 0.5 ) );
	}
		
	pev->health -= flDamage;
	if (pev->health <= 0)
	{
		Killed( pevAttacker, GIB_NORMAL );
		Die();
		UTIL_Remove( this );
		return 0;
	}

	// Make a shard noise each time func breakable is hit.
	// Don't play shard noise if cbreakable actually died.
	int pitch;
	float fvol;
	char *rgpsz[6];
	int i;

	if (RANDOM_LONG(0,2))
	{
		pitch = PITCH_NORM;
	}
	else
	{
		pitch = 95 + RANDOM_LONG(0,34);
	}

	fvol = RANDOM_FLOAT(0.75, 1.0);

	if(pev->armortype == 3)
	{
		rgpsz[0] = "debris/glass1.wav";
		rgpsz[1] = "debris/glass2.wav";
		rgpsz[2] = "debris/glass3.wav";
	}
	else if(pev->armortype == 2)
	{
		rgpsz[0] = "debris/flesh1.wav";
		rgpsz[1] = "debris/flesh2.wav";
		rgpsz[2] = "debris/flesh3.wav";
	}
	else if(pev->armortype == 1)
	{
		rgpsz[0] = "debris/wood1.wav";
		rgpsz[1] = "debris/wood2.wav";
		rgpsz[2] = "debris/wood3.wav";
	}
	else
	{
		rgpsz[0] = "debris/metal1.wav";
		rgpsz[1] = "debris/metal3.wav";
		rgpsz[2] = "debris/metal2.wav";
	}

	i = 2;
	
	EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, rgpsz[RANDOM_LONG(0,i-1)], fvol, ATTN_NORM, 0, pitch);

	return 1;
}

void CBaseDoor::Spawn()
{
	Precache();
	SetMovedir( pev );

	if( pev->skin == 0 )
	{
		//normal door
		if( FBitSet( pev->spawnflags, SF_DOOR_PASSABLE ) )
			pev->solid = SOLID_NOT;
		else
			pev->solid = SOLID_BSP;
	}
	else
	{
		// special contents
		pev->solid = SOLID_NOT;
		SetBits( pev->spawnflags, SF_DOOR_SILENT );	// water is silent for now
	}

	pev->movetype = MOVETYPE_PUSH;
	UTIL_SetOrigin( pev, pev->origin );
	SET_MODEL( ENT( pev ), STRING( pev->model ) );

	if( pev->speed == 0.0f )
		pev->speed = 100.0f;

	m_vecPosition1 = pev->origin;

	// Subtract 2 from size because the engine expands bboxes by 1 in all directions making the size too big
	m_vecPosition2 = m_vecPosition1 + ( pev->movedir * ( fabs( pev->movedir.x * ( pev->size.x - 2.0f ) ) + fabs( pev->movedir.y * ( pev->size.y - 2.0f ) ) + fabs( pev->movedir.z * ( pev->size.z - 2.0f ) ) - m_flLip ) );
	ASSERTSZ( m_vecPosition1 != m_vecPosition2, "door start/end positions are equal" );
	if( FBitSet( pev->spawnflags, SF_DOOR_START_OPEN ) )
	{
		// swap pos1 and pos2, put door at pos2
		UTIL_SetOrigin( pev, m_vecPosition2 );
		m_vecPosition2 = m_vecPosition1;
		m_vecPosition1 = pev->origin;
	}

	m_toggle_state = TS_AT_BOTTOM;

	if ( FClassnameIs( pev, "func_door_toggle" ) )
	{
		pev->solid = SOLID_NOT;
		pev->effects |= EF_NODRAW;
	}

	if ( FClassnameIs( pev, "func_door_breaker" ) || FClassnameIs ( pev, "func_door_rotating_breaker" ) )
	{
		pev->takedamage = DAMAGE_YES;
	}

	if(pev->impulse == 1)
	{
		pev->flags |= FL_MONSTERCLIP;
	}

	// if the door is flagged for USE button activation only, use NULL touch function
	if( FBitSet( pev->spawnflags, SF_DOOR_USE_ONLY ) )
	{
		SetTouch( NULL );
	}
	else // touchable button
		SetTouch( &CBaseDoor::DoorTouch );
}
 
void CBaseDoor::SetToggleState( int state )
{
	if( state == TS_AT_TOP )
		UTIL_SetOrigin( pev, m_vecPosition2 );
	else
		UTIL_SetOrigin( pev, m_vecPosition1 );
}

void CBaseDoor::Precache( void )
{
	const char *pszSound;
	BOOL NullSound = FALSE;

	if (FClassnameIs(pev, "func_door_breaker") || FClassnameIs ( pev, "func_door_rotating_breaker" ) )
	{
		if(pev->armortype == 3)
		{
			pGibName4 = PRECACHE_MODEL("models/glassgibs.mdl");
			PRECACHE_SOUND("debris/glass1.wav");
			PRECACHE_SOUND("debris/glass2.wav");
			PRECACHE_SOUND("debris/glass3.wav");
			PRECACHE_SOUND("debris/bustglass1.wav");
			PRECACHE_SOUND("debris/bustglass2.wav");
		}
		else if(pev->armortype == 2)
		{
			pGibName3 = PRECACHE_MODEL("models/fleshgibs.mdl");
			PRECACHE_SOUND("debris/flesh1.wav");
			PRECACHE_SOUND("debris/flesh2.wav");
			PRECACHE_SOUND("debris/flesh3.wav");
			PRECACHE_SOUND("debris/bustflesh1.wav");
			PRECACHE_SOUND("debris/bustflesh2.wav");
		}
		else if(pev->armortype == 1)
		{
			pGibName2 = PRECACHE_MODEL("models/woodgibs.mdl");
			PRECACHE_SOUND("debris/wood1.wav");
			PRECACHE_SOUND("debris/wood2.wav");
			PRECACHE_SOUND("debris/wood3.wav");
			PRECACHE_SOUND("debris/bustcrate1.wav");
			PRECACHE_SOUND("debris/bustcrate2.wav");
		}
		else
		{
			pGibName = PRECACHE_MODEL("models/metalplategibs.mdl");
			PRECACHE_SOUND("debris/metal1.wav");
			PRECACHE_SOUND("debris/metal2.wav");
			PRECACHE_SOUND("debris/metal3.wav");
			PRECACHE_SOUND("debris/bustmetal1.wav");
			PRECACHE_SOUND("debris/bustmetal2.wav");
		}
	}

	// set the door's "in-motion" sound
	switch( m_bMoveSnd )
	{
		case 1:
			pszSound = "doors/doormove1.wav";
			break;
		case 2:
			pszSound = "doors/doormove2.wav";
			break;
		case 3:
			pszSound = "doors/doormove3.wav";
			break;
		case 4:
			pszSound = "doors/doormove4.wav";
			break;
		case 5:
			pszSound = "doors/doormove5.wav";
			break;
		case 6:
			pszSound = "doors/doormove6.wav";
			break;
		case 7:
			pszSound = "doors/doormove7.wav";
			break;
		case 8:
			pszSound = "doors/doormove8.wav";
			break;
		case 9:
			pszSound = "doors/doormove9.wav";
			break;
		case 10:
			pszSound = "doors/doormove10.wav";
			break;
		case 11:
			pszSound = "doors/doormove11.wav";
			break;
		case 12:
			pszSound = "doors/doormove12.wav";
			break;
		case 13:
			pszSound = "doors/doormove13.wav";
			break;
		case 14:
			pszSound = "doors/doortrain_move.wav";
			break;
		case 0:
		default:
			pszSound = "common/null.wav";
			NullSound = TRUE;
			break;
	}

	if( !NullSound )
		PRECACHE_SOUND( pszSound );
	pev->noiseMoving = MAKE_STRING( pszSound );
	NullSound = FALSE;

	// set the door's 'reached destination' stop sound
	switch( m_bStopSnd )
	{
		case 1:
			pszSound = "doors/doorstop1.wav";
			break;
		case 2:
			pszSound = "doors/doorstop2.wav";
			break;
		case 3:
			pszSound = "doors/doorstop3.wav";
			break;
		case 4:
			pszSound = "doors/doorstop4.wav";
			break;
		case 5:
			pszSound = "doors/doorstop5.wav";
			break;
		case 6:
			pszSound = "doors/doorstop6.wav";
			break;
		case 7:
			pszSound = "doors/doorstop7.wav";
			break;
		case 8:
			pszSound = "doors/doorstop8.wav";
			break;
		case 0:
		default:
			pszSound = "common/null.wav";
			NullSound = TRUE;
			break;
	}

	if( !NullSound )
		PRECACHE_SOUND( pszSound );
	pev->noiseArrived = MAKE_STRING( pszSound );

	// get door button sounds, for doors which are directly 'touched' to open
	if( m_bLockedSound )
	{
		pszSound = ButtonSound( (int)m_bLockedSound );
		PRECACHE_SOUND( pszSound );
		m_ls.sLockedSound = MAKE_STRING( pszSound );
	}

	if( m_bUnlockedSound )
	{
		pszSound = ButtonSound( (int)m_bUnlockedSound );
		PRECACHE_SOUND( pszSound );
		m_ls.sUnlockedSound = MAKE_STRING( pszSound );
	}

	// get sentence group names, for doors which are directly 'touched' to open
	switch( m_bLockedSentence )
	{
		case 1:
			// access denied
			m_ls.sLockedSentence = MAKE_STRING( "NA" );
			break;
		case 2:
			// security lockout
			m_ls.sLockedSentence = MAKE_STRING( "ND" );
			break;
		case 3:
			// blast door
			m_ls.sLockedSentence = MAKE_STRING( "NF" );
			break;
		case 4:
			// fire door
			m_ls.sLockedSentence = MAKE_STRING( "NFIRE" );
			break;
		case 5:
			// chemical door
			m_ls.sLockedSentence = MAKE_STRING( "NCHEM" );
			break;
		case 6:
			// radiation door
			m_ls.sLockedSentence = MAKE_STRING( "NRAD" );
			break;
		case 7:
			// gen containment
			m_ls.sLockedSentence = MAKE_STRING( "NCON" );
			break;
		case 8:
			// maintenance door
			m_ls.sLockedSentence = MAKE_STRING( "NH" );
			break;
		case 9:
			// broken door
			m_ls.sLockedSentence = MAKE_STRING( "NG" );
			break;
		default:
			m_ls.sLockedSentence = 0;
			break;
	}

	switch( m_bUnlockedSentence )
	{
		case 1:
			// access granted
			m_ls.sUnlockedSentence = MAKE_STRING( "EA" );
			break;
		case 2:
			// security door
			m_ls.sUnlockedSentence = MAKE_STRING( "ED" );
			break;
		case 3:
			// blast door
			m_ls.sUnlockedSentence = MAKE_STRING( "EF" );
			break;
		case 4:
			// fire door
			m_ls.sUnlockedSentence = MAKE_STRING( "EFIRE" );
			break;
		case 5:
			// chemical door
			m_ls.sUnlockedSentence = MAKE_STRING( "ECHEM" );
			break;
		case 6:
			// radiation door
			m_ls.sUnlockedSentence = MAKE_STRING( "ERAD" );
			break;
		case 7:
			// gen containment
			m_ls.sUnlockedSentence = MAKE_STRING( "ECON" );
			break;
		case 8:
			// maintenance door
			m_ls.sUnlockedSentence = MAKE_STRING( "EH" );
			break;
		default:
			m_ls.sUnlockedSentence = 0;
			break;
	}
}

//
// Doors not tied to anything (e.g. button, another door) can be touched, to make them activate.
//
void CBaseDoor::DoorTouch( CBaseEntity *pOther )
{
	if(pev->frags > 0)
	{
		if ( !(pOther->pev->flags & FL_CLIENT) )
			return;
	}

	entvars_t*	pevToucher = pOther->pev;

	if(pev->frags == 1)
	{
		CBasePlayer *pPlayer = GetClassPtr((CBasePlayer *)pevToucher);
		if(pPlayer)
		{
			if(pPlayer->m_fGlodenKey)
			{
				pPlayer->m_fGlodenKey -= 1;
				pPlayer->MenuItem_remove(3);
				m_hActivator = pOther;// remember who activated the door

				if (DoorActivate( ))
					SetTouch( NULL ); // Temporarily disable the touch function, until movement is finished.

				pev->frags = 0;
			}
			else
			{
				UTIL_CenterPrintAll( "Need Gold Keys" );
				PlayLockSounds(pev, &m_ls, TRUE, FALSE);
			}
			return;
		}
	}
	else if(pev->frags == 2)
	{
		CBasePlayer *pPlayer = GetClassPtr((CBasePlayer *)pevToucher);
		if(pPlayer)
		{
			CBaseEntity *pEvent = Create( "main_cg_event_new", pevToucher->origin, Vector(0,180,0), NULL );
			pEvent->pev->armortype = 5;
			pev->frags = 0;
			return;
		}
	}
	else if(pev->frags == 3)
	{
		CBasePlayer *pPlayer = GetClassPtr((CBasePlayer *)pevToucher);
		if(pPlayer)
		{
			if(pPlayer->m_fGenerenKey)
			{
				pPlayer->m_fGenerenKey = FALSE;
				pPlayer->MenuItem_remove(4);
				m_hActivator = pOther;// remember who activated the door

				if (DoorActivate( ))
					SetTouch( NULL ); // Temporarily disable the touch function, until movement is finished.

				pev->frags = 0;
			}
			else
			{
				UTIL_CenterPrintAll( "Need Generic Keys" );
				PlayLockSounds(pev, &m_ls, TRUE, FALSE);
			}
			return;
		}
	}
	else if(pev->frags == 4)
	{
		CBasePlayer *pPlayer = GetClassPtr((CBasePlayer *)pevToucher);
		if(pPlayer)
		{
			if( g_restore_fix <= 0)
			{

				char text[256];
				sprintf( text, "- You passed the Training room!\n");
				UTIL_SayTextAll( text,this );

				pPlayer->EnableControl(FALSE);
				pPlayer->m_trainning = 1;
				UTIL_ScreenFade( pPlayer, Vector(0,0,0), 4.0, 6.0, 255, FFADE_OUT );
				pPlayer->m_iClient_Gameover = 3;
				pPlayer->m_fGameOverTime = gpGlobals->time + 6;

				pev->frags = 0;
			}
			return;
		}
	}
	else if(pev->frags == 5)
	{
		CBasePlayer *pPlayer = GetClassPtr((CBasePlayer *)pevToucher);
		if(pPlayer)
		{
			if(pPlayer->m_fSecurityKey)
			{
				pPlayer->m_fSecurityKey = FALSE;
				pPlayer->MenuItem_remove(5);
				m_hActivator = pOther;// remember who activated the door

				if (DoorActivate( ))
					SetTouch( NULL ); // Temporarily disable the touch function, until movement is finished.

				pev->frags = 0;
			}
			else
			{
				UTIL_CenterPrintAll( "Need Security Keys" );
				PlayLockSounds(pev, &m_ls, TRUE, FALSE);
			}
			return;
		}
	}
	else if(pev->frags == 6)
	{
		return;
	}
	/*
	else if(pev->frags == 7){
		CBasePlayer *pPlayer = GetClassPtr((CBasePlayer *)pevToucher);
		if(pPlayer)
		{
			if(pPlayer->m_fControlKey)
			{
				pPlayer->m_fControlKey = FALSE;
				m_hActivator = pOther;// remember who activated the door

				if (DoorActivate( ))
					SetTouch( NULL ); // Temporarily disable the touch function, until movement is finished.

				pev->frags = 0;
			}
			else
			{
				UTIL_CenterPrintAll( "Need Control Room Keys" );
				PlayLockSounds(pev, &m_ls, TRUE, FALSE);
			}
			return;
		}
	}*/
	else if(pev->frags == 8)
	{
		if(!FStringNull(pev->target))
		{
			CBasePlayer *pPlayer = GetClassPtr((CBasePlayer *)pevToucher);
			if(pPlayer)
			{
				SUB_UseTargets( pPlayer, USE_TOGGLE, 0 );
				pev->target = 0;
			}
		}
		return;
	}
	else if(pev->frags == 9)
	{
		CBasePlayer *pPlayer = GetClassPtr((CBasePlayer *)pevToucher);
		if(pPlayer)
		{
			pPlayer->m_trainning = 2;
			pev->frags = 0;
			return;
		}
	}
	else if(pev->frags == 10)
	{
		CBasePlayer *pPlayer = GetClassPtr((CBasePlayer *)pevToucher);
		if(pPlayer)
		{
			CBaseEntity *pEvent = Create( "main_cg_event_new4", pevToucher->origin, Vector(0,180,0), NULL );
			pEvent->pev->armortype = 52;
			pev->frags = 0;
			return;
		}
	}
	else if(pev->frags == 11)
	{
		CBasePlayer *pPlayer = GetClassPtr((CBasePlayer *)pevToucher);
		if(pPlayer)
		{
			CBaseEntity *pEvent = Create( "main_cg_event_new4", pevToucher->origin, Vector(0,180,0), NULL );
			pEvent->pev->armortype = 53;
			pev->frags = 0;
			return;
		}
	}
	else if(pev->frags == 12)
	{
		CBasePlayer *pPlayer = GetClassPtr((CBasePlayer *)pevToucher);
		if(pPlayer)
		{
			CBaseEntity *pEvent = Create( "main_cg_event_new6", pevToucher->origin, g_vecZero, NULL );
			pEvent->pev->armortype = 74;
			pev->frags = 0;
			return;
		}
	}
	else if(pev->frags == 13)
	{
		CBasePlayer *pPlayer = GetClassPtr((CBasePlayer *)pevToucher);
		if(pPlayer)
		{
			CBaseEntity *pEvent = Create( "main_cg_event_new6", pevToucher->origin, g_vecZero, NULL );
			pEvent->pev->armortype = 80;
			pev->frags = 0;
			return;
		}
	}
	else if(pev->frags == 14)
	{
		CBasePlayer *pPlayer = GetClassPtr((CBasePlayer *)pevToucher);
		if(pPlayer)
		{
			if( g_restore_fix <= 0)
			{
				pPlayer->EnableControl(FALSE);
				pPlayer->m_trainning = 1;
				UTIL_ScreenFade( pPlayer, Vector(0,0,0), 3.0, 3.0, 255, FFADE_OUT );
				pPlayer->m_iClient_Gameover = 3;
				pPlayer->m_fGameOverTime = gpGlobals->time + 4;

				pev->frags = 0;
			}
		}
		return;
	}
	else if(pev->frags == 15)
	{
		CBasePlayer *pPlayer = GetClassPtr((CBasePlayer *)pevToucher);
		if(pPlayer)
		{
			edict_t* pentTarget	= NULL;
			pentTarget = FIND_ENTITY_BY_TARGETNAME( pentTarget, STRING(pev->target) );
			if (FNullEnt(pentTarget))
				return;	
			
			Vector tmp = VARS( pentTarget )->origin;

			tmp.z -= pPlayer->pev->mins.z;// make origin adjustments in case the teleportee is a player. (origin in center, not at feet)

			tmp.z++;

			pevToucher->flags &= ~FL_ONGROUND;
			
			UTIL_SetOrigin( pevToucher, tmp );

			pevToucher->angles = pentTarget->v.angles;

			UTIL_ScreenFade( pPlayer, Vector(0,0,0), 0.5, 0.5, 255, FFADE_IN );

			pevToucher->fixangle = TRUE;
		}
		return;
	}
	else if(pev->frags == 16)
	{
		SERVER_COMMAND( "map wdoor_boss_rush\n" );
		pev->frags = 0;
		return;
	}
	else if(pev->frags == 17)
	{
		SERVER_COMMAND( "map wdoor_headcrab_ball\n" );
		pev->frags = 0;
		return;
	}

	// Ignore touches by anything but players
	if( !pOther->IsPlayer() )
	{
		if ( pevToucher->flags & FL_MONSTER )
		{
			CBaseMonster *pEnemyMonster;
			pEnemyMonster = pOther->MyMonsterPointer();
			if(pEnemyMonster)
			{
				if(pEnemyMonster->m_forcefuckdoor == TRUE)
				{
					goto monster_opendoor;
				}
			}
		}
		return;
	}
	monster_opendoor:

	// If door has master, and it's not ready to trigger, 
	// play 'locked' sound
	if( m_sMaster && !UTIL_IsMasterTriggered( m_sMaster, pOther ) )
		PlayLockSounds( pev, &m_ls, TRUE, FALSE );

	// If door is somebody's target, then touching does nothing.
	// You have to activate the owner (e.g. button).
	if( !FStringNull( pev->targetname ) )
	{
		// play locked sound
		PlayLockSounds( pev, &m_ls, TRUE, FALSE );
		return; 
	}

	m_hActivator = pOther;// remember who activated the door

	if( DoorActivate() )
		SetTouch( NULL ); // Temporarily disable the touch function, until movement is finished.
}

//
// Used by SUB_UseTargets, when a door is the target of a button.
//
void CBaseDoor::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	m_hActivator = pActivator;
	// if not ready to be used, ignore "use" command.
	if( m_toggle_state == TS_AT_BOTTOM || ( FBitSet( pev->spawnflags, SF_DOOR_NO_AUTO_RETURN ) && m_toggle_state == TS_AT_TOP ) )
		DoorActivate();
}

//
// Causes the door to "do its thing", i.e. start moving, and cascade activation.
//
int CBaseDoor::DoorActivate()
{
	if( !UTIL_IsMasterTriggered( m_sMaster, m_hActivator ) )
		return 0;

	if( FClassnameIs( pev, "func_door_toggle" ) )
	{
		pev->solid = SOLID_BSP;
		pev->effects &= ~EF_NODRAW;
	}

	if( FBitSet( pev->spawnflags, SF_DOOR_NO_AUTO_RETURN ) && m_toggle_state == TS_AT_TOP )
	{
		// door should close
		DoorGoDown();
	}
	else
	{
		// door should open
		if( m_hActivator != 0 && m_hActivator->IsPlayer() )
		{
			// give health if player opened the door (medikit)
			//VARS( m_eoActivator )->health += m_bHealthValue;

			m_hActivator->TakeHealth( m_bHealthValue, DMG_GENERIC );
		}

		// play door unlock sounds
		PlayLockSounds( pev, &m_ls, FALSE, FALSE );

		DoorGoUp();
	}

	return 1;
}

extern Vector VecBModelOrigin( entvars_t* pevBModel );

//
// Starts the door going to its "up" position (simply ToggleData->vecPosition2).
//
void CBaseDoor::DoorGoUp( void )
{
	entvars_t *pevActivator;

	// It could be going-down, if blocked.
	ASSERT( m_toggle_state == TS_AT_BOTTOM || m_toggle_state == TS_GOING_DOWN );

	// emit door moving and stop sounds on CHAN_STATIC so that the multicast doesn't
	// filter them out and leave a client stuck with looping door sounds!
	if( !FBitSet( pev->spawnflags, SF_DOOR_SILENT ) )
		if( m_toggle_state != TS_GOING_UP && m_toggle_state != TS_GOING_DOWN )
			EMIT_SOUND( ENT( pev ), CHAN_STATIC, STRING( pev->noiseMoving ), 1, ATTN_NORM );

	m_toggle_state = TS_GOING_UP;

	SetMoveDone( &CBaseDoor::DoorHitTop );
	if( FClassnameIs( pev, "func_door_rotating" ) || FClassnameIs(pev, "func_door_rotating_breaker") )		// !!! BUGBUG Triggered doors don't work with this yet
	{
		float sign = 1.0f;

		if( m_hActivator != 0 )
		{
			pevActivator = m_hActivator->pev;

			if( !FBitSet( pev->spawnflags, SF_DOOR_ONEWAY ) && pev->movedir.y ) 		// Y axis rotation, move away from the player
			{
				Vector vec = pevActivator->origin - pev->origin;
				Vector angles = pevActivator->angles;
				angles.x = 0.0f;
				angles.z = 0.0f;
				UTIL_MakeVectors( angles );
				//Vector vnext = ( pevToucher->origin + ( pevToucher->velocity * 10.f ) ) - pev->origin;
				UTIL_MakeVectors( pevActivator->angles );
				Vector vnext = ( pevActivator->origin + ( gpGlobals->v_forward * 10.f ) ) - pev->origin;
				if( ( vec.x * vnext.y - vec.y * vnext.x ) < 0.0f )
					sign = -1.0f;
			}
		}
		AngularMove( m_vecAngle2*sign, pev->speed );
	}
	else
		LinearMove( m_vecPosition2, pev->speed );
}

//
// The door has reached the "up" position.  Either go back down, or wait for another activation.
//
void CBaseDoor::DoorHitTop( void )
{
	if( FClassnameIs( pev, "func_door_toggle" ) )
	{
		pev->solid = SOLID_NOT;
		pev->effects |= EF_NODRAW;
	}

	if( !FBitSet( pev->spawnflags, SF_DOOR_SILENT ) )
	{
		STOP_SOUND( ENT( pev ), CHAN_STATIC, STRING( pev->noiseMoving ) );
		EMIT_SOUND( ENT( pev ), CHAN_STATIC, STRING( pev->noiseArrived ), 1.0f, ATTN_NORM );
	}

	ASSERT( m_toggle_state == TS_GOING_UP );
	m_toggle_state = TS_AT_TOP;

	// toggle-doors don't come down automatically, they wait for refire.
	if( FBitSet( pev->spawnflags, SF_DOOR_NO_AUTO_RETURN ) )
	{
		// Re-instate touch method, movement is complete
		if( !FBitSet( pev->spawnflags, SF_DOOR_USE_ONLY ) )
			SetTouch( &CBaseDoor::DoorTouch );
	}
	else
	{
		// In flWait seconds, DoorGoDown will fire, unless wait is -1, then door stays open
		pev->nextthink = pev->ltime + m_flWait;
		SetThink( &CBaseDoor::DoorGoDown );

		if( m_flWait == -1.0f )
		{
			pev->nextthink = -1.0f;
		}
	}

	// Fire the close target (if startopen is set, then "top" is closed) - netname is the close target
	if( pev->netname && ( pev->spawnflags & SF_DOOR_START_OPEN ) )
		FireTargets( STRING( pev->netname ), m_hActivator, this, USE_TOGGLE, 0 );

	SUB_UseTargets( m_hActivator, USE_TOGGLE, 0 ); // this isn't finished
}

//
// Starts the door going to its "down" position (simply ToggleData->vecPosition1).
//
void CBaseDoor::DoorGoDown( void )
{
	if( !FBitSet( pev->spawnflags, SF_DOOR_SILENT ) )
		if( m_toggle_state != TS_GOING_UP && m_toggle_state != TS_GOING_DOWN )
			EMIT_SOUND( ENT( pev ), CHAN_STATIC, STRING( pev->noiseMoving ), 1.0f, ATTN_NORM );	
#if DOOR_ASSERT
	ASSERT( m_toggle_state == TS_AT_TOP );
#endif // DOOR_ASSERT
	m_toggle_state = TS_GOING_DOWN;

	SetMoveDone( &CBaseDoor::DoorHitBottom );
	float slowsp = pev->speed;
	if(pev->armorvalue == 3)
	{
		slowsp = pev->speed * 0.25;
	}
	if( FClassnameIs( pev, "func_door_rotating" ) || FClassnameIs(pev, "func_door_rotating_breaker"))//rotating door
		AngularMove( m_vecAngle1, slowsp );
	else
		LinearMove( m_vecPosition1, slowsp);
}

//
// The door has reached the "down" position.  Back to quiescence.
//
void CBaseDoor::DoorHitBottom( void )
{
	if(pev->armorvalue == 3)
	{
		DoorGoUp();
		return;
	}

	if ( FClassnameIs( pev, "func_door_toggle" ) )
	{
		pev->solid = SOLID_NOT;
		pev->effects |= EF_NODRAW;
	}

	if( !FBitSet( pev->spawnflags, SF_DOOR_SILENT ) )
	{
		STOP_SOUND( ENT( pev ), CHAN_STATIC, STRING( pev->noiseMoving ) );
		EMIT_SOUND( ENT( pev ), CHAN_STATIC, STRING( pev->noiseArrived ), 1.0f, ATTN_NORM );
	}

	ASSERT( m_toggle_state == TS_GOING_DOWN );
	m_toggle_state = TS_AT_BOTTOM;

	// Re-instate touch method, cycle is complete
	if( FBitSet( pev->spawnflags, SF_DOOR_USE_ONLY ) )
	{
		// use only door
		SetTouch( NULL );
	}
	else // touchable door
		SetTouch( &CBaseDoor::DoorTouch );

	SUB_UseTargets( m_hActivator, USE_TOGGLE, 0 ); // this isn't finished

	// Fire the close target (if startopen is set, then "top" is closed) - netname is the close target
	if( pev->netname && !( pev->spawnflags & SF_DOOR_START_OPEN ) )
		FireTargets( STRING( pev->netname ), m_hActivator, this, USE_TOGGLE, 0 );
}

void CBaseDoor::Blocked( CBaseEntity *pOther )
{
	edict_t	*pentTarget = NULL;
	CBaseDoor *pDoor = NULL;

	// Hurt the blocker a little.
	if(pev->armorvalue == 3 || pev->armorvalue == 4)
	{
		if(pOther->pev->health > 1)
		{
			pOther->pev->health = 1;
		}
		pOther->Killed( pev, GIB_ALWAYS );
		return;
	}
		
	if(pOther->pev->deadflag != DEAD_NO)
	{
		pOther->TakeDamage( pev, pev, 300, DMG_CRUSH | GIB_ALWAYS );
	}

	if(pev->armorvalue == 5)
	{
		if(pOther->Classify() == CLASS_ALIEN_MONSTER)
		{
			pOther->TakeDamage( pev, pev, 300, DMG_CRUSH | GIB_ALWAYS );
		}
	}

	if( pev->dmg )
		pOther->TakeDamage( pev, pev, pev->dmg, DMG_CRUSH );

	if( satchelfix.value )
	{
		// Detonate satchels
		if( !strcmp( "monster_satchel", STRING( pOther->pev->classname ) ) )
			( (CSatchel*)pOther )->Use( this, this, USE_ON, 0 );
	}

	// if a door has a negative wait, it would never come back if blocked,
	// so let it just squash the object to death real fast
	if(pev->armorvalue != 1 && pev->armorvalue != 3 && pev->armorvalue != 5) 
	{
		if( m_flWait >= 0.0f )
		{
			// BMod Start - Door sound fix.
			if( !FBitSet( pev->spawnflags, SF_DOOR_SILENT ) )
				STOP_SOUND( ENT( pev ), CHAN_STATIC, STRING( pev->noiseMoving ) );
			// BMod End

			if( m_toggle_state == TS_GOING_DOWN )
			{
				DoorGoUp();
			}
			else
			{
				DoorGoDown();
			}
		}
	}

	// Block all door pieces with the same targetname here.
	if( !FStringNull( pev->targetname ) )
	{
		for( ; ; )
		{
			pentTarget = FIND_ENTITY_BY_TARGETNAME( pentTarget, STRING( pev->targetname ) );

			if( VARS( pentTarget ) != pev )
			{
				if( FNullEnt( pentTarget ) )
					break;

				if( FClassnameIs( pentTarget, "func_door" ) || FClassnameIs( pentTarget, "func_door_rotating" ) )
				{
					pDoor = GetClassPtr( (CBaseDoor *)VARS( pentTarget ) );

					if( pDoor->m_flWait >= 0.0f )
					{
						if( pDoor->pev->velocity == pev->velocity && pDoor->pev->avelocity == pev->velocity )
						{
							// this is the most hacked, evil, bastardized thing I've ever seen. kjb
							if( FClassnameIs( pentTarget, "func_door" ) )
							{
								// set origin to realign normal doors
								pDoor->pev->origin = pev->origin;
								pDoor->pev->velocity = g_vecZero;// stop!
							}
							else
							{
								// set angles to realign rotating doors
								pDoor->pev->angles = pev->angles;
								pDoor->pev->avelocity = g_vecZero;
							}
						}

						if(pev->armorvalue != 1)
						{
							if( !FBitSet( pev->spawnflags, SF_DOOR_SILENT ) )
								STOP_SOUND( ENT( pev ), CHAN_STATIC, STRING( pev->noiseMoving ) );

							if( pDoor->m_toggle_state == TS_GOING_DOWN )
								pDoor->DoorGoUp();
							else
								pDoor->DoorGoDown();
						}
					}
				}
			}
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
	void Spawn( void );
	virtual void SetToggleState( int state );
};

LINK_ENTITY_TO_CLASS( func_door_rotating, CRotDoor );
LINK_ENTITY_TO_CLASS( func_door_rotating_breaker, CRotDoor );

void CRotDoor::Spawn( void )
{
	Precache();
	// set the axis of rotation
	CBaseToggle::AxisDir( pev );

	// check for clockwise rotation
	if( FBitSet( pev->spawnflags, SF_DOOR_ROTATE_BACKWARDS ) )
		pev->movedir = pev->movedir * -1.0f;

	//m_flWait = 2; who the hell did this? (sjb)
	m_vecAngle1 = pev->angles;
	m_vecAngle2 = pev->angles + pev->movedir * m_flMoveDistance;

	ASSERTSZ( m_vecAngle1 != m_vecAngle2, "rotating door start/end positions are equal" );

	if( FBitSet( pev->spawnflags, SF_DOOR_PASSABLE ) )
		pev->solid = SOLID_NOT;
	else
		pev->solid = SOLID_BSP;

	pev->movetype = MOVETYPE_PUSH;
	UTIL_SetOrigin( pev, pev->origin );
	SET_MODEL( ENT( pev ), STRING( pev->model ) );

	if( pev->speed == 0.0f )
		pev->speed = 100.0f;

	// DOOR_START_OPEN is to allow an entity to be lighted in the closed position
	// but spawn in the open position
	if( FBitSet( pev->spawnflags, SF_DOOR_START_OPEN ) )
	{	
		// swap pos1 and pos2, put door at pos2, invert movement direction
		pev->angles = m_vecAngle2;
		Vector vecSav = m_vecAngle1;
		m_vecAngle2 = m_vecAngle1;
		m_vecAngle1 = vecSav;
		pev->movedir = pev->movedir * -1.0f;
	}

	m_toggle_state = TS_AT_BOTTOM;

	if( FClassnameIs ( pev, "func_door_rotating_breaker" ) )
	{
		pev->takedamage = DAMAGE_YES;
	}

	if( FBitSet( pev->spawnflags, SF_DOOR_USE_ONLY ) )
	{
		SetTouch( NULL );
	}
	else // touchable button
		SetTouch( &CBaseDoor::DoorTouch );
}

void CRotDoor::SetToggleState( int state )
{
	if( state == TS_AT_TOP )
		pev->angles = m_vecAngle2;
	else
		pev->angles = m_vecAngle1;

	UTIL_SetOrigin( pev, pev->origin );
}

class CMomentaryDoor : public CBaseToggle
{
public:
	void Spawn( void );
	void Precache( void );
	void EXPORT MomentaryMoveDone( void );
	void EXPORT StopMoveSound( void );

	void KeyValue( KeyValueData *pkvd );
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	virtual int ObjectCaps( void ) { return CBaseToggle::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }

	virtual int Save( CSave &save );
	virtual int Restore( CRestore &restore );
	static TYPEDESCRIPTION m_SaveData[];

	BYTE m_bMoveSnd;			// sound a door makes while moving	
	BYTE m_bStopSnd;			// sound a door makes when it stops
};

LINK_ENTITY_TO_CLASS( momentary_door, CMomentaryDoor )

TYPEDESCRIPTION	CMomentaryDoor::m_SaveData[] =
{
	DEFINE_FIELD( CMomentaryDoor, m_bMoveSnd, FIELD_CHARACTER ),
	DEFINE_FIELD( CMomentaryDoor, m_bStopSnd, FIELD_CHARACTER ),
};

IMPLEMENT_SAVERESTORE( CMomentaryDoor, CBaseToggle )

void CMomentaryDoor::Spawn( void )
{
	SetMovedir( pev );

	pev->solid = SOLID_BSP;
	pev->movetype = MOVETYPE_PUSH;

	UTIL_SetOrigin( pev, pev->origin );
	SET_MODEL( ENT( pev ), STRING( pev->model ) );

	if( pev->speed == 0.0f )
		pev->speed = 100.0f;
	if( pev->dmg == 0.0f )
		pev->dmg = 2.0f;

	m_vecPosition1 = pev->origin;
	// Subtract 2 from size because the engine expands bboxes by 1 in all directions making the size too big
	m_vecPosition2 = m_vecPosition1 + ( pev->movedir * ( fabs( pev->movedir.x * ( pev->size.x - 2.0f ) ) + fabs( pev->movedir.y * ( pev->size.y - 2.0f ) ) + fabs( pev->movedir.z * ( pev->size.z - 2.0f ) ) - m_flLip ) );
	ASSERTSZ( m_vecPosition1 != m_vecPosition2, "door start/end positions are equal" );

	if( FBitSet( pev->spawnflags, SF_DOOR_START_OPEN ) )
	{	// swap pos1 and pos2, put door at pos2
		UTIL_SetOrigin( pev, m_vecPosition2 );
		m_vecPosition2 = m_vecPosition1;
		m_vecPosition1 = pev->origin;
	}
	SetTouch( NULL );

	Precache();
}

void CMomentaryDoor::Precache( void )
{
	const char *pszSound;
	BOOL NullSound = FALSE;

	// set the door's "in-motion" sound
	switch( m_bMoveSnd )
	{
	case 1:
		pszSound = "doors/doormove1.wav";
		break;
	case 2:
		pszSound = "doors/doormove2.wav";
		break;
	case 3:
		pszSound = "doors/doormove3.wav";
		break;
	case 4:
		pszSound = "doors/doormove4.wav";
		break;
	case 5:
		pszSound = "doors/doormove5.wav";
		break;
	case 6:
		pszSound = "doors/doormove6.wav";
		break;
	case 7:
		pszSound = "doors/doormove7.wav";
		break;
	case 8:
		pszSound = "doors/doormove8.wav";
		break;
	case 0:
	default:
		pszSound = "common/null.wav";
		NullSound = TRUE;
		break;
	}

	if( !NullSound )
		PRECACHE_SOUND( pszSound );
	pev->noiseMoving = MAKE_STRING( pszSound );
	NullSound = FALSE;

	// set the door's 'reached destination' stop sound
	switch( m_bStopSnd )
	{
	case 1:
		pszSound = "doors/doorstop1.wav";
		break;
	case 2:
		pszSound = "doors/doorstop2.wav";
		break;
	case 3:
		pszSound = "doors/doorstop3.wav";
		break;
	case 4:
		pszSound = "doors/doorstop4.wav";
		break;
	case 5:
		pszSound = "doors/doorstop5.wav";
		break;
	case 6:
		pszSound = "doors/doorstop6.wav";
		break;
	case 7:
		pszSound = "doors/doorstop7.wav";
		break;
	case 8:
		pszSound = "doors/doorstop8.wav";
		break;
	case 0:
	default:
		pszSound = "common/null.wav";
		NullSound = TRUE;
		break;
	}

	if( !NullSound )
		PRECACHE_SOUND( pszSound );
	pev->noiseArrived = MAKE_STRING( pszSound );
}

void CMomentaryDoor::KeyValue( KeyValueData *pkvd )
{

	if( FStrEq( pkvd->szKeyName, "movesnd" ) )
	{
		m_bMoveSnd = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "stopsnd" ) )
	{
		m_bStopSnd = atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "healthvalue" ) )
	{
		//m_bHealthValue = atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
		CBaseToggle::KeyValue( pkvd );
}

void CMomentaryDoor::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if( useType != USE_SET )		// Momentary buttons will pass down a float in here
		return;

	if( value > 1.0f )
		value = 1.0f;
	if( value < 0.0f )
		value = 0.0f;

	Vector move = m_vecPosition1 + ( value * ( m_vecPosition2 - m_vecPosition1 ) );
	
	Vector delta = move - pev->origin;
	//float speed = delta.Length() * 10.0f;
	float speed = delta.Length() / 0.1f; // move there in 0.1 sec

	if( speed != 0 )
	{
		// This entity only thinks when it moves, so if it's thinking, it's in the process of moving
		// play the sound when it starts moving(not yet thinking)
		if( pev->nextthink < pev->ltime || pev->nextthink == 0.0f )
			EMIT_SOUND( ENT( pev ), CHAN_STATIC, STRING( pev->noiseMoving ), 1.0f, ATTN_NORM );
		// If we already moving to designated point, return
		else if( move == m_vecFinalDest )
			return;

		LinearMove( move, speed );
		SetMoveDone( &CMomentaryDoor::MomentaryMoveDone );
	}
}

void CMomentaryDoor::MomentaryMoveDone( void )
{
	SetThink(&CMomentaryDoor::StopMoveSound);
	pev->nextthink = pev->ltime + 0.1f;
}

void CMomentaryDoor::StopMoveSound()
{
	STOP_SOUND( ENT( pev ), CHAN_STATIC, STRING( pev->noiseMoving ) );
	EMIT_SOUND( ENT( pev ), CHAN_STATIC, STRING( pev->noiseArrived ), 1.0f, ATTN_NORM );
	pev->nextthink = -1.0f;
	ResetThink();
}
