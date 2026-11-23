#pragma once

#include "cbase.h"

class CBaseTrigger : public CBaseToggle
{
public:
	CBaseTrigger();
	void KeyValue( KeyValueData* data ) override;
	int Save( CSave& save ) override;
	int Restore( CRestore& restore ) override;
	int ObjectCaps() override;
	void EXPORT TeleportTouch( CBaseEntity* other );
	void EXPORT MultiTouch( CBaseEntity* other );
	void EXPORT HurtTouch( CBaseEntity* other );
	void ActivateMultiTrigger( CBaseEntity* activator );
	void EXPORT MultiWaitOver();
	void EXPORT CounterUse( CBaseEntity* activator, CBaseEntity* caller, USE_TYPE useType, float value );
	void EXPORT ToggleUse( CBaseEntity* activator, CBaseEntity* caller, USE_TYPE useType, float value );
	void InitTrigger();
	BOOL CheckConditions( CBaseEntity* activator );
	BOOL IsViewObjectVisible( CBaseEntity* activator );
	CBaseEntity* GetQueryTarget( CBaseEntity* activator );
	static TYPEDESCRIPTION m_SaveData[];

private:
	int		m_cTargets;
	int		m_iKey[32];
	int		m_iValue[32];
	int		m_iQueryTarget;
	int		m_iViewTargetName;
	float	m_fViewConeDegrees;
};

class CTriggerCamera : public CBaseDelay
{
public:
	void Spawn() override;
	void KeyValue( KeyValueData* data ) override;
	int Save( CSave& save ) override;
	int Restore( CRestore& restore ) override;
	int ObjectCaps() override;
	void Use( CBaseEntity* activator, CBaseEntity* caller, USE_TYPE useType, float value ) override;
	void EXPORT FollowTarget();
	void Move();
	BOOL MinimumTimeMet();
	static TYPEDESCRIPTION m_SaveData[];

	EHANDLE		m_hPlayer;
	EHANDLE		m_hTarget;
	CBaseEntity*	m_pentPath;
	int			m_sPath;
	float		m_flWait;
	float		m_flReturnTime;
	float		m_flStopTime;
	float		m_moveDistance;
	float		m_targetSpeed;
	float		m_initialSpeed;
	float		m_acceleration;
	float		m_deceleration;
	int			m_state;
	int			m_useOverlay;
	BOOL		m_immediate;
	float		m_flMinimumTime;
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

class CTriggerStealth : public CBaseTrigger
{
public:
	CTriggerStealth();
	~CTriggerStealth();
	void Spawn() override;
	void KeyValue(KeyValueData *data) override;
	int Save(CSave &save) override;
	int Restore(CRestore &restore) override;
	void EXPORT TouchTriggerStealth(CBaseEntity *other);
	BOOL IsPlayerHidden();
	static TYPEDESCRIPTION m_SaveData[];
	float m_flCover;
	float m_flCoverTime;
	BOOL m_bPlayerHidden;
	BOOL m_showIcon;
};

class CTriggerUseTool : public CBaseTrigger
{
public:
	CTriggerUseTool();
	void Spawn() override;
	void KeyValue( KeyValueData* data ) override;
	int Save( CSave& save ) override;
	int Restore( CRestore& restore ) override;
	void PreKillTarget() override;
	void EXPORT TouchTriggerUseTool( CBaseEntity* other );
	void EXPORT FireUseTarget( CBaseEntity* activator, CBaseEntity* caller, USE_TYPE useType, float value );
	BOOL IsPlayerInPosition();
	BOOL IsPlayerInTrigger();
	BOOL IsPlayerUsingProperTool();
	BOOL IsPlayerInPositionUsingProperTool();
	BOOL IsBombDefuseTrigger();
	CBaseEntity* GetRCBombTarget();
	void DisplayToolMessage();
	static TYPEDESCRIPTION m_SaveData[];

	int		m_iToolName;
	int		m_iToolTarget;
	int		m_iToolSet;
	int		m_iDefuseTime;
	int		m_iRCBombTarget;
	BOOL	m_bombPlanted;
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

class CTriggerObjective : public CBaseTrigger
{
public:
	CTriggerObjective();
	void Spawn() override;
	void KeyValue( KeyValueData* data ) override;
	int Save( CSave& save ) override;
	int Restore( CRestore& restore ) override;
	int ObjectCaps() override;
	void EXPORT ObjectiveUse( CBaseEntity* activator, CBaseEntity* caller, USE_TYPE useType, float value );
	BOOL DisplayObjective( int lineCount );
	static TYPEDESCRIPTION m_SaveData[];

	int		objectiveState;
	int		objectiveText;
	BOOL	transition;
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

class CClientFog : public CBaseEntity
{
public:
	CClientFog();
	void Spawn() override;
	void KeyValue( KeyValueData* data ) override;
	int Save( CSave& save ) override;
	int Restore( CRestore& restore ) override;
	static TYPEDESCRIPTION m_SaveData[];

	float	m_flDensity;
	float	m_fStartDistance;
	float	m_fStopDistance;
};
