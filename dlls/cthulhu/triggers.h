
#ifndef TRIGGERS_H
#define TRIGGERS_H

class CFrictionModifier : public CBaseEntity
{
public:
	void		Spawn( void );
	void		KeyValue( KeyValueData *pkvd );
	void EXPORT	ChangeFriction( CBaseEntity *pOther );
	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );

	virtual int	ObjectCaps( void ) { return CBaseEntity :: ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }

	static	TYPEDESCRIPTION m_SaveData[];

	float		m_frictionFraction;		// Sorry, couldn't resist this name :)
};

///////////////////////////////////////////////////////////////

class CAutoTrigger : public CBaseDelay
{
public:
	void KeyValue( KeyValueData *pkvd );
	void Spawn( void );
	void Precache( void );
	void Think( void );

	int ObjectCaps( void ) { return CBaseDelay::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }
	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );

	static	TYPEDESCRIPTION m_SaveData[];

private:
	int			m_globalstate;
	USE_TYPE	triggerType;
};

///////////////////////////////////////////////////////////////

class CTriggerRelay : public CBaseDelay
{
public:
	void KeyValue( KeyValueData *pkvd );
	void Spawn( void );
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );

	int ObjectCaps( void ) { return CBaseDelay::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }
	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );

	static	TYPEDESCRIPTION m_SaveData[];

private:
	USE_TYPE	m_triggerType;
	int			m_sMaster;
	string_t	m_iszAltTarget;
};

//**********************************************************
// The Multimanager Entity - when fired, will fire up to 16 targets 
// at specified times.
// FLAG:		THREAD (create clones when triggered)
// FLAG:		CLONE (this is a clone for a threaded execution)

#define SF_MULTIMAN_CLONE		0x80000000
#define SF_MULTIMAN_SAMETRIG	0x40000000
#define SF_MULTIMAN_TRIGCHOSEN	0x20000000

#define SF_MULTIMAN_THREAD		0x00000001
#define SF_MULTIMAN_LOOP		0x00000004
#define SF_MULTIMAN_ONLYONCE	0x00000008
#define SF_MULTIMAN_SPAWNFIRE	0x00000010
#define SF_MULTIMAN_DEBUG		0x00000020

#define MM_MODE_CHOOSE			1
#define MM_MODE_PERCENT			2
#define MM_MODE_SIMULTANEOUS	3

class CMultiManager : public CBaseEntity//Toggle
{
public:
	void KeyValue( KeyValueData *pkvd );
	void Spawn ( void );
	void EXPORT UseThink ( void );
	void EXPORT ManagerThink ( void );
	void EXPORT ManagerUse   ( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );

#if _DEBUG
	void EXPORT ManagerReport( void );
#endif

	BOOL		HasTarget( string_t targetname );
	
	int ObjectCaps( void ) { return CBaseEntity::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );

	static	TYPEDESCRIPTION m_SaveData[];

	STATE	m_iState;
	virtual STATE GetState( void ) { return m_iState; };

	int		m_cTargets;	// the total number of targets in this manager's fire list.
	int		m_index;	// Current target
	float	m_startTime;// Time we started firing
	int		m_iTargetName	[ MAX_MULTI_TARGETS ];// list if indexes into global string array
	float	m_flTargetDelay [ MAX_MULTI_TARGETS ];// delay (in seconds) from time of manager fire to target fire

	float	m_flWait; //LRC- minimum length of time to wait
	float	m_flMaxWait; //LRC- random, maximum length of time to wait
	string_t m_sMaster; //LRC- master
	int		m_iMode; //LRC- 0 = timed, 1 = pick random, 2 = each random
	int		m_iszThreadName; //LRC
	int		m_iszLocusThread; //LRC

	EHANDLE m_hActivator;
private:
	USE_TYPE	m_triggerType; //LRC
	inline BOOL IsClone( void ) { return (pev->spawnflags & SF_MULTIMAN_CLONE) ? TRUE : FALSE; }
	inline BOOL ShouldClone( void ) 
	{ 
		if ( IsClone() )
			return FALSE;

		return (pev->spawnflags & SF_MULTIMAN_THREAD) ? TRUE : FALSE; 
	}
	
	CMultiManager *Clone( void );
};

///////////////////////////////////////////////////////////////////////////////

#define SF_SWATCHER_SENDTOGGLE		  0x1
#define SF_SWATCHER_DONTSEND_ON		  0x2
#define SF_SWATCHER_DONTSEND_OFF	  0x4
#define SF_SWATCHER_NOTON			  0x8
#define SF_SWATCHER_OFF				 0x10
#define SF_SWATCHER_TURN_ON			 0x20
#define SF_SWATCHER_TURN_OFF		 0x40
#define SF_SWATCHER_IN_USE			 0x80
#define SF_SWATCHER_VALID			0x200

