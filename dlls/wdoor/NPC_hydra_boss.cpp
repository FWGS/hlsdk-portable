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
// Gargantua
//=========================================================
#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"nodes.h"
#include	"monsters.h"
#include	"schedule.h"
#include	"customentity.h"
#include	"weapons.h"
#include	"effects.h"
#include	"soundent.h"
#include	"decals.h"
#include	"explode.h"
#include	"func_break.h"
#include	"animation.h"

extern DLL_GLOBAL int		g_iSkillLevel;

//=========================================================
// Gargantua Monster
//=========================================================
const float HYDRA_ATTACKDIST = 128.0;

// Garg animation events
#define HYDRA_AE_KICK					1//�߷�
#define HYDRA_AE_BEAM					2//ͷ鳼���
#define HYDRA_AE_SHOCK					3//�����
#define HYDRA_AE_SUCK					4//ǣ��
#define HYDRA_AE_STONE					5//ʯ���
#define HYDRA_AE_LEFT_FOOT				6//��Ų�
#define HYDRA_AE_RIGHT_FOOT				7//�ҽŲ�
#define HYDRA_AE_BREATHE				8//����
#define HYDRA_AE_STOMP					9//ս����̤

// Gargantua is immune to any damage but this
#define HYDRA_DAMAGE					(DMG_SHOCK|DMG_ENERGYBEAM|DMG_CRUSH|DMG_MORTAR|DMG_BLAST|DMG_DARK|DMG_ENERGYBLAST|DMG_VALVE_SWORD)
#define HYDRA_EYE_SPRITE_NAME		"sprites/yelflare1.spr"
#define HYDRA_BEAM_SPRITE_NAME		"sprites/xbeam3.spr"
#define HYDRA_BEAM_SPRITE2			"sprites/xbeam3.spr"
#define HYDRA_STOMP_SPRITE_NAME		"sprites/glow_grn.spr"
#define HYDRA_STOMP_BUZZ_SOUND		"weapons/mine_charge.wav"
#define HYDRA_FLAME_LENGTH			350
#define HYDRA_GIB_MODEL				"models/metalplategibs.mdl"

#define ATTN_HYDRA					(ATTN_NORM)

#define STOMP_SPRITE_COUNT			10

int gHydraStompSprite = 0, gHydraGibModel = 0;
void SpawnExplosion( Vector center, float randomRange, float time, int magnitude );

class CSmoker;


class CStompHydra : public CBaseEntity
{
public:
	void Spawn( void );
	void Think( void );
	static CStompHydra *StompCreate( const Vector &origin, const Vector &end, float speed );

private:
// UNDONE: re-use this sprite list instead of creating new ones all the time
//	CSprite		*m_pSprites[ STOMP_SPRITE_COUNT ];
};

LINK_ENTITY_TO_CLASS( hydra_stomp, CStompHydra );
CStompHydra *CStompHydra::StompCreate( const Vector &origin, const Vector &end, float speed )
{
	CStompHydra *pStomp = GetClassPtr( (CStompHydra *)NULL );
	
	pStomp->pev->origin = origin;
	Vector dir = (end - origin);
	pStomp->pev->scale = dir.Length();
	pStomp->pev->movedir = dir.Normalize();
	pStomp->pev->speed = speed;
	pStomp->Spawn();
	
	return pStomp;
}

void CStompHydra::Spawn( void )
{
	pev->nextthink = gpGlobals->time;
	pev->classname = MAKE_STRING("hydra_stomp");
	pev->dmgtime = gpGlobals->time;

	pev->framerate = 30;
	pev->model = MAKE_STRING(HYDRA_STOMP_SPRITE_NAME);
	pev->rendermode = kRenderTransTexture;
	pev->renderamt = 0;
	EMIT_SOUND_DYN( edict(), CHAN_BODY, HYDRA_STOMP_BUZZ_SOUND, 1, ATTN_NORM, 0, PITCH_NORM * 0.55);
}


#define	STOMP_INTERVAL		0.025

void CStompHydra::Think( void )
{
	int dmg;
	dmg = 45;

	TraceResult tr;

	if ( UTIL_PointContents(pev->origin) == CONTENT_SKY ){//SKY BOX?
	UTIL_Remove(this);
	STOP_SOUND( edict(), CHAN_BODY, HYDRA_STOMP_BUZZ_SOUND );
	return;
	}

	pev->nextthink = gpGlobals->time + 0.1;

	CSoundEnt::InsertSound ( bits_SOUND_DANGER, pev->origin, 1024, 0.2 );

	::RadiusDamage2( pev->origin, pev, pev, dmg, 180, CLASS_ALIEN_MILITARY, DMG_ENERGYBEAM);
/*
	// Do damage for this frame
	Vector vecStart = pev->origin;
	vecStart.z += 30;
	Vector vecEnd = vecStart + (pev->movedir * pev->speed * gpGlobals->frametime);

	UTIL_TraceHull( vecStart, vecEnd, dont_ignore_monsters, head_hull, ENT(pev), &tr );
	
	if ( tr.pHit && tr.pHit != pev->owner )
	{
		CBaseEntity *pEntity = CBaseEntity::Instance( tr.pHit );
		entvars_t *pevOwner = pev;
		if ( pev->owner )
			pevOwner = VARS(pev->owner);

		if ( pEntity )
			pEntity->TakeDamage( pev, pevOwner, dmg, DMG_SONIC );
	}
*/	
	// Accelerate the effect
	pev->speed = pev->speed + (gpGlobals->frametime) * pev->framerate;
	pev->framerate = pev->framerate + (gpGlobals->frametime) * 1500;
	
	// Move and spawn trails
	while ( gpGlobals->time - pev->dmgtime > STOMP_INTERVAL )
	{
		pev->origin = pev->origin + pev->movedir * pev->speed * STOMP_INTERVAL;
		for ( int i = 0; i < 2; i++ )
		{
			CSprite *pSprite = CSprite::SpriteCreate( HYDRA_STOMP_SPRITE_NAME, pev->origin, TRUE );
			if ( pSprite )
			{
				UTIL_TraceLine( pev->origin, pev->origin - Vector(0,0,500), ignore_monsters, edict(), &tr );
				pSprite->pev->origin = tr.vecEndPos;
				pSprite->pev->velocity = Vector(RANDOM_FLOAT(-256,256),RANDOM_FLOAT(-256,256),192);
				// pSprite->AnimateAndDie( RANDOM_FLOAT( 8.0, 12.0 ) );
				pSprite->pev->nextthink = gpGlobals->time + 0.4;
				pSprite->SetThink( &CStompHydra::SUB_Remove_fx );
				pSprite->SetTransparency( kRenderTransAdd, 128, 128, 128, 64, kRenderFxFadeFast );
				if ( !FNullEnt( FIND_CLIENT_IN_PVS( edict() ) ) ){
				FX_Trail(pSprite->pev->origin, pSprite->entindex(), 140 );
				}
			}
		}
		pev->dmgtime += STOMP_INTERVAL;
		// Scale has the "life" of this effect
		pev->scale -= STOMP_INTERVAL * pev->speed;
		if ( pev->scale <= 0 )
		{
			// Life has run out
			UTIL_Remove(this);
			STOP_SOUND( edict(), CHAN_BODY, HYDRA_STOMP_BUZZ_SOUND );
		}

	}
}


