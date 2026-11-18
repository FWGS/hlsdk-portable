#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "schedule.h"
#include "squadmonster.h"
#include "talkmonster.h"
#include "weapons.h"
#include "effects.h"
#include "player.h"
#include "triggers.h"
#include "nodes.h"
#include "soundent.h"
#include "animation.h"
#include "skill.h"
#include "customentity.h"

enum
{
	SCHED_CT_SUPPRESS = LAST_COMMON_SCHEDULE + 1,
	SCHED_CT_ESTABLISH_LINE_OF_FIRE,
	SCHED_CT_COVER_AND_RELOAD,
	SCHED_CT_SWEEP,
	SCHED_CT_FOUND_ENEMY,
	SCHED_CT_REPEL,
	SCHED_CT_REPEL_ATTACK,
	SCHED_CT_REPEL_LAND,
	SCHED_CT_WAIT_FACE_ENEMY,
	SCHED_CT_TAKECOVER_FAILED,
	SCHED_CT_ELOF_FAIL,
	SCHED_CT_IN_SMOKE,
	SCHED_CT_FLASHED,
	SCHED_CT_SNIPER_ATTACK,
	SCHED_CT_SNIPER_TAKESHOT,
	SCHED_CT_SPECIAL_COVER_NODE,
	SCHED_CT_DUCK_NODE_ATTACK,
	SCHED_CT_CLOSEIN,
	SCHED_TAKE_COVER_LIGHT_DAMAGE,
	SCHED_TAKE_COVER_HEAVY_DAMAGE,
	SCHED_CT_CORNER_NODE_ATTACK,
	SCHED_CT_FIRE_PAUSE,
	SCHED_CT_CANT_FOLLOW,
	SCHED_CT_WAIT_CROUCHED,
	LAST_CUSTOM_SCHEDULE
};

enum
{
	TASK_CT_FACE_TOSS_DIR = LAST_COMMON_TASK + 1,
	TASK_CT_SPEAK_SENTENCE,
	TASK_CT_CHECK_FIRE,
	TASK_MOVE_TO_SPECIAL_COVER_NODE,
	TASK_MOVE_TO_PREFERED_RANGE,
	TASK_WANDER
};

class CCounterTerrorist : public CSquadMonster
{
public:
	CCounterTerrorist();
	void Spawn() override;
	void Precache() override;
	void KeyValue(KeyValueData* data) override;
	int Save(CSave& save) override;
	int Restore(CRestore& restore) override;
	int ObjectCaps() override;
	int Classify() override;
	int ISoundMask() override;
	void Touch(CBaseEntity* other) override;
	int LookupActivity(int activity) override;
	int LookupActivityHeaviest(int activity) override;
	void HandleAnimEvent(MonsterEvent_t* event) override;
	int GetWeaponEffectSchedule() override;
	BOOL FCanCheckAttacks() override;
	BOOL CheckMeleeAttack1(float dot, float distance) override;
	BOOL CheckRangeAttack1(float dot, float distance) override;
	BOOL CheckRangeAttack2(float dot, float distance) override;
	void CheckAmmo() override;
	void SetActivity(Activity activity) override;
	void SetYawSpeed() override;
	void StartTask(Task_t* task) override;
	void RunTask(Task_t* task) override;
	Schedule_t* GetSchedule() override;
	Schedule_t* GetScheduleOfType(int type) override;
	Schedule_t* ScheduleFromName(const char* name) override;
	void DeathSound() override;
	void PainSound() override;
	void IdleSound() override;
	Vector GetGunPosition() override;
	void PrescheduleThink() override;
	void GibMonster() override;
	void StopFollowing(BOOL clearSchedule) override;
	void PlayScriptedSentence(const char* sentence, float duration, float volume, float attenuation,
		BOOL concurrent, CBaseEntity* listener) override;
	int CanPlaySequence(BOOL disregardState, int interruptLevel) override;
	void TraceAttack(entvars_t* attacker, float damage, Vector direction, TraceResult* trace, int damageType) override;
	int TakeDamage(entvars_t* inflictor, entvars_t* attacker, float damage, int damageType) override;
	virtual void SpawnInit();
	virtual BOOL IsPlayerHidden();

	void EXPORT FollowerUse(CBaseEntity* activator, CBaseEntity* caller, USE_TYPE useType, float value);
	void EXPORT UseTarget(CBaseEntity* activator, CBaseEntity* caller, USE_TYPE useType, float value);
	BOOL CanFollow();
	void StartFollowing(CBaseEntity* leader);
	void LimitFollowers(CBaseEntity* leader, int maxFollowers);
	Schedule_t* HandleFollowing();
	Schedule_t* HandleIdleHeadWatch();
	void HandleSpeaking(int sentence);
	void IdleHeadTurn(Vector& friendPosition);
	void Talk(float duration);
	BOOL IsTalking();
	void ResetHeadWatch();
	void JustSpoke(float nextSpeakTime = -1);
	BOOL FOkToSpeak();
	void SpeakSentence();
	CBaseEntity* Kick();
	void ShootSMG();
	void ShootAssaultRifle();
	void ShootMP5();
	void ShootMachineGun();
	void ShootPistol();
	void ShootShotgun();
	void ShootSniperRifle();
	void ShootLAW();
	void SetupWeapons();
	void SetupLaserGlow();
	void LaserGlowOn();
	void LaserGlowOff();
	void SetupBehaviors(int type);
	int SetActivityPistolBase(Activity activity);
	int SetActivitySniperBase(Activity activity);
	int SetActivityShotgunBase(Activity activity);
	int SetActivityGrenadeBase(Activity activity);
	int SetActivityAutomaticBase(Activity activity);
	int SetActivityLAWBase(Activity activity);
	int GetFlyingSchedule();
	int GetHearingSchedule();
	int GetForcedTargetSchedule();
	void InitScheduleTable();
	void ClearScheduleTableFlags();
	bool ValidateActivation(int entry);
	void ActivateTableEntry(int entry);
	int GetScheduleFromTable();
	int GetBehaviorType();
	static Schedule_t* m_scheduleList[];
	static TYPEDESCRIPTION m_SaveData[];
	static const char* pTerroristSentences[];

	ScheduleEntry scheduleTable[LAST_CUSTOM_SCHEDULE];
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
	BOOL justShotFlag;
	float m_blastRadius;
	float m_blastDamage;
	int m_ctType;
	int m_ctBehavior;
	float m_fPreferedRange;
	float taskFailCount;
	EHANDLE m_hTalkTarget;
	CSprite* m_pLaserGlow;
	float m_glowBrightness;
	CBeam* m_pBeam;
	float m_laserBrightness;
	float m_flStopTalkTime;
	float m_flStareTime;
	float m_wanderTime;
	Vector m_wanderOrigin;
	BOOL m_canWander;
	unsigned short m_usFireM60;
	unsigned short m_usFireUSP;
	unsigned short m_usFireAK47;
	unsigned short m_usFireM4A1;
	unsigned short m_usFireScout;
	unsigned short m_usFireMAC10;
	unsigned short m_usFireShotgun;
	unsigned short m_usFireMP5;
};

LINK_ENTITY_TO_CLASS(monster_ct, CCounterTerrorist);

int g_fCTQuestion;

static Task_t tlCTFail[] =
{
	{ TASK_STOP_MOVING, 0 }, { TASK_SET_ACTIVITY, ACT_IDLE }, { TASK_WAIT, 2 }, { TASK_WAIT_PVS, 0 }
};
static Task_t tlCTCombatFail[] =
{
	{ TASK_STOP_MOVING, 0 }, { TASK_SET_ACTIVITY, ACT_IDLE }, { TASK_WAIT_FACE_ENEMY, 2 }, { TASK_WAIT_PVS, 0 }
};
static Task_t tlCTVictoryDance[] =
{
	{ TASK_STOP_MOVING, 0 }, { TASK_FACE_ENEMY, 0 }, { TASK_WAIT, 1.5 },
	{ TASK_GET_PATH_TO_ENEMY_CORPSE, 0 }, { TASK_WALK_PATH, 0 }, { TASK_WAIT_FOR_MOVEMENT, 0 },
	{ TASK_FACE_ENEMY, 0 }, { TASK_PLAY_SEQUENCE, ACT_VICTORY_DANCE }
};
static Task_t tlCTEstablishLineOfFire[] =
{
	{ TASK_SET_FAIL_SCHEDULE, SCHED_CT_ELOF_FAIL }, { TASK_GET_PATH_TO_ENEMY, 0 },
	{ TASK_CT_SPEAK_SENTENCE, 0 }, { TASK_RUN_PATH, 0 }, { TASK_WAIT_FOR_MOVEMENT, 0 }
};
static Task_t tlCTFoundEnemy[] =
{
	{ TASK_STOP_MOVING, 0 }, { TASK_SET_FAIL_SCHEDULE, SCHED_IDLE_STAND },
	{ TASK_FACE_ENEMY, 0 }, { TASK_PLAY_SEQUENCE_FACE_ENEMY, ACT_SIGNAL1 }
};
static Task_t tlCTCombatFace1[] =
{
	{ TASK_STOP_MOVING, 0 }, { TASK_SET_ACTIVITY, ACT_IDLE }, { TASK_FACE_ENEMY, 0 },
	{ TASK_WAIT, 1.5 }, { TASK_SET_SCHEDULE, SCHED_CT_SWEEP }
};
static Task_t tlCTSignalSuppress[] =
{
	{ TASK_STOP_MOVING, 0 }, { TASK_FACE_IDEAL, 0 }, { TASK_PLAY_SEQUENCE_FACE_ENEMY, ACT_SIGNAL2 },
	{ TASK_FACE_ENEMY, 0 }, { TASK_CT_CHECK_FIRE, 0 }, { TASK_RANGE_ATTACK1, 0 },
	{ TASK_FACE_ENEMY, 0 }, { TASK_CT_CHECK_FIRE, 0 }, { TASK_RANGE_ATTACK1, 0 },
	{ TASK_FACE_ENEMY, 0 }, { TASK_CT_CHECK_FIRE, 0 }, { TASK_RANGE_ATTACK1, 0 },
	{ TASK_FACE_ENEMY, 0 }, { TASK_CT_CHECK_FIRE, 0 }, { TASK_RANGE_ATTACK1, 0 },
	{ TASK_FACE_ENEMY, 0 }, { TASK_CT_CHECK_FIRE, 0 }, { TASK_RANGE_ATTACK1, 0 }
};
static Task_t tlCTSuppress[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_FACE_ENEMY, 0 }, { TASK_CT_CHECK_FIRE, 0 }, { TASK_RANGE_ATTACK1, 0 },
	{ TASK_FACE_ENEMY, 0 }, { TASK_CT_CHECK_FIRE, 0 }, { TASK_RANGE_ATTACK1, 0 },
	{ TASK_FACE_ENEMY, 0 }, { TASK_CT_CHECK_FIRE, 0 }, { TASK_RANGE_ATTACK1, 0 },
	{ TASK_FACE_ENEMY, 0 }, { TASK_CT_CHECK_FIRE, 0 }, { TASK_RANGE_ATTACK1, 0 },
	{ TASK_FACE_ENEMY, 0 }, { TASK_CT_CHECK_FIRE, 0 }, { TASK_RANGE_ATTACK1, 0 }
};
static Task_t tlCTWaitInCover[] =
{
	{ TASK_STOP_MOVING, 0 }, { TASK_SET_ACTIVITY, ACT_IDLE }, { TASK_FACE_ENEMY, 0 }
};
static Task_t tlCTTakeCover1[] =
{
	{ TASK_STOP_MOVING, 0 }, { TASK_SET_FAIL_SCHEDULE, SCHED_CT_TAKECOVER_FAILED },
	{ TASK_FIND_COVER_FROM_ENEMY, 0 }, { TASK_RUN_PATH, 0 }, { TASK_WAIT_FOR_MOVEMENT, 0 },
	{ TASK_REMEMBER, bits_MEMORY_INCOVER }, { TASK_SET_SCHEDULE, SCHED_CT_WAIT_FACE_ENEMY }
};
static Task_t tlCTGrenadeCover1[] =
{
	{ TASK_STOP_MOVING, 0 }, { TASK_SET_FAIL_SCHEDULE, SCHED_CT_TAKECOVER_FAILED },
	{ TASK_FIND_COVER_FROM_ENEMY, 99 }, { TASK_FIND_FAR_NODE_COVER_FROM_ENEMY, 384 },
	{ TASK_PLAY_SEQUENCE, ACT_SPECIAL_ATTACK1 }, { TASK_CLEAR_MOVE_WAIT, 0 }, { TASK_RUN_PATH, 0 },
	{ TASK_WAIT_FOR_MOVEMENT, 0 }, { TASK_SET_SCHEDULE, SCHED_CT_WAIT_FACE_ENEMY }
};
static Task_t tlCTTossGrenadeCover1[] =
{
	{ TASK_FACE_ENEMY, 0 }, { TASK_RANGE_ATTACK2, 0 }, { TASK_SET_SCHEDULE, SCHED_TAKE_COVER_FROM_ENEMY }
};
static Task_t tlCTTakeCoverFromBestSound[] =
{
	{ TASK_SET_FAIL_SCHEDULE, SCHED_COWER }, { TASK_STOP_MOVING, 0 }, { TASK_FIND_COVER_FROM_BEST_SOUND, 0 },
	{ TASK_RUN_PATH, 0 }, { TASK_WAIT_FOR_MOVEMENT, 0 }, { TASK_REMEMBER, bits_MEMORY_INCOVER }, { TASK_TURN_LEFT, 179 }
};
static Task_t tlCTHideReload[] =
{
	{ TASK_STOP_MOVING, 0 }, { TASK_SET_FAIL_SCHEDULE, SCHED_RELOAD }, { TASK_FIND_COVER_FROM_ENEMY, 0 },
	{ TASK_RUN_PATH, 0 }, { TASK_WAIT_FOR_MOVEMENT, 0 }, { TASK_REMEMBER, bits_MEMORY_INCOVER },
	{ TASK_FACE_ENEMY, 0 }, { TASK_PLAY_SEQUENCE, ACT_RELOAD }
};
static Task_t tlCTSweep[] =
{
	{ TASK_TURN_LEFT, 179 }, { TASK_WAIT, 1 }, { TASK_TURN_LEFT, 179 }, { TASK_WAIT, 1 }
};
static Task_t tlCTRangeAttack1A[] =
{
	{ TASK_STOP_MOVING, 0 }, { TASK_PLAY_SEQUENCE_FACE_ENEMY, ACT_CROUCH },
	{ TASK_CT_CHECK_FIRE, 0 }, { TASK_RANGE_ATTACK1, 0 }
};
static Task_t tlCTRangeAttack1B[] =
{
	{ TASK_STOP_MOVING, 0 }, { TASK_FACE_ENEMY, 0 }, { TASK_CT_CHECK_FIRE, 0 }, { TASK_RANGE_ATTACK1, 0 }
};
static Task_t tlCTRangeAttack1C[] =
{
	{ TASK_STOP_MOVING, 0 }, { TASK_FACE_ENEMY, 0 }, { TASK_CT_CHECK_FIRE, 0 }, { TASK_RANGE_ATTACK1, 0 }
};
static Task_t tlCTRangeAttack2[] =
{
	{ TASK_STOP_MOVING, 0 }, { TASK_CT_FACE_TOSS_DIR, 0 },
	{ TASK_PLAY_SEQUENCE, ACT_RANGE_ATTACK2 }, { TASK_SET_SCHEDULE, SCHED_CT_WAIT_FACE_ENEMY }
};
static Task_t tlCTRepel[] =
{
	{ TASK_STOP_MOVING, 0 }, { TASK_FACE_IDEAL, 0 }, { TASK_PLAY_SEQUENCE, ACT_GLIDE }
};
static Task_t tlCTRepelAttack[] =
{
	{ TASK_STOP_MOVING, 0 }, { TASK_FACE_ENEMY, 0 }, { TASK_PLAY_SEQUENCE, ACT_FLY }
};
static Task_t tlCTRepelLand[] =
{
	{ TASK_STOP_MOVING, 0 }, { TASK_PLAY_SEQUENCE, ACT_LAND }, { TASK_GET_PATH_TO_LASTPOSITION, 0 },
	{ TASK_RUN_PATH, 0 }, { TASK_WAIT_FOR_MOVEMENT, 0 }, { TASK_CLEAR_LASTPOSITION, 0 }
};
static Task_t tlCTInSmokeSchedule[] =
{
	{ TASK_STOP_MOVING, 0 }, { TASK_SET_FAIL_SCHEDULE, SCHED_COWER },
	{ TASK_FACE_ENEMY, 0 }, { TASK_PLAY_SEQUENCE, ACT_LASTACTIVITY + 3 }
};
static Task_t tlCTFlashedSchedule[] =
{
	{ TASK_STOP_MOVING, 0 }, { TASK_SET_FAIL_SCHEDULE, SCHED_COWER },
	{ TASK_FACE_ENEMY, 0 }, { TASK_PLAY_SEQUENCE, ACT_LASTACTIVITY + 4 }
};
static Task_t tlCTSniperAttack[] =
{
	{ TASK_SET_FAIL_SCHEDULE, SCHED_FAIL }, { TASK_STOP_MOVING, 0 }, { TASK_FACE_ENEMY, 0 },
	{ TASK_PLAY_SEQUENCE, ACT_ARM }, { TASK_CT_CHECK_FIRE, 0 }, { TASK_RANGE_ATTACK1, 0 },
	{ TASK_FACE_ENEMY, 0 }, { TASK_PLAY_SEQUENCE, ACT_DISARM }
};
static Task_t tlCTSniperTakeShot[] =
{
	{ TASK_SET_FAIL_SCHEDULE, SCHED_FAIL }, { TASK_STOP_MOVING, 0 }, { TASK_RANGE_ATTACK1, 0 }
};
static Task_t tlCTDuckCoverNode[] =
{
	{ TASK_STOP_MOVING, 0 }, { TASK_SET_FAIL_SCHEDULE, SCHED_CT_TAKECOVER_FAILED },
	{ TASK_MOVE_TO_SPECIAL_COVER_NODE, 0 }, { TASK_WAIT_FOR_MOVEMENT, 0 },
	{ TASK_REMEMBER, bits_MEMORY_INCOVER }, { TASK_SET_SCHEDULE, SCHED_CT_DUCK_NODE_ATTACK }
};
static Task_t tlCTDuckNodeAttack[] =
{
	{ TASK_STOP_MOVING, 0 }, { TASK_PLAY_SEQUENCE_FACE_ENEMY, ACT_CROUCH }, { TASK_WAIT, 1 },
	{ TASK_PLAY_SEQUENCE_FACE_ENEMY, ACT_STAND }, { TASK_CT_CHECK_FIRE, 0 },
	{ TASK_RANGE_ATTACK1, 0 }, { TASK_WAIT, 0.5 }
};
static Task_t tlCTCloseIn[] =
{
	{ TASK_SET_FAIL_SCHEDULE, SCHED_CT_ELOF_FAIL }, { TASK_MOVE_TO_PREFERED_RANGE, 0 }
};
static Task_t tlCTRangeAttackMG1[] =
{
	{ TASK_STOP_MOVING, 0 }, { TASK_PLAY_SEQUENCE_FACE_ENEMY, ACT_CROUCH },
	{ TASK_CT_CHECK_FIRE, 0 }, { TASK_RANGE_ATTACK1, 0 }
};
static Task_t tlCTRangeAttackMG2[] =
{
	{ TASK_STOP_MOVING, 0 }, { TASK_FACE_ENEMY, 0 }, { TASK_CT_CHECK_FIRE, 0 }, { TASK_RANGE_ATTACK1, 0 }
};
static Task_t tlCTCornerNodeAttack[] =
{
	{ TASK_STOP_MOVING, 0 }, { TASK_FACE_ENEMY, 0 }, { TASK_WAIT, 0.5 },
	{ TASK_PLAY_SEQUENCE, ACT_LASTACTIVITY + 1 }, { TASK_PLAY_SEQUENCE, ACT_LASTACTIVITY + 2 },
	{ TASK_PLAY_SEQUENCE, ACT_IDLE }
};
static Task_t tlCTCornerCoverNode[] =
{
	{ TASK_STOP_MOVING, 0 }, { TASK_SET_FAIL_SCHEDULE, SCHED_CT_TAKECOVER_FAILED },
	{ TASK_MOVE_TO_SPECIAL_COVER_NODE, 0 }, { TASK_WAIT_FOR_MOVEMENT, 0 },
	{ TASK_REMEMBER, bits_MEMORY_INCOVER }, { TASK_SET_SCHEDULE, SCHED_CT_CORNER_NODE_ATTACK }
};
static Task_t tlCTFiringPause[] =
{
	{ TASK_STOP_MOVING, 0 }, { TASK_SET_ACTIVITY, ACT_IDLE }, { TASK_WAIT_RANDOM, 0.6 }
};
static Task_t tlCTMoveAwayFollow[] =
{
	{ TASK_SET_FAIL_SCHEDULE, SCHED_TARGET_FACE }, { TASK_STORE_LASTPOSITION, 0 },
	{ TASK_MOVE_AWAY_PATH, 100 }, { TASK_WALK_PATH_FOR_UNITS, 100 },
	{ TASK_STOP_MOVING, 0 }, { TASK_SET_SCHEDULE, SCHED_TARGET_FACE }
};
static Task_t tlCTFollow[] =
{
	{ TASK_SET_FAIL_SCHEDULE, SCHED_CT_CANT_FOLLOW }, { TASK_MOVE_TO_TARGET_RANGE, 64 }
};
static Task_t tlCTStopFollowing[] =
{
	{ TASK_CANT_FOLLOW, 0 }
};
static Task_t tlCTFaceTarget[] =
{
	{ TASK_SET_ACTIVITY, ACT_IDLE }, { TASK_FACE_TARGET, 0 }, { TASK_SET_ACTIVITY, ACT_IDLE },
	{ TASK_SET_SCHEDULE, SCHED_TARGET_CHASE }
};
static Task_t tlCTWaitCrouched[] =
{
	{ TASK_STOP_MOVING, 0 }, { TASK_SET_ACTIVITY, ACT_IDLE }, { TASK_FACE_ENEMY, 0 }, { TASK_WAIT, 0.5 }
};
static Task_t tlCTIdleEyecontact[] =
{
	{ TASK_TLK_IDEALYAW, 0 }, { TASK_FACE_IDEAL, 0 }, { TASK_TLK_EYECONTACT, 0 }
};
static Task_t tlCTWander[] =
{
	{ TASK_STOP_MOVING, 0 }, { TASK_SET_FAIL_SCHEDULE, SCHED_IDLE_STAND },
	{ TASK_WANDER, 0 }, { TASK_WAIT_FOR_MOVEMENT, 0 }
};

