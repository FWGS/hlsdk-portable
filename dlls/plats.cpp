/***
*
*	Copyright (c) 1999, 2000 Valve LLC. All rights reserved.
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

===== plats.cpp ========================================================

  spawn, think, and touch functions for trains, etc

*/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "trains.h"
#include "saverestore.h"
#include "movewith.h"

static void PlatSpawnInsideTrigger(entvars_t* pevPlatform);

// from mathlib.h
// BUG BUG This is declared in pm_math.cpp, Linux will spit about it
// No idea why windows does not pick that up, in fact
// MSVC will complain it's not delcared if you use extern...
#if 0
#ifdef _WIN32
int nanmask = 255<<23;
#else
extern int nanmask;
#endif
#endif
#define nanmask (int)(255<<23)

#define	IS_NAN(x) (((*(int *)&x)&nanmask)==nanmask)

static float Fix( float angle )
{
	if ( IS_NAN(angle) )
	{
		ALERT(at_debug, "NaN error during Fix!\n");
		return angle;
	}
	while ( angle < 0 )
		angle += 360;
	while ( angle > 360 )
		angle -= 360;

	return angle;
}


static void FixupAngles( Vector &v )
{
	v.x = Fix( v.x );
	v.y = Fix( v.y );
	v.z = Fix( v.z );
}

class CFuncTrain;

//LRC - scripted_trainsequence
class CTrainSequence : public CBaseEntity
{
public:
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void EndThink( void );
	void TimeOutThink( void );
	void	KeyValue( KeyValueData *pkvd );
	STATE	GetState( void ) { return (m_pTrain||m_pTrackTrain)?STATE_ON:STATE_OFF; }
	virtual int	ObjectCaps( void );

	void StopSequence( void );
	void ArrivalNotify( void );

	virtual int	Save( CSave &save );
	virtual int	Restore( CRestore &restore );
	static TYPEDESCRIPTION m_SaveData[];

	string_t m_iszEntity;
	string_t m_iszDestination;
	string_t m_iszTerminate;
	int m_iDirection;
	int m_iPostDirection;
	float m_fDuration;
// at any given time, at most one of these pointers will be set.
	CFuncTrain *m_pTrain;
	CFuncTrackTrain *m_pTrackTrain;

	CBaseEntity *m_pDestination;
};



#define SF_PLAT_TOGGLE		0x0001

class CBasePlatTrain : public CBaseToggle
{
public:
	virtual int	ObjectCaps( void ) { return CBaseEntity :: ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }
	void KeyValue( KeyValueData* pkvd);
	void Precache( void );

	// This is done to fix spawn flag collisions between this class and a derived class
	virtual BOOL IsTogglePlat( void ) { return (pev->spawnflags & SF_PLAT_TOGGLE) ? TRUE : FALSE; }

	virtual int	Save( CSave &save );
	virtual int	Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	BYTE	m_bMoveSnd;			// sound a plat makes while moving
	BYTE	m_bStopSnd;			// sound a plat makes when it stops
	float	m_volume;			// Sound volume
};

TYPEDESCRIPTION	CBasePlatTrain::m_SaveData[] = 
{
	DEFINE_FIELD( CBasePlatTrain, m_bMoveSnd, FIELD_CHARACTER ),
	DEFINE_FIELD( CBasePlatTrain, m_bStopSnd, FIELD_CHARACTER ),
	DEFINE_FIELD( CBasePlatTrain, m_volume, FIELD_FLOAT ),
};

IMPLEMENT_SAVERESTORE( CBasePlatTrain, CBaseToggle );

void CBasePlatTrain :: KeyValue( KeyValueData *pkvd )
{
	if (FStrEq(pkvd->szKeyName, "lip"))
	{
		m_flLip = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "wait"))
	{
		m_flWait = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "height"))
	{
		m_flHeight = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "rotation"))
	{
		m_vecFinalAngle.x = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "movesnd"))
	{
		m_bMoveSnd = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "stopsnd"))
	{
		m_bStopSnd = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "custommovesnd"))
	{
		pev->noise = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "customstopsnd"))
	{
		pev->noise1 = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "volume"))
	{
		m_volume = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CBaseToggle::KeyValue( pkvd );
}

#define noiseMoving noise
#define noiseArrived noise1

void CBasePlatTrain::Precache( void )
{
// set the plat's "in-motion" sound
	if (FStringNull(pev->noiseMoving))
	{
		switch (m_bMoveSnd)
		{
		case	1:
			pev->noiseMoving = MAKE_STRING("plats/bigmove1.wav");
			break;
		case	2:
			pev->noiseMoving = MAKE_STRING("plats/bigmove2.wav");
			break;
		case	3:
			pev->noiseMoving = MAKE_STRING("plats/elevmove1.wav");
			break;
		case	4:
			pev->noiseMoving = MAKE_STRING("plats/elevmove2.wav");
			break;
		case	5:
			pev->noiseMoving = MAKE_STRING("plats/elevmove3.wav");
			break;
		case	6:
			pev->noiseMoving = MAKE_STRING("plats/freightmove1.wav");
			break;
		case	7:
			pev->noiseMoving = MAKE_STRING("plats/freightmove2.wav");
			break;
		case	8:
			pev->noiseMoving = MAKE_STRING("plats/heavymove1.wav");
			break;
		case	9:
			pev->noiseMoving = MAKE_STRING("plats/rackmove1.wav");
			break;
		case	10:
			pev->noiseMoving = MAKE_STRING("plats/railmove1.wav");
			break;
		case	11:
			pev->noiseMoving = MAKE_STRING("plats/squeekmove1.wav");
			break;
		case	12:
			pev->noiseMoving = MAKE_STRING("plats/talkmove1.wav");
			break;
		case	13:
			pev->noiseMoving = MAKE_STRING("plats/talkmove2.wav");
			break;
		default:
			pev->noiseMoving = MAKE_STRING("common/null.wav");
			break;
		}
	}
	PRECACHE_SOUND ((char*)STRING(pev->noiseMoving));

// set the plat's 'reached destination' stop sound
	if (FStringNull(pev->noiseArrived))
	{
		switch (m_bStopSnd)
		{
		case	1:
			pev->noiseArrived = MAKE_STRING("plats/bigstop1.wav");
			break;
		case	2:
			pev->noiseArrived = MAKE_STRING("plats/bigstop2.wav");
			break;
		case	3:
			pev->noiseArrived = MAKE_STRING("plats/freightstop1.wav");
			break;
		case	4:
			pev->noiseArrived = MAKE_STRING("plats/heavystop2.wav");
			break;
		case	5:
			pev->noiseArrived = MAKE_STRING("plats/rackstop1.wav");
			break;
		case	6:
			pev->noiseArrived = MAKE_STRING("plats/railstop1.wav");
			break;
		case	7:
			pev->noiseArrived = MAKE_STRING("plats/squeekstop1.wav");
			break;
		case	8:
			pev->noiseArrived = MAKE_STRING("plats/talkstop1.wav");
			break;
		default:
			pev->noiseArrived = MAKE_STRING("common/null.wav");
			break;
		}
	}
	PRECACHE_SOUND ((char*)STRING(pev->noiseArrived));
}

//
//====================== PLAT code ====================================================
//


#define noiseMovement noise
#define noiseStopMoving noise1

class CFuncPlat : public CBasePlatTrain
{
public:
	void Spawn( void );
	void Precache( void );
	void Setup( void );

	virtual void Blocked( CBaseEntity *pOther );

	void EXPORT PlatUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );

	void	EXPORT CallGoUp( void ) { GoUp(); }
	void	EXPORT CallGoDown( void ) { GoDown(); }
	void	EXPORT CallHitTop( void  ) { HitTop(); }
	void	EXPORT CallHitBottom( void ) { HitBottom(); }

	virtual void GoUp( void );
	virtual void GoDown( void );
	virtual void HitTop( void );
	virtual void HitBottom( void );
};
LINK_ENTITY_TO_CLASS( func_plat, CFuncPlat );


// UNDONE: Need to save this!!! It needs class & linkage
class CPlatTrigger : public CBaseEntity
{
public:
	virtual int	ObjectCaps( void ) { return (CBaseEntity :: ObjectCaps() & ~FCAP_ACROSS_TRANSITION) | FCAP_DONT_SAVE; }
	void SpawnInsideTrigger( CFuncPlat *pPlatform );
	void Touch( CBaseEntity *pOther );
	CFuncPlat *m_pPlatform;
};



/*QUAKED func_plat (0 .5 .8) ? PLAT_LOW_TRIGGER
speed	default 150

Plats are always drawn in the extended position, so they will light correctly.

If the plat is the target of another trigger or button, it will start out disabled in
the extended position until it is trigger, when it will lower and become a normal plat.

If the "height" key is set, that will determine the amount the plat moves, instead of
being implicitly determined by the model's height.

Set "sounds" to one of the following:
1) base fast
2) chain slow
*/

void CFuncPlat :: Setup( void )
{
	//pev->noiseMovement = MAKE_STRING("plats/platmove1.wav");
	//pev->noiseStopMoving = MAKE_STRING("plats/platstop1.wav");

	if (m_flTLength == 0)
		m_flTLength = 80;
	if (m_flTWidth == 0)
		m_flTWidth = 10;
	
	pev->angles		= g_vecZero;

	pev->solid		= SOLID_BSP;
	pev->movetype	= MOVETYPE_PUSH;

	UTIL_SetOrigin(this, pev->origin);		// set size and link into world
	UTIL_SetSize(pev, pev->mins, pev->maxs);
	SET_MODEL(ENT(pev), STRING(pev->model) );

	// vecPosition1 is the top position, vecPosition2 is the bottom
	if (m_pMoveWith)
	  m_vecPosition1 = pev->origin - m_pMoveWith->pev->origin;
	else
	  m_vecPosition1 = pev->origin;
	m_vecPosition2 = m_vecPosition1;
	if (m_flHeight != 0)
		m_vecPosition2.z = m_vecPosition2.z - m_flHeight;
	else
		m_vecPosition2.z = m_vecPosition2.z - pev->size.z + 8;
	if (pev->speed == 0)
		pev->speed = 150;

	if ( m_volume == 0 )
		m_volume = 0.85;
}


void CFuncPlat :: Precache( )
{
	CBasePlatTrain::Precache();
	//PRECACHE_SOUND("plats/platmove1.wav");
	//PRECACHE_SOUND("plats/platstop1.wav");
	if ( !IsTogglePlat() )
		PlatSpawnInsideTrigger( pev );		// the "start moving" trigger
}


void CFuncPlat :: Spawn( )
{
	Setup();

	Precache();

	// If this platform is the target of some button, it starts at the TOP position,
	// and is brought down by that button.  Otherwise, it starts at BOTTOM.
	if ( !FStringNull(pev->targetname) )
	{
	  	if (m_pMoveWith)
	  	  UTIL_AssignOrigin (this, m_vecPosition1 + m_pMoveWith->pev->origin);
	  	else
	  	  UTIL_AssignOrigin (this, m_vecPosition1);
		m_toggle_state = TS_AT_TOP;
		SetUse(&CFuncPlat :: PlatUse );
	}
	else
	{
		if (m_pMoveWith)
		  UTIL_AssignOrigin (this, m_vecPosition2 + m_pMoveWith->pev->origin);
		else
		  UTIL_AssignOrigin (this, m_vecPosition2);
		m_toggle_state = TS_AT_BOTTOM;
	}
}



static void PlatSpawnInsideTrigger(entvars_t* pevPlatform)
{
	GetClassPtr( (CPlatTrigger *)NULL)->SpawnInsideTrigger( GetClassPtr( (CFuncPlat *)pevPlatform ) );
}
		

//
// Create a trigger entity for a platform.
//
void CPlatTrigger :: SpawnInsideTrigger( CFuncPlat *pPlatform )
{
	m_pPlatform = pPlatform;
	// Create trigger entity, "point" it at the owning platform, give it a touch method
	pev->solid		= SOLID_TRIGGER;
	pev->movetype	= MOVETYPE_NONE;
	pev->origin = pPlatform->pev->origin;

	// Establish the trigger field's size
	Vector vecTMin = m_pPlatform->pev->mins + Vector ( 25 , 25 , 0 );
	Vector vecTMax = m_pPlatform->pev->maxs + Vector ( 25 , 25 , 8 );
	vecTMin.z = vecTMax.z - ( m_pPlatform->m_vecPosition1.z - m_pPlatform->m_vecPosition2.z + 8 );
	if (m_pPlatform->pev->size.x <= 50)
	{
		vecTMin.x = (m_pPlatform->pev->mins.x + m_pPlatform->pev->maxs.x) / 2;
		vecTMax.x = vecTMin.x + 1;
	}
	if (m_pPlatform->pev->size.y <= 50)
	{
		vecTMin.y = (m_pPlatform->pev->mins.y + m_pPlatform->pev->maxs.y) / 2;
		vecTMax.y = vecTMin.y + 1;
	}
	UTIL_SetSize ( pev, vecTMin, vecTMax );
}


//
// When the platform's trigger field is touched, the platform ???
//
void CPlatTrigger :: Touch( CBaseEntity *pOther )
{
	// Ignore touches by non-players
	entvars_t*	pevToucher = pOther->pev;
	if ( !FClassnameIs (pevToucher, "player") )
		return;

	// Ignore touches by corpses
	if (!pOther->IsAlive())
		return;
	
	// Make linked platform go up/down.
	if (m_pPlatform->m_toggle_state == TS_AT_BOTTOM)
		m_pPlatform->GoUp();
	else if (m_pPlatform->m_toggle_state == TS_AT_TOP)
		m_pPlatform->SetNextThink( 1 );// delay going down
}