void StreakSplash2( const Vector &origin, const Vector &direction, int color, int count, int speed, int velocityRange )
{
	MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, origin );
		WRITE_BYTE( TE_STREAK_SPLASH );
		WRITE_COORD( origin.x );		// origin
		WRITE_COORD( origin.y );
		WRITE_COORD( origin.z );
		WRITE_COORD( direction.x );	// direction
		WRITE_COORD( direction.y );
		WRITE_COORD( direction.z );
		WRITE_BYTE( color );	// Streak color 6
		WRITE_SHORT( count );	// count
		WRITE_SHORT( speed );
		WRITE_SHORT( velocityRange );	// Random velocity modifier
	MESSAGE_END();
}


class CHydra : public CBaseMonster
{
public:
	void Spawn( void );
	void Precache( void );
	void SetYawSpeed( void );
	int  Classify ( void );
	int TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType );
	void TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType );
	void HandleAnimEvent( MonsterEvent_t *pEvent );

	BOOL FCanCheckAttacks ( void );

	void SetActivity ( Activity NewActivity );

	int IRelationship ( CBaseEntity *pTarget );

	BOOL CheckMeleeAttack1( float flDot, float flDist );		// Swipe
	BOOL CheckMeleeAttack2( float flDot, float flDist );		// Flames
	BOOL CheckRangeAttack1( float flDot, float flDist );		// Stomp attack
	BOOL CheckRangeAttack2( float flDot, float flDist );		// Stomp attack
	void SetObjectCollisionBox( void )
	{
		pev->absmin = pev->origin + Vector( -100, -100, 0 );
		pev->absmax = pev->origin + Vector( 100, 100, 300 );
	}

	void StartTask( Task_t *pTask );
	void RunTask( Task_t *pTask );

	void PrescheduleThink( void );

	void Killed( entvars_t *pevAttacker, int iGib );
	void DeathEffect( void );

	void EyeOff( void );
	void EyeOn( int level );
	void EyeUpdate( void );
	void Leap( void );
	void StompAttack( void );

	int m_lightning;

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

private:
	static const char *pAttackHitSounds[];
	static const char *pBeamAttackSounds[];
	static const char *pAttackMissSounds[];
	static const char *pRicSounds[];
	static const char *pFootSounds[];
	static const char *pIdleSounds[];
	static const char *pAlertSounds[];
	static const char *pPainSounds[];
	static const char *pAttackSounds[];
	static const char *pStompSounds[];
	static const char *pBreatheSounds[];

	CBaseEntity* GargantuaCheckTraceHullAttack(float flDist, int iDamage, int iDmgType);

	CSprite		*m_pEyeGlow;		// Glow around the eyes

	int			m_eyeBrightness;	// Brightness target
	float		m_seeTime;			// Time to attack (when I see the enemy, I set this)
	float		m_painSoundTime;	// Time of next pain sound
	float		m_streakTime;		// streak timer (don't send too many)		

	float		m_stone_time;
	float		m_suck_time;
	float		m_blast_time;
	float		m_stomp_time;

	EHANDLE m_stone_eh1;
	EHANDLE m_stone_eh2;
	EHANDLE m_spore_eh3;
};

LINK_ENTITY_TO_CLASS( monster_hydra_boss, CHydra );

TYPEDESCRIPTION	CHydra::m_SaveData[] = 
{
	DEFINE_FIELD( CHydra, m_pEyeGlow, FIELD_CLASSPTR ),
	DEFINE_FIELD( CHydra, m_eyeBrightness, FIELD_INTEGER ),
	DEFINE_FIELD( CHydra, m_seeTime, FIELD_TIME ),
	DEFINE_FIELD( CHydra, m_streakTime, FIELD_TIME ),
	DEFINE_FIELD( CHydra, m_painSoundTime, FIELD_TIME ),

	DEFINE_FIELD( CHydra, m_stone_eh1, FIELD_EHANDLE ),
	DEFINE_FIELD( CHydra, m_stone_eh2, FIELD_EHANDLE ),
	DEFINE_FIELD( CHydra, m_spore_eh3, FIELD_EHANDLE ),

	DEFINE_FIELD( CHydra, m_stone_time, FIELD_TIME ),//ʯ֮��ħ�ٻ�
	DEFINE_FIELD( CHydra, m_suck_time, FIELD_TIME ),//���ǣ��
	DEFINE_FIELD( CHydra, m_blast_time, FIELD_TIME ),//ͷ鳳����
	DEFINE_FIELD( CHydra, m_stomp_time, FIELD_TIME )//ս����̤
};

IMPLEMENT_SAVERESTORE( CHydra, CBaseMonster );

const char *CHydra::pAttackHitSounds[] = 
{
	"zombie/claw_strike1.wav",
	"zombie/claw_strike2.wav",
	"zombie/claw_strike3.wav",
};

const char *CHydra::pBeamAttackSounds[] = 
{
	"garg/gar_flameoff1.wav",
	"garg/gar_flameon1.wav",
	"garg/gar_flamerun1.wav",
};


const char *CHydra::pAttackMissSounds[] = 
{
	"zombie/claw_miss1.wav",
	"zombie/claw_miss2.wav",
};

