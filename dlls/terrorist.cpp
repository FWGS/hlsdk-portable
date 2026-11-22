#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "squadmonster.h"
#include "weapons.h"
#include "soundent.h"
#include "talkmonster.h"
#include "effects.h"
#include "nodes.h"
#include "skill.h"
#include "player.h"
#include "triggers.h"
#include "animation.h"
#include "explode.h"
#include "customentity.h"

#define HGRUNT_AE_RELOAD			2
#define HGRUNT_AE_KICK			3
#define HGRUNT_AE_BURST1			4
#define HGRUNT_AE_BURST2			5
#define HGRUNT_AE_BURST3			6
#define HGRUNT_AE_GREN_TOSS		7
#define HGRUNT_AE_GREN_LAUNCH		8
#define HGRUNT_AE_GREN_DROP		9
#define HGRUNT_AE_CAUGHT_ENEMY	10
#define HGRUNT_AE_DROP_GUN		11

enum
{
	EASY_HEAD,
	MEDIUM_HEAD,
	HARD_HEAD
};

enum
{
	SCHED_TERRORIST_SUPPRESS = 49,
	SCHED_TERRORIST_ESTABLISH_LINE_OF_FIRE,
	SCHED_TERRORIST_COVER_AND_RELOAD,
	SCHED_TERRORIST_SWEEP,
	SCHED_TERRORIST_FOUND_ENEMY,
	SCHED_TERRORIST_REPEL,
	SCHED_TERRORIST_REPEL_ATTACK,
	SCHED_TERRORIST_REPEL_LAND,
	SCHED_TERRORIST_WAIT_FACE_ENEMY,
	SCHED_TERRORIST_TAKECOVER_FAILED,
	SCHED_TERRORIST_ELOF_FAIL,
	SCHED_TERRORIST_IN_SMOKE,
	SCHED_KAMAKAZI_BLOWUP,
	SCHED_TERRORIST_SNIPER_ATTACK,
	SCHED_TERRORIST_SNIPER_TAKESHOT,
	SCHED_TERRORIST_SPECIAL_COVER_NODE,
	SCHED_TERRORIST_DUCK_NODE_ATTACK,
	SCHED_TERRORIST_CLOSEIN,
	SCHED_TAKE_COVER_LIGHT_DAMAGE,
	SCHED_TAKE_COVER_HEAVY_DAMAGE,
	SCHED_TERRORIST_CORNER_NODE_ATTACK,
	SCHED_TERRORIST_FIRE_PAUSE,
	SCHED_TERRORIST_ALERT_SPEAK,
	SCHED_TERRORIST_INVESTIGATE,
	SCHED_TERRORIST_FLASHED,
	SCHED_TERRORIST_WAIT_CROUCHED,
	LAST_CUSTOM_SCHEDULE
};

enum
{
	ACT_TERRORIST_CORNER_FIRE1 = 81,
	ACT_TERRORIST_CORNER_FIRE2,
	ACT_TERRORIST_KAMAKAZI_EXPLODE,
	ACT_TERRORIST_CHOKING,
	ACT_TERRORIST_LOOKAROUND,
	ACT_TERRORIST_FLASHED
};

enum
{
	BEHAVIOR_PISTOL,
	BEHAVIOR_SHOTGUN,
	BEHAVIOR_ASSAULTRIFLE,
	BEHAVIOR_MACHINEGUN,
	BEHAVIOR_SNIPERRIFLE,
	BEHAVIOR_LAW,
	BEHAVIOR_SMG,
	BEHAVIOR_GRENADER,
	BEHAVIOR_MELEE,
	BEHAVIOR_SUICIDAL,
	BEHAVIOR_MP5
};

enum
{
	TASK_TERRORIST_FACE_TOSS_DIR = 92,
	TASK_TERRORIST_SPEAK_SENTENCE,
	TASK_TERRORIST_CHECK_FIRE,
	TASK_MOVE_TO_SPECIAL_COVER_NODE,
	TASK_MOVE_TO_PREFERED_RANGE,
	TASK_WANDER
};

enum
{
	TERRORIST_ARCTIC,
	TERRORIST_DESERT,
	TERRORIST_URBAN,
	TERRORIST_JUNGLE,
	TERRORIST_RUSSIAN,
	TERRORIST_ASIAN
};

class CTerrorist : public CSquadMonster
{
public:
	CTerrorist();
	void Spawn() override;
	void SpawnInit() override;
	int GetWeaponEffectSchedule() override;
	BOOL IsPlayerHidden() override;
	void KeyValue( KeyValueData* data ) override;
	int LookupActivity( int activity ) override;
	int LookupActivityHeaviest( int activity ) override;
	int ObjectCaps() override { return FCAP_ACROSS_TRANSITION | FCAP_IMPULSE_USE; }
	void EXPORT UseTarget( CBaseEntity* activator, CBaseEntity* caller, USE_TYPE useType, float value );
	void Precache() override;
	void SetYawSpeed() override;
	int Classify() override { return m_classtype; }
	int ISoundMask() override { return bits_SOUND_WORLD | bits_SOUND_COMBAT | bits_SOUND_PLAYER | bits_SOUND_DANGER; }
	void HandleAnimEvent( MonsterEvent_t* event ) override;
	BOOL FCanCheckAttacks() override { return !HasConditions( bits_COND_ENEMY_TOOFAR ); }
	BOOL CheckMeleeAttack1( float dot, float distance ) override;
	BOOL CheckRangeAttack1( float dot, float distance ) override;
	BOOL CheckRangeAttack2( float dot, float distance ) override;
	void CheckAmmo() override;
	void SetActivity( Activity activity ) override;
	void StartTask( Task_t* task ) override;
	void RunTask( Task_t* task ) override;
	void DeathSound() override;
	void PainSound() override;
	void IdleSound() override;
	Vector GetGunPosition() override;
	void ShootAssaultRifle();
	void ShootSMG();
	void ShootShotgun();
	void ShootSniperRifle();
	void ShootMachineGun();
	void ShootMP5();
	void ShootPistol();
	void ShootLAW();
	void PrescheduleThink() override;
	void GibMonster() override;
	void SpeakSentence();
	void SetupWeapons();
	void HandleSpeaking( int mode );
	int CanPlaySequence( BOOL disregardState, int interruptLevel ) override;
	void PlayScriptedSentence( const char* sentence, float duration, float volume, float attenuation,
		BOOL concurrent, CBaseEntity* listener ) override;
	void SetupLaserGlow();
	void LaserGlowOn();
	void LaserGlowOff();
	int Save( CSave& save ) override;
	int Restore( CRestore& restore ) override;
	CBaseEntity* Kick();
	Schedule_t* GetSchedule() override;
	Schedule_t* GetScheduleOfType( int type ) override;
	int GetBehaviorTypeSchedule();
	int GetFlyingSchedule();
	int GetHearingSchedule();
	int GetForcedTargetSchedule();
	void SetupBehaviors( int type );
	void TraceAttack( entvars_t* attacker, float damage, Vector direction, TraceResult* trace, int damageType ) override;
	int TakeDamage( entvars_t* inflictor, entvars_t* attacker, float damage, int damageType ) override;
	int IRelationship( CBaseEntity* target ) override;
	BOOL FOkToSpeak();
	void JustSpoke( float duration );
	Schedule_t* ScheduleFromName( const char* name ) override;
	void InitScheduleTable();
	void ClearScheduleTableFlags();
	int GetScheduleFromTable();
	int GetBehaviorType();
	void ActivateTableEntry( int type );
	bool ValidateActivation( int type );
	int SetActivityPistolBase( Activity activity );
	int SetActivityMeleeBase( Activity activity );
	int SetActivitySniperBase( Activity activity );
	int SetActivityKamakaziBase( Activity activity );
	int SetActivityShotgunBase( Activity activity );
	int SetActivityGrenadeBase( Activity activity );
	int SetActivityAutomaticBase( Activity activity );
	int SetActivityLAWBase( Activity activity );
	void SetHead( int head );

	static Schedule_t* m_scheduleList[];
	static TYPEDESCRIPTION m_SaveData[];
	ScheduleEntry scheduleTable[75];
	float m_flNextGrenadeCheck;
	float m_flNextPainTime;
	float m_flLastEnemySightTime;
	Vector m_vecTossVelocity;
	BOOL m_fThrowGrenade;
	BOOL m_bStanding;
	BOOL m_fFirstEncounter;
	int m_cClipSize;
	int m_voicePitch;
	int m_iBrassShell;
	int m_iShotgunShell;
	int m_iSentence;
	BOOL suicideFlag;
	BOOL justShotFlag;
	float m_blastRadius;
	float m_blastDamage;
	int m_terroristType;
	int m_terroristBehavior;
	float m_fPreferedRange;
	CSprite* m_pLaserGlow;
	float m_glowBrightness;
	CBeam* m_pBeam;
	float m_laserBrightness;
	BOOL m_flashbangOnly;
	BOOL m_heGrenadeOnly;
	float taskFailCount;
	float lawDelayTime;
	float m_alertRadius;
	float m_wanderTime;
	Vector m_wanderOrigin;
	BOOL m_canWander;
	unsigned short m_usFireGlock;
	unsigned short m_usFireM60;
	unsigned short m_usFireAK47;
	unsigned short m_usFireAWP;
	unsigned short m_usFireTMP;
	unsigned short m_usFireShotgun;
	unsigned short m_usFireMP5;
	BOOL stealthDeath;
	BOOL heardCombat;
	static const char* pTerroristSentences[];
};

int g_fTerroristQuestion;

extern Schedule_t slTerroristFail[];
extern Schedule_t slTerroristCombatFail[];
extern Schedule_t slTerroristVictoryDance[];
extern Schedule_t slTerroristEstablishLineOfFire[];
extern Schedule_t slTerroristFoundEnemy[];
extern Schedule_t slTerroristCombatFace[];
extern Schedule_t slTerroristSignalSuppress[];
extern Schedule_t slTerroristSuppress[];
extern Schedule_t slTerroristWaitInCover[];
extern Schedule_t slTerroristTakeCover[];
extern Schedule_t slTerroristGrenadeCover[];
extern Schedule_t slTerroristTossGrenadeCover[];
extern Schedule_t slTerroristTakeCoverFromBestSound[];
extern Schedule_t slTerroristHideReload[];
extern Schedule_t slTerroristSweep[];
extern Schedule_t slTerroristRangeAttack1A[];
extern Schedule_t slTerroristRangeAttack1B[];
extern Schedule_t slTerroristRangeAttack1C[];
extern Schedule_t slTerroristRangeAttack2[];
extern Schedule_t slTerroristRepel[];
extern Schedule_t slTerroristRepelAttack[];
extern Schedule_t slTerroristRepelLand[];
extern Schedule_t slTerroristInSmoke[];
extern Schedule_t slTerroristFlashed[];
extern Schedule_t slTerroristKamakaziBlowup[];
extern Schedule_t slTerroristSniperAttack[];
extern Schedule_t slTerroristSniperTakeShot[];
extern Schedule_t slTerroristDuckCoverNode[];
extern Schedule_t slTerroristDuckNodeAttack[];
extern Schedule_t slTerroristCloseIn[];
extern Schedule_t slTerroristRangeAttackMG1[];
extern Schedule_t slTerroristRangeAttackMG2[];
extern Schedule_t slTerroristCornerNodeAttack[];
extern Schedule_t slTerroristCornerCoverNode[];
extern Schedule_t slTerroristFiringPause[];
extern Schedule_t slTerroristAlertSpeak[];
extern Schedule_t slTerroristInvestigate[];
extern Schedule_t slTerroristWaitCrouched[];
extern Schedule_t slTerroristWander[];

TYPEDESCRIPTION CTerrorist::m_SaveData[] =
{
	DEFINE_FIELD( CTerrorist, m_flNextGrenadeCheck, FIELD_TIME ),
	DEFINE_FIELD( CTerrorist, m_flNextPainTime, FIELD_TIME ),
	DEFINE_FIELD( CTerrorist, m_flLastEnemySightTime, FIELD_TIME ),
	DEFINE_FIELD( CTerrorist, m_vecTossVelocity, FIELD_VECTOR ),
	DEFINE_FIELD( CTerrorist, m_fThrowGrenade, FIELD_BOOLEAN ),
	DEFINE_FIELD( CTerrorist, m_bStanding, FIELD_BOOLEAN ),
	DEFINE_FIELD( CTerrorist, m_fFirstEncounter, FIELD_BOOLEAN ),
	DEFINE_FIELD( CTerrorist, m_cClipSize, FIELD_INTEGER ),
	DEFINE_FIELD( CTerrorist, m_voicePitch, FIELD_INTEGER ),
	DEFINE_FIELD( CTerrorist, m_iBrassShell, FIELD_INTEGER ),
	DEFINE_FIELD( CTerrorist, m_iShotgunShell, FIELD_INTEGER ),
	DEFINE_FIELD( CTerrorist, m_iSentence, FIELD_INTEGER ),
	DEFINE_FIELD( CTerrorist, suicideFlag, FIELD_BOOLEAN ),
	DEFINE_FIELD( CTerrorist, justShotFlag, FIELD_BOOLEAN ),
	DEFINE_FIELD( CTerrorist, m_blastRadius, FIELD_FLOAT ),
	DEFINE_FIELD( CTerrorist, m_blastDamage, FIELD_FLOAT ),
	DEFINE_FIELD( CTerrorist, m_terroristType, FIELD_INTEGER ),
	DEFINE_FIELD( CTerrorist, m_terroristBehavior, FIELD_INTEGER ),
	DEFINE_FIELD( CTerrorist, m_fPreferedRange, FIELD_FLOAT ),
	DEFINE_ARRAY( CTerrorist, scheduleTable, FIELD_VECTOR, 75 ),
	DEFINE_FIELD( CTerrorist, m_pLaserGlow, FIELD_CLASSPTR ),
	DEFINE_FIELD( CTerrorist, m_glowBrightness, FIELD_FLOAT ),
	DEFINE_FIELD( CTerrorist, m_pBeam, FIELD_CLASSPTR ),
	DEFINE_FIELD( CTerrorist, m_laserBrightness, FIELD_FLOAT ),
	DEFINE_FIELD( CTerrorist, m_flashbangOnly, FIELD_BOOLEAN ),
	DEFINE_FIELD( CTerrorist, m_heGrenadeOnly, FIELD_BOOLEAN ),
	DEFINE_FIELD( CTerrorist, taskFailCount, FIELD_INTEGER ),
	DEFINE_FIELD( CTerrorist, stealthDeath, FIELD_BOOLEAN ),
	DEFINE_FIELD( CTerrorist, m_alertRadius, FIELD_FLOAT ),
	DEFINE_FIELD( CTerrorist, m_wanderTime, FIELD_TIME ),
	DEFINE_FIELD( CTerrorist, m_wanderOrigin, FIELD_VECTOR ),
	DEFINE_FIELD( CTerrorist, m_canWander, FIELD_BOOLEAN ),
	DEFINE_FIELD( CTerrorist, heardCombat, FIELD_BOOLEAN )
};

IMPLEMENT_SAVERESTORE( CTerrorist, CSquadMonster );

const char* CTerrorist::pTerroristSentences[] =
{
	"T_GREN",
	"T_ENEMY",
	"T_THROW",
	"T_TAUNT",
	"T_IDLE",
	"T_QUEST",
	"T_ANSWER",
	"T_CLEAR",
	"T_CHECK",
	"T_ALERT",
	"T_RELAX",
	"T_HUNT"
};

#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
CTerrorist::CTerrorist()
{
	m_bStanding = TRUE;
	m_terroristType = TERRORIST_ARCTIC;
	m_terroristBehavior = -1;
	m_flashbangOnly = FALSE;
	m_heGrenadeOnly = FALSE;
	m_blastRadius = 256;
	m_canWander = FALSE;
	stealthDeath = TRUE;
	taskFailCount = 0;
	suicideFlag = FALSE;
	lawDelayTime = 0;
	justShotFlag = FALSE;
	m_alertRadius = 256;
	m_blastDamage = 200;
	m_fPreferedRange = 64;
	heardCombat = FALSE;
	m_classtype = 14;
	float time = gpGlobals->time;
	m_wanderTime = time + RANDOM_FLOAT( 5, 10 );
}

void CTerrorist::CheckAmmo()
{
	if ( m_cAmmoLoaded <= 0 )
		SetConditions( bits_COND_NO_AMMO_LOADED );
}

Vector CTerrorist::GetGunPosition()
{
	return pev->origin + Vector( 0, 0, m_bStanding ? 60.0f : 36.0f );
}

void CTerrorist::SpawnInit()
{
	SET_MODEL( edict(), pev->model ? STRING( pev->model ) : "models/hgrunt.mdl" );
}

void CTerrorist::SetYawSpeed()
{
	int speed;
	switch ( m_Activity )
	{
	case ACT_IDLE:
	case ACT_RUN: speed = 150; break;
	case ACT_WALK:
	case ACT_TURN_LEFT:
	case ACT_TURN_RIGHT: speed = 180; break;
	case ACT_RANGE_ATTACK1:
	case ACT_RANGE_ATTACK2:
	case ACT_MELEE_ATTACK1:
	case ACT_MELEE_ATTACK2: speed = 120; break;
	case ACT_GLIDE:
	case ACT_FLY: speed = 30; break;
	default: speed = 90; break;
	}
	pev->yaw_speed = (float)speed;
}

void CTerrorist::Precache()
{
	PRECACHE_SOUND( "weapons/reload_shotgun.wav" );
	PRECACHE_SOUND( "weapons/pistol_reload.wav" );
	PRECACHE_SOUND( "weapons/reload_mp5.wav" );
	PRECACHE_SOUND( "weapons/glauncher.wav" );
	PRECACHE_SOUND( "weapons/sbarrel1.wav" );
	PRECACHE_SOUND( "weapons/machete_slash1.wav" );
	PRECACHE_SOUND( "weapons/machete_slash2.wav" );
	PRECACHE_SOUND( "weapons/knife_slash1.wav" );
	PRECACHE_SOUND( "weapons/melee_whoosh1.wav" );
	PRECACHE_SOUND( "weapons/melee_whoosh2.wav" );
	m_voicePitch = 100;
	m_iBrassShell = PRECACHE_MODEL( "models/shell.mdl" );
	m_iShotgunShell = PRECACHE_MODEL( "models/shotgunshell.mdl" );
	PRECACHE_MODEL( "sprites/laserbeam.spr" );
	PRECACHE_MODEL( "sprites/flare3.spr" );
	m_usFireM60 = PRECACHE_EVENT( 1, "events/m60.sc" );
	m_usFireTMP = PRECACHE_EVENT( 1, "events/tmp.sc" );
	m_usFireGlock = PRECACHE_EVENT( 1, "events/glock1.sc" );
	m_usFireAK47 = PRECACHE_EVENT( 1, "events/ak47.sc" );
	m_usFireAWP = PRECACHE_EVENT( 1, "events/awp.sc" );
	m_usFireMP5 = PRECACHE_EVENT( 1, "events/mp5n.sc" );
	m_usFireShotgun = PRECACHE_EVENT( 1, "events/m3.sc" );
}

void CTerrorist::Spawn()
{
	Precache();
	SpawnInit();
	m_terroristBehavior = GetBehaviorType();
	UTIL_SetSize( pev, Vector( -16, -16, 0 ), Vector( 16, 16, 72 ) );
	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	m_bloodColor = BLOOD_COLOR_RED;
	pev->effects = 0;
	pev->health = gSkillData.hgruntHealth * m_healthMultiplier;
	m_flFieldOfView = -0.7f;
	m_MonsterState = MONSTERSTATE_NONE;
	m_flNextGrenadeCheck = gpGlobals->time + 1;
	m_iSentence = -1;
	m_afCapability = bits_CAP_SQUAD | bits_CAP_TURN_HEAD | bits_CAP_DOORS_GROUP;
	m_flNextPainTime = gpGlobals->time;
	m_fEnemyEluded = FALSE;
	m_fFirstEncounter = TRUE;
	m_HackedGunPos = Vector( 0, 0, 55 );
	SetupWeapons();
	InitScheduleTable();
	SetupBehaviors( m_terroristBehavior );
	CTalkMonster::g_talkWaitTime = 0;
	MonsterInit();
	if ( (unsigned int)(m_terroristBehavior - BEHAVIOR_SNIPERRIFLE) <= 1 )
	{
		if ( !m_customRange )
			m_flDistTooFar = 2048;
		m_flDistLook = 4096;
		m_flFieldOfView = -0.7f;
		SetupLaserGlow();
	}
	if ( m_invulnerable )
		pev->takedamage = DAMAGE_NO;
	if ( m_head > 0 )
		SetBodygroup( 1, m_head );
	SetUse( &CTerrorist::UseTarget );
}

void CTerrorist::UseTarget( CBaseEntity* activator, CBaseEntity*, USE_TYPE useType, float value )
{
	if ( m_useTarget )
		FireTargets( STRING( m_useTarget ), activator, this, useType, value );
}