static Schedule_t slCTFail[] =
{
	{ tlCTFail, ARRAYSIZE(tlCTFail), bits_COND_CAN_ATTACK, 0, "CT Fail" }
};
static Schedule_t slCTCombatFail[] =
{
	{ tlCTCombatFail, ARRAYSIZE(tlCTCombatFail), bits_COND_CAN_RANGE_ATTACK1 | bits_COND_CAN_RANGE_ATTACK2, 0, "CT Combat Fail" }
};
static Schedule_t slCTVictoryDance[] =
{
	{ tlCTVictoryDance, ARRAYSIZE(tlCTVictoryDance), bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE | bits_COND_NEW_ENEMY, 0, "CTVictoryDance" }
};
static Schedule_t slCTEstablishLineOfFire[] =
{
	{ tlCTEstablishLineOfFire, ARRAYSIZE(tlCTEstablishLineOfFire), bits_COND_CAN_ATTACK | bits_COND_NEW_ENEMY | bits_COND_HEAR_SOUND | bits_COND_ENEMY_DEAD, bits_SOUND_DANGER, "CTEstablishLineOfFire" }
};
static Schedule_t slCTFoundEnemy[] =
{
	{ tlCTFoundEnemy, ARRAYSIZE(tlCTFoundEnemy), bits_COND_HEAR_SOUND, bits_SOUND_DANGER, "CTFoundEnemy" }
};
static Schedule_t slCTCombatFace[] =
{
	{ tlCTCombatFace1, ARRAYSIZE(tlCTCombatFace1), bits_COND_CAN_RANGE_ATTACK1 | bits_COND_CAN_RANGE_ATTACK2 | bits_COND_NEW_ENEMY | bits_COND_ENEMY_DEAD, 0, "Combat Face" }
};
static Schedule_t slCTSignalSuppress[] =
{
	{ tlCTSignalSuppress, ARRAYSIZE(tlCTSignalSuppress), bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE | bits_COND_HEAR_SOUND | bits_COND_ENEMY_DEAD, bits_SOUND_DANGER, "SignalSuppress" }
};
static Schedule_t slCTSuppress[] =
{
	{ tlCTSuppress, ARRAYSIZE(tlCTSuppress), bits_COND_NO_AMMO_LOADED | bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE | bits_COND_HEAR_SOUND | bits_COND_ENEMY_DEAD | bits_COND_SPECIAL1, bits_SOUND_DANGER, "Suppress" }
};
static Schedule_t slCTWaitInCover[] =
{
	{ tlCTWaitInCover, ARRAYSIZE(tlCTWaitInCover), bits_COND_CAN_ATTACK | bits_COND_NEW_ENEMY | bits_COND_HEAR_SOUND, bits_SOUND_DANGER, "CTWaitInCover" }
};
static Schedule_t slCTTakeCover[] =
{
	{ tlCTTakeCover1, ARRAYSIZE(tlCTTakeCover1), 0, 0, "TakeCover" }
};
static Schedule_t slCTGrenadeCover[] =
{
	{ tlCTGrenadeCover1, ARRAYSIZE(tlCTGrenadeCover1), 0, 0, "GrenadeCover" }
};
static Schedule_t slCTTossGrenadeCover[] =
{
	{ tlCTTossGrenadeCover1, ARRAYSIZE(tlCTTossGrenadeCover1), 0, 0, "TossGrenadeCover" }
};
static Schedule_t slCTTakeCoverFromBestSound[] =
{
	{ tlCTTakeCoverFromBestSound, ARRAYSIZE(tlCTTakeCoverFromBestSound), 0, 0, "CTTakeCoverFromBestSound" }
};
static Schedule_t slCTHideReload[] =
{
	{ tlCTHideReload, ARRAYSIZE(tlCTHideReload), bits_COND_HEAVY_DAMAGE | bits_COND_HEAR_SOUND, bits_SOUND_DANGER, "CTHideReload" }
};
static Schedule_t slCTSweep[] =
{
	{ tlCTSweep, ARRAYSIZE(tlCTSweep), bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE | bits_COND_CAN_RANGE_ATTACK1 | bits_COND_CAN_RANGE_ATTACK2 | bits_COND_NEW_ENEMY | bits_COND_HEAR_SOUND, bits_SOUND_WORLD | bits_SOUND_DANGER | bits_SOUND_PLAYER, "CT Sweep" }
};
static Schedule_t slCTRangeAttack1A[] =
{
	{ tlCTRangeAttack1A, ARRAYSIZE(tlCTRangeAttack1A), bits_COND_NO_AMMO_LOADED | bits_COND_ENEMY_OCCLUDED | bits_COND_HEAVY_DAMAGE | bits_COND_NEW_ENEMY | bits_COND_HEAR_SOUND | bits_COND_ENEMY_DEAD | bits_COND_SPECIAL1, bits_SOUND_DANGER, "Range Attack1A" }
};
static Schedule_t slCTRangeAttack1B[] =
{
	{ tlCTRangeAttack1B, ARRAYSIZE(tlCTRangeAttack1B), bits_COND_NO_AMMO_LOADED | bits_COND_ENEMY_OCCLUDED | bits_COND_HEAVY_DAMAGE | bits_COND_NEW_ENEMY | bits_COND_HEAR_SOUND | bits_COND_ENEMY_DEAD | bits_COND_SPECIAL1, bits_SOUND_DANGER, "Range Attack1B" }
};
static Schedule_t slCTRangeAttack1C[] =
{
	{ tlCTRangeAttack1C, ARRAYSIZE(tlCTRangeAttack1C), bits_COND_NO_AMMO_LOADED | bits_COND_ENEMY_OCCLUDED | bits_COND_HEAVY_DAMAGE | bits_COND_NEW_ENEMY | bits_COND_HEAR_SOUND | bits_COND_ENEMY_DEAD | bits_COND_SPECIAL1, bits_SOUND_DANGER, "Range Attack1C" }
};
static Schedule_t slCTRangeAttack2[] =
{
	{ tlCTRangeAttack2, ARRAYSIZE(tlCTRangeAttack2), 0, 0, "RangeAttack2" }
};
static Schedule_t slCTRepel[] =
{
	{ tlCTRepel, ARRAYSIZE(tlCTRepel), bits_COND_SEE_ENEMY | bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE | bits_COND_NEW_ENEMY | bits_COND_HEAR_SOUND, bits_SOUND_COMBAT | bits_SOUND_PLAYER | bits_SOUND_DANGER, "Repel" }
};
static Schedule_t slCTRepelAttack[] =
{
	{ tlCTRepelAttack, ARRAYSIZE(tlCTRepelAttack), bits_COND_ENEMY_OCCLUDED, 0, "Repel Attack" }
};
static Schedule_t slCTRepelLand[] =
{
	{ tlCTRepelLand, ARRAYSIZE(tlCTRepelLand), bits_COND_SEE_ENEMY | bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE | bits_COND_NEW_ENEMY | bits_COND_HEAR_SOUND, bits_SOUND_COMBAT | bits_SOUND_PLAYER | bits_SOUND_DANGER, "Repel Land" }
};
static Schedule_t slCTInSmoke[] =
{
	{ tlCTInSmokeSchedule, ARRAYSIZE(tlCTInSmokeSchedule), bits_COND_HEAVY_DAMAGE, 0, "CTInSmoke" }
};
static Schedule_t slCTFlashed[] =
{
	{ tlCTFlashedSchedule, ARRAYSIZE(tlCTFlashedSchedule), bits_COND_HEAVY_DAMAGE, 0, "CTFlashed" }
};
static Schedule_t slCTSniperAttack[] =
{
	{ tlCTSniperAttack, ARRAYSIZE(tlCTSniperAttack), bits_COND_ENEMY_OCCLUDED | bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE | bits_COND_NEW_ENEMY | bits_COND_ENEMY_DEAD, 0, "Sniper Attack" }
};
static Schedule_t slCTSniperTakeShot[] =
{
	{ tlCTSniperTakeShot, ARRAYSIZE(tlCTSniperTakeShot), bits_COND_NEW_ENEMY | bits_COND_ENEMY_DEAD, 0, "Sniper Take Shot" }
};
static Schedule_t slCTDuckCoverNode[] =
{
	{ tlCTDuckCoverNode, ARRAYSIZE(tlCTDuckCoverNode), 0, 0, "DuckCoverNode" }
};
static Schedule_t slCTDuckNodeAttack[] =
{
	{ tlCTDuckNodeAttack, ARRAYSIZE(tlCTDuckNodeAttack), bits_COND_NO_AMMO_LOADED | bits_COND_HEAVY_DAMAGE | bits_COND_NEW_ENEMY | bits_COND_HEAR_SOUND | bits_COND_ENEMY_DEAD | bits_COND_SPECIAL1, bits_SOUND_DANGER, "DuckNodeAttack" }
};
static Schedule_t slCTCloseIn[] =
{
	{ tlCTCloseIn, ARRAYSIZE(tlCTCloseIn), bits_COND_CAN_MELEE_ATTACK1 | bits_COND_NEW_ENEMY | bits_COND_HEAR_SOUND | bits_COND_ENEMY_DEAD, 0, "CloseIn" }
};
static Schedule_t slCTRangeAttackMG1[] =
{
	{ tlCTRangeAttackMG1, ARRAYSIZE(tlCTRangeAttackMG1), bits_COND_ENEMY_OCCLUDED | bits_COND_HEAVY_DAMAGE | bits_COND_NEW_ENEMY | bits_COND_HEAR_SOUND | bits_COND_ENEMY_DEAD | bits_COND_SPECIAL1, bits_SOUND_DANGER, "MachineGun Attack1" }
};
static Schedule_t slCTRangeAttackMG2[] =
{
	{ tlCTRangeAttackMG2, ARRAYSIZE(tlCTRangeAttackMG2), bits_COND_ENEMY_OCCLUDED | bits_COND_HEAVY_DAMAGE | bits_COND_NEW_ENEMY | bits_COND_HEAR_SOUND | bits_COND_ENEMY_DEAD | bits_COND_SPECIAL1, bits_SOUND_DANGER, "MachineGun Attack2" }
};
static Schedule_t slCTCornerNodeAttack[] =
{
	{ tlCTCornerNodeAttack, ARRAYSIZE(tlCTCornerNodeAttack), bits_COND_NO_AMMO_LOADED | bits_COND_HEAVY_DAMAGE | bits_COND_NEW_ENEMY | bits_COND_HEAR_SOUND | bits_COND_ENEMY_DEAD | bits_COND_SPECIAL1, bits_SOUND_DANGER, "CornerNodeAttack" }
};
static Schedule_t slCTCornerCoverNode[] =
{
	{ tlCTCornerCoverNode, ARRAYSIZE(tlCTCornerCoverNode), 0, 0, "CornerCoverNode" }
};
static Schedule_t slCTFiringPause[] =
{
	{ tlCTFiringPause, ARRAYSIZE(tlCTFiringPause), bits_COND_NO_AMMO_LOADED | bits_COND_ENEMY_OCCLUDED | bits_COND_ENEMY_DEAD | bits_COND_SPECIAL1, 0, "Fire Pause" }
};
static Schedule_t slCTMoveAwayFollow[] =
{
	{ tlCTMoveAwayFollow, ARRAYSIZE(tlCTMoveAwayFollow), 0, 0, "MoveAwayFollow" }
};
static Schedule_t slCTFollow[] =
{
	{ tlCTFollow, ARRAYSIZE(tlCTFollow), bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE | bits_COND_NEW_ENEMY | bits_COND_HEAR_SOUND, bits_SOUND_COMBAT | bits_SOUND_DANGER, "Follow" }
};
static Schedule_t slCTStopFollowing[] =
{
	{ tlCTStopFollowing, ARRAYSIZE(tlCTStopFollowing), 0, 0, "StopFollowing" }
};
static Schedule_t slCTFaceTarget[] =
{
	{ tlCTFaceTarget, ARRAYSIZE(tlCTFaceTarget), bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE | bits_COND_PROVOKED | bits_COND_NEW_ENEMY | bits_COND_HEAR_SOUND | bits_COND_SPECIAL1, bits_SOUND_DANGER, "FaceTarget" }
};
static Schedule_t slCTWaitCrouched[] =
{
	{ tlCTWaitCrouched, ARRAYSIZE(tlCTWaitCrouched), 0, 0, "Wait Crouched" }
};
static Schedule_t slCTIdleEyecontact[] =
{
	{ tlCTIdleEyecontact, ARRAYSIZE(tlCTIdleEyecontact), bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE | bits_COND_NEW_ENEMY | bits_COND_SPECIAL1, 0, "CTIdleEyecontact" }
};
static Schedule_t slCTWander[] =
{
	{ tlCTWander, ARRAYSIZE(tlCTWander), bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE | bits_COND_PROVOKED | bits_COND_NEW_ENEMY | bits_COND_HEAR_SOUND | bits_COND_SPECIAL1, bits_SOUND_DANGER, "Wander" }
};

DEFINE_CUSTOM_SCHEDULES(CCounterTerrorist)
{
	slCTFail, slCTCombatFail, slCTVictoryDance, slCTEstablishLineOfFire, slCTFoundEnemy,
	slCTCombatFace, slCTSignalSuppress, slCTSuppress, slCTWaitInCover, slCTTakeCover,
	slCTGrenadeCover, slCTTossGrenadeCover, slCTTakeCoverFromBestSound, slCTHideReload, slCTSweep,
	slCTRangeAttack1A, slCTRangeAttack1B, slCTRangeAttack1C, slCTRangeAttack2, slCTRepel,
	slCTRepelAttack, slCTRepelLand, slCTInSmoke, slCTFlashed, slCTSniperAttack, slCTSniperTakeShot,
	slCTDuckCoverNode, slCTDuckNodeAttack, slCTCloseIn, slCTRangeAttackMG1, slCTRangeAttackMG2,
	slCTCornerNodeAttack, slCTCornerCoverNode, slCTFiringPause, slCTMoveAwayFollow, slCTFollow,
	slCTStopFollowing, slCTFaceTarget, slCTWaitCrouched, slCTIdleEyecontact, slCTWander
};

IMPLEMENT_CUSTOM_SCHEDULES(CCounterTerrorist, CBaseMonster);

const char* CCounterTerrorist::pTerroristSentences[] =
{
	"CT_GREN", "CT_ENEMY", "CT_THROW", "CT_TAUNT", "CT_IDLE",
	"CT_QUEST", "CT_ANSWER", "CT_CLEAR", "CT_CHECK", "CT_ALLY"
};