#define SWATCHER_LOGIC_AND	0
#define SWATCHER_LOGIC_OR	1
#define SWATCHER_LOGIC_NAND	2
#define SWATCHER_LOGIC_NOR	3
#define SWATCHER_LOGIC_XOR	4
#define SWATCHER_LOGIC_XNOR	5

class CStateWatcher : public CBaseToggle
{
public:
	void Spawn ( void );
	void EXPORT Think ( void );
	void KeyValue( KeyValueData *pkvd );
	virtual STATE GetState( void );
	virtual STATE GetState( CBaseEntity *pActivator );
	virtual int	ObjectCaps( void ) { return CBaseEntity :: ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );

	static	TYPEDESCRIPTION m_SaveData[];

	int		m_fLogic;	// Logic by which to combine the targets
	int		m_cTargets;	// the total number of targets in this manager's fire list.
	int		m_iTargetName	[ MAX_MULTI_TARGETS ];// list of indexes into global string array
//	CBaseEntity*	m_pTargetEnt	[ MAX_MULTI_TARGETS ];

	BOOL	EvalLogic ( CBaseEntity *pEntity );
};

#define SF_WRCOUNT_FIRESTART	0x0001
#define SF_WRCOUNT_STARTED		0x8000
class CWatcherCount : public CBaseToggle
{
public:
	void Spawn ( void );
	void EXPORT Think ( void );
	virtual STATE GetState( void ) { return (pev->spawnflags & SF_SWATCHER_VALID)?STATE_ON:STATE_OFF; };
	virtual int	ObjectCaps( void ) { return CBaseEntity :: ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }
};

///////////////////////////////////////////////////////////////////////////////

// Flags to indicate masking off various render parameters that are normally copied to the targets
#define SF_RENDER_MASKFX		(1<<0)
#define SF_RENDER_MASKAMT		(1<<1)
#define SF_RENDER_MASKMODE		(1<<2)
#define SF_RENDER_MASKCOLOR		(1<<3)
//LRC
#define SF_RENDER_KILLTARGET	(1<<5)
#define SF_RENDER_ONLYONCE		(1<<6)


//LRC-  RenderFxFader, a subsidiary entity for RenderFxManager
class CRenderFxFader : public CBaseEntity
{
public:
	void Spawn ( void );
	void EXPORT FadeThink ( void );
	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	virtual int	ObjectCaps( void ) { return CBaseEntity :: ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }

	static	TYPEDESCRIPTION m_SaveData[];

	float m_flStartTime;
	float m_flDuration;
	float m_flCoarseness;
	int m_iStartAmt;
	int m_iOffsetAmt;
	Vector m_vecStartColor;
	Vector m_vecOffsetColor;
	float m_fStartScale;
	float m_fOffsetScale;
	EHANDLE m_hTarget;

	int m_iszAmtFactor;
};

///////////////////////////////////////////////////////////////////////////////

// RenderFxManager itself
class CRenderFxManager : public CPointEntity
{
public:
	void Use ( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void Affect( CBaseEntity *pEntity, BOOL bIsLocus, CBaseEntity *pActivator );

	void KeyValue( KeyValueData *pkvd );
};

///////////////////////////////////////////////////////////////////////////////

class CEnvCustomize : public CBaseEntity
{
public:
	void Spawn( void );
	void EXPORT Think( void );
	void Use ( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );

	void Affect (CBaseEntity *pTarget, USE_TYPE useType);
	int GetActionFor( int iField, int iActive, USE_TYPE useType, char *szDebug );
	void SetBoneController (float fController, int cnum, CBaseEntity *pTarget);

	virtual int	ObjectCaps( void ) { return CBaseEntity :: ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }

	void KeyValue( KeyValueData *pkvd );
	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );

	static	TYPEDESCRIPTION m_SaveData[];

	float	m_flRadius;
	int		m_iszModel;
	int		m_iClass;
	int		m_iPlayerReact;
	int		m_iPrisoner;
	int		m_iMonsterClip;
	int		m_iVisible;
	int		m_iSolid;
	int		m_iProvoked;
	int		m_voicePitch;
	int		m_iBloodColor;
	float	m_fFramerate;
	float	m_fController0;
	float	m_fController1;
	float	m_fController2;
	float	m_fController3;
};

///////////////////////////////////////////////////////////////////////////////