//
// Used by SUB_UseTargets, when a platform is the target of a button.
// Start bringing platform down.
//
void CFuncPlat :: PlatUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if ( IsTogglePlat() )
	{
		// Top is off, bottom is on
		BOOL on = (m_toggle_state == TS_AT_BOTTOM) ? TRUE : FALSE;

		if ( !ShouldToggle( useType, on ) )
			return;

		if (m_toggle_state == TS_AT_TOP)
		{
			SetNextThink( 0.01 );
			SetThink(&CFuncPlat :: CallGoDown );
		}
		else if ( m_toggle_state == TS_AT_BOTTOM )
		{
			SetNextThink( 0.01 );
			SetThink(&CFuncPlat :: CallGoUp );
		}
	}
	else
	{
		SetUse( NULL );

		if (m_toggle_state == TS_AT_TOP)
		{
			SetNextThink( 0.01 );
			SetThink(&CFuncPlat :: CallGoDown );
		}
	}
}


//
// Platform is at top, now starts moving down.
//
void CFuncPlat :: GoDown( void )
{
	if(pev->noiseMovement)
		EMIT_SOUND(ENT(pev), CHAN_STATIC, (char*)STRING(pev->noiseMovement), m_volume, ATTN_NORM);

	ASSERT(m_toggle_state == TS_AT_TOP || m_toggle_state == TS_GOING_UP);
	m_toggle_state = TS_GOING_DOWN;
	SetMoveDone(&CFuncPlat ::CallHitBottom);
	LinearMove(m_vecPosition2, pev->speed);
}


//
// Platform has hit bottom.  Stops and waits forever.
//
void CFuncPlat :: HitBottom( void )
{
	if(pev->noiseMovement)
		STOP_SOUND(ENT(pev), CHAN_STATIC, (char*)STRING(pev->noiseMovement));

	if (pev->noiseStopMoving)
		EMIT_SOUND(ENT(pev), CHAN_WEAPON, (char*)STRING(pev->noiseStopMoving), m_volume, ATTN_NORM);

	ASSERT(m_toggle_state == TS_GOING_DOWN);
	m_toggle_state = TS_AT_BOTTOM;
}


//
// Platform is at bottom, now starts moving up
//
void CFuncPlat :: GoUp( void )
{
	if (pev->noiseMovement)
		EMIT_SOUND(ENT(pev), CHAN_STATIC, (char*)STRING(pev->noiseMovement), m_volume, ATTN_NORM);
	
	ASSERT(m_toggle_state == TS_AT_BOTTOM || m_toggle_state == TS_GOING_DOWN);
	m_toggle_state = TS_GOING_UP;
	SetMoveDone(&CFuncPlat ::CallHitTop);
	LinearMove(m_vecPosition1, pev->speed);
}


//
// Platform has hit top.  Pauses, then starts back down again.
//
void CFuncPlat :: HitTop( void )
{
	if(pev->noiseMovement)
		STOP_SOUND(ENT(pev), CHAN_STATIC, (char*)STRING(pev->noiseMovement));

	if (pev->noiseStopMoving)
		EMIT_SOUND(ENT(pev), CHAN_WEAPON, (char*)STRING(pev->noiseStopMoving), m_volume, ATTN_NORM);
	
	ASSERT(m_toggle_state == TS_GOING_UP);
	m_toggle_state = TS_AT_TOP;

	if ( !IsTogglePlat() )
	{
		// After a delay, the platform will automatically start going down again.
		SetThink(&CFuncPlat :: CallGoDown );
		SetNextThink( 3 );
	}
}


void CFuncPlat :: Blocked( CBaseEntity *pOther )
{
	ALERT( at_aiconsole, "%s Blocked by %s\n", STRING(pev->classname), STRING(pOther->pev->classname) );
	// Hurt the blocker a little
	pOther->TakeDamage(pev, pev, 1, DMG_CRUSH);

	if(pev->noiseMovement)
		STOP_SOUND(ENT(pev), CHAN_STATIC, (char*)STRING(pev->noiseMovement));
	
	// Send the platform back where it came from
	ASSERT(m_toggle_state == TS_GOING_UP || m_toggle_state == TS_GOING_DOWN);
	if (m_toggle_state == TS_GOING_UP)
	{
		SetNextThink( 0 );
		SetThink(&CFuncPlat :: GoDown );
	}
	else if (m_toggle_state == TS_GOING_DOWN)
	{
		SetNextThink( 0 );
		SetThink(&CFuncPlat :: GoUp );
	}
}


class CFuncPlatRot : public CFuncPlat
{
public:
	void Spawn( void );
	void SetupRotation( void );
	virtual void KeyValue( KeyValueData *pkvd );

	virtual void	GoUp( void );
	virtual void	GoDown( void );
	virtual void	HitTop( void );
	virtual void	HitBottom( void );
	
	void			RotMove( Vector &destAngle, float time );
	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	Vector	m_end, m_start;
};
LINK_ENTITY_TO_CLASS( func_platrot, CFuncPlatRot );
TYPEDESCRIPTION	CFuncPlatRot::m_SaveData[] = 
{
	DEFINE_FIELD( CFuncPlatRot, m_end, FIELD_VECTOR ),
	DEFINE_FIELD( CFuncPlatRot, m_start, FIELD_VECTOR ),
};

IMPLEMENT_SAVERESTORE( CFuncPlatRot, CFuncPlat );