const char *CHydra::pRicSounds[] = 
{
#if 0
	"weapons/ric1.wav",
	"weapons/ric2.wav",
	"weapons/ric3.wav",
	"weapons/ric4.wav",
	"weapons/ric5.wav",
#else
	"debris/metal4.wav",
	"debris/metal6.wav",
	"weapons/ric4.wav",
	"weapons/ric5.wav",
#endif
};

const char *CHydra::pFootSounds[] = 
{
	"garg/gar_step1.wav",
	"garg/gar_step2.wav",
};


const char *CHydra::pIdleSounds[] = 
{
	"garg/gar_idle1.wav",
	"garg/gar_idle2.wav",
	"garg/gar_idle3.wav",
	"garg/gar_idle4.wav",
	"garg/gar_idle5.wav",
};


const char *CHydra::pAttackSounds[] = 
{
	"garg/gar_attack1.wav",
	"garg/gar_attack2.wav",
	"garg/gar_attack3.wav",
};

const char *CHydra::pAlertSounds[] = 
{
	"garg/gar_alert1.wav",
	"garg/gar_alert2.wav",
	"garg/gar_alert3.wav",
};

const char *CHydra::pPainSounds[] = 
{
	"garg/gar_pain1.wav",
	"garg/gar_pain2.wav",
	"garg/gar_pain3.wav",
};

const char *CHydra::pStompSounds[] = 
{
	"garg/gar_stomp1.wav",
};

const char *CHydra::pBreatheSounds[] = 
{
	"garg/gar_breathe1.wav",
	"garg/gar_breathe2.wav",
	"garg/gar_breathe3.wav",
};
//=========================================================
// AI Schedules Specific to this monster
//=========================================================


void CHydra::EyeOn( int level )
{
	m_eyeBrightness = level;	
}


void CHydra::EyeOff( void )
{
	m_eyeBrightness = 0;
}


void CHydra::EyeUpdate( void )
{
	if ( m_pEyeGlow )
	{
		m_pEyeGlow->pev->renderamt = UTIL_Approach( m_eyeBrightness, m_pEyeGlow->pev->renderamt, 26 );
		if ( m_pEyeGlow->pev->renderamt == 0 )
			m_pEyeGlow->pev->effects |= EF_NODRAW;
		else
			m_pEyeGlow->pev->effects &= ~EF_NODRAW;
		UTIL_SetOrigin( m_pEyeGlow->pev, pev->origin );
	}
}

void CHydra::StompAttack( void )
{
	TraceResult trace;

	UTIL_MakeVectors( pev->angles );
	Vector vecStart = pev->origin + Vector(0,0,60) + 240 * gpGlobals->v_forward;
	Vector vecAim = ShootAtEnemy( vecStart );
	Vector vecEnd = (vecAim * 2560) + vecStart;

	UTIL_TraceLine( vecStart, vecEnd, ignore_monsters, edict(), &trace );
	CStompHydra::StompCreate( vecStart, trace.vecEndPos, 1200 );

	FX_Explosion( vecStart - Vector(0,0,30), 105 );
	EMIT_SOUND_DYN ( ENT(pev), CHAN_VOICE, "hydra/shock.wav", 1.0, 0.6, 0, 100 + RANDOM_LONG(-5,5) );

	UTIL_TraceLine( pev->origin, pev->origin - Vector(0,0,20), ignore_monsters, edict(), &trace );
	if ( trace.flFraction < 1.0 )
		UTIL_DecalTrace( &trace, DECAL_GARGSTOMP1 );
}

// ����Ŀ��
void MovetoTarget( entvars_t *pevFucker, Vector vecTarget,float speed )
{
	// accelerate
	Vector m_vecIdeal;
	m_vecIdeal = Vector( 0, 0, 0 );
	float flSpeed = m_vecIdeal.Length();
	if (flSpeed == 0)
	{
		m_vecIdeal = pevFucker->velocity;
		flSpeed = m_vecIdeal.Length();
	}

	if (flSpeed > (speed + 768))
	{
		m_vecIdeal = m_vecIdeal.Normalize( ) * (speed + 768);
	}

	m_vecIdeal = m_vecIdeal + (vecTarget - pevFucker->origin).Normalize() * speed;

	pevFucker->velocity = m_vecIdeal;
}

