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
// hgrunt
//=========================================================

//=========================================================
// Hit groups!	
//=========================================================
/*

  1 - Head
  2 - Stomach
  3 - Gun

*/


#include	"extdll.h"
#include	"plane.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"schedule.h"
#include	"defaultai.h"
#include	"animation.h"
#include	"squadmonster.h"
#include	"weapons.h"
#include	"talkmonster.h"
#include	"soundent.h"
#include	"effects.h"
#include	"customentity.h"
#include	"game.h"

int g_fGruntQuestion2;				// true if an idle grunt asked a question. Cleared when someone answers.

extern DLL_GLOBAL int		g_iSkillLevel;

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define		FASSN_AE_RELOAD		( 2 )
#define		FASSN_AE_KICK			( 3 )
#define		FASSN_AE_BURST1		( 4 )
#define		FASSN_AE_BURST2		( 5 ) 
#define		FASSN_AE_BURST3		( 6 ) 
#define		FASSN_AE_GREN_TOSS		( 7 )
#define		FASSN_AE_GREN_LAUNCH	( 8 )
#define		FASSN_AE_GREN_DROP		( 9 )
#define		FASSN_AE_CAUGHT_ENEMY	( 10) // grunt established sight with an enemy (player only) that had previously eluded the squad.
#define		FASSN_AE_DROP_GUN		( 11) // grunt (probably dead) is dropping his mp5.

//=========================================================
// monster-specific schedule types
//=========================================================
enum
{
	SCHED_GRUNT_SUPPRESS = LAST_COMMON_SCHEDULE + 1,
	SCHED_GRUNT_ESTABLISH_LINE_OF_FIRE,// move to a location to set up an attack against the enemy. (usually when a friendly is in the way).
	SCHED_GRUNT_COVER_AND_RELOAD,
	SCHED_GRUNT_SWEEP,
	SCHED_GRUNT_FOUND_ENEMY,
	SCHED_GRUNT_REPEL,
	SCHED_GRUNT_REPEL_ATTACK,
	SCHED_GRUNT_REPEL_LAND,
	SCHED_GRUNT_WAIT_FACE_ENEMY,
	SCHED_GRUNT_TAKECOVER_FAILED,// special schedule type that forces analysis of conditions and picks the best possible schedule to recover from this type of failure.
	SCHED_GRUNT_ELOF_FAIL,
	SCHED_FASSN_JUMP,	// fly through the air
	SCHED_FASSN_JUMP_ATTACK,	// fly through the air and shoot
	SCHED_FASSN_JUMP_LAND, // hit and run away
};

//=========================================================
// monster-specific tasks
//=========================================================
enum 
{
	TASK_GRUNT_FACE_TOSS_DIR = LAST_COMMON_TASK + 1,
	TASK_GRUNT_SPEAK_SENTENCE,
	TASK_GRUNT_CHECK_FIRE,
	TASK_FASSN_FALL_TO_GROUND,
};

//=========================================================
// monster-specific conditions
//=========================================================
#define bits_COND_GRUNT_NOFIRE	( bits_COND_SPECIAL1 )

#define bits_MEMORY_BADJUMP		(bits_MEMORY_CUSTOM1)

class CFassn : public CSquadMonster
{
public:
	void Spawn( void );
	void Precache( void );
	void SetYawSpeed ( void );
	int  Classify ( void );
	int ISoundMask ( void );
	void HandleAnimEvent( MonsterEvent_t *pEvent );
	BOOL FCanCheckAttacks ( void );
	BOOL CheckMeleeAttack1 ( float flDot, float flDist );
	BOOL CheckRangeAttack1 ( float flDot, float flDist );
	BOOL CheckRangeAttack2 ( float flDot, float flDist );

	void CheckAmmo ( void );
	void RunAI( void );
	void Killed( entvars_t *pevAttacker, int iGib );

	void SetActivity ( Activity NewActivity );
	void StartTask ( Task_t *pTask );
	void RunTask ( Task_t *pTask );
	void DeathSound( void );
	void PainSound( void );
	Vector GetGunPosition( void );
	void Shoot ( void );
	void ShootCrossbow ( void );

	void PrescheduleThink ( void );
	void GibMonster( void );
	void SpeakSentence( void );

	int	Save( CSave &save ); 
	int Restore( CRestore &restore );
	
	CBaseEntity	*Kick( void );
	Schedule_t	*GetSchedule( void );
	Schedule_t  *GetScheduleOfType ( int Type );
	void TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType);
	int TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType );

	int IRelationship ( CBaseEntity *pTarget );

	CUSTOM_SCHEDULES;
	static TYPEDESCRIPTION m_SaveData[];

	// checking the feasibility of a grenade toss is kind of costly, so we do it every couple of seconds,
	// not every server frame.
	float m_flNextGrenadeCheck;
	float m_flNextPainTime;
	float m_flNextSkillTime;
	float m_flNextSkillDelayTime;
	
	float m_flLastEnemySightTime;

	float m_flNextJump;
	Vector m_vecJumpVelocity;

	Vector	m_vecTossVelocity;

	BOOL	m_fThrowGrenade;
	BOOL	m_fFirstEncounter;// only put on the handsign show in the squad's first encounter.

	int m_voicePitch;

	int		m_iBrassShell;
};

LINK_ENTITY_TO_CLASS( monster_human_fassn, CFassn );
LINK_ENTITY_TO_CLASS( monster_human_fassn_fiona, CFassn );

TYPEDESCRIPTION	CFassn::m_SaveData[] = 
{
	DEFINE_FIELD( CFassn, m_flNextGrenadeCheck, FIELD_TIME ),
	DEFINE_FIELD( CFassn, m_flNextPainTime, FIELD_TIME ),
	DEFINE_FIELD( CFassn, m_flNextSkillTime, FIELD_TIME ),
	DEFINE_FIELD( CFassn, m_flNextSkillDelayTime, FIELD_TIME ),
//	DEFINE_FIELD( CFassn, m_flLastEnemySightTime, FIELD_TIME ), // don't save, go to zero
	DEFINE_FIELD( CFassn, m_vecTossVelocity, FIELD_VECTOR ),
	DEFINE_FIELD( CFassn, m_fThrowGrenade, FIELD_BOOLEAN ),
	DEFINE_FIELD( CFassn, m_fFirstEncounter, FIELD_BOOLEAN ),
	DEFINE_FIELD( CFassn, m_voicePitch, FIELD_INTEGER ),

	DEFINE_FIELD( CFassn, m_flNextJump, FIELD_TIME ),
	DEFINE_FIELD( CFassn, m_vecJumpVelocity, FIELD_VECTOR ),
};

IMPLEMENT_SAVERESTORE( CFassn, CSquadMonster );

//=========================================================
// Speak Sentence - say your cued up sentence.
//
// Some grunt sentences (take cover and charge) rely on actually
// being able to execute the intended action. It's really lame
// when a grunt says 'COVER ME' and then doesn't move. The problem
// is that the sentences were played when the decision to TRY
// to move to cover was made. Now the sentence is played after 
// we know for sure that there is a valid path. The schedule
// may still fail but in most cases, well after the grunt has 
// started moving.
//=========================================================
void CFassn :: SpeakSentence( void )
{
	return; 
}

//=========================================================
// IRelationship - overridden because Alien Grunts are 
// Human Grunt's nemesis.
//=========================================================
int CFassn::IRelationship ( CBaseEntity *pTarget )
{
	return CSquadMonster::IRelationship( pTarget );
}


