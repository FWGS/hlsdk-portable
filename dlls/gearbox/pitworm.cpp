/***
*
*	Copyright (c) 1996-2001, Valve LLC. All rights reserved.
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

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"nodes.h"
#include	"monsters.h"
#include	"schedule.h"
#include	"soundent.h"
#include	"weapons.h"
#include	"player.h"

#define NUM_PITWORM_LEVELS		4

class CPitWorm : public CBaseMonster
{
public:
        void Spawn(void);
        void Precache(void);
        int  Classify(void);
        virtual int	ObjectCaps(void) { return CBaseMonster::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }

        void IdleSound(void);
        void AlertSound(void);
        void DeathSound(void);

        int		Save(CSave &save);
        int		Restore(CRestore &restore);
        static	TYPEDESCRIPTION m_SaveData[];

        void AngrySound(void);
        void FlinchSound(void);
        void SwipeSound(void);
        void BeamSound(void);
        void pPainSound(void);

        void SetObjectCollisionBox(void);
        void HandleAnimEvent(MonsterEvent_t *pEvent);
        int TakeDamage(entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType);
        void TraceAttack(entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType);
        int CanPlaySequence(BOOL fDisregardState);
        void Activate();

        void EXPORT StartupThink(void);
        void EXPORT DyingThink(void);
        void EXPORT StartupUse(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
        void EXPORT NullThink(void);
        void EXPORT CommandUse(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
        void EXPORT	HuntThink(void);

        void EXPORT WormTouch(CBaseEntity* pOther);

        void FloatSequence(void);
        void NextActivity(void);

        Vector m_avelocity;

        Vector m_vecTarget;
        Vector m_posTarget;

        Vector m_vecDesired;
        Vector m_posDesired;

        Vector m_vecEmemyPos;

        float  m_flMinZ;
        float  m_flMaxZ;

        float m_flLastSeen;
        float m_flPrevSeen;

        int m_iLevel;
        int m_iClass;
        float m_flAdj;
        float m_distDesired;
        float m_flNextBlink;

        int Level(float dz);

        const Vector& IdealPosition(const int level) const;
        const Vector& IdealPosition(const float dz) const;
        const Vector& IdealPosition(CBaseEntity* pEnemy) const;

        void UpdateBodyControllers(void);

        void CreateBeam(const Vector& src, const Vector& target, int width);
        void DestroyBeam(void);
        void UpdateBeam(const Vector& src, const Vector& target);

        void SetupBeamPoints(CBaseEntity* pEnemy, Vector* vecleft, Vector* vecRight);
        void UpdateBeamPoints(CBaseEntity* pEnemy, Vector* vecTarget);

        void CreateGlow(void);
        void DestroyGlow(void);
        void EyeOff(void);
        void EyeOn(int level);
        void EyeUpdate(void);
        void CreateBeam();

        void SetYawSpeed();
        BOOL CheckMeleeAttack1(float flDot, float flDist);
        BOOL CheckRangeAttack1(float flDot, float flDist);


        float m_flInitialYaw;
        Vector m_spawnAngles;
        Vector m_vecLevels[NUM_PITWORM_LEVELS];

        CBeam* m_pBeam;
        float m_flBeamYaw;
        float m_flBeamTime;
        BOOL m_fBeamOn;
        BOOL m_fActivated;
        CSprite* m_pEyeGlow;
        int m_eyeBrightness;
        float m_painSoundTime;
        float m_slowMode;
        float m_slowTime;
        Vector m_vecGoalAngles;
        Vector m_vecCurAngles;
        Vector m_eyes;
        float m_flDamageTime;
        float m_flDamage;
        float m_flPlayerDamage;
        BOOL m_fBlink;
        int m_iLaserHitTimes;

        float m_flNextAttackTime;
        float m_flNextIdleSoundTime;

        static const char* pHitSilo[];
        static const char* pClangSounds[];
        static const char* pAngrySounds[];
        static const char* pSwipeSounds[];
        static const char* pPainSounds[];
        static const char* pFlinchSounds[];

        static const char* pAlertSounds[];
        static const char* pIdleSounds[];
        static const char* pDeathSounds[];
};

LINK_ENTITY_TO_CLASS(monster_pitworm, CPitWorm)
LINK_ENTITY_TO_CLASS(monster_pitworm_up, CPitWorm)

#define PITWORM_HEIGHT					584
#define PITWORM_EYE_OFFSET				Vector(0, 0, 445)
#define PITWORM_EYEBLAST_ATTACK_DIST	460 // 464
#define PITWORM_EYEBLAST_DURATION		3.03f					// 0.49 approximatly. (49 miliseconds)

#define PITWORM_CONTROLLER_EYE_YAW		0
#define PITWORM_CONTROLLER_EYE_PITCH	1
#define PITWORM_CONTROLLER_BODY_YAW		2

#define PITWORM_EYE_PITCH_MIN	-45
#define PITWORM_EYE_PITCH_MAX	 45
#define PITWORM_EYE_YAW_MIN		-45
#define PITWORM_EYE_YAW_MAX		 45
#define PITWORM_BODY_YAW_MIN	-50
#define PITWORM_BODY_YAW_MAX	 50

#define PITWORM_LEVEL0			0		// Pitworm's 'foot' level | when player is at First level 1.
#define PITWORM_LEVEL1			1		// First level | player goes to waste station door 1 or 2.
#define PITWORM_LEVEL2			2		// Second level | player goes to narrow corridor that leads to stairs.
#define PITWORM_LEVEL3			3		// Third level | entry room, steam vent and valve pressure controls.

#define PITWORM_LEVEL0_HEIGHT	-632	// Pitworm's 'foot' level | when player is at First level 1. | (Hammer units)
#define PITWORM_LEVEL1_HEIGHT	-48		// First level | player goes to waste station door 1 or 2. | (Hammer units)
#define PITWORM_LEVEL2_HEIGHT	208		// Second level | player goes to narrow corridor that leads to stairs. | (Hammer units)
#define PITWORM_LEVEL3_HEIGHT	304		// Third level | entry room, steam vent and valve pressure controls. | (Hammer units)


#define PITWORM_LEVEL0_VIEWOFFSET	Vector(0, 0, PITWORM_HEIGHT)
#define PITWORM_LEVEL1_VIEWOFFSET	Vector(0, 0, 256)
#define PITWORM_LEVEL2_VIEWOFFSET	Vector(0, 0, 96)
#define PITWORM_LEVEL3_VIEWOFFSET	PITWORM_LEVEL2_VIEWOFFSET

#define bits_MEMORY_CHILDPAIR		( bits_MEMORY_CUSTOM1 )
#define bits_MEMORY_ADVANCE_NODE	( bits_MEMORY_CUSTOM2 )
#define bits_MEMORY_COMPLETED_NODE	( bits_MEMORY_CUSTOM3 )
#define bits_MEMORY_FIRED_NODE		( bits_MEMORY_CUSTOM4 )

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define PITWORM_AE_SWIPE			( 1 )
#define PITWORM_AE_EYEBLAST_START	( 2 )
#define PITWORM_AE_EYEBLAST_END		( 4 )

//=========================================================
// Save & Restore
//=========================================================
TYPEDESCRIPTION	CPitWorm::m_SaveData[] =
{
        DEFINE_FIELD(CPitWorm, m_avelocity, FIELD_VECTOR),

        DEFINE_FIELD(CPitWorm, m_vecTarget, FIELD_VECTOR),
        DEFINE_FIELD(CPitWorm, m_posTarget, FIELD_POSITION_VECTOR),
        DEFINE_FIELD(CPitWorm, m_vecDesired, FIELD_VECTOR),
        DEFINE_FIELD(CPitWorm, m_posDesired, FIELD_POSITION_VECTOR),
        DEFINE_FIELD(CPitWorm, m_flMinZ, FIELD_FLOAT),
        DEFINE_FIELD(CPitWorm, m_flMaxZ, FIELD_FLOAT),

        DEFINE_FIELD(CPitWorm, m_flLastSeen, FIELD_TIME),
        DEFINE_FIELD(CPitWorm, m_flPrevSeen, FIELD_TIME),
        DEFINE_FIELD(CPitWorm, m_iLevel, FIELD_INTEGER),
        DEFINE_FIELD(CPitWorm, m_flAdj, FIELD_FLOAT),
        DEFINE_FIELD(CPitWorm, m_distDesired, FIELD_FLOAT),

        DEFINE_FIELD(CPitWorm, m_flInitialYaw, FIELD_FLOAT),
        DEFINE_FIELD(CPitWorm, m_spawnAngles, FIELD_VECTOR),
        DEFINE_ARRAY(CPitWorm, m_vecLevels, FIELD_POSITION_VECTOR, NUM_PITWORM_LEVELS),
        DEFINE_FIELD(CPitWorm, m_pBeam, FIELD_CLASSPTR),
        DEFINE_FIELD(CPitWorm, m_flBeamTime, FIELD_TIME),
        DEFINE_FIELD(CPitWorm, m_flBeamYaw, FIELD_FLOAT),
        DEFINE_FIELD(CPitWorm, m_fBeamOn, FIELD_BOOLEAN),
        DEFINE_FIELD(CPitWorm, m_pEyeGlow, FIELD_CLASSPTR),
        DEFINE_FIELD(CPitWorm, m_eyeBrightness, FIELD_INTEGER),

        DEFINE_FIELD(CPitWorm, m_vecGoalAngles, FIELD_VECTOR),
        DEFINE_FIELD(CPitWorm, m_vecCurAngles, FIELD_VECTOR),
        DEFINE_FIELD(CPitWorm, m_flNextAttackTime, FIELD_TIME),
        DEFINE_FIELD(CPitWorm, m_flNextIdleSoundTime, FIELD_TIME),
        DEFINE_FIELD(CPitWorm, m_fActivated, FIELD_BOOLEAN),
        DEFINE_FIELD(CPitWorm, m_iLaserHitTimes, FIELD_INTEGER),
        DEFINE_FIELD(CPitWorm, m_flDamage, FIELD_FLOAT),
        DEFINE_FIELD(CPitWorm, m_fBlink, FIELD_BOOLEAN),
        DEFINE_FIELD(CPitWorm, m_flPlayerDamage, FIELD_FLOAT),
        DEFINE_FIELD(CPitWorm, m_vecEmemyPos, FIELD_VECTOR),
        DEFINE_FIELD(CPitWorm, m_flNextBlink, FIELD_FLOAT),
};

IMPLEMENT_SAVERESTORE(CPitWorm, CBaseMonster)


const char *CPitWorm::pHitSilo[] =
{
    "tentacle/te_strike1.wav",
    "tentacle/te_strike2.wav",
};

const char* CPitWorm::pClangSounds[] =
{
    "pitworm/clang1.wav",
    "pitworm/clang2.wav",
    "pitworm/clang3.wav",
};

const char* CPitWorm::pAngrySounds[] =
{
    "pitworm/pit_worm_angry1.wav",
    "pitworm/pit_worm_angry2.wav",
    "pitworm/pit_worm_angry3.wav",
};


const char* CPitWorm::pSwipeSounds[] =
{
    "pitworm/pit_worm_attack_swipe1.wav",
    "pitworm/pit_worm_attack_swipe2.wav",
    "pitworm/pit_worm_attack_swipe3.wav",
};

const char* CPitWorm::pPainSounds[] =
{
    "pitworm/pit_worm_flinch2.wav",
    "pitworm/pit_worm_flinch2.wav",
    "pitworm/pit_worm_flinch2.wav",
};

const char* CPitWorm::pFlinchSounds[] =
{
    "pitworm/pit_worm_flinch1.wav",
    "pitworm/pit_worm_flinch2.wav",
};

const char* CPitWorm::pAlertSounds[] =
{
    "pitworm/pit_worm_alert.wav",
};

const char* CPitWorm::pIdleSounds[] =
{
    "pitworm/pit_worm_idle1.wav",
    "pitworm/pit_worm_idle2.wav",
    "pitworm/pit_worm_idle3.wav",
};

const char* CPitWorm::pDeathSounds[] =
{
    "pitworm/pit_worm_death.wav",
};

//=========================================================
// Spawn
//=========================================================
void CPitWorm::Spawn()
{
    Precache();

    pev->movetype = MOVETYPE_STEP;
    pev->solid = SOLID_SLIDEBOX;

    SET_MODEL(ENT(pev), "models/pit_worm_up.mdl");
    UTIL_SetSize(pev, Vector(-64, -128, 0), Vector(64, 128, 64));
    UTIL_SetOrigin(pev, pev->origin);

    pev->health = 150 * gSkillData.pwormHealth;
    pev->view_ofs = PITWORM_EYE_OFFSET;

    m_flFieldOfView = -1;

    pev->sequence = 0;
    ResetSequenceInfo();

    InitBoneControllers();

    SetThink(&CPitWorm::StartupThink);
    pev->nextthink = gpGlobals->time + 0.1;

    m_spawnAngles = pev->angles;
    m_flInitialYaw = m_spawnAngles.y;
    pev->ideal_yaw = m_flInitialYaw;

    m_IdealMonsterState = MONSTERSTATE_IDLE;// Assume monster will be idle, until proven otherwise

    m_IdealActivity = ACT_IDLE;

    m_flDistTooFar = 1024.0;
    m_flDistLook = 2048.0;

        // set eye position
    SetEyePosition();

    m_flLastSeen = 0.0f;
    m_flPrevSeen = 0.0f;
    m_iLevel = PITWORM_LEVEL1;

    m_pBeam = NULL;
    m_flBeamYaw = 0.0f;
    m_fBeamOn = FALSE;
    m_eyeBrightness = 0;
    m_slowMode = 4;
    m_afCapability = bits_CAP_MELEE_ATTACK1;
    m_slowTime = gpGlobals->time;
    m_vecCurAngles = Vector(0, pev->angles.y, 0);
    m_vecGoalAngles = Vector(0, pev->angles.y, 0);
    m_flNextAttackTime = gpGlobals->time;
    m_flNextIdleSoundTime = gpGlobals->time;
    m_bloodColor = BLOOD_COLOR_GREEN;
    m_fActivated = FALSE;
    m_vecEmemyPos = g_vecZero;
    m_flNextBlink = gpGlobals->time;
    m_fBlink = FALSE;
    m_iLaserHitTimes = 0;

        // Create the eye glow.
    CreateGlow();
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CPitWorm::Precache()
{
        PRECACHE_MODEL("models/pit_worm_up.mdl");

        PRECACHE_SOUND_ARRAY(pHitSilo);

        PRECACHE_SOUND("pitworm/pit_worm_alert.wav");
        PRECACHE_SOUND("pitworm/pit_worm_attack_eyeblast.wav");
        PRECACHE_SOUND("pitworm/pit_worm_attack_eyeblast_impact.wav");
        PRECACHE_SOUND("pitworm/pit_worm_death.wav");
        PRECACHE_SOUND("pitworm/pit_worm_alert(scream).wav");

        PRECACHE_SOUND_ARRAY(pClangSounds);
        PRECACHE_SOUND_ARRAY(pAngrySounds);
        PRECACHE_SOUND_ARRAY(pSwipeSounds);
        PRECACHE_SOUND_ARRAY(pFlinchSounds);
        PRECACHE_SOUND_ARRAY(pIdleSounds);
        PRECACHE_SOUND_ARRAY(pPainSounds);

        PRECACHE_MODEL("sprites/gworm_beam_02.spr");
        PRECACHE_MODEL("sprites/glow_grn.spr");
}

//=========================================================
// Classify
//=========================================================
int CPitWorm::Classify(void)
{
        return CLASS_ALIEN_MONSTER;
}

//=========================================================
// IdleSound
//=========================================================
void CPitWorm::IdleSound(void)
{
        EMIT_SOUND(ENT(pev), CHAN_VOICE, RANDOM_SOUND_ARRAY(pIdleSounds), VOL_NORM, ATTN_NORM);
}

//=========================================================
// AlertSound
//=========================================================
void CPitWorm::AlertSound(void)
{
        EMIT_SOUND(ENT(pev), CHAN_VOICE, RANDOM_SOUND_ARRAY(pAlertSounds), VOL_NORM, ATTN_NORM);
}

//=========================================================
// DeathSound
//=========================================================
void CPitWorm::DeathSound(void)
{
        EMIT_SOUND(ENT(pev), CHAN_VOICE, RANDOM_SOUND_ARRAY(pDeathSounds), VOL_NORM, ATTN_NORM);
}

//=========================================================
// AngrySound
//=========================================================
void CPitWorm::AngrySound(void)
{
        EMIT_SOUND(ENT(pev), CHAN_VOICE, RANDOM_SOUND_ARRAY(pAngrySounds), VOL_NORM, ATTN_NORM);
}

//=========================================================
// FlinchSound
//=========================================================
void CPitWorm::FlinchSound(void)
{
        EMIT_SOUND(ENT(pev), CHAN_VOICE, RANDOM_SOUND_ARRAY(pFlinchSounds), VOL_NORM, ATTN_NORM);
}

//=========================================================
// SwipeSound
//=========================================================
void CPitWorm::SwipeSound(void)
{
        EMIT_SOUND(ENT(pev), CHAN_VOICE, RANDOM_SOUND_ARRAY(pSwipeSounds), VOL_NORM, ATTN_NORM);
}

//=========================================================
// BeamSound
//=========================================================
void CPitWorm::BeamSound(void)
{
        EMIT_SOUND(ENT(pev), CHAN_VOICE, "pitworm/pit_worm_attack_eyeblast.wav", VOL_NORM, ATTN_NORM);
}

void CPitWorm::pPainSound(void)
{
    EMIT_SOUND(ENT(pev), CHAN_VOICE, RANDOM_SOUND_ARRAY(pPainSounds), VOL_NORM, ATTN_NORM);
}


//=========================================================
// SetObjectCollisionBox
//=========================================================
void CPitWorm::SetObjectCollisionBox(void)
{
        pev->absmin = pev->origin + Vector(-96, -310, 0);
        pev->absmax = pev->origin + Vector(96, 32, 512);
}

//=========================================================
// HandleAnimEvent
//=========================================================
void CPitWorm::HandleAnimEvent(MonsterEvent_t *pEvent)
{
        if(gpGlobals->time - m_slowTime <= 3.5)
            return;

        switch (pEvent->event)
        {
                case PITWORM_AE_SWIPE:	// bang
                {
                        Vector vecSrc, vecAngles;
                        GetAttachment(1, vecSrc, vecAngles);
                        Vector mins = pev->origin - Vector( 360, 430, 0 );
                        Vector maxs = pev->origin + Vector( 410, 100, 800 );

                        CBaseEntity *pList[10];
                        int count = UTIL_EntitiesInBox( pList, 10, mins, maxs, ( FL_CLIENT | FL_MONSTER ) );

                        if( count )
                        {
                            for( int i = 0; i < count; i++ )
                            {
                                // only clients and monsters
                                if( pList[i] != this && IRelationship( pList[i] ) > R_NO && pList[ i ]->pev->deadflag == DEAD_NO )	// this ent is one of our enemies. Barnacle tries to eat it.
                                {
                                    pList[i]->TakeDamage(VARS(pev), VARS(pev), gSkillData.pwormDmgSwipe, DMG_CLUB | DMG_SLASH);
                                }
                            }
                        }

                        // Shake the screen.
                        UTIL_EmitAmbientSound(ENT(pev), vecSrc, RANDOM_SOUND_ARRAY(pHitSilo), 1.0, ATTN_NORM, 0, 100);
                        UTIL_ScreenShake(vecSrc, 6.0, 3.0, 1.0, 768);
                        gpGlobals->force_retouch++;
                }
                break;
                case PITWORM_AE_EYEBLAST_START: // start killing swing
                {
                       // ALERT(at_console, "PITWORM_AE_EYEBLAST_START\n");

                        // Remove the beam if not NULL.
                        DestroyBeam();

                        Vector src, angles;
                        GetAttachment(0, src, angles);

                        // Create a new beam.
                        CreateBeam(src, angles, 32);

                        // Turn eye glow on.
                        EyeOn(255);

                        // Reset beam yaw.
                        m_flBeamYaw = 0.0f;
                        m_flBeamTime = gpGlobals->time + PITWORM_EYEBLAST_DURATION;
                }
                break;
                case PITWORM_AE_EYEBLAST_END: // end killing swing
                {}
                break;
        default:
                CBaseMonster::HandleAnimEvent(pEvent);

    }
}


void CPitWorm::SetYawSpeed( void )
{
    int ys;

    switch ( m_Activity )
    {
    case ACT_IDLE:
        ys = 100;
        break;
    default:
        ys = 90;
    }
    pev->yaw_speed = ys;
}

BOOL CPitWorm::CheckRangeAttack1(float flDot, float flDist)
{
    if(flDist >= 1024 && m_flNextAttackTime > gpGlobals->time)
    {
        if(m_hEnemy != 0 && m_hEnemy->IsAlive())
            return TRUE;

    }
    return FALSE;
}

BOOL CPitWorm::CheckMeleeAttack1(float flDot, float flDist)
{
    if(flDist <= 192 && flDot > 0.7)
        return TRUE;

    return FALSE;
}

void CPitWorm::TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType )
{
   if ( ptr->iHitgroup == HITGROUP_HEAD )
   {
       if (gpGlobals->time - m_slowTime > 3.5 )
       {
         if ( m_flDamage >= 65)
         {
           m_slowTime = gpGlobals->time;
           pev->sequence = LookupSequence("flinch1");

           if(m_fBeamOn)
           {
               DestroyBeam();
               EyeOff();
           }

           EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "pitworm/pit_worm_flinch2.wav", 1, 0.8, 0, 100);

           m_flDamage = 0;
         }
       }

       if(gpGlobals->time - m_flDamageTime > 0.5)
           m_flDamage = 0;

       if((pev->sequence != LookupSequence("scream") || pev->sequence != LookupSequence("flinch1")) && m_fActivated)
       {
          m_flDamage += flDamage;

          if(gpGlobals->time - m_flNextBlink >= 0.25)
               m_fBlink = TRUE;
       }

       UTIL_BloodDrips(ptr->vecEndPos, ptr->vecEndPos, m_bloodColor, flDamage * 10);
       UTIL_BloodDecalTrace(ptr, m_bloodColor);

       m_flDamageTime = gpGlobals->time;
     }
   else
   {
       if (pev->dmgtime != gpGlobals->time || RANDOM_LONG(0, 10) >= 0 )
       {
           UTIL_Ricochet(ptr->vecEndPos, RANDOM_LONG(0.5, 1.5));
           pev->dmgtime = gpGlobals->time;
       }
   }
}

void CPitWorm::Activate()
{
    if(m_hTargetEnt == 0)
        Remember(bits_MEMORY_ADVANCE_NODE);
}



void CPitWorm::CommandUse(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
        switch (useType)
        {
        case USE_OFF:
                //ALERT(at_console, "USE_OFF\n");
                break;
        case USE_ON:
                //ALERT(at_console, "USE_ON\n");
                break;
        case USE_SET:
                //ALERT(at_console, "USE_SET\n");
                break;
        case USE_TOGGLE:
        {
                m_fSequenceFinished = TRUE;
                pev->health = 0;

                SetThink(&CPitWorm::DyingThink);
                pev->nextthink = gpGlobals->time;
        }
        // ALERT(at_console, "USE_TOGGLE\n");
        break;
        }
}


//=========================================================
//
//=========================================================
int CPitWorm::TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
    // Don't take any acid damage -- BigMomma's mortar is acid
    if( bitsDamageType & DMG_ACID )
        flDamage = 0;

    if( !HasMemory( bits_MEMORY_PATH_FINISHED ) )
    {
        if( pev->health <= flDamage )
        {
            pev->health = flDamage + 1;
            Remember( bits_MEMORY_ADVANCE_NODE | bits_MEMORY_COMPLETED_NODE );
            ALERT( at_aiconsole, "BM: Finished node health!!!\n" );
        }
    }

    return CBaseMonster::TakeDamage( pevInflictor, pevAttacker, flDamage, bitsDamageType );
}


//=========================================================
// StartupThink
//=========================================================
void CPitWorm::StartupThink(void)
{
        m_flAdj = 350;
        Vector src = pev->origin;

        CBaseEntity *pEntity = NULL;

        pEntity = UTIL_FindEntityByTargetname(NULL, "pw_tleveldead");
        if (pEntity)
                m_vecLevels[PITWORM_LEVEL0] = pEntity->pev->origin;
        else
                m_vecLevels[PITWORM_LEVEL0] = Vector(src.x, src.y, PITWORM_LEVEL0_HEIGHT);

        pEntity = UTIL_FindEntityByTargetname(NULL, "pw_tlevel1");
        if (pEntity)
                m_vecLevels[PITWORM_LEVEL1] = pEntity->pev->origin;
        else
                m_vecLevels[PITWORM_LEVEL1] = Vector(src.x, src.y, PITWORM_LEVEL2_HEIGHT);

        pEntity = UTIL_FindEntityByTargetname(NULL, "pw_tlevel2");
        if (pEntity)
                m_vecLevels[PITWORM_LEVEL2] = pEntity->pev->origin;
        else
                m_vecLevels[PITWORM_LEVEL2] = Vector(src.x, src.y, PITWORM_LEVEL2_HEIGHT);

        pEntity = UTIL_FindEntityByTargetname(NULL, "pw_tlevel3");
        if (pEntity)
                m_vecLevels[PITWORM_LEVEL3] = pEntity->pev->origin;
        else
                m_vecLevels[PITWORM_LEVEL3] = Vector(src.x, src.y, PITWORM_LEVEL3_HEIGHT);

        // Set altitude limits.
        m_flMinZ = m_vecLevels[PITWORM_LEVEL0].z;
        m_flMaxZ = m_vecLevels[PITWORM_LEVEL3].z;

        // Set default level.
        UTIL_SetOrigin(pev, IdealPosition(m_iLevel));

        SetThink(&CPitWorm::HuntThink);
        SetTouch(&CPitWorm::WormTouch);
        SetUse(&CPitWorm::CommandUse);
        pev->nextthink = gpGlobals->time + 0.1;
}

//=========================================================
//
//=========================================================
void CPitWorm::StartupUse(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
        SetThink(&CPitWorm::HuntThink);
        pev->nextthink = gpGlobals->time + 0.1;
        SetUse(&CPitWorm::CommandUse);
}

//=========================================================
// NullThink
//=========================================================
void CPitWorm::NullThink(void)
{
        StudioFrameAdvance();
        pev->nextthink = gpGlobals->time + 0.5;
}

//=========================================================
// DyingThink
//=========================================================
void CPitWorm::DyingThink(void)
{
        pev->nextthink = gpGlobals->time + 0.1;
        GlowShellUpdate();
        DispatchAnimEvents();
        StudioFrameAdvance();

        if (pev->deadflag == DEAD_NO)
        {
                DestroyGlow();
                DeathSound();
                pev->deadflag = DEAD_DYING;
        }

        if (pev->deadflag == DEAD_DYING)
        {
                if (fabs(pev->origin.z - m_flMaxZ) < 16)
                {
                        pev->velocity = Vector(0, 0, 0);
                        pev->deadflag = DEAD_DEAD;
                }
        }

        if(m_fBeamOn)
            DestroyBeam();

        if (m_fSequenceFinished)
        {
                if (pev->sequence != LookupSequence("death"))
                {
                        pev->sequence = LookupSequence("death");
                        pev->frame = 0;
                        pev->framerate = 0.2f; // Set the framerate manually until we find a better fix.
                }
                else
                {
                        UTIL_Remove(this);
                }
        }
}

//=========================================================
// HuntThink
//=========================================================
void CPitWorm::HuntThink(void)
{
        pev->nextthink = gpGlobals->time + 0.1;
        GlowShellUpdate();
        DispatchAnimEvents();
        StudioFrameAdvance();

        // Update the eye glow.
        EyeUpdate();

        if(!m_fActivated)
        {
            if(m_hEnemy == 0)
            {
                Look(4096);
                m_hEnemy = BestVisibleEnemy();
            }
            else
            {
                if((pev->origin - m_hEnemy->pev->origin).Length() <= 936)
                {
                    EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "pitworm/pit_worm_alert(scream).wav", 1, 0.8, 0, 100);
                    pev->frame = 0;
                    pev->sequence = LookupSequence("scream");
                    ResetSequenceInfo();
                    m_fActivated = TRUE;
                }
                else
                {
                    pev->nextthink = gpGlobals->time + 0.01f;
                    return;
                }
            }
        }

        if (m_hEnemy)
        {
                // Update body controllers.
                UpdateBodyControllers();

                //float flDist = (m_hEnemy->pev->origin - m_vecLevels[m_iLevel]).Length();
                //ALERT(at_console, "pitworm: Distance from eye %f\n", flDist);

                // Update beam if we need to.
                if (m_fBeamOn)
                {
                        if (m_flBeamTime > gpGlobals->time && !m_fSequenceFinished)
                        {
                                Vector src, target, angles;
                                GetAttachment(0, src, angles);
                                UpdateBeamPoints(m_hEnemy, &target);

                                TraceResult tr;
                                UTIL_TraceLine(src, target + ((target - src).Normalize() * 1000), dont_ignore_monsters, ENT(pev), &tr);

                                UpdateBeam(src, tr.vecEndPos);

                                // Test to see if we hit
                                CBaseEntity *pHurt = CBaseEntity::Instance(tr.pHit);

                                if (pHurt->pev->takedamage)
                                {
                                    if(m_flPlayerDamage <= gSkillData.pwormDmgBeam)
                                    {
                                      pHurt->TakeDamage(VARS(pev), VARS(pev), 1, DMG_GENERIC);
                                      UTIL_ScreenShake(pHurt->pev->origin, 1, 10, 0.1, 1);

                                      if(m_iLaserHitTimes >= 15)
                                      {
                                        UTIL_BloodDrips(tr.vecEndPos, tr.vecEndPos, BLOOD_COLOR_RED, 128);
                                        m_iLaserHitTimes = 0;
                                      }

                                    }
                                    m_flPlayerDamage += gSkillData.pwormDmgBeam/100;
                                }
                                else
                                {
                                    UTIL_DecalTrace(&tr, RANDOM_LONG(1,2));

                                    if(m_iLaserHitTimes >= 8)
                                    {
                                      UTIL_Sparks(tr.vecEndPos);
                                      m_iLaserHitTimes = 0;
                                    }
                                }

                                m_iLaserHitTimes++;
                        }
                        else
                        {
                                // Remove the beam.
                                DestroyBeam();

                                // Turn eye glow off.
                                EyeOff();

                                // Reset beam yaw.
                                m_flBeamYaw = 0.0f;

                                // Reset beam time.
                                m_flBeamTime = 0.0f;
                        }

                        pev->nextthink = gpGlobals->time + 0.01f;
                        return;
                }
        }

        switch (m_iLevel)
        {
        case PITWORM_LEVEL0: pev->view_ofs = PITWORM_EYE_OFFSET; break;
        case PITWORM_LEVEL1: pev->view_ofs = PITWORM_EYE_OFFSET; break;
        case PITWORM_LEVEL2: pev->view_ofs = PITWORM_EYE_OFFSET; break;
        case PITWORM_LEVEL3: pev->view_ofs = PITWORM_EYE_OFFSET; break;
        }
        {
                float currentZ = pev->origin.z;
                float desiredZ = m_posDesired.z - m_flAdj;

                if (currentZ > desiredZ)
                {
                        pev->origin.z -= 8;

                        if (pev->origin.z < desiredZ)
                                pev->origin.z = desiredZ;
                }
                else if (currentZ < desiredZ)
                {
                        pev->origin.z += 8;

                        if (pev->origin.z > desiredZ)
                                pev->origin.z = desiredZ;
                }
        }

        // if dead, force cancelation of current animation
        if (pev->health <= 0)
        {
                SetThink(&CPitWorm::DyingThink);
                m_fSequenceFinished = TRUE;
                return;
        }

        // get new sequence
        if (m_fSequenceFinished)
        {
                pev->frame = 0;
                NextActivity();
                ResetSequenceInfo();
        }

        // look for current enemy
        if (m_hEnemy != 0)
        {
                // Update level based on enemy's altitude (z).
                m_iLevel = Level(m_hEnemy->pev->origin.z);

                if (FVisible(m_hEnemy))
                {
                        if (m_flLastSeen < gpGlobals->time - 5)
                                m_flPrevSeen = gpGlobals->time;

                        m_flLastSeen = gpGlobals->time;
                        m_posTarget = m_hEnemy->pev->origin;
                        m_vecTarget = (m_posTarget - pev->origin).Normalize();
                        m_vecDesired = m_vecTarget;
                        m_posDesired = IdealPosition(m_iLevel);
                }
        }

        if (m_flNextIdleSoundTime < gpGlobals->time)
        {
                //IdleSound();
                // Use flinch sounds.
                FlinchSound();
                m_flNextIdleSoundTime = gpGlobals->time + RANDOM_LONG(4.0f, 4.1f);
        }

        // don't go too high
        if (m_posDesired.z > m_flMaxZ)
                m_posDesired.z = m_flMaxZ;

        // don't go too low
        if (m_posDesired.z < m_flMinZ)
                m_posDesired.z = m_flMinZ;

        if(RANDOM_LONG(0, 20) == 5 && gpGlobals->time - m_flNextBlink >= RANDOM_FLOAT(3,5))
        {
            m_fBlink = TRUE;
            m_flNextBlink = gpGlobals->time;
        }

        if(gpGlobals->time - m_flNextBlink >= 0.05 && m_fBlink && !m_fBeamOn)
        {
            if(pev->skin < 3)
            {
                pev->skin++;
                m_flNextBlink = gpGlobals->time;
            }
            else
            {
                pev->skin = 0;
                m_fBlink = FALSE;
            }
        }
}

//=========================================================
//
//=========================================================
void CPitWorm::WormTouch(CBaseEntity* pOther)
{
        return;
}

//=========================================================
//
//=========================================================
int CPitWorm::CanPlaySequence(BOOL fDisregardState)
{
        return TRUE;
}

//=========================================================
// NextActivity
//=========================================================
void CPitWorm::NextActivity()
{
        if(!m_fActivated)
            return;

        UTIL_MakeAimVectors(pev->angles);

        Vector forward = gpGlobals->v_forward;
        forward.z = 0;

        float flDist = (m_vecDesired - m_vecLevels[m_iLevel]).Length();
        float flDot = DotProduct(m_vecDesired, forward);
        float flDotSpawnAngles = DotProduct(m_vecDesired, m_spawnAngles);

        if (m_hEnemy == 0)
        {
                m_flLastSeen = gpGlobals->time;
                m_hEnemy = UTIL_PlayerByIndex(1);
        }

        if (m_hEnemy == 0)
        {
                Look(4096);
                m_hEnemy = BestVisibleEnemy();
        }

        if (m_hEnemy != 0)
        {

                if(gpGlobals->time - m_slowTime <= 3.5)
                    return;

                m_vecDesired = m_hEnemy->pev->origin;
                flDist = (m_vecDesired - m_vecLevels[m_iLevel]).Length();
                flDot = DotProduct(m_vecDesired, forward);
                flDotSpawnAngles = DotProduct(m_vecDesired, Vector(0, -1, 0));

                if (m_flLastSeen + 5 > gpGlobals->time && (m_flNextAttackTime < gpGlobals->time))
                {
                        Vector enemyDir = m_hEnemy->pev->origin - pev->origin;
                        Vector spawnAngles = m_spawnAngles;
                        Vector up = CrossProduct(enemyDir, spawnAngles);
                        float dist = UTIL_AngleDistance(m_hEnemy->pev->origin.y, pev->origin.y);

                        if (m_iLevel < PITWORM_LEVEL3)
                        {
                                if (FVisible(m_hEnemy) && flDot > 0.95f || dist <= -140 && dist >= -326)
                                {
                                        if (flDist > PITWORM_EYEBLAST_ATTACK_DIST)
                                        {
                                                pev->sequence = LookupSequence("eyeblast");
                                                BeamSound();
                                        }
                                        else
                                        {
                                                pev->sequence = LookupSequence("attacklow");
                                                SwipeSound();
                                        }
                                }
                                else
                                {
                                        if (up.z < 0)
                                        {
                                                pev->sequence = LookupSequence("doorclaw2");
                                        }
                                        else
                                        {
                                                if (m_iLevel < PITWORM_LEVEL2)
                                                {
                                                        pev->sequence = LookupSequence("doorclaw3");
                                                }
                                                else
                                                {
                                                        pev->sequence = LookupSequence("doorclaw1");
                                                }
                                        }
                                }
                        }
                        else
                        {
                                if (flDist > PITWORM_EYEBLAST_ATTACK_DIST)
                                {
                                        pev->sequence = LookupSequence("eyeblast");
                                        BeamSound();
                                }
                                else
                                {
                                        if (up.z < 0)
                                        {
                                                pev->sequence = LookupSequence("platclaw1");

                                                if (RANDOM_LONG(0, 1) == 1) SwipeSound();
                                        }
                                        else
                                        {
                                                pev->sequence = LookupSequence("platclaw2");
                                                if (RANDOM_LONG(0, 1) == 1) SwipeSound();
                                        }
                                }
                        }

                        m_flNextAttackTime = gpGlobals->time + RANDOM_FLOAT(3, 3.1f);
                        return;
                }
        }

        if (m_hEnemy)
        {
                pev->sequence = LookupSequence("idle2");
                return;
        }

        FloatSequence();
}

//=========================================================
// FloatSequence
//=========================================================
void CPitWorm::FloatSequence(void)
{
        switch (RANDOM_LONG(0, 1))
        {
        case 0:
                pev->sequence = LookupSequence("idle");
                break;

        case 1:
                pev->sequence = LookupSequence("idle2");
                break;
        }
}

//=========================================================
// IdealPosition
//=========================================================
int CPitWorm::Level(float dz)
{
        if (dz < PITWORM_LEVEL1_HEIGHT)
                return PITWORM_LEVEL0;

        if (dz < PITWORM_LEVEL2_HEIGHT)
                return PITWORM_LEVEL1;

        if (dz < PITWORM_LEVEL3_HEIGHT)
                return PITWORM_LEVEL2;

        return PITWORM_LEVEL3;
}

//=========================================================
// IdealPosition
//=========================================================
const Vector& CPitWorm::IdealPosition(const int level) const
{
        switch (level)
        {
        case PITWORM_LEVEL0:
                return m_vecLevels[PITWORM_LEVEL0];
        case PITWORM_LEVEL1:
                return m_vecLevels[PITWORM_LEVEL1];
        case PITWORM_LEVEL2:
                return m_vecLevels[PITWORM_LEVEL2];
        case PITWORM_LEVEL3:
                return m_vecLevels[PITWORM_LEVEL3];
        }

        return m_vecLevels[PITWORM_LEVEL3];
}

//=========================================================
// IdealPosition
//=========================================================
const Vector& CPitWorm::IdealPosition(const float dz) const
{
        if (dz < PITWORM_LEVEL1_HEIGHT)
                return m_vecLevels[PITWORM_LEVEL0];

        if (dz < PITWORM_LEVEL2_HEIGHT)
                return m_vecLevels[PITWORM_LEVEL1];

        if (dz < PITWORM_LEVEL3_HEIGHT)
                return m_vecLevels[PITWORM_LEVEL2];

        return m_vecLevels[PITWORM_LEVEL3];
}

//=========================================================
// IdealPosition
//=========================================================
const Vector& CPitWorm::IdealPosition(CBaseEntity* pEnemy) const
{
        return IdealPosition(pEnemy->pev->origin.z);
}

#define clamp( val, min, max ) ( ((val) > (max)) ? (max) : ( ((val) < (min)) ? (min) : (val) ) )

//=========================================================
// UpdateBodyControllers
//=========================================================
void CPitWorm::UpdateBodyControllers(void)
{
        if (m_hEnemy == 0 || m_fBeamOn)
                return;

        ASSERT(m_hEnemy);

        Vector vecGoal = m_hEnemy->EyePosition();
        Vector vecEye = EyePosition();

        Vector target = (vecGoal - vecEye).Normalize();
        Vector angles = UTIL_VecToAngles(target);
        float yaw = UTIL_VecToYaw(target);
        float pitch = angles.x;
        float targetYaw = yaw - m_flInitialYaw;


        if (pitch < -180)
                pitch += 360;

        if (pitch > 180)
                pitch -= 360;

        float bodyYaw = clamp(targetYaw, PITWORM_BODY_YAW_MIN, PITWORM_BODY_YAW_MAX);


        float targetBody = (fabs(targetYaw) * (bodyYaw - 5)) / 90;

        // Convert value to clamp it between min / max value of bone controller.
        float eyePitch = pitch;
        float eyeYaw = (targetBody * PITWORM_EYE_YAW_MAX) / PITWORM_BODY_YAW_MAX;


        // Clamp those values (ensure they stay between min / max).
        eyePitch = clamp(eyePitch, PITWORM_EYE_PITCH_MIN, PITWORM_EYE_PITCH_MAX);
        eyeYaw = clamp(eyeYaw, PITWORM_EYE_YAW_MIN, PITWORM_EYE_YAW_MAX);

        // Update body yaw controller value.
        SetBoneController(PITWORM_CONTROLLER_BODY_YAW, bodyYaw);

        // Update eye pitch controller value.
        SetBoneController(PITWORM_CONTROLLER_EYE_PITCH, -eyePitch);

        // Update eye yaw contoller value.
        SetBoneController(PITWORM_CONTROLLER_EYE_YAW, eyeYaw);
}

//=========================================================
// CreateBeam
//=========================================================
void CPitWorm::CreateBeam(const Vector& src, const Vector& target, int width)
{
        pev->skin = 0;

        m_pBeam = CBeam::BeamCreate("sprites/gworm_beam_02.spr", width);
        if (m_pBeam)
        {
                m_pBeam->PointsInit(src, target);
                m_pBeam->SetBrightness(200);
                m_pBeam->SetColor(0, 255, 0);
                m_pBeam->SetNoise(0);
                m_pBeam->SetFrame(0);
                m_pBeam->SetScrollRate(10);
                m_pBeam->RelinkBeam();

                m_fBeamOn = TRUE;
        }
}

//=========================================================
// DestroyBeam
//=========================================================
void CPitWorm::DestroyBeam(void)
{
        if (m_pBeam)
        {
                UTIL_Remove(m_pBeam);
                m_pBeam = NULL;

                m_fBeamOn = FALSE;
                m_vecEmemyPos = g_vecZero;
                m_flPlayerDamage = 0;
                m_iLaserHitTimes = 0;
        }
}

//=========================================================
// UpdateBeam
//=========================================================
void CPitWorm::UpdateBeam(const Vector& src, const Vector& target)
{
        if (m_pBeam)
        {
                m_pBeam->SetStartPos(src);
                m_pBeam->SetEndPos(target);
                m_pBeam->SetColor(0, 255, 0);
                m_pBeam->SetBrightness(200);
                m_pBeam->RelinkBeam();
        }
}

//=========================================================
// SetupBeamPoints
//=========================================================
void CPitWorm::SetupBeamPoints(CBaseEntity* pEnemy, Vector* vecleft, Vector* vecRight)
{
        Vector forward, right, up;

        if(m_vecEmemyPos == g_vecZero)
            m_vecEmemyPos = pEnemy->Center() + Vector(-10,0,25);

        if (pEnemy->IsPlayer())
                UTIL_MakeAimVectors(pEnemy->pev->v_angle);
        else
                UTIL_MakeAimVectors(pEnemy->pev->angles);

        forward = gpGlobals->v_forward;
        right = gpGlobals->v_right;
        up = gpGlobals->v_up;

        *vecleft = m_vecEmemyPos + (right * -16);
        *vecRight = m_vecEmemyPos + (right * 32);
}

//=========================================================
// UpdateBeamPoints
//=========================================================
void CPitWorm::UpdateBeamPoints(CBaseEntity* pEnemy, Vector* target)
{
        Vector left, right;
        Vector dir;
        float delta;

        delta = 1.0f / (PITWORM_EYEBLAST_DURATION * 2); // 0.24 approximatly. (248 miliseconds)

                                                                                          // Update beam yaw.
        m_flBeamYaw += delta;

        // Convert to distance to make the correct interpolation.
        float flDist = (6.25 * m_flBeamYaw) / PITWORM_EYEBLAST_DURATION;

        SetupBeamPoints(pEnemy, &left, &right);

        // Direction from right to left.
        dir = (left - right).Normalize();

        // This is the result representing the next beam target
        // position along the player's left and right computed beam
        // positions.
        *target = right + (dir * (flDist*1.2));
}

//=========================================================
// CreateGlow
//=========================================================
void CPitWorm::CreateGlow(void)
{
        m_pEyeGlow = CSprite::SpriteCreate("sprites/glow_grn.spr", pev->origin, TRUE);
        m_pEyeGlow->SetTransparency(kRenderGlow, 255, 255, 255, 0, kRenderFxNoDissipation);
        m_pEyeGlow->SetAttachment(edict(), 1);
        m_pEyeGlow->SetScale(2.0f);
}

//=========================================================
// DestroyGlow
//=========================================================
void CPitWorm::DestroyGlow(void)
{
        if (m_pEyeGlow)
        {
                UTIL_Remove(m_pEyeGlow);
                m_pEyeGlow = NULL;
        }
}

//=========================================================
// EyeOn
//=========================================================
void CPitWorm::EyeOn(int level)
{
        m_eyeBrightness = level;
}

//=========================================================
// EyeOff
//=========================================================
void CPitWorm::EyeOff(void)
{
        m_eyeBrightness = 0;
}

//=========================================================
// EyeUpdate
//=========================================================
void CPitWorm::EyeUpdate(void)
{
        if (m_pEyeGlow)
        {
                m_pEyeGlow->pev->renderamt = UTIL_Approach(m_eyeBrightness, m_pEyeGlow->pev->renderamt, 100);
                if (m_pEyeGlow->pev->renderamt == 0)
                        m_pEyeGlow->pev->effects |= EF_NODRAW;
                else
                        m_pEyeGlow->pev->effects &= ~EF_NODRAW;

                UTIL_SetOrigin(m_pEyeGlow->pev, pev->origin);
        }
}