void CFuncPlatRot::KeyValue( KeyValueData *pkvd )
{
	if (FStrEq(pkvd->szKeyName, "axes"))
	{
		UTIL_StringToVector((float*)(pev->movedir), pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CFuncPlat::KeyValue( pkvd );
}

void CFuncPlatRot :: SetupRotation( void )
{
	if ( m_vecFinalAngle.x != 0 )		// This plat rotates too!
	{
		CBaseToggle :: AxisDir( pev );
		m_start	= pev->angles;
		m_end = pev->angles + pev->movedir * m_vecFinalAngle.x;
	}
	else
	{
		m_start = g_vecZero;
		m_end = g_vecZero;
	}
	if ( !FStringNull(pev->targetname) )	// Start at top
	{
		UTIL_SetAngles(this, m_end);
		//pev->angles = m_end;
	}
}


void CFuncPlatRot :: Spawn( void )
{
	CFuncPlat :: Spawn();
	SetupRotation();
}

void CFuncPlatRot :: GoDown( void )
{
	CFuncPlat :: GoDown();
	RotMove( m_start, m_fNextThink - pev->ltime );
}


//
// Platform has hit bottom.  Stops and waits forever.
//
void CFuncPlatRot :: HitBottom( void )
{
	CFuncPlat :: HitBottom();
	UTIL_SetAvelocity(this, g_vecZero);
	//pev->avelocity = g_vecZero;
	UTIL_SetAngles(this, m_start);
	//pev->angles = m_start;
}


//
// Platform is at bottom, now starts moving up
//
void CFuncPlatRot :: GoUp( void )
{
	CFuncPlat :: GoUp();
	RotMove( m_end, m_fNextThink - pev->ltime );
}


//
// Platform has hit top.  Pauses, then starts back down again.
//
void CFuncPlatRot :: HitTop( void )
{
	CFuncPlat :: HitTop();
	UTIL_SetAvelocity(this, g_vecZero);
	//pev->avelocity = g_vecZero;
	UTIL_SetAngles(this, m_end);
	//pev->angles = m_end;
}


void CFuncPlatRot :: RotMove( Vector &destAngle, float time )
{
	// set destdelta to the vector needed to move
	Vector vecDestDelta = destAngle - pev->angles;

	// Travel time is so short, we're practically there already;  make it so.
	if ( time >= 0.1)
	{
		UTIL_SetAvelocity(this, vecDestDelta / time);
		//pev->avelocity = vecDestDelta / time;
	}
	else
	{
		UTIL_SetAvelocity(this, vecDestDelta);
		//pev->avelocity = vecDestDelta;
		SetNextThink( 1 );
	}
}


//
//====================== TRAIN code ==================================================
//

//the others are defined in const.h
//		SF_TRAIN_WAIT_RETRIGGER	1
#define SF_TRAIN_SETORIGIN		2
//		SF_TRAIN_START_ON		4		// Train is initially moving
//		SF_TRAIN_PASSABLE		8		// Train is not solid -- used to make water trains
#define SF_TRAIN_REVERSE		0x800000

class CFuncTrain : public CBasePlatTrain
{
public:
	void Spawn( void );
	void Precache( void );
	void PostSpawn( void );
	void OverrideReset( void );

	void Blocked( CBaseEntity *pOther );
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void KeyValue( KeyValueData *pkvd );

	//LRC
	void StartSequence(CTrainSequence *pSequence);
	void StopSequence( );
	CTrainSequence *m_pSequence;

	void EXPORT Wait( void );
	void EXPORT Next( void );
	void EXPORT ThinkDoNext( void );
	void EXPORT SoundSetup( void );

	STATE GetState( void ) { return m_iState; }

	virtual void ThinkCorrection( void );

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	entvars_t	*m_pevCurrentTarget;
	int			m_sounds;
//LRC - now part of CBaseEntity:	BOOL		m_activated;
	STATE		m_iState;
	float		m_fStoredThink;
	Vector		m_vecAvelocity;
};

LINK_ENTITY_TO_CLASS( func_train, CFuncTrain );
TYPEDESCRIPTION	CFuncTrain::m_SaveData[] = 
{
	DEFINE_FIELD( CFuncTrain, m_sounds, FIELD_INTEGER ),
	DEFINE_FIELD( CFuncTrain, m_pevCurrentTarget, FIELD_EVARS ),
//LRC - now part of CBaseEntity:	DEFINE_FIELD( CFuncTrain, m_activated, FIELD_BOOLEAN ),
	DEFINE_FIELD( CFuncTrain, m_iState, FIELD_INTEGER ),
	DEFINE_FIELD( CFuncTrain, m_fStoredThink, FIELD_TIME ),
	DEFINE_FIELD( CFuncTrain, m_pSequence, FIELD_CLASSPTR ), //LRC
	DEFINE_FIELD( CFuncTrain, m_vecAvelocity, FIELD_VECTOR ), //LRC
};

IMPLEMENT_SAVERESTORE( CFuncTrain, CBasePlatTrain );


void CFuncTrain :: KeyValue( KeyValueData *pkvd )
{
	if (FStrEq(pkvd->szKeyName, "sounds"))
	{
		m_sounds = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CBasePlatTrain::KeyValue( pkvd );
}


void CFuncTrain :: Blocked( CBaseEntity *pOther )
{
	// Keep "movewith" entities in line
	UTIL_AssignOrigin(this, pev->origin);

	if ( gpGlobals->time < m_flActivateFinished)
		return;

	m_flActivateFinished = gpGlobals->time + 0.5;

	if (pev->dmg)
		pOther->TakeDamage(pev, pev, pev->dmg, DMG_CRUSH);
}


void CFuncTrain :: Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if ( ShouldToggle( useType ) )
	{
		if (pev->spawnflags & SF_TRAIN_WAIT_RETRIGGER)
		{
			// Move toward my target
//			ALERT(at_console, "Unset Retrigger (use)\n");
			pev->spawnflags &= ~SF_TRAIN_WAIT_RETRIGGER;
			Next();
		}
		else
		{
//			ALERT(at_console, "Set Retrigger (use)\n");
			pev->spawnflags |= SF_TRAIN_WAIT_RETRIGGER;
			// Pop back to last target if it's available
			if ( pev->enemy )
				pev->target = pev->enemy->v.targetname;

			DontThink();
			UTIL_SetVelocity(this, g_vecZero);
			UTIL_SetAvelocity(this, g_vecZero);
			m_iState = STATE_OFF;
//			pev->velocity = g_vecZero;

			if ( pev->noiseMovement )
				STOP_SOUND( edict(), CHAN_STATIC, (char*)STRING(pev->noiseMovement) );

			if ( pev->noiseStopMoving )
				EMIT_SOUND (ENT(pev), CHAN_VOICE, (char*)STRING(pev->noiseStopMoving), m_volume, ATTN_NORM);
		}
	}
}


void CFuncTrain :: Wait( void )
{
//	ALERT(at_console, "Wait t %s, m %s\n", STRING(pev->target), STRING(pev->message));
	if (m_pSequence)
		m_pSequence->ArrivalNotify();

	// Fire the pass target if there is one
	if ( m_pevCurrentTarget->message )
	{
		FireTargets( STRING(m_pevCurrentTarget->message), this, this, USE_TOGGLE, 0 );
		if ( FBitSet( m_pevCurrentTarget->spawnflags, SF_CORNER_FIREONCE ) )
			m_pevCurrentTarget->message = 0;
	}

	// need pointer to LAST target.
	if ( FBitSet (m_pevCurrentTarget->spawnflags , SF_TRAIN_WAIT_RETRIGGER ) || ( pev->spawnflags & SF_TRAIN_WAIT_RETRIGGER ) )
    {
//		if (FBitSet (m_pevCurrentTarget->spawnflags , SF_TRAIN_WAIT_RETRIGGER ))
//			ALERT(at_console, "Wait: wait for retrigger from path %s\n", STRING(m_pevCurrentTarget->targetname));
//		else
//			ALERT(at_console, "Wait: wait for retrigger from train\n");
		pev->spawnflags |= SF_TRAIN_WAIT_RETRIGGER;
		m_iState = STATE_OFF;
        // clear the sound channel.
		if ( pev->noiseMovement )
			STOP_SOUND( edict(), CHAN_STATIC, (char*)STRING(pev->noiseMovement) );

		if ( pev->noiseStopMoving )
			EMIT_SOUND (ENT(pev), CHAN_VOICE, (char*)STRING(pev->noiseStopMoving), m_volume, ATTN_NORM);

		UTIL_SetVelocity(this, g_vecZero);
		UTIL_SetAvelocity(this, g_vecZero);
		DontThink();
		return;
	}
    
    // ALERT ( at_console, "%f\n", m_flWait );

	if (m_flWait != 0)
	{// -1 wait will wait forever!		
		m_iState = STATE_OFF;
		UTIL_SetAvelocity(this, g_vecZero);
		UTIL_SetVelocity(this, g_vecZero);
		SetNextThink( m_flWait );
		if ( pev->noiseMovement )
			STOP_SOUND( edict(), CHAN_STATIC, (char*)STRING(pev->noiseMovement) );
		if ( pev->noiseStopMoving )
			EMIT_SOUND (ENT(pev), CHAN_VOICE, (char*)STRING(pev->noiseStopMoving), m_volume, ATTN_NORM);
		SetThink(&CFuncTrain :: Next );
//		ALERT(at_console, "Wait: doing Next in %f\n", m_flWait);
	}
	else
	{
//		ALERT(at_console, "Wait: doing Next now\n");
		Next();// do it right now!
	}
}


//
// Train next - path corner needs to change to next target 
//
void CFuncTrain :: Next( void )
{
	CBaseEntity	*pTarg;
	
	// now find our next target
	pTarg = GetNextTarget();

    if ( !pTarg )
	{
		// no destination, just come to a halt
		m_iState = STATE_OFF;
		UTIL_SetVelocity(this, g_vecZero);
		UTIL_SetAvelocity(this, g_vecZero);

		if ( pev->noiseMovement )
			STOP_SOUND( edict(), CHAN_STATIC, (char*)STRING(pev->noiseMovement) );
		// Play stop sound
		if ( pev->noiseStopMoving )
			EMIT_SOUND (ENT(pev), CHAN_VOICE, (char*)STRING(pev->noiseStopMoving), m_volume, ATTN_NORM);

		return;
	}

	// Save last target in case we need to find it again
	pev->message = pev->target;

//	if (m_pevCurrentTarget)
//		ALERT(at_console, "Next, pTarg %s, pevTarg %s\n", STRING(pTarg->pev->targetname), STRING(m_pevCurrentTarget->targetname));
//	else
//		ALERT(at_console, "Next, pTarg %s, pevTarg null\n", STRING(pTarg->pev->targetname));

	if (pev->spawnflags & SF_TRAIN_REVERSE && m_pSequence)
	{
		//LRC - search backwards
		CBaseEntity *pSearch = m_pSequence->m_pDestination;
		while (pSearch)
		{
			if (FStrEq(STRING(pSearch->pev->target), STRING(pev->target)))
			{
				// pSearch leads to the current corner, so it's the next thing we're moving to.
				pev->target = pSearch->pev->targetname;
//				ALERT(at_console, "Next, pSearch %s\n", STRING(pSearch->pev->targetname));
				break;
			}
			pSearch = pSearch->GetNextTarget();
		}
	}
	else
	{
		pev->target = pTarg->pev->target;
	}

//	ALERT(at_console, "Next, new pevtarget %s, new message %s\n", STRING(pev->target), STRING(pev->message));

	m_flWait = pTarg->GetDelay();

	// don't copy speed from target if it is 0 (uninitialized)
	if ( m_pevCurrentTarget )
	{
		if ( m_pevCurrentTarget->speed != 0 )
		{
			switch ((int)(m_pevCurrentTarget->armortype))
			{
			case PATHSPEED_SET:
				pev->speed = m_pevCurrentTarget->speed;
				ALERT( at_aiconsole, "Train %s speed set to %4.2f\n", STRING(pev->targetname), pev->speed );
				break;
			case PATHSPEED_ACCEL:
				pev->speed += m_pevCurrentTarget->speed;
				ALERT( at_aiconsole, "Train %s speed accel to %4.2f\n", STRING(pev->targetname), pev->speed );
				break;
			case PATHSPEED_TIME:
				float distance;
				if (pev->spawnflags & SF_TRAIN_SETORIGIN)		
					distance = (pev->origin - pTarg->pev->origin).Length();
				else
					distance = (pev->origin - (pTarg->pev->origin - (pev->mins + pev->maxs) * 0.5)).Length();

				pev->speed = distance / m_pevCurrentTarget->speed;
				ALERT( at_aiconsole, "Train %s speed to %4.2f (timed)\n", STRING(pev->targetname), pev->speed );
				break;
			}
		}

		if (m_pevCurrentTarget->spawnflags & SF_CORNER_AVELOCITY)
		{
			m_vecAvelocity = pTarg->pev->avelocity;
			UTIL_SetAvelocity(this, m_vecAvelocity);
			//pev->avelocity = pTarg->pev->avelocity; //LRC
		}

		if (m_pevCurrentTarget->armorvalue)
		{
			UTIL_SetAngles(this, m_pevCurrentTarget->angles);
			//pev->angles = m_pevCurrentTarget->angles; //LRC - if we just passed a "turn to face" corner, set angle exactly.
		}
	}

	m_pevCurrentTarget = pTarg->pev;// keep track of this since path corners change our target for us.

    pev->enemy = pTarg->edict();//hack

	if( FBitSet(pTarg->pev->spawnflags, SF_CORNER_TELEPORT) )  //LRC - cosmetic change to use pTarg
	{
		// Path corner has indicated a teleport to the next corner.
		SetBits(pev->effects, EF_NOINTERP);
		if (m_pMoveWith)
		{
			if (pev->spawnflags & SF_TRAIN_SETORIGIN)		
				UTIL_AssignOrigin(this, pTarg->pev->origin - m_pMoveWith->pev->origin );
			else
				UTIL_AssignOrigin(this, pTarg->pev->origin - (pev->mins + pev->maxs) * 0.5 - m_pMoveWith->pev->origin );
		}
		else
		{
			if (pev->spawnflags & SF_TRAIN_SETORIGIN)		
				UTIL_AssignOrigin(this, pTarg->pev->origin );
			else
				UTIL_AssignOrigin(this, pTarg->pev->origin - (pev->mins + pev->maxs) * 0.5 );
		}

		if (pTarg->pev->armorvalue) //LRC - "teleport and turn to face" means you set an angle as you teleport.
		{
			UTIL_SetAngles(this, pTarg->pev->angles);
			//pev->angles = pTarg->pev->angles;
		}

		Wait(); // Get on with doing the next path corner.
	}
	else
	{
		// Normal linear move.

		// CHANGED this from CHAN_VOICE to CHAN_STATIC around OEM beta time because trains should
		// use CHAN_STATIC for their movement sounds to prevent sound field problems.
		// this is not a hack or temporary fix, this is how things should be. (sjb).
		if (m_iState == STATE_OFF) //LRC - don't restart the sound every time we hit a path_corner, it sounds weird
		{
			if ( pev->noiseMovement )
				STOP_SOUND( edict(), CHAN_STATIC, (char*)STRING(pev->noiseMovement) );
			if ( pev->noiseMovement )
				EMIT_SOUND (ENT(pev), CHAN_STATIC, (char*)STRING(pev->noiseMovement), m_volume, ATTN_NORM);
		}
		ClearBits(pev->effects, EF_NOINTERP);
		SetMoveDone(&CFuncTrain :: Wait );

		if (pTarg->pev->armorvalue) //LRC - "turn to face" the next corner
		{
			Vector vTemp = pev->angles;
			FixupAngles( vTemp );
			UTIL_SetAngles(this, vTemp);
			Vector oDelta = pTarg->pev->origin - pev->origin;
			Vector aDelta = pTarg->pev->angles - pev->angles;
			float timeTaken = oDelta.Length() / pev->speed;
			m_vecAvelocity = aDelta / timeTaken;
			//pev->avelocity = aDelta / timeTaken;
		}

		UTIL_SetAvelocity(this, m_vecAvelocity);

		m_iState = STATE_ON;

		if (m_pMoveWith)
		{
			if (pev->spawnflags & SF_TRAIN_SETORIGIN)
				LinearMove( pTarg->pev->origin - m_pMoveWith->pev->origin, pev->speed );
			else
				LinearMove (pTarg->pev->origin - (pev->mins + pev->maxs)* 0.5 - m_pMoveWith->pev->origin, pev->speed);
		}
		else
		{
			if (pev->spawnflags & SF_TRAIN_SETORIGIN)
				LinearMove( pTarg->pev->origin, pev->speed );
			else
				LinearMove (pTarg->pev->origin - (pev->mins + pev->maxs)* 0.5, pev->speed);
		}

//		ALERT(at_console, "Next: LMove done\n");
//		ALERT(at_console, "Next ends, nextthink %f, flags %f\n", pev->nextthink, m_iLFlags);
	}
}

//LRC- called by Activate. (but not when a game is loaded.)
void CFuncTrain :: PostSpawn( void )
{
	CBaseEntity *pTarget = UTIL_FindEntityByTargetname (NULL, STRING(pev->target) );
	entvars_t	*pevTarg;

	m_iState = STATE_OFF;

	if (pTarget)
	{
		pevTarg = pTarget->pev;
	}
	else
	{
		ALERT(at_debug, "Missing train target \"%s\"\n", STRING(pev->target));
		return;
	}

	pev->message = pevTarg->targetname; //LRC - record the old target so that we can find it again
	pev->target = pevTarg->target;
	m_pevCurrentTarget = pevTarg;// keep track of this since path corners change our target for us.

	if (pev->avelocity != g_vecZero)
	{
		m_vecAvelocity = pev->avelocity;
		UTIL_SetAvelocity(this, g_vecZero);
	}

	if (pev->spawnflags & SF_TRAIN_SETORIGIN)
	{
		m_vecSpawnOffset = m_vecSpawnOffset + pevTarg->origin - pev->origin;
		if (m_pMoveWith)
			UTIL_AssignOrigin (this, pevTarg->origin - m_pMoveWith->pev->origin );
		else
			UTIL_AssignOrigin (this, pevTarg->origin );
	}
	else
	{
		m_vecSpawnOffset = m_vecSpawnOffset + (pevTarg->origin - (pev->mins + pev->maxs) * 0.5) - pev->origin;
		if (m_pMoveWith)
			UTIL_AssignOrigin (this, pevTarg->origin - (pev->mins + pev->maxs) * 0.5 - m_pMoveWith->pev->origin );
		else
			UTIL_AssignOrigin (this, pevTarg->origin - (pev->mins + pev->maxs) * 0.5 );
	}

	if ( FStringNull(pev->targetname) || pev->spawnflags & SF_TRAIN_START_ON)
	{	// not triggered, so start immediately
		SetNextThink( 1.5 );
//		SetThink( Next );
		SetThink(&CFuncTrain :: ThinkDoNext );
	}
	else
	{
//		ALERT(at_console, "Set Retrigger (postspawn)\n");
		pev->spawnflags |= SF_TRAIN_WAIT_RETRIGGER;
	}

//	ALERT(at_console, "func_train postspawn: origin %f %f %f\n", pev->origin.x, pev->origin.y, pev->origin.z);
}

void CFuncTrain :: ThinkDoNext( void )
{
	SetNextThink( 0.1 );
//	ALERT(at_console, "TDN ");
	if (gpGlobals->time != 1.0) // only go on if the game has properly started yet
		SetThink(&CFuncTrain :: Next );
}

//LRC
void CFuncTrain :: StartSequence(CTrainSequence *pSequence)
{
	m_pSequence = pSequence;
//	ALERT(at_console, "Unset Retrigger (startsequence)\n");
	pev->spawnflags &= ~SF_TRAIN_WAIT_RETRIGGER;
//	m_iState = STATE_ON;
	//...
}

//LRC
void CFuncTrain :: StopSequence( )
{
	m_pSequence = NULL;
//	pev->spawnflags &= ~SF_TRAIN_WAIT_RETRIGGER;
	pev->spawnflags &= ~SF_TRAIN_REVERSE;
	Use(this, this, USE_OFF, 0);
	//...
}

/*QUAKED func_train (0 .5 .8) ?
Trains are moving platforms that players can ride.
The targets origin specifies the min point of the train at each corner.
The train spawns at the first target it is pointing at.
If the train is the target of a button or trigger, it will not begin moving until activated.
speed	default 100
dmg		default	2
sounds
1) ratchet metal
*/

void CFuncTrain :: Spawn( void )
{
	Precache();
	if (pev->speed == 0)
		pev->speed = 100;

//	if (!(pev->origin == g_vecZero))
//	{
//		pev->spawnflags |= SF_TRAIN_SETORIGIN;
//		m_vecSpawnOffset = pev->origin;
//	}

	if ( FStringNull(pev->target) )
		ALERT(at_debug, "func_train \"%s\" has no target\n", STRING(pev->targetname));
	
	if (pev->dmg == 0)
		pev->dmg = 2;
	else if (pev->dmg == -1) //LRC- a train that doesn't crush people!
		pev->dmg = 0;
	
	pev->movetype = MOVETYPE_PUSH;
	
	if ( FBitSet (pev->spawnflags, SF_TRACKTRAIN_PASSABLE) )
		pev->solid		= SOLID_NOT;
	else
		pev->solid		= SOLID_BSP;

	SET_MODEL( ENT(pev), STRING(pev->model) );
	UTIL_SetSize (pev, pev->mins, pev->maxs);
	UTIL_SetOrigin(this, pev->origin);

	m_iState = STATE_OFF;

	if ( m_volume == 0 )
		m_volume = 0.85;
}

//LRC - making movement sounds which continue after a game is loaded.
void CFuncTrain :: SoundSetup( void )
{
	EMIT_SOUND (ENT(pev), CHAN_STATIC, (char*)STRING(pev->noiseMovement), m_volume, ATTN_NORM);
	SetNextThink( m_fStoredThink - pev->ltime );
//	ALERT(at_console, "SoundSetup: mfNT %f, pevNT %f, stored was %f, time %f", m_fNextThink, pev->nextthink, m_fStoredThink, pev->ltime );
	m_fStoredThink = 0;
	SetThink(&CFuncTrain :: LinearMoveDone );
}

//LRC
void CFuncTrain :: ThinkCorrection( void )
{
	if (m_fStoredThink && pev->nextthink != m_fPevNextThink)
	{
//		ALERT(at_console, "StoredThink Correction for train \"%s\", %f -> %f\n", STRING(pev->targetname), m_fStoredThink, m_fStoredThink + pev->nextthink - m_fPevNextThink);
		m_fStoredThink += pev->nextthink -  m_fPevNextThink;
	}

	CBasePlatTrain::ThinkCorrection();
}

void CFuncTrain :: Precache( void )
{
	CBasePlatTrain::Precache();

	//LRC - continue the movement sound after loading a game
	if (m_iState == STATE_ON && pev->noiseMovement)
	{
		// we can't set up SFX during precache, so get a think to do it.
		// Unfortunately, since we're moving, we must be already thinking.
		// So we store the current think time, and will restore it after SFX are done.
		if (!m_fStoredThink)
			m_fStoredThink = m_fNextThink;
		SetNextThink( 0.1 );
//		ALERT(at_console, "preparing SoundSetup: stored %f, mfNT %f, pevNT %f, ltime %f", m_fStoredThink, m_fNextThink, pev->nextthink, pev->ltime);
		SetThink(&CFuncTrain :: SoundSetup );
	}

#if 0  // obsolete
	// otherwise use preset sound
	switch (m_sounds)
	{
	case 0:
		pev->noise = 0;
		pev->noise1 = 0;
		break;

	case 1:
		PRECACHE_SOUND ("plats/train2.wav");
		PRECACHE_SOUND ("plats/train1.wav");
		pev->noise = MAKE_STRING("plats/train2.wav");
		pev->noise1 = MAKE_STRING("plats/train1.wav");
		break;

	case 2:
		PRECACHE_SOUND ("plats/platmove1.wav");
		PRECACHE_SOUND ("plats/platstop1.wav");
		pev->noise = MAKE_STRING("plats/platstop1.wav");
		pev->noise1 = MAKE_STRING("plats/platmove1.wav");
		break;
	}
#endif
}

void CFuncTrain::OverrideReset( void )
{
	CBaseEntity	*pTarg;

	// Are we moving?
	if ( m_iState == STATE_ON ) //pev->velocity != g_vecZero && pev->nextthink != 0 )
	{
		pev->target = pev->message;
		// now find our next target
		pTarg = GetNextTarget();
		if ( !pTarg )
		{
			DontThink();
			UTIL_SetVelocity(this, g_vecZero);
			m_iState = STATE_OFF;
		}
		else	// Keep moving for 0.1 secs, then find path_corner again and restart
		{
			SetThink(&CFuncTrain:: Next );
			SetNextThink( 0.1 );
		}
	}
}




// ---------------------------------------------------------------------
//
// Track Train
//
// ---------------------------------------------------------------------
void CFuncTrackTrain :: Spawn( void )
{
	if ( pev->speed == 0 )
		m_speed = 100;
	else
		m_speed = pev->speed;
	
	pev->speed = 0;
	pev->velocity = g_vecZero; // why do they set this stuff? --LRC
	m_vecBaseAvel = pev->avelocity; //LRC - save it for later
	pev->avelocity = g_vecZero;
	pev->impulse = m_speed;

	m_dir = 1;

	if ( FStringNull(pev->target) )
	{
		if ( FStringNull(pev->targetname) )
			ALERT( at_debug, "func_tracktrain with no target\n" );
		else
			ALERT( at_debug, "func_tracktrain %s has no target\n", STRING(pev->targetname));
	}

	if ( pev->spawnflags & SF_TRACKTRAIN_PASSABLE )
		pev->solid = SOLID_NOT;
	else
		pev->solid = SOLID_BSP;
	pev->movetype = MOVETYPE_PUSH;
	
	SET_MODEL( ENT(pev), STRING(pev->model) );

	UTIL_SetSize( pev, pev->mins, pev->maxs );
	UTIL_SetOrigin( this, pev->origin );
//	ALERT(at_console, "SpawnOrigin %f %f %f\n", pev->origin.x, pev->origin.y, pev->origin.z);

	// Cache off placed origin for train controls
	pev->oldorigin = pev->origin;

	m_controlMins = pev->mins;
	m_controlMaxs = pev->maxs;
	m_controlMaxs.z += 72;
// start trains on the next frame, to make sure their targets have had
// a chance to spawn/activate
	NextThink( 0.1, FALSE );
	SetThink(&CFuncTrackTrain :: Find );
	Precache();
}

void CFuncTrackTrain :: Precache( void )
{
	if (m_flVolume == 0.0)
		m_flVolume = 1.0;

	switch (m_sounds)
	{
	default:
		//pev->noise = 0; LRC - allow custom sounds to be set in worldcraft.
		break;
	case 1: pev->noise = MAKE_STRING("plats/ttrain1.wav");break;
	case 2: pev->noise = MAKE_STRING("plats/ttrain2.wav");break;
	case 3: pev->noise = MAKE_STRING("plats/ttrain3.wav");break; 
	case 4: pev->noise = MAKE_STRING("plats/ttrain4.wav");break;
	case 5: pev->noise = MAKE_STRING("plats/ttrain6.wav");break;
	case 6: pev->noise = MAKE_STRING("plats/ttrain7.wav");break;
	}

	if (FStringNull(pev->noise1))
		pev->noise1 = MAKE_STRING("plats/ttrain_brake1.wav");

	if (FStringNull(pev->noise2))
		pev->noise2 = MAKE_STRING("plats/ttrain_start1.wav");

	if (pev->noise)
		PRECACHE_SOUND((char*)STRING(pev->noise)); //LRC
	PRECACHE_SOUND((char*)STRING(pev->noise1));
	PRECACHE_SOUND((char*)STRING(pev->noise2));

	m_usAdjustPitch = PRECACHE_EVENT( 1, "events/train.sc" );
}

TYPEDESCRIPTION	CFuncTrackTrain::m_SaveData[] = 
{
	DEFINE_FIELD( CFuncTrackTrain, m_ppath, FIELD_CLASSPTR ),
	DEFINE_FIELD( CFuncTrackTrain, m_length, FIELD_FLOAT ),
	DEFINE_FIELD( CFuncTrackTrain, m_height, FIELD_FLOAT ),
	DEFINE_FIELD( CFuncTrackTrain, m_speed, FIELD_FLOAT ),
	DEFINE_FIELD( CFuncTrackTrain, m_dir, FIELD_FLOAT ),
	DEFINE_FIELD( CFuncTrackTrain, m_startSpeed, FIELD_FLOAT ),
	DEFINE_FIELD( CFuncTrackTrain, m_controlMins, FIELD_VECTOR ),
	DEFINE_FIELD( CFuncTrackTrain, m_controlMaxs, FIELD_VECTOR ),
	DEFINE_FIELD( CFuncTrackTrain, m_sounds, FIELD_INTEGER ),
	DEFINE_FIELD( CFuncTrackTrain, m_flVolume, FIELD_FLOAT ),
	DEFINE_FIELD( CFuncTrackTrain, m_flBank, FIELD_FLOAT ),
	DEFINE_FIELD( CFuncTrackTrain, m_oldSpeed, FIELD_FLOAT ),
	DEFINE_FIELD( CFuncTrackTrain, m_vecMasterAvel, FIELD_VECTOR ), //LRC
	DEFINE_FIELD( CFuncTrackTrain, m_vecBaseAvel, FIELD_VECTOR ), //LRC
	DEFINE_FIELD( CFuncTrackTrain, m_pSequence, FIELD_CLASSPTR ), //LRC
};

IMPLEMENT_SAVERESTORE( CFuncTrackTrain, CBaseEntity );
LINK_ENTITY_TO_CLASS( func_tracktrain, CFuncTrackTrain );

void CFuncTrackTrain :: KeyValue( KeyValueData *pkvd )
{
	if (FStrEq(pkvd->szKeyName, "wheels"))
	{
		m_length = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "height"))
	{
		m_height = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "startspeed"))
	{
		m_startSpeed = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "sounds"))
	{
		m_sounds = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "custommovesound"))
	{
		pev->noise = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "custombrakesound"))
	{
		pev->noise1 = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "customstartsound"))
	{
		pev->noise2 = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "volume"))
	{
		m_flVolume = (float) (atoi(pkvd->szValue));
		m_flVolume *= 0.1;
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "bank"))
	{
		m_flBank = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CBaseEntity::KeyValue( pkvd );
}