TYPEDESCRIPTION CCounterTerrorist::m_SaveData[] =
{
	DEFINE_FIELD(CCounterTerrorist, m_flNextGrenadeCheck, FIELD_TIME),
	DEFINE_FIELD(CCounterTerrorist, m_flNextPainTime, FIELD_TIME),
	DEFINE_FIELD(CCounterTerrorist, m_vecTossVelocity, FIELD_VECTOR),
	DEFINE_FIELD(CCounterTerrorist, m_fThrowGrenade, FIELD_BOOLEAN),
	DEFINE_FIELD(CCounterTerrorist, m_bStanding, FIELD_BOOLEAN),
	DEFINE_FIELD(CCounterTerrorist, m_fFirstEncounter, FIELD_BOOLEAN),
	DEFINE_FIELD(CCounterTerrorist, m_cClipSize, FIELD_INTEGER),
	DEFINE_FIELD(CCounterTerrorist, m_voicePitch, FIELD_INTEGER),
	DEFINE_FIELD(CCounterTerrorist, m_iSentence, FIELD_INTEGER),
	DEFINE_FIELD(CCounterTerrorist, m_blastRadius, FIELD_FLOAT),
	DEFINE_FIELD(CCounterTerrorist, m_blastDamage, FIELD_FLOAT),
	DEFINE_FIELD(CCounterTerrorist, m_ctType, FIELD_INTEGER),
	DEFINE_FIELD(CCounterTerrorist, m_ctBehavior, FIELD_INTEGER),
	DEFINE_ARRAY(CCounterTerrorist, scheduleTable, FIELD_VECTOR, LAST_CUSTOM_SCHEDULE),
	DEFINE_FIELD(CCounterTerrorist, m_flLastEnemySightTime, FIELD_TIME),
	DEFINE_FIELD(CCounterTerrorist, m_iBrassShell, FIELD_INTEGER),
	DEFINE_FIELD(CCounterTerrorist, m_iShotgunShell, FIELD_INTEGER),
	DEFINE_FIELD(CCounterTerrorist, justShotFlag, FIELD_BOOLEAN),
	DEFINE_FIELD(CCounterTerrorist, m_fPreferedRange, FIELD_FLOAT),
	DEFINE_FIELD(CCounterTerrorist, m_pLaserGlow, FIELD_CLASSPTR),
	DEFINE_FIELD(CCounterTerrorist, m_glowBrightness, FIELD_FLOAT),
	DEFINE_FIELD(CCounterTerrorist, m_pBeam, FIELD_CLASSPTR),
	DEFINE_FIELD(CCounterTerrorist, m_laserBrightness, FIELD_FLOAT),
	DEFINE_FIELD(CCounterTerrorist, m_flStopTalkTime, FIELD_TIME),
	DEFINE_FIELD(CCounterTerrorist, m_flStareTime, FIELD_TIME),
	DEFINE_FIELD(CCounterTerrorist, m_wanderTime, FIELD_TIME),
	DEFINE_FIELD(CCounterTerrorist, m_wanderOrigin, FIELD_VECTOR),
	DEFINE_FIELD(CCounterTerrorist, m_canWander, FIELD_BOOLEAN),
};

IMPLEMENT_SAVERESTORE(CCounterTerrorist, CSquadMonster);

#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
CCounterTerrorist::CCounterTerrorist()
{
	m_ctType = 0;
	m_ctBehavior = -1;
	m_canWander = FALSE;
	m_bFollowStuck = FALSE;
	justShotFlag = FALSE;
	m_fPreferedRange = 64;
	taskFailCount = 0;
	m_flStareTime = 0;
	m_classtype = 16;
	m_wanderTime = gpGlobals->time + RANDOM_FLOAT(5, 10);
}

int CCounterTerrorist::Classify()
{
	return m_classtype;
}

int CCounterTerrorist::ObjectCaps()
{
	return FCAP_ACROSS_TRANSITION | FCAP_IMPULSE_USE;
}

void CCounterTerrorist::Touch(CBaseEntity* other)
{
	m_wanderTime = 0;
}

int CCounterTerrorist::ISoundMask()
{
	return bits_SOUND_WORLD | bits_SOUND_COMBAT | bits_SOUND_DANGER;
}

BOOL CCounterTerrorist::FCanCheckAttacks()
{
	return !HasConditions(bits_COND_ENEMY_TOOFAR);
}

void CCounterTerrorist::CheckAmmo()
{
	if (m_cAmmoLoaded <= 0)
	{
		SetConditions(bits_COND_NO_AMMO_LOADED);
	}
}

Vector CCounterTerrorist::GetGunPosition()
{
	if (m_bStanding)
		return pev->origin + Vector(0, 0, 60);
	else
		return pev->origin + Vector(0, 0, 36);
}

void CCounterTerrorist::SetYawSpeed()
{
	switch (m_Activity)
	{
	case ACT_IDLE:
	case ACT_RUN:
		pev->yaw_speed = 150;
		break;
	case ACT_WALK:
	case ACT_TURN_LEFT:
	case ACT_TURN_RIGHT:
		pev->yaw_speed = 180;
		break;
	case ACT_FLY:
	case ACT_GLIDE:
		pev->yaw_speed = 30;
		break;
	case ACT_RANGE_ATTACK1:
	case ACT_RANGE_ATTACK2:
	case ACT_MELEE_ATTACK1:
	case ACT_MELEE_ATTACK2:
		pev->yaw_speed = 120;
		break;
	default:
		pev->yaw_speed = 90;
		break;
	}
}

void CCounterTerrorist::KeyValue(KeyValueData* data)
{
	if (FStrEq(data->szKeyName, "behavior"))
	{
		m_behaviorType = atoi(data->szValue);
		data->fHandled = TRUE;
	}
	if (FStrEq(data->szKeyName, "wander"))
	{
		m_canWander = atoi(data->szValue) != 0;
		data->fHandled = TRUE;
	}
	else
	{
		CBaseMonster::KeyValue(data);
	}
}

void CCounterTerrorist::SpawnInit()
{
	SET_MODEL(edict(), pev->model ? STRING(pev->model) : "models/hgrunt.mdl");
}

int CCounterTerrorist::CanPlaySequence(BOOL disregardState, int interruptLevel)
{
	if (m_ctBehavior == 4)
	{
		UTIL_Remove(m_pBeam);
		m_pBeam = NULL;
		UTIL_Remove(m_pLaserGlow);
		m_pLaserGlow = NULL;
	}
	return CBaseMonster::CanPlaySequence(disregardState, interruptLevel);
}

BOOL CCounterTerrorist::IsPlayerHidden()
{
	if (m_MonsterState == MONSTERSTATE_ALERT || m_MonsterState == MONSTERSTATE_COMBAT)
	{
		return FALSE;
	}
	CBasePlayer* player = (CBasePlayer*)CBaseEntity::Instance(INDEXENT(1));
	if (!player->m_stealthTrigger)
	{
		return FALSE;
	}
	BOOL hidden = player->m_stealthTrigger->IsPlayerHidden();
	if (!hidden)
	{
		player->m_stealthTrigger = NULL;
	}
	return hidden;
}

BOOL CCounterTerrorist::CheckMeleeAttack1(float dot, float distance)
{
	if (m_hEnemy && !m_hEnemy->MyMonsterPointer())
	{
		return FALSE;
	}
	return distance <= 64 && dot >= 0.7;
}

BOOL CCounterTerrorist::CheckRangeAttack1(float dot, float distance)
{
	if (HasConditions(bits_COND_ENEMY_OCCLUDED) || !(distance <= 2048) || !(dot >= 0.5) || !NoFriendlyFire())
	{
		return FALSE;
	}
	Vector source = GetGunPosition();
	Vector target = m_hEnemy->BodyTarget(source);
	TraceResult trace;
	UTIL_TraceLine(source, target, ignore_monsters, ignore_glass, edict(), &trace);
	return trace.flFraction == 1;
}

void CCounterTerrorist::TraceAttack(entvars_t* attacker, float damage, Vector direction, TraceResult* trace, int damageType)
{
	if (trace->iHitgroup == 11)
	{
		if (GetBodygroup(1) == 0 && (damageType & (DMG_BULLET | DMG_SLASH | DMG_BLAST | DMG_CLUB)))
		{
			damage -= 20;
			if (damage <= 0)
			{
				UTIL_Ricochet(trace->vecEndPos, 1);
				damage = 0.01;
			}
		}
		trace->iHitgroup = HITGROUP_HEAD;
	}
	CBaseMonster::TraceAttack(attacker, damage, direction, trace, damageType);
}

void CCounterTerrorist::Talk(float duration)
{
	m_flStopTalkTime = gpGlobals->time + duration;
}

BOOL CCounterTerrorist::IsTalking()
{
	return m_flStopTalkTime > gpGlobals->time;
}

void CCounterTerrorist::ResetHeadWatch()
{
	SetBoneController(0, 0);
	m_hTalkTarget = NULL;
}

void CCounterTerrorist::JustSpoke(float nextSpeakTime)
{
	if (nextSpeakTime == -1)
	{
		CTalkMonster::g_talkWaitTime = gpGlobals->time + RANDOM_FLOAT(1.5, 2);
	}
	else
	{
		CTalkMonster::g_talkWaitTime = gpGlobals->time + nextSpeakTime;
	}
	m_iSentence = -1;
}

BOOL CCounterTerrorist::FOkToSpeak()
{
	if (CTalkMonster::g_talkWaitTime >= gpGlobals->time)
	{
		return FALSE;
	}
	CBasePlayer* player = (CBasePlayer*)CBaseEntity::Instance(INDEXENT(1));
	if (gpGlobals->time > 2 && player && player->m_chatterTime > gpGlobals->time)
	{
		return FALSE;
	}
	return !(pev->spawnflags & SF_MONSTER_GAG) || m_MonsterState == MONSTERSTATE_COMBAT;
}

void CCounterTerrorist::SpeakSentence()
{
	if (m_iSentence != -1 && FOkToSpeak())
	{
		SayNamedSentence(pTerroristSentences[m_iSentence], 0.8, ATTN_NORM, 0, m_voicePitch);
		JustSpoke();
	}
}

void CCounterTerrorist::PainSound()
{
	if (gpGlobals->time > m_flNextPainTime)
	{
		SayNamedSentence("CT_WOUND", 0.8, ATTN_NORM, 0, m_voicePitch);
		m_flNextPainTime = gpGlobals->time + 1;
	}
}

void CCounterTerrorist::DeathSound()
{
	if (!FOkToSpeak())
	{
		EMIT_SOUND_DYN(edict(), CHAN_VOICE, "common/null.wav", 0.5, ATTN_NORM, 0, m_voicePitch);
	}
	SayNamedSentence("CT_DEATH", 0.8, ATTN_NORM, 0, m_voicePitch);
	SetUse(NULL);
}

void CCounterTerrorist::PlayScriptedSentence(const char* sentence, float duration, float volume, float attenuation,
	BOOL concurrent, CBaseEntity* listener)
{
	if (IsAlive() && pev->deadflag != DEAD_DYING)
	{
		JustSpoke(duration + 2);
		PlaySentence(sentence, duration, volume, attenuation);
		CBasePlayer* player = (CBasePlayer*)CBaseEntity::Instance(INDEXENT(1));
		if (player)
		{
			player->m_chatterTime = gpGlobals->time + duration;
		}
		m_hTalkTarget = listener;
		if (m_hTalkTarget && (m_afCapability & bits_CAP_TURN_HEAD))
		{
			IdleHeadTurn(m_hTalkTarget->pev->origin);
		}
		Talk(duration);
	}
}

void CCounterTerrorist::InitScheduleTable()
{
	for (int i = 0; i < LAST_CUSTOM_SCHEDULE; i++)
	{
		scheduleTable[i].activeFlag = 0;
		scheduleTable[i].percentChance = 1;
	}
}

void CCounterTerrorist::ClearScheduleTableFlags()
{
	for (int i = 0; i < LAST_CUSTOM_SCHEDULE; i++)
	{
		scheduleTable[i].activeFlag = 0;
	}
}

bool CCounterTerrorist::ValidateActivation(int entry)
{
	if (entry == SCHED_CT_ESTABLISH_LINE_OF_FIRE || entry == SCHED_TAKE_COVER_FROM_ENEMY ||
		entry == SCHED_CT_SPECIAL_COVER_NODE || entry == SCHED_CT_CLOSEIN)
	{
		return m_cantMove == 0;
	}
	return true;
}

void CCounterTerrorist::ActivateTableEntry(int entry)
{
	if ( ValidateActivation( entry ) && (unsigned int)entry < ARRAYSIZE( scheduleTable ) )
	{
		scheduleTable[entry].activeFlag = 1;
	}
}

void CCounterTerrorist::Precache()
{
	PRECACHE_SOUND("weapons/reload_shotgun.wav");
	PRECACHE_SOUND("weapons/pistol_reload.wav");
	PRECACHE_SOUND("weapons/reload_mp5.wav");
	PRECACHE_SOUND("weapons/glauncher.wav");
	PRECACHE_SOUND("weapons/sbarrel1.wav");
	PRECACHE_SOUND("weapons/knife_slash1.wav");
	m_voicePitch = 100;
	m_iBrassShell = PRECACHE_MODEL("models/shell.mdl");
	m_iShotgunShell = PRECACHE_MODEL("models/shotgunshell.mdl");
	PRECACHE_MODEL("sprites/laserbeam.spr");
	PRECACHE_MODEL("sprites/flare3.spr");
	m_usFireM60 = PRECACHE_EVENT(1, "events/m60.sc");
	m_usFireUSP = PRECACHE_EVENT(1, "events/usp.sc");
	m_usFireAK47 = PRECACHE_EVENT(1, "events/ak47.sc");
	m_usFireM4A1 = PRECACHE_EVENT(1, "events/m4a1.sc");
	m_usFireScout = PRECACHE_EVENT(1, "events/scout.sc");
	m_usFireMAC10 = PRECACHE_EVENT(1, "events/mac10.sc");
	m_usFireShotgun = PRECACHE_EVENT(1, "events/xm1014.sc");
	m_usFireMP5 = PRECACHE_EVENT(1, "events/mp5n.sc");
}

void CCounterTerrorist::Spawn()
{
	Precache();
	SpawnInit();
	m_ctBehavior = GetBehaviorType();
	UTIL_SetSize(pev, Vector(-16, -16, 0), Vector(16, 16, 72));
	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	m_bloodColor = BLOOD_COLOR_RED;
	pev->effects = 0;
	pev->health = gSkillData.hgruntHealth * m_healthMultiplier;
	m_flFieldOfView = 0.1;
	m_MonsterState = MONSTERSTATE_NONE;
	m_flNextGrenadeCheck = gpGlobals->time + 1;
	m_flNextPainTime = gpGlobals->time;
	m_iSentence = -1;
	m_afCapability = bits_CAP_SQUAD | bits_CAP_TURN_HEAD | bits_CAP_DOORS_GROUP;
	m_fEnemyEluded = FALSE;
	m_fFirstEncounter = TRUE;
	m_HackedGunPos = Vector(0, 0, 55);
	SetupWeapons();
	InitScheduleTable();
	SetupBehaviors(m_ctBehavior);
	CTalkMonster::g_talkWaitTime = 0;
	MonsterInit();
	if (m_ctBehavior == 4 || m_ctBehavior == 5)
	{
		if (!m_customRange)
		{
			m_flDistTooFar = 2048;
		}
		m_flDistLook = 4096;
		m_flFieldOfView = -0.7;
		SetupLaserGlow();
	}
	if (m_invulnerable)
	{
		pev->takedamage = DAMAGE_NO;
	}
	if (m_head > 0)
	{
		SetBodygroup(1, m_head);
	}
	SetUse(&CCounterTerrorist::FollowerUse);
}

int CCounterTerrorist::GetBehaviorType()
{
	const char* classname = STRING(pev->classname);
	if (strstr(classname, "pistol"))
		return 0;
	if (strstr(classname, "shotgun"))
		return 1;
	if (strstr(classname, "assaultrifle"))
		return 2;
	if (strstr(classname, "machinegun"))
		return 3;
	if (strstr(classname, "sniperrifle"))
		return 4;
	if (strstr(classname, "law"))
		return 5;
	if (strstr(classname, "smg"))
		return 6;
	if (strstr(classname, "grenader"))
		return 7;
	if (strstr(classname, "mp5"))
		return 8;
	return m_behaviorType == 1 ? 4 : 8;
}

void CCounterTerrorist::SetupWeapons()
{
	m_cClipSize = 36;
	m_cAmmoLoaded = 36;
	if (pev->weapons & 64)
	{
		SetBodygroup(2, 8);
		m_cClipSize = 0;
		m_cAmmoLoaded = 0;
		return;
	}
	if (m_ctBehavior != -1)
	{
		switch (m_ctBehavior)
		{
		case 0:
			m_cClipSize = 16;
			SetBodygroup(2, 5);
			break;
		case 1:
			pev->weapons = 8;
			m_cClipSize = 8;
			SetBodygroup(2, 1);
			break;
		case 2:
			pev->weapons = 1;
			m_cClipSize = 30;
			SetBodygroup(2, 4);
			break;
		case 3:
			SetBodygroup(2, 6);
			break;
		case 4:
			pev->weapons = 16;
			SetBodygroup(2, 3);
			break;
		case 5:
			m_cClipSize = 1;
			SetBodygroup(2, 2);
			break;
		case 6:
			pev->weapons = 1;
			m_cClipSize = 45;
			SetBodygroup(2, 7);
			break;
		case 7:
			pev->weapons = 2;
			m_cClipSize = 16;
			SetBodygroup(2, 5);
			break;
		default:
			SetBodygroup(2, 0);
			break;
		}
		m_cAmmoLoaded = m_cClipSize;
		return;
	}
	if (!pev->weapons)
	{
		pev->weapons = 3;
		SetBodygroup(2, 1);
		return;
	}
	if (pev->weapons & 1)
		SetBodygroup(2, 0);
	if (pev->weapons & 8)
	{
		SetBodygroup(2, 1);
		m_cClipSize = 8;
	}
	if (pev->weapons & 32)
		SetBodygroup(2, 2);
	if (pev->weapons & 16)
		SetBodygroup(2, 3);
	m_cAmmoLoaded = m_cClipSize;
}