//=========================================================
// RunAI
//=========================================================
void CFassn :: RunAI( void )
{
	CBaseMonster :: RunAI();

	if(m_EyeMod == 0){
		if ( m_hEnemy != NULL || pev->health < pev->max_health || m_alert > 0){
			m_EyeMod = 2;//受伤或发现敌人警戒，进入战斗状态，目力模式
		}
	}

	

	if(pev->sequence == LookupSequence( "reload_pistol" ) || pev->sequence == LookupSequence( "shoot_pistol" )){
	m_duckseq = 2;
	}
	else if(pev->sequence == LookupSequence( "shoot" ) || pev->sequence == LookupSequence( "reload" )
	|| pev->sequence == LookupActivity ( ACT_RANGE_ATTACK2 ) ){
	m_duckseq = 3;
	}
	else if(m_duckseq != 0){
	m_duckseq = 0;
	}

	if(m_makerspawn_call == 1){
		if(pev->weapons == 1){
		m_cClipSize = 15;
		SetBodygroup( 2, 0 );
		m_cAmmoLoaded = m_cClipSize;
		}
		else if(pev->weapons == 2){
		m_cClipSize = 5;
		SetBodygroup( 2, 1 );
		m_cAmmoLoaded = m_cClipSize;
		}
	}

	if (pev->sequence == LookupActivity( ACT_RUN )){
	m_flGroundSpeed = 500;
	}

	if(m_killed_exp == 300){

		if(m_flNextSkillDelayTime <= gpGlobals->time && !FBitSet( pev->flags, FL_NOTARGET ) 
		&& pev->deadflag == DEAD_NO && m_freezetime == 0){
			if (m_hEnemy != NULL || m_flNPC_Pain > 0 || m_alert > 0){
			pev->flags |= FL_NOTARGET;
			m_flNextSkillTime = gpGlobals->time + 5.0;
			m_flNextSkillDelayTime = gpGlobals->time + 20.0;
			pev->effects |= EF_NODRAW;
			FX_Explosion( Center(), EXPLOSION_SPARKSHOWER );
			}
		}
		else if (m_flNextSkillTime <= gpGlobals->time){
			pev->flags &= ~FL_NOTARGET;
			pev->effects &= ~EF_NODRAW;
		}

	}

}


void CFassn::Killed( entvars_t *pevAttacker, int iGib )
{
	//Vector	vecGunPos;
	//Vector	vecGunAngles;
	//不掉落武器
	pev->flags &= ~FL_NOTARGET;
	pev->effects &= ~EF_NODRAW;
	SetUse( NULL );	
	CSquadMonster::Killed( pevAttacker, iGib );
}

//=========================================================
// GibMonster - make gun fly through the air.
//=========================================================
void CFassn :: GibMonster ( void )
{
	CBaseMonster :: GibMonster();
}

//=========================================================
// ISoundMask - Overidden for human grunts because they 
// hear the DANGER sound that is made by hand grenades and
// other dangerous items.
//=========================================================
int CFassn :: ISoundMask ( void )
{
	return	bits_SOUND_WORLD	|
			bits_SOUND_COMBAT	|
			bits_SOUND_PLAYER	|
			bits_SOUND_DANGER;
}

//=========================================================
// PrescheduleThink - this function runs after conditions
// are collected and before scheduling code is run.
//=========================================================
void CFassn :: PrescheduleThink ( void )
{
	if ( InSquad() && m_hEnemy != NULL )
	{
		if ( HasConditions ( bits_COND_SEE_ENEMY ) )
		{
			// update the squad's last enemy sighting time.
			MySquadLeader()->m_flLastEnemySightTime = gpGlobals->time;
		}
		else
		{
			if ( gpGlobals->time - MySquadLeader()->m_flLastEnemySightTime > 5 )
			{
				// been a while since we've seen the enemy
				MySquadLeader()->m_fEnemyEluded = TRUE;
			}
		}
	}
}

//=========================================================
// FCanCheckAttacks - this is overridden for human grunts
// because they can throw/shoot grenades when they can't see their
// target and the base class doesn't check attacks if the monster
// cannot see its enemy.
//
// !!!BUGBUG - this gets called before a 3-round burst is fired
// which means that a friendly can still be hit with up to 2 rounds. 
// ALSO, grenades will not be tossed if there is a friendly in front,
// this is a bad bug. Friendly machine gun fire avoidance
// will unecessarily prevent the throwing of a grenade as well.
//=========================================================
BOOL CFassn :: FCanCheckAttacks ( void )
{
	if ( !HasConditions( bits_COND_ENEMY_TOOFAR ) )
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}


//=========================================================
// CheckMeleeAttack1
//=========================================================
BOOL CFassn :: CheckMeleeAttack1 ( float flDot, float flDist )
{
	int dist = 192;
	if ( FClassnameIs(pev, "monster_human_fassn_fiona")){
	dist = 1536;
	}

	if ( m_flNextJump < gpGlobals->time && (flDist <= dist || HasMemory( bits_MEMORY_BADJUMP )) && m_hEnemy != NULL )
	{
		TraceResult	tr;

		Vector vecDest = pev->origin + Vector( RANDOM_FLOAT( -64, 64), RANDOM_FLOAT( -64, 64 ), 160 );

		UTIL_TraceHull( pev->origin + Vector( 0, 0, 36 ), vecDest + Vector( 0, 0, 36 ), dont_ignore_monsters, human_hull, ENT(pev), &tr);

		if ( tr.fStartSolid || tr.flFraction < 1.0)
		{
			return FALSE;
		}

		float flGravity = 800;

		float time = sqrt( 160 / (0.5 * flGravity));
		float speed = flGravity * time / 160;
		m_vecJumpVelocity = (vecDest - pev->origin) * speed;

		return TRUE;
	}
	return FALSE;
}

//=========================================================
// CheckRangeAttack1 - overridden for HGrunt, cause 
// FCanCheckAttacks() doesn't disqualify all attacks based
// on whether or not the enemy is occluded because unlike
// the base class, the HGrunt can attack when the enemy is
// occluded (throw grenade over wall, etc). We must 
// disqualify the machine gun attack if the enemy is occluded.
//=========================================================
BOOL CFassn :: CheckRangeAttack1 ( float flDot, float flDist )
{
	float dist = 1536;//消音手枪

	if (pev->weapons == 2 || FClassnameIs(pev, "monster_human_fassn_fiona") ){//弩
	dist = 2560;
	}

	if(m_hEnemy != NULL){
		if ( fabs( pev->origin.z - m_hEnemy->pev->origin.z ) > 256 ){//与敌人高度差较高时，增加攻击距离
		dist += 256;
		}
	}

	if(m_aimenemy_mod != 4 && flDist < (dist * 0.25) ){//近距离
	m_aimenemy_mod = 4;//眼睛
	}
	else if(m_aimenemy_mod != 3 && flDist < (dist * 0.5) ){//中距离
	m_aimenemy_mod = 3;//嘴
	}
	else if(m_aimenemy_mod != 8 && flDist < (dist * 0.75) ){//中远距离
	m_aimenemy_mod = 8;//脖子
	}
	else if(m_aimenemy_mod != 5){//远距离
	m_aimenemy_mod = 5;//胸
	}

	if ( FClassnameIs(pev, "monster_human_fassn_fiona")){
	m_aimenemy_mod = 4;//眼睛
	}

			if ( !HasConditions( bits_COND_ENEMY_OCCLUDED ) && flDist <= dist && flDot >= 0.5 && NoFriendlyFire())
			{
				TraceResult	tr;
					
				Vector vecSrc,vecdir;
				vecSrc = GetGunPosition();

				// verify that a bullet fired from the gun will hit the enemy before the world.
				UTIL_TraceLine( vecSrc, m_hEnemy->BodyTarget(vecSrc), ignore_monsters, ignore_glass, ENT(pev), &tr);

				if ( tr.flFraction == 1.0 )
				{
					return TRUE;
				}
			}


	return FALSE;
}

//=========================================================
// CheckRangeAttack2 - this checks the Grunt's grenade
// attack. 
//=========================================================
BOOL CFassn :: CheckRangeAttack2 ( float flDot, float flDist )
{
	m_fThrowGrenade = FALSE;
	if ( !FBitSet ( m_hEnemy->pev->flags, FL_ONGROUND ) )
	{
		// don't throw grenades at anything that isn't on the ground!
		return FALSE;
	}

	int dist = 768;

	if ( m_flNextGrenadeCheck < gpGlobals->time && flDist <= dist && flDist >= 192 /* && flDot >= 0.5 */ /* && NoFriendlyFire() */ )
	{
		Vector vecToss = VecCheckThrow( pev, GetGunPosition( ), m_hEnemy->Center(), flDist, 0.5 ); // use dist as speed to get there in 1 second

		if ( vecToss != g_vecZero )
		{
			m_vecTossVelocity = vecToss;
			// throw a hand grenade
			m_fThrowGrenade = TRUE;

			return TRUE;
		}
	}

	return FALSE;
}


//=========================================================
// TraceAttack - make sure we're not taking it in the helmet
//=========================================================
void CFassn :: TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType)
{
	CSquadMonster::TraceAttack( pevAttacker, flDamage, vecDir, ptr, bitsDamageType );
}