class CBaseTrigger : public CBaseToggle
{
public:
	//LRC - this was very bloated. I moved lots of methods into the
	// subclasses where they belonged.
	void InitTrigger( void );
	void EXPORT ToggleUse ( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	BOOL CanTouch( entvars_t *pevToucher );

	virtual int	ObjectCaps( void ) { return CBaseToggle :: ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }
};

///////////////////////////////////////////////////////////////

class CTriggerMonsterJump : public CBaseTrigger
{
public:
	void Spawn( void );
	void Touch( CBaseEntity *pOther );
	void Think( void );
};

///////////////////////////////////////////////////////////////

class CTriggerCDAudio : public CBaseTrigger
{
public:
	void Spawn( void );

	virtual void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void PlayTrack( void );
	void Touch ( CBaseEntity *pOther );
};

///////////////////////////////////////////////////////////////

class CTriggerPush : public CBaseTrigger
{
public:
	void Spawn( void );
	void KeyValue( KeyValueData *pkvd );
	void Touch( CBaseEntity *pOther );
};

///////////////////////////////////////////////////////////////

class CTriggerMultiple : public CBaseTrigger
{
public:
	void Spawn( void );
	void Precache( void )
	{
		if (!FStringNull(pev->noise))
			PRECACHE_SOUND((char*)STRING(pev->noise));
	}
	void EXPORT MultiTouch( CBaseEntity *pOther );
	void EXPORT MultiWaitOver( void );
	void ActivateMultiTrigger( CBaseEntity *pActivator );
};

///////////////////////////////////////////////////////////////

class CTriggerInOut;

class CInOutRegister : public CPointEntity
{
public:
	// returns true if found in the list
	BOOL IsRegistered ( CBaseEntity *pValue );
	// remove all invalid entries from the list, trigger their targets as appropriate
	// returns the new list
	CInOutRegister *Prune( void );
	BOOL IsEmpty( void ) { return m_pNext?FALSE:TRUE; };

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	CTriggerInOut *m_pField;
	CInOutRegister *m_pNext;
	EHANDLE m_hValue;
};

class CTriggerInOut : public CBaseTrigger
{
public:
	void Spawn( void );
	void EXPORT Touch( CBaseEntity *pOther );
	void EXPORT Think( void );
	void FireOnLeaving( CBaseEntity *pOther );

	void KeyValue( KeyValueData *pkvd );
	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	STATE GetState() { return m_pRegister->IsEmpty()?STATE_OFF:STATE_ON; }

	string_t m_iszAltTarget;
	string_t m_iszBothTarget;
	CInOutRegister *m_pRegister;

	// Cthulhu
	//BOOL mbRestored;
};


///////////////////////////////////////////////////////////////

class CTriggerHurt : public CBaseTrigger
{
public:
	void Spawn( void );
	void EXPORT HurtThink( void );
	void EXPORT RadiationThink( void );
	void EXPORT HurtTouch ( CBaseEntity *pOther );
	void EXPORT ToggleUse ( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	virtual void KeyValue( KeyValueData *pkvd );
};

///////////////////////////////////////////////////////////////

class CTriggerOnce : public CTriggerMultiple
{
public:
	void Spawn( void );
};

///////////////////////////////////////////////////////////////

class CTriggerCounter : public CTriggerMultiple
{
public:
	void Spawn( void );
	void EXPORT CounterUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void KeyValue( KeyValueData *pkvd );
};

///////////////////////////////////////////////////////////////

class CTriggerVolume : public CPointEntity	// Derive from point entity so this doesn't move across levels
{
public:
	void		Spawn( void );
};

///////////////////////////////////////////////////////////////

class CFireAndDie : public CBaseDelay
{
public:
	void Spawn( void );
	void Precache( void );
	void Think( void );
	int ObjectCaps( void ) { return CBaseDelay::ObjectCaps() | FCAP_FORCE_TRANSITION; }	// Always go across transitions
};

///////////////////////////////////////////////////////////////

class CChangeLevel : public CBaseTrigger
{
public:
	void Spawn( void );
	void KeyValue( KeyValueData *pkvd );
	void EXPORT UseChangeLevel ( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void EXPORT TriggerChangeLevel( void );
	void EXPORT ExecuteChangeLevel( void );
	void EXPORT TouchChangeLevel( CBaseEntity *pOther );
	void ChangeLevelNow( CBaseEntity *pActivator );

	static edict_t *FindLandmark( const char *pLandmarkName );
	static int ChangeList( LEVELLIST *pLevelList, int maxList );
	static int AddTransitionToList( LEVELLIST *pLevelList, int listCount, const char *pMapName, const char *pLandmarkName, edict_t *pentLandmark );
	static int InTransitionVolume( CBaseEntity *pEntity, char *pVolumeName );

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );

	static	TYPEDESCRIPTION m_SaveData[];

	char m_szMapName[cchMapNameMost];		// trigger_changelevel only:  next map
	char m_szLandmarkName[cchMapNameMost];		// trigger_changelevel only:  landmark on next map
	int		m_changeTarget;
	float	m_changeTargetDelay;
};

///////////////////////////////////////////////////////////////

class CLadder : public CBaseTrigger
{
public:
	void KeyValue( KeyValueData *pkvd );
	void Spawn( void );
	void Precache( void );
};

///////////////////////////////////////////////////////////////

class CTriggerTeleport : public CBaseTrigger
{
public:
	void Spawn( void );
	void EXPORT TeleportTouch ( CBaseEntity *pOther );
};

///////////////////////////////////////////////////////////////

class CTriggerSave : public CBaseTrigger
{
public:
	void Spawn( void );
	void EXPORT SaveTouch( CBaseEntity *pOther );
};

///////////////////////////////////////////////////////////////

class CTriggerEndSection : public CBaseTrigger
{
public:
	void Spawn( void );
	void EXPORT EndSectionTouch( CBaseEntity *pOther );
	void KeyValue( KeyValueData *pkvd );
	void EXPORT EndSectionUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
};

///////////////////////////////////////////////////////////////

class CTriggerGravity : public CBaseTrigger
{
public:
	void Spawn( void );
	void EXPORT GravityTouch( CBaseEntity *pOther );
};

///////////////////////////////////////////////////////////////

class CTriggerSetPatrol : public CBaseDelay
{
public:
	void KeyValue( KeyValueData *pkvd );
	void Spawn( void );
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );

	int ObjectCaps( void ) { return CBaseDelay::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }
	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );

	static	TYPEDESCRIPTION m_SaveData[];

private:
	int		m_iszPath;
};

///////////////////////////////////////////////////////////////

class CTriggerMotion : public CPointEntity
{
public:
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	void	KeyValue( KeyValueData *pkvd );

	int m_iszPosition;
	int m_iPosMode;
	int m_iszAngles;
	int m_iAngMode;
	int m_iszVelocity;
	int m_iVelMode;
	int m_iszAVelocity;
	int m_iAVelMode;
};

class CMotionThread : public CPointEntity
{
public:
	void Think( void );

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	int m_iszPosition;
	int m_iPosMode;
	int m_iszFacing;
	int m_iFaceMode;
	EHANDLE m_hLocus;
	EHANDLE m_hTarget;
};

class CMotionManager : public CPointEntity
{
public:
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void KeyValue( KeyValueData *pkvd );
	void Affect( CBaseEntity *pTarget, CBaseEntity *pActivator );

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	int m_iszPosition;
	int m_iPosMode;
	int m_iszFacing;
	int m_iFaceMode;
};

///////////////////////////////////////////////////////////////////

class CTriggerChangeTarget : public CBaseDelay
{
public:
	void KeyValue( KeyValueData *pkvd );
	void Spawn( void );
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );

	int ObjectCaps( void ) { return CBaseDelay::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }
	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );

	static	TYPEDESCRIPTION m_SaveData[];

private:
	int		m_iszNewTarget;
};

///////////////////////////////////////////////////////////////

class CTriggerCommand : public CBaseEntity
{
public:
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	virtual int	ObjectCaps( void ) { return CBaseEntity :: ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }
};

///////////////////////////////////////////////////////////////

class CTriggerChangeCVar : public CBaseEntity
{
public:
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void EXPORT Think( void );
	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	virtual int	ObjectCaps( void ) { return CBaseEntity :: ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }

	static	TYPEDESCRIPTION m_SaveData[];

	char m_szStoredString[256];
};

///////////////////////////////////////////////////////////////

class CTriggerCamera : public CBaseDelay
{
public:
	void Spawn( void );
	void KeyValue( KeyValueData *pkvd );
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void EXPORT FollowTarget( void );
	void Move(void);

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	virtual int	ObjectCaps( void ) { return CBaseEntity :: ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }
	static	TYPEDESCRIPTION m_SaveData[];

	EHANDLE m_hPlayer;
	EHANDLE m_hTarget;
	CBaseEntity *m_pentPath;
	int	  m_sPath;
	float m_flWait;
	float m_flReturnTime;
	float m_flStopTime;
	float m_moveDistance;
	float m_targetSpeed;
	float m_initialSpeed;
	float m_acceleration;
	float m_deceleration;
	int	  m_state;
	
};


///////////////////////////////////////////////////////////////


#endif