void CCounterTerrorist::SetupBehaviors(int type)
{
	if (type < 0 || type > 8)
		return;
	scheduleTable[SCHED_TAKE_COVER_FROM_ENEMY].percentChance = 0;
	scheduleTable[SCHED_SMALL_FLINCH].percentChance = 0.9;
	scheduleTable[SCHED_TAKE_COVER_LIGHT_DAMAGE].percentChance = 0;
	scheduleTable[SCHED_TAKE_COVER_HEAVY_DAMAGE].percentChance = 0;
	scheduleTable[SCHED_CT_COVER_AND_RELOAD].percentChance = 1;
	scheduleTable[SCHED_CT_ESTABLISH_LINE_OF_FIRE].percentChance = 0.8;
	scheduleTable[SCHED_CT_CLOSEIN].percentChance = 0.1;
	scheduleTable[SCHED_RANGE_ATTACK1].percentChance = 0.9;
	scheduleTable[SCHED_RANGE_ATTACK2].percentChance = 0;
	scheduleTable[SCHED_MELEE_ATTACK1].percentChance = 0.3;
	scheduleTable[SCHED_CT_SNIPER_ATTACK].percentChance = 0;
	scheduleTable[SCHED_CT_SNIPER_TAKESHOT].percentChance = 0;
	scheduleTable[SCHED_CT_SPECIAL_COVER_NODE].percentChance = 0.4;
	scheduleTable[SCHED_STANDOFF].percentChance = 0.1;
	scheduleTable[SCHED_CT_FIRE_PAUSE].percentChance = 0.6;
	switch (type)
	{
	case 0:
		scheduleTable[SCHED_CT_ESTABLISH_LINE_OF_FIRE].percentChance = 0.9;
		scheduleTable[SCHED_RANGE_ATTACK1].percentChance = 1;
		scheduleTable[SCHED_MELEE_ATTACK1].percentChance = 0.2;
		scheduleTable[SCHED_CT_SPECIAL_COVER_NODE].percentChance = 1;
		scheduleTable[SCHED_CT_FIRE_PAUSE].percentChance = 0.4;
		m_fPreferedRange = 192;
		break;
	case 1:
		scheduleTable[SCHED_CT_ESTABLISH_LINE_OF_FIRE].percentChance = 0;
		scheduleTable[SCHED_CT_CLOSEIN].percentChance = 0.4;
		scheduleTable[SCHED_CT_SPECIAL_COVER_NODE].percentChance = 0.2;
		scheduleTable[SCHED_CT_FIRE_PAUSE].percentChance = 0.4;
		m_fPreferedRange = 128;
		break;
	case 2:
		scheduleTable[SCHED_CT_CLOSEIN].percentChance = 0.05;
		m_fPreferedRange = 256;
		break;
	case 3:
		scheduleTable[SCHED_CT_COVER_AND_RELOAD].percentChance = 0;
		scheduleTable[SCHED_CT_ESTABLISH_LINE_OF_FIRE].percentChance = 0.5;
		scheduleTable[SCHED_CT_CLOSEIN].percentChance = 0;
		scheduleTable[SCHED_RANGE_ATTACK1].percentChance = 0.95;
		scheduleTable[SCHED_MELEE_ATTACK1].percentChance = 0;
		scheduleTable[SCHED_CT_SPECIAL_COVER_NODE].percentChance = 0.1;
		scheduleTable[SCHED_CT_FIRE_PAUSE].percentChance = 0.1;
		break;
	case 4:
		scheduleTable[SCHED_SMALL_FLINCH].percentChance = 1;
		scheduleTable[SCHED_CT_COVER_AND_RELOAD].percentChance = 0;
		scheduleTable[SCHED_CT_ESTABLISH_LINE_OF_FIRE].percentChance = 0.5;
		scheduleTable[SCHED_CT_CLOSEIN].percentChance = 0;
		scheduleTable[SCHED_RANGE_ATTACK1].percentChance = 0;
		scheduleTable[SCHED_MELEE_ATTACK1].percentChance = 0.2;
		scheduleTable[SCHED_CT_SNIPER_ATTACK].percentChance = 1;
		scheduleTable[SCHED_CT_SNIPER_TAKESHOT].percentChance = 0.5;
		scheduleTable[SCHED_CT_SPECIAL_COVER_NODE].percentChance = 0;
		scheduleTable[SCHED_STANDOFF].percentChance = 0;
		scheduleTable[SCHED_CT_FIRE_PAUSE].percentChance = 0;
		break;
	case 5:
		scheduleTable[SCHED_CT_ESTABLISH_LINE_OF_FIRE].percentChance = 0.9;
		scheduleTable[SCHED_CT_CLOSEIN].percentChance = 0;
		scheduleTable[SCHED_RANGE_ATTACK1].percentChance = 1;
		scheduleTable[SCHED_MELEE_ATTACK1].percentChance = 0;
		scheduleTable[SCHED_CT_SPECIAL_COVER_NODE].percentChance = 0;
		scheduleTable[SCHED_CT_FIRE_PAUSE].percentChance = 0;
		break;
	case 6:
		scheduleTable[SCHED_SMALL_FLINCH].percentChance = 0.8;
		scheduleTable[SCHED_MELEE_ATTACK1].percentChance = 0.1;
		scheduleTable[SCHED_CT_SPECIAL_COVER_NODE].percentChance = 0.3;
		scheduleTable[SCHED_CT_FIRE_PAUSE].percentChance = 0.1;
		m_fPreferedRange = 192;
		break;
	case 7:
		scheduleTable[SCHED_TAKE_COVER_FROM_ENEMY].percentChance = 0.1;
		scheduleTable[SCHED_CT_ESTABLISH_LINE_OF_FIRE].percentChance = 0;
		scheduleTable[SCHED_CT_CLOSEIN].percentChance = 0;
		scheduleTable[SCHED_RANGE_ATTACK1].percentChance = 0.1;
		scheduleTable[SCHED_RANGE_ATTACK2].percentChance = 1;
		scheduleTable[SCHED_MELEE_ATTACK1].percentChance = 0;
		scheduleTable[SCHED_CT_SPECIAL_COVER_NODE].percentChance = 0;
		scheduleTable[SCHED_STANDOFF].percentChance = 0.2;
		scheduleTable[SCHED_CT_FIRE_PAUSE].percentChance = 0.5;
		break;
	case 8:
		scheduleTable[SCHED_SMALL_FLINCH].percentChance = 0.8;
		m_fPreferedRange = 256;
		break;
	}
}

int CCounterTerrorist::GetScheduleFromTable()
{
	float total = 0;
	for (int i = 0; i < LAST_CUSTOM_SCHEDULE; i++)
	{
		if (scheduleTable[i].activeFlag && scheduleTable[i].percentChance > 0)
		{
			total += scheduleTable[i].percentChance;
		}
	}
	for (int i = 0; i < LAST_CUSTOM_SCHEDULE; i++)
	{
		if (scheduleTable[i].activeFlag && scheduleTable[i].percentChance > 0)
		{
			scheduleTable[i].normalizedChance = scheduleTable[i].percentChance / total;
		}
	}
	float choice = RANDOM_FLOAT(0, 1);
	float cumulative = 0;
	for (int i = 0; i < LAST_CUSTOM_SCHEDULE; i++)
	{
		if (scheduleTable[i].activeFlag && scheduleTable[i].percentChance > 0)
		{
			cumulative += scheduleTable[i].normalizedChance;
			if (cumulative > choice)
				return i;
		}
	}
	if (m_MonsterState != MONSTERSTATE_IDLE)
	{
		if (pev->targetname)
			ALERT(at_aiconsole, "%s -- %s at (%d, %d, %d), Failed to find valid schedule in the table!\n",
				STRING(pev->classname), STRING(pev->targetname),
				(int)pev->origin.x, (int)pev->origin.y, (int)pev->origin.z);
		else
			ALERT(at_aiconsole, "%s -- (no targetname) at (%d, %d, %d), Failed to find valid schedule in the table!\n",
				STRING(pev->classname), (int)pev->origin.x, (int)pev->origin.y, (int)pev->origin.z);
	}
	return -1;
}

void CCounterTerrorist::IdleHeadTurn(Vector& friendPosition)
{
	if (m_afCapability & bits_CAP_TURN_HEAD)
	{
		Vector direction = friendPosition - pev->origin;
		float yaw = VecToYaw(direction) - pev->angles.y;
		if (yaw > 180)
			yaw -= 360;
		if (yaw < -180)
			yaw += 360;
		SetBoneController(0, yaw);
	}
}

void CCounterTerrorist::StartTask(Task_t* task)
{
	m_iTaskStatus = TASKSTATUS_RUNNING;
	switch (task->iTask)
	{
	case TASK_RUN_PATH:
	case TASK_WALK_PATH:
		Forget(bits_MEMORY_INCOVER);
		CBaseMonster::StartTask(task);
		break;
	case TASK_FACE_IDEAL:
	case TASK_FACE_ENEMY:
		if (m_cantMove > 1)
		{
			TaskComplete();
		}
		else
		{
			CBaseMonster::StartTask(task);
			if (pev->movetype == MOVETYPE_FLY)
				m_IdealActivity = ACT_GLIDE;
		}
		break;
	case TASK_RELOAD:
		m_IdealActivity = ACT_RELOAD;
		break;
	case TASK_CT_FACE_TOSS_DIR:
	case TASK_TLK_EYECONTACT:
		break;
	case TASK_CT_SPEAK_SENTENCE:
		SpeakSentence();
		TaskComplete();
		break;
	case TASK_CT_CHECK_FIRE:
		if (!NoFriendlyFire())
			SetConditions(bits_COND_SPECIAL1);
		TaskComplete();
		break;
	case TASK_MOVE_TO_SPECIAL_COVER_NODE:
		if (!m_pSpecialCoverNode || !FValidateCover(m_pSpecialCoverNode->m_vecOrigin))
		{
			TaskFail();
		}
		else if (MoveToLocation(ACT_RUN, 0, m_pSpecialCoverNode->m_vecOrigin))
		{
			TaskComplete();
		}
		else
		{
			ALERT(at_console, "FAILED MOVING TO LOCATION.\n");
			TaskFail();
		}
		break;
	case TASK_MOVE_TO_PREFERED_RANGE:
		if (m_cantMove || (m_hEnemy->pev->origin - pev->origin).Length2D() < 64)
		{
			TaskComplete();
		}
		else
		{
			m_vecMoveGoal = m_hEnemy->pev->origin;
			if (!MoveToEnemy(ACT_RUN, 0))
				TaskFail();
		}
		break;
	case TASK_WANDER:
		if (MoveToLocation(ACT_WALK, 0, m_wanderOrigin))
			TaskComplete();
		else
			TaskFail();
		break;
	case TASK_TLK_IDEALYAW:
		if (m_hTalkTarget && m_cantMove != 2)
		{
			pev->yaw_speed = 60;
			float yaw = VecToYaw(m_hTalkTarget->pev->origin - pev->origin) - pev->angles.y;
			if (yaw > 180)
				yaw -= 360;
			if (yaw < -180)
				yaw += 360;
			if (yaw < 0)
				pev->ideal_yaw = pev->angles.y + min(yaw + 45, 0);
			else
				pev->ideal_yaw = pev->angles.y + max(yaw - 45, 0);
		}
		TaskComplete();
		break;
	default:
		CBaseMonster::StartTask(task);
		break;
	}
}

void CCounterTerrorist::RunTask(Task_t* task)
{
	switch (task->iTask)
	{
	case TASK_MOVE_TO_PREFERED_RANGE:
		if (m_cantMove)
		{
			TaskComplete();
			return;
		}
		if (!m_hEnemy)
		{
			TaskFail();
			return;
		}
		{
			float distance = (m_vecMoveGoal - pev->origin).Length2D();
			if (distance < m_fPreferedRange ||
				(m_vecMoveGoal - m_hEnemy->pev->origin).Length() > m_fPreferedRange * 0.5)
			{
				m_vecMoveGoal = m_hEnemy->pev->origin;
				distance = (m_vecMoveGoal - pev->origin).Length2D();
				FRefreshRoute();
			}
			if (distance < m_fPreferedRange)
			{
				TaskComplete();
				RouteClear();
			}
			if (m_movementActivity != ACT_RUN)
				m_movementActivity = ACT_RUN;
		}
		break;
	case TASK_TLK_EYECONTACT:
		if (IsTalking() && m_hTalkTarget)
			IdleHeadTurn(m_hTalkTarget->pev->origin);
		else
			TaskComplete();
		break;
	case TASK_CT_FACE_TOSS_DIR:
		MakeIdealYaw(pev->origin + m_vecTossVelocity * 64);
		ChangeYaw((int)pev->yaw_speed);
		if (FacingIdeal())
			m_iTaskStatus = TASKSTATUS_COMPLETE;
		break;
	default:
		CBaseMonster::RunTask(task);
		break;
	}
}

void CCounterTerrorist::StopFollowing(BOOL clearSchedule)
{
	if (m_hTargetEnt && m_hTargetEnt->IsPlayer())
	{
		if (m_movementGoal == MOVEGOAL_TARGETENT)
			RouteClear();
		m_hTargetEnt = NULL;
		if (clearSchedule)
			ClearSchedule();
		if (m_hEnemy)
			m_IdealMonsterState = MONSTERSTATE_COMBAT;
	}
}

void CCounterTerrorist::StartFollowing(CBaseEntity* leader)
{
	if (m_hEnemy)
		m_IdealMonsterState = MONSTERSTATE_ALERT;
	m_hTargetEnt = leader;
	ClearConditions(bits_COND_SPECIAL1);
	ClearSchedule();
}

BOOL CCounterTerrorist::CanFollow()
{
	return IsAlive() && (!m_hTargetEnt || !m_hTargetEnt->IsPlayer());
}

void CCounterTerrorist::LimitFollowers(CBaseEntity* leader, int maxFollowers)
{
}

void CCounterTerrorist::UseTarget(CBaseEntity* activator, CBaseEntity* caller, USE_TYPE useType, float value)
{
	if (m_useTarget)
		FireTargets(STRING(m_useTarget), activator, this, useType, value);
}

void CCounterTerrorist::FollowerUse(CBaseEntity* activator, CBaseEntity* caller, USE_TYPE useType, float value)
{
	if (m_MonsterState == MONSTERSTATE_SCRIPT)
		return;
	CBaseEntity* player = CBaseEntity::Instance(INDEXENT(1));
	if (!player)
		return;
	if (m_useTarget)
	{
		if (!FStrEq(STRING(m_useTarget), "none"))
			UseTarget(activator, caller, useType, value);
		return;
	}
	if (pev->spawnflags & 256)
		return;
	if (!CanFollow())
	{
		StopFollowing(TRUE);
		SayNamedSentence("CT_WAIT", 0.8, ATTN_NORM, 0, m_voicePitch);
	}
	else if (HasMemory(bits_MEMORY_PROVOKED))
	{
		ALERT(at_console, "I'm not following you, you evil person!\n");
	}
	else
	{
		StartFollowing(player);
		SayNamedSentence("CT_OK", 0.8, ATTN_NORM, 0, m_voicePitch);
	}
}

Schedule_t* CCounterTerrorist::HandleFollowing()
{
	if (!m_hEnemy && m_hTargetEnt && m_hTargetEnt->IsPlayer())
	{
		if (m_hTargetEnt->IsAlive())
			return GetScheduleOfType(SCHED_TARGET_FACE);
		StopFollowing(FALSE);
	}
	return NULL;
}

void CCounterTerrorist::ShootSMG()
{
	if (!m_hEnemy)
		return;
	Vector source = GetGunPosition();
	Vector direction = ShootAtEnemy(source);
	UTIL_MakeVectors(pev->angles);
	Vector spread = m_weaponAccuracy.Length() > 0 ? m_weaponAccuracy : VECTOR_CONE_10DEGREES;
	FireBullets(1, source, direction, spread, 2048, BULLET_MONSTER_MP5, 4, 4);
	pev->effects |= EF_MUZZLEFLASH;
	--m_cAmmoLoaded;
	SetBlending(0, UTIL_VecToAngles(direction).x);
	float forward = RANDOM_FLOAT(-40, 40);
	float up = RANDOM_FLOAT(75, 200);
	float side = RANDOM_FLOAT(40, 90);
	Vector velocity = gpGlobals->v_right * side + gpGlobals->v_up * up + gpGlobals->v_forward * forward;
	EjectBrass(source + direction * 10, velocity, pev->angles.y, m_iBrassShell, TE_BOUNCE_SHELL);
	EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, "weapons/mac10-1.wav", 0.75, ATTN_NORM, 0, 100);
}

void CCounterTerrorist::ShootMP5()
{
	if (!m_hEnemy)
		return;
	Vector source = GetGunPosition();
	Vector direction = ShootAtEnemy(source);
	UTIL_MakeVectors(pev->angles);
	Vector spread = m_weaponAccuracy.Length() > 0 ? m_weaponAccuracy : VECTOR_CONE_10DEGREES;
	FireBullets(1, source, direction, spread, 2048, BULLET_MONSTER_MP5, 4, 10);
	pev->effects |= EF_MUZZLEFLASH;
	--m_cAmmoLoaded;
	SetBlending(0, UTIL_VecToAngles(direction).x);
	float forward = RANDOM_FLOAT(-40, 40);
	float up = RANDOM_FLOAT(75, 200);
	float side = RANDOM_FLOAT(40, 90);
	Vector velocity = gpGlobals->v_right * side + gpGlobals->v_up * up + gpGlobals->v_forward * forward;
	EjectBrass(source + direction * 8, velocity, pev->angles.y, m_iBrassShell, TE_BOUNCE_SHELL);
	EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, RANDOM_LONG(0, 1) ? "weapons/mp5-1.wav" : "weapons/mp5-2.wav",
		0.75, ATTN_NORM, 0, 100);
}

void CCounterTerrorist::ShootPistol()
{
	if (!m_hEnemy)
		return;
	Vector source = GetGunPosition();
	Vector direction = ShootAtEnemy(source);
	UTIL_MakeVectors(pev->angles);
	Vector spread = m_weaponAccuracy.Length() > 0 ? m_weaponAccuracy : VECTOR_CONE_10DEGREES;
	FireBullets(1, source, direction, spread, 1024, BULLET_MONSTER_MP5, 0, 5);
	--m_cAmmoLoaded;
	pev->effects |= EF_MUZZLEFLASH;
	SetBlending(0, UTIL_VecToAngles(direction).x);
	float forward = RANDOM_FLOAT(-40, 40);
	float up = RANDOM_FLOAT(75, 200);
	float side = RANDOM_FLOAT(40, 90);
	Vector velocity = gpGlobals->v_right * side + gpGlobals->v_up * up + gpGlobals->v_forward * forward;
	EjectBrass(source + direction * 13, velocity, pev->angles.y, m_iBrassShell, TE_BOUNCE_SHELL);
	EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, "weapons/usp_unsil-1.wav", 0.75, ATTN_NORM, 0, 100);
}

void CCounterTerrorist::ShootSniperRifle()
{
	if (!m_hEnemy)
		return;
	Vector source = GetGunPosition();
	Vector direction = ShootAtEnemy(source);
	UTIL_MakeVectors(pev->angles);
	Vector spread = m_weaponAccuracy.Length() > 0 ? m_weaponAccuracy : VECTOR_CONE_5DEGREES;
	FireBullets(1, source, direction, spread, 4096, BULLET_MONSTER_MP5, 0, 45);
	pev->effects |= EF_MUZZLEFLASH;
	SetBlending(0, UTIL_VecToAngles(direction).x);
	float forward = RANDOM_FLOAT(-40, 40);
	float up = RANDOM_FLOAT(75, 200);
	float side = RANDOM_FLOAT(40, 90);
	Vector velocity = gpGlobals->v_right * side + gpGlobals->v_up * up + gpGlobals->v_forward * forward;
	EjectBrass(source + direction * 8, velocity, pev->angles.y, m_iBrassShell, TE_BOUNCE_SHELL);
	EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, "weapons/scout_fire-1.wav", 1, ATTN_NORM, 0, 100);
}