//=========================================================
// TakeDamage - overridden for the grunt because the grunt
// needs to forget that he is in cover if he's hurt. (Obviously
// not in a safe place anymore).
//=========================================================
int CFassn :: TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	m_cleardally_enemy_long = 60;//防偷袭!
	m_alert = 100;

	Forget( bits_MEMORY_INCOVER );

	return CSquadMonster :: TakeDamage ( pevInflictor, pevAttacker, flDamage, bitsDamageType );
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CFassn :: SetYawSpeed ( void )
{
	pev->yaw_speed = 360;
}

//=========================================================
// CheckAmmo - overridden for the grunt because he actually
// uses ammo! (base class doesn't)
//=========================================================
void CFassn :: CheckAmmo ( void )
{
	if ( m_cAmmoLoaded <= 0 )
	{
		SetConditions(bits_COND_NO_AMMO_LOADED);
	}
}

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int	CFassn :: Classify ( void )
{
	return	CLASS_HUMAN_MILITARY;
}

//=========================================================
//=========================================================
CBaseEntity *CFassn :: Kick( void )
{
	TraceResult tr;

	UTIL_MakeVectors( pev->angles );
	Vector vecStart = pev->origin;
	vecStart.z += pev->size.z * 0.5;
	Vector vecEnd = vecStart + (gpGlobals->v_forward * 80);

	UTIL_TraceHull( vecStart, vecEnd, dont_ignore_monsters, head_hull, ENT(pev), &tr );
	
	if ( tr.pHit )
	{
		CBaseEntity *pEntity = CBaseEntity::Instance( tr.pHit );
		return pEntity;
	}

	return NULL;
}

//=========================================================
// GetGunPosition	return the end of the barrel
//=========================================================

Vector CFassn :: GetGunPosition( )
{
	return pev->origin + Vector( 0, 0, 48 );
}

//=========================================================
// Shoot
//=========================================================

void CFassn :: Shoot ( void )
{
	if (m_hEnemy == NULL)
	{
		return;
	}

	Vector vecShootOrigin = GetGunPosition();
	Vector vecShootDir = ShootAtEnemy( vecShootOrigin );

	UTIL_MakeVectors ( pev->angles );

	Vector	vecShellVelocity = gpGlobals->v_right * RANDOM_FLOAT(40,90) + gpGlobals->v_up * RANDOM_FLOAT(75,200) + gpGlobals->v_forward * RANDOM_FLOAT(-40, 40);
	EjectBrass ( pev->origin + gpGlobals->v_up * 32 + gpGlobals->v_forward * 12, vecShellVelocity, pev->angles.y, m_iBrassShell, TE_BOUNCE_SHELL); 

	int iBulletType;
	iBulletType = BULLET_MONSTER_9MM;

	if ( FClassnameIs(pev, "monster_human_fassn_fiona")){
	//精准度增加，无限弹
	FireBullets(1, vecShootOrigin, vecShootDir, Vector( 0.01, 0.01, 0.01 ), 2560, iBulletType,1 );
	}
	else{
	m_cAmmoLoaded--;
	FireBullets(1, vecShootOrigin, vecShootDir, Vector( 0.015, 0.015, 0.015 ), 2048, iBulletType,1 );
	}

	switch(RANDOM_LONG(0,1))
	{
	case 0:
		EMIT_SOUND(ENT(pev), CHAN_WEAPON, "weapons/pl_gun1.wav", 1, ATTN_NORM);
		break;
	case 1:
		EMIT_SOUND(ENT(pev), CHAN_WEAPON, "weapons/pl_gun2.wav", 1, ATTN_NORM);
		break;
	}

	pev->effects |= EF_MUZZLEFLASH;

	Vector angDir = UTIL_VecToAngles( vecShootDir );
	SetBlending( 0, angDir.x );


}