void CFuncTrackTrain :: NextThink( float thinkTime, BOOL alwaysThink )
{
	if ( alwaysThink )
//		m_iLFlags |= LF_ALWAYSTHINK;
		pev->flags |= FL_ALWAYSTHINK;
	else
//		m_iLFlags &= ~LF_ALWAYSTHINK;
		pev->flags &= ~FL_ALWAYSTHINK;

	SetNextThink( thinkTime, TRUE );
}


void CFuncTrackTrain :: Blocked( CBaseEntity *pOther )
{
	entvars_t	*pevOther = pOther->pev;

	// Blocker is on-ground on the train
	if ( FBitSet( pevOther->flags, FL_ONGROUND ) && VARS(pevOther->groundentity) == pev )
	{
		float deltaSpeed = fabs(pev->speed);
		if ( deltaSpeed > 50 )
			deltaSpeed = 50;
		if ( !pevOther->velocity.z )
			pevOther->velocity.z += deltaSpeed;
		return;
	}
	else
		pevOther->velocity = (pevOther->origin - pev->origin ).Normalize() * pev->dmg;

	ALERT( at_aiconsole, "TRAIN(%s): Blocked by %s (dmg:%.2f)\n", STRING(pev->targetname), STRING(pOther->pev->classname), pev->dmg );
	if ( pev->dmg <= 0 )
		return;
	// we can't hurt this thing, so we're not concerned with it
	pOther->TakeDamage(pev, pev, pev->dmg, DMG_CRUSH);
}


void CFuncTrackTrain :: Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
//	ALERT(at_debug, "TRAIN: use\n");

	if ( useType != USE_SET )
	{
		if ( !ShouldToggle( useType, (pev->speed != 0) ) )
			return;

		if ( pev->speed == 0 )
		{
			pev->speed = m_speed * m_dir;
			
			PostponeNext();
		}
		else
		{
			pev->speed = 0;
			UTIL_SetVelocity(this, g_vecZero); //LRC
			//pev->velocity = g_vecZero;
			if (!FBitSet(pev->spawnflags, SF_TRACKTRAIN_AVELOCITY))
				UTIL_SetAvelocity(this, g_vecZero); //LRC
			//pev->avelocity = g_vecZero;
			StopSound();
			SetThink( NULL );
		}
	}
	else
	{
		float delta = value;

		delta = ((int)(pev->speed * 4) / (int)m_speed)*0.25 + 0.25 * delta;
		if ( delta > 1 )
			delta = 1;
		else if ( delta < -1 )
			delta = -1;
		if ( pev->spawnflags & SF_TRACKTRAIN_FORWARDONLY )
		{
			if ( delta < 0 )
				delta = 0;
		}
		pev->speed = m_speed * delta;

		if ( pev->spawnflags & SF_TRACKTRAIN_AVEL_GEARS )
		{
			UTIL_SetAvelocity(this, m_vecMasterAvel * delta);
			//pev->avelocity = m_vecMasterAvel * delta; //LRC
		}

		PostponeNext();	
		ALERT( at_aiconsole, "TRAIN(%s), speed to %.2f\n", STRING(pev->targetname), pev->speed );
	}
}

#define TRAIN_STARTPITCH	60
#define TRAIN_MAXPITCH		200
#define TRAIN_MAXSPEED		1000	// approx max speed for sound pitch calculation

void CFuncTrackTrain :: StopSound( void )
{
	// if sound playing, stop it
	if (m_soundPlaying && pev->noise)
	{
		if (m_sounds) //LRC - flashy event-based method, for normal sounds.
		{
			unsigned short us_encode;
			unsigned short us_sound  = ( ( unsigned short )( m_sounds ) & 0x0007 ) << 12;
	
			us_encode = us_sound;

			PLAYBACK_EVENT_FULL( FEV_RELIABLE | FEV_UPDATE, edict(), m_usAdjustPitch, 0.0, 
				(float *)&g_vecZero, (float *)&g_vecZero, 0.0, 0.0, us_encode, 0, 1, 0 );
		}
		else
		{
			//LRC - down-to-earth method, for custom sounds.
			STOP_SOUND(ENT(pev), CHAN_STATIC, (char*)STRING(pev->noise));
		}

		EMIT_SOUND_DYN(ENT(pev), CHAN_ITEM, (char*)STRING(pev->noise1), m_flVolume, ATTN_NORM, 0, 100);
	}

	m_soundPlaying = 0;
}

// update pitch based on speed, start sound if not playing
// NOTE: when train goes through transition, m_soundPlaying should go to 0, 
// which will cause the looped sound to restart.

