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
// Zombie
//=========================================================

// UNDONE: Don't flinch every time you get hit

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"schedule.h"
#include	"weapons.h"
#include	"animation.h"
#include	"soundent.h"

extern DLL_GLOBAL int		g_iSkillLevel;
//=========================================================
// Monster's Anim Events Go Here
//=========================================================

class CCofMs4 : public CBaseMonster
{
public:
	void Spawn( void );
	void Precache( void );
	void SetYawSpeed( void );
	int  Classify ( void );
	void HandleAnimEvent( MonsterEvent_t *pEvent );

	void CheckAmmo ( void );
	void RunAI( void );

	float m_flNextFlinch;
	float m_flNextPainTime;

	int IgnoreConditions ( void );
	void Killed( entvars_t *pevAttacker, int iGib );

	Schedule_t *GetScheduleOfType ( int Type );
	Schedule_t *GetSchedule ( void );

	void PainSound( void );
	void AlertSound( void );
	void IdleSound( void );
	void AttackSound( void );
	void TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType);
	void SetActivity ( Activity NewActivity );

	// No range attacks
	BOOL CheckRangeAttack1 ( float flDot, float flDist );
	BOOL CheckRangeAttack2 ( float flDot, float flDist ) { return FALSE; }
	BOOL CheckMeleeAttack1 ( float flDot, float flDist ) { return FALSE; }
	int TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType );
};

LINK_ENTITY_TO_CLASS( monster_cof_ms4, CCofMs4 );

void CCofMs4 :: RunAI( void )
{
	CBaseMonster :: RunAI();
	/*
	if(pev->sequence == LookupActivity ( ACT_WALK )
	|| pev->sequence == LookupActivity ( ACT_WALK_SCARED )){
    m_flGroundSpeed = 100;
	}*/
	if(pev->frags == 8 ){
	m_duckseq = 4;//老八蹲
	}
	else if(m_duckseq != 0){
	m_duckseq = 0;
	}
}