//=========================================================
// Shoot
//=========================================================
void CFassn :: ShootCrossbow ( void )
{
	if (m_hEnemy == NULL)
	{
		return;
	}

	Vector vecShootOrigin,vecdir;
	UTIL_MakeVectors(pev->angles);
	GetAttachment( 1, vecShootOrigin,vecdir);

	Vector vecShootDir = ShootAtEnemy( vecShootOrigin );

	CBaseEntity *pRocket = CBaseEntity::Create( "crossbow_bolt", vecShootOrigin, pev->angles, edict() );
	pRocket->pev->velocity = vecShootDir * 6000;
	pRocket->pev->angles = UTIL_VecToAngles (vecShootDir * 6000);

	pev->effects |= EF_MUZZLEFLASH;
	
	m_cAmmoLoaded--;// take away a bullet!

	Vector angDir = UTIL_VecToAngles( vecShootDir );
	SetBlending( 0, angDir.x );

	EMIT_SOUND(ENT(pev), CHAN_WEAPON, "weapons/xbow_fire1.wav", 1, ATTN_NORM);
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CFassn :: HandleAnimEvent( MonsterEvent_t *pEvent )
{
	Vector	vecShootDir;
	Vector	vecShootOrigin;

	switch( pEvent->event )
	{
		case 1://开火
		if(pev->weapons == 1){
		Shoot();
		}
		else{
		ShootCrossbow();
		}
		break;

		case 2://手雷
		{
			UTIL_MakeVectors( pev->angles );
			CGrenade::ShootTimed( pev, pev->origin + gpGlobals->v_forward * 34 + Vector (0, 0, 32), m_vecTossVelocity, 2.0 );

			m_flNextGrenadeCheck = gpGlobals->time + 6;// wait six seconds before even looking again to see if a grenade can be thrown.
			m_fThrowGrenade = FALSE;
			// !!!LATER - when in a group, only try to throw grenade if ordered.
		}
		break;

		case 3://起跳
		{
			// ALERT( at_console, "jumping");
			UTIL_MakeAimVectors( pev->angles );
			pev->movetype = MOVETYPE_TOSS;
			pev->flags &= ~FL_ONGROUND;
			pev->velocity = m_vecJumpVelocity;
			m_flNextJump = gpGlobals->time + 3.0;
		}
		return;

		case 4://换弹
		{
		m_cAmmoLoaded = m_cClipSize;
		ClearConditions(bits_COND_NO_AMMO_LOADED);
		}
		break;


		default:
			CSquadMonster::HandleAnimEvent( pEvent );
			break;
	}
}

//=========================================================
// Spawn
//=========================================================
void CFassn :: Spawn()
{
	Precache( );

	SET_MODEL(ENT(pev), "models/fassn.mdl");
	UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	m_bloodColor		= BLOOD_COLOR_RED;
	pev->effects		= 0;

	if (g_iSkillLevel == SKILL_HARD){
	pev->health			= 70;
	}
	else{
	pev->health			= 60;
	}

	m_flFieldOfView		= 0.2;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState		= MONSTERSTATE_NONE;
	m_flNextGrenadeCheck = gpGlobals->time + 2;
	m_flNextPainTime	= gpGlobals->time;

	m_flNextSkillTime   = gpGlobals->time;
	m_flNextSkillDelayTime = gpGlobals->time;

	m_afCapability		= bits_CAP_MELEE_ATTACK1 | bits_CAP_SQUAD | bits_CAP_TURN_HEAD | bits_CAP_DOORS_GROUP;

	m_fEnemyEluded		= FALSE;
	m_fFirstEncounter	= TRUE;// this is true when the grunt spawns, because he hasn't encountered an enemy yet.

	m_HackedGunPos = Vector ( 0, 0, 48 );

	int oldbody = pev->body;
	pev->body = 0;
	if ( oldbody <= 0 )
	{
		SetBodygroup( 1, RANDOM_LONG(0,2));
	}
	else{
		SetBodygroup( 1, oldbody - 1);
	}

	if(pev->weapons == 0){
	pev->weapons = RANDOM_LONG(1,2);
	}
	if(pev->weapons == 1){
	m_cClipSize = 15;
	SetBodygroup( 2, 0 );
	}
	else if(pev->weapons == 2){
	m_cClipSize = 5;
	SetBodygroup( 2, 1 );
	}


	m_cAmmoLoaded		= m_cClipSize;
	m_canheadcrab_mode  = 0;
	m_canbarnacle_mode  = 1;

	CTalkMonster::g_talkWaitTime = 0;

	MonsterInit();

	m_candrownwater = 1;

	m_forcefuckdoor  = TRUE;
	m_chase_mode = 3;
	m_chase_failed_max = 4;

	if ( FClassnameIs(pev, "monster_human_fassn_fiona")){
		SET_MODEL(ENT(pev), "models/fiona.mdl");
		UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);
		if (g_iSkillLevel == SKILL_HARD){//血略厚一些但依旧脆皮
		pev->health			= 150;
		}
		else{
		pev->health			= 120;
		}
		pev->body = 0;
		pev->weapons = 1;
		m_cClipSize = 15;
		SetBodygroup( 2, 0 );
		m_killed_exp = 300;
		m_rpgms_level = 60;
		pev->netname = MAKE_STRING( "Assassin.Fiona" );

		m_headdef			= 2;//坚硬の头部
		m_ignoredamage		= 1;
		pev->gravity        = 0.5;

		m_singdelay_max = 0;//0反应
		m_singdelay_use = m_singdelay_max;
	}
	else{
		m_killed_exp = 100;
		m_rpgms_level = 50;
		pev->netname = MAKE_STRING( "Assassin" );
		pev->gravity = 0.7;

		m_singdelay_max		= 1;//快速反应
		m_singdelay_use		= m_singdelay_max;
	}
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CFassn :: Precache()
{
	PRECACHE_MODEL("models/fassn.mdl");
	PRECACHE_MODEL("models/fiona.mdl");

	PRECACHE_SOUND("weapons/xbow_fire1.wav");// because we use the basemonster SWIPE animation event
	UTIL_PrecacheOther( "crossbow_bolt" );

	m_voicePitch = 100 + RANDOM_LONG(-5,10);

	m_iBrassShell = PRECACHE_MODEL ("models/shell.mdl");// brass shell

	PRECACHE_SOUND("debris/beamstart1.wav");
}	

//=========================================================
// start task
//=========================================================
void CFassn :: StartTask ( Task_t *pTask )
{
	m_iTaskStatus = TASKSTATUS_RUNNING;

	switch ( pTask->iTask )
	{
	case TASK_GRUNT_CHECK_FIRE:
		if ( !NoFriendlyFire() )
		{
			SetConditions( bits_COND_GRUNT_NOFIRE );
		}
		TaskComplete();
		break;

	case TASK_GRUNT_SPEAK_SENTENCE:
		SpeakSentence();
		TaskComplete();
		break;
	
	case TASK_WALK_PATH:
	case TASK_RUN_PATH:
	case TASK_STRAFE_PATH:
		// grunt no longer assumes he is covered if he moves
		Forget( bits_MEMORY_INCOVER );
		CSquadMonster ::StartTask( pTask );
		break;

	case TASK_RELOAD:
		m_IdealActivity = ACT_RELOAD;
		break;

	case TASK_GRUNT_FACE_TOSS_DIR:
		break;

	case TASK_FACE_IDEAL:
	case TASK_FACE_ENEMY:
		CSquadMonster :: StartTask( pTask );
		if (pev->movetype == MOVETYPE_FLY)
		{
			m_IdealActivity = ACT_GLIDE;
		}
		break;
	case TASK_RANGE_ATTACK2:
		if (!m_fThrowGrenade)
		{
			TaskComplete( );
		}
		break;

	default: 
		CSquadMonster :: StartTask( pTask );
		break;
	}
}

//=========================================================
// RunTask
//=========================================================
void CFassn :: RunTask ( Task_t *pTask )
{
	switch ( pTask->iTask )
	{
	case TASK_FASSN_FALL_TO_GROUND:
		MakeIdealYaw( m_vecEnemyLKP );
		ChangeYaw( pev->yaw_speed );

		if (m_fSequenceFinished)
		{
			if (HasConditions ( bits_COND_SEE_ENEMY ))
			{
				if(pev->weapons == 2){
				pev->sequence = LookupSequence( "fly_down_pistol" );
				}
				else{
				pev->sequence = LookupSequence( "fly_attack_pistol" );
				}
				pev->frame = 0;
			}
			else if (pev->velocity.z > 0)
			{
				pev->sequence = LookupSequence( "fly_up" );
			}
			else
			{
				pev->sequence = LookupSequence( "fly_down_pistol" );
				pev->frame = 0;
			}
			
			ResetSequenceInfo( );
			SetYawSpeed();
		}
		if (pev->flags & FL_ONGROUND)
		{
			// ALERT( at_console, "on ground\n");
			TaskComplete( );
		}
		break;
	case TASK_GRUNT_FACE_TOSS_DIR:
		{
			// project a point along the toss vector and turn to face that point.
			MakeIdealYaw( pev->origin + m_vecTossVelocity * 64 );
			ChangeYaw( pev->yaw_speed );

			if ( FacingIdeal() )
			{
				m_iTaskStatus = TASKSTATUS_COMPLETE;
			}
			break;
		}
	default:
		{
			CSquadMonster :: RunTask( pTask );
			break;
		}
	}
}

//=========================================================
// PainSound
//=========================================================
void CFassn :: PainSound ( void )
{

}

//=========================================================
// DeathSound 
//=========================================================
void CFassn :: DeathSound ( void )
{

}

//=========================================================
// AI Schedules Specific to this monster
//=========================================================

//=========================================================
// GruntFail
//=========================================================
Task_t	tlFassnFail[] =
{
	{ TASK_STOP_MOVING,			0				},
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE },
	{ TASK_WAIT,				(float)1		},
};

Schedule_t	slFassnFail[] =
{
	{
		tlFassnFail,
		ARRAYSIZE ( tlFassnFail ),
		bits_COND_CAN_RANGE_ATTACK1 |
		bits_COND_CAN_RANGE_ATTACK2 |
		bits_COND_CAN_MELEE_ATTACK1 |
		bits_COND_CAN_MELEE_ATTACK2,
		0,
		"Grunt Fail"
	},
};

//=========================================================
// Grunt Combat Fail
//=========================================================
Task_t	tlFassnCombatFail[] =
{
	{ TASK_STOP_MOVING,			0				},
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE },
	{ TASK_WAIT_FACE_ENEMY,		(float)1		},
};

Schedule_t	slFassnCombatFail[] =
{
	{
		tlFassnCombatFail,
		ARRAYSIZE ( tlFassnCombatFail ),
		bits_COND_CAN_RANGE_ATTACK1 |
		bits_COND_CAN_RANGE_ATTACK2 |
		bits_COND_CAN_MELEE_ATTACK1 |
		bits_COND_CAN_MELEE_ATTACK2,
		0,
		"Grunt Combat Fail"
	},
};

//=========================================================
// Victory dance!
//=========================================================
Task_t	tlFassnVictoryDance[] =
{
	{ TASK_STOP_MOVING,						(float)0					},
	{ TASK_FACE_ENEMY,						(float)0					},
	{ TASK_WAIT,							(float)1.5					},
	{ TASK_GET_PATH_TO_ENEMY_CORPSE,		(float)0					},
	{ TASK_WALK_PATH,						(float)0					},
	{ TASK_WAIT_FOR_MOVEMENT,				(float)0					},
	{ TASK_FACE_ENEMY,						(float)0					},
	{ TASK_PLAY_SEQUENCE,					(float)ACT_VICTORY_DANCE	},
};

Schedule_t	slFassnVictoryDance[] =
{
	{ 
		tlFassnVictoryDance,
		ARRAYSIZE ( tlFassnVictoryDance ), 
		bits_COND_NEW_ENEMY		|
		bits_COND_SEE_ENEMY		|
		bits_COND_LIGHT_DAMAGE	|
		bits_COND_HEAVY_DAMAGE  |
		bits_COND_HEAR_SOUND,
		bits_SOUND_DANGER,
		"GruntVictoryDance"
	},
};

//=========================================================
// Establish line of fire - move to a position that allows
// the grunt to attack.
//=========================================================
Task_t tlFassnEstablishLineOfFire[] = 
{
	{ TASK_SET_FAIL_SCHEDULE,	(float)SCHED_GRUNT_ELOF_FAIL	},
	{ TASK_GET_PATH_TO_ENEMY,	(float)0						},
	{ TASK_GRUNT_SPEAK_SENTENCE,(float)0						},
	{ TASK_RUN_PATH,			(float)0						},
	{ TASK_WAIT_FOR_MOVEMENT,	(float)0						},
};