//RUNAI
void CHydra :: PrescheduleThink( void )
{
	if(pev->sequence == LookupActivity ( ACT_WALK )){
	m_flGroundSpeed = 200;
	}

	if(m_flPlayerDamage_hate > 0){
		if(m_flPlayerDamage_hate > 300){
		m_flPlayerDamage_hate = 300;
		}
		else{
		m_flPlayerDamage_hate -= 1;
		}
	}

	if ( m_stone_eh1 != NULL ){
		if(m_stone_eh1->pev->deadflag == DEAD_NO && m_stone_eh1->pev->movetype == MOVETYPE_FLY){
			Vector vecArmPos,vecArmDir,vecArmEnd;
			GetAttachment( 0, vecArmPos, vecArmDir );
			float flDist = ( m_stone_eh1->pev->origin - vecArmPos).Length();
			if(flDist >= 4 && flDist <= 512){
			UTIL_SetOrigin( m_stone_eh1->pev, vecArmPos);
			m_stone_eh1->pev->velocity = g_vecZero;
			}
			if(m_hEnemy != NULL){
				CBaseMonster *pEnemyMonster;
				pEnemyMonster = m_stone_eh1->MyMonsterPointer();
				if(pEnemyMonster->m_alert == 0){
				pEnemyMonster->m_hEnemy = m_hEnemy;
				pEnemyMonster->m_alert = 5;
				}
			}
		}
	}
	if ( m_stone_eh2 != NULL ){
		if(m_stone_eh2->pev->deadflag == DEAD_NO && m_stone_eh2->pev->movetype == MOVETYPE_FLY){
			Vector vecArmPos,vecArmDir,vecArmEnd;
			GetAttachment( 1, vecArmPos, vecArmDir );
			float flDist = ( m_stone_eh2->pev->origin - vecArmPos).Length();
			if(flDist >= 4 && flDist <= 512){
			UTIL_SetOrigin( m_stone_eh2->pev, vecArmPos);
			m_stone_eh2->pev->velocity = g_vecZero;
			}
			if(m_hEnemy != NULL){
				CBaseMonster *pEnemyMonster;
				pEnemyMonster = m_stone_eh2->MyMonsterPointer();
				if(pEnemyMonster->m_alert == 0){
				pEnemyMonster->m_hEnemy = m_hEnemy;
				pEnemyMonster->m_alert = 5;
				}
			}
		}
	}

	if ( m_spore_eh3 == NULL ){
		if(pev->frags == 0){
			CBaseEntity *pEntity2 = UTIL_FindEntityByClassname( NULL, "hydra_spore_fun" );
			if ( pEntity2 ){
			m_spore_eh3 = pEntity2;
			}
			pev->frags = 1;
		}
	}
	else{//��Ѫ
		if(pev->health <= pev->max_health * 0.5 && pev->sequence != LookupActivity ( ACT_SPECIAL_ATTACK1 ) 
		&& pev->sequence != LookupActivity ( ACT_MELEE_ATTACK2 ) ){
			if(m_spore_eh3->IsAlive() && m_spore_eh3->pev->body == 0){
				m_streakTime = gpGlobals->time + 10.0;
				SetActivity ( ACT_SPECIAL_ATTACK1 );
				SetState( MONSTERSTATE_HUNT );
				m_hEnemy = NULL;
				m_hOldEnemy[0] = NULL;
				m_hOldEnemy[1] = NULL;
				m_hOldEnemy[2] = NULL;
				m_hOldEnemy[3] = NULL;
				ClearSchedule();
				SetYawSpeed();
			}
		}
	}

	if(m_freeze_def == 2 && gpGlobals->time > m_stone_time 
	&& pev->sequence != LookupActivity ( ACT_SPECIAL_ATTACK1 )){
	m_freeze_def = 1;
	}

	if ( pev->deadflag == DEAD_NO && m_MonsterState == MONSTERSTATE_HUNT && m_streakTime < gpGlobals->time){
	SetState( MONSTERSTATE_IDLE );
	SetActivity( ACT_IDLE );
	ClearSchedule();
	}

	if ( !HasConditions( bits_COND_SEE_ENEMY ) )
	{
		EyeOff();
	}
	else
		EyeOn( 255 );
	
	EyeUpdate();
}


//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int	CHydra :: Classify ( void )
{
	return	CLASS_ALIEN_MILITARY;
}

int CHydra::IRelationship ( CBaseEntity *pTarget )
{
	if ( FClassnameIs( pTarget->pev, "player" ))
	{//��Ҥ�����ޣ�������ɵ��˺��������ȶ�
		if(m_flPlayerDamage_hate >= 240){//���
			return R_NM;
		}
		else if(m_flPlayerDamage_hate >= 60){//����
			return R_HT;
		}
		else{//����
			return R_DL;
		}
	}
	if ( FClassnameIs( pTarget->pev, "monster_wisebeast" ))
	{
		return R_HT;
	}
	if ( FClassnameIs( pTarget->pev, "monster_misaliya" ))
	{
		return R_DL;
	}
	return CBaseMonster::IRelationship( pTarget );
}


//=========================================================
// SetActivity 
//=========================================================
void CHydra :: SetActivity ( Activity NewActivity )
{
	int	iSequence = ACTIVITY_NOT_AVAILABLE;
	void *pmodel = GET_MODEL_PTR( ENT(pev) );

	switch ( NewActivity)
	{
	case ACT_MELEE_ATTACK1:
		if (gpGlobals->time > m_stomp_time)
		{
			iSequence = LookupSequence( "stomp" );
		}
		else
		{
			iSequence = LookupActivity ( NewActivity );
		}
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

		if(pev->deadflag == DEAD_NO){
		pev->sequence		= 0;	// Set to the reset anim (if it's there)
		}
	}
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CHydra :: SetYawSpeed ( void )
{
	pev->yaw_speed = 180;
}


//=========================================================
// Spawn
//=========================================================
void CHydra :: Spawn()
{
	Precache( );

	SET_MODEL(ENT(pev), "models/hydra_boss.mdl");
	UTIL_SetSize( pev, Vector( -48, -48, 0 ), Vector( 48, 48, 288 ) );

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	m_bloodColor		= BLOOD_COLOR_GREEN;

	if (g_iSkillLevel == SKILL_HARD){
	pev->health			= 9000;
	}
	else{
	pev->health			= 7500;
	}

	m_EyeMod = 2;

	pev->gravity = 2.0;

	//pev->view_ofs		= Vector ( 0, 0, 96 );// taken from mdl file
	m_flFieldOfView		= -1;// width of forward view cone ( as a dotproduct result )
	m_MonsterState		= MONSTERSTATE_NONE;

	MonsterInit();

	m_pEyeGlow = CSprite::SpriteCreate( HYDRA_EYE_SPRITE_NAME, pev->origin, FALSE );
	m_pEyeGlow->SetTransparency( kRenderGlow, 255, 255, 255, 0, kRenderFxNoDissipation );
	m_pEyeGlow->SetAttachment( edict(), 4 );
	EyeOff();
	m_seeTime = gpGlobals->time + 5;

	m_aimflag_dist  = 120.0;

	//m_MoveFail_FuckRoad = TRUE;
	m_MoveFail_SimpleRoad = TRUE;

	m_ignoredamage		= 1;

	m_attack_dist = 1024;

	m_stone_eh1 = NULL;
	m_stone_eh2 = NULL;
	m_spore_eh3 = NULL;

	m_stomp_time = gpGlobals->time + 2;
	m_blast_time = gpGlobals->time + 2;
	m_stone_time = gpGlobals->time + 2;
	m_suck_time = gpGlobals->time + 2;

	m_streakTime = 0;

	m_killed_exp	= 1800;
	m_rpgms_level	= 72;
	m_is_the_boss	= TRUE;
	pev->netname = MAKE_STRING( "Hydra.Suture" );

	m_freeze_def = 1;//����ο���LV1
}


//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CHydra :: Precache()
{
	int i;

	PRECACHE_MODEL("models/hydra_boss.mdl");
	PRECACHE_MODEL("models/hydra_spore.mdl");
	PRECACHE_MODEL("sprites/streak.spr");

	PRECACHE_MODEL( HYDRA_EYE_SPRITE_NAME );
	PRECACHE_MODEL( HYDRA_BEAM_SPRITE_NAME );
	PRECACHE_MODEL( HYDRA_BEAM_SPRITE2 );
	gHydraStompSprite = PRECACHE_MODEL( HYDRA_STOMP_SPRITE_NAME );
	gHydraGibModel = PRECACHE_MODEL( HYDRA_GIB_MODEL );
	PRECACHE_SOUND( HYDRA_STOMP_BUZZ_SOUND );

	PRECACHE_SOUND ("hydra/kick.wav");
	PRECACHE_SOUND ("zombie/claw_strike1.wav");
	PRECACHE_SOUND ("hydra/stomp.wav");
	PRECACHE_SOUND ("hydra/shock.wav");
	PRECACHE_SOUND ("hydra/die.wav");

	UTIL_PrecacheOther( "monster_stone_devil_h" );
	UTIL_PrecacheOther( "hydra_spore" );

	m_lightning =  PRECACHE_MODEL("sprites/lgtning.spr");

	for ( i = 0; i < ARRAYSIZE( pAttackHitSounds ); i++ )
		PRECACHE_SOUND((char *)pAttackHitSounds[i]);

	for ( i = 0; i < ARRAYSIZE( pBeamAttackSounds ); i++ )
		PRECACHE_SOUND((char *)pBeamAttackSounds[i]);

	for ( i = 0; i < ARRAYSIZE( pAttackMissSounds ); i++ )
		PRECACHE_SOUND((char *)pAttackMissSounds[i]);

	for ( i = 0; i < ARRAYSIZE( pRicSounds ); i++ )
		PRECACHE_SOUND((char *)pRicSounds[i]);

	for ( i = 0; i < ARRAYSIZE( pFootSounds ); i++ )
		PRECACHE_SOUND((char *)pFootSounds[i]);

	for ( i = 0; i < ARRAYSIZE( pIdleSounds ); i++ )
		PRECACHE_SOUND((char *)pIdleSounds[i]);

	for ( i = 0; i < ARRAYSIZE( pAlertSounds ); i++ )
		PRECACHE_SOUND((char *)pAlertSounds[i]);

	for ( i = 0; i < ARRAYSIZE( pPainSounds ); i++ )
		PRECACHE_SOUND((char *)pPainSounds[i]);

	for ( i = 0; i < ARRAYSIZE( pAttackSounds ); i++ )
		PRECACHE_SOUND((char *)pAttackSounds[i]);

	for ( i = 0; i < ARRAYSIZE( pStompSounds ); i++ )
		PRECACHE_SOUND((char *)pStompSounds[i]);

	for ( i = 0; i < ARRAYSIZE( pBreatheSounds ); i++ )
		PRECACHE_SOUND((char *)pBreatheSounds[i]);
}	


void CHydra::TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType )
{
	CBaseMonster::TraceAttack( pevAttacker, flDamage, vecDir, ptr, bitsDamageType );
}