void CCounterTerrorist::ShootMachineGun()
{
	if (!m_hEnemy)
		return;
	Vector source = GetGunPosition();
	Vector direction = ShootAtEnemy(source);
	UTIL_MakeVectors(pev->angles);
	Vector spread = m_weaponAccuracy.Length() > 0 ? m_weaponAccuracy : VECTOR_CONE_10DEGREES;
	FireBullets(1, source, direction, spread, 2048, BULLET_MONSTER_MP5, 8, 15);
	pev->effects |= EF_MUZZLEFLASH;
	SetBlending(0, UTIL_VecToAngles(direction).x);
	float forward = RANDOM_FLOAT(-40, 40);
	float up = RANDOM_FLOAT(75, 200);
	float side = RANDOM_FLOAT(40, 90);
	Vector velocity = gpGlobals->v_right * side + gpGlobals->v_up * up + gpGlobals->v_forward * forward;
	EjectBrass(source + direction * 8, velocity, pev->angles.y, m_iBrassShell, TE_BOUNCE_SHELL);
	EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, RANDOM_LONG(0, 1) ? "weapons/m60-1.wav" : "weapons/m60-2.wav",
		0.75, ATTN_NORM, 0, 100);
}

void CCounterTerrorist::ShootAssaultRifle()
{
	if (!m_hEnemy)
		return;
	Vector source = GetGunPosition();
	Vector direction = ShootAtEnemy(source);
	UTIL_MakeVectors(pev->angles);
	Vector spread = m_weaponAccuracy.Length() > 0 ? m_weaponAccuracy : VECTOR_CONE_10DEGREES;
	FireBullets(1, source, direction, spread, 2048, BULLET_MONSTER_MP5, 4, 12);
	pev->effects |= EF_MUZZLEFLASH;
	--m_cAmmoLoaded;
	SetBlending(0, UTIL_VecToAngles(direction).x);
	float forward = RANDOM_FLOAT(-40, 40);
	float up = RANDOM_FLOAT(75, 200);
	float side = RANDOM_FLOAT(40, 90);
	Vector velocity = gpGlobals->v_right * side + gpGlobals->v_up * up + gpGlobals->v_forward * forward;
	if (m_ctType == 3)
	{
		EjectBrass(source + direction * 8, velocity, pev->angles.y, m_iBrassShell, TE_BOUNCE_SHELL);
		EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, RANDOM_LONG(0, 1) ? "weapons/ak47-1.wav" : "weapons/ak47-2.wav",
			0.75, ATTN_NORM, 0, 100);
	}
	else
	{
		EjectBrass(source - direction * 24, velocity, pev->angles.y, m_iBrassShell, TE_BOUNCE_SHELL);
		EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON,
			RANDOM_LONG(0, 99) > 50 ? "weapons/m4a1_unsil-1.wav" : "weapons/m4a1_unsil-2.wav",
			0.75, ATTN_NORM, 0, 100);
	}
}

void CCounterTerrorist::ShootShotgun()
{
	if (!m_hEnemy)
		return;
	Vector source = GetGunPosition();
	Vector direction = ShootAtEnemy(source);
	UTIL_MakeVectors(pev->angles);
	float forward = RANDOM_FLOAT(-40, 40);
	float up = RANDOM_FLOAT(75, 200);
	float side = RANDOM_FLOAT(40, 90);
	Vector velocity = gpGlobals->v_right * side + gpGlobals->v_up * up + gpGlobals->v_forward * forward;
	EjectBrass(source - direction * 24, velocity, pev->angles.y, m_iShotgunShell, TE_BOUNCE_SHOTSHELL);
	Vector spread = m_weaponAccuracy.Length() > 0 ? m_weaponAccuracy : VECTOR_CONE_15DEGREES;
	FireBullets(8, source, direction, spread, 2048, BULLET_PLAYER_BUCKSHOT, 0, 10);
	pev->effects |= EF_MUZZLEFLASH;
	--m_cAmmoLoaded;
	SetBlending(0, UTIL_VecToAngles(direction).x);
	forward = RANDOM_FLOAT(-40, 40);
	up = RANDOM_FLOAT(75, 200);
	side = RANDOM_FLOAT(40, 90);
	velocity = gpGlobals->v_right * side + gpGlobals->v_up * up + gpGlobals->v_forward * forward;
	EjectBrass(source + direction * 8, velocity, pev->angles.y, m_iShotgunShell, TE_BOUNCE_SHOTSHELL);
	EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, "weapons/xm1014-1.wav", 0.75, ATTN_NORM, 0, 100);
}

void CCounterTerrorist::ShootLAW()
{
	if (!m_hEnemy)
		return;
	Vector source = GetGunPosition();
	Vector direction = ShootAtEnemy(source);
	Vector angles = UTIL_VecToAngles(direction);
	angles.x *= -1;
	UTIL_MakeVectors(angles);
	float x = RANDOM_FLOAT(-0.5, 0.5);
	x += RANDOM_FLOAT(-0.5, 0.5);
	float y = RANDOM_FLOAT(-0.5, 0.5);
	y += RANDOM_FLOAT(-0.5, 0.5);
	direction = direction + gpGlobals->v_right * (x * m_weaponAccuracy.x) + gpGlobals->v_up * (y * m_weaponAccuracy.y);
	Vector rocketAngles = UTIL_VecToAngles(direction);
	rocketAngles.x *= -1;
	CLawsRocket::CreateLawsRocket(source, rocketAngles, this, NULL);
	unsigned short event = PRECACHE_EVENT(1, "events/laws.sc");
	PLAYBACK_EVENT_FULL(0, edict(), event, 0, (float*)&g_vecZero, (float*)&g_vecZero, 0, 0, 0, 0, 0, 0);
	angles.x *= -1;
	SetBlending(0, angles.x);
	--m_cAmmoLoaded;
}

CBaseEntity* CCounterTerrorist::Kick()
{
	UTIL_MakeVectors(pev->angles);
	Vector start = pev->origin + Vector(0, 0, pev->size.z * 0.75);
	Vector end;
	if (m_hEnemy)
	{
		Vector direction = m_hEnemy->pev->origin - Vector(0, 0, pev->size.z * 0.25) - start;
		end = start + direction.Normalize() * 128;
	}
	else
		end = start + gpGlobals->v_forward * 128;
	TraceResult trace;
	UTIL_TraceHull(start, end, dont_ignore_monsters, head_hull, edict(), &trace);
	return CBaseEntity::Instance(trace.pHit);
}

void CCounterTerrorist::PrescheduleThink()
{
	if (m_flStopTalkTime != 0 && gpGlobals->time > m_flStopTalkTime)
	{
		m_flStopTalkTime = 0;
		ResetHeadWatch();
	}
	if (InSquad() && m_hEnemy)
	{
		if (HasConditions(bits_COND_SEE_ENEMY))
			MySquadLeader()->m_flLastEnemySightTime = gpGlobals->time;
		else if (gpGlobals->time - MySquadLeader()->m_flLastEnemySightTime > 5)
			MySquadLeader()->m_fEnemyEluded = TRUE;
	}
}

void CCounterTerrorist::IdleSound()
{
	if (RANDOM_LONG(0, 99) > 75 || !FOkToSpeak())
		return;
	if (!g_fCTQuestion && !RANDOM_LONG(0, 1))
		return;
	if (g_fCTQuestion == 1)
	{
		SayNamedSentence("CT_CLEAR", 0.8, ATTN_NORM, 0, m_voicePitch);
		g_fCTQuestion = 0;
	}
	else if (g_fCTQuestion == 2)
	{
		SayNamedSentence("CT_ANSWER", 0.8, ATTN_NORM, 0, m_voicePitch);
		g_fCTQuestion = 0;
	}
	else
	{
		switch (RANDOM_LONG(0, 2))
		{
		case 0:
			SayNamedSentence("CT_CHECK", 0.8, ATTN_NORM, 0, m_voicePitch);
			g_fCTQuestion = 1;
			break;
		case 1:
			SayNamedSentence("CT_QUEST", 0.8, ATTN_NORM, 0, m_voicePitch);
			g_fCTQuestion = 2;
			break;
		case 2:
			SayNamedSentence("CT_IDLE", 0.8, ATTN_NORM, 0, m_voicePitch);
			break;
		}
	}
	CTalkMonster::g_talkWaitTime = gpGlobals->time + 5;
	m_iSentence = -1;
}

int CCounterTerrorist::TakeDamage(entvars_t* inflictor, entvars_t* attacker, float damage, int damageType)
{
	Forget(bits_MEMORY_INCOVER);
	CBaseEntity* entity = CBaseEntity::Instance(attacker);
	if (entity && entity->IsPlayer())
	{
		SayNamedSentence("CT_SHOT", 0.8, ATTN_NORM, 0, m_voicePitch);
		CTalkMonster::g_talkWaitTime = gpGlobals->time + 1;
		m_iSentence = -1;
		m_flNextPainTime = gpGlobals->time + 2;
	}
	return CBaseMonster::TakeDamage(inflictor, attacker, damage, damageType);
}

void CCounterTerrorist::GibMonster()
{
	if (GetBodygroup(2) != 2)
	{
		Vector position, angles;
		GetAttachment(0, position, angles);
		if (pev->weapons & 4)
		{
			CBaseEntity* item = DropItem("ammo_ARgrenades", position, angles);
			if (item)
			{
				item->pev->velocity = Vector(RANDOM_FLOAT(-100, 100), RANDOM_FLOAT(-100, 100), RANDOM_FLOAT(200, 300));
				item->pev->avelocity = Vector(0, RANDOM_FLOAT(200, 400), 0);
			}
		}
	}
	CBaseMonster::GibMonster();
}

BOOL CCounterTerrorist::CheckRangeAttack2(float dot, float distance)
{
	if (!(pev->weapons & 6))
		return FALSE;
	if (m_flGroundSpeed != 0)
	{
		m_fThrowGrenade = FALSE;
		return FALSE;
	}
	if (gpGlobals->time < m_flNextGrenadeCheck)
		return m_fThrowGrenade;
	if (!(m_hEnemy->pev->flags & FL_ONGROUND) && m_hEnemy->pev->waterlevel == 0 && m_vecEnemyLKP.z > pev->absmax.z)
	{
		m_fThrowGrenade = FALSE;
		return FALSE;
	}
	Vector target;
	if (pev->weapons & 2)
	{
		if (RANDOM_LONG(0, 1))
			target = Vector(m_hEnemy->pev->origin.x, m_hEnemy->pev->origin.y, m_hEnemy->pev->absmin.z);
		else
			target = m_vecEnemyLKP;
	}
	else
	{
		target = m_vecEnemyLKP + m_hEnemy->BodyTarget(pev->origin) - m_hEnemy->pev->origin;
		if (HasConditions(bits_COND_SEE_ENEMY))
			target = target + m_hEnemy->pev->velocity * ((target - pev->origin).Length() / gSkillData.hgruntGrenadeSpeed);
	}
	if (InSquad() && SquadMemberInRange(target, 256))
	{
		m_fThrowGrenade = FALSE;
		m_flNextGrenadeCheck = gpGlobals->time + 1;
	}
	if ((target - pev->origin).Length2D() <= 256)
	{
		m_fThrowGrenade = FALSE;
		m_flNextGrenadeCheck = gpGlobals->time + 1;
		return FALSE;
	}
	Vector velocity;
	if (pev->weapons & 2)
		velocity = VecCheckToss(pev, GetGunPosition(), target, 0.5);
	else
		velocity = VecCheckThrow(pev, GetGunPosition(), target, gSkillData.hgruntGrenadeSpeed, 0.5);
	if (velocity != g_vecZero)
	{
		m_vecTossVelocity = velocity;
		m_fThrowGrenade = TRUE;
		m_flNextGrenadeCheck = gpGlobals->time + ((pev->weapons & 2) ? 0 : 0.3);
	}
	else
	{
		m_fThrowGrenade = FALSE;
		m_flNextGrenadeCheck = gpGlobals->time + 1;
	}
	return m_fThrowGrenade;
}

void CCounterTerrorist::HandleAnimEvent(MonsterEvent_t* event)
{
	switch (event->event)
	{
	case 2:
		if (m_ctBehavior == 2 || m_ctBehavior == 4 || m_ctBehavior == 8)
			EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, "weapons/reload_mp5.wav", 0.75, ATTN_NORM, 0, 100);
		else if (m_ctBehavior == 0 || m_ctBehavior == 6)
			EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, "weapons/pistol_reload.wav", 0.75, ATTN_NORM, 0, 100);
		else if (m_ctBehavior == 1)
			EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, "weapons/reload_shotgun.wav", 0.75, ATTN_NORM, 0, 100);
		ClearConditions(bits_COND_NO_AMMO_LOADED);
		m_cAmmoLoaded = m_cClipSize;
		break;
	case 3:
		{
			CBaseEntity* victim = Kick();
			if (victim)
			{
				EMIT_SOUND_DYN(ENT(pev), CHAN_ITEM, "weapons/knife_slash1.wav", 1, ATTN_NORM, 0, 100);
				UTIL_MakeVectors(pev->angles);
				victim->pev->punchangle.x = 5;
				victim->pev->velocity = gpGlobals->v_forward * 100 + victim->pev->velocity + gpGlobals->v_up * 50;
				victim->TakeDamage(pev, pev, gSkillData.hgruntDmgKick, DMG_CLUB);
			}
		}
		break;
	case 4:
	case 5:
	case 6:
		justShotFlag = TRUE;
		switch (m_ctBehavior)
		{
		case 0:
		case 7: ShootPistol(); break;
		case 1: ShootShotgun(); break;
		case 3: ShootMachineGun(); break;
		case 4: ShootSniperRifle(); break;
		case 5: ShootLAW(); break;
		case 6: ShootSMG(); break;
		case 8: ShootMP5(); break;
		default: ShootAssaultRifle(); break;
		}
		CSoundEnt::InsertSound(bits_SOUND_COMBAT, pev->origin, 384, 0.3);
		break;
	case 7:
		UTIL_MakeVectors(pev->angles);
		CGrenade::ShootTimed(pev, GetGunPosition(), m_vecTossVelocity, 3.5, 100, 0);
		m_fThrowGrenade = FALSE;
		m_flNextGrenadeCheck = gpGlobals->time + 6;
		break;
	case 8:
		EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, "weapons/glauncher.wav", 0.8, ATTN_NORM, 0, 100);
		CGrenade::ShootContact(pev, GetGunPosition(), m_vecTossVelocity);
		m_fThrowGrenade = FALSE;
		m_flNextGrenadeCheck = gpGlobals->time + (g_iSkillLevel == 3 ? RANDOM_FLOAT(2, 5) : 6);
		break;
	case 9:
		UTIL_MakeVectors(pev->angles);
		CGrenade::ShootTimed(pev, pev->origin + gpGlobals->v_forward * 17 - gpGlobals->v_right * 27
			+ gpGlobals->v_up * 6, g_vecZero, 3, 35, 0);
		break;
	case 10:
		if (FOkToSpeak())
		{
			SayNamedSentence("CT_ENEMY", 0.8, ATTN_NORM, 0, m_voicePitch);
			JustSpoke();
		}
		CBaseMonster::HandleAnimEvent(event);
		break;
	case 11:
		{
			Vector position, angles;
			GetAttachment(0, position, angles);
			SetBodygroup(2, 8);
			if (m_dropItem && m_dropChance > RANDOM_FLOAT(0, 1))
				DropItem((char*)STRING(m_dropItem), position, angles);
		}
		break;
	case 13:
		break;
	case 14:
		LaserGlowOff();
		break;
	default:
		CBaseMonster::HandleAnimEvent(event);
		break;
	}
}

void CCounterTerrorist::SetupLaserGlow()
{
	if (!m_pBeam)
	{
		m_pBeam = CBeam::BeamCreate("sprites/laserbeam.spr", 3);
		m_pBeam->pev->spawnflags |= SF_BEAM_TEMPORARY;
		m_pBeam->SetFlags(BEAM_FSHADEOUT);
		m_pBeam->SetColor(255, 0, 0);
		m_pBeam->SetBrightness(0);
		m_pBeam->pev->rendermode = kRenderGlow;
		m_pBeam->SetColor(255, 0, 0);
		m_pBeam->SetBrightness(0);
		m_pBeam->pev->renderfx = kRenderFxNoDissipation;
		m_laserBrightness = 0;
	}
	if (!m_pLaserGlow)
	{
		m_pLaserGlow = CSprite::SpriteCreate("sprites/flare3.spr", pev->origin, FALSE);
		m_pLaserGlow->SetTransparency(kRenderGlow, 255, 0, 0, 0, kRenderFxNoDissipation);
		m_pLaserGlow->SetAttachment(edict(), 2);
		m_pLaserGlow->SetBrightness(0);
		m_glowBrightness = 0;
	}
}

void CCounterTerrorist::LaserGlowOn()
{
	if (m_pBeam)
	{
		if (!m_hEnemy)
			return;
		m_pBeam->EntsInit(entindex(), m_hEnemy->entindex());
		m_pBeam->SetStartAttachment(2);
		if (m_laserBrightness != 64)
			m_laserBrightness = 64;
		m_pBeam->SetBrightness(64);
	}
	if (m_pLaserGlow)
	{
		if (m_glowBrightness != 255)
			m_glowBrightness = 255;
		m_pLaserGlow->SetBrightness(255);
	}
}

void CCounterTerrorist::LaserGlowOff()
{
	UTIL_Remove(m_pBeam);
	m_pBeam = NULL;
	UTIL_Remove(m_pLaserGlow);
	m_pLaserGlow = NULL;
}

int CCounterTerrorist::LookupActivity(int activity)
{
	void* model = GET_MODEL_PTR(edict());
	if (activity != ACT_IDLE)
		return ::LookupActivity(model, pev, activity);
	if (m_ctBehavior == 5)
		return LookupSequence("LAW_fire_idle");
	if (m_ctBehavior == 0 || m_ctBehavior == 6 || m_ctBehavior == 7)
		return LookupSequence(m_MonsterState == MONSTERSTATE_COMBAT ? "pistol_combatidle" : "pistol_idle1");
	if (m_MonsterState == MONSTERSTATE_COMBAT)
		return LookupSequence(m_bStanding ? "combatidle" : "crouching_wait");
	return ::LookupActivity(model, pev, activity);
}

int CCounterTerrorist::LookupActivityHeaviest(int activity)
{
	void* model = GET_MODEL_PTR(edict());
	if (activity != ACT_IDLE)
		return ::LookupActivity(model, pev, activity);
	if (m_ctBehavior == 5)
		return LookupSequence("LAW_fire_idle");
	if (m_ctBehavior == 0 || m_ctBehavior == 6 || m_ctBehavior == 7)
		return LookupSequence("pistol_idle1");
	if (m_MonsterState == MONSTERSTATE_COMBAT)
		return LookupSequence(m_bStanding ? "combatidle" : "crouching_wait");
	return ::LookupActivityHeaviest(model, pev, activity);
}