Schedule_t slFassnEstablishLineOfFire[] =
{
	{ 
		tlFassnEstablishLineOfFire,
		ARRAYSIZE ( tlFassnEstablishLineOfFire ),
		bits_COND_NEW_ENEMY			|
		bits_COND_ENEMY_DEAD		|
		bits_COND_CAN_RANGE_ATTACK1	|
		bits_COND_CAN_MELEE_ATTACK1	|
		bits_COND_CAN_RANGE_ATTACK2	|
		bits_COND_CAN_MELEE_ATTACK2	|
		bits_COND_HEAR_SOUND,
		
		bits_SOUND_DANGER,
		"GruntEstablishLineOfFire"
	},
};

//=========================================================
// GruntFoundEnemy - grunt established sight with an enemy
// that was hiding from the squad.
//=========================================================
Task_t	tlFassnFoundEnemy[] =
{
	{ TASK_STOP_MOVING,				0							},
	{ TASK_FACE_ENEMY,				(float)0					},
	{ TASK_PLAY_SEQUENCE_FACE_ENEMY,(float)ACT_SIGNAL1			},
};

Schedule_t	slFassnFoundEnemy[] =
{
	{ 
		tlFassnFoundEnemy,
		ARRAYSIZE ( tlFassnFoundEnemy ), 
		bits_COND_HEAR_SOUND,
		
		bits_SOUND_DANGER,
		"GruntFoundEnemy"
	},
};

//=========================================================
// GruntCombatFace Schedule
//=========================================================
Task_t	tlFassnCombatFace1[] =
{
	{ TASK_STOP_MOVING,				0							},
	{ TASK_SET_ACTIVITY,			(float)ACT_IDLE				},
	{ TASK_FACE_ENEMY,				(float)0					},
	{ TASK_WAIT,					(float)1.5					},
	{ TASK_SET_SCHEDULE,			(float)SCHED_GRUNT_SWEEP	},
};

Schedule_t	slFassnCombatFace[] =
{
	{ 
		tlFassnCombatFace1,
		ARRAYSIZE ( tlFassnCombatFace1 ), 
		bits_COND_NEW_ENEMY				|
		bits_COND_ENEMY_DEAD			|
		bits_COND_CAN_RANGE_ATTACK1		|
		bits_COND_CAN_RANGE_ATTACK2,
		0,
		"Combat Face"
	},
};

//=========================================================
// Suppressing fire - don't stop shooting until the clip is
// empty or grunt gets hurt.
//=========================================================
Task_t	tlFassnSignalSuppress[] =
{
	{ TASK_STOP_MOVING,					0						},
	{ TASK_FACE_IDEAL,					(float)0				},
	{ TASK_PLAY_SEQUENCE_FACE_ENEMY,	(float)ACT_SIGNAL2		},
	{ TASK_FACE_ENEMY,					(float)0				},
	{ TASK_GRUNT_CHECK_FIRE,			(float)0				},
	{ TASK_RANGE_ATTACK1,				(float)0				},
	{ TASK_FACE_ENEMY,					(float)0				},
	{ TASK_GRUNT_CHECK_FIRE,			(float)0				},
	{ TASK_RANGE_ATTACK1,				(float)0				},
	{ TASK_FACE_ENEMY,					(float)0				},
	{ TASK_GRUNT_CHECK_FIRE,			(float)0				},
	{ TASK_RANGE_ATTACK1,				(float)0				},
	{ TASK_FACE_ENEMY,					(float)0				},
	{ TASK_GRUNT_CHECK_FIRE,			(float)0				},
	{ TASK_RANGE_ATTACK1,				(float)0				},
	{ TASK_FACE_ENEMY,					(float)0				},
	{ TASK_GRUNT_CHECK_FIRE,			(float)0				},
	{ TASK_RANGE_ATTACK1,				(float)0				},
};

Schedule_t	slFassnSignalSuppress[] =
{
	{ 
		tlFassnSignalSuppress,
		ARRAYSIZE ( tlFassnSignalSuppress ), 
		bits_COND_ENEMY_DEAD		|
		bits_COND_LIGHT_DAMAGE		|
		bits_COND_HEAVY_DAMAGE		|
		bits_COND_HEAR_SOUND		|
		bits_COND_GRUNT_NOFIRE		|
		bits_COND_NO_AMMO_LOADED,

		bits_SOUND_DANGER,
		"SignalSuppress"
	},
};

Task_t	tlFassnSuppress[] =
{
	{ TASK_STOP_MOVING,			0							},
	{ TASK_FACE_ENEMY,			(float)0					},
	{ TASK_GRUNT_CHECK_FIRE,	(float)0					},
	{ TASK_RANGE_ATTACK1,		(float)0					},
	{ TASK_FACE_ENEMY,			(float)0					},
	{ TASK_GRUNT_CHECK_FIRE,	(float)0					},
	{ TASK_RANGE_ATTACK1,		(float)0					},
	{ TASK_FACE_ENEMY,			(float)0					},
	{ TASK_GRUNT_CHECK_FIRE,	(float)0					},
	{ TASK_RANGE_ATTACK1,		(float)0					},
	{ TASK_FACE_ENEMY,			(float)0					},
	{ TASK_GRUNT_CHECK_FIRE,	(float)0					},
	{ TASK_RANGE_ATTACK1,		(float)0					},
	{ TASK_FACE_ENEMY,			(float)0					},
	{ TASK_GRUNT_CHECK_FIRE,	(float)0					},
	{ TASK_RANGE_ATTACK1,		(float)0					},
};

Schedule_t	slFassnSuppress[] =
{
	{ 
		tlFassnSuppress,
		ARRAYSIZE ( tlFassnSuppress ), 
		bits_COND_ENEMY_DEAD		|
		bits_COND_LIGHT_DAMAGE		|
		bits_COND_HEAVY_DAMAGE		|
		bits_COND_HEAR_SOUND		|
		bits_COND_GRUNT_NOFIRE		|
		bits_COND_NO_AMMO_LOADED,

		bits_SOUND_DANGER,
		"Suppress"
	},
};


//=========================================================
// grunt wait in cover - we don't allow danger or the ability
// to attack to break a grunt's run to cover schedule, but
// when a grunt is in cover, we do want them to attack if they can.
//=========================================================
Task_t	tlFassnWaitInCover[] =
{
	{ TASK_STOP_MOVING,				(float)0					},
	{ TASK_SET_ACTIVITY,			(float)ACT_IDLE				},
	{ TASK_WAIT_FACE_ENEMY,			(float)1					},
};

Schedule_t	slFassnWaitInCover[] =
{
	{ 
		tlFassnWaitInCover,
		ARRAYSIZE ( tlFassnWaitInCover ), 
		bits_COND_NEW_ENEMY			|
		bits_COND_HEAR_SOUND		|
		bits_COND_CAN_RANGE_ATTACK1	|
		bits_COND_CAN_RANGE_ATTACK2	|
		bits_COND_CAN_MELEE_ATTACK1	|
		bits_COND_CAN_MELEE_ATTACK2,

		bits_SOUND_DANGER,
		"GruntWaitInCover"
	},
};

//=========================================================
// run to cover.
// !!!BUGBUG - set a decent fail schedule here.
//=========================================================
Task_t	tlFassnTakeCover1[] =
{
	{ TASK_STOP_MOVING,				(float)0							},
	{ TASK_SET_FAIL_SCHEDULE,		(float)SCHED_GRUNT_TAKECOVER_FAILED	},
	{ TASK_WAIT,					(float)0.2							},
	{ TASK_FIND_COVER_FROM_ENEMY,	(float)0							},
	{ TASK_GRUNT_SPEAK_SENTENCE,	(float)0							},
	{ TASK_RUN_PATH,				(float)0							},
	{ TASK_WAIT_FOR_MOVEMENT,		(float)0							},
	{ TASK_REMEMBER,				(float)bits_MEMORY_INCOVER			},
	{ TASK_SET_SCHEDULE,			(float)SCHED_GRUNT_WAIT_FACE_ENEMY	},
};

Schedule_t	slFassnTakeCover[] =
{
	{ 
		tlFassnTakeCover1,
		ARRAYSIZE ( tlFassnTakeCover1 ), 
		0,
		0,
		"TakeCover"
	},
};