int CHydra::TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	if ( (bitsDamageType & DMG_CLUB)){//����
	flDamage *= 1.25;
	}

	return CBaseMonster::TakeDamage( pevInflictor, pevAttacker, flDamage, bitsDamageType );
}


void CHydra::DeathEffect( void )
{
	//pev->renderfx = kRenderFxExplode;
	//pev->rendercolor.x = 255;
	//pev->rendercolor.y = 255;
	//pev->rendercolor.z = 255;
	EMIT_SOUND_DYN ( ENT(pev), CHAN_VOICE, "hydra/die.wav", 1.0, 0.5, 0, 100);
}


void CHydra::Killed( entvars_t *pevAttacker, int iGib )
{
	if(pev->body == 0 && m_fightmode){//ʯ������䡤�ٻ�����ģʽ
		Vector vecArmPos,vecArmDir,vecArmEnd;
		GetAttachment( 0, vecArmPos, vecArmDir );
		CBaseEntity *pMS1 = Create( "monster_stone_devil_h", vecArmPos, pev->angles, edict() );
		pMS1->pev->spawnflags |= SF_MONSTER_FADECORPSE;

		GetAttachment( 1, vecArmPos, vecArmDir );
		CBaseEntity *pMS2 = Create( "monster_stone_devil_h", vecArmPos, pev->angles, edict() );
		pMS2->pev->spawnflags |= SF_MONSTER_FADECORPSE;

		pev->body = 1;
	}
	else{//ʯ���һ����ȥ
		if(m_stone_eh1 != NULL){
			if(m_stone_eh1->pev->deadflag == DEAD_NO){
				m_stone_eh1->Killed( pev, GIB_NEVER );
			}
		}
		if(m_stone_eh2 != NULL){
			if(m_stone_eh2->pev->deadflag == DEAD_NO){
				m_stone_eh2->Killed( pev, GIB_NEVER );
			}
		}
	}

	m_spore_eh3 = NULL;
	//Bug Fix 3.0 Hydra����ɱ��Ҳ�����Ѫ��

	EyeOff();
	UTIL_Remove( m_pEyeGlow );
	m_pEyeGlow = NULL;
	CBaseMonster::Killed( pevAttacker, GIB_NEVER );
}

BOOL CHydra :: FCanCheckAttacks ( void )
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
// �߷�
//=========================================================
BOOL CHydra::CheckMeleeAttack1( float flDot, float flDist )
{
	if (flDist <= 192 && gpGlobals->time > m_stomp_time 
	&& !HasConditions(bits_COND_CAN_RANGE_ATTACK1)
	&& !HasConditions(bits_COND_CAN_RANGE_ATTACK2)
	&& !HasConditions(bits_COND_CAN_MELEE_ATTACK2)){
		m_facing_fucking_mode = 1;
		return TRUE;
	}
	else if (flDist <= 128 && flDot >= 0.7)
	{
		m_facing_fucking_mode = 0;
		return TRUE;
	}

	m_facing_fucking_mode = 0;
	return FALSE;
}

//=========================================================
// �ٻ�ʯͷ��
//=========================================================
BOOL CHydra::CheckMeleeAttack2( float flDot, float flDist )
{
	if ( pev->body == 0 && pev->health <= pev->max_health * 0.6 
	&& gpGlobals->time > m_stone_time)
	{
		return TRUE;
	}

	return FALSE;
}