void CTerrorist::KeyValue( KeyValueData* data )
{
	if ( FStrEq( data->szKeyName, "behavior" ) )
	{
		m_behaviorType = atoi( data->szValue );
		data->fHandled = TRUE;
	}
	else if ( FStrEq( data->szKeyName, "blastradius" ) )
	{
		m_blastRadius = (float)atof( data->szValue );
		data->fHandled = TRUE;
	}
	else if ( FStrEq( data->szKeyName, "blastdamage" ) )
	{
		m_blastDamage = (float)atof( data->szValue );
		data->fHandled = TRUE;
	}
	else if ( FStrEq( data->szKeyName, "flashbangonly" ) )
	{
		m_flashbangOnly = atoi( data->szValue ) != 0;
		data->fHandled = TRUE;
	}
	else if ( FStrEq( data->szKeyName, "hegrenadeonly" ) )
	{
		m_heGrenadeOnly = atoi( data->szValue ) != 0;
		data->fHandled = TRUE;
	}
	else if ( FStrEq( data->szKeyName, "alertradius" ) )
	{
		m_alertRadius = (float)atof( data->szValue );
		data->fHandled = TRUE;
	}
	if ( FStrEq( data->szKeyName, "wander" ) )
	{
		m_canWander = atoi( data->szValue ) != 0;
		data->fHandled = TRUE;
	}
	else
		CBaseMonster::KeyValue( data );
}

void CTerrorist::InitScheduleTable()
{
	for ( int i = 0; i < ARRAYSIZE( scheduleTable ); ++i )
	{
		scheduleTable[i].activeFlag = 0;
		scheduleTable[i].percentChance = 1;
	}
}

void CTerrorist::ClearScheduleTableFlags()
{
	for ( int i = 0; i < ARRAYSIZE( scheduleTable ); ++i )
		scheduleTable[i].activeFlag = 0;
}

bool CTerrorist::ValidateActivation( int entry )
{
	if ( entry == SCHED_TAKE_COVER_FROM_ENEMY || entry == SCHED_TERRORIST_ESTABLISH_LINE_OF_FIRE ||
		entry == SCHED_TERRORIST_SPECIAL_COVER_NODE || entry == SCHED_TERRORIST_CLOSEIN )
		return !m_cantMove;
	return true;
}

void CTerrorist::ActivateTableEntry( int entry )
{
	if ( ValidateActivation( entry ) && (unsigned int)entry < ARRAYSIZE( scheduleTable ) )
		scheduleTable[entry].activeFlag = 1;
}

int CTerrorist::GetScheduleFromTable()
{
	float total = 0;
	for ( int i = 0; i < ARRAYSIZE( scheduleTable ); ++i )
	{
		if ( scheduleTable[i].activeFlag != 0 && scheduleTable[i].percentChance > 0 )
			total += scheduleTable[i].percentChance;
	}
	for ( int i = 0; i < ARRAYSIZE( scheduleTable ); ++i )
	{
		if ( scheduleTable[i].activeFlag != 0 && scheduleTable[i].percentChance > 0 )
			scheduleTable[i].normalizedChance = scheduleTable[i].percentChance / total;
	}
	float choice = RANDOM_FLOAT( 0, 1 );
	float accumulated = 0;
	for ( int i = 0; i < ARRAYSIZE( scheduleTable ); ++i )
	{
		if ( scheduleTable[i].activeFlag != 0 && scheduleTable[i].percentChance > 0 )
		{
			accumulated += scheduleTable[i].normalizedChance;
			if ( accumulated > choice )
				return i;
		}
	}
	if ( m_MonsterState != MONSTERSTATE_IDLE )
	{
		if ( pev->targetname )
			ALERT( at_aiconsole, "%s -- %s at (%d, %d, %d), Failed to find valid schedule in the table!\n",
				STRING( pev->classname ), STRING( pev->targetname ),
				(int)pev->origin.x, (int)pev->origin.y, (int)pev->origin.z );
		else
			ALERT( at_aiconsole, "%s -- (no targetname) at (%d, %d, %d), Failed to find valid schedule in the table!\n",
				STRING( pev->classname ), (int)pev->origin.x, (int)pev->origin.y, (int)pev->origin.z );
	}
	return -1;
}

int CTerrorist::GetBehaviorType()
{
	const char* name = STRING( pev->classname );
	if ( strstr( name, "pistol" ) ) return BEHAVIOR_PISTOL;
	if ( strstr( name, "shotgun" ) ) return BEHAVIOR_SHOTGUN;
	if ( strstr( name, "assaultrifle" ) ) return BEHAVIOR_ASSAULTRIFLE;
	if ( strstr( name, "machinegun" ) ) return BEHAVIOR_MACHINEGUN;
	if ( strstr( name, "sniperrifle" ) ) return BEHAVIOR_SNIPERRIFLE;
	if ( strstr( name, "law" ) ) return BEHAVIOR_LAW;
	if ( strstr( name, "smg" ) ) return BEHAVIOR_SMG;
	if ( strstr( name, "melee" ) ) return BEHAVIOR_MELEE;
	if ( strstr( name, "grenader" ) ) return BEHAVIOR_GRENADER;
	if ( strstr( name, "kamakazi" ) ) return BEHAVIOR_SUICIDAL;
	if ( strstr( name, "mp5" ) ) return BEHAVIOR_MP5;
	if ( m_behaviorType == 2 ) return BEHAVIOR_SNIPERRIFLE;
	return m_behaviorType == 1 ? BEHAVIOR_SUICIDAL : BEHAVIOR_MP5;
}

void CTerrorist::TraceAttack( entvars_t* attacker, float damage, Vector direction, TraceResult* trace, int damageType )
{
	CBaseMonster::TraceAttack( attacker, damage, direction, trace, damageType );
}

int CTerrorist::TakeDamage( entvars_t* inflictor, entvars_t* attacker, float damage, int damageType )
{
	Forget( bits_MEMORY_INCOVER );
	if ( FClassnameIs( attacker, "player" ) && !m_hEnemy )
	{
		CBasePlayer* player = (CBasePlayer*)CBaseEntity::Instance( INDEXENT( 1 ) );
		if ( FStrEq( STRING( player->GetActiveWeaponClassname() ), "weapon_knife" ) )
		{
			stealthDeath = TRUE;
			damage = 105;
		}
	}
	if ( !m_hEnemy )
		m_hEnemy = CBaseEntity::Instance( attacker );
	return CBaseMonster::TakeDamage( inflictor, attacker, damage, damageType );
}

int CTerrorist::IRelationship( CBaseEntity* target )
{
	if ( FClassnameIs( target->pev, "player" ) &&
		((CBasePlayer*)CBaseEntity::Instance( INDEXENT( 1 ) ))->m_iIncognito )
		return R_NO;
	return CBaseMonster::IRelationship( target );
}

BOOL CTerrorist::IsPlayerHidden()
{
	if ( (unsigned int)(m_MonsterState - MONSTERSTATE_ALERT) <= 1 )
		return FALSE;
	CBasePlayer* player = (CBasePlayer*)CBaseEntity::Instance( INDEXENT( 1 ) );
	if ( !player->m_stealthTrigger )
		return FALSE;
	BOOL hidden = player->m_stealthTrigger->IsPlayerHidden();
	if ( !hidden )
		player->m_stealthTrigger = NULL;
	return hidden;
}

BOOL CTerrorist::FOkToSpeak()
{
	if ( !(CTalkMonster::g_talkWaitTime < gpGlobals->time) )
		return FALSE;
	CBasePlayer* player = (CBasePlayer*)CBaseEntity::Instance( INDEXENT( 1 ) );
	if ( gpGlobals->time >= 2 && player && player->m_chatterTime > gpGlobals->time )
		return FALSE;
	if ( (pev->spawnflags & SF_MONSTER_GAG) && m_MonsterState != MONSTERSTATE_COMBAT )
		return FALSE;
	return player->FVisible( this ) != FALSE;
}

void CTerrorist::JustSpoke( float duration )
{
	if ( duration == -1 )
	{
		float time = gpGlobals->time;
		CTalkMonster::g_talkWaitTime = time + RANDOM_FLOAT( 8, 15 );
	}
	else
		CTalkMonster::g_talkWaitTime = gpGlobals->time + duration;
	m_iSentence = -1;
}

void CTerrorist::SpeakSentence()
{
	if ( m_iSentence != -1 && CTalkMonster::g_talkWaitTime < gpGlobals->time && FOkToSpeak() )
	{
		SayNamedSentence( pTerroristSentences[m_iSentence], 0.8f, ATTN_NORM, 0, m_voicePitch );
		JustSpoke( -1 );
	}
}

void CTerrorist::PainSound()
{
	if ( gpGlobals->time > m_flNextPainTime )
	{
		SayNamedSentence( "T_WOUND", 0.8f, ATTN_NORM, 0, m_voicePitch );
		m_flNextPainTime = gpGlobals->time + 1;
	}
}

void CTerrorist::DeathSound()
{
	if ( CTalkMonster::g_talkWaitTime >= gpGlobals->time || !FOkToSpeak() )
		EMIT_SOUND_DYN( edict(), CHAN_VOICE, "common/null.wav", 0.5f, ATTN_NORM, 0, m_voicePitch );
	SayNamedSentence( "T_DEATH", 0.85f, ATTN_NORM, 0, m_voicePitch );
}

void CTerrorist::PlayScriptedSentence( const char* sentence, float duration, float volume, float attenuation,
	BOOL, CBaseEntity* )
{
	if ( IsAlive() && pev->deadflag != DEAD_DYING )
	{
		JustSpoke( duration + 2 );
		PlaySentence( sentence, duration, volume, attenuation );
	}
}

int CTerrorist::CanPlaySequence( BOOL disregardState, int interruptLevel )
{
	if ( m_terroristBehavior == BEHAVIOR_SNIPERRIFLE )
		LaserGlowOff();
	return CBaseMonster::CanPlaySequence( disregardState, interruptLevel );
}

void CTerrorist::PrescheduleThink()
{
	if ( InSquad() && m_hEnemy )
	{
		if ( HasConditions( bits_COND_SEE_ENEMY ) )
			MySquadLeader()->m_flLastEnemySightTime = gpGlobals->time;
		else if ( gpGlobals->time - MySquadLeader()->m_flLastEnemySightTime > 5 )
			MySquadLeader()->m_fEnemyEluded = TRUE;
	}
}

void CTerrorist::SetupWeapons()
{
	m_cClipSize = 36;
	m_cAmmoLoaded = 36;
	if ( m_terroristBehavior == -1 )
	{
		if ( !pev->weapons )
		{
			pev->weapons = 3;
			SetBodygroup( 2, 1 );
			return;
		}
		if ( pev->weapons & 1 ) SetBodygroup( 2, 0 );
		if ( pev->weapons & 8 )
		{
			SetBodygroup( 2, 1 );
			m_cClipSize = 8;
		}
		if ( pev->weapons & 32 ) SetBodygroup( 2, 2 );
		if ( pev->weapons & 16 ) SetBodygroup( 2, 3 );
		if ( pev->weapons & 64 )
		{
			SetBodygroup( 2, 9 );
			m_cClipSize = 0;
		}
	}
	else
	{
		switch ( m_terroristBehavior )
		{
		case BEHAVIOR_PISTOL:
			m_cClipSize = 16;
			SetBodygroup( 2, 5 );
			break;
		case BEHAVIOR_SHOTGUN:
			pev->weapons = 8;
			m_cClipSize = 8;
			SetBodygroup( 2, 1 );
			break;
		case BEHAVIOR_ASSAULTRIFLE:
			pev->weapons = 1;
			m_cClipSize = 30;
			SetBodygroup( 2, 4 );
			break;
		case BEHAVIOR_MACHINEGUN: SetBodygroup( 2, 7 ); break;
		case BEHAVIOR_SNIPERRIFLE:
			pev->weapons = 16;
			SetBodygroup( 2, 3 );
			break;
		case BEHAVIOR_LAW:
			m_cClipSize = 1;
			SetBodygroup( 2, 2 );
			break;
		case BEHAVIOR_SMG:
			m_cClipSize = 45;
			pev->weapons = 1;
			SetBodygroup( 2, 8 );
			break;
		case BEHAVIOR_GRENADER:
			m_cClipSize = 16;
			pev->weapons = 2;
			SetBodygroup( 2, 5 );
			break;
		case BEHAVIOR_MELEE: SetBodygroup( 2, 6 ); break;
		case BEHAVIOR_SUICIDAL:
			SetBodygroup( 2, 9 );
			SetBodygroup( 3, 1 );
			break;
		default: SetBodygroup( 2, 0 ); break;
		}
	}
	m_cAmmoLoaded = m_cClipSize;
}

void CTerrorist::SetHead( int head )
{
	int choice = RANDOM_LONG( 0, 99 );
	if ( head == EASY_HEAD )
		SetBodygroup( 1, choice <= 50 ? 4 : 3 );
	else if ( head == MEDIUM_HEAD )
		SetBodygroup( 1, choice <= 50 ? 2 : 1 );
	else if ( head == HARD_HEAD )
		SetBodygroup( 1, 0 );
}

int CTerrorist::GetFlyingSchedule()
{
	if ( pev->movetype != MOVETYPE_FLY || m_MonsterState == MONSTERSTATE_PRONE )
		return -1;
	if ( !(pev->flags & FL_ONGROUND) )
		return m_MonsterState == MONSTERSTATE_COMBAT ? SCHED_TERRORIST_REPEL_ATTACK : SCHED_TERRORIST_REPEL;
	pev->movetype = MOVETYPE_STEP;
	return SCHED_TERRORIST_REPEL_LAND;
}

int CTerrorist::GetHearingSchedule()
{
	if ( !HasConditions( bits_COND_HEAR_SOUND ) )
		return -1;
	CSound* sound = PBestSound();
	if ( !sound )
		return -1;
	if ( sound->m_iType == bits_SOUND_COMBAT )
		heardCombat = TRUE;
	if ( sound->m_iType & bits_SOUND_DANGER )
		return SCHED_TAKE_COVER_FROM_BEST_SOUND;
	if ( !HasConditions( bits_COND_SEE_ENEMY ) && (sound->m_iType & (bits_SOUND_COMBAT | bits_SOUND_PLAYER)) &&
		m_cantMove <= 1 )
		MakeIdealYaw( sound->m_vecOrigin );
	return -1;
}

int CTerrorist::GetForcedTargetSchedule()
{
	if ( !m_forcedTarget )
		return -1;
	CheckAmmo();
	if ( HasConditions( bits_COND_NO_AMMO_LOADED ) )
		return SCHED_RELOAD;
	return RANDOM_LONG( 0, 100 ) < 31 ? SCHED_TERRORIST_WAIT_FACE_ENEMY : SCHED_FORCED_TARGET;
}

int CTerrorist::GetBehaviorTypeSchedule()
{
	if ( m_terroristBehavior != BEHAVIOR_SUICIDAL )
		return -1;
	if ( suicideFlag )
	{
		TakeDamage( pev, pev, pev->health + 50, DMG_GENERIC );
		ExplosionCreate( pev->origin, pev->angles, edict(), 100, TRUE );
		return -1;
	}
	if ( !HasConditions( bits_COND_SEE_ENEMY ) || !m_hEnemy )
		return -1;
	if ( (pev->origin - m_hEnemy->pev->origin).Length2D() < 64 )
	{
		suicideFlag = TRUE;
		return SCHED_KAMAKAZI_BLOWUP;
	}
	return SCHED_TERRORIST_CLOSEIN;
}

void CTerrorist::SetupLaserGlow()
{
	if ( !m_pBeam )
	{
		m_pBeam = CBeam::BeamCreate( "sprites/laserbeam.spr", 3 );
		m_pBeam->SetFlags( BEAM_FSHADEOUT );
		m_pBeam->pev->spawnflags |= SF_BEAM_TEMPORARY;
		m_pBeam->pev->rendercolor = Vector( 255, 0, 0 );
		m_pBeam->pev->renderamt = 0;
		m_pBeam->SetTransparency( kRenderGlow, 255, 0, 0, 0, kRenderFxNoDissipation );
		m_laserBrightness = 0;
	}
	if ( !m_pLaserGlow )
	{
		m_pLaserGlow = CSprite::SpriteCreate( "sprites/flare3.spr", pev->origin, FALSE );
		m_pLaserGlow->SetTransparency( kRenderGlow, 255, 0, 0, 0, kRenderFxNoDissipation );
		if ( edict() )
			m_pLaserGlow->SetAttachment( edict(), 2 );
		m_pLaserGlow->pev->renderamt = 0;
		m_glowBrightness = 0;
	}
}

void CTerrorist::LaserGlowOn()
{
	if ( (m_pBeam && m_pLaserGlow) || (SetupLaserGlow(), m_pBeam) )
	{
		if ( !m_hEnemy )
			return;
		int end = m_hEnemy->entindex();
		m_pBeam->EntsInit( entindex(), end );
		m_pBeam->SetStartAttachment( 2 );
		if ( m_laserBrightness != 64 )
			m_laserBrightness = 64;
		m_pBeam->pev->renderamt = 64;
	}
	if ( m_pLaserGlow && m_hEnemy->IsPlayer() )
	{
		if ( m_glowBrightness != 255 )
			m_glowBrightness = 255;
		m_pLaserGlow->pev->renderamt = 255;
	}
}

void CTerrorist::LaserGlowOff()
{
	if ( m_pBeam )
	{
		UTIL_Remove( m_pBeam );
		m_pBeam = NULL;
	}
	if ( m_pLaserGlow )
	{
		UTIL_Remove( m_pLaserGlow );
		m_pLaserGlow = NULL;
	}
}

void CTerrorist::GibMonster()
{
	if ( m_terroristBehavior == BEHAVIOR_SNIPERRIFLE )
		LaserGlowOff();
	if ( GetBodygroup( 2 ) != 2 )
	{
		Vector position, angles;
		GetAttachment( 0, position, angles );
		if ( pev->weapons & 4 )
		{
			CBaseEntity* item = DropItem( "ammo_ARgrenades", position, angles );
			if ( item )
			{
				item->pev->velocity = Vector( RANDOM_FLOAT( -100, 100 ), RANDOM_FLOAT( -100, 100 ), RANDOM_FLOAT( 200, 300 ) );
				item->pev->avelocity = Vector( 0, RANDOM_FLOAT( 200, 400 ), 0 );
			}
		}
	}
	CBaseMonster::GibMonster();
}

#pragma warning(push)
#pragma warning(disable: 4305)
void CTerrorist::ShootAssaultRifle()
{
	if ( !m_hEnemy )
		return;
	int damage = 12;
	if ( m_hEnemy->IsPlayer() )
	{
		if ( g_iSkillLevel == SKILL_EASY ) damage = 5;
		else if ( g_iSkillLevel != SKILL_HARD ) damage = g_iSkillLevel == 0 ? 50 : 8;
	}
	Vector origin = GetGunPosition();
	Vector direction = ShootAtEnemy( origin );
	UTIL_MakeVectors( pev->angles );
	Vector spread = m_weaponAccuracy.Length() <= 0 ? VECTOR_CONE_10DEGREES : m_weaponAccuracy;
	FireBullets( 1, origin, direction, spread, 2048, BULLET_MONSTER_MP5, 4, damage );
	pev->effects |= EF_MUZZLEFLASH;
	--m_cAmmoLoaded;
	Vector angles = UTIL_VecToAngles( direction );
	SetBlending( 0, angles.x );
	Vector forward = gpGlobals->v_forward * RANDOM_FLOAT( -40, 40 );
	Vector up = gpGlobals->v_up * RANDOM_FLOAT( 75, 200 );
	Vector velocity = gpGlobals->v_right * RANDOM_FLOAT( 40, 90 ) + up + forward;
	EjectBrass( origin + direction * 8, velocity, pev->angles.y, m_iBrassShell, TE_BOUNCE_SHELL );
	const char* sound = RANDOM_LONG( 0, 1 ) ? "weapons/ak47-1.wav" : "weapons/ak47-2.wav";
	EMIT_SOUND( edict(), CHAN_WEAPON, sound, 0.75f, ATTN_NORM );
}

void CTerrorist::ShootSMG()
{
	if ( !m_hEnemy )
		return;
	Vector origin = GetGunPosition();
	Vector direction = ShootAtEnemy( origin );
	UTIL_MakeVectors( pev->angles );
	int damage = 4;
	if ( m_hEnemy->IsPlayer() )
	{
		if ( g_iSkillLevel == SKILL_EASY ) damage = 1;
		else if ( g_iSkillLevel != SKILL_HARD ) damage = g_iSkillLevel == 0 ? 35 : 2;
	}
	Vector spread = m_weaponAccuracy.Length() <= 0 ? VECTOR_CONE_10DEGREES : m_weaponAccuracy;
	FireBullets( 1, origin, direction, spread, 2048, BULLET_MONSTER_MP5, 4, damage );
	pev->effects |= EF_MUZZLEFLASH;
	--m_cAmmoLoaded;
	Vector angles = UTIL_VecToAngles( direction );
	SetBlending( 0, angles.x );
	Vector forward = gpGlobals->v_forward * RANDOM_FLOAT( -40, 40 );
	Vector up = gpGlobals->v_up * RANDOM_FLOAT( 75, 200 );
	Vector velocity = gpGlobals->v_right * RANDOM_FLOAT( 40, 90 ) + up + forward;
	EjectBrass( origin + direction * 10, velocity, pev->angles.y, m_iBrassShell, TE_BOUNCE_SHELL );
	const char* sound = RANDOM_LONG( 0, 1 ) ? "weapons/tmp-1.wav" : "weapons/tmp-2.wav";
	EMIT_SOUND( edict(), CHAN_WEAPON, sound, 0.75f, ATTN_NORM );
}