void CFuncTrackTrain :: UpdateSound( void )
{
	float flpitch;
	
	if (!pev->noise)
		return;

	flpitch = TRAIN_STARTPITCH + ( fabs( pev->speed ) * ( TRAIN_MAXPITCH - TRAIN_STARTPITCH ) / TRAIN_MAXSPEED );

	if (!m_soundPlaying)
	{
		// play startup sound for train
		EMIT_SOUND_DYN(ENT(pev), CHAN_ITEM, (char*)STRING(pev->noise2), m_flVolume, ATTN_NORM, 0, 100);
		EMIT_SOUND_DYN(ENT(pev), CHAN_STATIC, (char*)STRING(pev->noise), m_flVolume, ATTN_NORM, 0, (int) flpitch);
		m_soundPlaying = 1;
	} 
	else
	{
		if (m_sounds) //LRC - flashy event-based method, for normal sounds.
		{
			// volume 0.0 - 1.0 - 6 bits
			// m_sounds 3 bits
			// flpitch = 6 bits
			// 15 bits total

			unsigned short us_encode;
			unsigned short us_sound  = ( ( unsigned short )( m_sounds ) & 0x0007 ) << 12;
			unsigned short us_pitch  = ( ( unsigned short )( flpitch / 10.0 ) & 0x003f ) << 6;
			unsigned short us_volume = ( ( unsigned short )( m_flVolume * 40.0 ) & 0x003f );

			us_encode = us_sound | us_pitch | us_volume;

			PLAYBACK_EVENT_FULL( FEV_RELIABLE | FEV_UPDATE, edict(), m_usAdjustPitch, 0.0, 
				(float *)&g_vecZero, (float *)&g_vecZero, 0.0, 0.0, us_encode, 0, 0, 0 );
		}
		else
		{
			//LRC - down-to-earth method, for custom sounds.
			// update pitch
			EMIT_SOUND_DYN(ENT(pev), CHAN_STATIC, (char*)STRING(pev->noise), m_flVolume, ATTN_NORM, SND_CHANGE_PITCH, (int) flpitch);
		}
	}
}

void CFuncTrackTrain :: PostponeNext( void )
{
	UTIL_DesiredAction(this);
}

void CFuncTrackTrain :: DesiredAction( void ) // Next( void )
{
	float time = 0.5;

//	ALERT(at_console, "Next: pos %f %f %f, vel %f %f %f. Child pos %f %f %f, vel %f %f %f\n", pev->origin.x, pev->origin.y, pev->origin.z, pev->velocity.x, pev->velocity.y, pev->velocity.z, m_pChildMoveWith->pev->origin.x, m_pChildMoveWith->pev->origin.y, m_pChildMoveWith->pev->origin.z, m_pChildMoveWith->pev->velocity.x, m_pChildMoveWith->pev->velocity.y, m_pChildMoveWith->pev->velocity.z);
//	UTIL_DesiredInfo(this);

//	static float stime;
//	ALERT(at_console, "TRAIN: think delay = %f\n", gpGlobals->time - stime);
//	stime = gpGlobals->time;

	if ( !pev->speed )
	{
//		ALERT(at_console, "TRAIN: no speed\n");
		UTIL_SetVelocity(this, g_vecZero);
		DontThink();
		ALERT( at_aiconsole, "TRAIN(%s): Speed is 0\n", STRING(pev->targetname) );
		StopSound();
		return;
	}

//	if ( !m_ppath )
//		m_ppath = UTIL_FindEntityByTargetname( NULL, STRING(pev->target) );
	if ( !m_ppath )
	{	
//		ALERT(at_debug, "TRAIN: no path\n");
		UTIL_SetVelocity(this, g_vecZero);
		DontThink();
		ALERT( at_aiconsole, "TRAIN(%s): Lost path\n", STRING(pev->targetname) );
		StopSound();
		return;
	}

	UpdateSound();

	Vector nextPos = pev->origin;

	nextPos.z -= m_height;
	CPathTrack *pnext = m_ppath->LookAhead( &nextPos, pev->speed * 0.1, 1 );
	nextPos.z += m_height;

	UTIL_SetVelocity( this, (nextPos - pev->origin) * 10 ); //LRC
//	Vector vD = (nextPos - pev->origin) * 10;
//	ALERT(at_debug, "TRAIN: Set vel to (%f %f %f)\n", vD.x, vD.y, vD.z);
	//pev->velocity = (nextPos - pev->origin) * 10;
	Vector nextFront = pev->origin;

	nextFront.z -= m_height;
	if ( m_length > 0 )
		m_ppath->LookAhead( &nextFront, m_length, 0 );
	else
		m_ppath->LookAhead( &nextFront, 100, 0 );
	nextFront.z += m_height;

	if (!FBitSet(pev->spawnflags, SF_TRACKTRAIN_AVELOCITY)) //LRC
	{
		Vector delta = nextFront - pev->origin;

		Vector angles = UTIL_VecToAngles( delta );
		// The train actually points west
		angles.y += 180; //LRC, FIXME: add a 'built facing' field.

		// !!!  All of this crap has to be done to make the angles not wrap around, revisit this.
		FixupAngles( angles );
		FixupAngles( pev->angles );

		if ( !pnext || (delta.x == 0 && delta.y == 0) )
			angles = pev->angles;

		float vy, vx, vz;
		if ( !(pev->spawnflags & SF_TRACKTRAIN_NOPITCH) )
			vx = 10*UTIL_AngleDistance( angles.x, pev->angles.x );
		else
			vx = m_vecBaseAvel.x;

		if ( !(pev->spawnflags & SF_TRACKTRAIN_NOYAW) ) //LRC
			vy = 10*UTIL_AngleDistance( angles.y, pev->angles.y );
		else
			vy = m_vecBaseAvel.y;

		if ( m_flBank != 0 )
		{
			if ( pev->avelocity.y < -5 )
				vz = UTIL_AngleDistance( UTIL_ApproachAngle( -m_flBank, pev->angles.z, m_flBank*2 ), pev->angles.z);
			else if ( pev->avelocity.y > 5 )
				vz = UTIL_AngleDistance( UTIL_ApproachAngle( m_flBank, pev->angles.z, m_flBank*2 ), pev->angles.z);
			else
				vz = UTIL_AngleDistance( UTIL_ApproachAngle( 0, pev->angles.z, m_flBank*4 ), pev->angles.z) * 4;
		}
		else
		{
			vz = m_vecBaseAvel.z;
		}

		UTIL_SetAvelocity(this, Vector(vx, vy, vz));
		//pev->avelocity.y = vy;
		//pev->avelocity.x = vx;
	}
		
	if ( pnext )
	{
		if ( pnext != m_ppath )
		{
//			ALERT(at_debug, "TRAIN: new m_ppath %s, was %s. Origin %f %f %f\n", STRING(pnext->pev->targetname), STRING(m_ppath->pev->targetname), pev->origin.x, pev->origin.y, pev->origin.z);
			CPathTrack *pFire;
			if ( pev->speed >= 0 ) // check whether we're going forwards or backwards
				pFire = pnext;
			else 
				pFire = m_ppath;

			m_ppath = pnext;
			// Fire the pass target if there is one
			if ( pFire->pev->message )
			{
				FireTargets( STRING(pFire->pev->message), this, this, USE_TOGGLE, 0 );
				if ( FBitSet( pFire->pev->spawnflags, SF_PATH_FIREONCE ) )
					pFire->pev->message = 0;
			}

			if ( pFire->pev->spawnflags & SF_PATH_DISABLE_TRAIN )
				pev->spawnflags |= SF_TRACKTRAIN_NOCONTROL;

			//LRC is "match angle" set to "Yes"? If so, set the angle exactly, because we've reached the corner.
			if ( pFire->pev->armorvalue == PATHMATCH_YES && pev->spawnflags & SF_TRACKTRAIN_AVELOCITY )
			{
				Vector vTemp = pFire->pev->angles;
				vTemp.y -= 180; //the train is actually built facing west.
				UTIL_SetAngles(this, vTemp);
				//pev->angles = pFire->pev->angles;
				//pev->angles.y -= 180; //the train is actually built facing west.
			}

			float setting = ((int)(pev->speed*4) / (int)m_speed) / 4.0; //LRC - one of { 1, 0.75, 0.5, 0.25, 0, ... -1 }

			//LRC
			if ( pFire->pev->frags == PATHTURN_RESET )
			{
				pev->spawnflags &= ~(SF_TRACKTRAIN_AVEL_GEARS | SF_TRACKTRAIN_AVELOCITY);
			}
			else if ( pFire->pev->spawnflags & SF_PATH_AVELOCITY)
			{
				if ( pFire->pev->frags == PATHTURN_SET_MASTER )
				{
					m_vecMasterAvel = pFire->pev->avelocity;
					UTIL_SetAvelocity(this, m_vecMasterAvel * setting);
					//pev->avelocity = m_vecMasterAvel * setting;
					pev->spawnflags |= (SF_TRACKTRAIN_AVEL_GEARS | SF_TRACKTRAIN_AVELOCITY);
				}
				else if ( pFire->pev->frags == PATHTURN_SET )
				{
					UTIL_SetAvelocity(this, pFire->pev->avelocity);
					//pev->avelocity = pFire->pev->avelocity;
					pev->spawnflags |= SF_TRACKTRAIN_AVELOCITY;
					pev->spawnflags &= ~SF_TRACKTRAIN_AVEL_GEARS;
				}
			}

			CPathTrack* pDest; //LRC - the path_track we're heading for, after pFire.
			if (pev->speed > 0)
				pDest = pFire->GetNext();
			else
				pDest = pFire->GetPrevious();
//			ALERT(at_debug, "and pDest is %s\n", STRING(pDest->pev->targetname));

			//LRC
			// don't look at speed from target if it is 0 (uninitialized)
			if ( pFire->pev->speed != 0)
			{
				//ALERT( at_console, "TrackTrain setting is %d / %d = %.2f\n", (int)(pev->speed*4), (int)m_speed, setting );

				switch ( (int)(pFire->pev->armortype) )
				{
				case PATHSPEED_SET:
					// Don't override speed if under user control
					if (pev->spawnflags & SF_TRACKTRAIN_NOCONTROL)
					pev->speed = pFire->pev->speed;
					ALERT( at_aiconsole, "TrackTrain %s speed set to %4.2f\n", STRING(pev->targetname), pev->speed );
					break;
				case PATHSPEED_SET_MASTER:
					m_speed = pFire->pev->speed;
					pev->impulse = m_speed;
					pev->speed = setting * m_speed;
					ALERT( at_aiconsole, "TrackTrain %s master speed set to %4.2f\n", STRING(pev->targetname), pev->speed );
					break;
				case PATHSPEED_ACCEL:
					m_speed += pFire->pev->speed;
					pev->impulse = m_speed;
					pev->speed = setting * m_speed;
					ALERT( at_aiconsole, "TrackTrain %s speed accel to %4.2f\n", STRING(pev->targetname), pev->speed );
					break;
				case PATHSPEED_TIME:
					float distance = (pev->origin - pDest->pev->origin).Length();
					//ALERT(at_debug, "pFire=%s, distance=%.2f, ospeed=%.2f, nspeed=%.2f\n", STRING(pFire->pev->targetname), distance, pev->speed, distance / pFire->pev->speed);
					m_speed = distance / pFire->pev->speed;
					pev->impulse = m_speed;
					pev->speed = setting * m_speed;
					ALERT( at_aiconsole, "TrackTrain %s speed to %4.2f (timed)\n", STRING(pev->targetname), pev->speed );
					break;
				}
			}
			
			//LRC
			if (pDest->pev->armorvalue == PATHMATCH_YES)
			{
				pev->spawnflags |= SF_TRACKTRAIN_AVELOCITY | SF_TRACKTRAIN_AVEL_GEARS;
				Vector vTemp = pev->angles;
				FixupAngles( vTemp );
				UTIL_SetAngles(this, vTemp );
				Vector oDelta = pDest->pev->origin - pev->origin;
				Vector aDelta;
				if (setting > 0)
				{
					aDelta.x = UTIL_AngleDistance(pDest->pev->angles.x, pev->angles.x);
					aDelta.y = UTIL_AngleDistance(pDest->pev->angles.y, pev->angles.y);
					aDelta.z = UTIL_AngleDistance(pDest->pev->angles.z, pev->angles.z);
				}
				else
				{
					aDelta.x = UTIL_AngleDistance(pev->angles.x, pDest->pev->angles.x);
					aDelta.y = UTIL_AngleDistance(pev->angles.y, pDest->pev->angles.y);
					aDelta.z = UTIL_AngleDistance(pev->angles.z, pDest->pev->angles.z);
				}
				if (aDelta.y > 0) // the train is actually built facing west.
					aDelta.y -= 180;
				else
					aDelta.y += 180;
				float timeTaken = oDelta.Length() / m_speed;
				m_vecMasterAvel = aDelta / timeTaken;
				UTIL_SetAvelocity(this, setting * m_vecMasterAvel);
				//pev->avelocity = setting * m_vecMasterAvel;
			}
			//LRC- FIXME: add support, here, for a Teleport flag.
		}
//		else
//		{
//			ALERT(at_debug, "TRAIN: same pnext\n");
//		}
		SetThink(&CFuncTrackTrain :: PostponeNext );
		NextThink( time, TRUE );
	}
	else	// end of path, stop
	{
		Vector vecTemp; //LRC
		StopSound();
		vecTemp = (nextPos - pev->origin); //LRC

//		ALERT(at_debug, "TRAIN: path end\n");

//		UTIL_SetVelocity( this, (nextPos - pev->origin) * 10 ); //LRC
//		pev->velocity = (nextPos - pev->origin);
		if (!FBitSet(pev->spawnflags, SF_TRACKTRAIN_AVELOCITY)) //LRC
			UTIL_SetAvelocity(this, g_vecZero);
		//pev->avelocity = g_vecZero;
		float distance = vecTemp.Length(); //LRC
		//float distance = pev->velocity.Length();
		m_oldSpeed = pev->speed;


		pev->speed = 0;
		
		// Move to the dead end
		
		// Are we there yet?
		if ( distance > 0 )
		{
			// no, how long to get there?
			time = distance / m_oldSpeed;
			UTIL_SetVelocity( this, vecTemp * (m_oldSpeed / distance) ); //LRC
			//pev->velocity = pev->velocity * (m_oldSpeed / distance);
			SetThink(&CFuncTrackTrain :: DeadEnd );
			NextThink( time, FALSE );
		}
		else
		{
			UTIL_SetVelocity( this, vecTemp ); //LRC
			DeadEnd();
		}
	}
}