int CCounterTerrorist::SetActivityPistolBase(Activity activity)
{
	GET_MODEL_PTR(edict());
	const char* sequence;
	switch (activity)
	{
	case ACT_IDLE:
		if (m_idleAnim)
			sequence = STRING(m_idleAnim);
		else if (m_MonsterState != MONSTERSTATE_COMBAT)
			sequence = "pistol_idle1";
		else
			sequence = m_bStanding ? "pistol_combatidlle" : "pistol_crouching_wait";
		break;
	case ACT_WALK:
		sequence = pev->health > pev->max_health * 0.2f && m_MonsterState == MONSTERSTATE_COMBAT
			? "combat_walk_pistol" : "pistol_walk";
		break;
	case ACT_RUN:
		sequence = pev->health > pev->max_health * 0.2f && m_MonsterState == MONSTERSTATE_COMBAT
			? "combat_run_pistol" : "pistol_run";
		break;
	case ACT_STRAFE_LEFT: sequence = "pistol_strafefire_L"; break;
	case ACT_STRAFE_RIGHT: sequence = "pistol_strafefire_R"; break;
	case ACT_TURN_LEFT:
		sequence = m_MonsterState == MONSTERSTATE_COMBAT ? "pistol_180L" : "pistol_idle1";
		break;
	case ACT_TURN_RIGHT:
		sequence = m_MonsterState == MONSTERSTATE_COMBAT ? "pistol_180R" : "pistol_idle1";
		break;
	case ACT_CROUCH: sequence = "pistol_crouching_wait"; break;
	case ACT_COWER: sequence = "pistol_cower"; break;
	case ACT_SMALL_FLINCH: sequence = "pistol_smflinch"; break;
	case ACT_RANGE_ATTACK1:
		if (m_ctBehavior == 6)
			sequence = m_bStanding ? "shootcycle_tmp" : "crouching_tmp";
		else
			sequence = m_bStanding ? "pistol_standing" : "pistol_crouching";
		break;
	case ACT_MELEE_ATTACK1: sequence = "pistol_melee"; break;
	case ACT_RELOAD: sequence = "pistol_reload"; break;
	case ACT_DIESIMPLE: sequence = m_deathAnim ? STRING(m_deathAnim) : "pistol_die-simple"; break;
	case ACT_DIEBACKWARD: sequence = m_deathAnim ? STRING(m_deathAnim) : "pistol_dieback1"; break;
	case ACT_DIEFORWARD:
	case ACT_DIE_BACKSHOT: sequence = m_deathAnim ? STRING(m_deathAnim) : "pistol_dieforward"; break;
	case ACT_DIEVIOLENT:
	case ACT_DIE_HEADSHOT: sequence = m_deathAnim ? STRING(m_deathAnim) : "pistol_headshot"; break;
	case ACT_DIE_CHESTSHOT:
	case ACT_DIE_GUTSHOT: sequence = m_deathAnim ? STRING(m_deathAnim) : "pistol_diegutshot"; break;
	case ACT_FLINCH_LEFTARM: sequence = "pistol_leftarmflinch"; break;
	case ACT_FLINCH_RIGHTARM: sequence = "pistol_rightarmflinch"; break;
	case ACT_FLINCH_LEFTLEG: sequence = "pistol_leftlegsmflinch"; break;
	case ACT_FLINCH_RIGHTLEG: sequence = "pistol_rightlegsmflinch"; break;
	default: return -1;
	}
	return LookupSequence(sequence);
}