void CTerrorist::ShootMP5()
{
	if ( !m_hEnemy )
		return;
	int damage = 10;
	if ( m_hEnemy->IsPlayer() )
	{
		if ( g_iSkillLevel == SKILL_EASY ) damage = 3;
		else if ( g_iSkillLevel != SKILL_HARD ) damage = g_iSkillLevel == 0 ? 50 : 6;
	}
	Vector origin = GetGunPosition();
	Vector direction = ShootAtEnemy( origin );
	UTIL_MakeVectors( pev->angles );
	Vector spread = m_weaponAccuracy.Length() <= 0 ? VECTOR_CONE_10DEGREES : m_weaponAccuracy;
	FireBullets( 1, origin, direction, spread, 2048, BULLET_MONSTER_MP5, 6, damage );
	pev->effects |= EF_MUZZLEFLASH;
	--m_cAmmoLoaded;
	Vector angles = UTIL_VecToAngles( direction );
	SetBlending( 0, angles.x );
	Vector forward = gpGlobals->v_forward * RANDOM_FLOAT( -40, 40 );
	Vector up = gpGlobals->v_up * RANDOM_FLOAT( 75, 200 );
	Vector velocity = gpGlobals->v_right * RANDOM_FLOAT( 40, 90 ) + up + forward;
	EjectBrass( origin + direction * 8, velocity, pev->angles.y, m_iBrassShell, TE_BOUNCE_SHELL );
	const char* sound = RANDOM_LONG( 0, 1 ) ? "weapons/mp5-1.wav" : "weapons/mp5-2.wav";
	EMIT_SOUND( edict(), CHAN_WEAPON, sound, 0.75f, ATTN_NORM );
}

void CTerrorist::ShootMachineGun()
{
	if ( !m_hEnemy )
		return;
	int damage = 15;
	if ( m_hEnemy->IsPlayer() )
	{
		if ( g_iSkillLevel == SKILL_EASY ) damage = 8;
		else if ( g_iSkillLevel != SKILL_HARD ) damage = g_iSkillLevel == 0 ? 50 : 11;
	}
	Vector origin = GetGunPosition();
	Vector direction = ShootAtEnemy( origin );
	UTIL_MakeVectors( pev->angles );
	Vector spread = m_weaponAccuracy.Length() <= 0 ? VECTOR_CONE_10DEGREES : m_weaponAccuracy;
	FireBullets( 1, origin, direction, spread, 2048, BULLET_MONSTER_MP5, 8, damage );
	pev->effects |= EF_MUZZLEFLASH;
	Vector angles = UTIL_VecToAngles( direction );
	SetBlending( 0, angles.x );
	Vector forward = gpGlobals->v_forward * RANDOM_FLOAT( -40, 40 );
	Vector up = gpGlobals->v_up * RANDOM_FLOAT( 75, 200 );
	Vector velocity = gpGlobals->v_right * RANDOM_FLOAT( 40, 90 ) + up + forward;
	EjectBrass( origin + direction * 8, velocity, pev->angles.y, m_iBrassShell, TE_BOUNCE_SHELL );
	const char* sound = RANDOM_LONG( 0, 1 ) ? "weapons/m60-1.wav" : "weapons/m60-2.wav";
	EMIT_SOUND( edict(), CHAN_WEAPON, sound, 0.75f, ATTN_NORM );
}

void CTerrorist::ShootPistol()
{
	if ( !m_hEnemy )
		return;
	int damage = 5;
	if ( m_hEnemy->IsPlayer() )
	{
		if ( g_iSkillLevel == SKILL_EASY ) damage = 1;
		else if ( g_iSkillLevel != SKILL_HARD ) damage = g_iSkillLevel == 0 ? 35 : 2;
	}
	Vector origin = GetGunPosition();
	Vector direction = ShootAtEnemy( origin );
	UTIL_MakeVectors( pev->angles );
	bool burst = RANDOM_LONG( 0, 99 ) <= 14 && m_cAmmoLoaded > 2;
	Vector spread = m_weaponAccuracy.Length() <= 0 ? VECTOR_CONE_10DEGREES : m_weaponAccuracy;
	FireBullets( burst ? 3 : 1, origin, direction, spread, 1024, BULLET_MONSTER_MP5, 0, damage );
	m_cAmmoLoaded -= burst ? 3 : 1;
	EMIT_SOUND( edict(), CHAN_WEAPON, burst ? "weapons/glock18-1.wav" : "weapons/glock18-2.wav", 0.75f, ATTN_NORM );
	pev->effects |= EF_MUZZLEFLASH;
	Vector angles = UTIL_VecToAngles( direction );
	SetBlending( 0, angles.x );
	Vector forward = gpGlobals->v_forward * RANDOM_FLOAT( -40, 40 );
	Vector up = gpGlobals->v_up * RANDOM_FLOAT( 75, 200 );
	Vector velocity = gpGlobals->v_right * RANDOM_FLOAT( 40, 90 ) + up + forward;
	EjectBrass( origin + direction * 13, velocity, pev->angles.y, m_iBrassShell, TE_BOUNCE_SHELL );
}

void CTerrorist::ShootShotgun()
{
	if ( !m_hEnemy )
		return;
	int damage = 10;
	if ( m_hEnemy->IsPlayer() )
	{
		if ( g_iSkillLevel == SKILL_EASY ) damage = 3;
		else if ( g_iSkillLevel != SKILL_HARD ) damage = g_iSkillLevel == 0 ? 20 : 5;
	}
	Vector origin = GetGunPosition();
	Vector direction = ShootAtEnemy( origin );
	UTIL_MakeVectors( pev->angles );
	Vector spread = m_weaponAccuracy.Length() <= 0 ? VECTOR_CONE_15DEGREES : m_weaponAccuracy;
	FireBullets( 5, origin, direction, spread, 2048, BULLET_PLAYER_BUCKSHOT, 0, damage );
	Vector angles = UTIL_VecToAngles( direction );
	SetBlending( 0, angles.x );
	pev->effects |= EF_MUZZLEFLASH;
	--m_cAmmoLoaded;
	Vector forward = gpGlobals->v_forward * RANDOM_FLOAT( -40, 40 );
	Vector up = gpGlobals->v_up * RANDOM_FLOAT( 75, 200 );
	Vector velocity = gpGlobals->v_right * RANDOM_FLOAT( 40, 90 ) + up + forward;
	EjectBrass( origin + direction * 8, velocity, pev->angles.y, m_iShotgunShell, TE_BOUNCE_SHOTSHELL );
	EMIT_SOUND( edict(), CHAN_WEAPON, "weapons/sbarrel1.wav", 0.75f, ATTN_NORM );
}

void CTerrorist::ShootSniperRifle()
{
	if ( !m_hEnemy )
		return;
	int damage = 45;
	if ( m_hEnemy->IsPlayer() )
	{
		if ( g_iSkillLevel == SKILL_EASY ) damage = 18;
		else if ( g_iSkillLevel != SKILL_HARD ) damage = g_iSkillLevel == 0 ? 115 : 25;
	}
	Vector origin = GetGunPosition();
	Vector direction = ShootAtEnemy( origin );
	UTIL_MakeVectors( pev->angles );
	Vector spread = m_weaponAccuracy.Length() <= 0 ? VECTOR_CONE_5DEGREES : m_weaponAccuracy;
	FireBullets( 1, origin, direction, spread, 4096, BULLET_MONSTER_MP5, 0, damage );
	pev->effects |= EF_MUZZLEFLASH;
	Vector angles = UTIL_VecToAngles( direction );
	SetBlending( 0, angles.x );
	Vector forward = gpGlobals->v_forward * RANDOM_FLOAT( -40, 40 );
	Vector up = gpGlobals->v_up * RANDOM_FLOAT( 75, 200 );
	Vector velocity = gpGlobals->v_right * RANDOM_FLOAT( 40, 90 ) + up + forward;
	EjectBrass( origin + direction * 8, velocity, pev->angles.y, m_iBrassShell, TE_BOUNCE_SHELL );
	EMIT_SOUND( edict(), CHAN_WEAPON, "weapons/awp1.wav", VOL_NORM, ATTN_NONE );
}

#pragma warning(pop)

void CTerrorist::ShootLAW()
{
	if ( !m_hEnemy )
		return;
	lawDelayTime = gpGlobals->time + 5;
	Vector origin = GetGunPosition();
	Vector direction = ShootAtEnemy( origin );
	Vector angles = UTIL_VecToAngles( direction );
	angles.x *= -1;
	UTIL_MakeVectors( angles );
	float x = RANDOM_FLOAT( -0.5f, 0.5f );
	float horizontal = RANDOM_FLOAT( -0.5f, 0.5f ) + x;
	float y = RANDOM_FLOAT( -0.5f, 0.5f );
	float vertical = RANDOM_FLOAT( -0.5f, 0.5f ) + y;
	Vector spreadDirection = direction + gpGlobals->v_right * ( horizontal * m_weaponAccuracy.x ) +
		gpGlobals->v_up * ( vertical * m_weaponAccuracy.y );
	Vector rocketAngles = UTIL_VecToAngles( spreadDirection );
	rocketAngles.x *= -1;
	CLawsRocket::CreateLawsRocket( origin, rocketAngles, this, NULL );
	unsigned short event = PRECACHE_EVENT( 1, "events/laws.sc" );
	PLAYBACK_EVENT_FULL( 0, edict(), event, 0, (float*)&g_vecZero, (float*)&g_vecZero, 0, 0, 0, 0, 0, 0 );
	angles.x *= -1;
	SetBlending( 0, angles.x );
	--m_cAmmoLoaded;
}

CBaseEntity* CTerrorist::Kick()
{
	UTIL_MakeVectors( pev->angles );
	Vector start = pev->origin;
	start.z += pev->size.z * 0.75f;
	Vector end;
	if ( m_hEnemy )
	{
		Vector direction = m_hEnemy->pev->origin - Vector( 0, 0, pev->size.z * 0.25f ) - start;
		end = start + direction.Normalize() * 128;
	}
	else
		end = start + gpGlobals->v_forward * 128;
	TraceResult trace;
	UTIL_TraceHull( start, end, dont_ignore_monsters, head_hull, edict(), &trace );
	return trace.pHit ? (CBaseEntity*)GET_PRIVATE( trace.pHit ) : NULL;
}

int CTerrorist::GetWeaponEffectSchedule()
{
	if ( m_iWeaponEffectType == 7 && m_bWeaponEffectActive )
	{
		m_iWeaponEffectType = 0;
		m_bWeaponEffectActive = FALSE;
		if ( CTalkMonster::g_talkWaitTime < gpGlobals->time && FOkToSpeak() )
		{
			SayNamedSentence( "T_INSMOKE", 0.8f, ATTN_NORM, 0, m_voicePitch );
			JustSpoke( -1 );
		}
		int choice = RANDOM_LONG( 0, 99 );
		if ( choice <= 69 ) return SCHED_TERRORIST_IN_SMOKE;
		if ( choice <= 89 ) return SCHED_SMALL_FLINCH;
	}
	if ( m_iWeaponEffectType == 4 && m_bWeaponEffectActive )
	{
		m_iWeaponEffectType = 0;
		m_bWeaponEffectActive = FALSE;
		if ( CTalkMonster::g_talkWaitTime < gpGlobals->time && FOkToSpeak() )
		{
			SayNamedSentence( "T_FLASHED", 0.8f, ATTN_NORM, 0, m_voicePitch );
			JustSpoke( -1 );
		}
		return SCHED_TERRORIST_FLASHED;
	}
	return -1;
}

void CTerrorist::IdleSound()
{
	if ( RANDOM_LONG( 0, 99 ) > 50 || !(CTalkMonster::g_talkWaitTime < gpGlobals->time) || !FOkToSpeak() )
		return;
	if ( !g_fTerroristQuestion && !RANDOM_LONG( 0, 1 ) )
		return;
	if ( !g_fTerroristQuestion )
	{
		switch ( RANDOM_LONG( 0, 2 ) )
		{
		case 0:
			SayNamedSentence( "T_CHECK", 0.8f, ATTN_NORM, 0, m_voicePitch );
			g_fTerroristQuestion = 1;
			break;
		case 1:
			SayNamedSentence( "T_QUEST", 0.8f, ATTN_NORM, 0, m_voicePitch );
			g_fTerroristQuestion = 2;
			break;
		case 2:
			SayNamedSentence( "T_IDLE", 0.8f, ATTN_NORM, 0, m_voicePitch );
			break;
		}
	}
	else
	{
		if ( g_fTerroristQuestion == 1 )
			SayNamedSentence( "T_CLEAR", 0.8f, ATTN_NORM, 0, m_voicePitch );
		else if ( g_fTerroristQuestion == 2 )
			SayNamedSentence( "T_ANSWER", 0.8f, ATTN_NORM, 0, m_voicePitch );
		g_fTerroristQuestion = 0;
	}
	JustSpoke( -1 );
}

void CTerrorist::HandleSpeaking( int schedule )
{
	if ( !(CTalkMonster::g_talkWaitTime < gpGlobals->time) || !FOkToSpeak() )
		return;
	const char* sentence;
	switch ( schedule )
	{
	case SCHED_TAKE_COVER_FROM_BEST_SOUND: sentence = "T_GREN"; break;
	case SCHED_RANGE_ATTACK1:
		if ( RANDOM_LONG( 0, 99 ) > 34 )
			return;
		sentence = "T_ATTACK";
		break;
	case SCHED_RANGE_ATTACK2: sentence = "T_THROW"; break;
	case SCHED_STANDOFF: sentence = "T_TAUNT"; break;
	case SCHED_TERRORIST_CLOSEIN:
		if ( RANDOM_LONG( 0, 0 ) == 0 )
			return;
		sentence = "T_CHARGE";
		break;
	default: return;
	}
	SayNamedSentence( sentence, 0.8f, ATTN_NORM, 0, m_voicePitch );
	JustSpoke( -1 );
}

void CTerrorist::StartTask( Task_t* task )
{
	TaskBegin();
	switch ( task->iTask )
	{
	case TASK_RUN_PATH:
	case TASK_WALK_PATH:
		Forget( bits_MEMORY_INCOVER );
		CBaseMonster::StartTask( task );
		break;
	case TASK_FACE_IDEAL:
	case TASK_FACE_ENEMY:
		if ( m_cantMove > 1 && m_MonsterState != MONSTERSTATE_DEAD )
			TaskComplete();
		else
		{
			CBaseMonster::StartTask( task );
			if ( pev->movetype == MOVETYPE_FLY )
				m_IdealActivity = ACT_GLIDE;
		}
		break;
	case TASK_RELOAD:
		m_IdealActivity = ACT_RELOAD;
		break;
	case TASK_TERRORIST_FACE_TOSS_DIR:
		break;
	case TASK_TERRORIST_SPEAK_SENTENCE:
		SpeakSentence();
		TaskComplete();
		break;
	case TASK_TERRORIST_CHECK_FIRE:
		if ( !NoFriendlyFire() )
			SetConditions( bits_COND_SPECIAL1 );
		TaskComplete();
		break;
	case TASK_MOVE_TO_SPECIAL_COVER_NODE:
		if ( !m_pSpecialCoverNode || !FValidateCover( m_pSpecialCoverNode->m_vecOrigin ) )
			TaskFail();
		else if ( MoveToLocation( ACT_RUN, 0, m_pSpecialCoverNode->m_vecOrigin ) )
			TaskComplete();
		else
		{
			ALERT( at_console, "FAILED MOVING TO LOCATION.\n" );
			TaskFail();
		}
		break;
	case TASK_MOVE_TO_PREFERED_RANGE:
		if ( m_cantMove || (m_hEnemy->pev->origin - pev->origin).Length2D() < 64 )
			TaskComplete();
		else
		{
			m_vecMoveGoal = m_hEnemy->pev->origin;
			if ( !MoveToEnemy( ACT_RUN, 0 ) )
				TaskFail();
		}
		break;
	case TASK_WANDER:
		if ( MoveToLocation( ACT_WALK, 0, m_wanderOrigin ) )
			TaskComplete();
		else
			TaskFail();
		break;
	default:
		CBaseMonster::StartTask( task );
		break;
	}
}

void CTerrorist::RunTask( Task_t* task )
{
	if ( task->iTask == TASK_TERRORIST_FACE_TOSS_DIR )
	{
		MakeIdealYaw( pev->origin + m_vecTossVelocity * 64 );
		ChangeYaw( (int)pev->yaw_speed );
		if ( FacingIdeal() )
			m_iTaskStatus = TASKSTATUS_COMPLETE;
	}
	else if ( task->iTask == TASK_MOVE_TO_PREFERED_RANGE )
	{
		if ( m_cantMove )
		{
			TaskComplete();
			return;
		}
		if ( !m_hEnemy )
		{
			TaskFail();
			return;
		}
		float distance = (m_vecMoveGoal - pev->origin).Length2D();
		if ( m_fPreferedRange > distance || !((m_vecMoveGoal - m_hEnemy->pev->origin).Length() <= 0.5 * m_fPreferedRange) )
		{
			m_vecMoveGoal = m_hEnemy->pev->origin;
			distance = (m_vecMoveGoal - pev->origin).Length2D();
			FRefreshRoute();
		}
		if ( (unsigned int)(m_terroristBehavior - BEHAVIOR_MELEE) <= 1 && CTalkMonster::g_talkWaitTime < gpGlobals->time &&
			FOkToSpeak() && distance < 312 )
		{
			SayNamedSentence( "T_MELEE", VOL_NORM, ATTN_NORM, 0, m_voicePitch );
			JustSpoke( 10 );
		}
		if ( m_fPreferedRange > distance )
		{
			TaskComplete();
			RouteClear();
		}
		if ( m_movementActivity != ACT_RUN )
			m_movementActivity = ACT_RUN;
	}
	else
		CBaseMonster::RunTask( task );
}

BOOL CTerrorist::CheckMeleeAttack1( float dot, float )
{
	if ( m_hEnemy && !m_hEnemy->MyMonsterPointer() )
		return FALSE;
	if ( (pev->origin - m_hEnemy->pev->origin).Length2D() > 80 )
		return FALSE;
	return !(dot < 0.7);
}

BOOL CTerrorist::CheckRangeAttack1( float dot, float distance )
{
	if ( m_terroristBehavior == BEHAVIOR_MELEE || HasConditions( bits_COND_ENEMY_OCCLUDED ) ||
		!( distance <= 2048 ) || !( dot >= 0.5f ) || !NoFriendlyFire() )
		return FALSE;
	Vector origin = GetGunPosition();
	edict_t* ignore = edict();
	Vector target = m_hEnemy->BodyTarget( origin );
	TraceResult trace;
	UTIL_TraceLine( origin, target, ignore_monsters, ignore_glass, ignore, &trace );
	return trace.flFraction == 1;
}

BOOL CTerrorist::CheckRangeAttack2( float, float )
{
	if ( !(pev->weapons & 6) )
		return FALSE;
	if ( m_flGroundSpeed != 0 )
	{
		m_fThrowGrenade = FALSE;
		return FALSE;
	}
	if ( m_flNextGrenadeCheck > gpGlobals->time )
		return m_fThrowGrenade;
	if ( !(m_hEnemy->pev->flags & FL_ONGROUND) && !m_hEnemy->pev->waterlevel && m_vecEnemyLKP.z > pev->absmax.z )
	{
		m_fThrowGrenade = FALSE;
		return FALSE;
	}
	Vector target;
	if ( pev->weapons & 2 )
	{
		if ( RANDOM_LONG( 0, 1 ) )
		{
			target.z = m_hEnemy->pev->absmin.z;
			target.y = m_hEnemy->pev->origin.y;
			target.x = m_hEnemy->pev->origin.x;
		}
		else
			target = m_vecEnemyLKP;
	}
	else
	{
		entvars_t* enemy = m_hEnemy->pev;
		target = m_hEnemy->BodyTarget( pev->origin ) - enemy->origin + m_vecEnemyLKP;
		if ( HasConditions( bits_COND_SEE_ENEMY ) )
		{
			enemy = m_hEnemy->pev;
			float flightTime = (target - pev->origin).Length() / gSkillData.hgruntGrenadeSpeed;
			target = target + enemy->velocity * flightTime;
		}
	}
	if ( (target - pev->origin).Length2D() <= 128 )
	{
		m_flNextGrenadeCheck = gpGlobals->time + 1;
		m_fThrowGrenade = FALSE;
		return FALSE;
	}
	Vector toss;
	bool tossGrenade = (pev->weapons & 2) != 0;
	if ( tossGrenade )
		toss = VecCheckToss( pev, GetGunPosition(), target, 0.5f );
	else
	{
		float speed = gSkillData.hgruntGrenadeSpeed;
		toss = VecCheckThrow( pev, GetGunPosition(), target, speed, 0.5f );
	}
	if ( toss == g_vecZero )
	{
		m_fThrowGrenade = FALSE;
		m_flNextGrenadeCheck = gpGlobals->time + 1;
		return FALSE;
	}
	m_vecTossVelocity = toss;
	m_fThrowGrenade = TRUE;
	if ( tossGrenade )
		m_flNextGrenadeCheck = gpGlobals->time;
	else
		m_flNextGrenadeCheck = gpGlobals->time + 0.3;
	return TRUE;
}

int CTerrorist::LookupActivity( int activity )
{
	void* model = GET_MODEL_PTR( edict() );
	if ( activity != ACT_IDLE )
		return ::LookupActivity( model, pev, activity );
	switch ( m_terroristBehavior )
	{
	case BEHAVIOR_PISTOL:
	case BEHAVIOR_SMG:
	case BEHAVIOR_GRENADER:
		return LookupSequence( m_MonsterState == MONSTERSTATE_COMBAT || heardCombat ? "pistol_combatidlle" : "pistol_idle1" );
	case BEHAVIOR_LAW:
		return LookupSequence( "LAW_fire_idle" );
	case BEHAVIOR_MELEE:
		return LookupSequence( m_MonsterState == MONSTERSTATE_COMBAT || heardCombat ? "machete_combatidle" : "machete_idle1" );
	case BEHAVIOR_SUICIDAL:
		return LookupSequence( "machete_idle1" );
	default:
		if ( m_MonsterState == MONSTERSTATE_COMBAT || heardCombat )
			return LookupSequence( m_bStanding ? "combatidle" : "crouching_wait" );
		return ::LookupActivity( model, pev, ACT_IDLE );
	}
}