//=========================================================
// ͷ鳳����
//=========================================================
BOOL CHydra::CheckRangeAttack1( float flDot, float flDist )
{
	if ( gpGlobals->time > m_blast_time && pev->health <= pev->max_health * 0.75)
	{
		if (flDot >= 0.5 && flDist >= 256)
		{
			return TRUE;
		}
	}
	return FALSE;
}

//=========================================================
// ǣ��
//=========================================================
BOOL CHydra::CheckRangeAttack2( float flDot, float flDist )
{
	if ( gpGlobals->time > m_suck_time && pev->health <= pev->max_health * 0.9)
	{
		if (flDot >= 0.2 && flDist >= 600)
		{
			return TRUE;
		}
	}
	return FALSE;
}



//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CHydra::HandleAnimEvent(MonsterEvent_t *pEvent)
{
	int dmg,dmg2;
	dmg			= 60;
	dmg2		= 100;

	switch( pEvent->event )
	{
	case HYDRA_AE_KICK:
		{
			EMIT_SOUND_DYN ( ENT(pev), CHAN_VOICE, "hydra/kick.wav", 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
			
			if(m_hEnemy != NULL){
				if(( pev->origin - m_hEnemy->pev->origin).Length() <= HYDRA_ATTACKDIST + 32){
				m_hEnemy->TakeDamage( pev, pev, dmg, DMG_SLASH );
				m_hEnemy->pev->velocity = m_hEnemy->pev->velocity + (m_hEnemy->pev->origin - pev->origin).Normalize() * 600;
				m_hEnemy->pev->velocity.z += 500;
				m_hEnemy->pev->punchangle.x += -60;

				UTIL_MakeVectors ( pev->angles );
				m_hEnemy->pev->velocity = m_hEnemy->pev->velocity + gpGlobals->v_right * 300;

				EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, "zombie/claw_strike1.wav", 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
				}
			}
		}
		break;

	case HYDRA_AE_BEAM:
		{
			MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
			WRITE_BYTE( TE_BEAMFOLLOW );
			WRITE_SHORT( entindex() + 0x1000 * 3 );		// entity, attachment
			WRITE_SHORT(g_sModelIndexTrail );	// model
			WRITE_BYTE( 8 ); // life
			WRITE_BYTE( 6 );  // width
			WRITE_BYTE( 128 );	// R
			WRITE_BYTE( 255 );	// G
			WRITE_BYTE( 192 );	// B
			WRITE_BYTE( 192 );	// brightness
			MESSAGE_END();  // move PHS/PVS data sending into here (SEND_ALL, SEND_PVS, SEND_PHS)
		}
		break;

	case 10:
		{
			MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
			WRITE_BYTE( TE_KILLBEAM );
			WRITE_SHORT( entindex() + 0x1000 * 3 );		// entity, attachment
			MESSAGE_END();

			ClearSchedule();
			SetYawSpeed();
		}
		break;

	case 11:
		{//Bug Fix 1.0 ʯ�����������
			if ( m_stone_eh1 != NULL ){
				m_stone_eh1->pev->velocity = m_stone_eh1->pev->velocity + (m_stone_eh1->pev->origin - pev->origin).Normalize() * 300;
				m_stone_eh1->pev->owner = NULL;
				m_stone_eh1->pev->movetype = MOVETYPE_STEP;
				m_stone_eh1->pev->effects = 0;
				//m_stone_eh1 = NULL;
			}
			if ( m_stone_eh2 != NULL ){
				m_stone_eh2->pev->velocity = m_stone_eh2->pev->velocity + (m_stone_eh2->pev->origin - pev->origin).Normalize() * 300;
				m_stone_eh2->pev->owner = NULL;
				m_stone_eh2->pev->movetype = MOVETYPE_STEP;
				m_stone_eh2->pev->effects = 0;
				//m_stone_eh2 = NULL;
			}
		}
		break;

	case 12://�ù��ӻ�Ѫ����ʼ
		{
			if(m_spore_eh3 == NULL){
				SetState( MONSTERSTATE_IDLE );
				SetActivity( ACT_IDLE );
				ClearSchedule();
			}
			else if(m_spore_eh3->IsAlive() && m_spore_eh3->pev->body == 0){
				m_spore_eh3->pev->frags = 1;
				m_freeze_def = 2;//����ο���LV2
			}
		}
		break;

	case 13://�ù��ӻ�Ѫ������
		{
		//	pev->flags &= ~FL_NOTARGET;
			SetState( MONSTERSTATE_IDLE );
			SetActivity( ACT_IDLE );
			ClearSchedule();
		}
		break;

	case 14:
		{
		//	pev->flags		|= FL_NOTARGET;
		}
		break;

	case HYDRA_AE_STOMP:
		{
			EMIT_SOUND_DYN ( ENT(pev), CHAN_VOICE, "hydra/stomp.wav", 1.0, 0.6, 0, 100 + RANDOM_LONG(-5,5) );
			FX_Explosion( pev->origin + Vector(0,0,16), 104 );
			::RadiusDamage2( pev->origin + Vector(0,0,16), pev, pev, dmg2, 500, CLASS_ALIEN_MILITARY, DMG_ENERGYBEAM);
			m_stomp_time = gpGlobals->time + 10;
			UTIL_ScreenShake( pev->origin, 12.0, 100.0, 2.0, 1000 );
		}
		break;

	case HYDRA_AE_RIGHT_FOOT:
	case HYDRA_AE_LEFT_FOOT:
		UTIL_ScreenShake( pev->origin, 4.0, 3.0, 1.0, 750 );
		EMIT_SOUND_DYN ( edict(), CHAN_BODY, pFootSounds[ RANDOM_LONG(0,ARRAYSIZE(pFootSounds)-1) ], 1.0, ATTN_HYDRA, 0, PITCH_NORM + RANDOM_LONG(-10,10) );
		break;

	case HYDRA_AE_SHOCK:
		StompAttack();
		m_blast_time = gpGlobals->time + 10;
		break;

	case HYDRA_AE_STONE:
		{
			if(pev->body == 0){
			m_freeze_def = 2;//����ο���LV2

			TraceResult tr;
			Vector vecArmPos,vecArmDir,vecArmEnd;

			GetAttachment( 0, vecArmPos, vecArmDir );
			CBaseEntity *pMS1 = Create( "monster_stone_devil_h", vecArmPos, pev->angles, edict() );
			pMS1->pev->spawnflags |= SF_MONSTER_FADECORPSE;
			pMS1->pev->movetype = MOVETYPE_FLY;
			m_stone_eh1 = pMS1;
			m_stone_eh1->pev->effects = EF_LIGHT;

			GetAttachment( 1, vecArmPos, vecArmDir );
			CBaseEntity *pMS2 = Create( "monster_stone_devil_h", vecArmPos, pev->angles, edict() );
			pMS2->pev->spawnflags |= SF_MONSTER_FADECORPSE;
			pMS2->pev->movetype = MOVETYPE_FLY;
			m_stone_eh2 = pMS2;
			m_stone_eh2->pev->effects = EF_LIGHT;

			pev->body = 1;
			m_stone_time = gpGlobals->time + 6;//���߶����ʱ��
			}
		}
		break;

	case HYDRA_AE_SUCK:
		{
			if(m_hEnemy != NULL){
				if(m_suck_time <= gpGlobals->time){
					m_suck_time = gpGlobals->time + 18;

					MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
						WRITE_BYTE( TE_BEAMENTS );
						WRITE_SHORT( entindex() + 0x4000 );
						WRITE_SHORT( m_hEnemy->entindex() );
						WRITE_SHORT( m_lightning );
						WRITE_BYTE( 0 ); // framestart
						WRITE_BYTE( 0 ); // framerate
						WRITE_BYTE( 35 ); // life
						WRITE_BYTE( 70 );  // width
						WRITE_BYTE( 30 );   // noise
						WRITE_BYTE( 255 );   // r, g, b
						WRITE_BYTE( 192 );   // r, g, b
						WRITE_BYTE( 128 );   // r, g, b
						WRITE_BYTE( 255 );	// brightness
						WRITE_BYTE( 30 );		// speed
					MESSAGE_END();
				}

				m_hEnemy->TakeDamage( pev, pev, 0, DMG_CONCUSSION);//ץȡѣ��!

				MovetoTarget( m_hEnemy->pev,pev->origin + Vector(0,0,72),900 );
			}
		
		}
		break;

	case HYDRA_AE_BREATHE:
		EMIT_SOUND_DYN ( edict(), CHAN_VOICE, pBreatheSounds[ RANDOM_LONG(0,ARRAYSIZE(pBreatheSounds)-1) ], 1.0, ATTN_HYDRA, 0, PITCH_NORM + RANDOM_LONG(-10,10) );
		break;

	default:
		CBaseMonster::HandleAnimEvent(pEvent);
		break;
	}
}


//=========================================================
// CheckTraceHullAttack - expects a length to trace, amount 
// of damage to do, and damage type. Returns a pointer to
// the damaged entity in case the monster wishes to do
// other stuff to the victim (punchangle, etc)
// Used for many contact-range melee attacks. Bites, claws, etc.

// Overridden for Gargantua because his swing starts lower as
// a percentage of his height (otherwise he swings over the
// players head)
//=========================================================
CBaseEntity* CHydra::GargantuaCheckTraceHullAttack(float flDist, int iDamage, int iDmgType)
{
	TraceResult tr;

	UTIL_MakeVectors( pev->angles );
	Vector vecStart = pev->origin;
	vecStart.z += 64;
	Vector vecEnd = vecStart + (gpGlobals->v_forward * flDist) - (gpGlobals->v_up * flDist * 0.3);

	UTIL_TraceHull( vecStart, vecEnd, dont_ignore_monsters, head_hull, ENT(pev), &tr );
	
	if ( tr.pHit )
	{
		CBaseEntity *pEntity = CBaseEntity::Instance( tr.pHit );

		if ( iDamage > 0 )
		{
			pEntity->TakeDamage( pev, pev, iDamage, iDmgType );
		}

		return pEntity;
	}

	return NULL;
}

void CHydra::StartTask( Task_t *pTask )
{
	switch ( pTask->iTask )
	{
	case TASK_DIE:
		DeathEffect();
		// FALL THROUGH
	default: 
		CBaseMonster::StartTask( pTask );
		break;
	}
}

//=========================================================
// RunTask
//=========================================================
void CHydra::RunTask( Task_t *pTask )
{
	switch ( pTask->iTask )
	{
	case TASK_DIE:
		CBaseMonster::RunTask(pTask);
		break;

	default:
		CBaseMonster::RunTask( pTask );
		break;
	}
}




// HACKHACK Cut and pasted from explode.cpp
void SpawnExplosion2( Vector center, float randomRange, float time, int magnitude )
{
	KeyValueData	kvd;
	char			buf[128];

	center.x += RANDOM_FLOAT( -randomRange, randomRange );
	center.y += RANDOM_FLOAT( -randomRange, randomRange );

	CBaseEntity *pExplosion = CBaseEntity::Create( "env_explosion", center, g_vecZero, NULL );
	sprintf( buf, "%3d", magnitude );
	kvd.szKeyName = "iMagnitude";
	kvd.szValue = buf;
	pExplosion->KeyValue( &kvd );
	pExplosion->pev->spawnflags |= SF_ENVEXPLOSION_NODAMAGE;

	pExplosion->Spawn();
	pExplosion->pev->frags = 1;
	pExplosion->SetThink( &CBaseEntity::SUB_CallUseToggle );
	pExplosion->pev->nextthink = gpGlobals->time + time;
}

//======Hydra�ĺù���=========
class CHydra_Spore : public CBaseMonster
{
public:
	void	Spawn( void );
	int		Classify ( void );
	void	Killed( entvars_t *pevAttacker, int iGib );
	void	EXPORT Spore_Think ( void );
};

LINK_ENTITY_TO_CLASS( hydra_spore_fun, CHydra_Spore );
LINK_ENTITY_TO_CLASS( hydra_spore, CHydra_Spore );

int	CHydra_Spore :: Classify ( void )
{
	return	CLASS_ALIEN_MILITARY;
}


void CHydra_Spore::Killed( entvars_t *pevAttacker, int iGib )
{
	SpawnBlood(Center() + Vector(0,0,64), BloodColor(), 255);
	SpawnBlood(Center() - Vector(0,0,64), BloodColor(), 255);
	SetThink( NULL );
	CBaseMonster::Killed( pevAttacker, GIB_ALWAYS );
}

void CHydra_Spore::Spawn( void )
{
	if (g_iSkillLevel == SKILL_HARD){
	pev->health			= 1800;
	}
	else{
	pev->health			= 1500;
	}

	pev->max_health = pev->health;

	m_bloodColor		= BLOOD_COLOR_GREEN;

	if ( FClassnameIs( pev, "hydra_spore_fun" )){
		pev->solid			= SOLID_BBOX;
		pev->movetype		= MOVETYPE_NONE;
		pev->effects		= EF_DIMLIGHT;
		pev->flags		   |= FL_MONSTER;

		pev->body			= 0;
		pev->skin			= 0;
		pev->takedamage		= DAMAGE_AIM;

		SET_MODEL(ENT(pev), "models/hydra_spore.mdl");
		UTIL_SetSize( pev, Vector( -48, -48, -96 ), Vector( 48, 48, 96 ) );
		pev->netname = MAKE_STRING( "Spore" );
	}
	else{
		pev->solid			= SOLID_NOT;
		pev->movetype		= MOVETYPE_BOUNCEMISSILE;
		pev->flags		    = FL_NOTARGET;

		pev->effects		= EF_LIGHT;

		pev->body			= 2;

		pev->health			= 60;

		SET_MODEL(ENT(pev), "models/hydra_spore.mdl");

		SetBodygroup( 0, 2 );
		SetBodygroup( 1, 2 );
		SetBodygroup( 2, 2 );

			MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );

				WRITE_BYTE( TE_BEAMFOLLOW );
				WRITE_SHORT(entindex());	// entity
				WRITE_SHORT(g_sModelIndexTrail );	// model
				WRITE_BYTE( 10 ); // life
				WRITE_BYTE( 10 );  // width
				WRITE_BYTE( 192 );   // r, g, b
				WRITE_BYTE( 255 );   // r, g, b
				WRITE_BYTE( 192 );   // r, g, b
				WRITE_BYTE( 255 );	// brightness

			MESSAGE_END();  // move PHS/PVS data sending into here (SEND_ALL, SEND_PVS, SEND_PHS)

	}

	SetThink (&CHydra_Spore::Spore_Think);
	pev->nextthink = gpGlobals->time + 0.1;
}