//=========================================================
// drop grenade then run to cover.
//=========================================================
Task_t	tlFassnGrenadeCover1[] =
{
	{ TASK_STOP_MOVING,				(float)0							},
	{ TASK_SET_FAIL_SCHEDULE,		(float)SCHED_GRUNT_TAKECOVER_FAILED	},
	{ TASK_WAIT,					(float)0.2							},
	{ TASK_FIND_COVER_FROM_ENEMY,	(float)0							},
	{ TASK_RUN_PATH,				(float)0							},
	{ TASK_WAIT_FOR_MOVEMENT,		(float)0							},
	{ TASK_REMEMBER,				(float)bits_MEMORY_INCOVER			},
	{ TASK_SET_SCHEDULE,			(float)SCHED_GRUNT_WAIT_FACE_ENEMY	},
};

Schedule_t	slFassnGrenadeCover[] =
{
	{ 
		tlFassnGrenadeCover1,
		ARRAYSIZE ( tlFassnGrenadeCover1 ), 
		0,
		0,
		"TakeCover"
	},
};


//=========================================================
// drop grenade then run to cover.
//=========================================================
Task_t	tlFassnTossGrenadeCover1[] =
{
	{ TASK_STOP_MOVING,						(float)0							},
	{ TASK_FACE_ENEMY,						(float)0							},
	{ TASK_RANGE_ATTACK2, 					(float)0							},
	{ TASK_SET_SCHEDULE,					(float)SCHED_TAKE_COVER_FROM_ENEMY	},
};

Schedule_t	slFassnTossGrenadeCover[] =
{
	{ 
		tlFassnTossGrenadeCover1,
		ARRAYSIZE ( tlFassnTossGrenadeCover1 ), 
		0,
		0,
		"TossGrenadeCover"
	},
};
//=========================================================
// hide from the loudest sound source (to run from grenade)
//=========================================================
Task_t	tlFassnTakeCoverFromBestSound[] =
{
//	{ TASK_SET_FAIL_SCHEDULE,			(float)SCHED_COWER			},// duck and cover if cannot move from explosion
	{ TASK_STOP_MOVING,					(float)0					},
	{ TASK_FIND_COVER_FROM_BEST_SOUND,	(float)0					},
	{ TASK_RUN_PATH,					(float)0					},
	{ TASK_WAIT_FOR_MOVEMENT,			(float)0					},
	{ TASK_REMEMBER,					(float)bits_MEMORY_INCOVER	},
	{ TASK_TURN_LEFT,					(float)179					},
};

Schedule_t	slFassnTakeCoverFromBestSound[] =
{
	{ 
		tlFassnTakeCoverFromBestSound,
		ARRAYSIZE ( tlFassnTakeCoverFromBestSound ), 
		0,
		0,
		"TakeCoverFromBestSound"
	},
};

//=========================================================
// Grunt reload schedule
//=========================================================
Task_t	tlFassnHideReload[] =
{
	{ TASK_STOP_MOVING,				(float)0					},
	{ TASK_SET_FAIL_SCHEDULE,		(float)SCHED_RELOAD			},
	{ TASK_FIND_COVER_FROM_ENEMY,	(float)0					},
	{ TASK_RUN_PATH,				(float)0					},
	{ TASK_WAIT_FOR_MOVEMENT,		(float)0					},
	{ TASK_REMEMBER,				(float)bits_MEMORY_INCOVER	},
	{ TASK_FACE_ENEMY,				(float)0					},
	{ TASK_PLAY_SEQUENCE,			(float)ACT_RELOAD			},
};

Schedule_t slFassnHideReload[] = 
{
	{
		tlFassnHideReload,
		ARRAYSIZE ( tlFassnHideReload ),
		bits_COND_HEAVY_DAMAGE	|
		bits_COND_HEAR_SOUND,

		bits_SOUND_DANGER,
		"GruntHideReload"
	}
};

//=========================================================
// Do a turning sweep of the area
//=========================================================
Task_t	tlFassnSweep[] =
{
	{ TASK_TURN_LEFT,			(float)179	},
	{ TASK_WAIT,				(float)1	},
	{ TASK_TURN_LEFT,			(float)179	},
	{ TASK_WAIT,				(float)1	},
};

Schedule_t	slFassnSweep[] =
{
	{ 
		tlFassnSweep,
		ARRAYSIZE ( tlFassnSweep ), 
		
		bits_COND_NEW_ENEMY		|
		bits_COND_LIGHT_DAMAGE	|
		bits_COND_HEAVY_DAMAGE	|
		bits_COND_CAN_RANGE_ATTACK1	|
		bits_COND_CAN_RANGE_ATTACK2	|
		bits_COND_HEAR_SOUND,

		bits_SOUND_WORLD		|// sound flags
		bits_SOUND_DANGER		|
		bits_SOUND_PLAYER,

		"Grunt Sweep"
	},
};