int CTerrorist::LookupActivityHeaviest( int activity )
{
	void* model = GET_MODEL_PTR( edict() );
	if ( activity != ACT_IDLE )
		return ::LookupActivity( model, pev, activity );
	switch ( m_terroristBehavior )
	{
	case BEHAVIOR_PISTOL:
	case BEHAVIOR_SMG:
	case BEHAVIOR_GRENADER:
		return LookupSequence( m_MonsterState == MONSTERSTATE_COMBAT || heardCombat ? "pistol_combatidlle" : "pistol_idle1" );
	case BEHAVIOR_LAW:
		return LookupSequence( "LAW_fire_idle" );
	case BEHAVIOR_MELEE:
		return LookupSequence( m_MonsterState == MONSTERSTATE_COMBAT || heardCombat ? "machete_combatidle" : "machete_idle1" );
	case BEHAVIOR_SUICIDAL:
		return LookupSequence( "machete_idle1" );
	default:
		if ( m_MonsterState == MONSTERSTATE_COMBAT || heardCombat )
			return LookupSequence( m_bStanding ? "combatidle" : "crouching_wait" );
		return ::LookupActivityHeaviest( model, pev, ACT_IDLE );
	}
}

int CTerrorist::SetActivityPistolBase( Activity activity )
{
	GET_MODEL_PTR( edict() );
	const char* name;
	switch ( activity )
	{
	case ACT_IDLE:
		if ( UseIdleAnim() ) name = STRING( m_idleAnim );
		else if ( m_MonsterState == MONSTERSTATE_COMBAT || heardCombat )
			name = m_bStanding ? "pistol_combatidlle" : "pistol_crouching_wait";
		else name = "pistol_idle1";
		break;
	case ACT_WALK:
		name = pev->max_health * 0.2f < pev->health && m_MonsterState == MONSTERSTATE_COMBAT ?
			"combat_walk_pistol" : "pistol_walk";
		break;
	case ACT_RUN:
		name = pev->max_health * 0.2f < pev->health && m_MonsterState == MONSTERSTATE_COMBAT ?
			"combat_run_pistol" : "pistol_run";
		break;
	case ACT_STRAFE_LEFT: name = "pistol_strafefire_L"; break;
	case ACT_STRAFE_RIGHT: name = "pistol_strafefire_R"; break;
	case ACT_TURN_LEFT: name = "pistol_180L"; break;
	case ACT_TURN_RIGHT: name = "pistol_180R"; break;
	case ACT_CROUCH: name = "pistol_crouching_wait"; break;
	case ACT_COWER: name = "pistol_cower"; break;
	case ACT_SMALL_FLINCH: name = "pistol_smflinch"; break;
	case ACT_RANGE_ATTACK1:
		if ( m_terroristBehavior == BEHAVIOR_SMG ) name = m_bStanding ? "shootcycle_tmp" : "crouching_tmp";
		else name = m_bStanding ? "pistol_standing" : "pistol_crouching";
		break;
	case ACT_MELEE_ATTACK1: name = "pistol_melee"; break;
	case ACT_RELOAD: name = m_bStanding ? "pistol_reload" : "crouch_pistol_reload"; break;
	case ACT_DIESIMPLE: name = m_deathAnim ? STRING( m_deathAnim ) : "pistol_die-simple"; break;
	case ACT_DIEBACKWARD: name = m_deathAnim ? STRING( m_deathAnim ) : "pistol_dieback1"; break;
	case ACT_DIEFORWARD:
	case ACT_DIE_BACKSHOT: name = m_deathAnim ? STRING( m_deathAnim ) : "pistol_dieforward"; break;
	case ACT_DIEVIOLENT:
	case ACT_DIE_HEADSHOT: name = m_deathAnim ? STRING( m_deathAnim ) : "pistol_headshot"; break;
	case ACT_DIE_CHESTSHOT:
	case ACT_DIE_GUTSHOT: name = m_deathAnim ? STRING( m_deathAnim ) : "pistol_diegutshot"; break;
	case ACT_FLINCH_LEFTARM: name = "pistol_leftarmflinch"; break;
	case ACT_FLINCH_RIGHTARM: name = "pistol_rightarmflinch"; break;
	case ACT_FLINCH_LEFTLEG: name = "pistol_leftlegsmflinch"; break;
	case ACT_FLINCH_RIGHTLEG: name = "pistol_rightlegsmflinch"; break;
	default: return -1;
	}
	return LookupSequence( name );
}

int CTerrorist::SetActivityMeleeBase( Activity activity )
{
	GET_MODEL_PTR( edict() );
	const char* name;
	switch ( activity )
	{
	case ACT_IDLE:
		if ( UseIdleAnim() ) name = STRING( m_idleAnim );
		else name = m_MonsterState == MONSTERSTATE_COMBAT || heardCombat ? "machete_combatidle" : "machete_idle1";
		break;
	case ACT_WALK: name = "machete_patrol_street"; break;
	case ACT_RUN: name = "machete_run"; break;
	case ACT_STRAFE_LEFT: name = "machete_strafefire_L"; break;
	case ACT_STRAFE_RIGHT: name = "machete_strafefire_R"; break;
	case ACT_TURN_LEFT: name = "machete_180L"; break;
	case ACT_TURN_RIGHT: name = "machete_180R"; break;
	case ACT_COWER: name = "machete_cower"; break;
	case ACT_SMALL_FLINCH: name = "machete_smflinch"; break;
	case ACT_MELEE_ATTACK1:
		switch ( RANDOM_LONG( 0, 5 ) )
		{
		case 1: name = "machete_melee2"; break;
		case 2: name = "machete_melee3"; break;
		case 3: name = "machete_melee4"; break;
		case 4: name = "machete_melee5"; break;
		case 5: name = "machete_melee6"; break;
		default: name = "machete_melee"; break;
		}
		break;
	case ACT_DIESIMPLE: name = m_deathAnim ? STRING( m_deathAnim ) : "machete_die-simple"; break;
	case ACT_DIEBACKWARD: name = m_deathAnim ? STRING( m_deathAnim ) : "machete_dieback1"; break;
	case ACT_DIEFORWARD:
	case ACT_DIE_BACKSHOT: name = m_deathAnim ? STRING( m_deathAnim ) : "machete_dieforward"; break;
	case ACT_DIEVIOLENT:
	case ACT_DIE_HEADSHOT: name = m_deathAnim ? STRING( m_deathAnim ) : "machete_headshot"; break;
	case ACT_DIE_CHESTSHOT:
	case ACT_DIE_GUTSHOT: name = m_deathAnim ? STRING( m_deathAnim ) : "machete_diegutshot"; break;
	case ACT_FLINCH_LEFTARM: name = "machete_leftarmflinch"; break;
	case ACT_FLINCH_RIGHTARM: name = "machete_rightarmflinch"; break;
	case ACT_FLINCH_LEFTLEG: name = "machete_leftlegsmflinch"; break;
	case ACT_FLINCH_RIGHTLEG: name = "machete_rightlegsmflinch"; break;
	default: return -1;
	}
	return LookupSequence( name );
}

int CTerrorist::SetActivitySniperBase( Activity activity )
{
	GET_MODEL_PTR( edict() );
	switch ( activity )
	{
	case ACT_RANGE_ATTACK1:
		return LookupSequence( m_bStanding ? "sniper-fire" : "sniper_crouch-fire" );
	case ACT_ARM:
		{
			int sequence = LookupSequence( "laser_on_stand" );
			LaserGlowOn();
			return sequence;
		}
	case ACT_DISARM:
		{
			int sequence = LookupSequence( "laser_off_stand" );
			LaserGlowOff();
			return sequence;
		}
	case ACT_DIESIMPLE:
	case ACT_DIEBACKWARD:
	case ACT_DIEFORWARD:
	case ACT_DIEVIOLENT:
	case ACT_DIE_HEADSHOT:
	case ACT_DIE_CHESTSHOT:
	case ACT_DIE_GUTSHOT:
	case ACT_DIE_BACKSHOT:
		LaserGlowOff();
		if ( m_pBeam )
		{
			UTIL_Remove( m_pBeam );
			m_pBeam = NULL;
		}
		if ( m_pLaserGlow )
		{
			UTIL_Remove( m_pLaserGlow );
			m_pLaserGlow = NULL;
		}
		break;
	default: break;
	}
	return -1;
}

int CTerrorist::SetActivityKamakaziBase( Activity activity )
{
	GET_MODEL_PTR( edict() );
	switch ( (int)activity )
	{
	case ACT_IDLE: return LookupSequence( "machete_idle1" );
	case ACT_RUN: return LookupSequence( "pistol_run" );
	case ACT_TURN_LEFT: return LookupSequence( "machete_180L" );
	case ACT_TURN_RIGHT: return LookupSequence( "machete_180R" );
	case ACT_TERRORIST_KAMAKAZI_EXPLODE:
		if ( suicideFlag )
			return LookupSequence( "kamakazi" );
		break;
	default: break;
	}
	return -1;
}

int CTerrorist::SetActivityShotgunBase( Activity activity )
{
	GET_MODEL_PTR( edict() );
	if ( activity == ACT_RANGE_ATTACK1 )
		return LookupSequence( m_bStanding ? "standing_shotgun" : "crouching_shotgun" );
	if ( activity == ACT_RELOAD )
		return LookupSequence( m_bStanding ? "reload_shotgun" : "crouch_reload_shotgun" );
	return -1;
}

int CTerrorist::SetActivityGrenadeBase( Activity activity )
{
	GET_MODEL_PTR( edict() );
	if ( activity != ACT_RANGE_ATTACK2 )
		return -1;
	return LookupSequence( (pev->weapons & 2) ? "throwgrenade" : "launchgrenade" );
}

int CTerrorist::SetActivityAutomaticBase( Activity activity )
{
	GET_MODEL_PTR( edict() );
	if ( activity == ACT_RANGE_ATTACK1 )
	{
		if ( m_terroristBehavior == BEHAVIOR_MACHINEGUN )
			return LookupSequence( "machinegun_fire" );
		return LookupSequence( m_bStanding ? "standing_mp5" : "crouching_mp5" );
	}
	if ( activity == ACT_RELOAD )
		return LookupSequence( m_bStanding ? "reload_mp5" : "crouch_reload_mp5" );
	return -1;
}

int CTerrorist::SetActivityLAWBase( Activity activity )
{
	GET_MODEL_PTR( edict() );
	switch ( activity )
	{
	case ACT_IDLE: return LookupSequence( UseIdleAnim() ? STRING( m_idleAnim ) : "LAW_fire_idle" );
	case ACT_RUN: return LookupSequence( "law_run" );
	case ACT_TURN_LEFT: return LookupSequence( "law_180L" );
	case ACT_TURN_RIGHT: return LookupSequence( "law_180R" );
	case ACT_RANGE_ATTACK1: return LookupSequence( "LAW_fire" );
	case ACT_RELOAD: return LookupSequence( "law_reload" );
	default: return -1;
	}
}

void CTerrorist::SetActivity( Activity activity )
{
	GET_MODEL_PTR( edict() );
	int sequence = -1;
	switch ( m_terroristBehavior )
	{
	case BEHAVIOR_PISTOL:
	case BEHAVIOR_SMG: sequence = SetActivityPistolBase( activity ); break;
	case BEHAVIOR_SHOTGUN: sequence = SetActivityShotgunBase( activity ); break;
	case BEHAVIOR_ASSAULTRIFLE:
	case BEHAVIOR_MACHINEGUN:
	case BEHAVIOR_MP5: sequence = SetActivityAutomaticBase( activity ); break;
	case BEHAVIOR_SNIPERRIFLE: sequence = SetActivitySniperBase( activity ); break;
	case BEHAVIOR_LAW: sequence = SetActivityLAWBase( activity ); break;
	case BEHAVIOR_GRENADER:
		sequence = SetActivityPistolBase( activity );
		if ( sequence == -1 ) sequence = SetActivityGrenadeBase( activity );
		break;
	case BEHAVIOR_MELEE: sequence = SetActivityMeleeBase( activity ); break;
	case BEHAVIOR_SUICIDAL: sequence = SetActivityKamakaziBase( activity ); break;
	default: break;
	}
	if ( sequence == -1 )
	{
		switch ( (int)activity )
		{
		case ACT_IDLE:
			if ( UseIdleAnim() ) sequence = LookupSequence( STRING( m_idleAnim ) );
			else if ( m_MonsterState == MONSTERSTATE_COMBAT )
				sequence = LookupSequence( m_bStanding ? "combatidle" : "crouching_wait" );
			break;
		case ACT_WALK:
			if ( 0.2f * pev->max_health >= pev->health ) sequence = LookupActivity( ACT_WALK_HURT );
			else if ( m_MonsterState == MONSTERSTATE_COMBAT ) sequence = LookupSequence( "combat_walk_primary" );
			break;
		case ACT_RUN:
			if ( 0.2f * pev->max_health >= pev->health ) sequence = LookupActivity( ACT_RUN_HURT );
			else if ( m_MonsterState == MONSTERSTATE_COMBAT ) sequence = LookupSequence( "combat_run_primary" );
			break;
		case ACT_STAND: sequence = LookupSequence( "crouching_2_stand" ); break;
		case ACT_DIESIMPLE:
		case ACT_DIEBACKWARD:
		case ACT_DIEFORWARD:
		case ACT_DIEVIOLENT:
		case ACT_DIE_HEADSHOT:
		case ACT_DIE_CHESTSHOT:
		case ACT_DIE_GUTSHOT:
		case ACT_DIE_BACKSHOT:
			if ( m_deathAnim ) sequence = LookupSequence( STRING( m_deathAnim ) );
			else if ( stealthDeath )
			{
				sequence = LookupSequence( "death02" );
				stealthDeath = FALSE;
			}
			break;
		case ACT_TERRORIST_CORNER_FIRE1: sequence = LookupSequence( "pistol_popout_right01" ); break;
		case ACT_TERRORIST_CORNER_FIRE2: sequence = LookupSequence( "pistol_popout_right02" ); break;
		case ACT_TERRORIST_CHOKING: sequence = LookupSequence( "choking" ); break;
		case ACT_TERRORIST_LOOKAROUND: sequence = LookupSequence( "patrol_idle" ); break;
		case ACT_TERRORIST_FLASHED: sequence = LookupSequence( "stunned01" ); break;
		default: break;
		}
		if ( sequence == -1 )
			sequence = LookupActivity( activity );
	}
	m_Activity = activity;
	if ( sequence < 0 )
	{
		ALERT( at_console, "%s has no sequence for act:%d\n", STRING( pev->classname ), activity );
		pev->sequence = 0;
	}
	else
	{
		if ( sequence != pev->sequence || m_fSequenceLoops )
			pev->frame = 0;
		pev->sequence = sequence;
		ResetSequenceInfo();
		SetYawSpeed();
	}
}

void CTerrorist::HandleAnimEvent( MonsterEvent_t* event )
{
	switch ( event->event )
	{
	case HGRUNT_AE_RELOAD:
		switch ( m_terroristBehavior )
		{
		case BEHAVIOR_ASSAULTRIFLE:
		case BEHAVIOR_SNIPERRIFLE:
		case BEHAVIOR_MP5: EMIT_SOUND( edict(), CHAN_WEAPON, "weapons/reload_mp5.wav", 0.75f, ATTN_NORM ); break;
		case BEHAVIOR_PISTOL:
		case BEHAVIOR_SMG: EMIT_SOUND( edict(), CHAN_WEAPON, "weapons/pistol_reload.wav", 0.75f, ATTN_NORM ); break;
		case BEHAVIOR_SHOTGUN: EMIT_SOUND( edict(), CHAN_WEAPON, "weapons/reload_shotgun.wav", 0.75f, ATTN_NORM ); break;
		default: break;
		}
		ClearConditions( bits_COND_NO_AMMO_LOADED );
		m_cAmmoLoaded = m_cClipSize;
		return;
	case HGRUNT_AE_KICK:
		{
			CBaseEntity* target = Kick();
			if ( target )
			{
				int damage = g_iSkillLevel == SKILL_EASY ? 7 : (g_iSkillLevel == SKILL_HARD ? 17 : 12);
				if ( m_terroristBehavior == BEHAVIOR_MELEE )
					damage = g_iSkillLevel == SKILL_EASY ? 15 : (g_iSkillLevel == SKILL_HARD ? 35 : 25);
				else
					EMIT_SOUND( edict(), CHAN_WEAPON, "weapons/knife_slash1.wav", 0.75f, ATTN_NORM );
				UTIL_MakeVectors( pev->angles );
				target->pev->punchangle.x = 5;
				target->pev->velocity = gpGlobals->v_forward * 100 + target->pev->velocity + gpGlobals->v_up * 50;
				target->TakeDamage( pev, pev, (float)damage, DMG_CLUB );
			}
			return;
		}
	case HGRUNT_AE_BURST1:
	case HGRUNT_AE_BURST2:
	case HGRUNT_AE_BURST3:
		justShotFlag = TRUE;
		switch ( m_terroristBehavior )
		{
		case BEHAVIOR_PISTOL:
		case BEHAVIOR_GRENADER: ShootPistol(); break;
		case BEHAVIOR_SHOTGUN: ShootShotgun(); break;
		case BEHAVIOR_MACHINEGUN: ShootMachineGun(); break;
		case BEHAVIOR_SNIPERRIFLE: ShootSniperRifle(); break;
		case BEHAVIOR_LAW: ShootLAW(); break;
		case BEHAVIOR_SMG: ShootSMG(); break;
		case BEHAVIOR_MP5: ShootMP5(); break;
		default: ShootAssaultRifle(); break;
		}
		CSoundEnt::InsertSound( bits_SOUND_COMBAT, pev->origin, 384, 0.3f );
		return;
	case HGRUNT_AE_GREN_TOSS:
		{
			UTIL_MakeVectors( pev->angles );
			bool flash = m_flashbangOnly || (!m_heGrenadeOnly && RANDOM_LONG( 0, 99 ) <= 40);
			Vector velocity = m_vecTossVelocity;
			int classification = flash ? m_classtype : 0;
			Vector origin = GetGunPosition();
			CGrenade::ShootTimed( pev, origin, velocity, 3.5f, flash ? 0.0f : 85.0f, classification );
			m_fThrowGrenade = FALSE;
			m_flNextGrenadeCheck = gpGlobals->time + 6;
			return;
		}
	case HGRUNT_AE_GREN_LAUNCH:
		{
			EMIT_SOUND( edict(), CHAN_WEAPON, "weapons/glauncher.wav", 0.8f, ATTN_NORM );
			Vector velocity = m_vecTossVelocity;
			Vector origin = GetGunPosition();
			CGrenade::ShootContact( pev, origin, velocity );
			m_fThrowGrenade = FALSE;
			if ( g_iSkillLevel == SKILL_HARD )
			{
				float time = gpGlobals->time;
				m_flNextGrenadeCheck = RANDOM_FLOAT( 2, 5 ) + time;
			}
			else
				m_flNextGrenadeCheck = gpGlobals->time + 6;
			return;
		}
	case HGRUNT_AE_GREN_DROP:
		{
			UTIL_MakeVectors( pev->angles );
			Vector origin = gpGlobals->v_forward * 17 + pev->origin - gpGlobals->v_right * 27 + gpGlobals->v_up * 6;
			CGrenade::ShootTimed( pev, origin, g_vecZero, 3, 35, 0 );
			return;
		}
	case HGRUNT_AE_CAUGHT_ENEMY:
		if ( CTalkMonster::g_talkWaitTime < gpGlobals->time && FOkToSpeak() )
		{
			SayNamedSentence( "T_ENEMY", 0.8f, ATTN_NORM, 0, m_voicePitch );
			JustSpoke( -1 );
		}
		break;
	case HGRUNT_AE_DROP_GUN:
		{
			Vector origin, angles;
			GetAttachment( 0, origin, angles );
			SetBodygroup( 2, 9 );
			if ( m_dropItem && m_dropChance > RANDOM_FLOAT( 0, 1 ) )
				DropItem( (char*)STRING( m_dropItem ), origin, angles );
			return;
		}
	case 13: return;
	case 14: LaserGlowOff(); return;
	default: break;
	}
	CBaseMonster::HandleAnimEvent( event );
}