//=========================================================
// SetActivity 
//=========================================================
void CCofMs4 :: SetActivity ( Activity NewActivity )
{
	int	iSequence = ACTIVITY_NOT_AVAILABLE;
	void *pmodel = GET_MODEL_PTR( ENT(pev) );

	switch ( NewActivity)
	{
	case ACT_RUN:
		if ( pev->frags == 8 )
		{
			iSequence = LookupActivity ( ACT_CROUCHIDLE );
		}
		else
		{
			iSequence = LookupActivity ( NewActivity );
		}
		break;
	case ACT_WALK:
		if ( pev->frags == 8 )
		{
			iSequence = LookupActivity ( ACT_CROUCHIDLE );
		}
		else
		{
			iSequence = LookupActivity ( NewActivity );
		}
		break;
	case ACT_RELOAD:
		if ( pev->frags == 8 )
		{
			iSequence = LookupSequence( "suicide_duck" );
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
	
	if(pev->frags != 8){
		if ( m_IdealMonsterState == MONSTERSTATE_DEAD || pev->deadflag != DEAD_NO || pev->health <= 0 || m_die >= 1 )
		{
			if(NewActivity <= ACT_EAT || pev->deadflag == DEAD_DEAD){
				if(NewActivity != ACT_FALL){
				return;
				}
			}
		}
	}

	// Set to the desired anim, or default anim if the desired is not present
	if ( iSequence > ACTIVITY_NOT_AVAILABLE )
	{
		if ( pev->sequence != iSequence || !m_fSequenceLoops )
		{
			// don't reset frame between walk and run
			if ( !(m_Activity == ACT_WALK || m_Activity == ACT_RUN) || !(NewActivity == ACT_WALK || NewActivity == ACT_RUN))
				pev->frame = 0;
		}

		pev->sequence		= iSequence;	// Set to the reset anim (if it's there)
		ResetSequenceInfo( );
		SetYawSpeed();
	}
	else
	{
		// Not available try to get default anim
		ALERT ( at_aiconsole, "%s has no sequence for act:%d\n", STRING(pev->classname), NewActivity );
		//pev->sequence		= 0;	// Set to the reset anim (if it's there)
	}

	m_Activity = NewActivity; // Go ahead and set this so it doesn't keep trying when the anim is not present
	
	// In case someone calls this with something other than the ideal activity
	m_IdealActivity = m_Activity;

}


//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int	CCofMs4 :: Classify ( void )
{
	return	CLASS_HUMAN_ASS;
}

BOOL CCofMs4 :: CheckRangeAttack1 ( float flDot, float flDist )
{
	if ( m_cAmmoLoaded <= 1 ){
	return FALSE;
	}

	if(pev->frags == 8){
		if (flDist <= 1200 && m_hEnemy != NULL && flDot >= 0.6)
		{
		m_facing_fucking_mode = 1;
		return TRUE;
		}
	}

			if (flDist <= 1500 && m_hEnemy != NULL && flDot >= 0.75)
			{
				if (m_hEnemy->IsAlive() ){
					if(pev->sequence == LookupActivity ( ACT_WALK )){
					pev->sequence = LookupActivity ( ACT_WALK_SCARED );
					ResetSequenceInfo( );
					pev->frame = 0;
					}
				}
				else if(pev->sequence == LookupActivity ( ACT_WALK_SCARED )){
					pev->sequence = LookupActivity ( ACT_WALK );
					ResetSequenceInfo( );
					pev->frame = 0;
				}
			}
			else
			{
					if(pev->sequence == LookupActivity ( ACT_WALK_SCARED )){
					pev->sequence = LookupActivity ( ACT_WALK );
					ResetSequenceInfo( );
					pev->frame = 0;
					}
			}

	return FALSE;
}

void CCofMs4::TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType)
{
	/*
	if (ptr->iHitgroup == 1)
	{
		if(GetBodygroup( 1 ) == 0 && pev->health <= flDamage){
		SetBodygroup( 0, 1 );
		Vector vecGunPos,vecGunAngles;
		GetAttachment( 0, vecGunPos, vecGunAngles );
		FX_Explosion( vecGunPos, 236 );
		SpawnBlood(vecGunPos, BloodColor(), 100);
		FX_Trail(vecGunPos, entindex(), PROJ_GUTS );
		EMIT_SOUND(ENT(pev), CHAN_BODY, "newadd/zom_headburst.wav", 1, ATTN_NORM);	
		pev->health = 1;
		}
	}*/

	CBaseMonster::TraceAttack( pevAttacker, flDamage, vecDir, ptr, bitsDamageType );
}

Schedule_t* CCofMs4 :: GetScheduleOfType ( int Type )
{
	return CBaseMonster::GetScheduleOfType( Type );
}

//=========================================================
// GetSchedule - Decides which type of schedule best suits
// the monster's current state and conditions. Then calls
// monster's member function to get a pointer to a schedule
// of the proper type.
//=========================================================
Schedule_t *CCofMs4 :: GetSchedule ( void )
{
		if ( HasConditions ( bits_COND_NO_AMMO_LOADED ) )
		{
		return GetScheduleOfType ( SCHED_RELOAD_DEEP );
		}

	return CBaseMonster::GetSchedule();
}
//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CCofMs4 :: SetYawSpeed ( void )
{
	if(pev->frags == 8){
	pev->yaw_speed = 60;
	}
	else{
	pev->yaw_speed = 180;
	}
}

int CCofMs4 :: TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	if(pev->deadflag == DEAD_DYING && m_crouchmode == 1){
	return 0;
	}

	return CBaseMonster::TakeDamage( pevInflictor, pevAttacker, flDamage, bitsDamageType );
}

void CCofMs4::Killed( entvars_t *pevAttacker, int iGib )
{
	if ( GetBodygroup( 1 ) == 0 && m_cAmmoLoaded > 1 && !m_undropgun)
	{// drop the gun!
		Vector vecGunPos;
		Vector vecGunAngles;

		SetBodygroup( 1, 1 );

		GetAttachment( 1, vecGunPos, vecGunAngles );
		
		CBaseEntity *pGun = DropItem( "weapon_glock", vecGunPos, vecGunAngles );

		if ( pGun )
		{
			pGun->pev->velocity = pev->velocity + Vector (RANDOM_FLOAT(-100,100), RANDOM_FLOAT(-100,100), RANDOM_FLOAT(200,300));
			pGun->pev->avelocity = Vector ( 0, RANDOM_FLOAT( 200, 400 ), 0 );
		}
	}

	CBaseMonster::Killed( pevAttacker, iGib );

	if(m_LastHitGroup == 1 && GetBodygroup( 0 ) == 0 && m_gibed == 0){
	SetBodygroup( 0, 1 );
	Vector vecGunPos,vecGunAngles;
	GetAttachment( 0, vecGunPos, vecGunAngles );
	FX_Explosion( vecGunPos, 236 );
	SpawnBlood(vecGunPos, BloodColor(), 100);
	FX_Trail(vecGunPos, entindex(), PROJ_GUTS );
	EMIT_SOUND(ENT(pev), CHAN_BODY, "newadd/zom_headburst.wav", 1, ATTN_NORM);	
	}
}