void CFuncTrackTrain::DeadEnd( void )
{
	// Fire the dead-end target if there is one
	CPathTrack *pTrack, *pNext;

	pTrack = m_ppath;

	ALERT( at_aiconsole, "TRAIN(%s): Dead end ", STRING(pev->targetname) );
	// Find the dead end path node
	// HACKHACK -- This is bugly, but the train can actually stop moving at a different node depending on it's speed
	// so we have to traverse the list to it's end.
	if ( pTrack )
	{
		if ( m_oldSpeed < 0 )
		{
			do
			{
				pNext = pTrack->ValidPath( pTrack->GetPrevious(), TRUE );
				if ( pNext )
					pTrack = pNext;
			} while ( pNext );
		}
		else
		{
			do
			{
				pNext = pTrack->ValidPath( pTrack->GetNext(), TRUE );
				if ( pNext )
					pTrack = pNext;
			} while ( pNext );
		}
	}

	UTIL_SetVelocity( this, g_vecZero ); //LRC
//	pev->velocity = g_vecZero;
	if (!FBitSet(pev->spawnflags, SF_TRACKTRAIN_AVELOCITY)) //LRC
		UTIL_SetAvelocity(this, g_vecZero );
	//pev->avelocity = g_vecZero;
	if ( pTrack )
	{
		ALERT( at_aiconsole, "at %s\n", STRING(pTrack->pev->targetname) );
		if ( pTrack->pev->netname )
			FireTargets( STRING(pTrack->pev->netname), this, this, USE_TOGGLE, 0 );
	}
	else
		ALERT( at_aiconsole, "\n" );
}


void CFuncTrackTrain :: SetControls( entvars_t *pevControls )
{
	Vector offset = pevControls->origin - pev->oldorigin;

	m_controlMins = pevControls->mins + offset;
	m_controlMaxs = pevControls->maxs + offset;
}


BOOL CFuncTrackTrain :: OnControls( entvars_t *pevTest )
{
	Vector offset = pevTest->origin - pev->origin;

	if ( pev->spawnflags & SF_TRACKTRAIN_NOCONTROL )
		return FALSE;

	// Transform offset into local coordinates
	UTIL_MakeVectors( pev->angles );
	Vector local;
	local.x = DotProduct( offset, gpGlobals->v_forward );
	local.y = -DotProduct( offset, gpGlobals->v_right );
	local.z = DotProduct( offset, gpGlobals->v_up );

	if ( local.x >= m_controlMins.x && local.y >= m_controlMins.y && local.z >= m_controlMins.z &&
		 local.x <= m_controlMaxs.x && local.y <= m_controlMaxs.y && local.z <= m_controlMaxs.z )
		 return TRUE;

	return FALSE;
}


void CFuncTrackTrain :: Find( void )
{
	m_ppath = (CPathTrack*)UTIL_FindEntityByTargetname( NULL, STRING(pev->target) );
	if ( !m_ppath )
		return;

	entvars_t *pevTarget = m_ppath->pev;
	if ( !FClassnameIs( pevTarget, "path_track" ) )
	{
		ALERT( at_error, "func_track_train must be on a path of path_track\n" );
		m_ppath = NULL;
		return;
	}

	Vector nextPos = pevTarget->origin;
	nextPos.z += m_height;

	Vector look = nextPos;
	look.z -= m_height;
	m_ppath->LookAhead( &look, m_length, 0 );
	look.z += m_height;

	Vector vTemp = UTIL_VecToAngles( look - nextPos );
	vTemp.y += 180;
	// The train actually points west
	//pev->angles.y += 180;

	if ( pev->spawnflags & SF_TRACKTRAIN_NOPITCH )
	{
		vTemp.x = 0;
		//pev->angles.x = 0;
	}

	UTIL_SetAngles(this, vTemp); //LRC

	UTIL_AssignOrigin ( this, nextPos ); //LRC
//	ALERT(at_console, "Train Find; origin %f %f %f\n", pev->origin.x, pev->origin.y, pev->origin.z);
    //UTIL_SetOrigin( this, nextPos );
	NextThink( 0.1, FALSE );
//	NextThink( 8, FALSE ); //LRC - What was this for?!
//	SetThink( Next );
	SetThink(&CFuncTrackTrain :: PostponeNext );
	pev->speed = m_startSpeed;

	UpdateSound();
}

void CFuncTrackTrain :: NearestPath( void )
{
	CBaseEntity *pTrack = NULL;
	CBaseEntity *pNearest = NULL;
	float dist, closest;

	closest = 1024;

	while ((pTrack = UTIL_FindEntityInSphere( pTrack, pev->origin, 1024 )) != NULL)
	{
		// filter out non-tracks
		if ( !(pTrack->pev->flags & (FL_CLIENT|FL_MONSTER)) && FClassnameIs( pTrack->pev, "path_track" ) )
		{
			dist = (pev->origin - pTrack->pev->origin).Length();
			if ( dist < closest )
			{
				closest = dist;
				pNearest = pTrack;
			}
		}
	}

	if ( !pNearest )
	{
		ALERT( at_debug, "Can't find a nearby track !!!\n" );
		SetThink(NULL);
		return;
	}

	ALERT( at_aiconsole, "TRAIN: %s, Nearest track is %s\n", STRING(pev->targetname), STRING(pNearest->pev->targetname) );
	// If I'm closer to the next path_track on this path, then it's my real path
	pTrack = ((CPathTrack *)pNearest)->GetNext();
	if ( pTrack )
	{
		if ( (pev->origin - pTrack->pev->origin).Length() < (pev->origin - pNearest->pev->origin).Length() )
			pNearest = pTrack;
	}

	m_ppath = (CPathTrack *)pNearest;

	if ( pev->speed != 0 )
	{
		NextThink( 0.1, FALSE );
		SetThink(&CFuncTrackTrain :: PostponeNext );
	}
}


void CFuncTrackTrain::OverrideReset( void )
{
	NextThink( 0.1, FALSE );
	SetThink(&CFuncTrackTrain:: NearestPath );
}


CFuncTrackTrain *CFuncTrackTrain::Instance( edict_t *pent )
{ 
	if ( FClassnameIs( pent, "func_tracktrain" ) )
		return (CFuncTrackTrain *)GET_PRIVATE(pent);
	return NULL;
}

//LRC
void CFuncTrackTrain :: StartSequence(CTrainSequence *pSequence)
{
	m_pSequence = pSequence;
//	ALERT(at_console, "Unset Retrigger (startsequence)\n");
	pev->spawnflags &= ~SF_TRAIN_WAIT_RETRIGGER;
	//...
}

//LRC
void CFuncTrackTrain :: StopSequence( )
{
	DontThink();
	m_pSequence = NULL;
	//...
}

// This class defines the volume of space that the player must stand in to control the train
class CFuncTrainControls : public CBaseEntity
{
public:
	virtual int	ObjectCaps( void ) { return CBaseEntity :: ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }
	void Spawn( void );
	void EXPORT Find( void );
};
LINK_ENTITY_TO_CLASS( func_traincontrols, CFuncTrainControls );


void CFuncTrainControls :: Find( void )
{
	CBaseEntity *pTarget = NULL;

	do 
	{
		pTarget = UTIL_FindEntityByTargetname( pTarget, STRING(pev->target) );
	} while ( pTarget && !FClassnameIs(pTarget->pev, "func_tracktrain") );

	if ( !pTarget )
	{
		ALERT( at_debug, "TrackTrainControls: No train %s\n", STRING(pev->target) );
		return;
	}

	CFuncTrackTrain *ptrain = (CFuncTrackTrain*)pTarget;
	ptrain->SetControls( pev );
	UTIL_Remove( this );
}


void CFuncTrainControls :: Spawn( void )
{
	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_NONE;
	SET_MODEL( ENT(pev), STRING(pev->model) );

	UTIL_SetSize( pev, pev->mins, pev->maxs );
	UTIL_SetOrigin( this, pev->origin );
	
	SetThink(&CFuncTrainControls :: Find );
	SetNextThink( 0 );
}



// ----------------------------------------------------------------------------
//
// Track changer / Train elevator
//
// ----------------------------------------------------------------------------

#define SF_TRACK_ACTIVATETRAIN		0x00000001
#define SF_TRACK_RELINK				0x00000002
#define SF_TRACK_ROTMOVE			0x00000004
#define SF_TRACK_STARTBOTTOM		0x00000008
#define SF_TRACK_DONT_MOVE			0x00000010

//
// This entity is a rotating/moving platform that will carry a train to a new track.
// It must be larger in X-Y planar area than the train, since it must contain the
// train within these dimensions in order to operate when the train is near it.
//

typedef enum { TRAIN_SAFE, TRAIN_BLOCKING, TRAIN_FOLLOWING } TRAIN_CODE;

class CFuncTrackChange : public CFuncPlatRot
{
public:
	void Spawn( void );
	void Precache( void );

//	virtual void	Blocked( void );
	virtual void	EXPORT GoUp( void );
	virtual void	EXPORT GoDown( void );

	void			KeyValue( KeyValueData* pkvd );
	void			Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void			EXPORT Find( void );
	TRAIN_CODE		EvaluateTrain( CPathTrack *pcurrent );
	void			UpdateTrain( Vector &dest );
	virtual void	HitBottom( void );
	virtual void	HitTop( void );
	void			Touch( CBaseEntity *pOther );
	virtual void	UpdateAutoTargets( int toggleState );
	virtual	BOOL	IsTogglePlat( void ) { return TRUE; }

	void			DisableUse( void ) { m_use = 0; }
	void			EnableUse( void ) { m_use = 1; }
	int				UseEnabled( void ) { return m_use; }

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	virtual void	OverrideReset( void );


	CPathTrack		*m_trackTop;
	CPathTrack		*m_trackBottom;

	CFuncTrackTrain	*m_train;

	int				m_trackTopName;
	int				m_trackBottomName;
	int				m_trainName;
	TRAIN_CODE		m_code;
	int				m_targetState;
	int				m_use;
};
LINK_ENTITY_TO_CLASS( func_trackchange, CFuncTrackChange );

TYPEDESCRIPTION	CFuncTrackChange::m_SaveData[] = 
{
	DEFINE_GLOBAL_FIELD( CFuncTrackChange, m_trackTop, FIELD_CLASSPTR ),
	DEFINE_GLOBAL_FIELD( CFuncTrackChange, m_trackBottom, FIELD_CLASSPTR ),
	DEFINE_GLOBAL_FIELD( CFuncTrackChange, m_train, FIELD_CLASSPTR ),
	DEFINE_GLOBAL_FIELD( CFuncTrackChange, m_trackTopName, FIELD_STRING ),
	DEFINE_GLOBAL_FIELD( CFuncTrackChange, m_trackBottomName, FIELD_STRING ),
	DEFINE_GLOBAL_FIELD( CFuncTrackChange, m_trainName, FIELD_STRING ),
	DEFINE_FIELD( CFuncTrackChange, m_code, FIELD_INTEGER ),
	DEFINE_FIELD( CFuncTrackChange, m_targetState, FIELD_INTEGER ),
	DEFINE_FIELD( CFuncTrackChange, m_use, FIELD_INTEGER ),
};

IMPLEMENT_SAVERESTORE( CFuncTrackChange, CFuncPlatRot );

void CFuncTrackChange :: Spawn( void )
{
	Setup();
	if ( FBitSet( pev->spawnflags, SF_TRACK_DONT_MOVE ) )
		m_vecPosition2.z = pev->origin.z;

	SetupRotation();

	if ( FBitSet( pev->spawnflags, SF_TRACK_STARTBOTTOM ) )
	{
		UTIL_SetOrigin (this, m_vecPosition2);
		m_toggle_state = TS_AT_BOTTOM;
		pev->angles = m_start;
		m_targetState = TS_AT_TOP;
	}
	else
	{
		UTIL_SetOrigin (this, m_vecPosition1);
		m_toggle_state = TS_AT_TOP;
		pev->angles = m_end;
		m_targetState = TS_AT_BOTTOM;
	}

	EnableUse();
	pev->nextthink = pev->ltime + 2.0;
	SetThink(&CFuncTrackChange :: Find );
	Precache();
}

void CFuncTrackChange :: Precache( void )
{
	// Can't trigger sound
	PRECACHE_SOUND( "buttons/button11.wav" );
	
	CFuncPlatRot::Precache();
}


// UNDONE: Filter touches before re-evaluating the train.
void CFuncTrackChange :: Touch( CBaseEntity *pOther )
{
#if 0
	TRAIN_CODE code;
	entvars_t *pevToucher = pOther->pev;
#endif
}