//=========================================================
// primary range attack. Overriden because base class stops attacking when the enemy is occluded.
// grunt's grenade toss requires the enemy be occluded.
//=========================================================
Task_t	tlFassnRangeAttack1A[] =
{
	{ TASK_STOP_MOVING,			(float)0		},
	{ TASK_GRUNT_CHECK_FIRE,	(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
	{ TASK_FACE_ENEMY,			(float)0		},
	{ TASK_GRUNT_CHECK_FIRE,	(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
	{ TASK_FACE_ENEMY,			(float)0		},
	{ TASK_GRUNT_CHECK_FIRE,	(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
	{ TASK_FACE_ENEMY,			(float)0		},
	{ TASK_GRUNT_CHECK_FIRE,	(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
};

Schedule_t	slFassnRangeAttack1A[] =
{
	{ 
		tlFassnRangeAttack1A,
		ARRAYSIZE ( tlFassnRangeAttack1A ), 
		bits_COND_NEW_ENEMY			|
		bits_COND_ENEMY_DEAD		|
		bits_COND_HEAVY_DAMAGE		|
		bits_COND_ENEMY_OCCLUDED	|
		bits_COND_HEAR_SOUND		|
		bits_COND_GRUNT_NOFIRE		|
		bits_COND_NO_AMMO_LOADED,
		
		bits_SOUND_DANGER,
		"Range Attack1A"
	},
};


//=========================================================
// Jumping Schedules
//=========================================================
Task_t	tlFassnJump[] =
{
	{ TASK_STOP_MOVING,			(float)0		},
	{ TASK_PLAY_SEQUENCE,		(float)ACT_HOP	},
	{ TASK_SET_SCHEDULE,		(float)SCHED_FASSN_JUMP_ATTACK },
};

Schedule_t	slFassnJump[] =
{
	{ 
		tlFassnJump,
		ARRAYSIZE ( tlFassnJump ), 
		0, 
		0, 
		"FassnJump"
	},
};


//=========================================================
// repel 
//=========================================================
Task_t	tlFassnJumpAttack[] =
{
	{ TASK_SET_FAIL_SCHEDULE,	(float)SCHED_FASSN_JUMP_LAND	},
	// { TASK_SET_ACTIVITY,		(float)ACT_FLY	},
	{ TASK_FASSN_FALL_TO_GROUND, (float)0		},
};


Schedule_t	slFassnJumpAttack[] =
{
	{ 
		tlFassnJumpAttack,
		ARRAYSIZE ( tlFassnJumpAttack ), 
		0, 
		0,
		"Fassn JumpAttack"
	},
};


//=========================================================
// repel 
//=========================================================
Task_t	tlFassnJumpLand[] =
{
	// { TASK_SET_FAIL_SCHEDULE,		(float)SCHED_MELEE_ATTACK1	},
	{ TASK_SET_ACTIVITY,			(float)ACT_IDLE				},
	{ TASK_REMEMBER,				(float)bits_MEMORY_BADJUMP	},
	{ TASK_FIND_NODE_COVER_FROM_ENEMY,	(float)0					},
	{ TASK_RUN_PATH,				(float)0					},
	{ TASK_FORGET,					(float)bits_MEMORY_BADJUMP	},
	{ TASK_WAIT_FOR_MOVEMENT,		(float)0					},
	{ TASK_REMEMBER,				(float)bits_MEMORY_INCOVER	},
	{ TASK_FACE_ENEMY,				(float)0					},
	{ TASK_SET_FAIL_SCHEDULE,		(float)SCHED_RANGE_ATTACK1	},
};

Schedule_t	slFassnJumpLand[] =
{
	{ 
		tlFassnJumpLand,
		ARRAYSIZE ( tlFassnJumpLand ), 
		0, 
		0,
		"Fassn JumpLand"
	},
};

//=========================================================
// primary range attack. Overriden because base class stops attacking when the enemy is occluded.
// grunt's grenade toss requires the enemy be occluded.
//=========================================================
Task_t	tlFassnRangeAttack1B[] =
{
	{ TASK_STOP_MOVING,				(float)0		},
	{ TASK_GRUNT_CHECK_FIRE,	(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
	{ TASK_FACE_ENEMY,			(float)0		},
	{ TASK_GRUNT_CHECK_FIRE,	(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
	{ TASK_FACE_ENEMY,			(float)0		},
	{ TASK_GRUNT_CHECK_FIRE,	(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
	{ TASK_FACE_ENEMY,			(float)0		},
	{ TASK_GRUNT_CHECK_FIRE,	(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
};

Schedule_t	slFassnRangeAttack1B[] =
{
	{ 
		tlFassnRangeAttack1B,
		ARRAYSIZE ( tlFassnRangeAttack1B ), 
		bits_COND_NEW_ENEMY			|
		bits_COND_ENEMY_DEAD		|
		bits_COND_HEAVY_DAMAGE		|
		bits_COND_ENEMY_OCCLUDED	|
		bits_COND_NO_AMMO_LOADED	|
		bits_COND_GRUNT_NOFIRE		|
		bits_COND_HEAR_SOUND,
		
		bits_SOUND_DANGER,
		"Range Attack1B"
	},
};

//=========================================================
// secondary range attack. Overriden because base class stops attacking when the enemy is occluded.
// grunt's grenade toss requires the enemy be occluded.
//=========================================================
Task_t	tlFassnRangeAttack2[] =
{
	{ TASK_STOP_MOVING,				(float)0					},
	{ TASK_GRUNT_FACE_TOSS_DIR,		(float)0					},
	{ TASK_PLAY_SEQUENCE,			(float)ACT_RANGE_ATTACK2	},
	{ TASK_SET_SCHEDULE,			(float)SCHED_GRUNT_WAIT_FACE_ENEMY	},// don't run immediately after throwing grenade.
};

Schedule_t	slFassnRangeAttack2[] =
{
	{ 
		tlFassnRangeAttack2,
		ARRAYSIZE ( tlFassnRangeAttack2 ), 
		0,
		0,
		"RangeAttack2"
	},
};


//=========================================================
// repel 
//=========================================================
Task_t	tlFassnRepel[] =
{
	{ TASK_STOP_MOVING,			(float)0		},
	{ TASK_FACE_IDEAL,			(float)0		},
	{ TASK_PLAY_SEQUENCE,		(float)ACT_GLIDE 	},
};

Schedule_t	slFassnRepel[] =
{
	{ 
		tlFassnRepel,
		ARRAYSIZE ( tlFassnRepel ), 
		bits_COND_SEE_ENEMY			|
		bits_COND_NEW_ENEMY			|
		bits_COND_LIGHT_DAMAGE		|
		bits_COND_HEAVY_DAMAGE		|
		bits_COND_HEAR_SOUND,
		
		bits_SOUND_DANGER			|
		bits_SOUND_COMBAT			|
		bits_SOUND_PLAYER, 
		"Repel"
	},
};


//=========================================================
// repel 
//=========================================================
Task_t	tlFassnRepelAttack[] =
{
	{ TASK_STOP_MOVING,			(float)0		},
	{ TASK_FACE_ENEMY,			(float)0		},
	{ TASK_PLAY_SEQUENCE,		(float)ACT_FLY 	},
};

Schedule_t	slFassnRepelAttack[] =
{
	{ 
		tlFassnRepelAttack,
		ARRAYSIZE ( tlFassnRepelAttack ), 
		bits_COND_ENEMY_OCCLUDED,
		0,
		"Repel Attack"
	},
};

//=========================================================
// repel land
//=========================================================
Task_t	tlFassnRepelLand[] =
{
	{ TASK_STOP_MOVING,			(float)0		},
	{ TASK_PLAY_SEQUENCE,		(float)ACT_LAND	},
	{ TASK_GET_PATH_TO_LASTPOSITION,(float)0				},
	{ TASK_RUN_PATH,				(float)0				},
	{ TASK_WAIT_FOR_MOVEMENT,		(float)0				},
	{ TASK_CLEAR_LASTPOSITION,		(float)0				},
};

Schedule_t	slFassnRepelLand[] =
{
	{ 
		tlFassnRepelLand,
		ARRAYSIZE ( tlFassnRepelLand ), 
		bits_COND_SEE_ENEMY			|
		bits_COND_NEW_ENEMY			|
		bits_COND_LIGHT_DAMAGE		|
		bits_COND_HEAVY_DAMAGE		|
		bits_COND_HEAR_SOUND,
		
		bits_SOUND_DANGER			|
		bits_SOUND_COMBAT			|
		bits_SOUND_PLAYER, 
		"Repel Land"
	},
};


DEFINE_CUSTOM_SCHEDULES( CFassn )
{
	slFassnFail,
	slFassnCombatFail,
	slFassnVictoryDance,
	slFassnEstablishLineOfFire,
	slFassnFoundEnemy,
	slFassnCombatFace,
	slFassnSignalSuppress,
	slFassnSuppress,
	slFassnWaitInCover,
	slFassnTakeCover,
	slFassnGrenadeCover,
	slFassnTossGrenadeCover,
	slFassnTakeCoverFromBestSound,
	slFassnHideReload,
	slFassnSweep,
	slFassnRangeAttack1A,
	slFassnRangeAttack1B,
	slFassnRangeAttack2,
	slFassnRepel,
	slFassnRepelAttack,
	slFassnRepelLand,
	slFassnJump,
	slFassnJumpAttack,
	slFassnJumpLand,
};

IMPLEMENT_CUSTOM_SCHEDULES( CFassn, CSquadMonster );

//=========================================================
// SetActivity 
//=========================================================
void CFassn :: SetActivity ( Activity NewActivity )
{
	int	iSequence = ACTIVITY_NOT_AVAILABLE;
	void *pmodel = GET_MODEL_PTR( ENT(pev) );

	switch ( NewActivity)
	{
	case ACT_RANGE_ATTACK1:
		if(pev->weapons == 1){
		iSequence = LookupSequence( "shoot_pistol" );
		}
		else{
		iSequence = LookupSequence( "shoot" );
		}
		break;
	case ACT_RELOAD:
		if(pev->weapons == 1){
		iSequence = LookupSequence( "reload_pistol" );
		}
		else{
		iSequence = LookupSequence( "reload" );
		}
		break;
	case ACT_RUN:
		if(pev->weapons == 1){
		iSequence = LookupSequence( "run_pistol" );
		}
		else{
		iSequence = LookupSequence( "run" );
		}
		break;
	case ACT_WALK:
		if(pev->weapons == 1){
		iSequence = LookupSequence( "walk_pistol" );
		}
		else{
		iSequence = LookupSequence( "walk" );
		}
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
		pev->sequence		= 0;	// Set to the reset anim (if it's there)
	}
}

//=========================================================
// Get Schedule!
//=========================================================
Schedule_t *CFassn :: GetSchedule( void )
{

	// grunts place HIGH priority on running away from danger sounds.
	if ( HasConditions(bits_COND_HEAR_SOUND) )
	{
		CSound *pSound;
		pSound = PBestSound();

		ASSERT( pSound != NULL );
		if ( pSound)
		{
			if (pSound->m_iType & bits_SOUND_DANGER)
			{
				// dangerous sound nearby!
				
				//!!!KELLY - currently, this is the grunt's signal that a grenade has landed nearby,
				// and the grunt should find cover from the blast
				// good place for "SHIT!" or some other colorful verbal indicator of dismay.
				// It's not safe to play a verbal order here "Scatter", etc cause 
				// this may only affect a single individual in a squad. 
				return GetScheduleOfType( SCHED_TAKE_COVER_FROM_BEST_SOUND );
			}
			/*
			if (!HasConditions( bits_COND_SEE_ENEMY ) && ( pSound->m_iType & (bits_SOUND_PLAYER | bits_SOUND_COMBAT) ))
			{
				MakeIdealYaw( pSound->m_vecOrigin );
			}
			*/
		}
	}
switch	( m_MonsterState )
	{		
	case MONSTERSTATE_IDLE:
	case MONSTERSTATE_ALERT:
		{
				if ( HasConditions ( bits_COND_HEAR_SOUND ))
				{
					CSound *pSound;
					pSound = PBestSound();

						if ( pSound && (pSound->m_iType & bits_SOUND_COMBAT) )
						{
							return GetScheduleOfType( SCHED_INVESTIGATE_SOUND );
						}
				}
		}
		break;
		case MONSTERSTATE_COMBAT:
		{
// dead enemy
			if ( HasConditions( bits_COND_ENEMY_DEAD ) )
			{
				// call base class, all code to handle dead enemies is centralized there.
				return CBaseMonster :: GetSchedule();
			}

			// flying?
			if ( pev->movetype == MOVETYPE_TOSS)
			{
				if (pev->flags & FL_ONGROUND)
				{
					// ALERT( at_console, "landed\n");
					// just landed
					pev->movetype = MOVETYPE_STEP;
					return GetScheduleOfType ( SCHED_FASSN_JUMP_LAND );
				}
				else
				{
					// ALERT( at_console, "jump\n");
					// jump or jump/shoot
					if ( m_MonsterState == MONSTERSTATE_COMBAT )
						return GetScheduleOfType ( SCHED_FASSN_JUMP );
					else
						return GetScheduleOfType ( SCHED_FASSN_JUMP_ATTACK );
				}
			}
// new enemy
			if ( HasConditions(bits_COND_NEW_ENEMY) )
			{
					if ( HasConditions ( bits_COND_CAN_RANGE_ATTACK1 ) )
					{
						return GetScheduleOfType ( SCHED_GRUNT_SUPPRESS );
					}
					else
					{
						return GetScheduleOfType ( SCHED_TAKE_COVER_FROM_ENEMY );
					}
			}
// no ammo
			else if ( HasConditions ( bits_COND_NO_AMMO_LOADED ) )
			{
				return GetScheduleOfType ( SCHED_GRUNT_COVER_AND_RELOAD );
			}
			
// damaged just a little
			else if ( HasConditions( bits_COND_LIGHT_DAMAGE ) )
			{
				return GetScheduleOfType( SCHED_TAKE_COVER_FROM_ENEMY );
			}
// can kick
			else if ( HasConditions ( bits_COND_CAN_MELEE_ATTACK1 ) )
			{
				return GetScheduleOfType ( SCHED_MELEE_ATTACK1 );
			}
// can grenade launch
			else if ( HasConditions ( bits_COND_CAN_RANGE_ATTACK2 ) )
			{
				// shoot a grenade if you can
				return GetScheduleOfType( SCHED_RANGE_ATTACK2 );
			}
			else if ( HasConditions ( bits_COND_CAN_RANGE_ATTACK1 ) )
			{
				if ( OccupySlot ( bits_SLOTS_HGRUNT_ENGAGE ) )
				{
					// try to take an available ENGAGE slot
					return GetScheduleOfType( SCHED_RANGE_ATTACK1 );
				}
				else if ( HasConditions ( bits_COND_CAN_RANGE_ATTACK2 ) )
				{
					// throw a grenade if can and no engage slots are available
					return GetScheduleOfType( SCHED_RANGE_ATTACK2 );
				}
				else
				{
					// hide!
					return GetScheduleOfType( SCHED_TAKE_COVER_FROM_ENEMY );
				}
			}
// can't see enemy
			else if ( HasConditions( bits_COND_ENEMY_OCCLUDED ) )
			{
				if ( HasConditions( bits_COND_CAN_RANGE_ATTACK2 ))
				{
					return GetScheduleOfType( SCHED_RANGE_ATTACK2 );
				}
				else if ( OccupySlot( bits_SLOTS_HGRUNT_ENGAGE ) )
				{
					return GetScheduleOfType( SCHED_GRUNT_ESTABLISH_LINE_OF_FIRE );
				}
				else
				{
					return GetScheduleOfType( SCHED_STANDOFF );
				}
			}
			
			if ( HasConditions( bits_COND_SEE_ENEMY ) && !HasConditions ( bits_COND_CAN_RANGE_ATTACK1 ) )
			{
				return GetScheduleOfType ( SCHED_GRUNT_ESTABLISH_LINE_OF_FIRE );
			}
		}
	}
	
	// no special cases here, call the base class
	return CSquadMonster :: GetSchedule();
}

//=========================================================
//=========================================================
Schedule_t* CFassn :: GetScheduleOfType ( int Type ) 
{
	switch	( Type )
	{
	case SCHED_TAKE_COVER_FROM_ENEMY:
		{
			return &slFassnGrenadeCover[ 0 ];
		}
	case SCHED_TAKE_COVER_FROM_BEST_SOUND:
		{
			return &slFassnTakeCoverFromBestSound[ 0 ];
		}
	case SCHED_GRUNT_TAKECOVER_FAILED:
		{
			if ( HasConditions( bits_COND_CAN_RANGE_ATTACK1 ) && OccupySlot( bits_SLOTS_HGRUNT_ENGAGE ) )
			{
				return GetScheduleOfType( SCHED_RANGE_ATTACK1 );
			}

			return GetScheduleOfType ( SCHED_FAIL );
		}
		break;
	case SCHED_GRUNT_ELOF_FAIL:
		{
			// human grunt is unable to move to a position that allows him to attack the enemy.
			return GetScheduleOfType ( SCHED_TAKE_COVER_FROM_ENEMY );
		}
		break;
	case SCHED_GRUNT_ESTABLISH_LINE_OF_FIRE:
		{
			return &slFassnEstablishLineOfFire[ 0 ];
		}
		break;
	case SCHED_RANGE_ATTACK1:
		{
			return &slFassnRangeAttack1A[ 0 ];
		}
	case SCHED_RANGE_ATTACK2:
		{
			return &slFassnRangeAttack2[ 0 ];
		}
	case SCHED_COMBAT_FACE:
		{
			return &slFassnCombatFace[ 0 ];
		}
	case SCHED_MELEE_ATTACK1:
		if (pev->flags & FL_ONGROUND)
		{
			if (m_flNextJump > gpGlobals->time)
			{
				// can't jump yet, go ahead and fail
				return GetScheduleOfType ( SCHED_TAKE_COVER_FROM_ENEMY );
			}
			else
			{
				return slFassnJump;
			}
		}
		else
		{
			return slFassnJumpAttack;
		}
	case SCHED_FASSN_JUMP:
	case SCHED_FASSN_JUMP_ATTACK:
		return slFassnJumpAttack;
	case SCHED_FASSN_JUMP_LAND:
		return slFassnJumpLand;
	case SCHED_GRUNT_WAIT_FACE_ENEMY:
		{
			return &slFassnWaitInCover[ 0 ];
		}
	case SCHED_GRUNT_SWEEP:
		{
			return &slFassnSweep[ 0 ];
		}
	case SCHED_GRUNT_COVER_AND_RELOAD:
		{
			if(m_hEnemy != NULL){
				float flDist = ( m_hEnemy->pev->origin - pev->origin).Length();

				if ( flDist < 1024 && fabs( pev->origin.z - m_hEnemy->pev->origin.z ) < 256 && HasConditions ( bits_COND_ENEMY_FACING_ME ) ){
				return &slFassnHideReload[ 0 ];
				}
				else{
				return &slReload[ 0 ];
				}
			}
			else{
				return &slReload[ 0 ];
			}
			
		}
	case SCHED_GRUNT_FOUND_ENEMY:
		{
			return &slFassnFoundEnemy[ 0 ];
		}
	case SCHED_VICTORY_DANCE:
		{
			return &slFassnVictoryDance[ 0 ];
		}
	case SCHED_GRUNT_SUPPRESS:
		{
			return &slFassnSuppress[ 0 ];
		}
	case SCHED_FAIL:
		{
			if ( m_hEnemy != NULL )
			{
				// grunt has an enemy, so pick a different default fail schedule most likely to help recover.
				return &slFassnCombatFail[ 0 ];
			}

			return &slFassnFail[ 0 ];
		}
	case SCHED_GRUNT_REPEL:
		{
			if (pev->velocity.z > -128)
				pev->velocity.z -= 32;
			return &slFassnRepel[ 0 ];
		}
	case SCHED_GRUNT_REPEL_ATTACK:
		{
			if (pev->velocity.z > -128)
				pev->velocity.z -= 32;
			return &slFassnRepelAttack[ 0 ];
		}
	case SCHED_GRUNT_REPEL_LAND:
		{
			return &slFassnRepelLand[ 0 ];
		}
	default:
		{
			return CSquadMonster :: GetScheduleOfType ( Type );
		}
	}
}