void CCofMs4 :: PainSound( void )
{

}

void CCofMs4 :: AlertSound( void )
{
	
}

void CCofMs4 :: IdleSound( void )
{

}

void CCofMs4 :: AttackSound( void )
{
	
}


void CCofMs4 :: CheckAmmo ( void )
{
	if ( m_cAmmoLoaded <= 1 )
	{
		m_cAmmoLoaded = 0;
		SetConditions(bits_COND_NO_AMMO_LOADED);
	}
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CCofMs4 :: HandleAnimEvent( MonsterEvent_t *pEvent )
{
	switch( pEvent->event )
	{
		case 1:
		{
			SetBodygroup( 0, 1 );
			Vector vecGunPos,vecGunAngles;
			GetAttachment( 0, vecGunPos, vecGunAngles );
			FX_Explosion( vecGunPos, 236 );
			SpawnBlood(vecGunPos, BloodColor(), 100);
			FX_Trail(vecGunPos, entindex(), PROJ_GUTS );
			m_crouchmode = 1;


			EMIT_SOUND(ENT(pev), CHAN_WEAPON, "weapons/pl_gun3.wav", 1, 0.5);
			EMIT_SOUND(ENT(pev), CHAN_BODY, "newadd/zom_headburst.wav", 1, ATTN_NORM);	
			pev->effects = pev->effects | EF_MUZZLEFLASH;
			CSoundEnt::InsertSound ( bits_SOUND_COMBAT, pev->origin, 512, 0.3 );

			m_cAmmoLoaded--;
		}
		break;

		case 2:
		{
			if(m_cAmmoLoaded > 1){
			Vector vecGunPos,vecGunAngles;
			GetAttachment( 1, vecGunPos, vecGunAngles );
			UTIL_MakeVectors(pev->angles);

			Vector vecShootDir = ShootAtEnemy( vecGunPos );

			FireBullets( 1, vecGunPos, vecShootDir * 0.8 + gpGlobals->v_forward * 0.2, Vector(0.03,0.03,0.03), 2048, BULLET_MONSTER_9MM, 1 );

			EMIT_SOUND(ENT(pev), CHAN_WEAPON, "weapons/pl_gun3.wav", 1, 0.5);
			pev->effects = pev->effects | EF_MUZZLEFLASH;

			CSoundEnt::InsertSound ( bits_SOUND_COMBAT, pev->origin, 512, 0.3 );

			m_cAmmoLoaded--;
			}
			else{
			RouteClear();
			}
		}
		break;

		case 3:
		{
			Killed( pev, GIB_NORMAL );
		}
		break;

		default:
			CBaseMonster::HandleAnimEvent( pEvent );
			break;
	}
}

//=========================================================
// Spawn
//=========================================================
void CCofMs4 :: Spawn()
{
	Precache( );
	SET_MODEL(ENT(pev), "models/suicider.mdl");
	UTIL_SetSize( pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX );

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	m_bloodColor		= BLOOD_COLOR_RED;

	if (g_iSkillLevel == SKILL_HARD){
	pev->health			= 40;
	}
	else{
	pev->health			= 30;
	}

	pev->view_ofs		= VEC_VIEW;// position of the eyes relative to monster's origin.
	m_flFieldOfView		= 0.3;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState		= MONSTERSTATE_NONE;
	m_afCapability		= bits_CAP_DOORS_GROUP;

	pev->body           = 0;

	m_cAmmoLoaded		= 15;
	m_aimenemy_mod		= 5;
	m_headdef			= 1;
	m_chase_mode		= 2;

	MonsterInit();

	m_ignoreFail_MAX = 30;
	m_ignoreFail_OFF = 0;
	m_MoveFail_FuckRoad = TRUE;
	m_MoveFail_SimpleRoad = TRUE;
//	m_forcefuckdoor  = TRUE;

	m_killed_exp = 30;
	m_rpgms_level = 20;
	pev->netname = MAKE_STRING( "Ollegey" );

	m_selfmode = TRUE;

	if(pev->frags == 8){//老八模式
	m_singdelay_max = 0;//0反应
	m_singdelay_use = m_singdelay_max;
	}
	else{
	m_singdelay_max = 1;//快速反应
	m_singdelay_use = m_singdelay_max;
	}
}
//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CCofMs4 :: Precache()
{
	PRECACHE_MODEL("models/suicider.mdl");
}	

int CCofMs4::IgnoreConditions ( void )
{
	int iIgnore = CBaseMonster::IgnoreConditions();

	return iIgnore;
}