int CCounterTerrorist::SetActivitySniperBase(Activity activity)
{
	GET_MODEL_PTR(edict());
	switch (activity)
	{
	case ACT_RANGE_ATTACK1:
		return LookupSequence(m_bStanding ? "sniper-fire" : "sniper_crouch-fire");
	case ACT_ARM:
		{
			int sequence = LookupSequence("laser_on_stand");
			LaserGlowOn();
			return sequence;
		}
	case ACT_DISARM:
		{
			int sequence = LookupSequence("laser_off_stand");
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
		break;
	default:
		break;
	}
	return -1;
}

int CCounterTerrorist::SetActivityShotgunBase(Activity activity)
{
	GET_MODEL_PTR(edict());
	if (activity == ACT_RANGE_ATTACK1)
		return LookupSequence(m_bStanding ? "standing_shotgun" : "crouching_shotgun");
	if (activity == ACT_RELOAD)
		return LookupSequence(m_bStanding ? "reload_shotgun" : "crouch_reload_shotgun");
	return -1;
}

int CCounterTerrorist::SetActivityGrenadeBase(Activity activity)
{
	GET_MODEL_PTR(edict());
	if (activity == ACT_RANGE_ATTACK2)
		return LookupSequence((pev->weapons & 2) ? "throwgrenade" : "launchgrenade");
	return -1;
}

int CCounterTerrorist::SetActivityAutomaticBase(Activity activity)
{
	GET_MODEL_PTR(edict());
	if (activity == ACT_RANGE_ATTACK1)
		return LookupSequence(m_ctBehavior == 3 ? "machinegun_fire" : (m_bStanding ? "standing_mp5" : "crouching_mp5"));
	if (activity == ACT_RELOAD)
		return LookupSequence("reload_mp5");
	return -1;
}

int CCounterTerrorist::SetActivityLAWBase(Activity activity)
{
	GET_MODEL_PTR(edict());
	switch (activity)
	{
	case ACT_IDLE: return LookupSequence(m_idleAnim ? STRING(m_idleAnim) : "LAW_fire_idle");
	case ACT_WALK:
	case ACT_RUN: return LookupSequence("law_run");
	case ACT_TURN_LEFT: return LookupSequence("law_180L");
	case ACT_TURN_RIGHT: return LookupSequence("law_180R");
	case ACT_RANGE_ATTACK1: return LookupSequence("LAW_fire");
	case ACT_RELOAD: return LookupSequence("law_reload");
	default: return -1;
	}
}

void CCounterTerrorist::SetActivity(Activity activity)
{
	GET_MODEL_PTR(edict());
	int sequence = -1;
	switch (m_ctBehavior)
	{
	case 0:
	case 6: sequence = SetActivityPistolBase(activity); break;
	case 1: sequence = SetActivityShotgunBase(activity); break;
	case 2:
	case 3:
	case 8: sequence = SetActivityAutomaticBase(activity); break;
	case 4: sequence = SetActivitySniperBase(activity); break;
	case 5: sequence = SetActivityLAWBase(activity); break;
	case 7:
		sequence = SetActivityPistolBase(activity);
		if (sequence == -1)
			sequence = SetActivityGrenadeBase(activity);
		break;
	default: break;
	}
	if (sequence == -1)
	{
		switch ((int)activity)
		{
		case ACT_IDLE:
			if (m_idleAnim)
				sequence = LookupSequence(STRING(m_idleAnim));
			else if (m_MonsterState == MONSTERSTATE_COMBAT)
				sequence = LookupSequence(m_bStanding ? "combatidle" : "crouching_wait");
			break;
		case ACT_WALK:
			if (pev->health <= pev->max_health * 0.2f)
				sequence = LookupActivity(ACT_WALK_HURT);
			else if (m_MonsterState == MONSTERSTATE_COMBAT)
				sequence = LookupSequence("combat_walk_primary");
			break;
		case ACT_RUN:
			if (pev->health <= pev->max_health * 0.2f)
				sequence = LookupActivity(ACT_RUN_HURT);
			else if (m_MonsterState == MONSTERSTATE_COMBAT)
				sequence = LookupSequence("combat_run_primary");
			break;
		case ACT_STAND: sequence = LookupSequence("crouching_2_stand"); break;
		case ACT_DIESIMPLE:
		case ACT_DIEBACKWARD:
		case ACT_DIEFORWARD:
		case ACT_DIEVIOLENT:
		case ACT_DIE_HEADSHOT:
		case ACT_DIE_CHESTSHOT:
		case ACT_DIE_GUTSHOT:
		case ACT_DIE_BACKSHOT:
			if (m_deathAnim)
				sequence = LookupSequence(STRING(m_deathAnim));
			break;
		case (Activity)(ACT_LASTACTIVITY + 1): sequence = LookupSequence("pistol_popout_right01"); break;
		case (Activity)(ACT_LASTACTIVITY + 2): sequence = LookupSequence("pistol_popout_right02"); break;
		case (Activity)(ACT_LASTACTIVITY + 3): sequence = LookupSequence("choking"); break;
		case (Activity)(ACT_LASTACTIVITY + 4): sequence = LookupSequence("stunned01"); break;
		default: break;
		}
		if (sequence == -1)
			sequence = LookupActivity(activity);
	}
	m_Activity = activity;
	if (sequence < 0)
	{
		ALERT(at_console, "%s has no sequence for act:%d\n", STRING(pev->classname), activity);
		pev->sequence = 0;
	}
	else
	{
		if (sequence != pev->sequence || m_fSequenceLoops)
			pev->frame = 0;
		pev->sequence = sequence;
		ResetSequenceInfo();
		SetYawSpeed();
	}
}

int CCounterTerrorist::GetWeaponEffectSchedule()
{
	if (m_iWeaponEffectType == 7 && m_bWeaponEffectActive)
	{
		m_iWeaponEffectType = 0;
		m_bWeaponEffectActive = FALSE;
		if (FOkToSpeak())
		{
			SayNamedSentence("CT_INSMOKE", 0.8, ATTN_NORM, 0, m_voicePitch);
			JustSpoke(6);
		}
		int choice = RANDOM_LONG(0, 99);
		if (choice <= 59)
			return SCHED_CT_IN_SMOKE;
		if (choice <= 89)
			return SCHED_SMALL_FLINCH;
	}
	if (m_iWeaponEffectType == 4 && m_bWeaponEffectActive)
	{
		m_iWeaponEffectType = 0;
		m_bWeaponEffectActive = FALSE;
		if (FOkToSpeak())
		{
			SayNamedSentence("CT_FLASHED", 0.8, ATTN_NORM, 0, m_voicePitch);
			JustSpoke();
		}
		return SCHED_CT_FLASHED;
	}
	return -1;
}

int CCounterTerrorist::GetFlyingSchedule()
{
	if (pev->movetype != MOVETYPE_FLY || m_MonsterState == MONSTERSTATE_PRONE)
		return -1;
	if (pev->flags & FL_ONGROUND)
	{
		pev->takedamage = DAMAGE_YES;
		pev->movetype = MOVETYPE_STEP;
		return SCHED_CT_REPEL_LAND;
	}
	pev->takedamage = DAMAGE_NO;
	return m_MonsterState == MONSTERSTATE_COMBAT ? SCHED_CT_REPEL_ATTACK : SCHED_CT_REPEL;
}

int CCounterTerrorist::GetHearingSchedule()
{
	if (!HasConditions(bits_COND_HEAR_SOUND))
		return -1;
	CSound* sound = PBestSound();
	if (!sound)
		return -1;
	if (sound->m_iType & bits_SOUND_DANGER)
		return SCHED_TAKE_COVER_FROM_BEST_SOUND;
	if (!HasConditions(bits_COND_SEE_ENEMY) && (sound->m_iType & (bits_SOUND_COMBAT | bits_SOUND_PLAYER)))
		MakeIdealYaw(sound->m_vecOrigin);
	return -1;
}

int CCounterTerrorist::GetForcedTargetSchedule()
{
	if (!m_forcedTarget)
		return -1;
	CheckAmmo();
	if (HasConditions(bits_COND_NO_AMMO_LOADED))
		return SCHED_RELOAD;
	return RANDOM_LONG(0, 100) < 31 ? SCHED_CT_WAIT_FACE_ENEMY : SCHED_FORCED_TARGET;
}

void CCounterTerrorist::HandleSpeaking(int schedule)
{
	if (!FOkToSpeak())
		return;
	const char* sentence = NULL;
	float delay = -1;
	switch (schedule)
	{
	case SCHED_TAKE_COVER_FROM_ENEMY:
		if (RANDOM_LONG(0, 1)) { sentence = "CT_COVER"; delay = 5; }
		break;
	case SCHED_TAKE_COVER_FROM_BEST_SOUND: sentence = "CT_GREN"; break;
	case SCHED_RANGE_ATTACK1:
		if (RANDOM_LONG(0, 99) > 50) sentence = "CT_ATTACK";
		break;
	case SCHED_RANGE_ATTACK2: sentence = "CT_THROW"; break;
	case SCHED_STANDOFF:
		if (RANDOM_LONG(0, 99) > 50) { sentence = "CT_TAUNT"; delay = 7; }
		break;
	case SCHED_CT_ESTABLISH_LINE_OF_FIRE:
		if (RANDOM_LONG(0, 1)) { sentence = "CT_MOVING"; delay = 5; }
		break;
	case SCHED_CT_CLOSEIN:
		if (RANDOM_LONG(0, 1)) { sentence = "CT_CHARGE"; delay = 5; }
		break;
	default: break;
	}
	if (sentence)
	{
		SayNamedSentence(sentence, 0.8, ATTN_NORM, 0, m_voicePitch);
		JustSpoke(delay);
	}
	if (m_hEnemy && m_hEnemy->IsPlayer() && RANDOM_LONG(0, 99) > 75)
	{
		SayNamedSentence("CT_MAD", 0.8, ATTN_NORM, 0, m_voicePitch);
		JustSpoke(6);
	}
}

Schedule_t* CCounterTerrorist::HandleIdleHeadWatch()
{
	if (m_flStareTime > gpGlobals->time || !HasConditions(bits_COND_SEE_CLIENT) || RANDOM_LONG(0, 99) <= 80)
		return NULL;
	CBaseEntity* player = CBaseEntity::Instance(INDEXENT(1));
	if (!player || (player->pev->origin - pev->origin).Length2D() >= 256)
		return NULL;
	m_hTalkTarget = player;
	m_flStopTalkTime = gpGlobals->time + RANDOM_FLOAT(4, 6);
	m_flStareTime = gpGlobals->time + 12;
	return slCTIdleEyecontact;
}

Schedule_t* CCounterTerrorist::GetScheduleOfType(int type)
{
	switch (type)
	{
	case SCHED_IDLE_STAND:
		{
			if (m_flStopTalkTime > gpGlobals->time)
				return slCTIdleEyecontact;
			Schedule_t* schedule = HandleIdleHeadWatch();
			if (schedule)
				return schedule;
		}
		break;
	case SCHED_COMBAT_FACE: return slCTCombatFace;
	case SCHED_VICTORY_DANCE: return InSquad() && !IsLeader() ? slCTFail : slCTVictoryDance;
	case SCHED_TARGET_FACE: return slCTFaceTarget;
	case SCHED_TARGET_CHASE: return slCTFollow;
	case SCHED_TAKE_COVER_FROM_ENEMY:
	case SCHED_TAKE_COVER_LIGHT_DAMAGE:
	case SCHED_TAKE_COVER_HEAVY_DAMAGE: return slCTTakeCover;
	case SCHED_TAKE_COVER_FROM_BEST_SOUND: return slCTTakeCoverFromBestSound;
	case SCHED_MELEE_ATTACK1:
		taskFailCount = 0;
		break;
	case SCHED_RANGE_ATTACK1:
		taskFailCount = 0;
		if (m_ctBehavior == 0)
			return slCTRangeAttack1C;
		if (RANDOM_LONG(0, 99) <= 49)
		{
			if (m_ctBehavior != 3)
				return slCTRangeAttack1C;
		}
		else if (m_ctBehavior != 3)
		{
			if (RANDOM_LONG(0, 9) <= 4)
				m_bStanding = RANDOM_LONG(0, 1);
			return m_bStanding ? slCTRangeAttack1B : slCTRangeAttack1A;
		}
		m_bStanding = TRUE;
		return slCTRangeAttackMG2;
	case SCHED_RANGE_ATTACK2: return slCTRangeAttack2;
	case SCHED_FAIL: return m_hEnemy ? slCTCombatFail : slCTFail;
	case SCHED_WANDER: return slCTWander;
	case SCHED_CT_SUPPRESS:
		if (m_hEnemy->IsPlayer() && m_fFirstEncounter)
		{
			m_fFirstEncounter = FALSE;
			return slCTSignalSuppress;
		}
		return slCTSuppress;
	case SCHED_CT_ESTABLISH_LINE_OF_FIRE:
		if (m_ctBehavior == 4)
			LaserGlowOff();
		return slCTEstablishLineOfFire;
	case SCHED_CT_COVER_AND_RELOAD: return slCTHideReload;
	case SCHED_CT_SWEEP: return slCTSweep;
	case SCHED_CT_FOUND_ENEMY: return slCTFoundEnemy;
	case SCHED_CT_REPEL:
		if (pev->velocity.z > -128)
			pev->velocity.z -= 32;
		return slCTRepel;
	case SCHED_CT_REPEL_ATTACK:
		if (pev->velocity.z > -128)
			pev->velocity.z -= 32;
		return slCTRepelAttack;
	case SCHED_CT_REPEL_LAND: return slCTRepelLand;
	case SCHED_CT_WAIT_FACE_ENEMY: return slCTWaitInCover;
	case SCHED_CT_TAKECOVER_FAILED:
	case SCHED_CT_ELOF_FAIL:
		if (type == SCHED_CT_ELOF_FAIL)
		{
			++taskFailCount;
			m_bStanding = TRUE;
		}
		if (m_ctBehavior == 4)
		{
			if (HasConditions(bits_COND_CAN_RANGE_ATTACK1) && scheduleTable[SCHED_CT_SNIPER_ATTACK].percentChance > 0)
				return GetScheduleOfType(SCHED_CT_SNIPER_ATTACK);
		}
		else
		{
			if (HasConditions(bits_COND_CAN_MELEE_ATTACK1) && scheduleTable[SCHED_MELEE_ATTACK1].percentChance > 0)
				return GetScheduleOfType(SCHED_MELEE_ATTACK1);
			if (HasConditions(bits_COND_CAN_RANGE_ATTACK2) && scheduleTable[SCHED_RANGE_ATTACK2].percentChance > 0)
				return GetScheduleOfType(SCHED_RANGE_ATTACK2);
			if (HasConditions(bits_COND_CAN_RANGE_ATTACK1) && scheduleTable[SCHED_RANGE_ATTACK1].percentChance > 0)
				return GetScheduleOfType(SCHED_RANGE_ATTACK1);
			if (type == SCHED_CT_TAKECOVER_FAILED)
			{
				if (scheduleTable[SCHED_CT_ESTABLISH_LINE_OF_FIRE].percentChance > 0)
					return GetScheduleOfType(SCHED_CT_ESTABLISH_LINE_OF_FIRE);
				if (scheduleTable[SCHED_CT_CLOSEIN].percentChance > 0)
					return GetScheduleOfType(SCHED_CT_CLOSEIN);
			}
		}
		if (type == SCHED_CT_TAKECOVER_FAILED)
			return GetScheduleOfType(SCHED_CT_WAIT_FACE_ENEMY);
		return GetScheduleOfType(taskFailCount <= 1 ? SCHED_TAKE_COVER_FROM_ENEMY : SCHED_CT_WAIT_CROUCHED);
	case SCHED_CT_IN_SMOKE: return slCTInSmoke;
	case SCHED_CT_FLASHED: return slCTFlashed;
	case SCHED_CT_SNIPER_ATTACK:
		taskFailCount = 0;
		m_bStanding = TRUE;
		return slCTSniperAttack;
	case SCHED_CT_SNIPER_TAKESHOT:
		if (m_hEnemy)
		{
			LaserGlowOff();
			m_hEnemy = NULL;
		}
		return slCTSniperTakeShot;
	case SCHED_CT_SPECIAL_COVER_NODE:
		if (m_pSpecialCoverNode->m_nodeType & 1)
		{
			m_bStanding = TRUE;
			return slCTDuckCoverNode;
		}
		if (m_pSpecialCoverNode->m_nodeType & 6)
			m_bStanding = TRUE;
		return slCTCornerCoverNode;
	case SCHED_CT_DUCK_NODE_ATTACK: return slCTDuckNodeAttack;
	case SCHED_CT_CLOSEIN: return slCTCloseIn;
	case SCHED_CT_CORNER_NODE_ATTACK: return slCTCornerNodeAttack;
	case SCHED_CT_FIRE_PAUSE: return slCTFiringPause;
	case SCHED_CT_CANT_FOLLOW:
		m_bFollowStuck = TRUE;
		return slCTStopFollowing;
	case SCHED_CT_WAIT_CROUCHED: return slCTWaitCrouched;
	default: break;
	}
	return CSquadMonster::GetScheduleOfType(type);
}

Schedule_t* CCounterTerrorist::GetSchedule()
{
	m_iSentence = -1;
	if (!INDEXENT(1))
		ENT(0);
	if (m_flStopTalkTime > gpGlobals->time)
		return GetScheduleOfType(SCHED_IDLE_STAND);
	int flying = GetFlyingSchedule();
	if (flying != -1)
		return GetScheduleOfType(flying);
	if (m_ctBehavior == 4)
		LaserGlowOff();
	float enemyDistance = 0;
	if (m_hEnemy)
	{
		enemyDistance = (pev->origin - m_hEnemy->pev->origin).Length2D();
		m_IdealMonsterState = MONSTERSTATE_COMBAT;
	}
	ClearScheduleTableFlags();
	ActivateTableEntry(GetForcedTargetSchedule());
	ActivateTableEntry(GetWeaponEffectSchedule());
	ActivateTableEntry(GetFlyingSchedule());
	ActivateTableEntry(GetHearingSchedule());
	if (m_MonsterState == MONSTERSTATE_COMBAT)
	{
		if (HasConditions(bits_COND_ENEMY_DEAD) && RANDOM_LONG(0, 99) > 66 && FOkToSpeak())
		{
			SayNamedSentence("CT_KILL", 0.8, ATTN_NORM, 0, m_voicePitch);
			JustSpoke(3);
		}
		if (m_cantMove)
			scheduleTable[SCHED_CT_WAIT_FACE_ENEMY].activeFlag = 1;
		if (HasConditions(bits_COND_ENEMY_DEAD))
			return CBaseMonster::GetSchedule();
		if (HasConditions(bits_COND_NO_AMMO_LOADED))
		{
			scheduleTable[SCHED_CT_COVER_AND_RELOAD].activeFlag = 1;
			int choice = GetScheduleFromTable();
			HandleSpeaking(choice);
			if (choice != -1)
			{
				Schedule_t* schedule = GetScheduleOfType(choice);
				ALERT(at_aiconsole, "%s Playing Schedule: %s\n", STRING(pev->classname), schedule->pName);
				return schedule;
			}
		}
		if (HasConditions(bits_COND_NEW_ENEMY))
		{
			if (FOkToSpeak())
			{
				SayNamedSentence("CT_ENEMY", 0.8, ATTN_NORM, 0, m_voicePitch);
				JustSpoke();
			}
			if (!m_cantMove)
			{
				scheduleTable[SCHED_CT_ESTABLISH_LINE_OF_FIRE].activeFlag = 1;
				if (enemyDistance > m_fPreferedRange)
					scheduleTable[SCHED_CT_CLOSEIN].activeFlag = 1;
				scheduleTable[SCHED_TAKE_COVER_FROM_ENEMY].activeFlag = 1;
			}
		}
		if (HasConditions(bits_COND_LIGHT_DAMAGE))
		{
			scheduleTable[SCHED_SMALL_FLINCH].activeFlag = 1;
			scheduleTable[SCHED_TAKE_COVER_LIGHT_DAMAGE].activeFlag = 1;
		}
		if (HasConditions(bits_COND_HEAVY_DAMAGE))
		{
			scheduleTable[SCHED_SMALL_FLINCH].activeFlag = 1;
			scheduleTable[SCHED_TAKE_COVER_HEAVY_DAMAGE].activeFlag = 1;
		}
		if (HasConditions(bits_COND_CAN_MELEE_ATTACK1))
			scheduleTable[SCHED_MELEE_ATTACK1].activeFlag = 1;
		if (HasConditions(bits_COND_CAN_RANGE_ATTACK1))
		{
			if (InSquad() && MySquadLeader()->m_fEnemyEluded && !HasConditions(bits_COND_ENEMY_FACING_ME))
			{
				MySquadLeader()->m_fEnemyEluded = FALSE;
				return GetScheduleOfType(SCHED_CT_FOUND_ENEMY);
			}
			if (enemyDistance < 128 || m_ctBehavior != 7)
				scheduleTable[SCHED_RANGE_ATTACK1].activeFlag = 1;
			if (justShotFlag)
				scheduleTable[SCHED_CT_FIRE_PAUSE].activeFlag = 1;
			justShotFlag = FALSE;
			scheduleTable[SCHED_CT_SNIPER_ATTACK].activeFlag = 1;
			if (!m_cantMove)
			{
				scheduleTable[SCHED_TAKE_COVER_FROM_ENEMY].activeFlag = 1;
				if (enemyDistance > m_fPreferedRange)
					scheduleTable[SCHED_CT_CLOSEIN].activeFlag = 1;
			}
		}
		if (HasConditions(bits_COND_CAN_RANGE_ATTACK2))
			scheduleTable[SCHED_RANGE_ATTACK2].activeFlag = 1;
		if (HasConditions(bits_COND_ENEMY_OCCLUDED))
		{
			scheduleTable[SCHED_CT_SNIPER_TAKESHOT].activeFlag = 1;
			if (!m_cantMove)
			{
				scheduleTable[SCHED_CT_ESTABLISH_LINE_OF_FIRE].activeFlag = 1;
				scheduleTable[SCHED_CT_CLOSEIN].activeFlag = 1;
			}
			scheduleTable[SCHED_STANDOFF].activeFlag = 1;
		}
		if (!HasConditions(bits_COND_CAN_RANGE_ATTACK1)
			&& (!HasConditions(bits_COND_CAN_MELEE_ATTACK1) || scheduleTable[SCHED_MELEE_ATTACK1].percentChance == 0)
			&& !m_cantMove)
		{
			scheduleTable[SCHED_CT_ESTABLISH_LINE_OF_FIRE].activeFlag = 1;
			if (enemyDistance > m_fPreferedRange)
				scheduleTable[SCHED_CT_CLOSEIN].activeFlag = 1;
		}
		m_pSpecialCoverNode = HasValidSpecialCoverNode();
		if (m_pSpecialCoverNode && enemyDistance > (pev->origin - m_pSpecialCoverNode->m_vecOrigin).Length2D() * 0.75f
			&& !m_cantMove)
			scheduleTable[SCHED_CT_SPECIAL_COVER_NODE].activeFlag = 1;
	}
	else
	{
		Schedule_t* following = HandleFollowing();
		if (following)
			return following;
		if (m_pCine && m_MonsterState == MONSTERSTATE_IDLE)
		{
			m_IdealMonsterState = MONSTERSTATE_SCRIPT;
			return GetScheduleOfType(SCHED_AISCRIPT);
		}
		if (!m_hEnemy && gpGlobals->time > m_wanderTime && m_canWander && !m_cantMove)
		{
			float distance = RANDOM_FLOAT(64, 256);
			m_vecMoveGoal.x += RANDOM_LONG((int)-distance, (int)distance);
			m_vecMoveGoal.y += RANDOM_LONG((int)-distance, (int)distance);
			m_vecMoveGoal.z = pev->origin.z;
			m_wanderOrigin = m_vecMoveGoal;
			m_wanderTime = gpGlobals->time + RANDOM_FLOAT(8, 15);
			return GetScheduleOfType(SCHED_WANDER);
		}
	}
	int choice = GetScheduleFromTable();
	HandleSpeaking(choice);
	if (choice == -1)
		return CBaseMonster::GetSchedule();
	Schedule_t* schedule = GetScheduleOfType(choice);
	ALERT(at_aiconsole, "%s Playing Schedule: %s\n", STRING(pev->classname), schedule->pName);
	return schedule;
}

class CCounterTerroristSwat : public CCounterTerrorist
{
public:
	void Precache() override;
	void SpawnInit() override;
};

void CCounterTerroristSwat::Precache()
{
	PRECACHE_MODEL(pev->model ? (char*)STRING(pev->model) : "models/seal.mdl");
	CCounterTerrorist::Precache();
}

void CCounterTerroristSwat::SpawnInit()
{
	SET_MODEL(edict(), pev->model ? STRING(pev->model) : "models/seal.mdl");
	m_ctType = 0;
	if (!m_language)
		m_language = ALLOC_STRING("AM");
}

LINK_ENTITY_TO_CLASS(monster_counter_terrorist_swat, CCounterTerroristSwat);
LINK_ENTITY_TO_CLASS(monster_ct_swat, CCounterTerroristSwat);
LINK_ENTITY_TO_CLASS(monster_ct_swat_pistol, CCounterTerroristSwat);
LINK_ENTITY_TO_CLASS(monster_ct_swat_shotgun, CCounterTerroristSwat);
LINK_ENTITY_TO_CLASS(monster_ct_swat_assaultrifle, CCounterTerroristSwat);
LINK_ENTITY_TO_CLASS(monster_ct_swat_machinegun, CCounterTerroristSwat);
LINK_ENTITY_TO_CLASS(monster_ct_swat_sniperrifle, CCounterTerroristSwat);
LINK_ENTITY_TO_CLASS(monster_ct_swat_law, CCounterTerroristSwat);
LINK_ENTITY_TO_CLASS(monster_ct_swat_smg, CCounterTerroristSwat);
LINK_ENTITY_TO_CLASS(monster_ct_swat_grenader, CCounterTerroristSwat);
LINK_ENTITY_TO_CLASS(monster_ct_swat_mp5, CCounterTerroristSwat);

class CCounterTerroristGIGN : public CCounterTerrorist
{
public:
	void Precache() override;
	void SpawnInit() override;
};

void CCounterTerroristGIGN::Precache()
{
	PRECACHE_MODEL(pev->model ? (char*)STRING(pev->model) : "models/gign.mdl");
	CCounterTerrorist::Precache();
}

void CCounterTerroristGIGN::SpawnInit()
{
	SET_MODEL(edict(), pev->model ? STRING(pev->model) : "models/gign.mdl");
	m_ctType = 1;
	if (!m_language)
		m_language = ALLOC_STRING("FR");
}

LINK_ENTITY_TO_CLASS(monster_counter_terrorist_gign, CCounterTerroristGIGN);
LINK_ENTITY_TO_CLASS(monster_ct_gign, CCounterTerroristGIGN);
LINK_ENTITY_TO_CLASS(monster_ct_gign_pistol, CCounterTerroristGIGN);
LINK_ENTITY_TO_CLASS(monster_ct_gign_shotgun, CCounterTerroristGIGN);
LINK_ENTITY_TO_CLASS(monster_ct_gign_assaultrifle, CCounterTerroristGIGN);
LINK_ENTITY_TO_CLASS(monster_ct_gign_machinegun, CCounterTerroristGIGN);
LINK_ENTITY_TO_CLASS(monster_ct_gign_sniperrifle, CCounterTerroristGIGN);
LINK_ENTITY_TO_CLASS(monster_ct_gign_law, CCounterTerroristGIGN);
LINK_ENTITY_TO_CLASS(monster_ct_gign_smg, CCounterTerroristGIGN);
LINK_ENTITY_TO_CLASS(monster_ct_gign_grenader, CCounterTerroristGIGN);
LINK_ENTITY_TO_CLASS(monster_ct_gign_mp5, CCounterTerroristGIGN);

class CCounterTerroristGSG9 : public CCounterTerrorist
{
public:
	void Precache() override;
	void SpawnInit() override;
};

void CCounterTerroristGSG9::Precache()
{
	PRECACHE_MODEL(pev->model ? (char*)STRING(pev->model) : "models/gsg9.mdl");
	CCounterTerrorist::Precache();
}

void CCounterTerroristGSG9::SpawnInit()
{
	SET_MODEL(edict(), pev->model ? STRING(pev->model) : "models/gsg9.mdl");
	m_ctType = 2;
	if (!m_language)
		m_language = ALLOC_STRING("GE");
}

LINK_ENTITY_TO_CLASS(monster_counter_terrorist_gsg9, CCounterTerroristGSG9);
LINK_ENTITY_TO_CLASS(monster_ct_gsg9, CCounterTerroristGSG9);
LINK_ENTITY_TO_CLASS(monster_ct_gsg9_pistol, CCounterTerroristGSG9);
LINK_ENTITY_TO_CLASS(monster_ct_gsg9_shotgun, CCounterTerroristGSG9);
LINK_ENTITY_TO_CLASS(monster_ct_gsg9_assaultrifle, CCounterTerroristGSG9);
LINK_ENTITY_TO_CLASS(monster_ct_gsg9_machinegun, CCounterTerroristGSG9);
LINK_ENTITY_TO_CLASS(monster_ct_gsg9_sniperrifle, CCounterTerroristGSG9);
LINK_ENTITY_TO_CLASS(monster_ct_gsg9_law, CCounterTerroristGSG9);
LINK_ENTITY_TO_CLASS(monster_ct_gsg9_smg, CCounterTerroristGSG9);
LINK_ENTITY_TO_CLASS(monster_ct_gsg9_grenader, CCounterTerroristGSG9);
LINK_ENTITY_TO_CLASS(monster_ct_gsg9_mp5, CCounterTerroristGSG9);

class CCounterTerroristSpetsnaz : public CCounterTerrorist
{
public:
	void Precache() override;
	void SpawnInit() override;
};

void CCounterTerroristSpetsnaz::Precache()
{
	PRECACHE_MODEL(pev->model ? (char*)STRING(pev->model) : "models/spetsnaz.mdl");
	CCounterTerrorist::Precache();
}

void CCounterTerroristSpetsnaz::SpawnInit()
{
	SET_MODEL(edict(), pev->model ? STRING(pev->model) : "models/spetsnaz.mdl");
	m_ctType = 3;
	if (!m_language)
		m_language = ALLOC_STRING("RU");
}

LINK_ENTITY_TO_CLASS(monster_counter_terrorist_spetsnaz, CCounterTerroristSpetsnaz);
LINK_ENTITY_TO_CLASS(monster_ct_spetsnaz, CCounterTerroristSpetsnaz);
LINK_ENTITY_TO_CLASS(monster_ct_spetsnaz_pistol, CCounterTerroristSpetsnaz);
LINK_ENTITY_TO_CLASS(monster_ct_spetsnaz_shotgun, CCounterTerroristSpetsnaz);
LINK_ENTITY_TO_CLASS(monster_ct_spetsnaz_assaultrifle, CCounterTerroristSpetsnaz);
LINK_ENTITY_TO_CLASS(monster_ct_spetsnaz_machinegun, CCounterTerroristSpetsnaz);
LINK_ENTITY_TO_CLASS(monster_ct_spetsnaz_sniperrifle, CCounterTerroristSpetsnaz);
LINK_ENTITY_TO_CLASS(monster_ct_spetsnaz_law, CCounterTerroristSpetsnaz);
LINK_ENTITY_TO_CLASS(monster_ct_spetsnaz_smg, CCounterTerroristSpetsnaz);
LINK_ENTITY_TO_CLASS(monster_ct_spetsnaz_grenader, CCounterTerroristSpetsnaz);
LINK_ENTITY_TO_CLASS(monster_ct_spetsnaz_mp5, CCounterTerroristSpetsnaz);

class CCounterTerroristSAS : public CCounterTerrorist
{
public:
	void Precache() override;
	void SpawnInit() override;
};

void CCounterTerroristSAS::Precache()
{
	PRECACHE_MODEL(pev->model ? (char*)STRING(pev->model) : "models/sas.mdl");
	CCounterTerrorist::Precache();
}

void CCounterTerroristSAS::SpawnInit()
{
	SET_MODEL(edict(), pev->model ? STRING(pev->model) : "models/sas.mdl");
	m_ctType = 4;
	if (!m_language)
		m_language = ALLOC_STRING("BR");
}

LINK_ENTITY_TO_CLASS(monster_counter_terrorist_sas, CCounterTerroristSAS);
LINK_ENTITY_TO_CLASS(monster_ct_sas, CCounterTerroristSAS);
LINK_ENTITY_TO_CLASS(monster_ct_sas_pistol, CCounterTerroristSAS);
LINK_ENTITY_TO_CLASS(monster_ct_sas_shotgun, CCounterTerroristSAS);
LINK_ENTITY_TO_CLASS(monster_ct_sas_assaultrifle, CCounterTerroristSAS);
LINK_ENTITY_TO_CLASS(monster_ct_sas_machinegun, CCounterTerroristSAS);
LINK_ENTITY_TO_CLASS(monster_ct_sas_sniperrifle, CCounterTerroristSAS);
LINK_ENTITY_TO_CLASS(monster_ct_sas_law, CCounterTerroristSAS);
LINK_ENTITY_TO_CLASS(monster_ct_sas_smg, CCounterTerroristSAS);
LINK_ENTITY_TO_CLASS(monster_ct_sas_grenader, CCounterTerroristSAS);
LINK_ENTITY_TO_CLASS(monster_ct_sas_mp5, CCounterTerroristSAS);

class CCounterTerroristRepel : public CBaseMonster
{
public:
	void Spawn() override;
	void Precache() override;
	void KeyValue(KeyValueData* data) override;
	int Save(CSave& save) override;
	int Restore(CRestore& restore) override;
	void EXPORT RepelUse(CBaseEntity* activator, CBaseEntity* caller, USE_TYPE useType, float value);
	int m_iSpriteTexture;
	static TYPEDESCRIPTION m_SaveData[];
	int m_iSkin;
	int m_iHead;
	string_t m_classname;
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

LINK_ENTITY_TO_CLASS(monster_counter_terrorist_repel, CCounterTerroristRepel);
LINK_ENTITY_TO_CLASS(monster_ct_repel, CCounterTerroristRepel);

TYPEDESCRIPTION CCounterTerroristRepel::m_SaveData[] =
{
	DEFINE_FIELD(CCounterTerroristRepel, m_iSkin, FIELD_INTEGER),
	DEFINE_FIELD(CCounterTerroristRepel, m_iHead, FIELD_INTEGER),
	DEFINE_FIELD(CCounterTerroristRepel, m_classname, FIELD_STRING),
};

IMPLEMENT_SAVERESTORE(CCounterTerroristRepel, CBaseMonster);

void CCounterTerroristRepel::KeyValue(KeyValueData* data)
{
	if (FStrEq(data->szKeyName, "repelskin"))
	{
		m_iSkin = atoi(data->szValue);
		data->fHandled = TRUE;
	}
	else if (FStrEq(data->szKeyName, "repelhead"))
	{
		m_iHead = atoi(data->szValue);
		data->fHandled = TRUE;
	}
	else if (FStrEq(data->szKeyName, "monstertype"))
	{
		m_classname = ALLOC_STRING(data->szValue);
		data->fHandled = TRUE;
	}
	else
		CBaseMonster::KeyValue(data);
}

void CCounterTerroristRepel::Precache()
{
	if (!m_classname)
		m_classname = ALLOC_STRING("monster_ct_sas_mp5");
	UTIL_PrecacheOther(STRING(m_classname));
	PRECACHE_SOUND("common/body_zipdrop.wav");
	PRECACHE_SOUND("common/body_ziploop.wav");
	m_iSpriteTexture = PRECACHE_MODEL("sprites/rope.spr");
}

void CCounterTerroristRepel::Spawn()
{
	Precache();
	pev->solid = SOLID_NOT;
	SetUse(&CCounterTerroristRepel::RepelUse);
}

void CCounterTerroristRepel::RepelUse(CBaseEntity* activator, CBaseEntity* caller, USE_TYPE useType, float value)
{
	TraceResult trace;
	UTIL_TraceLine(pev->origin, pev->origin + Vector(0, 0, -4096), dont_ignore_monsters, edict(), &trace);
	edict_t* created = CREATE_NAMED_ENTITY(m_classname);
	if (FNullEnt(created))
	{
		ALERT(at_console, "NULL Ent in Create!\n");
		return;
	}
	CBaseEntity* entity = CBaseEntity::Instance(created);
	CBaseMonster* monster = entity->MyMonsterPointer();
	entity->pev->owner = NULL;
	entity->pev->origin = pev->origin;
	entity->pev->angles = pev->angles;
	monster->m_head = m_iHead;
	DispatchSpawn(entity->edict());
	monster->pev->health = 200;
	monster->pev->movetype = MOVETYPE_FLY;
	monster->pev->velocity = Vector(0, 0, RANDOM_FLOAT(-196, -128));
	monster->pev->skin = m_iSkin;
	monster->SetActivity(ACT_GLIDE);
	monster->m_vecLastPosition = trace.vecEndPos;
	CBeam* beam = CBeam::BeamCreate("sprites/rope.spr", 10);
	beam->PointEntInit(pev->origin + Vector(0, 0, 112), monster->entindex());
	beam->SetFlags(BEAM_FSOLID);
	beam->SetColor(255, 255, 255);
	beam->SetThink(&CBaseEntity::SUB_Remove);
	beam->pev->nextthink = gpGlobals->time + -4096.0 * trace.flFraction / monster->pev->velocity.z + 0.5;
	UTIL_Remove(this);
}

class CDeadCT : public CBaseMonster
{
public:
	void Spawn() override;
	void KeyValue(KeyValueData* data) override;
	int Classify() override;
	int m_iPose;
	static const char* m_szPoses[];
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

LINK_ENTITY_TO_CLASS(monster_ct_dead, CDeadCT);

const char* CDeadCT::m_szPoses[] = { "deadstomach", "deadside", "deadsitting" };

int CDeadCT::Classify()
{
	return CLASS_HUMAN_MILITARY;
}

void CDeadCT::KeyValue(KeyValueData* data)
{
	if (FStrEq(data->szKeyName, "pose"))
	{
		m_iPose = atoi(data->szValue);
		data->fHandled = TRUE;
	}
	else
		CBaseMonster::KeyValue(data);
}

void CDeadCT::Spawn()
{
	PRECACHE_MODEL("models/hgrunt.mdl");
	SET_MODEL(edict(), "models/hgrunt.mdl");
	pev->effects = 0;
	pev->yaw_speed = 8;
	pev->sequence = 0;
	m_bloodColor = BLOOD_COLOR_RED;
	pev->sequence = LookupSequence(m_szPoses[m_iPose]);
	if (pev->sequence == -1)
		ALERT(at_console, "Dead terrorist with bad pose\n");
	pev->health = 8;
	switch (pev->body)
	{
	case 0:
		pev->skin = 0;
		SetBodygroup(1, 0);
		SetBodygroup(2, 0);
		break;
	case 1:
		pev->body = 0;
		pev->skin = 0;
		SetBodygroup(1, 1);
		SetBodygroup(2, 0);
		break;
	case 2:
		pev->body = 0;
		pev->skin = 0;
		SetBodygroup(1, 0);
		SetBodygroup(2, 8);
		break;
	case 3:
		pev->body = 0;
		pev->skin = 0;
		SetBodygroup(1, 1);
		SetBodygroup(2, 8);
		break;
	default: break;
	}
	MonsterInitDead();
}

// CZ:DS: the SAS squad of cz_sandstorm2 (train_ct1, train_ct2) is made in cz_sandstorm and goes through the level change.
// When the map is started by itself there is nobody, and the scripts of the map can't find them. Make them then,
// where the level change puts them: the same place relative to the landmark.
static CBaseEntity *CZ_CreateSandstormCT( const char *name, const Vector &origin, float yaw, const char *idleanim )
{
	edict_t *pent = CREATE_NAMED_ENTITY( MAKE_STRING( "monster_counter_terrorist_sas" ));
	if( FNullEnt( pent ))
		return NULL;

	struct { const char *key; const char *value; } keys[] = {
		{ "targetname", name },
		{ "spawnflags", "130" },
		{ "usetarget", "null_usetargetname" },
		{ "skin", "2" },
		{ "head", "1" },
		{ "idleanim", idleanim },
	};

	for( size_t i = 0; i < sizeof( keys ) / sizeof( keys[0] ); i++ )
	{
		if( !keys[i].value || !keys[i].value[0] )
			continue;
		KeyValueData kvd;
		kvd.szClassName = "monster_counter_terrorist_sas";
		kvd.szKeyName = (char *)keys[i].key;
		kvd.szValue = (char *)keys[i].value;
		kvd.fHandled = FALSE;
		DispatchKeyValue( pent, &kvd );
	}

	pent->v.origin = origin;
	pent->v.angles = Vector( 0, yaw, 0 );
	DispatchSpawn( pent );
	return CBaseEntity::Instance( pent );
}

void CZ_SpawnSandstormCTs( void )
{
	if( !FStrEq( STRING( gpGlobals->mapname ), "cz_sandstorm2" ))
		return;

	// they came with the level change
	if( UTIL_FindEntityByTargetname( NULL, "train_ct1" ) || UTIL_FindEntityByTargetname( NULL, "train_ct2" ))
		return;

	ALERT( at_console, "CZ:DS: cz_sandstorm2 started without the level change, making the SAS squad\n" );
	CZ_CreateSandstormCT( "train_ct2", Vector( 1298, 2526, 38 ), 315, NULL );
	CZ_CreateSandstormCT( "train_ct1", Vector( 1312, 2478, 38 ), 45, "idle4" );
}

// CZ:DS: some maps have no weapons of their own: the player brings them through the level change. When such a map is started
// by itself (the map command, restart) the player has nothing, so give him what the earlier maps of the mission gave
// (their game_player_equip, put together). The things must be precached while the map is loading, the player is made later.
struct CZStartKit_t
{
	const char *map;
	struct { const char *name; int count; } items[16];
};

static const CZStartKit_t g_StartKits[] =
{
	{ "cz_silo03", { { "item_armor", 1 }, { "weapon_knife", 1 }, { "ammo_9mm", 2 }, { "weapon_mp5navy", 1 }, { NULL, 0 } } },
	{ "cz_silo04", { { "item_armor", 1 }, { "weapon_knife", 1 }, { "ammo_9mm", 2 }, { "weapon_mp5navy", 1 }, { "weapon_radiocontrolledbomb", 1 }, { NULL, 0 } } },
	{ "cz_downed3", { { "item_armor", 1 }, { "weapon_p228", 1 }, { "weapon_knife", 1 }, { "weapon_hegrenade", 1 }, { "weapon_smokegrenade", 1 }, { "weapon_flashbang", 1 }, { "ammo_556nato", 3 }, { "ammo_357sig", 4 }, { "weapon_famas", 1 }, { NULL, 0 } } },
	{ "cz_druglab3", { { "item_armor", 1 }, { "ammo_57mm", 2 }, { "ammo_buckshot", 4 }, { "weapon_smokegrenade", 1 }, { "weapon_hegrenade", 1 }, { "weapon_flashbang", 1 }, { "weapon_fiveseven", 1 }, { "weapon_knife", 1 }, { "weapon_radiocontrolledbomb", 1 }, { "weapon_blowtorch", 1 }, { "weapon_m3", 1 }, { NULL, 0 } } },
	{ "cz_druglab4", { { "item_armor", 1 }, { "ammo_57mm", 2 }, { "ammo_buckshot", 4 }, { "weapon_smokegrenade", 1 }, { "weapon_hegrenade", 1 }, { "weapon_flashbang", 1 }, { "weapon_fiveseven", 1 }, { "weapon_knife", 1 }, { "weapon_radiocontrolledbomb", 1 }, { "weapon_blowtorch", 1 }, { "weapon_m3", 1 }, { NULL, 0 } } },
	{ "cz_fastline3", { { "ammo_45acp", 7 }, { "ammo_57mm", 2 }, { "weapon_knife", 1 }, { "weapon_shieldgun", 1 }, { NULL, 0 } } },
	{ "cz_fastline4", { { "ammo_45acp", 7 }, { "ammo_57mm", 2 }, { "weapon_knife", 1 }, { "weapon_shieldgun", 1 }, { NULL, 0 } } },
	{ "cz_hankagai3", { { "item_armor", 1 }, { "weapon_knife", 1 }, { "weapon_usp", 1 }, { "weapon_smokegrenade", 1 }, { "ammo_generic", 1 }, { "weapon_aug", 1 }, { NULL, 0 } } },
	{ "cz_hankagai4", { { "item_armor", 1 }, { "weapon_knife", 1 }, { "weapon_usp", 1 }, { "weapon_smokegrenade", 1 }, { "ammo_generic", 1 }, { "weapon_aug", 1 }, { NULL, 0 } } },
	{ "cz_hr07", { { "ammo_50ae", 4 }, { "ammo_556nato", 2 }, { "item_armor", 1 }, { "weapon_knife", 1 }, { "weapon_deagle", 1 }, { "weapon_hegrenade", 1 }, { "weapon_radio", 1 }, { "weapon_m4a1", 1 }, { "weapon_radiocontrolledbomb", 1 }, { "ammo_generic", 1 }, { "weapon_blowtorch", 1 }, { "weapon_scout", 1 }, { NULL, 0 } } },
	{ "cz_lostcause2", { { "weapon_radio", 1 }, { "ammo_45acp", 7 }, { "ammo_57mm", 2 }, { "weapon_fiveseven", 1 }, { "weapon_ump45", 1 }, { "weapon_knife", 1 }, { "weapon_hegrenade", 1 }, { "weapon_fiberopticcamera", 1 }, { NULL, 0 } } },
	{ "cz_motor2", { { "item_armor", 1 }, { "weapon_flashbang", 2 }, { "weapon_knife", 1 }, { "weapon_fiveseven", 1 }, { "ammo_generic", 1 }, { "weapon_aug", 1 }, { "weapon_laws", 1 }, { NULL, 0 } } },
	{ "cz_pipedream2", { { "weapon_radio", 1 }, { "weapon_knife", 1 }, { "weapon_glock18", 1 }, { "ammo_generic", 3 }, { "item_armor", 1 }, { "weapon_radiocontrolledbomb", 2 }, { "weapon_ak47", 1 }, { NULL, 0 } } },
	{ "cz_pipedream3", { { "weapon_radio", 1 }, { "weapon_knife", 1 }, { "weapon_glock18", 1 }, { "ammo_generic", 3 }, { "item_armor", 1 }, { "weapon_radiocontrolledbomb", 2 }, { "weapon_ak47", 1 }, { NULL, 0 } } },
	{ "cz_recoil2", { { "item_armor", 1 }, { "weapon_usp", 1 }, { "weapon_knife", 1 }, { "ammo_45acp", 6 }, { "ammo_556nato", 3 }, { "weapon_m4a1", 1 }, { NULL, 0 } } },
	{ "cz_run2", { { "item_armor", 1 }, { "ammo_generic", 1 }, { "weapon_usp", 1 }, { "weapon_knife", 1 }, { "weapon_scout", 1 }, { NULL, 0 } } },
	{ "cz_run3", { { "item_armor", 1 }, { "ammo_generic", 1 }, { "weapon_usp", 1 }, { "weapon_knife", 1 }, { "weapon_scout", 1 }, { NULL, 0 } } },
	{ "cz_sandstorm3", { { "ammo_556nato", 3 }, { "weapon_deagle", 1 }, { "weapon_knife", 1 }, { "weapon_radiocontrolledbomb", 1 }, { "item_armor", 1 }, { "weapon_m4a1", 1 }, { NULL, 0 } } },
	{ "cz_thinice02", { { "item_armor", 1 }, { "weapon_radiocontrolledbomb", 1 }, { "weapon_blowtorch", 1 }, { "weapon_knife", 1 }, { "ammo_357sig", 2 }, { "weapon_p228", 1 }, { NULL, 0 } } },
	{ "cz_thinice03", { { "item_armor", 1 }, { "weapon_radiocontrolledbomb", 1 }, { "weapon_blowtorch", 1 }, { "weapon_knife", 1 }, { "ammo_357sig", 2 }, { "weapon_p228", 1 }, { NULL, 0 } } },
	{ "cz_truth3", { { "weapon_radio", 1 }, { "weapon_knife", 1 }, { "ammo_generic", 1 }, { "weapon_smokegrenade", 1 }, { "weapon_usp", 1 }, { "weapon_flashbang", 1 }, { "item_armor", 1 }, { "weapon_m4a1", 1 }, { NULL, 0 } } },
	{ "cz_truth4", { { "weapon_radio", 1 }, { "weapon_knife", 1 }, { "ammo_generic", 1 }, { "weapon_smokegrenade", 1 }, { "weapon_usp", 1 }, { "weapon_flashbang", 1 }, { "item_armor", 1 }, { "weapon_m4a1", 1 }, { NULL, 0 } } },
	{ "cz_turncrank3", { { "weapon_briefcase", 1 }, { "item_armor", 1 }, { "ammo_generic", 1 }, { "weapon_p228", 1 }, { NULL, 0 } } },
};

static const CZStartKit_t *CZ_FindStartKit( void )
{
	for( size_t i = 0; i < sizeof( g_StartKits ) / sizeof( g_StartKits[0] ); i++ )
		if( FStrEq( STRING( gpGlobals->mapname ), g_StartKits[i].map ))
			return &g_StartKits[i];
	return NULL;
}

void CZ_PrecacheDirectStart( void )
{
	const CZStartKit_t *kit = CZ_FindStartKit();
	if( kit )
		for( size_t i = 0; kit->items[i].name; i++ )
			UTIL_PrecacheOther( kit->items[i].name );

	if( FStrEq( STRING( gpGlobals->mapname ), "cz_sandstorm2" ))
		UTIL_PrecacheOther( "monster_counter_terrorist_sas" );
}

// the things the level change starts on the map: the player arrives inside a trigger and it fires at once. When the map is
// started otherwise (the map command, restart, loading after death) he stands outside of it and nothing starts.
static bool g_bMapStartEventsDone = false;

void CZ_MapStartReset( void )
{
	g_bMapStartEventsDone = false;
}

void CZ_MapStartEvents( CBasePlayer *pPlayer )
{
	if( g_bMapStartEventsDone )
		return;
	g_bMapStartEventsDone = true;

	// cz_silo04: the trigger at the arrival place starts the run-out; the wall in front of the player is lowered.
	// A trigger that already fired is gone, so this does nothing after the level change.
	if( FStrEq( STRING( gpGlobals->mapname ), "cz_silo04" ))
	{
		CBaseEntity *pTrigger = NULL;
		while(( pTrigger = UTIL_FindEntityByClassname( pTrigger, "trigger_once" )) != NULL )
			if( pTrigger->pev->target && FStrEq( STRING( pTrigger->pev->target ), "Start_runout" ))
				pTrigger->Touch( pPlayer );

		// the wall (func_door t1) goes down at once, not after the 11.5 seconds of the trigger_relay
		FireTargets( "t1", pPlayer, pPlayer, USE_ON, 0 );
	}
}

void CZ_EquipDirectStart( CBasePlayer *pPlayer )
{
	const CZStartKit_t *kit = CZ_FindStartKit();
	if( !kit )
		return;

	// he came with the level change and has his things
	for( size_t i = 0; i < sizeof( pPlayer->m_rgpPlayerItems ) / sizeof( pPlayer->m_rgpPlayerItems[0] ); i++ )
		if( pPlayer->m_rgpPlayerItems[i] )
			return;

	ALERT( at_console, "CZ:DS: %s started without the level change, equipping the player\n", STRING( gpGlobals->mapname ));

	edict_t *pent = CREATE_NAMED_ENTITY( MAKE_STRING( "game_player_equip" ));
	if( FNullEnt( pent ))
		return;

	for( size_t i = 0; kit->items[i].name; i++ )
	{
		char value[16];
		sprintf( value, "%d", kit->items[i].count );
		KeyValueData kvd;
		kvd.szClassName = "game_player_equip";
		kvd.szKeyName = (char *)kit->items[i].name;
		kvd.szValue = value;
		kvd.fHandled = FALSE;
		DispatchKeyValue( pent, &kvd );
	}
	DispatchSpawn( pent );

	CBaseEntity *pEquip = CBaseEntity::Instance( pent );
	if( pEquip )
	{
		pEquip->Use( pPlayer, pPlayer, USE_TOGGLE, 0 );
		UTIL_Remove( pEquip );
	}

}