void CTerrorist::SetupBehaviors( int type )
{
	int tier = (int)CVAR_GET_FLOAT( "maptier" );
	int head;
	switch ( type )
	{
	case BEHAVIOR_SHOTGUN:
		scheduleTable[SCHED_TAKE_COVER_FROM_ENEMY].percentChance = 0;
		scheduleTable[SCHED_SMALL_FLINCH].percentChance = 0.9f;
		scheduleTable[SCHED_TAKE_COVER_LIGHT_DAMAGE].percentChance = 0;
		scheduleTable[SCHED_TAKE_COVER_HEAVY_DAMAGE].percentChance = 0;
		scheduleTable[SCHED_TERRORIST_COVER_AND_RELOAD].percentChance = 1;
		scheduleTable[SCHED_TERRORIST_ESTABLISH_LINE_OF_FIRE].percentChance = 0;
		scheduleTable[SCHED_TERRORIST_CLOSEIN].percentChance = 0.4f;
		scheduleTable[SCHED_RANGE_ATTACK1].percentChance = 0.9f;
		scheduleTable[SCHED_RANGE_ATTACK2].percentChance = 0;
		scheduleTable[SCHED_MELEE_ATTACK1].percentChance = 0.3f;
		scheduleTable[SCHED_TERRORIST_SNIPER_ATTACK].percentChance = 0;
		scheduleTable[SCHED_TERRORIST_SNIPER_TAKESHOT].percentChance = 0;
		scheduleTable[SCHED_TERRORIST_SPECIAL_COVER_NODE].percentChance = 0.2f;
		scheduleTable[SCHED_STANDOFF].percentChance = 0.1f;
		scheduleTable[SCHED_TERRORIST_FIRE_PAUSE].percentChance = 0.4f;
		m_fPreferedRange = 128;
		head = tier > 2 ? EASY_HEAD : MEDIUM_HEAD;
		break;
	case BEHAVIOR_ASSAULTRIFLE:
		scheduleTable[SCHED_TAKE_COVER_FROM_ENEMY].percentChance = 0;
		scheduleTable[SCHED_SMALL_FLINCH].percentChance = 0.9f;
		scheduleTable[SCHED_TAKE_COVER_LIGHT_DAMAGE].percentChance = 0;
		scheduleTable[SCHED_TAKE_COVER_HEAVY_DAMAGE].percentChance = 0;
		scheduleTable[SCHED_TERRORIST_COVER_AND_RELOAD].percentChance = 1;
		scheduleTable[SCHED_TERRORIST_ESTABLISH_LINE_OF_FIRE].percentChance = 0.8f;
		scheduleTable[SCHED_TERRORIST_CLOSEIN].percentChance = 0.05f;
		scheduleTable[SCHED_RANGE_ATTACK1].percentChance = 0.9f;
		scheduleTable[SCHED_RANGE_ATTACK2].percentChance = 0;
		scheduleTable[SCHED_MELEE_ATTACK1].percentChance = 0.2f;
		scheduleTable[SCHED_TERRORIST_SNIPER_ATTACK].percentChance = 0;
		scheduleTable[SCHED_TERRORIST_SNIPER_TAKESHOT].percentChance = 0;
		scheduleTable[SCHED_TERRORIST_SPECIAL_COVER_NODE].percentChance = 0.3f;
		scheduleTable[SCHED_STANDOFF].percentChance = 0.1f;
		scheduleTable[SCHED_TERRORIST_FIRE_PAUSE].percentChance = 0.6f;
		m_fPreferedRange = 256;
		head = tier <= 3 ? HARD_HEAD : (tier == 4 ? MEDIUM_HEAD : EASY_HEAD);
		break;
	case BEHAVIOR_MACHINEGUN:
		scheduleTable[SCHED_TAKE_COVER_FROM_ENEMY].percentChance = 0;
		scheduleTable[SCHED_SMALL_FLINCH].percentChance = 0.9f;
		scheduleTable[SCHED_TAKE_COVER_LIGHT_DAMAGE].percentChance = 0;
		scheduleTable[SCHED_TAKE_COVER_HEAVY_DAMAGE].percentChance = 0;
		scheduleTable[SCHED_TERRORIST_COVER_AND_RELOAD].percentChance = 0;
		scheduleTable[SCHED_TERRORIST_ESTABLISH_LINE_OF_FIRE].percentChance = 0.5f;
		scheduleTable[SCHED_TERRORIST_CLOSEIN].percentChance = 0;
		scheduleTable[SCHED_RANGE_ATTACK1].percentChance = 0.95f;
		scheduleTable[SCHED_RANGE_ATTACK2].percentChance = 0;
		scheduleTable[SCHED_MELEE_ATTACK1].percentChance = 0;
		scheduleTable[SCHED_TERRORIST_SNIPER_ATTACK].percentChance = 0;
		scheduleTable[SCHED_TERRORIST_SNIPER_TAKESHOT].percentChance = 0;
		scheduleTable[SCHED_TERRORIST_SPECIAL_COVER_NODE].percentChance = 0.1f;
		scheduleTable[SCHED_STANDOFF].percentChance = 0.1f;
		scheduleTable[SCHED_TERRORIST_FIRE_PAUSE].percentChance = 0.1f;
		head = tier <= 4 ? HARD_HEAD : (tier == 5 ? MEDIUM_HEAD : EASY_HEAD);
		break;
	case BEHAVIOR_SMG:
		scheduleTable[SCHED_TAKE_COVER_FROM_ENEMY].percentChance = 0;
		scheduleTable[SCHED_SMALL_FLINCH].percentChance = 0.9f;
		scheduleTable[SCHED_TAKE_COVER_LIGHT_DAMAGE].percentChance = 0;
		scheduleTable[SCHED_TAKE_COVER_HEAVY_DAMAGE].percentChance = 0;
		scheduleTable[SCHED_TERRORIST_COVER_AND_RELOAD].percentChance = 1;
		scheduleTable[SCHED_TERRORIST_ESTABLISH_LINE_OF_FIRE].percentChance = 0.8f;
		scheduleTable[SCHED_TERRORIST_CLOSEIN].percentChance = 0.1f;
		scheduleTable[SCHED_RANGE_ATTACK1].percentChance = 0.9f;
		scheduleTable[SCHED_RANGE_ATTACK2].percentChance = 0;
		scheduleTable[SCHED_MELEE_ATTACK1].percentChance = 0.1f;
		scheduleTable[SCHED_TERRORIST_SNIPER_ATTACK].percentChance = 0;
		scheduleTable[SCHED_TERRORIST_SNIPER_TAKESHOT].percentChance = 0;
		scheduleTable[SCHED_TERRORIST_SPECIAL_COVER_NODE].percentChance = 0.3f;
		scheduleTable[SCHED_STANDOFF].percentChance = 0.1f;
		scheduleTable[SCHED_TERRORIST_FIRE_PAUSE].percentChance = 0.1f;
		m_fPreferedRange = 192;
		head = tier <= 2 ? HARD_HEAD : (tier == 3 ? MEDIUM_HEAD : EASY_HEAD);
		break;
	case BEHAVIOR_MP5:
		scheduleTable[SCHED_TAKE_COVER_FROM_ENEMY].percentChance = 0;
		scheduleTable[SCHED_SMALL_FLINCH].percentChance = 0.9f;
		scheduleTable[SCHED_TAKE_COVER_LIGHT_DAMAGE].percentChance = 0;
		scheduleTable[SCHED_TAKE_COVER_HEAVY_DAMAGE].percentChance = 0;
		scheduleTable[SCHED_TERRORIST_COVER_AND_RELOAD].percentChance = 1;
		scheduleTable[SCHED_TERRORIST_ESTABLISH_LINE_OF_FIRE].percentChance = 0.8f;
		scheduleTable[SCHED_TERRORIST_CLOSEIN].percentChance = 0.1f;
		scheduleTable[SCHED_RANGE_ATTACK1].percentChance = 0.9f;
		scheduleTable[SCHED_RANGE_ATTACK2].percentChance = 0;
		scheduleTable[SCHED_MELEE_ATTACK1].percentChance = 0.1f;
		scheduleTable[SCHED_TERRORIST_SNIPER_ATTACK].percentChance = 0;
		scheduleTable[SCHED_TERRORIST_SNIPER_TAKESHOT].percentChance = 0;
		scheduleTable[SCHED_TERRORIST_SPECIAL_COVER_NODE].percentChance = 0.4f;
		scheduleTable[SCHED_STANDOFF].percentChance = 0.1f;
		scheduleTable[SCHED_TERRORIST_FIRE_PAUSE].percentChance = 0.6f;
		m_fPreferedRange = 256;
		head = tier <= 2 ? HARD_HEAD : (tier == 3 ? MEDIUM_HEAD : EASY_HEAD);
		break;
	case BEHAVIOR_PISTOL:
		scheduleTable[SCHED_TAKE_COVER_FROM_ENEMY].percentChance = 0;
		scheduleTable[SCHED_SMALL_FLINCH].percentChance = 0.8f;
		scheduleTable[SCHED_TAKE_COVER_LIGHT_DAMAGE].percentChance = 0;
		scheduleTable[SCHED_TAKE_COVER_HEAVY_DAMAGE].percentChance = 0;
		scheduleTable[SCHED_TERRORIST_COVER_AND_RELOAD].percentChance = 1;
		scheduleTable[SCHED_TERRORIST_ESTABLISH_LINE_OF_FIRE].percentChance = 0.9f;
		scheduleTable[SCHED_TERRORIST_CLOSEIN].percentChance = 0.1f;
		scheduleTable[SCHED_RANGE_ATTACK1].percentChance = 1;
		scheduleTable[SCHED_RANGE_ATTACK2].percentChance = 0;
		scheduleTable[SCHED_MELEE_ATTACK1].percentChance = 0.2f;
		scheduleTable[SCHED_TERRORIST_SNIPER_ATTACK].percentChance = 0;
		scheduleTable[SCHED_TERRORIST_SNIPER_TAKESHOT].percentChance = 0;
		scheduleTable[SCHED_TERRORIST_SPECIAL_COVER_NODE].percentChance = 0.5f;
		scheduleTable[SCHED_STANDOFF].percentChance = 0.1f;
		scheduleTable[SCHED_TERRORIST_FIRE_PAUSE].percentChance = 0.4f;
		m_fPreferedRange = 192;
		head = EASY_HEAD;
		break;
	case BEHAVIOR_MELEE:
		scheduleTable[SCHED_TAKE_COVER_FROM_ENEMY].percentChance = 0;
		scheduleTable[SCHED_SMALL_FLINCH].percentChance = 1;
		scheduleTable[SCHED_TAKE_COVER_LIGHT_DAMAGE].percentChance = 0;
		scheduleTable[SCHED_TAKE_COVER_HEAVY_DAMAGE].percentChance = 0;
		scheduleTable[SCHED_TERRORIST_COVER_AND_RELOAD].percentChance = 0;
		scheduleTable[SCHED_TERRORIST_ESTABLISH_LINE_OF_FIRE].percentChance = 0;
		scheduleTable[SCHED_TERRORIST_CLOSEIN].percentChance = 0.8f;
		scheduleTable[SCHED_RANGE_ATTACK1].percentChance = 0;
		scheduleTable[SCHED_RANGE_ATTACK2].percentChance = 0;
		scheduleTable[SCHED_MELEE_ATTACK1].percentChance = 1;
		scheduleTable[SCHED_TERRORIST_SNIPER_ATTACK].percentChance = 0;
		scheduleTable[SCHED_TERRORIST_SNIPER_TAKESHOT].percentChance = 0;
		scheduleTable[SCHED_TERRORIST_SPECIAL_COVER_NODE].percentChance = 0;
		scheduleTable[SCHED_STANDOFF].percentChance = 0.3f;
		scheduleTable[SCHED_TERRORIST_FIRE_PAUSE].percentChance = 0;
		head = EASY_HEAD;
		break;
	case BEHAVIOR_GRENADER:
		scheduleTable[SCHED_TAKE_COVER_FROM_ENEMY].percentChance = 0.1f;
		scheduleTable[SCHED_SMALL_FLINCH].percentChance = 0.8f;
		scheduleTable[SCHED_TAKE_COVER_LIGHT_DAMAGE].percentChance = 0.1f;
		scheduleTable[SCHED_TAKE_COVER_HEAVY_DAMAGE].percentChance = 0.2f;
		scheduleTable[SCHED_TERRORIST_COVER_AND_RELOAD].percentChance = 1;
		scheduleTable[SCHED_TERRORIST_ESTABLISH_LINE_OF_FIRE].percentChance = 0;
		scheduleTable[SCHED_TERRORIST_CLOSEIN].percentChance = 0;
		scheduleTable[SCHED_RANGE_ATTACK1].percentChance = 0.1f;
		scheduleTable[SCHED_RANGE_ATTACK2].percentChance = 1;
		scheduleTable[SCHED_MELEE_ATTACK1].percentChance = 0;
		scheduleTable[SCHED_TERRORIST_SNIPER_ATTACK].percentChance = 0;
		scheduleTable[SCHED_TERRORIST_SNIPER_TAKESHOT].percentChance = 0;
		scheduleTable[SCHED_TERRORIST_SPECIAL_COVER_NODE].percentChance = 0;
		scheduleTable[SCHED_STANDOFF].percentChance = 0.2f;
		scheduleTable[SCHED_TERRORIST_FIRE_PAUSE].percentChance = 0.5f;
		head = HARD_HEAD;
		break;
	case BEHAVIOR_LAW:
		scheduleTable[SCHED_TAKE_COVER_FROM_ENEMY].percentChance = 0.1f;
		scheduleTable[SCHED_SMALL_FLINCH].percentChance = 0.9f;
		scheduleTable[SCHED_TAKE_COVER_LIGHT_DAMAGE].percentChance = 0;
		scheduleTable[SCHED_TAKE_COVER_HEAVY_DAMAGE].percentChance = 0;
		scheduleTable[SCHED_TERRORIST_COVER_AND_RELOAD].percentChance = 1;
		scheduleTable[SCHED_TERRORIST_ESTABLISH_LINE_OF_FIRE].percentChance = 0.9f;
		scheduleTable[SCHED_TERRORIST_CLOSEIN].percentChance = 0;
		scheduleTable[SCHED_RANGE_ATTACK1].percentChance = 1;
		scheduleTable[SCHED_RANGE_ATTACK2].percentChance = 0;
		scheduleTable[SCHED_MELEE_ATTACK1].percentChance = 0;
		scheduleTable[SCHED_TERRORIST_SNIPER_ATTACK].percentChance = 0;
		scheduleTable[SCHED_TERRORIST_SNIPER_TAKESHOT].percentChance = 0;
		scheduleTable[SCHED_TERRORIST_SPECIAL_COVER_NODE].percentChance = 0;
		scheduleTable[SCHED_STANDOFF].percentChance = 0.1f;
		scheduleTable[SCHED_TERRORIST_FIRE_PAUSE].percentChance = 0;
		head = HARD_HEAD;
		break;
	case BEHAVIOR_SNIPERRIFLE:
		scheduleTable[SCHED_TAKE_COVER_FROM_ENEMY].percentChance = 0;
		scheduleTable[SCHED_SMALL_FLINCH].percentChance = 1;
		scheduleTable[SCHED_TAKE_COVER_LIGHT_DAMAGE].percentChance = 0.1f;
		scheduleTable[SCHED_TAKE_COVER_HEAVY_DAMAGE].percentChance = 0.2f;
		scheduleTable[SCHED_TERRORIST_COVER_AND_RELOAD].percentChance = 0;
		scheduleTable[SCHED_TERRORIST_ESTABLISH_LINE_OF_FIRE].percentChance = 0.5f;
		scheduleTable[SCHED_TERRORIST_CLOSEIN].percentChance = 0;
		scheduleTable[SCHED_RANGE_ATTACK1].percentChance = 0;
		scheduleTable[SCHED_RANGE_ATTACK2].percentChance = 0;
		scheduleTable[SCHED_MELEE_ATTACK1].percentChance = 0.2f;
		scheduleTable[SCHED_TERRORIST_SNIPER_ATTACK].percentChance = 1;
		scheduleTable[SCHED_TERRORIST_SNIPER_TAKESHOT].percentChance = 0.5f;
		scheduleTable[SCHED_TERRORIST_SPECIAL_COVER_NODE].percentChance = 0;
		scheduleTable[SCHED_STANDOFF].percentChance = 0;
		scheduleTable[SCHED_TERRORIST_FIRE_PAUSE].percentChance = 0;
		head = HARD_HEAD;
		break;
	case BEHAVIOR_SUICIDAL:
		scheduleTable[SCHED_TAKE_COVER_FROM_ENEMY].percentChance = 0;
		scheduleTable[SCHED_SMALL_FLINCH].percentChance = 0;
		scheduleTable[SCHED_TAKE_COVER_LIGHT_DAMAGE].percentChance = 0;
		scheduleTable[SCHED_TAKE_COVER_HEAVY_DAMAGE].percentChance = 0;
		scheduleTable[SCHED_TERRORIST_COVER_AND_RELOAD].percentChance = 0;
		scheduleTable[SCHED_TERRORIST_ESTABLISH_LINE_OF_FIRE].percentChance = 0.1f;
		scheduleTable[SCHED_TERRORIST_CLOSEIN].percentChance = 1;
		scheduleTable[SCHED_RANGE_ATTACK1].percentChance = 0;
		scheduleTable[SCHED_RANGE_ATTACK2].percentChance = 0;
		scheduleTable[SCHED_MELEE_ATTACK1].percentChance = 0;
		scheduleTable[SCHED_TERRORIST_SNIPER_ATTACK].percentChance = 0;
		scheduleTable[SCHED_TERRORIST_SNIPER_TAKESHOT].percentChance = 0;
		scheduleTable[SCHED_TERRORIST_SPECIAL_COVER_NODE].percentChance = 0;
		scheduleTable[SCHED_KAMAKAZI_BLOWUP].percentChance = 1;
		scheduleTable[SCHED_STANDOFF].percentChance = 0;
		scheduleTable[SCHED_TERRORIST_FIRE_PAUSE].percentChance = 0;
		head = HARD_HEAD;
		break;
	default: return;
	}
	SetHead( head );
}

Schedule_t* CTerrorist::ScheduleFromName( const char* name )
{
	Schedule_t* schedule = ScheduleInList( name, m_scheduleList, 39 );
	return schedule ? schedule : CBaseMonster::ScheduleFromName( name );
}