void CFuncTrackChange :: KeyValue( KeyValueData *pkvd )
{
	if ( FStrEq(pkvd->szKeyName, "train") )
	{
		m_trainName = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq(pkvd->szKeyName, "toptrack") )
	{
		m_trackTopName = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq(pkvd->szKeyName, "bottomtrack") )
	{
		m_trackBottomName = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
	{
		CFuncPlatRot::KeyValue( pkvd );		// Pass up to base class
	}
}


void CFuncTrackChange::OverrideReset( void )
{
	pev->nextthink = pev->ltime + 1.0;
	SetThink(&CFuncTrackChange:: Find );
}

void CFuncTrackChange :: Find( void )
{
	// Find track entities
	CBaseEntity *pTarget;

	pTarget = UTIL_FindEntityByTargetname( NULL, STRING(m_trackTopName) );
	if ( pTarget && FClassnameIs(pTarget->pev, "path_track"))
	{
		m_trackTop = (CPathTrack*)pTarget;
		pTarget = UTIL_FindEntityByTargetname( NULL, STRING(m_trackBottomName) );
		if ( pTarget && FClassnameIs(pTarget->pev, "path_track"))
		{
			m_trackBottom = (CPathTrack*)pTarget;
			pTarget = UTIL_FindEntityByTargetname( NULL, STRING(m_trainName) );
			if ( pTarget && FClassnameIs(pTarget->pev, "func_tracktrain"))
			{
				m_train = (CFuncTrackTrain*)pTarget;
				Vector center = (pev->absmin + pev->absmax) * 0.5;
				m_trackBottom = m_trackBottom->Nearest( center );
				m_trackTop = m_trackTop->Nearest( center );
				UpdateAutoTargets( m_toggle_state );
				SetThink( NULL );
				return;
			}
			else
				ALERT( at_error, "Can't find train for track change! %s\n", STRING(m_trainName) );
		}
		else
			ALERT( at_error, "Can't find bottom track for track change! %s\n", STRING(m_trackBottomName) );
	}
	else
		ALERT( at_error, "Can't find top track for track change! %s\n", STRING(m_trackTopName) );
}



TRAIN_CODE CFuncTrackChange :: EvaluateTrain( CPathTrack *pcurrent )
{
	// Go ahead and work, we don't have anything to switch, so just be an elevator
	if ( !pcurrent || !m_train )
		return TRAIN_SAFE;

	if ( m_train->m_ppath == pcurrent || (pcurrent->m_pprevious && m_train->m_ppath == pcurrent->m_pprevious) ||
		 (pcurrent->m_pnext && m_train->m_ppath == pcurrent->m_pnext) )
	{
		if ( m_train->pev->speed != 0 )
			return TRAIN_BLOCKING;

		Vector dist = pev->origin - m_train->pev->origin;
		float length = dist.Length2D();
		if ( length < m_train->m_length )		// Empirically determined close distance
			return TRAIN_FOLLOWING;
		else if ( length > (150 + m_train->m_length) )
			return TRAIN_SAFE;

		return TRAIN_BLOCKING;
	}
	
	return TRAIN_SAFE;
}

void CFuncTrackChange :: UpdateTrain( Vector &dest )
{
	float time;
	Vector vel = pev->velocity;

	if (m_pfnThink == &CFuncTrackChange::LinearMoveNow)
	{
		// we're going to do a LinearMoveNow: calculate the velocity it'll have
		Vector vecDest;
		if (m_pMoveWith)
			vecDest = m_vecFinalDest + m_pMoveWith->pev->origin;
		else
		    vecDest = m_vecFinalDest;
		Vector vecDestDelta = vecDest - pev->origin;
		time = vecDestDelta.Length() / m_flLinearMoveSpeed;
		vel = vecDestDelta / time;
	}
	else
	{
		time = (pev->nextthink - pev->ltime);
	}

	m_train->pev->velocity = vel;
	m_train->pev->avelocity = pev->avelocity;
	m_train->NextThink( m_train->pev->ltime + time, FALSE );

	// Attempt at getting the train to rotate properly around the origin of the trackchange
	if ( time <= 0 )
	{
//		ALERT(at_console, "no time, set trainvel %f %f %f\n", m_train->pev->velocity.x, m_train->pev->velocity.y, m_train->pev->velocity.z);
		return;
	}

	Vector offset = m_train->pev->origin - pev->origin;
	Vector delta = dest - pev->angles;
	// Transform offset into local coordinates
	UTIL_MakeInvVectors( delta, gpGlobals );
	Vector local;
	local.x = DotProduct( offset, gpGlobals->v_forward );
	local.y = DotProduct( offset, gpGlobals->v_right );
	local.z = DotProduct( offset, gpGlobals->v_up );

	local = local - offset;
	m_train->pev->velocity = vel + (local * (1.0/time));

//	ALERT(at_console, "set trainvel %f %f %f\n", m_train->pev->velocity.x, m_train->pev->velocity.y, m_train->pev->velocity.z);
}

void CFuncTrackChange :: GoDown( void )
{
	if ( m_code == TRAIN_BLOCKING )
		return;

	// HitBottom may get called during CFuncPlat::GoDown(), so set up for that
	// before you call GoDown()

	UpdateAutoTargets( TS_GOING_DOWN );
	// If ROTMOVE, move & rotate
	if ( FBitSet( pev->spawnflags, SF_TRACK_DONT_MOVE ) )
	{
		SetMoveDone(&CFuncTrackChange :: CallHitBottom );
		m_toggle_state = TS_GOING_DOWN;
		AngularMove( m_start, pev->speed );
	}
	else
	{
		CFuncPlat :: GoDown();
		SetMoveDone(&CFuncTrackChange :: CallHitBottom );

		Vector vecDest;
		if (m_pMoveWith)
		{
		    vecDest = m_vecFinalDest + m_pMoveWith->pev->origin;
		}
		else
		    vecDest = m_vecFinalDest;
		Vector vecDestDelta = vecDest - pev->origin;
		float flTravelTime = vecDestDelta.Length() / m_flLinearMoveSpeed;

		RotMove( m_start, flTravelTime );
//		RotMove( m_start, pev->nextthink - pev->ltime );
	}
	// Otherwise, rotate first, move second

	// If the train is moving with the platform, update it
	if ( m_code == TRAIN_FOLLOWING )
	{
		UpdateTrain( m_start );
		m_train->m_ppath = NULL;
	}
}


//
// Platform is at bottom, now starts moving up
//
void CFuncTrackChange :: GoUp( void )
{
	if ( m_code == TRAIN_BLOCKING )
		return;

	// HitTop may get called during CFuncPlat::GoUp(), so set up for that
	// before you call GoUp();

	UpdateAutoTargets( TS_GOING_UP );
	if ( FBitSet( pev->spawnflags, SF_TRACK_DONT_MOVE ) )
	{
		m_toggle_state = TS_GOING_UP;
		SetMoveDone(&CFuncTrackChange :: CallHitTop );
		AngularMove( m_end, pev->speed );
	}
	else
	{
		// If ROTMOVE, move & rotate
		CFuncPlat :: GoUp();
		SetMoveDone(&CFuncTrackChange :: CallHitTop );
		RotMove( m_end, pev->nextthink - pev->ltime );
	}
	
	// Otherwise, move first, rotate second

	// If the train is moving with the platform, update it
	if ( m_code == TRAIN_FOLLOWING )
	{
		UpdateTrain( m_end );
		m_train->m_ppath = NULL;
	}
}


// Normal track change
void CFuncTrackChange :: UpdateAutoTargets( int toggleState )
{
	if ( !m_trackTop || !m_trackBottom )
		return;

	if ( toggleState == TS_AT_TOP )
		ClearBits( m_trackTop->pev->spawnflags, SF_PATH_DISABLED );
	else
		SetBits( m_trackTop->pev->spawnflags, SF_PATH_DISABLED );

	if ( toggleState == TS_AT_BOTTOM )
		ClearBits( m_trackBottom->pev->spawnflags, SF_PATH_DISABLED );
	else
		SetBits( m_trackBottom->pev->spawnflags, SF_PATH_DISABLED );
}


void CFuncTrackChange :: Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if ( m_toggle_state != TS_AT_TOP && m_toggle_state != TS_AT_BOTTOM )
		return;

	// If train is in "safe" area, but not on the elevator, play alarm sound
	if ( m_toggle_state == TS_AT_TOP )
		m_code = EvaluateTrain( m_trackTop );
	else if ( m_toggle_state == TS_AT_BOTTOM )
		m_code = EvaluateTrain( m_trackBottom );
	else
		m_code = TRAIN_BLOCKING;
	if ( m_code == TRAIN_BLOCKING )
	{
		// Play alarm and return
		EMIT_SOUND(ENT(pev), CHAN_VOICE, "buttons/button11.wav", 1, ATTN_NORM);
		return;
	}

	// Otherwise, it's safe to move
	// If at top, go down
	// at bottom, go up

	DisableUse();
	if (m_toggle_state == TS_AT_TOP)
		GoDown();
	else
		GoUp();
}


//
// Platform has hit bottom.  Stops and waits forever.
//
void CFuncTrackChange :: HitBottom( void )
{
	CFuncPlatRot :: HitBottom();
	if ( m_code == TRAIN_FOLLOWING )
	{
//		UpdateTrain();
		m_train->SetTrack( m_trackBottom );
	}
	SetThink( NULL );
	pev->nextthink = -1;

	UpdateAutoTargets( m_toggle_state );

	EnableUse();
}


//
// Platform has hit bottom.  Stops and waits forever.
//
void CFuncTrackChange :: HitTop( void )
{
	CFuncPlatRot :: HitTop();
	if ( m_code == TRAIN_FOLLOWING )
	{
//		UpdateTrain();
		m_train->SetTrack( m_trackTop );
	}
	
	// Don't let the plat go back down
	SetThink( NULL );
	pev->nextthink = -1;
	UpdateAutoTargets( m_toggle_state );
	EnableUse();
}



class CFuncTrackAuto : public CFuncTrackChange
{
public:
	void			Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	virtual void	UpdateAutoTargets( int toggleState );
};

LINK_ENTITY_TO_CLASS( func_trackautochange, CFuncTrackAuto );

// Auto track change
void CFuncTrackAuto :: UpdateAutoTargets( int toggleState )
{
	CPathTrack *pTarget, *pNextTarget;

	if ( !m_trackTop || !m_trackBottom )
		return;

	if ( m_targetState == TS_AT_TOP )
	{
		pTarget = m_trackTop->GetNext();
		pNextTarget = m_trackBottom->GetNext();
	}
	else
	{
		pTarget = m_trackBottom->GetNext();
		pNextTarget = m_trackTop->GetNext();
	}
	if ( pTarget )
	{
		ClearBits( pTarget->pev->spawnflags, SF_PATH_DISABLED );
		if ( m_code == TRAIN_FOLLOWING && m_train && m_train->pev->speed == 0 )
			m_train->Use( this, this, USE_ON, 0 );
	}

	if ( pNextTarget )
		SetBits( pNextTarget->pev->spawnflags, SF_PATH_DISABLED );

}


void CFuncTrackAuto :: Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	CPathTrack *pTarget;

	if ( !UseEnabled() )
		return;

	if ( m_toggle_state == TS_AT_TOP )
		pTarget = m_trackTop;
	else if ( m_toggle_state == TS_AT_BOTTOM )
		pTarget = m_trackBottom;
	else
		pTarget = NULL;

	if ( FClassnameIs( pActivator->pev, "func_tracktrain" ) )
	{
		m_code = EvaluateTrain( pTarget );
		// Safe to fire?
		if ( m_code == TRAIN_FOLLOWING && m_toggle_state != m_targetState )
		{
			DisableUse();
			if (m_toggle_state == TS_AT_TOP)
				GoDown();
			else
				GoUp();
		}
	}
	else
	{
		if ( pTarget )
			pTarget = pTarget->GetNext();
		if ( pTarget && m_train->m_ppath != pTarget && ShouldToggle( useType, m_targetState ) )
		{
			if ( m_targetState == TS_AT_TOP )
				m_targetState = TS_AT_BOTTOM;
			else
				m_targetState = TS_AT_TOP;
		}

		UpdateAutoTargets( m_targetState );
	}
}


// ----------------------------------------------------------
//
//
// pev->speed is the travel speed
// pev->health is current health
// pev->max_health is the amount to reset to each time it starts

#define FGUNTARGET_START_ON			0x0001

class CGunTarget : public CBaseMonster
{
public:
	void			Spawn( void );
	void			Activate( void );
	void EXPORT		Next( void );
	void EXPORT		Start( void );
	void EXPORT		Wait( void );
	void			Stop( void );

	int				BloodColor( void ) { return DONT_BLEED; }
	int				Classify( void ) { return CLASS_MACHINE; }
	int				TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType );
	void			Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	Vector			BodyTarget( const Vector &posSrc ) { return pev->origin; }

	virtual int	ObjectCaps( void ) { return CBaseEntity :: ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }
	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );

	static	TYPEDESCRIPTION m_SaveData[];

private:
	BOOL			m_on;
};


LINK_ENTITY_TO_CLASS( func_guntarget, CGunTarget );

TYPEDESCRIPTION	CGunTarget::m_SaveData[] = 
{
	DEFINE_FIELD( CGunTarget, m_on, FIELD_BOOLEAN ),
};

IMPLEMENT_SAVERESTORE( CGunTarget, CBaseMonster );


void CGunTarget::Spawn( void )
{
	pev->solid		= SOLID_BSP;
	pev->movetype	= MOVETYPE_PUSH;

	UTIL_SetOrigin(this, pev->origin);
	SET_MODEL(ENT(pev), STRING(pev->model) );

	if ( pev->speed == 0 )
		pev->speed = 100;

	// Don't take damage until "on"
	pev->takedamage = DAMAGE_NO;
	pev->flags |= FL_MONSTER;

	m_on = FALSE;
	pev->max_health = pev->health;

	if ( pev->spawnflags & FGUNTARGET_START_ON )
	{
		SetThink(&CGunTarget:: Start );
		SetNextThink( 0.3 );
	}
}


void CGunTarget::Activate( void )
{
	CBaseEntity	*pTarg;

	// now find our next target
	pTarg = GetNextTarget();
	if ( pTarg )
	{
		m_hTargetEnt = pTarg;
		UTIL_SetOrigin( this, pTarg->pev->origin - (pev->mins + pev->maxs) * 0.5 );
	}
	CBaseMonster::Activate();
}


void CGunTarget::Start( void )
{
	Use( this, this, USE_ON, 0 );
}


void CGunTarget::Next( void )
{
	SetThink( NULL );

	m_hTargetEnt = GetNextTarget();
	CBaseEntity *pTarget = m_hTargetEnt;
	
	if ( !pTarget )
	{
		Stop();
		return;
	}
	SetMoveDone(&CGunTarget:: Wait );
	LinearMove( pTarget->pev->origin - (pev->mins + pev->maxs) * 0.5, pev->speed );
}


