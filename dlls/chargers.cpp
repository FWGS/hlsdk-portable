//
// Decay entities (chargers: health and hev; retinal scanner)
//

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"schedule.h"
#include	"defaultai.h"
#include	"scripted.h"
#include	"animation.h"
#include	"soundent.h"
#include    "actanimating.h"

//
//	EYE RETINAL SCANNER
//

#define EYESCANNER_HIDE_TIME 3
#define EYESCANNER_SCAN_LOOPS 3

class CEyeScanner : public CActAnimating
{
public:
    void    Precache( void );
    void    Spawn( void );
	void	Touch( CBaseEntity *pOther );
	void	EXPORT ScannerThink( void );
	void	EXPORT UseThink( void );
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void KeyValue( KeyValueData *pkvd );

	virtual int	ObjectCaps( void ) { return CActAnimating :: ObjectCaps() | /*FCAP_CONTINUOUS_USE | */FCAP_IMPULSE_USE; }

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	BOOL m_bState;
	BOOL m_bIsKeymaker;
	int m_iCheckFrame;
	int m_iCheckLoop;
	int m_iszTargetUnlocked;
	int m_iszTargetLocked;
	int m_iszKeymaker;			// The One's who unlocks name :)
};

LINK_ENTITY_TO_CLASS(item_eyescanner, CEyeScanner);

//
// Implementing save/restore
//

TYPEDESCRIPTION	CEyeScanner::m_SaveData[] = 
{
	DEFINE_FIELD( CEyeScanner, m_bState, FIELD_BOOLEAN ),
	DEFINE_FIELD( CEyeScanner, m_bIsKeymaker, FIELD_BOOLEAN ),
	DEFINE_FIELD( CEyeScanner, m_iCheckLoop, FIELD_INTEGER ),
	DEFINE_FIELD( CEyeScanner, m_iCheckFrame, FIELD_INTEGER ),
	DEFINE_FIELD( CEyeScanner, m_iszTargetUnlocked, FIELD_STRING ),
	DEFINE_FIELD( CEyeScanner, m_iszTargetLocked, FIELD_STRING ),
	DEFINE_FIELD( CEyeScanner, m_iszKeymaker, FIELD_STRING ),
};
IMPLEMENT_SAVERESTORE( CEyeScanner, CActAnimating );

//
// common functions
//

#define mdlScanner	   "models/eye_scanner.mdl"
#define	sndScannerBeep "buttons/blip1.wav"
#define sndScannerOpen "buttons/blip2.wav"
#define sndScannerDeny "buttons/button11.wav"

void CEyeScanner::Precache( void )
{
	PRECACHE_MODEL( mdlScanner );
	PRECACHE_SOUND( sndScannerBeep );
	PRECACHE_SOUND( sndScannerOpen );
	PRECACHE_SOUND( sndScannerDeny );
}

void CEyeScanner::Spawn( void )
{
	Precache( );

 	SET_MODEL(ENT(pev), mdlScanner );  

	pev->movetype	= MOVETYPE_NONE;
	pev->solid		= SOLID_TRIGGER;

	UTIL_SetSize( pev, Vector(-8,-8,0), Vector(8,8,32));
	SetActivity( ACT_CROUCHIDLE );

	SetThink ( ScannerThink );
	pev->nextthink = gpGlobals->time + 0.1;
	//pev->frame = RANDOM_FLOAT(0,255);
}

void CEyeScanner::KeyValue( KeyValueData *pkvd )
{
	if (FStrEq(pkvd->szKeyName, "unlocked_target"))
	{
		m_iszTargetUnlocked = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "locked_target"))
	{
		m_iszTargetLocked = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "unlockersname"))
	{
		m_iszKeymaker = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CActAnimating::KeyValue( pkvd );
	//m_iReactivate = atoi(pkvd->szValue);
}

void CEyeScanner::ScannerThink( void )
{
	StudioFrameAdvance();
	pev->nextthink = gpGlobals->time + 0.1;

	switch( GetActivity() )
	{
	case ACT_CROUCH:	// deactivate in progress
		if ( m_fSequenceFinished )
			SetActivity( ACT_CROUCHIDLE );
		break;

	case ACT_CROUCHIDLE: // deactivated
		    pev->skin = 0;
			m_bState = FALSE;
		break;

	case ACT_STAND: // activate in progress
		m_bState = TRUE;
		if ( m_fSequenceFinished ) 
			SetActivity( ACT_IDLE );
		break;

	case ACT_IDLE: // activated
		if ( gpGlobals->time > pev->dmgtime )
			SetActivity( ACT_CROUCH );
		break;
	default:
		break;
	}
}

void CEyeScanner :: Touch( CBaseEntity *pOther )
{/*
	int iClass = pOther->Classify();
	//if ( pOther->IsPlayer() )
	if (iClass == CLASS_PLAYER || iClass == CLASS_PLAYER_ALLY)
	{
		pev->dmgtime = gpGlobals->time + EYESCANNER_HIDE_TIME;
		if ( GetActivity() == ACT_CROUCH || GetActivity() == ACT_CROUCHIDLE )
		{
			SetActivity( ACT_STAND );
		}
	}*/
}

void CEyeScanner :: Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if (GetActivity() == ACT_CROUCHIDLE)
	{
		pev->dmgtime = gpGlobals->time + EYESCANNER_HIDE_TIME;
		//if ( GetActivity() == ACT_CROUCH || GetActivity() == ACT_CROUCHIDLE )
		//{
			SetActivity( ACT_STAND );
		//}
	}

	//if (m_bState == FALSE)
	//	return;

	int m_iszCallerName;
	if (pActivator)
		m_iszCallerName = pActivator->pev->targetname;
	else
		m_iszCallerName = pCaller->pev->targetname;

	//ALERT( at_console, "caller name is %s\n", STRING(m_iszCallerName) );

	if (m_iszKeymaker)	// if unlocker's name is specified then do check activator's name
		m_bIsKeymaker = !strcmp(STRING(m_iszCallerName), STRING(m_iszKeymaker));
	else				// otherwise open for everyone
		m_bIsKeymaker = true;

	m_iCheckLoop = 0;
	m_iCheckFrame = 1;
	m_bState = FALSE;
	SetThink( UseThink );
	pev->nextthink = gpGlobals->time + 1.5;
}

void CEyeScanner :: UseThink( void )
{
	pev->nextthink = gpGlobals->time + 0.15;
 	// buttons/blip1.wav
    EMIT_SOUND( ENT(pev), CHAN_ITEM, sndScannerBeep, 0.85, ATTN_NORM );

	if (m_iCheckLoop == EYESCANNER_SCAN_LOOPS)
	{
		// scan process finished - do something!
		if (m_bIsKeymaker)
		{
			FireTargets( STRING(m_iszTargetUnlocked), this, this, USE_TOGGLE, 0 );
			EMIT_SOUND( ENT(pev), CHAN_ITEM, sndScannerOpen, 0.85, ATTN_NORM );
		} else
		{
			FireTargets( STRING(m_iszTargetLocked), this, this, USE_TOGGLE, 0 );
			EMIT_SOUND( ENT(pev), CHAN_ITEM, sndScannerDeny, 0.85, ATTN_NORM );
		}

		SetThink( ScannerThink );
		pev->skin = 0;
		return;
	}

	// if we are in last animation frame, skip to first one
	if (m_iCheckFrame == 4)
	{
		m_iCheckFrame = 1;
		m_iCheckLoop++;
	}

	pev->skin = m_iCheckFrame;
	m_iCheckFrame++;
}