Schedule_t* CTerrorist::GetSchedule()
{
	m_iSentence = -1;
	float enemyDistance = m_hEnemy ? ( pev->origin - m_hEnemy->pev->origin ).Length2D() : 0;
	if ( m_terroristBehavior == BEHAVIOR_SNIPERRIFLE )
		LaserGlowOff();
	ClearScheduleTableFlags();
	ActivateTableEntry( GetForcedTargetSchedule() );
	ActivateTableEntry( GetWeaponEffectSchedule() );
	ActivateTableEntry( GetBehaviorTypeSchedule() );
	ActivateTableEntry( GetFlyingSchedule() );
	ActivateTableEntry( GetHearingSchedule() );

	if ( m_MonsterState == MONSTERSTATE_IDLE )
	{
		BOOL followPath = FALSE;
		if ( m_alertClearTime != 0 && gpGlobals->time > m_alertClearTime && m_alertable )
		{
			SayNamedSentence( "T_RELAX", 0.8f, ATTN_NORM, 0, m_voicePitch );
			--m_alertLevel;
			m_iSentence = -1;
			m_alertClearTime = 0;
			CTalkMonster::g_talkWaitTime = gpGlobals->time + 3;
			followPath = m_pGoalEnt && !m_cantMove && FClassnameIs( m_pGoalEnt->pev, "path_corner" );
		}
		else if ( m_pGoalEnt && !m_cantMove && FClassnameIs( m_pGoalEnt->pev, "path_corner" ) )
			followPath = !m_alertable || gpGlobals->time > m_alertClearTime;
		if ( followPath )
		{
			MakeIdealYaw( m_pGoalEnt->pev->origin );
			m_movementGoal = MOVEGOAL_PATHCORNER;
			if ( !FRefreshRoute() )
				ALERT( at_aiconsole, "Can't Create Route!\n" );
			return GetScheduleOfType( SCHED_IDLE_WALK );
		}
		if ( !m_hEnemy && gpGlobals->time > m_wanderTime && m_canWander && !m_cantMove )
		{
			float range = RANDOM_FLOAT( 64, 256 );
			m_vecMoveGoal.x += (float)RANDOM_LONG( (int)-range, (int)range );
			m_vecMoveGoal.y += (float)RANDOM_LONG( (int)-range, (int)range );
			m_vecMoveGoal.z = pev->origin.z;
			m_wanderOrigin = m_vecMoveGoal;
			float time = gpGlobals->time;
			m_wanderTime = time + RANDOM_FLOAT( 8, 15 );
			return GetScheduleOfType( SCHED_WANDER );
		}
	}
	if ( m_MonsterState == MONSTERSTATE_ALERT )
	{
		if ( m_hEnemy && ( pev->origin - m_hEnemy->pev->origin ).Length2D() > m_alertRadius && m_alertable )
		{
			if ( m_alertLevel <= 1 )
			{
				MakeIdealYaw( m_hEnemy->pev->origin );
				m_hEnemy = NULL;
				SetState( MONSTERSTATE_IDLE );
				SayNamedSentence( "T_ALERT", 0.8f, ATTN_NORM, 0, m_voicePitch );
				m_iSentence = -1;
				CTalkMonster::g_talkWaitTime = gpGlobals->time + 2;
				float duration = g_iSkillLevel == SKILL_MEDIUM ? 3.5f :
					( g_iSkillLevel == 0 || g_iSkillLevel == SKILL_HARD ? 2.0f : 5.0f );
				m_alertClearTime = gpGlobals->time + duration;
				return GetScheduleOfType( SCHED_TERRORIST_ALERT_SPEAK );
			}
		}
		else if ( m_hEnemy && m_alertRadius >= enemyDistance && m_alertable )
			m_alertLevel = 2;
	}
	if ( m_MonsterState == MONSTERSTATE_COMBAT )
	{
		if ( m_cantMove )
			scheduleTable[SCHED_TERRORIST_WAIT_FACE_ENEMY].activeFlag = 1;
		if ( HasConditions( bits_COND_ENEMY_DEAD ) )
			return CBaseMonster::GetSchedule();
		if ( HasConditions( bits_COND_NO_AMMO_LOADED ) )
		{
			scheduleTable[SCHED_TERRORIST_COVER_AND_RELOAD].activeFlag = 1;
			int type = GetScheduleFromTable();
			HandleSpeaking( type );
			if ( type != -1 )
			{
				Schedule_t* schedule = GetScheduleOfType( type );
				ALERT( at_aiconsole, "%s Playing Schedule: %s\n", STRING( pev->classname ), schedule->pName );
				return schedule;
			}
		}
		if ( HasConditions( bits_COND_NEW_ENEMY ) )
		{
			if ( m_alertable || ( ( m_terroristBehavior == BEHAVIOR_MELEE || m_terroristBehavior == BEHAVIOR_SUICIDAL ) &&
				CTalkMonster::g_talkWaitTime < gpGlobals->time && !FOkToSpeak() && m_alertable ) )
			{
				if ( CTalkMonster::g_talkWaitTime < gpGlobals->time && FOkToSpeak() )
				{
					SayNamedSentence( "T_ENEMY", 0.8f, ATTN_NORM, 0, m_voicePitch );
					JustSpoke( 3 );
				}
			}
			if ( !m_cantMove )
			{
				scheduleTable[SCHED_TERRORIST_ESTABLISH_LINE_OF_FIRE].activeFlag = 1;
				if ( enemyDistance > m_fPreferedRange )
					scheduleTable[SCHED_TERRORIST_CLOSEIN].activeFlag = 1;
				scheduleTable[SCHED_TAKE_COVER_FROM_ENEMY].activeFlag = 1;
			}
		}
		if ( HasConditions( bits_COND_LIGHT_DAMAGE ) )
		{
			scheduleTable[SCHED_SMALL_FLINCH].activeFlag = 1;
			scheduleTable[SCHED_TAKE_COVER_LIGHT_DAMAGE].activeFlag = 1;
		}
		if ( HasConditions( bits_COND_HEAVY_DAMAGE ) )
		{
			scheduleTable[SCHED_SMALL_FLINCH].activeFlag = 1;
			scheduleTable[SCHED_TAKE_COVER_HEAVY_DAMAGE].activeFlag = 1;
		}
		if ( HasConditions( bits_COND_CAN_MELEE_ATTACK1 ) )
			scheduleTable[SCHED_MELEE_ATTACK1].activeFlag = 1;
		if ( HasConditions( bits_COND_CAN_RANGE_ATTACK1 ) )
		{
			if ( InSquad() && MySquadLeader()->m_fEnemyEluded && !HasConditions( bits_COND_ENEMY_FACING_ME ) )
			{
				MySquadLeader()->m_fEnemyEluded = FALSE;
				return GetScheduleOfType( SCHED_TERRORIST_FOUND_ENEMY );
			}
			if ( justShotFlag )
				scheduleTable[SCHED_TERRORIST_FIRE_PAUSE].activeFlag = 1;
			justShotFlag = FALSE;
			scheduleTable[SCHED_TERRORIST_SNIPER_ATTACK].activeFlag = 1;
			if ( !m_cantMove )
			{
				scheduleTable[SCHED_TAKE_COVER_FROM_ENEMY].activeFlag = 1;
				if ( enemyDistance > m_fPreferedRange )
					scheduleTable[SCHED_TERRORIST_CLOSEIN].activeFlag = 1;
			}
			if ( m_terroristBehavior == BEHAVIOR_LAW )
			{
				if ( gpGlobals->time >= lawDelayTime )
					scheduleTable[SCHED_RANGE_ATTACK1].activeFlag = 1;
			}
			else if ( m_terroristBehavior != BEHAVIOR_GRENADER || enemyDistance < 128 )
				scheduleTable[SCHED_RANGE_ATTACK1].activeFlag = 1;
		}
		if ( HasConditions( bits_COND_CAN_RANGE_ATTACK2 ) )
			scheduleTable[SCHED_RANGE_ATTACK2].activeFlag = 1;
		if ( HasConditions( bits_COND_ENEMY_OCCLUDED ) )
		{
			if ( m_hEnemy->IsPlayer() )
				shotPlayerRecently = FALSE;
			scheduleTable[SCHED_TERRORIST_SNIPER_TAKESHOT].activeFlag = 1;
			if ( HasConditions( bits_COND_CAN_RANGE_ATTACK2 ) )
				scheduleTable[SCHED_RANGE_ATTACK2].activeFlag = 1;
			if ( !m_cantMove )
			{
				scheduleTable[SCHED_TERRORIST_ESTABLISH_LINE_OF_FIRE].activeFlag = 1;
				scheduleTable[SCHED_TERRORIST_CLOSEIN].activeFlag = 1;
			}
			scheduleTable[SCHED_STANDOFF].activeFlag = 1;
		}
		if ( !HasConditions( bits_COND_CAN_RANGE_ATTACK1 ) &&
			( !HasConditions( bits_COND_CAN_MELEE_ATTACK1 ) || scheduleTable[SCHED_MELEE_ATTACK1].percentChance == 0 ) && !m_cantMove )
		{
			scheduleTable[SCHED_TERRORIST_ESTABLISH_LINE_OF_FIRE].activeFlag = 1;
			if ( enemyDistance > m_fPreferedRange )
				scheduleTable[SCHED_TERRORIST_CLOSEIN].activeFlag = 1;
		}
		m_pSpecialCoverNode = HasValidSpecialCoverNode();
		if ( m_pSpecialCoverNode && enemyDistance > ( pev->origin - m_pSpecialCoverNode->m_vecOrigin ).Length2D() * 0.75f && !m_cantMove )
			scheduleTable[SCHED_TERRORIST_SPECIAL_COVER_NODE].activeFlag = 1;
	}
	int type = GetScheduleFromTable();
	HandleSpeaking( type );
	if ( type == -1 )
		return CBaseMonster::GetSchedule();
	Schedule_t* schedule = GetScheduleOfType( type );
	ALERT( at_aiconsole, "%s Playing Schedule: %s\n", STRING( pev->classname ), schedule->pName );
	return schedule;
}

Schedule_t* CTerrorist::GetScheduleOfType( int type )
{
	switch ( type )
	{
	case SCHED_COMBAT_FACE: return slTerroristCombatFace;
	case SCHED_VICTORY_DANCE:
		return InSquad() && MySquadLeader() != this ? slTerroristFail : slTerroristVictoryDance;
	case SCHED_TAKE_COVER_FROM_ENEMY:
	case SCHED_TAKE_COVER_LIGHT_DAMAGE:
	case SCHED_TAKE_COVER_HEAVY_DAMAGE: return slTerroristTakeCover;
	case SCHED_TAKE_COVER_FROM_BEST_SOUND: return slTerroristTakeCoverFromBestSound;
	case SCHED_MELEE_ATTACK1:
		taskFailCount = 0;
		return CSquadMonster::GetScheduleOfType( type );
	case SCHED_RANGE_ATTACK1:
		taskFailCount = 0;
		if ( m_terroristBehavior == BEHAVIOR_PISTOL || m_terroristBehavior == BEHAVIOR_LAW )
			return slTerroristRangeAttack1C;
		if ( RANDOM_LONG( 0, 99 ) <= 49 )
		{
			if ( m_terroristBehavior != BEHAVIOR_MACHINEGUN )
				return slTerroristRangeAttack1C;
		}
		else if ( m_terroristBehavior != BEHAVIOR_MACHINEGUN )
		{
			if ( RANDOM_LONG( 0, 9 ) <= 4 )
				m_bStanding = RANDOM_LONG( 0, 1 );
			return m_bStanding ? slTerroristRangeAttack1B : slTerroristRangeAttack1A;
		}
		m_bStanding = TRUE;
		return slTerroristRangeAttackMG2;
	case SCHED_RANGE_ATTACK2: return slTerroristRangeAttack2;
	case SCHED_FAIL: return m_hEnemy ? slTerroristCombatFail : slTerroristFail;
	case SCHED_WANDER: return slTerroristWander;
	case SCHED_TERRORIST_SUPPRESS:
		if ( m_hEnemy->IsPlayer() && m_fFirstEncounter )
		{
			m_fFirstEncounter = FALSE;
			return slTerroristSignalSuppress;
		}
		return slTerroristSuppress;
	case SCHED_TERRORIST_ESTABLISH_LINE_OF_FIRE:
		if ( m_terroristBehavior == BEHAVIOR_SNIPERRIFLE )
			LaserGlowOff();
		return slTerroristEstablishLineOfFire;
	case SCHED_TERRORIST_COVER_AND_RELOAD: return slTerroristHideReload;
	case SCHED_TERRORIST_SWEEP: return slTerroristSweep;
	case SCHED_TERRORIST_FOUND_ENEMY: return slTerroristFoundEnemy;
	case SCHED_TERRORIST_REPEL:
	case SCHED_TERRORIST_REPEL_ATTACK:
		if ( pev->velocity.z > -128 )
			pev->velocity.z -= 32;
		return type == SCHED_TERRORIST_REPEL ? slTerroristRepel : slTerroristRepelAttack;
	case SCHED_TERRORIST_REPEL_LAND: return slTerroristRepelLand;
	case SCHED_TERRORIST_WAIT_FACE_ENEMY: return slTerroristWaitInCover;
	case SCHED_TERRORIST_TAKECOVER_FAILED:
	case SCHED_TERRORIST_ELOF_FAIL:
		if ( type == SCHED_TERRORIST_ELOF_FAIL )
		{
			m_bStanding = TRUE;
			taskFailCount += 1;
		}
		if ( m_terroristBehavior == BEHAVIOR_SNIPERRIFLE )
		{
			if ( HasConditions( bits_COND_CAN_RANGE_ATTACK1 ) && scheduleTable[SCHED_TERRORIST_SNIPER_ATTACK].percentChance > 0 )
				return GetScheduleOfType( SCHED_TERRORIST_SNIPER_ATTACK );
		}
		else
		{
			if ( HasConditions( bits_COND_CAN_MELEE_ATTACK1 ) && scheduleTable[SCHED_MELEE_ATTACK1].percentChance > 0 )
				return GetScheduleOfType( SCHED_MELEE_ATTACK1 );
			if ( HasConditions( bits_COND_CAN_RANGE_ATTACK2 ) && scheduleTable[SCHED_RANGE_ATTACK2].percentChance > 0 )
				return GetScheduleOfType( SCHED_RANGE_ATTACK2 );
			if ( HasConditions( bits_COND_CAN_RANGE_ATTACK1 ) && scheduleTable[SCHED_RANGE_ATTACK1].percentChance > 0 && m_terroristBehavior != BEHAVIOR_LAW )
				return GetScheduleOfType( SCHED_RANGE_ATTACK1 );
			if ( type == SCHED_TERRORIST_TAKECOVER_FAILED )
			{
				if ( scheduleTable[SCHED_TERRORIST_ESTABLISH_LINE_OF_FIRE].percentChance > 0 )
					return GetScheduleOfType( SCHED_TERRORIST_ESTABLISH_LINE_OF_FIRE );
				if ( scheduleTable[SCHED_TERRORIST_CLOSEIN].percentChance > 0 )
					return GetScheduleOfType( SCHED_TERRORIST_CLOSEIN );
			}
		}
		if ( type == SCHED_TERRORIST_TAKECOVER_FAILED )
			return GetScheduleOfType( SCHED_TERRORIST_WAIT_FACE_ENEMY );
		if ( taskFailCount <= 1 )
			return GetScheduleOfType( SCHED_TAKE_COVER_FROM_ENEMY );
		if ( RANDOM_LONG( 0, 1 ) && CTalkMonster::g_talkWaitTime < gpGlobals->time && FOkToSpeak() )
		{
			SayNamedSentence( "T_TAUNT", 0.8f, ATTN_NORM, 0, m_voicePitch );
			JustSpoke( -1 );
		}
		return GetScheduleOfType( SCHED_TERRORIST_WAIT_CROUCHED );
	case SCHED_TERRORIST_IN_SMOKE: return slTerroristInSmoke;
	case SCHED_KAMAKAZI_BLOWUP: return slTerroristKamakaziBlowup;
	case SCHED_TERRORIST_SNIPER_ATTACK:
		taskFailCount = 0;
		m_bStanding = TRUE;
		return slTerroristSniperAttack;
	case SCHED_TERRORIST_SNIPER_TAKESHOT:
		if ( m_hEnemy )
		{
			LaserGlowOff();
			m_hEnemy = NULL;
		}
		return slTerroristSniperTakeShot;
	case SCHED_TERRORIST_SPECIAL_COVER_NODE:
		if ( m_pSpecialCoverNode->m_nodeType & 1 )
		{
			m_bStanding = TRUE;
			return slTerroristDuckCoverNode;
		}
		if ( m_pSpecialCoverNode->m_nodeType & 6 )
			m_bStanding = TRUE;
		return slTerroristCornerCoverNode;
	case SCHED_TERRORIST_DUCK_NODE_ATTACK: return slTerroristDuckNodeAttack;
	case SCHED_TERRORIST_CLOSEIN: return slTerroristCloseIn;
	case SCHED_TERRORIST_CORNER_NODE_ATTACK: return slTerroristCornerNodeAttack;
	case SCHED_TERRORIST_FIRE_PAUSE: return slTerroristFiringPause;
	case SCHED_TERRORIST_ALERT_SPEAK: return slTerroristAlertSpeak;
	case SCHED_TERRORIST_INVESTIGATE: return slTerroristInvestigate;
	case SCHED_TERRORIST_FLASHED: return slTerroristFlashed;
	case SCHED_TERRORIST_WAIT_CROUCHED: return slTerroristWaitCrouched;
	default: return CSquadMonster::GetScheduleOfType( type );
	}
}

class CTerroristArctic : public CTerrorist
{
public:
	void Precache() override;
	void SpawnInit() override;
};

void CTerroristArctic::Precache()
{
	PRECACHE_MODEL( pev->model ? (char*)STRING( pev->model ) : "models/arctic.mdl" );
	CTerrorist::Precache();
}

void CTerroristArctic::SpawnInit()
{
	SET_MODEL( edict(), pev->model ? STRING( pev->model ) : "models/arctic.mdl" );
	m_terroristType = TERRORIST_ARCTIC;
	if ( !m_language )
		m_language = ALLOC_STRING( "RU" );
}

class CTerroristDesert : public CTerrorist
{
public:
	void Precache() override;
	void SpawnInit() override;
};

void CTerroristDesert::Precache()
{
	PRECACHE_MODEL( pev->model ? (char*)STRING( pev->model ) : "models/leet.mdl" );
	CTerrorist::Precache();
}

void CTerroristDesert::SpawnInit()
{
	SET_MODEL( edict(), pev->model ? STRING( pev->model ) : "models/leet.mdl" );
	m_terroristType = TERRORIST_DESERT;
	if ( !m_language )
		m_language = ALLOC_STRING( "AR" );
}

class CTerroristUrban : public CTerrorist
{
public:
	void Precache() override;
	void SpawnInit() override;
};

void CTerroristUrban::Precache()
{
	PRECACHE_MODEL( pev->model ? (char*)STRING( pev->model ) : "models/militia.mdl" );
	CTerrorist::Precache();
}

void CTerroristUrban::SpawnInit()
{
	SET_MODEL( edict(), pev->model ? STRING( pev->model ) : "models/militia.mdl" );
	m_terroristType = TERRORIST_URBAN;
	if ( !m_language )
		m_language = ALLOC_STRING( "AM" );
}

class CTerroristJungle : public CTerrorist
{
public:
	void Precache() override;
	void SpawnInit() override;
};

void CTerroristJungle::Precache()
{
	PRECACHE_MODEL( pev->model ? (char*)STRING( pev->model ) : "models/guerilla.mdl" );
	CTerrorist::Precache();
}

void CTerroristJungle::SpawnInit()
{
	SET_MODEL( edict(), pev->model ? STRING( pev->model ) : "models/guerilla.mdl" );
	m_terroristType = TERRORIST_JUNGLE;
	if ( !m_language )
		m_language = ALLOC_STRING( "SP" );
}

class CTerroristRussian : public CTerrorist
{
public:
	void Precache() override;
	void SpawnInit() override;
};

void CTerroristRussian::Precache()
{
	PRECACHE_MODEL( pev->model ? (char*)STRING( pev->model ) : "models/terror.mdl" );
	CTerrorist::Precache();
}

void CTerroristRussian::SpawnInit()
{
	SET_MODEL( edict(), pev->model ? STRING( pev->model ) : "models/terror.mdl" );
	m_terroristType = TERRORIST_RUSSIAN;
	if ( !m_language )
		m_language = ALLOC_STRING( "RU" );
}

class CTerroristAsian : public CTerrorist
{
public:
	void Precache() override;
	void SpawnInit() override;
};

void CTerroristAsian::Precache()
{
	PRECACHE_MODEL( pev->model ? (char*)STRING( pev->model ) : "models/asian.mdl" );
	CTerrorist::Precache();
}

void CTerroristAsian::SpawnInit()
{
	SET_MODEL( edict(), pev->model ? STRING( pev->model ) : "models/asian.mdl" );
	m_terroristType = TERRORIST_ASIAN;
	if ( !m_language )
		m_language = ALLOC_STRING( "JA" );
}