void CGunTarget::Wait( void )
{
	CBaseEntity *pTarget = m_hTargetEnt;
	
	if ( !pTarget )
	{
		Stop();
		return;
	}

	// Fire the pass target if there is one
	if ( pTarget->pev->message )
	{
		FireTargets( STRING(pTarget->pev->message), this, this, USE_TOGGLE, 0 );
		if ( FBitSet( pTarget->pev->spawnflags, SF_CORNER_FIREONCE ) )
			pTarget->pev->message = 0;
	}
		
	m_flWait = pTarget->GetDelay();

	pev->target = pTarget->pev->target;
	SetThink(&CGunTarget:: Next );
	if (m_flWait != 0)
	{// -1 wait will wait forever!		
		SetNextThink( m_flWait );
	}
	else
	{
		Next();// do it RIGHT now!
	}
}


void CGunTarget::Stop( void )
{
	pev->velocity = g_vecZero;
	DontThink();
	pev->takedamage = DAMAGE_NO;
}


int	CGunTarget::TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	if ( pev->health > 0 )
	{
		pev->health -= flDamage;
		if ( pev->health <= 0 )
		{
			pev->health = 0;
			Stop();
			if ( pev->message )
				FireTargets( STRING(pev->message), this, this, USE_TOGGLE, 0 );
		}
	}
	return 0;
}


void CGunTarget::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if ( !ShouldToggle( useType, m_on ) )
		return;

	if ( m_on )
	{
		Stop();
	}
	else
	{
		pev->takedamage = DAMAGE_AIM;
		m_hTargetEnt = GetNextTarget();
		if ( m_hTargetEnt == NULL )
			return;
		pev->health = pev->max_health;
		Next();
	}
}

//============================================================================
//LRC - Scripted Train Sequence
//============================================================================

#define DIRECTION_NONE 0
#define DIRECTION_FORWARDS 1
#define DIRECTION_BACKWARDS 2
#define DIRECTION_STOP 3
#define DIRECTION_DESTINATION 4

#define SF_TRAINSEQ_REMOVE 2
#define SF_TRAINSEQ_DIRECT 4
#define SF_TRAINSEQ_DEBUG  8

LINK_ENTITY_TO_CLASS( scripted_trainsequence, CTrainSequence );

TYPEDESCRIPTION	CTrainSequence::m_SaveData[] = 
{
	DEFINE_FIELD( CTrainSequence, m_iszEntity, FIELD_STRING ),
	DEFINE_FIELD( CTrainSequence, m_iszDestination, FIELD_STRING ),
	DEFINE_FIELD( CTrainSequence, m_pDestination, FIELD_CLASSPTR),
	DEFINE_FIELD( CTrainSequence, m_iszTerminate, FIELD_STRING ),
	DEFINE_FIELD( CTrainSequence, m_fDuration, FIELD_FLOAT ),
	DEFINE_FIELD( CTrainSequence, m_iDirection, FIELD_INTEGER ),
	DEFINE_FIELD( CTrainSequence, m_iPostDirection, FIELD_INTEGER ),
	DEFINE_FIELD( CTrainSequence, m_pTrain, FIELD_CLASSPTR),
	DEFINE_FIELD( CTrainSequence, m_pTrackTrain, FIELD_CLASSPTR),
};

IMPLEMENT_SAVERESTORE( CTrainSequence, CBaseEntity );

int	CTrainSequence :: ObjectCaps( void ) 
{
	return (CBaseEntity::ObjectCaps() & ~FCAP_ACROSS_TRANSITION);
}

void CTrainSequence :: KeyValue( KeyValueData *pkvd )
{
	if (FStrEq(pkvd->szKeyName, "m_iDirection"))
	{
		m_iDirection = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "m_iPostDirection"))
	{
		m_iPostDirection = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "m_iszEntity"))
	{
		m_iszEntity = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "m_iszDestination"))
	{
		m_iszDestination = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "m_iszTerminate"))
	{
		m_iszTerminate = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CBaseEntity::KeyValue( pkvd );
}

void CTrainSequence :: Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
//	ALERT(at_console, "SeqUse\n");
	if (!ShouldToggle(useType))
	{
//		ALERT(at_console, "SeqUse, don't toggle\n");
		return;
	}
	else
	{
//		ALERT(at_console, "SeqUse ok\n");
	}

	if (GetState() == STATE_OFF)
	{
		// start the sequence, take control of the train

		CBaseEntity* pEnt = UTIL_FindEntityByTargetname(NULL, STRING(m_iszEntity), pActivator);
		if (pEnt)
		{
			m_pDestination = UTIL_FindEntityByTargetname(NULL, STRING(m_iszDestination), pActivator);

			if (pev->spawnflags & SF_TRAINSEQ_DEBUG)
			{
				ALERT(at_console, "trainsequence \"%s\" found train \"%s\"", STRING(pev->targetname), STRING(pEnt->pev->targetname));
				if (m_pDestination)
					ALERT(at_console, "found destination %s\n", STRING(m_pDestination->pev->targetname));
				else
					ALERT(at_console, "missing destination\n");
			}

			if (FStrEq(STRING(pEnt->pev->classname), "func_train"))
			{
				CFuncTrain *pTrain = (CFuncTrain*)pEnt;

				// check whether it's being controlled by another sequence
				if (pTrain->m_pSequence)
				{
//					ALERT(at_console, "SeqUse: Train sequence already set\n");
					return;
				}
//				ALERT(at_console, "SeqUse: Train takecontrol\n");

				//ok, we can now take control of it.
				pTrain->StartSequence(this);
				m_pTrain = pTrain;

				if (pev->spawnflags & SF_TRAINSEQ_DIRECT)
				{
					pTrain->pev->target = m_pDestination->pev->targetname;
					pTrain->Next();
				}
				else
				{
					int iDir = DIRECTION_NONE;

					switch (m_iDirection)
					{
					case DIRECTION_DESTINATION:
						if (m_pDestination)
						{
							Vector vecFTemp, vecBTemp;
							CBaseEntity *pTrainDest = UTIL_FindEntityByTargetname(NULL, STRING(pTrain->pev->message));
							float fForward;
							if (pTrain->pev->spawnflags & SF_TRAIN_SETORIGIN)
								fForward = (pTrainDest->pev->origin - pTrain->pev->origin).Length();
							else
								fForward = (pTrainDest->pev->origin - (pTrain->pev->origin + (pTrain->pev->maxs + pTrain->pev->mins)*0.5)).Length();
							float fBackward = -fForward; // the further back from the TrainDest entity we are, the shorter the backward distance.
							CBaseEntity *pCurForward = pTrainDest;
							CBaseEntity *pCurBackward = m_pDestination;
							vecFTemp = pCurForward->pev->origin;
							vecBTemp = pCurBackward->pev->origin;
							int loopbreaker = 10;
							while(iDir == DIRECTION_NONE)
							{
								if (pCurForward)
								{
									fForward += (pCurForward->pev->origin - vecFTemp).Length();
									vecFTemp = pCurForward->pev->origin;

	//								ALERT(at_console, "SeqUse: Forward %f %s (%p == %p)\n", fForward, STRING(pCurForward->pev->targetname), pCurForward, m_pDestination);
									// if we've finished tracing the forward line
									if (pCurForward == m_pDestination)
									{
										// if the backward line is longest
										if (fBackward >= fForward || pCurBackward == NULL)
											iDir = DIRECTION_FORWARDS;
									}
									else
									{
										pCurForward = pCurForward->GetNextTarget();
									}
								}
								if (pCurBackward)
								{
									fBackward += (pCurBackward->pev->origin - vecBTemp).Length();
									vecBTemp = pCurBackward->pev->origin;

	//								ALERT(at_console, "SeqUse: Backward %f %s (%p == %p)\n", fBackward, STRING(pCurBackward->pev->targetname), pCurBackward, pTrainDest);
									// if we've finished tracng the backward line
									if (pCurBackward == pTrainDest)
									{
										// if the forward line is shorter
										if (fBackward < fForward || pCurForward == NULL)
											iDir = DIRECTION_BACKWARDS;
									}
									else
									{
										pCurBackward = pCurBackward->GetNextTarget();
									}
								}
								loopbreaker--;
								if (loopbreaker <= 0)
									iDir = DIRECTION_STOP;
							}
						}
						else
						{
							iDir = DIRECTION_STOP;
						}
						break;
					case DIRECTION_FORWARDS: iDir = DIRECTION_FORWARDS; break;
					case DIRECTION_BACKWARDS: iDir = DIRECTION_BACKWARDS; break;
					case DIRECTION_STOP: iDir = DIRECTION_STOP; break;
					}
					
	//				ALERT(at_console, "SeqUse: iDir is %d\n", iDir);

					if (iDir == DIRECTION_BACKWARDS && !(pTrain->pev->spawnflags & SF_TRAIN_REVERSE))
					{
//						ALERT(at_console, "Reversing from \"%s\" \"%s\"\n", STRING(pTrain->pev->target), STRING(pTrain->pev->message));
						// change direction
						pTrain->pev->spawnflags |= SF_TRAIN_REVERSE;

						CBaseEntity *pSearch = m_pDestination;
						while (pSearch)
						{
							if (FStrEq(STRING(pSearch->pev->target), STRING(pTrain->pev->message)))
							{
	//							ALERT(at_console, "SeqUse reverse: pSearch is %s\n", STRING(pSearch->pev->targetname));
								CBaseEntity *pTrainTarg = pSearch->GetNextTarget();
								if (pTrainTarg)
									pTrain->pev->enemy = pTrainTarg->edict();
								else
									pTrain->pev->enemy = NULL;
								pTrain->pev->target = pSearch->pev->targetname;
								break;
							}
							pSearch = pSearch->GetNextTarget();
						}

						if (!pSearch)
						{
							// this shouldn't happen.
							ALERT(at_error, "Found no path to reach destination! (train has t %s, m %s; dest is %s)\n", STRING(pTrain->pev->target), STRING(pTrain->pev->message), STRING(m_pDestination->pev->targetname));
							return;
						}
						pTrain->m_pevCurrentTarget = NULL; // we haven't reached the corner, so don't use its settings
//						if (pTrain->pev->enemy)
//							ALERT(at_console, "SeqUse: pTrain target %s, enemy %s\n", STRING(pTrain->pev->target), STRING(pTrain->pev->enemy->v.targetname));
//						else
//							ALERT(at_console, "SeqUse: pTrain target %s, no enemy\n", STRING(pTrain->pev->target));
						pTrain->Next();
					}
					else if (iDir == DIRECTION_FORWARDS)
					{
//						ALERT(at_console, "Dir_Forwards targ %s\n", STRING(pTrain->pev->target));
						pTrain->pev->target = pTrain->pev->message;
						pTrain->Next();
					}
					else if (iDir == DIRECTION_STOP)
					{
						SetNextThink(0.1);
						SetThink(&CTrainSequence ::EndThink);
						return;
					}
				}
			}
			else if (FStrEq(STRING(pEnt->pev->classname), "func_tracktrain"))
			{
				CFuncTrackTrain *pTrackTrain = (CFuncTrackTrain*)pEnt;

				// check whether it's being controlled by another sequence
				if (pTrackTrain->m_pSequence)
					return;

				//ok, we can now take control of it.
				pTrackTrain->StartSequence(this);
				m_pTrackTrain = pTrackTrain;
			}
			else
			{
				ALERT(at_error, "scripted_trainsequence %s can't affect %s \"%s\": not a train!\n", STRING(pev->targetname), STRING(pEnt->pev->classname), STRING(pEnt->pev->targetname));
				return;
			}
		}
		else // no entity with that name
		{
			ALERT(at_error, "Missing train \"%s\" for scripted_trainsequence %s!\n", STRING(m_iszEntity), STRING(pev->targetname));
			return;
		}

		// if we got here, we've set up a sequence successfully.
		// do the rest of the setup.
		if (m_fDuration)
		{
			SetThink(&CTrainSequence :: TimeOutThink );
			SetNextThink( m_fDuration );
		}

//		if (m_pTrain)
//			ALERT(at_console, "m_pTrain nextthink %f, flags %f\n", STRING(m_pTrain->pev->nextthink), m_pTrain->m_iLFlags);
	}
	else // prematurely end the sequence
	{
		//disable the other end conditions
		DontThink();

		// release control of the train
		StopSequence();
	}
}

void CTrainSequence :: ArrivalNotify()
{
//	ALERT(at_console, "ArrivalNotify\n");
	// check whether the current path is our destination,
	// and end the sequence if it is.
	if (m_pTrain)
	{
		if (m_pTrain->m_pevCurrentTarget == m_pDestination->pev)
		{
			// we've reached the destination. Stop now.
//			ALERT(at_console, "ArrivalNotify %s stop\n", STRING(pev->targetname));
			EndThink();
		}
		else
		{
//			ALERT(at_console, "ArrivalNotify %s continue\n", STRING(pev->targetname));
		}
	}
	else if (m_pTrackTrain)
	{
		//...
	}
	else
	{
		ALERT(at_error, "scripted_trainsequence: ArrivalNotify without a train!?\n");
		return; // this shouldn't happen.
	}
}

void CTrainSequence :: EndThink()
{
	//the sequence has expired. Release control.
	StopSequence();
	FireTargets(STRING(pev->target), this, this, USE_TOGGLE, 0);
}

void CTrainSequence :: TimeOutThink()
{
	//the sequence has timed out. Release control.
	StopSequence();
	FireTargets(STRING(pev->netname), this, this, USE_TOGGLE, 0);
}

void CTrainSequence :: StopSequence()
{
	if (m_pTrain)
	{
//		ALERT(at_console, "StopSequence called\n");
		//stuff...
		m_pTrain->StopSequence();
		m_pTrain = NULL;

		if (FBitSet(pev->spawnflags, SF_TRAINSEQ_REMOVE))
			UTIL_Remove( this );
	}
	else if (m_pTrackTrain)
	{
		//stuff...
	}
	else
	{
		ALERT(at_error, "scripted_trainsequence: StopSequence without a train!?\n");
		return; // this shouldn't happen.
	}
	FireTargets(STRING(m_iszTerminate), this, this, USE_TOGGLE, 0);
}
