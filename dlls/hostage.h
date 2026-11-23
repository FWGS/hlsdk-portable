#ifndef HOSTAGE_H
#define HOSTAGE_H

class CBasePlayer;
class CLocalNav;

enum state
{
	FOLLOW = 0,
	STAND,
	DUCK,
	SCARED,
	IDLE,
	FOLLOWPATH,
};

class CHostage : public CBaseMonster
{
public:
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
	CHostage() : m_rescueTarget( 0 ) {}
	void Spawn() override;
	void Precache() override;
	void EXPORT IdleThink();
	void EXPORT Remove();
	void RePosition();
	void SetActivity( int activity );
	int ObjectCaps() override;
	void Restart() override;
	void KeyValue( KeyValueData* data ) override;
	Schedule_t* GetSchedule() override;
	void EXPORT FollowUse( CBaseEntity* activator, CBaseEntity* caller, USE_TYPE useType, float value );
	int TakeDamage( entvars_t* inflictor, entvars_t* attacker, float damage, int damageType ) override;
	void Touch( CBaseEntity* other ) override;
	int Classify() override;
	int GetActivity() { return m_Activity; }
	int BloodColor() override;
	float GetModifiedDamage( float damage, int hitgroup );
	void SetFlinchActivity();
	void SetDeathActivity();
	void PlayPainSound();
	void DoFollow();
	BOOL IsOnLadder();
	void PointAt( Vector& point );
	void MoveToward( Vector& point );
	void NavReady();
	void Wiggle();
	void PreThink();
	void SetYawSpeed() override;
	int Save( CSave& save ) override;
	int Restore( CRestore& restore ) override;
	static TYPEDESCRIPTION m_SaveData[];

	int			m_Activity;
	BOOL		m_bTouched;
	BOOL		m_bRescueMe;
	BOOL		m_rescued;
	float		m_flFlinchTime;
	float		m_flNextChange;
	float		m_flMarkPosition;
	int			m_iModel;
	int			m_iSkin;
	float		m_flNextRadarTime;
	state		m_State;
	Vector		m_vStart;
	Vector		m_vStartAngles;
	Vector		m_vPathToFollow[20];
	CBasePlayer*	m_target;
	float		m_flNextPainTime;
	CLocalNav*	m_LocalNav;
	int			nTargetNode;
	Vector		vecNodes[100];
	EHANDLE		m_hStoppedTargetEnt;
	float		m_flNextFullThink;
	float		m_flPathCheckInterval;
	float		m_flLastPathCheck;
	int			m_nPathNodes;
	BOOL		m_fHasPath;
	float		m_flPathAcquired;
	string_t	m_rescueTarget;
	string_t	m_rescueAnim;
	BOOL		m_noAutoRemove;
	float		m_voicePitch;
	Vector		m_vOldPos;
	int			m_iHostageIndex;
};

#endif