class CTerroristRepel : public CBaseMonster
{
public:
	void Spawn() override;
	void Precache() override;
	void EXPORT RepelUse( CBaseEntity* activator, CBaseEntity* caller, USE_TYPE useType, float value );
	int m_iSpriteTexture;
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

void CTerroristRepel::Spawn()
{
	Precache();
	pev->solid = SOLID_NOT;
	SetUse( &CTerroristRepel::RepelUse );
}

void CTerroristRepel::Precache()
{
	UTIL_PrecacheOther( "monster_terrorist" );
	m_iSpriteTexture = PRECACHE_MODEL( "sprites/rope.spr" );
}

void CTerroristRepel::RepelUse( CBaseEntity*, CBaseEntity*, USE_TYPE, float )
{
	TraceResult trace;
	UTIL_TraceLine( pev->origin, pev->origin + Vector( 0, 0, -4096 ), dont_ignore_monsters, edict(), &trace );
	CBaseMonster* monster = CBaseEntity::Create( "monster_terrorist", pev->origin, pev->angles )->MyMonsterPointer();
	monster->pev->movetype = MOVETYPE_FLY;
	float speed = RANDOM_FLOAT( -196, -128 );
	monster->pev->velocity = Vector( 0, 0, speed );
	monster->SetActivity( ACT_GLIDE );
	monster->m_vecLastPosition = trace.vecEndPos;
	CBeam* beam = CBeam::BeamCreate( "sprites/rope.spr", 10 );
	beam->PointEntInit( pev->origin + Vector( 0, 0, 112 ), monster->entindex() );
	beam->SetFlags( BEAM_FSOLID );
	beam->SetColor( 255, 255, 255 );
	beam->SetThink( &CBaseEntity::SUB_Remove );
	beam->pev->nextthink = (float)( -4096.0 * trace.flFraction / monster->pev->velocity.z + gpGlobals->time + 0.5 );
	UTIL_Remove( this );
}

class CDeadTerrorist : public CBaseMonster
{
public:
	void Spawn() override;
	int Classify() override { return CLASS_HUMAN_MILITARY; }
	void KeyValue( KeyValueData* data ) override;
	int m_iPose;
	static const char* m_szPoses[3];
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

const char* CDeadTerrorist::m_szPoses[3] = { "deadstomach", "deadside", "deadsitting" };

void CDeadTerrorist::KeyValue( KeyValueData* data )
{
	if ( FStrEq( data->szKeyName, "pose" ) )
	{
		m_iPose = atoi( data->szValue );
		data->fHandled = TRUE;
	}
	else
		CBaseMonster::KeyValue( data );
}

void CDeadTerrorist::Spawn()
{
	PRECACHE_MODEL( "models/monster_terrorist_urban_mp5.mdl" );
	SET_MODEL( edict(), "models/monster_terrorist_urban_mp5.mdl" );
	pev->effects = 0;
	pev->yaw_speed = 8;
	pev->sequence = 0;
	m_bloodColor = BLOOD_COLOR_RED;
	pev->sequence = LookupSequence( m_szPoses[m_iPose] );
	if ( pev->sequence == -1 )
		ALERT( at_console, "Dead terrorist with bad pose\n" );
	pev->health = 8;
	pev->body = 0;
	pev->skin = 0;
	SetBodygroup( 1, RANDOM_LONG( 0, 5 ) );
	SetBodygroup( 2, RANDOM_LONG( 0, 10 ) );
	MonsterInitDead();
}

LINK_ENTITY_TO_CLASS( monster_terrorist, CTerrorist );
LINK_ENTITY_TO_CLASS( monster_t, CTerrorist );
LINK_ENTITY_TO_CLASS( monster_terrorist_repel, CTerroristRepel );
LINK_ENTITY_TO_CLASS( monster_t_repel, CTerroristRepel );
LINK_ENTITY_TO_CLASS( monster_terrorist_dead, CDeadTerrorist );
LINK_ENTITY_TO_CLASS( monster_t_dead, CDeadTerrorist );

LINK_ENTITY_TO_CLASS( monster_terrorist_arctic, CTerroristArctic );
LINK_ENTITY_TO_CLASS( monster_terrorist_arctic_pistol, CTerroristArctic );
LINK_ENTITY_TO_CLASS( monster_terrorist_arctic_shotgun, CTerroristArctic );
LINK_ENTITY_TO_CLASS( monster_terrorist_arctic_assaultrifle, CTerroristArctic );
LINK_ENTITY_TO_CLASS( monster_terrorist_arctic_machinegun, CTerroristArctic );
LINK_ENTITY_TO_CLASS( monster_terrorist_arctic_sniperrifle, CTerroristArctic );
LINK_ENTITY_TO_CLASS( monster_terrorist_arctic_law, CTerroristArctic );
LINK_ENTITY_TO_CLASS( monster_terrorist_arctic_smg, CTerroristArctic );
LINK_ENTITY_TO_CLASS( monster_terrorist_arctic_melee, CTerroristArctic );
LINK_ENTITY_TO_CLASS( monster_terrorist_arctic_grenader, CTerroristArctic );
LINK_ENTITY_TO_CLASS( monster_terrorist_arctic_kamakazi, CTerroristArctic );
LINK_ENTITY_TO_CLASS( monster_terrorist_arctic_mp5, CTerroristArctic );
LINK_ENTITY_TO_CLASS( monster_t_arctic, CTerroristArctic );
LINK_ENTITY_TO_CLASS( monster_t_arctic_pistol, CTerroristArctic );
LINK_ENTITY_TO_CLASS( monster_t_arctic_shotgun, CTerroristArctic );
LINK_ENTITY_TO_CLASS( monster_t_arctic_assaultrifle, CTerroristArctic );
LINK_ENTITY_TO_CLASS( monster_t_arctic_machinegun, CTerroristArctic );
LINK_ENTITY_TO_CLASS( monster_t_arctic_sniperrifle, CTerroristArctic );
LINK_ENTITY_TO_CLASS( monster_t_arctic_law, CTerroristArctic );
LINK_ENTITY_TO_CLASS( monster_t_arctic_smg, CTerroristArctic );
LINK_ENTITY_TO_CLASS( monster_t_arctic_melee, CTerroristArctic );
LINK_ENTITY_TO_CLASS( monster_t_arctic_grenader, CTerroristArctic );
LINK_ENTITY_TO_CLASS( monster_t_arctic_kamakazi, CTerroristArctic );
LINK_ENTITY_TO_CLASS( monster_t_arctic_mp5, CTerroristArctic );

LINK_ENTITY_TO_CLASS( monster_terrorist_desert, CTerroristDesert );
LINK_ENTITY_TO_CLASS( monster_terrorist_desert_pistol, CTerroristDesert );
LINK_ENTITY_TO_CLASS( monster_terrorist_desert_shotgun, CTerroristDesert );
LINK_ENTITY_TO_CLASS( monster_terrorist_desert_assaultrifle, CTerroristDesert );
LINK_ENTITY_TO_CLASS( monster_terrorist_desert_machinegun, CTerroristDesert );
LINK_ENTITY_TO_CLASS( monster_terrorist_desert_sniperrifle, CTerroristDesert );
LINK_ENTITY_TO_CLASS( monster_terrorist_desert_law, CTerroristDesert );
LINK_ENTITY_TO_CLASS( monster_terrorist_desert_smg, CTerroristDesert );
LINK_ENTITY_TO_CLASS( monster_terrorist_desert_melee, CTerroristDesert );
LINK_ENTITY_TO_CLASS( monster_terrorist_desert_grenader, CTerroristDesert );
LINK_ENTITY_TO_CLASS( monster_terrorist_desert_kamakazi, CTerroristDesert );
LINK_ENTITY_TO_CLASS( monster_terrorist_desert_mp5, CTerroristDesert );
LINK_ENTITY_TO_CLASS( monster_t_desert, CTerroristDesert );
LINK_ENTITY_TO_CLASS( monster_t_desert_pistol, CTerroristDesert );
LINK_ENTITY_TO_CLASS( monster_t_desert_shotgun, CTerroristDesert );
LINK_ENTITY_TO_CLASS( monster_t_desert_assaultrifle, CTerroristDesert );
LINK_ENTITY_TO_CLASS( monster_t_desert_machinegun, CTerroristDesert );
LINK_ENTITY_TO_CLASS( monster_t_desert_sniperrifle, CTerroristDesert );
LINK_ENTITY_TO_CLASS( monster_t_desert_law, CTerroristDesert );
LINK_ENTITY_TO_CLASS( monster_t_desert_smg, CTerroristDesert );
LINK_ENTITY_TO_CLASS( monster_t_desert_melee, CTerroristDesert );
LINK_ENTITY_TO_CLASS( monster_t_desert_grenader, CTerroristDesert );
LINK_ENTITY_TO_CLASS( monster_t_desert_kamakazi, CTerroristDesert );
LINK_ENTITY_TO_CLASS( monster_t_desert_mp5, CTerroristDesert );

LINK_ENTITY_TO_CLASS( monster_terrorist_urban, CTerroristUrban );
LINK_ENTITY_TO_CLASS( monster_terrorist_urban_pistol, CTerroristUrban );
LINK_ENTITY_TO_CLASS( monster_terrorist_urban_shotgun, CTerroristUrban );
LINK_ENTITY_TO_CLASS( monster_terrorist_urban_assaultrifle, CTerroristUrban );
LINK_ENTITY_TO_CLASS( monster_terrorist_urban_machinegun, CTerroristUrban );
LINK_ENTITY_TO_CLASS( monster_terrorist_urban_sniperrifle, CTerroristUrban );
LINK_ENTITY_TO_CLASS( monster_terrorist_urban_law, CTerroristUrban );
LINK_ENTITY_TO_CLASS( monster_terrorist_urban_smg, CTerroristUrban );
LINK_ENTITY_TO_CLASS( monster_terrorist_urban_melee, CTerroristUrban );
LINK_ENTITY_TO_CLASS( monster_terrorist_urban_grenader, CTerroristUrban );
LINK_ENTITY_TO_CLASS( monster_terrorist_urban_kamakazi, CTerroristUrban );
LINK_ENTITY_TO_CLASS( monster_terrorist_urban_mp5, CTerroristUrban );
LINK_ENTITY_TO_CLASS( monster_t_urban, CTerroristUrban );
LINK_ENTITY_TO_CLASS( monster_t_urban_pistol, CTerroristUrban );
LINK_ENTITY_TO_CLASS( monster_t_urban_shotgun, CTerroristUrban );
LINK_ENTITY_TO_CLASS( monster_t_urban_assaultrifle, CTerroristUrban );
LINK_ENTITY_TO_CLASS( monster_t_urban_machinegun, CTerroristUrban );
LINK_ENTITY_TO_CLASS( monster_t_urban_sniperrifle, CTerroristUrban );
LINK_ENTITY_TO_CLASS( monster_t_urban_law, CTerroristUrban );
LINK_ENTITY_TO_CLASS( monster_t_urban_smg, CTerroristUrban );
LINK_ENTITY_TO_CLASS( monster_t_urban_melee, CTerroristUrban );
LINK_ENTITY_TO_CLASS( monster_t_urban_grenader, CTerroristUrban );
LINK_ENTITY_TO_CLASS( monster_t_urban_kamakazi, CTerroristUrban );
LINK_ENTITY_TO_CLASS( monster_t_urban_mp5, CTerroristUrban );

LINK_ENTITY_TO_CLASS( monster_terrorist_jungle, CTerroristJungle );
LINK_ENTITY_TO_CLASS( monster_terrorist_jungle_pistol, CTerroristJungle );
LINK_ENTITY_TO_CLASS( monster_terrorist_jungle_shotgun, CTerroristJungle );
LINK_ENTITY_TO_CLASS( monster_terrorist_jungle_assaultrifle, CTerroristJungle );
LINK_ENTITY_TO_CLASS( monster_terrorist_jungle_machinegun, CTerroristJungle );
LINK_ENTITY_TO_CLASS( monster_terrorist_jungle_sniperrifle, CTerroristJungle );
LINK_ENTITY_TO_CLASS( monster_terrorist_jungle_law, CTerroristJungle );
LINK_ENTITY_TO_CLASS( monster_terrorist_jungle_smg, CTerroristJungle );
LINK_ENTITY_TO_CLASS( monster_terrorist_jungle_melee, CTerroristJungle );
LINK_ENTITY_TO_CLASS( monster_terrorist_jungle_grenader, CTerroristJungle );
LINK_ENTITY_TO_CLASS( monster_terrorist_jungle_kamakazi, CTerroristJungle );
LINK_ENTITY_TO_CLASS( monster_terrorist_jungle_mp5, CTerroristJungle );
LINK_ENTITY_TO_CLASS( monster_t_jungle, CTerroristJungle );
LINK_ENTITY_TO_CLASS( monster_t_jungle_pistol, CTerroristJungle );
LINK_ENTITY_TO_CLASS( monster_t_jungle_shotgun, CTerroristJungle );
LINK_ENTITY_TO_CLASS( monster_t_jungle_assaultrifle, CTerroristJungle );
LINK_ENTITY_TO_CLASS( monster_t_jungle_machinegun, CTerroristJungle );
LINK_ENTITY_TO_CLASS( monster_t_jungle_sniperrifle, CTerroristJungle );
LINK_ENTITY_TO_CLASS( monster_t_jungle_law, CTerroristJungle );
LINK_ENTITY_TO_CLASS( monster_t_jungle_smg, CTerroristJungle );
LINK_ENTITY_TO_CLASS( monster_t_jungle_melee, CTerroristJungle );
LINK_ENTITY_TO_CLASS( monster_t_jungle_grenader, CTerroristJungle );
LINK_ENTITY_TO_CLASS( monster_t_jungle_kamakazi, CTerroristJungle );
LINK_ENTITY_TO_CLASS( monster_t_jungle_mp5, CTerroristJungle );

LINK_ENTITY_TO_CLASS( monster_terrorist_russian, CTerroristRussian );
LINK_ENTITY_TO_CLASS( monster_terrorist_russian_pistol, CTerroristRussian );
LINK_ENTITY_TO_CLASS( monster_terrorist_russian_shotgun, CTerroristRussian );
LINK_ENTITY_TO_CLASS( monster_terrorist_russian_assaultrifle, CTerroristRussian );
LINK_ENTITY_TO_CLASS( monster_terrorist_russian_machinegun, CTerroristRussian );
LINK_ENTITY_TO_CLASS( monster_terrorist_russian_sniperrifle, CTerroristRussian );
LINK_ENTITY_TO_CLASS( monster_terrorist_russian_law, CTerroristRussian );
LINK_ENTITY_TO_CLASS( monster_terrorist_russian_smg, CTerroristRussian );
LINK_ENTITY_TO_CLASS( monster_terrorist_russian_melee, CTerroristRussian );
LINK_ENTITY_TO_CLASS( monster_terrorist_russian_grenader, CTerroristRussian );
LINK_ENTITY_TO_CLASS( monster_terrorist_russian_kamakazi, CTerroristRussian );
LINK_ENTITY_TO_CLASS( monster_terrorist_russian_mp5, CTerroristRussian );
LINK_ENTITY_TO_CLASS( monster_t_russian, CTerroristRussian );
LINK_ENTITY_TO_CLASS( monster_t_russian_pistol, CTerroristRussian );
LINK_ENTITY_TO_CLASS( monster_t_russian_shotgun, CTerroristRussian );
LINK_ENTITY_TO_CLASS( monster_t_russian_assaultrifle, CTerroristRussian );
LINK_ENTITY_TO_CLASS( monster_t_russian_machinegun, CTerroristRussian );
LINK_ENTITY_TO_CLASS( monster_t_russian_sniperrifle, CTerroristRussian );
LINK_ENTITY_TO_CLASS( monster_t_russian_law, CTerroristRussian );
LINK_ENTITY_TO_CLASS( monster_t_russian_smg, CTerroristRussian );
LINK_ENTITY_TO_CLASS( monster_t_russian_melee, CTerroristRussian );
LINK_ENTITY_TO_CLASS( monster_t_russian_grenader, CTerroristRussian );
LINK_ENTITY_TO_CLASS( monster_t_russian_kamakazi, CTerroristRussian );
LINK_ENTITY_TO_CLASS( monster_t_russian_mp5, CTerroristRussian );

LINK_ENTITY_TO_CLASS( monster_terrorist_asian, CTerroristAsian );
LINK_ENTITY_TO_CLASS( monster_terrorist_asian_pistol, CTerroristAsian );
LINK_ENTITY_TO_CLASS( monster_terrorist_asian_shotgun, CTerroristAsian );
LINK_ENTITY_TO_CLASS( monster_terrorist_asian_assaultrifle, CTerroristAsian );
LINK_ENTITY_TO_CLASS( monster_terrorist_asian_machinegun, CTerroristAsian );
LINK_ENTITY_TO_CLASS( monster_terrorist_asian_sniperrifle, CTerroristAsian );
LINK_ENTITY_TO_CLASS( monster_terrorist_asian_law, CTerroristAsian );
LINK_ENTITY_TO_CLASS( monster_terrorist_asian_smg, CTerroristAsian );
LINK_ENTITY_TO_CLASS( monster_terrorist_asian_melee, CTerroristAsian );
LINK_ENTITY_TO_CLASS( monster_terrorist_asian_grenader, CTerroristAsian );
LINK_ENTITY_TO_CLASS( monster_terrorist_asian_kamakazi, CTerroristAsian );
LINK_ENTITY_TO_CLASS( monster_terrorist_asian_mp5, CTerroristAsian );
LINK_ENTITY_TO_CLASS( monster_t_asian, CTerroristAsian );
LINK_ENTITY_TO_CLASS( monster_t_asian_pistol, CTerroristAsian );
LINK_ENTITY_TO_CLASS( monster_t_asian_shotgun, CTerroristAsian );
LINK_ENTITY_TO_CLASS( monster_t_asian_assaultrifle, CTerroristAsian );
LINK_ENTITY_TO_CLASS( monster_t_asian_machinegun, CTerroristAsian );
LINK_ENTITY_TO_CLASS( monster_t_asian_sniperrifle, CTerroristAsian );
LINK_ENTITY_TO_CLASS( monster_t_asian_law, CTerroristAsian );
LINK_ENTITY_TO_CLASS( monster_t_asian_smg, CTerroristAsian );
LINK_ENTITY_TO_CLASS( monster_t_asian_melee, CTerroristAsian );
LINK_ENTITY_TO_CLASS( monster_t_asian_grenader, CTerroristAsian );
LINK_ENTITY_TO_CLASS( monster_t_asian_kamakazi, CTerroristAsian );
LINK_ENTITY_TO_CLASS( monster_t_asian_mp5, CTerroristAsian );

Task_t tlTerroristFail[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_SET_ACTIVITY, ACT_IDLE },
	{ TASK_WAIT, 2 },
	{ TASK_WAIT_PVS, 0 }
};

Schedule_t slTerroristFail[] =
{
	{ tlTerroristFail, ARRAYSIZE( tlTerroristFail ), bits_COND_CAN_ATTACK, 0, "Terrorist Fail" }
};

Task_t tlTerroristCombatFail[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_SET_ACTIVITY, ACT_IDLE },
	{ TASK_WAIT_FACE_ENEMY, 2 },
	{ TASK_WAIT_PVS, 0 }
};

Schedule_t slTerroristCombatFail[] =
{
	{ tlTerroristCombatFail, ARRAYSIZE( tlTerroristCombatFail ),
		bits_COND_CAN_RANGE_ATTACK1 | bits_COND_CAN_RANGE_ATTACK2, 0, "Terrorist Combat Fail" }
};

Task_t tlTerroristVictoryDance[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_FACE_ENEMY, 0 },
	{ TASK_WAIT, 1.5f },
	{ TASK_GET_PATH_TO_ENEMY_CORPSE, 0 },
	{ TASK_WALK_PATH, 0 },
	{ TASK_WAIT_FOR_MOVEMENT, 0 },
	{ TASK_FACE_ENEMY, 0 },
	{ TASK_PLAY_SEQUENCE, ACT_VICTORY_DANCE }
};

Schedule_t slTerroristVictoryDance[] =
{
	{ tlTerroristVictoryDance, ARRAYSIZE( tlTerroristVictoryDance ),
		bits_COND_NEW_ENEMY | bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE, 0, "TerroristVictoryDance" }
};

Task_t tlTerroristEstablishLineOfFire[] =
{
	{ TASK_SET_FAIL_SCHEDULE, SCHED_TERRORIST_ELOF_FAIL },
	{ TASK_GET_PATH_TO_ENEMY, 0 },
	{ TASK_TERRORIST_SPEAK_SENTENCE, 0 },
	{ TASK_RUN_PATH, 0 },
	{ TASK_WAIT_FOR_MOVEMENT, 0 }
};

Schedule_t slTerroristEstablishLineOfFire[] =
{
	{ tlTerroristEstablishLineOfFire, ARRAYSIZE( tlTerroristEstablishLineOfFire ),
		bits_COND_ENEMY_DEAD | bits_COND_NEW_ENEMY | bits_COND_HEAR_SOUND |
		bits_COND_CAN_ATTACK, bits_SOUND_DANGER, "TerroristEstablishLineOfFire" }
};

Task_t tlTerroristFoundEnemy[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_SET_FAIL_SCHEDULE, SCHED_IDLE_STAND },
	{ TASK_FACE_ENEMY, 0 },
	{ TASK_PLAY_SEQUENCE_FACE_ENEMY, ACT_SIGNAL1 }
};

Schedule_t slTerroristFoundEnemy[] =
{
	{ tlTerroristFoundEnemy, ARRAYSIZE( tlTerroristFoundEnemy ), bits_COND_HEAR_SOUND,
		bits_SOUND_DANGER, "TerroristFoundEnemy" }
};

Task_t tlTerroristCombatFace1[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_SET_ACTIVITY, ACT_IDLE },
	{ TASK_FACE_ENEMY, 0 },
	{ TASK_WAIT, 1.5f },
	{ TASK_SET_SCHEDULE, SCHED_TERRORIST_SWEEP }
};

Schedule_t slTerroristCombatFace[] =
{
	{ tlTerroristCombatFace1, ARRAYSIZE( tlTerroristCombatFace1 ),
		bits_COND_ENEMY_DEAD | bits_COND_NEW_ENEMY | bits_COND_CAN_RANGE_ATTACK1 | bits_COND_CAN_RANGE_ATTACK2,
		0, "Combat Face" }
};

Task_t tlTerroristSignalSuppress[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_FACE_IDEAL, 0 },
	{ TASK_PLAY_SEQUENCE_FACE_ENEMY, ACT_SIGNAL2 },
	{ TASK_FACE_ENEMY, 0 },
	{ TASK_TERRORIST_CHECK_FIRE, 0 },
	{ TASK_RANGE_ATTACK1, 0 },
	{ TASK_FACE_ENEMY, 0 },
	{ TASK_TERRORIST_CHECK_FIRE, 0 },
	{ TASK_RANGE_ATTACK1, 0 },
	{ TASK_FACE_ENEMY, 0 },
	{ TASK_TERRORIST_CHECK_FIRE, 0 },
	{ TASK_RANGE_ATTACK1, 0 },
	{ TASK_FACE_ENEMY, 0 },
	{ TASK_TERRORIST_CHECK_FIRE, 0 },
	{ TASK_RANGE_ATTACK1, 0 },
	{ TASK_FACE_ENEMY, 0 },
	{ TASK_TERRORIST_CHECK_FIRE, 0 },
	{ TASK_RANGE_ATTACK1, 0 }
};

Schedule_t slTerroristSignalSuppress[] =
{
	{ tlTerroristSignalSuppress, ARRAYSIZE( tlTerroristSignalSuppress ),
		bits_COND_ENEMY_DEAD | bits_COND_HEAR_SOUND | bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE,
		bits_SOUND_DANGER, "SignalSuppress" }
};

Task_t tlTerroristSuppress[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_FACE_ENEMY, 0 },
	{ TASK_TERRORIST_CHECK_FIRE, 0 },
	{ TASK_RANGE_ATTACK1, 0 },
	{ TASK_FACE_ENEMY, 0 },
	{ TASK_TERRORIST_CHECK_FIRE, 0 },
	{ TASK_RANGE_ATTACK1, 0 },
	{ TASK_FACE_ENEMY, 0 },
	{ TASK_TERRORIST_CHECK_FIRE, 0 },
	{ TASK_RANGE_ATTACK1, 0 },
	{ TASK_FACE_ENEMY, 0 },
	{ TASK_TERRORIST_CHECK_FIRE, 0 },
	{ TASK_RANGE_ATTACK1, 0 },
	{ TASK_FACE_ENEMY, 0 },
	{ TASK_TERRORIST_CHECK_FIRE, 0 },
	{ TASK_RANGE_ATTACK1, 0 }
};

Schedule_t slTerroristSuppress[] =
{
	{ tlTerroristSuppress, ARRAYSIZE( tlTerroristSuppress ),
		bits_COND_SPECIAL1 | bits_COND_ENEMY_DEAD | bits_COND_HEAR_SOUND | bits_COND_LIGHT_DAMAGE |
		bits_COND_HEAVY_DAMAGE | bits_COND_NO_AMMO_LOADED, bits_SOUND_DANGER, "Suppress" }
};

Task_t tlTerroristWaitInCover[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_SET_ACTIVITY, ACT_IDLE },
	{ TASK_FACE_ENEMY, 0 }
};

Schedule_t slTerroristWaitInCover[] =
{
	{ tlTerroristWaitInCover, ARRAYSIZE( tlTerroristWaitInCover ),
		bits_COND_NEW_ENEMY | bits_COND_HEAR_SOUND | bits_COND_CAN_ATTACK, bits_SOUND_DANGER, "TerroristWaitInCover" }
};