void CHydra_Spore::Spore_Think(void)
{
	if(pev->health < pev->max_health * 0.5){
	pev->skin = 1;
	}
	else{
	pev->skin = 0;
	}

	if ( FClassnameIs( pev, "hydra_spore" )){
		pev->health -= 1;
		if(pev->health <= 0){
		UTIL_Remove(this);
		return;
		}

		CBaseEntity *pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_hydra_boss" );
		if ( pEntity2 ){
			if (FVisible( pEntity2 )){
				MovetoTarget( pev,pEntity2->Center(),256 );
				float flDist = ( pev->origin - pEntity2->Center()).Length();
				if(flDist <= 256){
					int hphpb =  PRECACHE_MODEL("sprites/anim_spr4.spr");
					MESSAGE_BEGIN(MSG_BROADCAST, SVC_TEMPENTITY, pev->origin);
					WRITE_BYTE(3);
					WRITE_COORD( pev->origin.x);
					WRITE_COORD( pev->origin.y);	
					WRITE_COORD( pev->origin.z);
					WRITE_SHORT(hphpb);
					WRITE_BYTE(25);
					WRITE_BYTE(15);
					WRITE_BYTE(4);
					MESSAGE_END();

					if (g_iSkillLevel == SKILL_HARD){
					pEntity2->TakeHealth(300, DMG_GENERIC);//���ӻָ�����������ֵ
					}
					else{
					pEntity2->TakeHealth(250, DMG_GENERIC);//���ӻָ�����������ֵ
					}
					UTIL_Remove(this);
					return;
				}
			}
		}
	}
	else{
		if(pev->frags >= 1 && pev->frags < 40){
			pev->frags += 1;

			if(pev->frags == 4 || pev->frags == 12 || pev->frags == 20 || pev->frags == 29 || pev->frags == 36 ){
			pev->body = 1;
			SetBodygroup( 0, 1 );
			SetBodygroup( 1, 1 );
			SetBodygroup( 2, 1 );
			UTIL_MakeVectors( pev->angles );
			Vector start_org = pev->origin;
			start_org = start_org + 128 * gpGlobals->v_forward;

			CBaseEntity *pSpore1 = Create( "hydra_spore", start_org + gpGlobals->v_right * 16, pev->angles, edict() );
			pSpore1->pev->velocity = gpGlobals->v_forward * 512 + gpGlobals->v_right * 512;

			CBaseEntity *pSpore2 = Create( "hydra_spore", start_org + gpGlobals->v_right * -16, pev->angles, edict() );
			pSpore2->pev->velocity = gpGlobals->v_forward * 512 + gpGlobals->v_right * -512;

			CBaseEntity *pSpore3 = Create( "hydra_spore", start_org + gpGlobals->v_right * 32 + gpGlobals->v_up * 32, pev->angles, edict() );
			pSpore3->pev->velocity = gpGlobals->v_forward * 768 + gpGlobals->v_right * 256;

			CBaseEntity *pSpore4 = Create( "hydra_spore", start_org + gpGlobals->v_right * -32 + gpGlobals->v_up * 32, pev->angles, edict() );
			pSpore4->pev->velocity = gpGlobals->v_forward * 768 + gpGlobals->v_right * -256;
			}
		}
		else if(pev->frags >= 1){//����
			if(pev->health > 1 && pev->armorvalue <= 150){
				pev->health -= 1;
				pev->armorvalue += 1;
			}
			else if(pev->armorvalue > 150){
				pev->body = 0;
				SetBodygroup( 0, 0 );
				SetBodygroup( 1, 0 );
				SetBodygroup( 2, 0 );
				pev->armorvalue = 0;
				pev->frags = 0;
			}
		}
	}

	pev->nextthink = gpGlobals->time + 0.1;
}