Task_t tlTerroristTakeCover1[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_SET_FAIL_SCHEDULE, SCHED_TERRORIST_TAKECOVER_FAILED },
	{ TASK_FIND_COVER_FROM_ENEMY, 0 },
	{ TASK_RUN_PATH, 0 },
	{ TASK_WAIT_FOR_MOVEMENT, 0 },
	{ TASK_REMEMBER, bits_MEMORY_INCOVER },
	{ TASK_SET_SCHEDULE, SCHED_TERRORIST_WAIT_FACE_ENEMY }
};

Schedule_t slTerroristTakeCover[] =
{
	{ tlTerroristTakeCover1, ARRAYSIZE( tlTerroristTakeCover1 ), 0, 0, "TakeCover" }
};

Task_t tlTerroristGrenadeCover1[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_SET_FAIL_SCHEDULE, SCHED_TERRORIST_TAKECOVER_FAILED },
	{ TASK_FIND_COVER_FROM_ENEMY, 99 },
	{ TASK_FIND_FAR_NODE_COVER_FROM_ENEMY, 384 },
	{ TASK_PLAY_SEQUENCE, ACT_SPECIAL_ATTACK1 },
	{ TASK_CLEAR_MOVE_WAIT, 0 },
	{ TASK_RUN_PATH, 0 },
	{ TASK_WAIT_FOR_MOVEMENT, 0 },
	{ TASK_SET_SCHEDULE, SCHED_TERRORIST_WAIT_FACE_ENEMY }
};

Schedule_t slTerroristGrenadeCover[] =
{
	{ tlTerroristGrenadeCover1, ARRAYSIZE( tlTerroristGrenadeCover1 ), 0, 0, "GrenadeCover" }
};

Task_t tlTerroristTossGrenadeCover1[] =
{
	{ TASK_FACE_ENEMY, 0 },
	{ TASK_RANGE_ATTACK2, 0 },
	{ TASK_SET_SCHEDULE, SCHED_TAKE_COVER_FROM_ENEMY }
};

Schedule_t slTerroristTossGrenadeCover[] =
{
	{ tlTerroristTossGrenadeCover1, ARRAYSIZE( tlTerroristTossGrenadeCover1 ), 0, 0, "TossGrenadeCover" }
};

Task_t tlTerroristTakeCoverFromBestSound[] =
{
	{ TASK_SET_FAIL_SCHEDULE, SCHED_COWER },
	{ TASK_STOP_MOVING, 0 },
	{ TASK_FIND_COVER_FROM_BEST_SOUND, 0 },
	{ TASK_RUN_PATH, 0 },
	{ TASK_WAIT_FOR_MOVEMENT, 0 },
	{ TASK_REMEMBER, bits_MEMORY_INCOVER },
	{ TASK_TURN_LEFT, 179 }
};

Schedule_t slTerroristTakeCoverFromBestSound[] =
{
	{ tlTerroristTakeCoverFromBestSound, ARRAYSIZE( tlTerroristTakeCoverFromBestSound ),
		0, 0, "TerroristTakeCoverFromBestSound" }
};

Task_t tlTerroristHideReload[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_SET_FAIL_SCHEDULE, SCHED_RELOAD },
	{ TASK_FIND_COVER_FROM_ENEMY, 0 },
	{ TASK_RUN_PATH, 0 },
	{ TASK_WAIT_FOR_MOVEMENT, 0 },
	{ TASK_REMEMBER, bits_MEMORY_INCOVER },
	{ TASK_FACE_ENEMY, 0 },
	{ TASK_PLAY_SEQUENCE, ACT_RELOAD }
};

Schedule_t slTerroristHideReload[] =
{
	{ tlTerroristHideReload, ARRAYSIZE( tlTerroristHideReload ), bits_COND_HEAR_SOUND | bits_COND_HEAVY_DAMAGE,
		bits_SOUND_DANGER, "TerroristHideReload" }
};

Task_t tlTerroristSweep[] =
{
	{ TASK_TURN_LEFT, 179 },
	{ TASK_WAIT, 1 },
	{ TASK_TURN_LEFT, 179 },
	{ TASK_WAIT, 1 }
};

Schedule_t slTerroristSweep[] =
{
	{ tlTerroristSweep, ARRAYSIZE( tlTerroristSweep ),
		bits_COND_NEW_ENEMY | bits_COND_HEAR_SOUND | bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE |
		bits_COND_CAN_RANGE_ATTACK1 | bits_COND_CAN_RANGE_ATTACK2,
		bits_SOUND_WORLD | bits_SOUND_PLAYER | bits_SOUND_DANGER, "Terrorist Sweep" }
};

Task_t tlTerroristRangeAttack1A[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_PLAY_SEQUENCE_FACE_ENEMY, ACT_CROUCH },
	{ TASK_TERRORIST_CHECK_FIRE, 0 },
	{ TASK_RANGE_ATTACK1, 0 }
};

Schedule_t slTerroristRangeAttack1A[] =
{
	{ tlTerroristRangeAttack1A, ARRAYSIZE( tlTerroristRangeAttack1A ),
		bits_COND_SPECIAL1 | bits_COND_ENEMY_DEAD | bits_COND_NEW_ENEMY | bits_COND_HEAR_SOUND |
		bits_COND_HEAVY_DAMAGE | bits_COND_ENEMY_OCCLUDED | bits_COND_NO_AMMO_LOADED,
		bits_SOUND_DANGER, "Range Attack1A" }
};

Task_t tlTerroristRangeAttack1B[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_FACE_ENEMY, 0 },
	{ TASK_TERRORIST_CHECK_FIRE, 0 },
	{ TASK_RANGE_ATTACK1, 0 }
};

Schedule_t slTerroristRangeAttack1B[] =
{
	{ tlTerroristRangeAttack1B, ARRAYSIZE( tlTerroristRangeAttack1B ),
		bits_COND_SPECIAL1 | bits_COND_ENEMY_DEAD | bits_COND_NEW_ENEMY | bits_COND_HEAR_SOUND |
		bits_COND_HEAVY_DAMAGE | bits_COND_ENEMY_OCCLUDED | bits_COND_NO_AMMO_LOADED,
		bits_SOUND_DANGER, "Range Attack1B" }
};

Task_t tlTerroristRangeAttack1C[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_FACE_ENEMY, 0 },
	{ TASK_TERRORIST_CHECK_FIRE, 0 },
	{ TASK_RANGE_ATTACK1, 0 }
};

Schedule_t slTerroristRangeAttack1C[] =
{
	{ tlTerroristRangeAttack1C, ARRAYSIZE( tlTerroristRangeAttack1C ),
		bits_COND_SPECIAL1 | bits_COND_ENEMY_DEAD | bits_COND_NEW_ENEMY | bits_COND_HEAR_SOUND |
		bits_COND_HEAVY_DAMAGE | bits_COND_ENEMY_OCCLUDED | bits_COND_NO_AMMO_LOADED,
		bits_SOUND_DANGER, "Range Attack1C" }
};

Task_t tlTerroristRangeAttack2[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_TERRORIST_FACE_TOSS_DIR, 0 },
	{ TASK_PLAY_SEQUENCE, ACT_RANGE_ATTACK2 },
	{ TASK_SET_SCHEDULE, SCHED_TERRORIST_WAIT_FACE_ENEMY }
};

Schedule_t slTerroristRangeAttack2[] =
{
	{ tlTerroristRangeAttack2, ARRAYSIZE( tlTerroristRangeAttack2 ), 0, 0, "RangeAttack2" }
};

Task_t tlTerroristRepel[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_FACE_IDEAL, 0 },
	{ TASK_PLAY_SEQUENCE, ACT_GLIDE }
};

Schedule_t slTerroristRepel[] =
{
	{ tlTerroristRepel, ARRAYSIZE( tlTerroristRepel ),
		bits_COND_NEW_ENEMY | bits_COND_HEAR_SOUND | bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE |
		bits_COND_SEE_ENEMY, bits_SOUND_DANGER | bits_SOUND_COMBAT | bits_SOUND_PLAYER, "Repel" }
};

Task_t tlTerroristRepelAttack[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_FACE_ENEMY, 0 },
	{ TASK_PLAY_SEQUENCE, ACT_FLY }
};

Schedule_t slTerroristRepelAttack[] =
{
	{ tlTerroristRepelAttack, ARRAYSIZE( tlTerroristRepelAttack ), bits_COND_ENEMY_OCCLUDED, 0, "Repel Attack" }
};

Task_t tlTerroristRepelLand[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_PLAY_SEQUENCE, ACT_LAND },
	{ TASK_GET_PATH_TO_LASTPOSITION, 0 },
	{ TASK_RUN_PATH, 0 },
	{ TASK_WAIT_FOR_MOVEMENT, 0 },
	{ TASK_CLEAR_LASTPOSITION, 0 }
};

Schedule_t slTerroristRepelLand[] =
{
	{ tlTerroristRepelLand, ARRAYSIZE( tlTerroristRepelLand ),
		bits_COND_NEW_ENEMY | bits_COND_HEAR_SOUND | bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE |
		bits_COND_SEE_ENEMY, bits_SOUND_DANGER | bits_SOUND_COMBAT | bits_SOUND_PLAYER, "Repel Land" }
};

Task_t tlTerroristInSmokeSchedule[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_SET_FAIL_SCHEDULE, SCHED_COWER },
	{ TASK_FACE_ENEMY, 0 },
	{ TASK_PLAY_SEQUENCE, ACT_TERRORIST_CHOKING }
};

Schedule_t slTerroristInSmoke[] =
{
	{ tlTerroristInSmokeSchedule, ARRAYSIZE( tlTerroristInSmokeSchedule ), bits_COND_HEAVY_DAMAGE, 0, "TerroristInSmoke" }
};

Task_t tlTerroristFlashedSchedule[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_SET_FAIL_SCHEDULE, SCHED_COWER },
	{ TASK_FACE_ENEMY, 0 },
	{ TASK_PLAY_SEQUENCE, ACT_TERRORIST_FLASHED }
};

Schedule_t slTerroristFlashed[] =
{
	{ tlTerroristFlashedSchedule, ARRAYSIZE( tlTerroristFlashedSchedule ), bits_COND_HEAVY_DAMAGE, 0, "TerroristFlashed" }
};

Task_t tlTerroristKamakaziBlowup[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_PLAY_SEQUENCE, ACT_TERRORIST_KAMAKAZI_EXPLODE }
};

Schedule_t slTerroristKamakaziBlowup[] =
{
	{ tlTerroristKamakaziBlowup, ARRAYSIZE( tlTerroristKamakaziBlowup ), 0, 0, "KamakaziTerroristBlowup" }
};

Task_t tlTerroristSniperAttack[] =
{
	{ TASK_SET_FAIL_SCHEDULE, SCHED_FAIL },
	{ TASK_STOP_MOVING, 0 },
	{ TASK_FACE_ENEMY, 0 },
	{ TASK_PLAY_SEQUENCE, ACT_ARM },
	{ TASK_TERRORIST_CHECK_FIRE, 0 },
	{ TASK_RANGE_ATTACK1, 0 },
	{ TASK_PLAY_SEQUENCE, ACT_DISARM },
	{ TASK_FACE_ENEMY, 0 }
};

Schedule_t slTerroristSniperAttack[] =
{
	{ tlTerroristSniperAttack, ARRAYSIZE( tlTerroristSniperAttack ),
		bits_COND_ENEMY_DEAD | bits_COND_NEW_ENEMY | bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE |
		bits_COND_ENEMY_OCCLUDED | bits_COND_NO_AMMO_LOADED, 0, "Sniper Attack" }
};

Task_t tlTerroristSniperTakeShot[] =
{
	{ TASK_SET_FAIL_SCHEDULE, SCHED_FAIL },
	{ TASK_STOP_MOVING, 0 },
	{ TASK_RANGE_ATTACK1, 0 }
};

Schedule_t slTerroristSniperTakeShot[] =
{
	{ tlTerroristSniperTakeShot, ARRAYSIZE( tlTerroristSniperTakeShot ),
		bits_COND_ENEMY_DEAD | bits_COND_NEW_ENEMY | bits_COND_NO_AMMO_LOADED, 0, "Sniper Take Shot" }
};

Task_t tlTerroristDuckCoverNode[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_SET_FAIL_SCHEDULE, SCHED_TERRORIST_TAKECOVER_FAILED },
	{ TASK_MOVE_TO_SPECIAL_COVER_NODE, 0 },
	{ TASK_WAIT_FOR_MOVEMENT, 0 },
	{ TASK_REMEMBER, bits_MEMORY_INCOVER },
	{ TASK_SET_SCHEDULE, SCHED_TERRORIST_DUCK_NODE_ATTACK }
};

Schedule_t slTerroristDuckCoverNode[] =
{
	{ tlTerroristDuckCoverNode, ARRAYSIZE( tlTerroristDuckCoverNode ), 0, 0, "DuckCoverNode" }
};

Task_t tlTerroristDuckNodeAttack[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_PLAY_SEQUENCE_FACE_ENEMY, ACT_CROUCH },
	{ TASK_WAIT, 1 },
	{ TASK_PLAY_SEQUENCE_FACE_ENEMY, ACT_STAND },
	{ TASK_TERRORIST_CHECK_FIRE, 0 },
	{ TASK_RANGE_ATTACK1, 0 },
	{ TASK_WAIT, 0.5f }
};

Schedule_t slTerroristDuckNodeAttack[] =
{
	{ tlTerroristDuckNodeAttack, ARRAYSIZE( tlTerroristDuckNodeAttack ),
		bits_COND_SPECIAL1 | bits_COND_ENEMY_DEAD | bits_COND_NEW_ENEMY | bits_COND_HEAR_SOUND |
		bits_COND_HEAVY_DAMAGE | bits_COND_NO_AMMO_LOADED, bits_SOUND_DANGER, "DuckNodeAttack" }
};

Task_t tlTerroristCloseIn[] =
{
	{ TASK_SET_FAIL_SCHEDULE, SCHED_TERRORIST_ELOF_FAIL },
	{ TASK_MOVE_TO_PREFERED_RANGE, 0 }
};

Schedule_t slTerroristCloseIn[] =
{
	{ tlTerroristCloseIn, ARRAYSIZE( tlTerroristCloseIn ),
		bits_COND_ENEMY_DEAD | bits_COND_NEW_ENEMY | bits_COND_HEAR_SOUND | bits_COND_CAN_MELEE_ATTACK1,
		0, "CloseInTerrorist" }
};

Task_t tlTerroristRangeAttackMG1[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_PLAY_SEQUENCE_FACE_ENEMY, ACT_CROUCH },
	{ TASK_TERRORIST_CHECK_FIRE, 0 },
	{ TASK_RANGE_ATTACK1, 0 }
};

Schedule_t slTerroristRangeAttackMG1[] =
{
	{ tlTerroristRangeAttackMG1, ARRAYSIZE( tlTerroristRangeAttackMG1 ),
		bits_COND_SPECIAL1 | bits_COND_ENEMY_DEAD | bits_COND_NEW_ENEMY | bits_COND_HEAR_SOUND |
		bits_COND_HEAVY_DAMAGE | bits_COND_ENEMY_OCCLUDED, bits_SOUND_DANGER, "MachineGun Attack1" }
};

Task_t tlTerroristRangeAttackMG2[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_FACE_ENEMY, 0 },
	{ TASK_TERRORIST_CHECK_FIRE, 0 },
	{ TASK_RANGE_ATTACK1, 0 }
};

Schedule_t slTerroristRangeAttackMG2[] =
{
	{ tlTerroristRangeAttackMG2, ARRAYSIZE( tlTerroristRangeAttackMG2 ),
		bits_COND_SPECIAL1 | bits_COND_ENEMY_DEAD | bits_COND_NEW_ENEMY | bits_COND_HEAR_SOUND |
		bits_COND_HEAVY_DAMAGE | bits_COND_ENEMY_OCCLUDED, bits_SOUND_DANGER, "MachineGun Attack2" }
};

Task_t tlTerroristCornerNodeAttack[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_FACE_ENEMY, 0 },
	{ TASK_WAIT, 0.5f },
	{ TASK_PLAY_SEQUENCE, ACT_TERRORIST_CORNER_FIRE1 },
	{ TASK_PLAY_SEQUENCE, ACT_TERRORIST_CORNER_FIRE2 },
	{ TASK_PLAY_SEQUENCE, ACT_IDLE }
};

Schedule_t slTerroristCornerNodeAttack[] =
{
	{ tlTerroristCornerNodeAttack, ARRAYSIZE( tlTerroristCornerNodeAttack ),
		bits_COND_SPECIAL1 | bits_COND_ENEMY_DEAD | bits_COND_NEW_ENEMY | bits_COND_HEAR_SOUND |
		bits_COND_HEAVY_DAMAGE | bits_COND_NO_AMMO_LOADED, bits_SOUND_DANGER, "CornerNodeAttack" }
};

Task_t tlTerroristCornerCoverNode[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_SET_FAIL_SCHEDULE, SCHED_TERRORIST_TAKECOVER_FAILED },
	{ TASK_MOVE_TO_SPECIAL_COVER_NODE, 0 },
	{ TASK_WAIT_FOR_MOVEMENT, 0 },
	{ TASK_REMEMBER, bits_MEMORY_INCOVER },
	{ TASK_SET_SCHEDULE, SCHED_TERRORIST_CORNER_NODE_ATTACK }
};

Schedule_t slTerroristCornerCoverNode[] =
{
	{ tlTerroristCornerCoverNode, ARRAYSIZE( tlTerroristCornerCoverNode ), 0, 0, "CornerCoverNode" }
};

Task_t tlTerroristFiringPause[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_SET_ACTIVITY, ACT_IDLE },
	{ TASK_FACE_ENEMY, 0 },
	{ TASK_WAIT_RANDOM, 0.6f }
};

Schedule_t slTerroristFiringPause[] =
{
	{ tlTerroristFiringPause, ARRAYSIZE( tlTerroristFiringPause ),
		bits_COND_SPECIAL1 | bits_COND_ENEMY_DEAD | bits_COND_ENEMY_OCCLUDED | bits_COND_NO_AMMO_LOADED,
		0, "Fire Pause" }
};

Task_t tlTerroristAlertSpeak[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_FACE_IDEAL, 0 },
	{ TASK_PLAY_SEQUENCE, ACT_TERRORIST_LOOKAROUND }
};

Schedule_t slTerroristAlertSpeak[] =
{
	{ tlTerroristAlertSpeak, ARRAYSIZE( tlTerroristAlertSpeak ), bits_COND_NEW_ENEMY, 0, "Alert Speak" }
};

Task_t tlTerroristInvestigate[] =
{
	{ TASK_SET_FAIL_SCHEDULE, SCHED_TARGET_FACE },
	{ TASK_GET_PATH_TO_SPOT, 0 },
	{ TASK_WALK_PATH, 0 },
	{ TASK_WAIT_FOR_MOVEMENT, 0 }
};

Schedule_t slTerroristInvestigate[] =
{
	{ tlTerroristInvestigate, ARRAYSIZE( tlTerroristInvestigate ), 0, 0, "Investigate" }
};

Task_t tlTerroristWaitCrouched[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_SET_ACTIVITY, ACT_IDLE },
	{ TASK_FACE_ENEMY, 0 },
	{ TASK_WAIT, 0.5f }
};

Schedule_t slTerroristWaitCrouched[] =
{
	{ tlTerroristWaitCrouched, ARRAYSIZE( tlTerroristWaitCrouched ), 0, 0, "Wait Crouched" }
};

Task_t tlTerroristWander[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_SET_FAIL_SCHEDULE, SCHED_IDLE_STAND },
	{ TASK_WANDER, 0 },
	{ TASK_WAIT_FOR_MOVEMENT, 0 }
};

Schedule_t slTerroristWander[] =
{
	{ tlTerroristWander, ARRAYSIZE( tlTerroristWander ),
		bits_COND_SPECIAL1 | bits_COND_NEW_ENEMY | bits_COND_HEAR_SOUND | bits_COND_PROVOKED |
		bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE, bits_SOUND_DANGER, "Wander" }
};

Schedule_t* CTerrorist::m_scheduleList[] =
{
	slTerroristFail,
	slTerroristCombatFail,
	slTerroristVictoryDance,
	slTerroristEstablishLineOfFire,
	slTerroristFoundEnemy,
	slTerroristCombatFace,
	slTerroristSignalSuppress,
	slTerroristSuppress,
	slTerroristWaitInCover,
	slTerroristTakeCover,
	slTerroristGrenadeCover,
	slTerroristTossGrenadeCover,
	slTerroristTakeCoverFromBestSound,
	slTerroristHideReload,
	slTerroristSweep,
	slTerroristRangeAttack1A,
	slTerroristRangeAttack1B,
	slTerroristRangeAttack1C,
	slTerroristRangeAttack2,
	slTerroristRepel,
	slTerroristRepelAttack,
	slTerroristRepelLand,
	slTerroristInSmoke,
	slTerroristFlashed,
	slTerroristKamakaziBlowup,
	slTerroristSniperAttack,
	slTerroristSniperTakeShot,
	slTerroristDuckCoverNode,
	slTerroristDuckNodeAttack,
	slTerroristCloseIn,
	slTerroristRangeAttackMG1,
	slTerroristRangeAttackMG2,
	slTerroristCornerNodeAttack,
	slTerroristCornerCoverNode,
	slTerroristFiringPause,
	slTerroristAlertSpeak,
	slTerroristInvestigate,
	slTerroristWaitCrouched,
	slTerroristWander
};
