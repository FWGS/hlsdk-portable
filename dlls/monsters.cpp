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
/*

===== monsters.cpp ========================================================

  Monster-related utility code

*/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "nodes.h"
#include "monsters.h"
#include "animation.h"
#include "saverestore.h"
#include "weapons.h"
#include "scripted.h"
#include "squadmonster.h"
#include "decals.h"
#include "soundent.h"
#include "gamerules.h"
#include "game.h"
#include "player.h"

#define MONSTER_CUT_CORNER_DIST		8 // 8 means the monster's bounding box is contained without the box of the node in WC

Vector VecBModelOrigin( entvars_t *pevBModel );

extern DLL_GLOBAL	BOOL	g_fDrawLines;
extern DLL_GLOBAL	short	g_sModelIndexLaser;// holds the index for the laser beam
extern DLL_GLOBAL	short	g_sModelIndexLaserDot;// holds the index for the laser beam dot

extern CGraph WorldGraph;// the world node graph

// Global Savedata for monster
// UNDONE: Save schedule data?  Can this be done?  We may
// lose our enemy pointer or other data (goal ent, target, etc)
// that make the current schedule invalid, perhaps it's best
// to just pick a new one when we start up again.
TYPEDESCRIPTION	CBaseMonster::m_SaveData[] =
{
	DEFINE_FIELD( CBaseMonster, m_hEnemy, FIELD_EHANDLE ),
	DEFINE_FIELD( CBaseMonster, m_hTargetEnt, FIELD_EHANDLE ),
	DEFINE_ARRAY( CBaseMonster, m_hOldEnemy, FIELD_EHANDLE, MAX_OLD_ENEMIES ),
	DEFINE_ARRAY( CBaseMonster, m_vecOldEnemy, FIELD_POSITION_VECTOR, MAX_OLD_ENEMIES ),
	DEFINE_FIELD( CBaseMonster, m_flFieldOfView, FIELD_FLOAT ),
	DEFINE_FIELD( CBaseMonster, m_flWaitFinished, FIELD_TIME ),
	DEFINE_FIELD( CBaseMonster, m_flMoveWaitFinished, FIELD_TIME ),

	DEFINE_FIELD( CBaseMonster, m_Activity, FIELD_INTEGER ),
	DEFINE_FIELD( CBaseMonster, m_IdealActivity, FIELD_INTEGER ),
	DEFINE_FIELD( CBaseMonster, m_LastHitGroup, FIELD_INTEGER ),
	DEFINE_FIELD( CBaseMonster, m_MonsterState, FIELD_INTEGER ),
	DEFINE_FIELD( CBaseMonster, m_IdealMonsterState, FIELD_INTEGER ),
	DEFINE_FIELD( CBaseMonster, m_iTaskStatus, FIELD_INTEGER ),

	//Schedule_t			*m_pSchedule;

	DEFINE_FIELD( CBaseMonster, m_iScheduleIndex, FIELD_INTEGER ),
	DEFINE_FIELD( CBaseMonster, m_afConditions, FIELD_INTEGER ),
	//WayPoint_t			m_Route[ ROUTE_SIZE ];
	//DEFINE_FIELD( CBaseMonster, m_movementGoal, FIELD_INTEGER ),
	//DEFINE_FIELD( CBaseMonster, m_iRouteIndex, FIELD_INTEGER ),
	//DEFINE_FIELD( CBaseMonster, m_moveWaitTime, FIELD_FLOAT ),

	DEFINE_FIELD( CBaseMonster, m_flVelocityModifier, FIELD_FLOAT ),

	DEFINE_FIELD( CBaseMonster, m_vecMoveGoal, FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( CBaseMonster, m_movementActivity, FIELD_INTEGER ),

	DEFINE_FIELD( CBaseMonster, m_headdef, FIELD_INTEGER ),
	DEFINE_FIELD( CBaseMonster, m_EyeMod, FIELD_INTEGER ),
	DEFINE_FIELD( CBaseMonster, m_can_kill_script, FIELD_INTEGER ),

	//int					m_iAudibleList; // first index of a linked list of sounds that the monster can hear.
	//DEFINE_FIELD( CBaseMonster, m_afSoundTypes, FIELD_INTEGER ),
	DEFINE_FIELD( CBaseMonster, m_vecLastPosition, FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( CBaseMonster, m_iHintNode, FIELD_INTEGER ),
	DEFINE_FIELD( CBaseMonster, m_afMemory, FIELD_INTEGER ),
	DEFINE_FIELD( CBaseMonster, m_iMaxHealth, FIELD_INTEGER ),

	DEFINE_FIELD( CBaseMonster, m_vecEnemyLKP, FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( CBaseMonster, m_cAmmoLoaded, FIELD_INTEGER ),
	DEFINE_FIELD( CBaseMonster, m_afCapability, FIELD_INTEGER ),
	DEFINE_FIELD( CBaseMonster, m_cClipSize, FIELD_INTEGER ),

	DEFINE_FIELD( CBaseMonster, m_flNextAttack, FIELD_TIME ),
	DEFINE_FIELD( CBaseMonster, m_bitsDamageType, FIELD_INTEGER ),
	DEFINE_ARRAY( CBaseMonster, m_rgbTimeBasedDamage, FIELD_CHARACTER, CDMG_TIMEBASED ),
	DEFINE_FIELD( CBaseMonster, m_bloodColor, FIELD_INTEGER ),
	DEFINE_FIELD( CBaseMonster, m_failSchedule, FIELD_INTEGER ),
	DEFINE_FIELD( CBaseMonster, m_aimflag_dist, FIELD_FLOAT ),

	DEFINE_FIELD( CBaseMonster, m_flHungryTime, FIELD_TIME ),
	DEFINE_FIELD( CBaseMonster, m_flDistTooFar, FIELD_FLOAT ),
	DEFINE_FIELD( CBaseMonster, m_flDistLook, FIELD_FLOAT ),
	DEFINE_FIELD( CBaseMonster, m_iTriggerCondition, FIELD_INTEGER ),
	DEFINE_FIELD( CBaseMonster, m_iszTriggerTarget, FIELD_STRING ),

	DEFINE_FIELD( CBaseMonster, m_HackedGunPos, FIELD_VECTOR ),

	DEFINE_FIELD( CBaseMonster, m_scriptState, FIELD_INTEGER ),
	DEFINE_FIELD( CBaseMonster, m_pCine, FIELD_CLASSPTR ),

	DEFINE_FIELD( CBaseMonster, m_hHitEnemy, FIELD_EHANDLE ),

	DEFINE_FIELD( CBaseMonster, m_vecOldMovePoint, FIELD_VECTOR ),
	DEFINE_FIELD( CBaseMonster, m_MovePointMax, FIELD_INTEGER ),

	DEFINE_FIELD( CBaseMonster, m_attack_dist, FIELD_FLOAT ),
	DEFINE_FIELD( CBaseMonster, m_cover_dist, FIELD_INTEGER ),
	DEFINE_FIELD( CBaseMonster, m_thinkspeed, FIELD_INTEGER ),

	DEFINE_FIELD( CBaseMonster, m_running_rangeattack, FIELD_INTEGER ),

	DEFINE_FIELD( CBaseMonster, m_elseuseful, FIELD_INTEGER ),
	DEFINE_FIELD( CBaseMonster, m_killedbydmg, FIELD_INTEGER ),

	DEFINE_FIELD( CBaseMonster, m_ctmod, FIELD_INTEGER ),
	DEFINE_FIELD( CBaseMonster, m_lookignoremod, FIELD_INTEGER ),
	DEFINE_FIELD( CBaseMonster, m_crouchmode, FIELD_INTEGER ),

	DEFINE_FIELD( CBaseMonster, m_candrownwater, FIELD_INTEGER ),

	DEFINE_FIELD( CBaseMonster, m_killbyheadcrab, FIELD_INTEGER ),
	DEFINE_FIELD( CBaseMonster, m_canheadcrab_mode, FIELD_INTEGER ),
	DEFINE_FIELD( CBaseMonster, m_canbarnacle_mode, FIELD_INTEGER ),
	DEFINE_FIELD( CBaseMonster, m_crabzombie_begain, FIELD_INTEGER ),

	DEFINE_FIELD( CBaseMonster, m_grenadekilled, FIELD_INTEGER ),
	DEFINE_FIELD( CBaseMonster, m_MSblastvec, FIELD_POSITION_VECTOR ),

	DEFINE_FIELD( CBaseMonster, m_no_cover_mode, FIELD_INTEGER ),
	DEFINE_FIELD( CBaseMonster, m_lovehate, FIELD_INTEGER ),
	DEFINE_FIELD( CBaseMonster, m_die, FIELD_INTEGER ),
	DEFINE_FIELD( CBaseMonster, m_dieseq, FIELD_INTEGER ),
	DEFINE_FIELD( CBaseMonster, m_diefadeout, FIELD_INTEGER ),

	DEFINE_FIELD( CBaseMonster, m_killed_exp, FIELD_INTEGER ),

	DEFINE_FIELD( CBaseMonster, m_barnacleres_time, FIELD_INTEGER ),

	DEFINE_FIELD( CBaseMonster, m_alert, FIELD_INTEGER ),
	DEFINE_FIELD( CBaseMonster, m_notarget_hide, FIELD_INTEGER ),

	DEFINE_FIELD( CBaseMonster, m_flGodTime, FIELD_TIME ),

	DEFINE_FIELD( CBaseMonster, m_zombiehead_health, FIELD_FLOAT ),
	DEFINE_FIELD( CBaseMonster, m_no_victdance, FIELD_INTEGER ),
	DEFINE_FIELD( CBaseMonster, m_no_pov_limit, FIELD_INTEGER ),
	DEFINE_FIELD( CBaseMonster, m_die_dont_alert, FIELD_INTEGER ),
	DEFINE_FIELD( CBaseMonster, m_die_dont_move, FIELD_INTEGER ),
	DEFINE_FIELD( CBaseMonster, m_die_corpse_solid, FIELD_INTEGER ),
	DEFINE_FIELD( CBaseMonster, m_die_for_back, FIELD_INTEGER ),
	
	DEFINE_FIELD( CBaseMonster, m_flDeadTime, FIELD_TIME ),

	DEFINE_FIELD( CBaseMonster, m_enemyfollower, FIELD_INTEGER ),
	DEFINE_FIELD( CBaseMonster, m_enemyfollower_walk, FIELD_INTEGER ),
	DEFINE_FIELD( CBaseMonster, m_enemyfollower_combat, FIELD_INTEGER ),

	DEFINE_FIELD( CBaseMonster, m_duckseq, FIELD_INTEGER ),

	DEFINE_FIELD( CBaseMonster, m_chase_mode, FIELD_INTEGER ),
	DEFINE_FIELD( CBaseMonster, m_chase_failed_max, FIELD_INTEGER ),
	DEFINE_FIELD( CBaseMonster, m_makerspawn_call, FIELD_INTEGER ),

	DEFINE_FIELD( CBaseMonster, m_MoveFail_FuckRoad_Mode, FIELD_INTEGER ),

	DEFINE_FIELD( CBaseMonster, m_MoveFail_SimpleRoad, FIELD_BOOLEAN ),

	DEFINE_FIELD( CBaseMonster, m_rpgms_actor, FIELD_INTEGER ),
	DEFINE_FIELD( CBaseMonster, m_rpgms_level, FIELD_INTEGER ),
	DEFINE_FIELD( CBaseMonster, m_rpgms_exp, FIELD_INTEGER ),
	DEFINE_FIELD( CBaseMonster, m_rpgms_type, FIELD_INTEGER ),
	DEFINE_FIELD( CBaseMonster, m_rpgms_maxexp, FIELD_INTEGER ),
	DEFINE_FIELD( CBaseMonster, m_rpgms_inteam, FIELD_INTEGER ),
	DEFINE_FIELD( CBaseMonster, m_rpgms_skill1_learn, FIELD_INTEGER ),
	DEFINE_FIELD( CBaseMonster, m_rpgms_skill2_learn, FIELD_INTEGER ),
	DEFINE_FIELD( CBaseMonster, m_rpgms_skill3_learn, FIELD_INTEGER ),
	DEFINE_FIELD( CBaseMonster, m_rpgms_skill4_learn, FIELD_INTEGER ),
	DEFINE_FIELD( CBaseMonster, m_rpgms_skill5_learn, FIELD_INTEGER ),
	DEFINE_FIELD( CBaseMonster, m_rpgms_skill6_learn, FIELD_INTEGER ),
	DEFINE_FIELD( CBaseMonster, m_rpgms_skill7_learn, FIELD_INTEGER ),
	DEFINE_FIELD( CBaseMonster, m_rpgms_skill8_learn, FIELD_INTEGER ),
	DEFINE_FIELD( CBaseMonster, m_rpgms_skill9_learn, FIELD_INTEGER ),
	DEFINE_FIELD( CBaseMonster, m_rpgms_skill10_learn, FIELD_INTEGER ),
	DEFINE_FIELD( CBaseMonster, m_rpgms_skill11_learn, FIELD_INTEGER ),
	DEFINE_FIELD( CBaseMonster, m_rpgms_skill12_learn, FIELD_INTEGER ),

	DEFINE_FIELD( CBaseMonster, m_flPlayerTeamMateDamage_exp1, FIELD_FLOAT ),
	DEFINE_FIELD( CBaseMonster, m_flPlayerTeamMateDamage_exp2, FIELD_FLOAT ),
	DEFINE_FIELD( CBaseMonster, m_flPlayerTeamMateDamage_exp3, FIELD_FLOAT ),
	DEFINE_FIELD( CBaseMonster, m_flPlayerTeamMateDamage_exp4, FIELD_FLOAT ),

	DEFINE_FIELD( CBaseMonster, m_HenemyEnemyMe, FIELD_INTEGER ),
	DEFINE_FIELD( CBaseMonster, m_HenemyEnemyMe2, FIELD_INTEGER ),
	DEFINE_FIELD( CBaseMonster, m_hPlayer, FIELD_EHANDLE ),
	DEFINE_FIELD( CBaseMonster, m_hPortecter, FIELD_EHANDLE ),
	DEFINE_FIELD( CBaseMonster, m_hTeamMate1, FIELD_EHANDLE ),
	DEFINE_FIELD( CBaseMonster, m_hTeamMate2, FIELD_EHANDLE ),
	DEFINE_FIELD( CBaseMonster, m_hTeamMate3, FIELD_EHANDLE ),
	DEFINE_FIELD( CBaseMonster, m_hTeamMate4, FIELD_EHANDLE ),

	DEFINE_FIELD( CBaseMonster, m_MoveFail_FuckRoad, FIELD_BOOLEAN ),
	DEFINE_FIELD( CBaseMonster, m_forcefuckdoor, FIELD_BOOLEAN ),
	DEFINE_FIELD( CBaseMonster, m_selfmode, FIELD_BOOLEAN ),
	DEFINE_FIELD( CBaseMonster, m_godmode, FIELD_BOOLEAN ),
	DEFINE_FIELD( CBaseMonster, m_walkaround, FIELD_BOOLEAN ),
	DEFINE_FIELD( CBaseMonster, m_walkaroundFail, FIELD_BOOLEAN ),
	
	DEFINE_FIELD( CBaseMonster, m_is_the_boss, FIELD_BOOLEAN ),

	DEFINE_FIELD( CBaseMonster, m_new_ally_type, FIELD_BOOLEAN ),

	DEFINE_FIELD( CBaseMonster, m_enemyget_mode, FIELD_INTEGER ),

	DEFINE_FIELD( CBaseMonster, m_chaofhate, FIELD_INTEGER ),

	DEFINE_FIELD( CBaseMonster, m_undropgun, FIELD_BOOLEAN ),
	
	DEFINE_FIELD( CBaseMonster, m_cover_fromplayer, FIELD_BOOLEAN ),

	DEFINE_FIELD( CBaseMonster, m_noidleseq, FIELD_BOOLEAN ),

	DEFINE_FIELD( CBaseMonster, m_alwaysrunpath, FIELD_BOOLEAN ),

	DEFINE_FIELD( CBaseMonster, m_user_aimflag, FIELD_BOOLEAN ),

	DEFINE_FIELD( CBaseMonster, m_guard_mode, FIELD_BOOLEAN ),
	DEFINE_FIELD( CBaseMonster, m_playerguardian_mode, FIELD_INTEGER ),
	DEFINE_FIELD( CBaseMonster, m_follow_mode, FIELD_INTEGER ),

	DEFINE_FIELD( CBaseMonster, m_groundElev, FIELD_BOOLEAN ),
	DEFINE_FIELD( CBaseMonster, m_groundElev2, FIELD_BOOLEAN ),

	DEFINE_FIELD( CBaseMonster, m_victoryeat, FIELD_BOOLEAN ),

	DEFINE_FIELD( CBaseMonster, m_pathtrack_reuse, FIELD_INTEGER ),

	DEFINE_FIELD( CBaseMonster, m_fightmode, FIELD_INTEGER ),
	DEFINE_FIELD( CBaseMonster, m_nevergibmode, FIELD_BOOLEAN ),
	
	DEFINE_FIELD( CBaseMonster, m_flNPC_Pain, FIELD_FLOAT ),

	DEFINE_FIELD( CBaseMonster, m_flPlayerDamage_exp, FIELD_FLOAT ),
	DEFINE_FIELD( CBaseMonster, m_flPlayerDamage_hate, FIELD_FLOAT ),

	DEFINE_FIELD( CBaseMonster, m_MoveFailNum, FIELD_INTEGER ),
	DEFINE_FIELD( CBaseMonster, m_ignoreFail, FIELD_INTEGER ),
	DEFINE_FIELD( CBaseMonster, m_ignorePlayer, FIELD_INTEGER ),
	DEFINE_FIELD( CBaseMonster, m_ignoreFail_MAX, FIELD_INTEGER ),
	DEFINE_FIELD( CBaseMonster, m_ignoredamage, FIELD_INTEGER ),
	DEFINE_FIELD( CBaseMonster, m_boltpoison, FIELD_INTEGER ),
	DEFINE_FIELD( CBaseMonster, m_freezetime, FIELD_INTEGER ),
	DEFINE_FIELD( CBaseMonster, m_freeze_def, FIELD_INTEGER ),
	DEFINE_FIELD( CBaseMonster, m_listenlong, FIELD_INTEGER ),
	DEFINE_FIELD( CBaseMonster, m_longming, FIELD_INTEGER ),
	DEFINE_FIELD( CBaseMonster, m_facing_fucking_mode, FIELD_INTEGER ),
	DEFINE_FIELD( CBaseMonster, m_FTSmod, FIELD_INTEGER ),
	DEFINE_FIELD( CBaseMonster, m_aimenemy_mod, FIELD_INTEGER ),
	DEFINE_FIELD( CBaseMonster, m_vecOldLKP, FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( CBaseMonster, m_vecAllyLKP, FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( CBaseMonster, m_singdelay_max, FIELD_INTEGER ),
	DEFINE_FIELD( CBaseMonster, m_singdelay_use, FIELD_INTEGER ),
	DEFINE_FIELD( CBaseMonster, m_MoveStuckCheck, FIELD_INTEGER ),
};

//IMPLEMENT_SAVERESTORE( CBaseMonster, CBaseToggle )

int CBaseMonster::Save( CSave &save )
{
	if( !CBaseToggle::Save( save ) )
		return 0;
	return save.WriteFields( "CBaseMonster", this, m_SaveData, ARRAYSIZE( m_SaveData ) );
}

int CBaseMonster::Restore( CRestore &restore )
{
	if( !CBaseToggle::Restore( restore ) )
		return 0;
	int status = restore.ReadFields( "CBaseMonster", this, m_SaveData, ARRAYSIZE( m_SaveData ) );

	// We don't save/restore routes yet
	RouteClear();

	// We don't save/restore schedules yet
	m_pSchedule = NULL;
	m_iTaskStatus = TASKSTATUS_NEW;

	// Reset animation
	m_Activity = ACT_RESET;

	// If we don't have an enemy, clear conditions like see enemy, etc.
	if( m_hEnemy == 0 )
		m_afConditions = 0;

	return status;
}

//=========================================================
// Eat - makes a monster full for a little while.
//=========================================================
void CBaseMonster::Eat( float flFullDuration )
{
	m_flHungryTime = gpGlobals->time + flFullDuration;
}

//=========================================================
// FShouldEat - returns true if a monster is hungry.
//=========================================================
BOOL CBaseMonster::FShouldEat( void )
{
	if( m_flHungryTime > gpGlobals->time )
	{
		return FALSE;
	}

	return TRUE;
}

//=========================================================
// BarnacleVictimBitten - called
// by Barnacle victims when the barnacle pulls their head
// into its mouth
//=========================================================
void CBaseMonster::BarnacleVictimBitten( entvars_t *pevBarnacle )
{
	Schedule_t	*pNewSchedule;

	pNewSchedule = GetScheduleOfType( SCHED_BARNACLE_VICTIM_CHOMP );

	if( pNewSchedule )
	{
		ChangeSchedule( pNewSchedule );
	}
}

//=========================================================
// BarnacleVictimReleased - called by barnacle victims when
// the host barnacle is killed.
//=========================================================
void CBaseMonster::BarnacleVictimReleased( void )
{
	if(pev->health <= 0)
	{
		if(pev->deadflag == DEAD_NO)
		{
			pev->deadflag = DEAD_DYING;
		}
		m_die = 2;
		m_IdealMonsterState = MONSTERSTATE_DEAD;
		pev->movetype = MOVETYPE_TOSS;
		SetActivity( ACT_DIESIMPLE );
	}
	else
	{
		m_IdealMonsterState = MONSTERSTATE_IDLE;
		pev->velocity = Vector(0,0,-1);
		pev->movetype = MOVETYPE_STEP;
		m_barnacleres_time = 30;
	}
}

//=========================================================
// Listen - monsters dig through the active sound list for
// any sounds that may interest them. (smells, too!)
//=========================================================
void CBaseMonster::Listen( void )
{
	int	iSound;
	int	iMySounds;
	float	hearingSensitivity;
	CSound	*pCurrentSound;

	m_iAudibleList = SOUNDLIST_EMPTY; 
	ClearConditions( bits_COND_HEAR_SOUND | bits_COND_SMELL | bits_COND_SMELL_FOOD );
	m_afSoundTypes = 0;

	if ( (pev->spawnflags & SF_MONSTER_WAIT_FOR_SCRIPT) )
		return;
	
	if(m_listenlong > 0 || m_boltpoison > 0 || m_iTriggerCondition == 123 || m_longming >= 1 || m_crouchmode == 1 || m_freezetime > 0)
		return;
	
	if(m_iTriggerCondition == 1919)
	{
		m_alert = 100;
		return;
	}
	
	if ( pev->spawnflags & SF_MONSTER_PRISONER || m_fightmode || m_godmode)
		return;

	if (m_pCine)
		return;

	if(m_movementGoal == MOVEGOAL_PATHCORNER)
		return;

	if ( m_MonsterState == MONSTERSTATE_PRONE || m_IdealMonsterState == MONSTERSTATE_PRONE )
		return;

	iMySounds = ISoundMask();

	if( m_pSchedule )
	{
		//!!!WATCH THIS SPOT IF YOU ARE HAVING SOUND RELATED BUGS!
		// Make sure your schedule AND personal sound masks agree!
		iMySounds &= m_pSchedule->iSoundMask;
	}

	iSound = CSoundEnt::ActiveList();

	// UNDONE: Clear these here?
	ClearConditions( bits_COND_HEAR_SOUND | bits_COND_SMELL_FOOD | bits_COND_SMELL );
	hearingSensitivity = HearingSensitivity();

	while( iSound != SOUNDLIST_EMPTY )
	{
		pCurrentSound = CSoundEnt::SoundPointerForIndex( iSound );

		if( pCurrentSound &&
			( pCurrentSound->m_iType & iMySounds )	&&
			( pCurrentSound->m_vecOrigin - EarPosition() ).Length() <= pCurrentSound->m_iVolume * hearingSensitivity )

		//if( ( g_pSoundEnt->m_SoundPool[iSound].m_iType & iMySounds ) && ( g_pSoundEnt->m_SoundPool[iSound].m_vecOrigin - EarPosition()).Length () <= g_pSoundEnt->m_SoundPool[iSound].m_iVolume * hearingSensitivity )
		{
 			// the monster cares about this sound, and it's close enough to hear.
			//g_pSoundEnt->m_SoundPool[iSound].m_iNextAudible = m_iAudibleList;
			pCurrentSound->m_iNextAudible = m_iAudibleList;

			if( pCurrentSound->FIsSound() )
			{
				// this is an audible sound.
				SetConditions( bits_COND_HEAR_SOUND );
			}
			else
			{
				// if not a sound, must be a smell - determine if it's just a scent, or if it's a food scent
				//if( g_pSoundEnt->m_SoundPool[iSound].m_iType & ( bits_SOUND_MEAT | bits_SOUND_CARCASS ) )
				if( pCurrentSound->m_iType & ( bits_SOUND_MEAT | bits_SOUND_CARCASS ) )
				{
					// the detected scent is a food item, so set both conditions.
					// !!!BUGBUG - maybe a virtual function to determine whether or not the scent is food?
					SetConditions( bits_COND_SMELL_FOOD );
					SetConditions( bits_COND_SMELL );
				}
				else
				{
					// just a normal scent. 
					SetConditions( bits_COND_SMELL );
				}
			}
			//m_afSoundTypes |= g_pSoundEnt->m_SoundPool[iSound].m_iType;
			m_afSoundTypes |= pCurrentSound->m_iType;

			m_iAudibleList = iSound;
		}

		//iSound = g_pSoundEnt->m_SoundPool[iSound].m_iNext;
		if( pCurrentSound )
			iSound = pCurrentSound->m_iNext;
	}
}

//=========================================================
// FLSoundVolume - subtracts the volume of the given sound
// from the distance the sound source is from the caller, 
// and returns that value, which is considered to be the 'local' 
// volume of the sound. 
//=========================================================
float CBaseMonster::FLSoundVolume( CSound *pSound )
{
	return ( pSound->m_iVolume - ( ( pSound->m_vecOrigin - pev->origin ).Length() ) );
}

//=========================================================
// FValidateHintType - tells use whether or not the monster cares
// about the type of Hint Node given
//=========================================================
BOOL CBaseMonster::FValidateHintType( short sHint )
{
	return FALSE;
}

//=========================================================
// Look - Base class monster function to find enemies or 
// food by sight. iDistance is distance ( in units ) that the 
// monster can see.
//
// Sets the sight bits of the m_afConditions mask to indicate
// which types of entities were sighted.
// Function also sets the Looker's m_pLink 
// to the head of a link list that contains all visible ents.
// (linked via each ent's m_pLink field)
//
//=========================================================
void CBaseMonster::Look( int iDistance )
{
	if(m_cleardally >0)
	{
		m_cleardally -= 1;
	}

	if(m_barnacleres_time > 0)
	{
		m_barnacleres_time -= 1;
	}
	
	if(m_cleardally_enemy >0)
	{
		m_cleardally_enemy -= 1;
	}
	if(m_cleardally_enemy_long >0)
	{
		m_cleardally_enemy_long -= 1;
		if(m_cleardally_enemy_long == 0 && m_pathtrack_reuse == 1)
		{
			Use_PathWalk();
		}
	}
	if(m_fucked_run > 0)
	{
		m_fucked_run -= 1;
	}
	if(m_igonre_npc > 0)
	{
		m_igonre_npc -= 1;
	}

	int	iSighted = 0;
	if(m_alert > 0)
	{
		m_alert--;
	}
	if(m_finish > 0)
	{
		m_finish--;
	}
	if((m_alert > 0 || m_cleardally_enemy >0) && m_thinkspeed == -1)
	{
		m_thinkspeed = 0;
	}

	if(m_iTriggerCondition == 7456)
	{
		if(m_hEnemy != NULL)
		{
			if(m_hEnemy->IsPlayer())
			{
				CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "cshl623_prevent" );
				if ( pEntity )
				{
					if(pEntity->pev->armortype == 0 && pEntity->pev->armorvalue == 0)
					{
						pEntity->pev->armorvalue = 1;
					}
				}
			}
		}
	}

	// DON'T let visibility information from last frame sit around!
	ClearConditions( bits_COND_SEE_HATE | bits_COND_SEE_DISLIKE | bits_COND_SEE_ENEMY | bits_COND_SEE_FEAR | bits_COND_SEE_NEMESIS | bits_COND_SEE_CLIENT );

	m_pLink = NULL;

	if(m_boltpoison > 0 || m_iTriggerCondition == 123 || m_freezetime > 0)
		return;

	if ( m_MonsterState == MONSTERSTATE_PRONE || m_IdealMonsterState == MONSTERSTATE_PRONE )
		return;

	CBaseEntity *pSightEnt = NULL;// the current visible entity that we're dealing with

	int once_use = 1;

	// See no evil if prisoner is set
	if( !FBitSet( pev->spawnflags, SF_MONSTER_PRISONER ) )
	{
		CBaseEntity *pList[100];

		Vector delta = Vector( iDistance, iDistance, iDistance );

		// Find only monsters/clients in box, NOT limited to PVS
		int count = UTIL_EntitiesInBox( pList, 100, pev->origin - delta, pev->origin + delta, FL_CLIENT | FL_MONSTER );
		for( int i = 0; i < count; i++ )
		{
			pSightEnt = pList[i];
			// !!!temporarily only considering other monsters and clients, don't see prisoners
			if( pSightEnt != this && !FBitSet( pSightEnt->pev->spawnflags, SF_MONSTER_PRISONER ))
			{
				// the looker will want to consider this entity
				// don't check anything else about an entity that can't be seen, or an entity that you don't care about.
				if( IRelationship( pSightEnt ) != R_NO && !FBitSet( pSightEnt->pev->flags, FL_NOTARGET ) && FVisible( pSightEnt )
				&& !FBitSet( pSightEnt->pev->flags, FL_FROZEN ) && pSightEnt->pev->deadflag == DEAD_NO)
				{
					if(m_igonre_npc > 0 && FBitSet( pSightEnt->pev->flags, FL_MONSTER ) )
					continue;

					if(FClassnameIs(pSightEnt->pev,"npc_aim_flag") && m_aimflag_dist > 0)
					{
						if(m_user_aimflag || ( pSightEnt->pev->origin - pev->origin).Length() < m_aimflag_dist)
						{
							if(m_elseuseful >= 1 && m_elseuseful <= 3 && m_cAmmoLoaded > 0)
							{
								if(pSightEnt->pev->armorvalue < 100)
								{
									if(m_elseuseful < 3)
									{
										ClearSchedule();
										m_cAmmoLoaded = -114;
										m_elseuseful = 3;
										m_thinkspeed = 2;
									}
									else
									{
										pSightEnt->pev->armorvalue += 1;
										UTIL_Sparks( pSightEnt->pev->origin + Vector(RANDOM_LONG(-16,16),RANDOM_LONG(-16,16),RANDOM_LONG(8,24)));
									}
									char text[256];
									sprintf( text, "Bomb Defusing:%1.0f%\n",pSightEnt->pev->armorvalue);
									UTIL_CenterPrintAll( text );
								}
									
								if(pSightEnt->pev->armorvalue >= 100 && pSightEnt->pev->armorvalue < 200)
								{
									pSightEnt->pev->armorvalue = 300;
									FireTargets( "nuke_defuse_end", this, this, USE_TOGGLE, 0 );
									pSightEnt->SUB_StartFadeOut();
								}
								continue;
							}
						}
					}

					if(FClassnameIs(pSightEnt->pev,"player_aim_flag"))
					{
						if(( pSightEnt->pev->origin - pev->origin).Length() < 96)
							continue;
	
					}

					if(!FVisible( pSightEnt ))
					{
							
						if(m_enemyfollower == 1 && m_lovehate > 0 && m_hPlayer != NULL)
						{
							if ( pSightEnt->IsPlayer() )
							{
							}
							else if(pSightEnt->Classify() != CLASS_PLAYER_ALLY 
							&& pSightEnt->Classify() != CLASS_NONE
							&& FVisible(m_hPlayer)
							&& pSightEnt->FVisible(m_hPlayer))
							{	
							}
							else
							{
								continue;
							}
						}
						else if(m_FTSmod <= 0)
						{
							continue;
						}
						else if(m_FTSmod == 1)
						{
							if(pSightEnt->IsPlayer())
							{
							}
							else if(m_enemyfollower == 0)
							{
								CBaseMonster *pEnemyMonster = pSightEnt->MyMonsterPointer();
								if ( pEnemyMonster )
								{
									if ( pEnemyMonster->Classify() == CLASS_PLAYER_ALLY || pEnemyMonster->Classify() == CLASS_HUMAN_MILITARY)
									{
									}
									else
									{
										continue;
									}
								}
								else
								{
									continue;
								}
							}
						}
						else if(m_FTSmod == 2)
						{
							if ( FClassnameIs(pev, "monster_gargantua" ) )
							{
								if(!pSightEnt->IsPlayer())
								{
									continue;
								}
								else if(FNullEnt( FIND_CLIENT_IN_PVS( edict() ) ))
								{
									continue;
								}
							}
							else if ( FClassnameIs(pev, "monster_human_grunt" ) )
							{
								if(!FClassnameIs(pSightEnt->pev,"monster_gargantua"))
								{
									continue;
								}
							}
							else
							{
								continue;
							}
						}
						else if(m_FTSmod == 3)
						{
							//float flDist = ( pSightEnt->pev->origin - pev->origin).Length();
							if(!FClassnameIs(pSightEnt->pev,"player"))
								continue;
							
							//if ( flDist > 4096 ){
							//continue;
							//}
						}
						else if(m_FTSmod == 4)
						{
							if(!FClassnameIs(pSightEnt->pev,"npc_aim_flag") && m_enemyfollower == 0)
							{
								continue;
							}
							else if(!pSightEnt->IsPlayer() && m_enemyfollower == 1)
							{
								continue;
							}
							if( !FInViewCone( pSightEnt ) )
							{
								if(m_alert == 0 && !pSightEnt->IsPlayer() && !FClassnameIs(pSightEnt->pev,"npc_aim_flag"))
									continue;
							}
						}
						else if(m_FTSmod == 5)
						{
							if(!pSightEnt->IsPlayer() && m_enemyfollower == 1)
							{
								continue;
							}
							if( !FInViewCone( pSightEnt ) )
							{
								if(m_alert == 0 && !pSightEnt->IsPlayer())
									continue;
							}
						}
						else if(m_FTSmod == 6)
						{
							float flDist = ( pSightEnt->pev->origin - pev->origin).Length();
							if(FClassnameIs(pSightEnt->pev,"player"))
								continue;
								
							if ( flDist > 4096 )
								continue;
								
						}
						else if(m_FTSmod == 7)
						{
							float flDist = ( pSightEnt->pev->origin - pev->origin).Length();
							if ( flDist > 2048 )
								continue;
								
						}
						else if(m_FTSmod == 8)
						{
							if(!FClassnameIs(pSightEnt->pev,"npc_aim_flag"))
								continue;
								
						}
					}

					if(m_FTSmod == 1)
					{
						if ( FClassnameIs( pSightEnt->pev, "monster_gargantua" )  )
						{
							float flDist = ( pSightEnt->pev->origin - pev->origin).Length();
							if ( flDist > 384 )
							{
								continue;
							}
							else
							{
								CBaseMonster *pEnemyMonster = pSightEnt->MyMonsterPointer();
								if ( pEnemyMonster )
								{
									if(pEnemyMonster->m_hEnemy != NULL)
									{
										if ( (pEnemyMonster->m_hEnemy)->Classify() == CLASS_PLAYER || (pEnemyMonster->m_hEnemy)->Classify() == Classify())
										{
											//Fuck You!
										}
										else
										{
											continue;
										}
									}
									else
									{
										continue;
									}
								}
							}
						}
						if ( FClassnameIs( pSightEnt->pev, "monster_human_grunt" ) )
						{
							CBaseMonster *pEnemyMonster = pSightEnt->MyMonsterPointer();
							if ( pEnemyMonster )
							{
								if(pEnemyMonster->m_hEnemy != NULL){
									if ( (pEnemyMonster->m_hEnemy)->Classify() == CLASS_PLAYER || (pEnemyMonster->m_hEnemy)->Classify() == Classify())
									{
										//Fuck You!
									}
									else
									{
										continue;
									}
								}
								else
								{
									continue;
								}
							}
						}
					}

					if(m_enemyfollower == 0 && m_FTSmod <= 1 && m_lovehate != 810)
					{
						if( !FInViewCone( pSightEnt ) && m_alert == 0)
							continue;
					}

					if(m_ctmod >= 1)
					{
						if ( m_ctmod == 1 && fabs(pSightEnt->pev->origin.z - pev->origin.z) > 128 )
						{
							continue;
						}
						else if ( m_ctmod == 2 && fabs(pSightEnt->pev->origin.z - pev->origin.z) > 384 )
						{
							continue;
						}
						else if ( m_ctmod == 3 && fabs(pSightEnt->pev->origin.z - pev->origin.z) >= 96 )
						{
							continue;
						}
					}

					if ( FClassnameIs( pSightEnt->pev, "npc_aim_flag" ) )
					{
						if(pSightEnt->pev->armortype == 1 && pSightEnt->pev->health > 0)
						{
							float flDist = ( pSightEnt->pev->origin - pev->origin).Length();
							if ( flDist < 200 )
							{
								m_cleardally = 0;
								m_FTSmod = 5;
								m_enemyfollower = 1;
								RouteClear();
								m_boltpoison = 10;
								m_hEnemy = NULL;
								m_hOldEnemy[0] = NULL;
								m_hOldEnemy[1] = NULL;
								m_hOldEnemy[2] = NULL;
								m_hOldEnemy[3] = NULL;
								pSightEnt->pev->health = 0;
							}
						}
					}

					CBaseMonster *pClient;
					pClient = pSightEnt->MyMonsterPointer();

					if(pClient->m_die >= 1)
						continue;

					if ( m_lookignoremod == 1 && pClient->m_MonsterState == MONSTERSTATE_PRONE )
						continue;
						
					if(IRelationship ( pSightEnt ) >= R_DL && m_singdelay_use > 0 && once_use == 1)
					{
						if(m_FTSmod == 0)
						{
							m_singdelay_use--;
							once_use = 0;
							continue;
						}
						else if( FVisible(pSightEnt) )
						{
							m_singdelay_use--;
							once_use = 0;
							continue;
						}
					}
					if( pSightEnt->IsPlayer() )
					{
						if(m_hPlayer == NULL)
						{
							m_hPlayer = pSightEnt;
						}
						if (m_playerguardian_mode == 0 && m_enemyfollower == 1 && m_enemyfollower_combat == 1 && m_lovehate > 0)
							continue;
						
						if(Classify() == CLASS_PLAYER_ALLY && m_hTargetEnt != NULL)
						{
							if(m_hTargetEnt == pSightEnt)
							{
								CBasePlayer *pPlayer = GetClassPtr((CBasePlayer *)m_hTargetEnt->pev);
								if(pPlayer->m_iWeaponFlash >= 128)
								{
									m_alert = 100;
								}
							}
						}
						if( pev->spawnflags & SF_MONSTER_WAIT_TILL_SEEN )
						{
							CBaseMonster *pClient = pSightEnt->MyMonsterPointer();

							// don't link this client in the list if the monster is wait till seen and the player isn't facing the monster
							if( pClient && !pClient->FInViewCone( this ) )
							{
								// we're not in the player's view cone. 
								continue;
							}
							else
							{
								// player sees us, become normal now.
								pev->spawnflags &= ~SF_MONSTER_WAIT_TILL_SEEN;
							}
						}

						// if we see a client, remember that (mostly for scripted AI)
						iSighted |= bits_COND_SEE_CLIENT;

						if ( pSightEnt && pClient->FInViewCone( this ) )
						{
							float flDist = ( pSightEnt->pev->origin - pev->origin).Length();
							if ( FClassnameIs(pev,"monster_saintna") || FClassnameIs(pev,"monster_misaliya") )
							{
								if ( flDist <= 200 )
								{
									m_cleardally += 3;
								}
							}
							else{
								if ( flDist <= 300 )
								{
									m_cleardally += 3;
								}
							}

							if(m_cleardally > 30)
								m_cleardally = 30;
						}
					}

					pSightEnt->m_pLink = m_pLink;
					m_pLink = pSightEnt;

					if( pSightEnt == m_hEnemy )
					{
						if(m_iTriggerCondition != 1919)
						{
							// we know this ent is visible, so if it also happens to be our enemy, store that now.
							iSighted |= bits_COND_SEE_ENEMY;
							m_hasenemy_nosee = 0;
						}
						else if(pev->health == pev->max_health)
						{
							iSighted |= bits_COND_SEE_ENEMY;
							m_hasenemy_nosee = 0;
						}
					}

					// don't add the Enemy's relationship to the conditions. We only want to worry about conditions when
					// we see monsters other than the Enemy.
					switch( IRelationship( pSightEnt ) )
					{
					case R_NM:
						iSighted |= bits_COND_SEE_NEMESIS;
						m_cleardally_enemy = 5;
						m_cleardally_enemy_long = 60;		
						break;
					case R_HT:
						iSighted |= bits_COND_SEE_HATE;
						m_cleardally_enemy = 5;
						m_cleardally_enemy_long = 60;		
						break;
					case R_DL:
						iSighted |= bits_COND_SEE_DISLIKE;
						m_cleardally_enemy = 5;
						m_cleardally_enemy_long = 60;
						break;
					case R_FR:
						iSighted |= bits_COND_SEE_FEAR;
						break;
					case R_AL:
						break;
					default:
						ALERT( at_aiconsole, "%s can't assess %s\n", STRING( pev->classname ), STRING( pSightEnt->pev->classname ) );
						break;
					}
				}
			}
		}
	}

	if(!FBitSet( pev->spawnflags, SF_MONSTER_PRISONER ))
		SetConditions( iSighted );

	if(m_singdelay_use < m_singdelay_max && once_use == 1 && m_cleardally_enemy_long == 0)
		m_singdelay_use = m_singdelay_max;
}

void CBaseMonster::Freeze_Monster ( int time )
{
		if ( pev->deadflag == DEAD_NO && pev->takedamage)
		{

			if(m_MonsterState == MONSTERSTATE_SCRIPT || m_IdealMonsterState == MONSTERSTATE_SCRIPT)
				return;

			if(m_freeze_def == 3)
			{
				time -= 30;
			}
			else if(m_freeze_def == 2)
			{
				time -= 20;
			}
			else if(m_freeze_def == 1)
			{
				time -= 10;
			}

			if(time <= 5)
				return;

			if(m_freezetime <= 2)
			{
				m_hEnemy = NULL;
				ClearSchedule();
				RouteClear();
				m_flGroundSpeed = 0;
				pev->yaw_speed = 0;
				SetActivity( ACT_IDLE );
			}

			if(m_freezetime < time)
			{
				m_freezetime = time;
			}
		}
}

void CBaseMonster::Hunt_Stand_Set( int mode )
{
	if(mode == 0)
	{
		ClearSchedule();
		SetActivity( ACT_IDLE );
		SetState( MONSTERSTATE_HUNT );
		m_enemyfollower = 0;
		pev->movetype = MOVETYPE_STEP;
		pev->effects &= ~EF_NODRAW;
		pev->velocity = g_vecZero;
		pev->yaw_speed = 0;
		RouteClear();
		m_cleardally_enemy = 0;
		m_cleardally_enemy_long = 0;
		m_hEnemy = NULL;
		m_hOldEnemy[0] = NULL;
		m_hOldEnemy[1] = NULL;
		m_hOldEnemy[2] = NULL;
		m_hOldEnemy[3] = NULL;

		if(m_playerguardian_mode > 0 && m_hPlayer != NULL)
		{
			m_playerguardian_mode = 0;
			CBasePlayer *pPlayer = GetClassPtr((CBasePlayer *)m_hPlayer->pev);
			pPlayer->m_wdoor_mynpc = NULL;
			pPlayer->m_guard_mynpc = 0;
		}
		pev->owner   = NULL;
		pev->flags	|= FL_NOTARGET;
		m_godmode	 = TRUE;
	}
	else if(mode == 1)
	{
		ClearSchedule();
		SetState( MONSTERSTATE_HUNT );
		m_die = 0;
		m_dieseq	= 0;
		pev->deadflag = DEAD_NO;
		pev->movetype = MOVETYPE_STEP;
		pev->health = pev->max_health;
		SetActivity( ACT_IDLE );
		Forget( bits_MEMORY_KILLED );
		pev->skin = 0;
		pev->body = 0;
		UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);
		pev->flags		|= FL_NOTARGET;
		m_godmode		 = TRUE;
		RouteClear();
		m_enemyfollower = 0;
		m_cleardally_enemy = 0;
		m_cleardally_enemy_long = 0;
		m_hEnemy = NULL;
		m_hOldEnemy[0] = NULL;
		m_hOldEnemy[1] = NULL;
		m_hOldEnemy[2] = NULL;
		m_hOldEnemy[3] = NULL;
		pev->effects &= ~EF_NODRAW;
		m_killedbydmg = 0;
	}
	else if(mode == 2)
	{
		if(pev->deadflag == DEAD_NO)
		{
			SetActivity( ACT_IDLE );
			SetState( MONSTERSTATE_IDLE );
			if(m_new_ally_type){
				m_enemyfollower = 1;
			}
			else
			{
				m_hTargetEnt = m_hPlayer;
			}
			SetYawSpeed();
			ClearSchedule();
			pev->movetype = MOVETYPE_STEP;
			pev->flags &= ~FL_NOTARGET;
			m_godmode = FALSE;
			m_selfmode = FALSE;
		}
	}
	else if(mode == 3)
	{
		ClearSchedule();
		SetState( MONSTERSTATE_IDLE );
		m_die = 0;
		m_dieseq	= 0;
		pev->deadflag = DEAD_NO;
		pev->movetype = MOVETYPE_STEP;
		pev->health = pev->max_health;
		SetActivity( ACT_IDLE );
		Forget( bits_MEMORY_KILLED );
		pev->skin = 0;
		pev->body = 0;
		UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);
		pev->flags &= ~FL_NOTARGET;
		pev->effects &= ~EF_NODRAW;
		m_godmode = FALSE;
		m_killedbydmg = 0;
	}
}

void CBaseMonster::Alert_Ally( CBaseEntity *Attacker )
{
	if( m_MonsterState == MONSTERSTATE_SCRIPT)
		return;
	
	if ( m_MonsterState == MONSTERSTATE_PRONE || m_IdealMonsterState == MONSTERSTATE_PRONE )
		return;
	
	if(m_boltpoison > 0 || m_selfmode == TRUE)
		return;

	CBaseEntity *pEntity = NULL;
	Vector vecSpot;
	Vector vecSrc = BodyTarget_e( pev->origin );
	TraceResult	tr;
	while ((pEntity = UTIL_FindEntityInSphere( pEntity, vecSrc, 1536 )) != NULL)
	{
		CBaseMonster *pMonster = pEntity->MyMonsterPointer();
		if ( pMonster )
		{
			if(pMonster->Classify() == Classify() )
			{
				if ( pMonster->IsAlive() && pMonster->m_selfmode == FALSE && pMonster->m_igonre_npc == 0)
				{
					vecSpot = pEntity->BodyTarget_e( vecSrc );
					UTIL_TraceLine ( vecSrc, vecSpot, ignore_monsters, ENT(pev), &tr );
					if ( tr.flFraction == 1.0 || tr.pHit == pEntity->edict() )
					{
						alert_chese:
						if(Attacker != NULL)
						{
							if(Attacker->Classify() != pMonster->Classify() && Attacker->Classify() != Classify() )
							{
								if(Attacker->Classify() == CLASS_PLAYER && pMonster->Classify() == CLASS_PLAYER_ALLY || Attacker->Classify() == CLASS_PLAYER && pMonster->Classify() == CLASS_HUMAN_PASSIVE)
								{
								}
								else
								{
									pMonster->PushEnemy( Attacker, Attacker->pev->origin );
								}
							}
						}
						else if(pMonster != this && pMonster->m_hEnemy == NULL)
						{
							pMonster->m_vecAllyLKP = pev->origin;
							pMonster->m_vecOldLKP = pMonster->pev->origin;
							pMonster->m_allydeadcheck = 1;
							pMonster->m_alert = 100;
							if( pMonster->m_MonsterState == MONSTERSTATE_IDLE && pMonster->pev->health > 0)
							{
								if(pMonster->m_movementGoal == MOVEGOAL_PATHCORNER)
								{
									pMonster->ClearSchedule();
									pMonster->m_cleardally_enemy_long = 60;
								}
								pMonster->SetState( MONSTERSTATE_ALERT );
							}
						}
					}
					else
					{
						if( pMonster->IsAlive() && (pMonster->pev->origin - pev->origin ).Length() < 256)
							goto alert_chese;
					}
				}
			}
		}
	}
}

void CBaseMonster::Alert_Clients( CBaseEntity *Attacker )
{
	if( m_MonsterState == MONSTERSTATE_SCRIPT)
		return;
	
	if ( m_MonsterState == MONSTERSTATE_PRONE || m_IdealMonsterState == MONSTERSTATE_PRONE )
		return;
	
	if(m_boltpoison > 0 || m_selfmode == TRUE)
		return;
	
	CBaseEntity *pEntity = NULL;
	Vector vecSpot;
	Vector vecSrc = EyePosition();
	TraceResult	tr;
	while ((pEntity = UTIL_FindEntityInSphere( pEntity, vecSrc, 2048 )) != NULL)
	{
		CBaseMonster *pMonster = pEntity->MyMonsterPointer();
		if ( pMonster )
		{
			if( pMonster->Classify() == Classify() && pMonster->m_selfmode == FALSE && pMonster->m_rpgms_inteam == 0)
			{
				if ( pMonster->IsAlive() )
				{
					vecSpot = pEntity->EyePosition();
					UTIL_TraceLine ( vecSrc, vecSpot, ignore_monsters, ENT(pev), &tr );
					if ( tr.flFraction == 1.0 || tr.pHit == pEntity->edict() )
					{
						alert_chese:
						if(Attacker != NULL)
						{
							if(pMonster->m_lovehate == 810)
							{
								pMonster->m_lovehate = 0;
							}
							else
							{
								pMonster->m_lovehate -= 60;
							}
							
							if(pMonster->m_lovehate <= 0)
							{
								if(Attacker->Classify() != CLASS_PLAYER)
									pMonster->PushEnemy( Attacker, Attacker->pev->origin );
										
							}
						}
						else if(pMonster != this && pMonster->m_hEnemy == NULL)
						{
							pMonster->m_vecAllyLKP = pev->origin;
							pMonster->m_vecOldLKP = pMonster->pev->origin;
							pMonster->m_allydeadcheck = 1;
							pMonster->m_alert = 100;
							if( pMonster->m_MonsterState == MONSTERSTATE_IDLE && pMonster->pev->health > 0)
								pMonster->SetState( MONSTERSTATE_ALERT );
								
						}
					}
					else
					{
						if(m_die_dont_alert == 0)
						{
							if( pMonster->IsAlive() && (pMonster->pev->origin - pev->origin ).Length() < 256)
								goto alert_chese;
								
						}
					}
				}
			}
		}
	}

}

//=========================================================
// ISoundMask - returns a bit mask indicating which types
// of sounds this monster regards. In the base class implementation,
// monsters care about all sounds, but no scents.
//=========================================================
int CBaseMonster::ISoundMask( void )
{
	return	bits_SOUND_WORLD |
		bits_SOUND_COMBAT |
		bits_SOUND_PLAYER;
}

//=========================================================
// PBestSound - returns a pointer to the sound the monster 
// should react to. Right now responds only to nearest sound.
//=========================================================
CSound *CBaseMonster::PBestSound( void )
{
	if(m_listenlong > 0 || m_boltpoison > 0)
		return NULL;

	int iThisSound; 
	int iBestSound = -1;
	float flBestDist = 8192;// so first nearby sound will become best so far.
	float flDist;
	CSound *pSound;

	iThisSound = m_iAudibleList; 

	if( iThisSound == SOUNDLIST_EMPTY )
	{
		ALERT( at_aiconsole, "ERROR! monster %s has no audible sounds!\n", STRING( pev->classname ) );
#if _DEBUG
		ALERT( at_error, "NULL Return from PBestSound\n" );
#endif
		return NULL;
	}

	while( iThisSound != SOUNDLIST_EMPTY )
	{
		pSound = CSoundEnt::SoundPointerForIndex( iThisSound );

		if( pSound )
		{
			if( pSound->FIsSound() )
			{
				flDist = ( pSound->m_vecOrigin - EarPosition() ).Length();

				if( flDist < flBestDist )
				{
					iBestSound = iThisSound;
					flBestDist = flDist;
				}
			}

			iThisSound = pSound->m_iNextAudible;
		}
	}
	if( iBestSound >= 0 )
	{
		pSound = CSoundEnt::SoundPointerForIndex( iBestSound );
		return pSound;
	}
#if _DEBUG
	ALERT( at_error, "NULL Return from PBestSound\n" );
#endif
	return NULL;
}

//=========================================================
// PBestScent - returns a pointer to the scent the monster 
// should react to. Right now responds only to nearest scent
//=========================================================
CSound *CBaseMonster::PBestScent( void )
{
	int iThisScent; 
	int iBestScent = -1;
	float flBestDist = 8192;// so first nearby smell will become best so far.
	float flDist;
	CSound *pSound;

	iThisScent = m_iAudibleList;// smells are in the sound list.

	if( iThisScent == SOUNDLIST_EMPTY )
	{
		ALERT( at_aiconsole, "ERROR! PBestScent() has empty soundlist!\n" );
#if _DEBUG
		ALERT( at_error, "NULL Return from PBestSound\n" );
#endif
		return NULL;
	}

	while( iThisScent != SOUNDLIST_EMPTY )
	{
		pSound = CSoundEnt::SoundPointerForIndex( iThisScent );

		if( pSound->FIsScent() )
		{
			flDist = ( pSound->m_vecOrigin - pev->origin ).Length();

			if( flDist < flBestDist )
			{
				iBestScent = iThisScent;
				flBestDist = flDist;
			}
		}

		iThisScent = pSound->m_iNextAudible;
	}
	if( iBestScent >= 0 )
	{
		pSound = CSoundEnt::SoundPointerForIndex( iBestScent );

		return pSound;
	}
#if _DEBUG
	ALERT( at_error, "NULL Return from PBestScent\n" );
#endif
	return NULL;
}

//=========================================================
// Monster Think - calls out to core AI functions and handles this
// monster's specific animation events
//=========================================================
void CBaseMonster::MonsterThink( void )
{
	if(m_playerguardian_mode == 20)
	{
		pev->origin.z += 1;
		DROP_TO_FLOOR ( ENT(pev) );
		pev->movetype = MOVETYPE_STEP;
		pev->velocity = g_vecZero;

		if (!WALK_MOVE ( ENT(pev), 0, 0, WALKMOVE_NORMAL ) )
		{
			m_playerguardian_mode = 1;
			UTIL_SetOrigin( pev, pev->origin + Vector(0,0,16384) );
			pev->movetype = MOVETYPE_NONE;
			pev->effects |= EF_NODRAW;
		}
		else if(m_hPlayer != NULL)
		{
			m_playerguardian_mode = 0;
			pev->flags &= ~FL_NOTARGET;
			m_no_cover_mode = 0;
			pev->effects &= ~EF_NODRAW;
			CBasePlayer *pPlayer = GetClassPtr((CBasePlayer *)m_hPlayer->pev);
			pPlayer->m_wdoor_mynpc = NULL;
			pPlayer->m_guard_mynpc = 0;
			FX_Explosion( Center(), 102);
			pev->owner = NULL;
		}
	}
	else if(m_playerguardian_mode == 1)
	{
		if(m_hPlayer)
		{
			if(pev->deadflag != DEAD_NO)
			{
				pev->effects &= ~EF_NODRAW;
				m_no_cover_mode = 0;
				CBasePlayer *pPlayer = GetClassPtr((CBasePlayer *)m_hPlayer->pev);
				pPlayer->m_wdoor_mynpc = NULL;
				pPlayer->m_guard_mynpc = 0;
				m_playerguardian_mode = 0;
				pev->flags &= ~FL_NOTARGET;
				FX_Explosion( Center(), 102);
				pev->owner = NULL;
			}
			else
			{
				if (pev->angles.y != m_hPlayer->pev->angles.y)
					pev->angles.y = m_hPlayer->pev->angles.y;
						

				if ( IsMoving() )
				{
					RouteClear();
					m_flGroundSpeed = 0;
					pev->velocity = g_vecZero;
				}

				UTIL_MakeVectors( m_hPlayer->pev->angles );
				UTIL_SetOrigin( pev, m_hPlayer->pev->origin + gpGlobals->v_forward * -16);
			}
		}
		else if(m_rpgms_inteam == 0)
		{
			pev->effects &= ~EF_NODRAW;
			m_no_cover_mode = 0;
			m_playerguardian_mode = 0;
			pev->flags &= ~FL_NOTARGET;
			pev->owner = NULL;
		}
	}

	if(pev->deadflag == DEAD_DEAD)
	{

		if( !FNullEnt(FIND_CLIENT_IN_PVS(edict())) && m_diefadeout == 0)
		{
			if ( pev->waterlevel >= 1 && pev->movetype != MOVETYPE_FLY )
			{
				pev->gravity = 0.1;
				if ( pev->waterlevel >= 2 )
				{
					if (pev->velocity.Length() > 60){
						pev->velocity = pev->velocity * 0.8;
					}
					else if (pev->velocity.Length() < 30){
						if ( pev->flags & FL_ONGROUND ){
						pev->flags &= ~FL_ONGROUND;
						}
						pev->velocity.z += 1.0;
						pev->velocity = pev->velocity * 1.1;
					}
				}

				if (pev->watertype == CONTENT_LAVA)
				{
					TakeDamage(VARS(eoNullEntity), VARS(eoNullEntity), 20 * pev->waterlevel, DMG_BURN);
				}
				else if (pev->watertype == CONTENT_SLIME)
				{
					TakeDamage(VARS(eoNullEntity), VARS(eoNullEntity), 10 * pev->waterlevel, DMG_ACID);
				}

			}

			if (pev->velocity == g_vecZero)
			{
				m_droptofloor++;
				if(m_droptofloor >= 5)
				{
					pev->velocity.z -= 0.1;
					m_droptofloor = 0;
				}
			}
			else
			{
				m_droptofloor = 0;
			}
		}

		if(m_thinkspeed == 3)
		{
			pev->nextthink = gpGlobals->time + 0.02;
		}
		else if(m_thinkspeed == 2)
		{
			pev->nextthink = gpGlobals->time + 0.2;
		}
		else if(m_thinkspeed == -1)
		{
			pev->nextthink = gpGlobals->time + 0.3;
		}
		else
		{
			pev->nextthink = gpGlobals->time + 0.1;
		}

		if(m_die_for_back >= 1)
		{
			if(m_flDeadTime + 5.0 <= gpGlobals->time)
			{
				if(m_die_for_back == 1 && m_killed_exp > 1)
				{
					m_killed_exp = 1;
				}
				m_die_for_back--;
				ClearSchedule();
				SetState( MONSTERSTATE_IDLE );
				m_die = 0;
				m_dieseq	= 0;
				pev->deadflag = DEAD_NO;
				pev->movetype = MOVETYPE_STEP;
				pev->health = pev->max_health;
				SetActivity( ACT_IDLE );
				Forget( bits_MEMORY_KILLED );
				m_freezetime = 0;
				m_alert = 100;
				FX_Explosion( Center(), 102);
			}
		}

	}

	if(pev->deadflag == DEAD_NO)
	{

		if(m_trainstuck > 0)
		{
			pev->origin.z += 1;
			m_trainstuck--;
		}

		if (m_MoveStuckCheck == 1)
		{
			float dist;
			if ( IsMoving() )
			{
				dist = (m_vecOldMovePoint - pev->origin).Length();
				if(dist <= 4)
				{
					m_MovePointMax += 1;
					if(m_MovePointMax > 10)
					{
						m_hEnemy = NULL;
						ClearSchedule();
						m_MovePointMax = 0;
						pev->velocity.x += RANDOM_LONG(-250,250);
						pev->velocity.y += RANDOM_LONG(-250,250);
						pev->velocity.z += 50;
					}
				}
				else
				{
					m_MovePointMax = 0;
				}
					m_vecOldMovePoint = pev->origin;
			}
			else
			{	
			}
		}

		if (m_groundElev2 == TRUE)
		{
			TraceResult tr;
			UTIL_TraceLine ( Center(), pev->origin, dont_ignore_monsters, ENT(pev), &tr );
			if(tr.vecEndPos.z > pev->origin.z)
			{
				UTIL_SetOrigin ( pev, pev->origin + Vector(0,0,1) );
			}
		}

		if( !FNullEnt(FIND_CLIENT_IN_PVS(edict())) )
		{
			if(m_MonsterState != MONSTERSTATE_PRONE && m_IdealMonsterState != MONSTERSTATE_PRONE
			&& pev->movetype == MOVETYPE_STEP && !m_groundElev && m_freezetime == 0)
			{
				m_droptofloor++;
				if(m_droptofloor >= 4)
				{
					TraceResult tr;
					UTIL_TraceLine ( Center(), pev->origin - Vector(0,0,4), dont_ignore_monsters, ENT(pev), &tr );
					if(pev->velocity.Length() <= 10 && tr.flFraction == 1.0)
						pev->velocity.z -= 1;
					m_droptofloor = 0;
				}
			}
		}
		

		if(m_boltpoison > 0)
			m_boltpoison -= 1;
			
		if(m_freezetime > 0)
		{
				m_freezetime -= 1;
				if(m_freezetime <= 1)
				{
					ChangeSchedule( GetScheduleOfType( SCHED_ALERT_STAND ) );
					SetActivity( ACT_IDLE );
					SetYawSpeed();
				}
		}
		if(m_listenlong > 0)
			m_listenlong -= 1;
			
		if(m_ignorePlayer > 0)
			m_ignorePlayer -= 1;
			
		if ( m_candrownwater >= 1 && IsAlive() && pev->movetype != MOVETYPE_FLY)
		{
			if (pev->waterlevel >= 2)
			{
				if(pev->health > 0)
				{
					if(m_candrownwater == 1)
					{
						m_candrownwater = 2;
						pev->air_finished = gpGlobals->time + 10.0;
					}
					if (pev->air_finished < gpGlobals->time)
					{
						Killed( pev, GIB_NEVER );
						UTIL_Bubbles( pev->origin - Vector( 12, 12, -48 ), pev->origin + Vector( 12, 12, 96 ), 16 );
					}
				}
			}
			else if ( pev->waterlevel <= 1 && m_candrownwater >= 2 )
			{
				m_candrownwater = 1;
				pev->air_finished = 0;
			}
		}

		if ( pev->waterlevel >= 1)
		{
			if (pev->watertype == CONTENT_LAVA)
			{
				TakeDamage(VARS(eoNullEntity), VARS(eoNullEntity), 60 * pev->waterlevel, DMG_BURN);
			}
			else if (pev->watertype == CONTENT_SLIME)
			{
				TakeDamage(VARS(eoNullEntity), VARS(eoNullEntity), 30 * pev->waterlevel, DMG_ACID);
			}
		}

		if(pev->movetype == MOVETYPE_STEP)
		{
			if(m_ignoreFail == 1919)
			{
			}
			else
			{
				if(m_ignoreFail_MAX <= 0)
				{
					m_ignoreFail = -1;
				}
				else
				{
					if(m_ignoreFail > m_ignoreFail_MAX)
					{
						m_ignoreFail = -20;
					}
					else if(m_ignoreFail < 0 && m_ignoreFail > -100)
					{
						m_ignoreFail += 1;
					}
				}
			}
		}
		else
		{
			m_ignoreFail = -1;
		}

		if(m_ignoreFail > 0 && !IsMoving() )
		{
			if(m_ignoreFail == 1919)
			{
			}
			else
			{
				m_ignoreFail -= 1;
			}
		}	
	
		if(m_notarget_hide > 0)
		{
			pev->flags |= FL_FROZEN;
			if(m_notarget_hide < 1919)
			{
				m_notarget_hide--;
				if(m_notarget_hide <= 0)
					pev->flags &= ~FL_FROZEN;
				
			}
		}
		else if(m_notarget_hide <= 0 && FBitSet( pev->flags, FL_FROZEN ) ){
			pev->flags &= ~FL_FROZEN;
		}

		if(m_forcefuckdoor)
		{
			if(IsMoving() && m_OpenDoorWaitTime <= gpGlobals->time)
			{
				m_OpenDoorWaitTime = gpGlobals->time + 0.5;
				TraceResult tr;
				UTIL_MakeVectors(pev->angles);
				Vector vecSrc	= BodyTarget(pev->origin);
				Vector vecEnd	= vecSrc + gpGlobals->v_forward * (pev->maxs.x + 64);
				UTIL_TraceLine( vecSrc, vecEnd, dont_ignore_monsters, ENT( pev ), &tr );
				CBaseEntity *pEntity = CBaseEntity::Instance(tr.pHit);
				if(tr.flFraction < 1.0)
				{
					if ( FClassnameIs( pEntity->pev, "func_door" ) || FClassnameIs( pEntity->pev, "func_door_rotating" ) )
					{
						pEntity->TakeDamage(pev, pev, 0, DMG_OPEN_DOOR);
						m_OpenDoorWaitTime = gpGlobals->time + 2.0;
					}
				}
			}
		}
	}

	if(m_rpgms_type == 1)
	{
		if(m_rpgms_exp >= 1000)
		{
			m_rpgms_exp -= 1000;
			m_rpgms_level += 1;
			if(m_rpgms_level > 99)
				m_rpgms_level = 99;
		}
	}

	RunAI();

	if(m_thinkspeed == 3)
	{
		pev->nextthink = gpGlobals->time + 0.02;
	}
	else if(m_thinkspeed == 2)
	{
		pev->nextthink = gpGlobals->time + 0.2;
	}
	else if(m_thinkspeed == -1)
	{
		pev->nextthink = gpGlobals->time + 0.3;
	}
	else
	{
		pev->nextthink = gpGlobals->time + 0.1;
	}

	float flInterval = StudioFrameAdvance( ); // animate

	// start or end a fidget
	// This needs a better home -- switching animations over time should be encapsulated on a per-activity basis
	// perhaps MaintainActivity() or a ShiftAnimationOverTime() or something.
	if( m_MonsterState != MONSTERSTATE_HUNT && m_MonsterState != MONSTERSTATE_SCRIPT && m_MonsterState != MONSTERSTATE_DEAD && m_Activity == ACT_IDLE && m_fSequenceFinished )
	{
		int iSequence;

		if( m_fSequenceLoops )
		{
			// animation does loop, which means we're playing subtle idle. Might need to 
			// fidget.
			iSequence = LookupActivity( m_Activity );
		}
		else
		{
			// animation that just ended doesn't loop! That means we just finished a fidget
			// and should return to our heaviest weighted idle (the subtle one)
			iSequence = LookupActivityHeaviest( m_Activity );
		}
		if( iSequence != ACTIVITY_NOT_AVAILABLE )
		{
			pev->sequence = iSequence;	// Set to new anim (if it's there)
			ResetSequenceInfo();
		}
	}

	DispatchAnimEvents( flInterval );

	if( !MovementIsComplete() && m_freezetime == 0 )
	{
		Move( flInterval );
	}
#if _DEBUG	
	else 
	{
		if( !TaskIsRunning() && !TaskIsComplete() )
			ALERT( at_error, "Schedule stalled!!\n" );
	}
#endif
}

//=========================================================
// CBaseMonster - USE - will make a monster angry at whomever
// activated it.
//=========================================================
void CBaseMonster::MonsterUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if (m_MonsterState == MONSTERSTATE_IDLE)
		m_IdealMonsterState = MONSTERSTATE_ALERT;
}

//=========================================================
// Ignore conditions - before a set of conditions is allowed
// to interrupt a monster's schedule, this function removes
// conditions that we have flagged to interrupt the current
// schedule, but may not want to interrupt the schedule every
// time. (Pain, for instance)
//=========================================================
int CBaseMonster::IgnoreConditions( void )
{
	int iIgnoreConditions = 0;

	if( !FShouldEat() )
	{
		// not hungry? Ignore food smell.
		iIgnoreConditions |= bits_COND_SMELL_FOOD;
	}

	if( m_MonsterState == MONSTERSTATE_SCRIPT && m_pCine )
		iIgnoreConditions |= m_pCine->IgnoreConditions();

	return iIgnoreConditions;
}

//=========================================================
// 	RouteClear - zeroes out the monster's route array and goal
//=========================================================
void CBaseMonster::RouteClear( void )
{
	RouteNew();
	if(m_movementGoal == MOVEGOAL_PATHCORNER && m_cleardally_enemy == 0 && pev->deadflag == DEAD_NO )
	{
	}
	else
	{
		m_movementGoal = MOVEGOAL_NONE;
		m_movementActivity = ACT_IDLE;
		Forget( bits_MEMORY_MOVE_FAILED );
	}
}

//=========================================================
// Route New - clears out a route to be changed, but keeps
//				goal intact.
//=========================================================
void CBaseMonster::RouteNew( void )
{
	m_Route[0].iType = 0;
	m_iRouteIndex = 0;
}

//=========================================================
// FRouteClear - returns TRUE is the Route is cleared out
// ( invalid )
//=========================================================
BOOL CBaseMonster::FRouteClear( void )
{
	if( m_Route[m_iRouteIndex].iType == 0 || m_movementGoal == MOVEGOAL_NONE )
		return TRUE;

	return FALSE;
}

//=========================================================
// FRefreshRoute - after calculating a path to the monster's
// target, this function copies as many waypoints as possible
// from that path to the monster's Route array
//=========================================================
BOOL CBaseMonster::FRefreshRoute( void )
{
	CBaseEntity	*pPathCorner;
	int		i;
	BOOL		returnCode;

	RouteNew();

	returnCode = FALSE;

	switch( m_movementGoal )
	{
		case MOVEGOAL_PATHCORNER:
			{
				// monster is on a path_corner loop
				pPathCorner = m_pGoalEnt;
				i = 0;

				while( pPathCorner && i < ROUTE_SIZE )
				{
					m_Route[i].iType = bits_MF_TO_PATHCORNER;
					m_Route[i].vecLocation = pPathCorner->pev->origin;

					pPathCorner = pPathCorner->GetNextTarget();

					// Last path_corner in list?
					if( !pPathCorner )
						m_Route[i].iType |= bits_MF_IS_GOAL;
					i++;
				}
			}
			returnCode = TRUE;
			break;
		case MOVEGOAL_ENEMY:
			returnCode = BuildRoute( m_vecEnemyLKP, bits_MF_TO_ENEMY, m_hEnemy );
			break;
		case MOVEGOAL_LOCATION:
			returnCode = BuildRoute( m_vecMoveGoal, bits_MF_TO_LOCATION, NULL );
			break;
		case MOVEGOAL_TARGETENT:
			if( m_hTargetEnt != 0 )
			{
				returnCode = BuildRoute( m_hTargetEnt->pev->origin, bits_MF_TO_TARGETENT, m_hTargetEnt );
			}
			break;
		case MOVEGOAL_NODE:
			returnCode = FGetNodeRoute( m_vecMoveGoal );
			//if( returnCode )
			//	RouteSimplify( NULL );
			break;
	}

	return returnCode;
}

BOOL CBaseMonster::MoveToEnemy( Activity movementAct, float waitTime )
{
	m_movementActivity = movementAct;
	m_moveWaitTime = waitTime;

	m_movementGoal = MOVEGOAL_ENEMY;
	return FRefreshRoute();
}

BOOL CBaseMonster::MoveToLocation( Activity movementAct, float waitTime, const Vector &goal )
{
	m_movementActivity = movementAct;
	m_moveWaitTime = waitTime;

	m_movementGoal = MOVEGOAL_LOCATION;
	m_vecMoveGoal = goal;
	return FRefreshRoute();
}

BOOL CBaseMonster::MoveToTarget( Activity movementAct, float waitTime )
{
	m_movementActivity = movementAct;
	m_moveWaitTime = waitTime;

	m_movementGoal = MOVEGOAL_TARGETENT;
	return FRefreshRoute();
}

BOOL CBaseMonster::MoveToNode( Activity movementAct, float waitTime, const Vector &goal )
{
	m_movementActivity = movementAct;
	m_moveWaitTime = waitTime;

	m_movementGoal = MOVEGOAL_NODE;
	m_vecMoveGoal = goal;
	return FRefreshRoute();
}

#if _DEBUG
void DrawRoute( entvars_t *pev, WayPoint_t *m_Route, int m_iRouteIndex, int r, int g, int b )
{
	int i;

	if( m_Route[m_iRouteIndex].iType == 0 )
	{
		ALERT( at_aiconsole, "Can't draw route!\n" );
		return;
	}

	//UTIL_ParticleEffect ( m_Route[m_iRouteIndex].vecLocation, g_vecZero, 255, 25 );

	MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
		WRITE_BYTE( TE_BEAMPOINTS);
		WRITE_COORD( pev->origin.x );
		WRITE_COORD( pev->origin.y );
		WRITE_COORD( pev->origin.z );
		WRITE_COORD( m_Route[m_iRouteIndex].vecLocation.x );
		WRITE_COORD( m_Route[m_iRouteIndex].vecLocation.y );
		WRITE_COORD( m_Route[m_iRouteIndex].vecLocation.z );

		WRITE_SHORT( g_sModelIndexLaser );
		WRITE_BYTE( 0 ); // frame start
		WRITE_BYTE( 10 ); // framerate
		WRITE_BYTE( 1 ); // life
		WRITE_BYTE( 16 );  // width
		WRITE_BYTE( 0 );   // noise
		WRITE_BYTE( r );   // r, g, b
		WRITE_BYTE( g );   // r, g, b
		WRITE_BYTE( b );   // r, g, b
		WRITE_BYTE( 255 );	// brightness
		WRITE_BYTE( 10 );		// speed
	MESSAGE_END();

	for( i = m_iRouteIndex; i < ROUTE_SIZE - 1; i++ )
	{
		if( ( m_Route[i].iType & bits_MF_IS_GOAL ) || ( m_Route[i + 1].iType == 0 ) )
			break;

		MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
			WRITE_BYTE( TE_BEAMPOINTS );
			WRITE_COORD( m_Route[i].vecLocation.x );
			WRITE_COORD( m_Route[i].vecLocation.y );
			WRITE_COORD( m_Route[i].vecLocation.z );
			WRITE_COORD( m_Route[i + 1].vecLocation.x );
			WRITE_COORD( m_Route[i + 1].vecLocation.y );
			WRITE_COORD( m_Route[i + 1].vecLocation.z );
			WRITE_SHORT( g_sModelIndexLaser );
			WRITE_BYTE( 0 ); // frame start
			WRITE_BYTE( 10 ); // framerate
			WRITE_BYTE( 1 ); // life
			WRITE_BYTE( 8 );  // width
			WRITE_BYTE( 0 );   // noise
			WRITE_BYTE( r );   // r, g, b
			WRITE_BYTE( g );   // r, g, b
			WRITE_BYTE( b );   // r, g, b
			WRITE_BYTE( 255 );	// brightness
			WRITE_BYTE( 10 );		// speed
		MESSAGE_END();

		//UTIL_ParticleEffect( m_Route[i].vecLocation, g_vecZero, 255, 25 );
	}
}
#endif

int ShouldSimplify( int routeType )
{
	routeType &= ~bits_MF_IS_GOAL;

	if( ( routeType == bits_MF_TO_PATHCORNER ) || ( routeType & bits_MF_DONT_SIMPLIFY ) )
		return FALSE;
	return TRUE;
}

//=========================================================
// RouteSimplify
//
// Attempts to make the route more direct by cutting out
// unnecessary nodes & cutting corners.
//
//=========================================================
void CBaseMonster::RouteSimplify( CBaseEntity *pTargetEnt )
{
	// BUGBUG: this doesn't work 100% yet
	int		i, count, outCount;
	Vector		vecStart;
	WayPoint_t	outRoute[ROUTE_SIZE * 2];	// Any points except the ends can turn into 2 points in the simplified route

	count = 0;

	for( i = m_iRouteIndex; i < ROUTE_SIZE; i++ )
	{
		if( !m_Route[i].iType )
			break;
		else
			count++;
		if( m_Route[i].iType & bits_MF_IS_GOAL )
			break;
	}
	// Can't simplify a direct route!
	if( count < 2 )
	{
		//DrawRoute( pev, m_Route, m_iRouteIndex, 0, 0, 255 );
		return;
	}

	outCount = 0;
	vecStart = pev->origin;
	for( i = 0; i < count - 1; i++ )
	{
		// Don't eliminate path_corners
		if( !ShouldSimplify( m_Route[m_iRouteIndex + i].iType ) )
		{
			outRoute[outCount] = m_Route[m_iRouteIndex + i];
			outCount++;
		}
		else if( CheckLocalMove( vecStart, m_Route[m_iRouteIndex+i + 1].vecLocation, pTargetEnt, NULL, 1 ) == LOCALMOVE_VALID )
		{
			// Skip vert
			continue;
		}
		else
		{
			Vector vecTest, vecSplit;

			// Halfway between this and next
			vecTest = ( m_Route[m_iRouteIndex + i + 1].vecLocation + m_Route[m_iRouteIndex + i].vecLocation ) * 0.5f;

			// Halfway between this and previous
			vecSplit = ( m_Route[m_iRouteIndex + i].vecLocation + vecStart ) * 0.5f;

			int iType = ( m_Route[m_iRouteIndex + i].iType | bits_MF_TO_DETOUR ) & ~bits_MF_NOT_TO_MASK;
			if( CheckLocalMove( vecStart, vecTest, pTargetEnt, NULL, 1  ) == LOCALMOVE_VALID )
			{
				outRoute[outCount].iType = iType;
				outRoute[outCount].vecLocation = vecTest;
			}
			else if( CheckLocalMove( vecSplit, vecTest, pTargetEnt, NULL, 1 ) == LOCALMOVE_VALID )
			{
				outRoute[outCount].iType = iType;
				outRoute[outCount].vecLocation = vecSplit;
				outRoute[outCount+1].iType = iType;
				outRoute[outCount+1].vecLocation = vecTest;
				outCount++; // Adding an extra point
			}
			else
			{
				outRoute[outCount] = m_Route[m_iRouteIndex + i];
			}
		}
		// Get last point
		vecStart = outRoute[outCount].vecLocation;
		outCount++;
	}
	ASSERT( i < count );
	outRoute[outCount] = m_Route[m_iRouteIndex + i];
	outCount++;

	// Terminate
	outRoute[outCount].iType = 0;
	ASSERT( outCount < ( ROUTE_SIZE * 2 ) );

	// Copy the simplified route, disable for testing
	m_iRouteIndex = 0;
	for( i = 0; i < ROUTE_SIZE && i < outCount; i++ )
	{
		m_Route[i] = outRoute[i];
	}

	// Terminate route
	if( i < ROUTE_SIZE )
		m_Route[i].iType = 0;

// Debug, test movement code
#if 0
//	if( CVAR_GET_FLOAT( "simplify" ) != 0 )
		DrawRoute( pev, outRoute, 0, 255, 0, 0 );
//	else
		DrawRoute( pev, m_Route, m_iRouteIndex, 0, 255, 0 );
#endif
}

//=========================================================
// FBecomeProne - tries to send a monster into PRONE state.
// right now only used when a barnacle snatches someone, so 
// may have some special case stuff for that.
//=========================================================
BOOL CBaseMonster::FBecomeProne( void )
{
	if( FBitSet( pev->flags, FL_ONGROUND ) )
	{
		pev->flags -= FL_ONGROUND;
	}

	m_hEnemy = NULL;
	m_hOldEnemy[0] = NULL;
	m_hOldEnemy[1] = NULL;
	m_hOldEnemy[2] = NULL;
	m_hOldEnemy[3] = NULL;
	ClearSchedule();
	SetState( MONSTERSTATE_PRONE );
	//m_IdealMonsterState = MONSTERSTATE_PRONE;
	RouteClear();

	//pev->movetype = MOVETYPE_TOSS;
	return TRUE;
}

//=========================================================
// CheckRangeAttack1
//=========================================================
BOOL CBaseMonster::CheckRangeAttack1( float flDot, float flDist )
{
	if( flDist > 64.0f && flDist <= 784.0f && flDot >= 0.5f )
	{
		return TRUE;
	}
	return FALSE;
}

//=========================================================
// CheckRangeAttack2
//=========================================================
BOOL CBaseMonster::CheckRangeAttack2( float flDot, float flDist )
{
	if( flDist > 64.0f && flDist <= 512.0f && flDot >= 0.5f )
	{
		return TRUE;
	}
	return FALSE;
}

//=========================================================
// CheckMeleeAttack1
//=========================================================
BOOL CBaseMonster::CheckMeleeAttack1( float flDot, float flDist )
{
	// Decent fix to keep folks from kicking/punching hornets and snarks is to check the onground flag(sjb)
	if( flDist <= 64.0f && flDot >= 0.7f && m_hEnemy != 0 && FBitSet( m_hEnemy->pev->flags, FL_ONGROUND ) )
	{
		return TRUE;
	}
	return FALSE;
}

//=========================================================
// CheckMeleeAttack2
//=========================================================
BOOL CBaseMonster::CheckMeleeAttack2( float flDot, float flDist )
{
	if( flDist <= 64.0f && flDot >= 0.7f )
	{
		return TRUE;
	}
	return FALSE;
}

//=========================================================
// CheckAttacks - sets all of the bits for attacks that the
// monster is capable of carrying out on the passed entity.
//=========================================================
void CBaseMonster::CheckAttacks( CBaseEntity *pTarget, float flDist )
{
	Vector2D vec2LOS;
	float flDot;

	UTIL_MakeVectors( pev->angles );

	vec2LOS = ( pTarget->pev->origin - pev->origin ).Make2D();
	vec2LOS = vec2LOS.Normalize();

	flDot = DotProduct( vec2LOS, gpGlobals->v_forward.Make2D() );

	// we know the enemy is in front now. We'll find which attacks the monster is capable of by
	// checking for corresponding Activities in the model file, then do the simple checks to validate
	// those attack types.

	// Clear all attack conditions
	ClearConditions( bits_COND_CAN_RANGE_ATTACK1 | bits_COND_CAN_RANGE_ATTACK2 | bits_COND_CAN_MELEE_ATTACK1 |bits_COND_CAN_MELEE_ATTACK2 );

	if(m_hEnemy != NULL)
	{
		if(m_enemyfollower == 1 && m_lovehate > 0)
		{
			if(m_hEnemy->IsPlayer())
			{
				if(m_enemyfollower_combat == 1)
				{
					m_enemyfollower_combat = 0;
				}
				goto dont_attack;
			}
		}
	}

	if(FClassnameIs( pTarget->pev, "npc_aim_flag" ) || FClassnameIs( pTarget->pev, "player_aim_flag" ) )
		goto dont_attack;

	if( m_afCapability & bits_CAP_RANGE_ATTACK1 )
	{
		if( CheckRangeAttack1( flDot, flDist ) )
			SetConditions( bits_COND_CAN_RANGE_ATTACK1 );
	}
	if( m_afCapability & bits_CAP_RANGE_ATTACK2 )
	{
		if( CheckRangeAttack2( flDot, flDist ) )
			SetConditions( bits_COND_CAN_RANGE_ATTACK2 );
	}
	if( m_afCapability & bits_CAP_MELEE_ATTACK1 )
	{
		if( CheckMeleeAttack1( flDot, flDist ) )
			SetConditions( bits_COND_CAN_MELEE_ATTACK1 );
	}
	if( m_afCapability & bits_CAP_MELEE_ATTACK2 )
	{
		if( CheckMeleeAttack2( flDot, flDist ) )
			SetConditions( bits_COND_CAN_MELEE_ATTACK2 );
	}
	dont_attack:;
}

//=========================================================
// CanCheckAttacks - prequalifies a monster to do more fine
// checking of potential attacks. 
//=========================================================
BOOL CBaseMonster::FCanCheckAttacks( void )
{
	if(!HasConditions(bits_COND_SEE_ENEMY) && m_hasenemy_nosee < 100)
	{
		m_hasenemy_nosee++;
	}
	if( HasConditions( bits_COND_SEE_ENEMY ) && !HasConditions( bits_COND_ENEMY_TOOFAR ) )
	{
		return TRUE;
	}

	return FALSE;
}

//=========================================================
// CheckEnemy - part of the Condition collection process,
// gets and stores data and conditions pertaining to a monster's
// enemy. Returns TRUE if Enemy LKP was updated.
//=========================================================
int CBaseMonster::CheckEnemy( CBaseEntity *pEnemy )
{
	float	flDistToEnemy;
	int	iUpdatedLKP;// set this to TRUE if you update the EnemyLKP in this function.

	iUpdatedLKP = FALSE;
	ClearConditions( bits_COND_ENEMY_FACING_ME );
	ClearConditions( bits_COND_NEW_COVER_MODE );

	if( !FVisible( pEnemy ) )
	{
		ASSERT( !HasConditions( bits_COND_SEE_ENEMY ) );
		SetConditions( bits_COND_ENEMY_OCCLUDED );
	}
	else
		ClearConditions( bits_COND_ENEMY_OCCLUDED );

	if( !pEnemy->IsAlive() )
	{
		SetConditions( bits_COND_ENEMY_DEAD );
		ClearConditions( bits_COND_SEE_ENEMY | bits_COND_ENEMY_OCCLUDED );
		m_alert = 100;

		if(m_enemyfollower == 1 && m_new_ally_type)
		{
			m_enemyfollower_combat = 0;
		}
		return FALSE;
	}

	if(pEnemy->IsPlayer())
	{
		if(m_enemyfollower == 0 && m_new_ally_type && m_lovehate > 0)
		{
			return FALSE;
		}
		else if(m_enemyfollower == 1 && m_new_ally_type && m_lovehate > 0)
		{
			if ( HasConditions( bits_COND_SEE_ENEMY ) )
			{
				float flDist = ( pEnemy->pev->origin - pev->origin).Length();
						
				if(m_cover_fromplayer == TRUE)
				{
					if ( flDist >= 768)
					{
						m_cover_fromplayer = FALSE;
					}
					else if ( flDist <= 192)
					{
						if(m_igonre_npc <= 0)
							m_cover_fromplayer = FALSE;
					}
					else if(m_igonre_npc < 5)
					{
						m_igonre_npc = 5;
					}
				}

							
				if ( flDist <= 128 )
				{
					if ( m_IdealActivity == m_movementActivity )
					{
						m_IdealActivity = GetStoppedActivity();
					}
					RouteClear();
				}
				else if ( flDist <= 256 )
				{
					if(m_movementActivity == ACT_RUN)
					{
						m_movementActivity = ACT_WALK;
					}
				}
				else
				{
					if(m_movementActivity == ACT_WALK)
						m_movementActivity = ACT_RUN;	
				}
			}
			else
			{
				if(m_movementActivity == ACT_WALK)
					m_movementActivity = ACT_RUN;
							
			}
		}
	}
	else
	{
		if(m_enemyfollower == 1 && m_new_ally_type)
		{
			if ( HasConditions( bits_COND_SEE_ENEMY ) )
				m_enemyfollower_combat = 1;
		}
	}

	Vector vecEnemyPos = pEnemy->pev->origin;

	// distance to enemy's origin
	flDistToEnemy = ( vecEnemyPos - pev->origin ).Length();
	vecEnemyPos.z += pEnemy->pev->size.z * 0.5f;

	// distance to enemy's head
	float flDistToEnemy2 = ( vecEnemyPos - pev->origin ).Length();
	if( flDistToEnemy2 < flDistToEnemy )
		flDistToEnemy = flDistToEnemy2;
	else
	{
		// distance to enemy's feet
		vecEnemyPos.z -= pEnemy->pev->size.z;
		flDistToEnemy2 = ( vecEnemyPos - pev->origin ).Length();
		if( flDistToEnemy2 < flDistToEnemy )
			flDistToEnemy = flDistToEnemy2;
	}

	if( HasConditions( bits_COND_SEE_ENEMY ) )
	{
		CBaseMonster *pEnemyMonster;

		iUpdatedLKP = TRUE;
		m_vecEnemyLKP = pEnemy->pev->origin;

		pEnemyMonster = pEnemy->MyMonsterPointer();

		if( pEnemyMonster )
		{
			if(pEnemyMonster->FInViewCone2 ( this ))
			{
				m_HenemyEnemyMe2 = 2;
			}
			else if(pEnemyMonster->FInViewCone3 ( this ))
			{
				m_HenemyEnemyMe2 = 1;
			}
			else
			{
				m_HenemyEnemyMe2 = 0;
			}

			if(pEnemyMonster->m_hEnemy != NULL)
			{
				if(pEnemyMonster->m_hEnemy != this){
					if(pEnemyMonster->FInViewCone3 ( this ))
					{
						m_HenemyEnemyMe = 1;
					}
					else{
						m_HenemyEnemyMe = 0;
					}
				}
				else if(pEnemyMonster->m_hEnemy == this)
				{
					if(pEnemyMonster->HasConditions(bits_COND_CAN_RANGE_ATTACK1)
					|| pEnemyMonster->HasConditions(bits_COND_CAN_RANGE_ATTACK2)
					|| pEnemyMonster->HasConditions(bits_COND_CAN_MELEE_ATTACK1)
					|| pEnemyMonster->HasConditions(bits_COND_CAN_MELEE_ATTACK2)  )
					{
						if ( FClassnameIs( m_hEnemy->pev, "monster_houndeye") )
						{
							m_HenemyEnemyMe = 1;
						}
						else
						{
							m_HenemyEnemyMe = 3;
						}
					}
					else if(pEnemyMonster->pev->sequence == LookupActivity ( ACT_RELOAD ) 
					|| pEnemyMonster->pev->sequence == LookupActivity ( ACT_COWER ))
					{
						m_HenemyEnemyMe = 0;
					}
					else if(pEnemyMonster->FInViewCone3 ( this ) )
					{
						m_HenemyEnemyMe = 2;
					}
					else
					{
						m_HenemyEnemyMe = 1;
					}
				}

				if(FClassnameIs(pev,"monster_misaliya") || FClassnameIs(pev,"monster_wisebeast")
				|| FClassnameIs(pev,"monster_hime") || FClassnameIs(pev,"monster_mario"))
				{
					float flDist = ( pEnemy->pev->origin - pev->origin).Length();
					if(pEnemyMonster->m_hEnemy == this)
					{
						if(pEnemyMonster->HasConditions(bits_COND_CAN_RANGE_ATTACK1)
						|| pEnemyMonster->HasConditions(bits_COND_CAN_RANGE_ATTACK2)
						|| pEnemyMonster->HasConditions(bits_COND_CAN_MELEE_ATTACK1)
						|| pEnemyMonster->HasConditions(bits_COND_CAN_MELEE_ATTACK2) 
						|| flDist <= pEnemyMonster->m_attack_dist)
							m_HenemyEnemyMe = 4;
					}
					else
					{
						if(FClassnameIs(m_hEnemy->pev, "monster_hydra_boss"))
						{
							if((pEnemyMonster->FInViewCone3 ( this ) || flDist <= pEnemyMonster->m_attack_dist))
								m_HenemyEnemyMe = 4;
						}
					}
				}
			}
			else if ( (m_hEnemy->pev->flags & FL_CLIENT) )
			{
				if(m_enemyfollower == 0)
				{
					if(pEnemyMonster->FInViewCone2 ( this ))
					{
						m_HenemyEnemyMe = 2;
					}
					else if(pEnemyMonster->FInViewCone3 ( this ))
					{
						m_HenemyEnemyMe = 1;
					}
					else
					{
						m_HenemyEnemyMe = 0;
					}
				}
				else
				{
					m_HenemyEnemyMe = 0;
				}
			}
			else
			{
				m_HenemyEnemyMe = 0;
			}

			if( pEnemyMonster->FInViewCone( this ) )
			{
				SetConditions( bits_COND_ENEMY_FACING_ME );
			}
			else
				ClearConditions( bits_COND_ENEMY_FACING_ME );
		}

		if( pEnemy->pev->velocity != Vector( 0, 0, 0 ) )
		{
			// trail the enemy a bit
			m_vecEnemyLKP = m_vecEnemyLKP - pEnemy->pev->velocity * RANDOM_FLOAT( -0.05f, 0.05f );
			m_vecEnemyLKP.z = pEnemy->pev->origin.z;
		}
		else
		{
			// UNDONE: use pev->oldorigin?
		}
		if(m_cover_dist >= 64 )
		{
			if( !m_crouchmode && !IsMoving() )
			{
				if(FClassnameIs(m_hEnemy->pev, "monster_headcrab") || FClassnameIs(m_hEnemy->pev, "monster_houndeye"))
				{
					goto no_cover;
				}
				if ( fabs( pev->origin.z - pEnemyMonster->pev->origin.z ) <= 128 )
				{
					if(pEnemyMonster->m_hEnemy != NULL)
					{
						if(pEnemyMonster->m_hEnemy == this 
						&& pEnemyMonster->FInViewCone3(this) 
						&& flDistToEnemy <= m_cover_dist
						&& FInViewCone3(pEnemyMonster)){
							SetConditions ( bits_COND_NEW_COVER_MODE );
						}
					}
				}
			}
			no_cover:;
		}
	}
	else if( !HasConditions( bits_COND_ENEMY_OCCLUDED | bits_COND_SEE_ENEMY ) && ( flDistToEnemy <= 256 ) )
	{
		// if the enemy is not occluded, and unseen, that means it is behind or beside the monster.
		// if the enemy is near enough the monster, we go ahead and let the monster know where the
		// enemy is. 
		iUpdatedLKP = TRUE;
		m_vecEnemyLKP = pEnemy->pev->origin;
	}

	if( flDistToEnemy >= m_flDistTooFar )
	{
		// enemy is very far away from monster
		SetConditions( bits_COND_ENEMY_TOOFAR );
	}
	else
		ClearConditions( bits_COND_ENEMY_TOOFAR );

	if( FCanCheckAttacks() )	
	{
		CheckAttacks( m_hEnemy, flDistToEnemy );
	}

	if( m_movementGoal == MOVEGOAL_ENEMY )
	{
		for( int i = m_iRouteIndex; i < ROUTE_SIZE; i++ )
		{
			if( m_Route[i].iType == ( bits_MF_IS_GOAL | bits_MF_TO_ENEMY ) )
			{
				// UNDONE: Should we allow monsters to override this distance (80?)
				if( ( m_Route[i].vecLocation - m_vecEnemyLKP ).Length() > 80.0f )
				{
					// Refresh
					FRefreshRoute();
					return iUpdatedLKP;
				}
			}
		}
	}

	return iUpdatedLKP;
}

//=========================================================
// PushEnemy - remember the last few enemies, always remember the player
//=========================================================
void CBaseMonster::PushEnemy( CBaseEntity *pEnemy, Vector &vecLastKnownPos )
{
	if(m_movementGoal == MOVEGOAL_PATHCORNER)
		m_movementGoal = MOVEGOAL_NONE;

	if(m_boltpoison > 0 || m_iTriggerCondition == 123)
		return;

	int i;

	if( pEnemy == NULL )
		return;

	if (FClassnameIs( pEnemy->pev, "player_aim_flag" ) || FClassnameIs( pEnemy->pev, "npc_aim_flag" ) )
		return;
		
	// UNDONE: blah, this is bad, we should use a stack but I'm too lazy to code one.
	for( i = 0; i < MAX_OLD_ENEMIES; i++ )
	{
		if( m_hOldEnemy[i] == pEnemy )
			return;
		if( m_hOldEnemy[i] == 0 ) // someone died, reuse their slot
			break;
	}
	if( i >= MAX_OLD_ENEMIES )
		return;

	m_hOldEnemy[i] = pEnemy;
	m_vecOldEnemy[i] = vecLastKnownPos;
}

//=========================================================
// PopEnemy - try remembering the last few enemies
//=========================================================
BOOL CBaseMonster::PopEnemy()
{
	// UNDONE: blah, this is bad, we should use a stack but I'm too lazy to code one.
	for( int i = MAX_OLD_ENEMIES - 1; i >= 0; i-- )
	{
		if( m_hOldEnemy[i] != 0 )
		{
			if( m_hOldEnemy[i]->IsAlive()) // cheat and know when they die
			{
				m_hEnemy = m_hOldEnemy[i];
				m_vecEnemyLKP = m_vecOldEnemy[i];
				// ALERT( at_console, "remembering\n" );
				return TRUE;
			}
			else
			{
				m_hOldEnemy[i] = NULL;
			}
		}
	}
	return FALSE;
}

//=========================================================
// SetActivity 
//=========================================================
void CBaseMonster::SetActivity( Activity NewActivity )
{
	if ( m_IdealMonsterState == MONSTERSTATE_DEAD || pev->deadflag != DEAD_NO || pev->health <= 0 || m_die >= 1 )
	{
		if(m_die == 2)
		{
			if(NewActivity == ACT_BARNACLE_HIT || NewActivity == ACT_BARNACLE_PULL
			|| NewActivity == ACT_BARNACLE_CHOMP || NewActivity == ACT_BARNACLE_CHEW)
			{
				return;
			}
		}

		if(NewActivity <= ACT_EAT || pev->deadflag == DEAD_DEAD)
		{
			if(NewActivity != ACT_FALL && NewActivity != ACT_DIESIMPLE)
			{
				return;
			}
		}
	}

	int iSequence;

	iSequence = LookupActivity( NewActivity );

	Activity OldActivity = m_Activity;
	m_Activity = NewActivity; // Go ahead and set this so it doesn't keep trying when the anim is not present

	// In case someone calls this with something other than the ideal activity
	m_IdealActivity = m_Activity;

	// Set to the desired anim, or default anim if the desired is not present
	if( iSequence > ACTIVITY_NOT_AVAILABLE )
	{
		if( pev->sequence != iSequence || !m_fSequenceLoops )
		{
			// don't reset frame between walk and run
			if( !( OldActivity == ACT_WALK || OldActivity == ACT_RUN ) || !( NewActivity == ACT_WALK || NewActivity == ACT_RUN ) )
				pev->frame = 0;
		}

		pev->sequence = iSequence;	// Set to the reset anim (if it's there)
		ResetSequenceInfo();
		SetYawSpeed();
	}
	else
	{
		// Not available try to get default anim
		ALERT( at_aiconsole, "%s has no sequence for act:%d\n", STRING( pev->classname ), NewActivity );
		if(pev->deadflag == DEAD_NO)
		{
			pev->sequence = 0;	// Set to the reset anim (if it's there)
		}
	}
}

//=========================================================
// SetSequenceByName
//=========================================================
void CBaseMonster::SetSequenceByName( const char *szSequence )
{
	int iSequence;

	iSequence = LookupSequence( szSequence );

	// Set to the desired anim, or default anim if the desired is not present
	if( iSequence > ACTIVITY_NOT_AVAILABLE )
	{
		if( pev->sequence != iSequence || !m_fSequenceLoops )
		{
			pev->frame = 0;
		}

		pev->sequence = iSequence;	// Set to the reset anim (if it's there)
		ResetSequenceInfo();
		SetYawSpeed();
	}
	else
	{
		// Not available try to get default anim
		ALERT( at_aiconsole, "%s has no sequence named:%f\n", STRING(pev->classname), szSequence );
		pev->sequence = 0;	// Set to the reset anim (if it's there)
	}
}

//=========================================================
// CheckLocalMove - returns TRUE if the caller can walk a 
// straight line from its current origin to the given 
// location. If so, don't use the node graph!
//
// if a valid pointer to a int is passed, the function
// will fill that int with the distance that the check 
// reached before hitting something. THIS ONLY HAPPENS
// IF THE LOCAL MOVE CHECK FAILS!
//
// !!!PERFORMANCE - should we try to load balance this?
// DON"T USE SETORIGIN! 
//=========================================================
#define	LOCAL_STEP_SIZE	16
int CBaseMonster::CheckLocalMove( const Vector &vecStart, const Vector &vecEnd, CBaseEntity *pTarget, float *pflDist, int igonrefailed )
{
	Vector vecStartPos;// record monster's position before trying the move
	float flYaw;
	float flDist;
	float flStep, stepSize;
	int iReturn;

	vecStartPos = pev->origin;

	flYaw = UTIL_VecToYaw( vecEnd - vecStart );// build a yaw that points to the goal.
	flDist = ( vecEnd - vecStart ).Length2D();// get the distance.
	iReturn = LOCALMOVE_VALID;// assume everything will be ok.

	// move the monster to the start of the local move that's to be checked.
	UTIL_SetOrigin( pev, vecStart );// !!!BUGBUG - won't this fire triggers? - nope, SetOrigin doesn't fire

	if( !m_groundElev && !( pev->flags & ( FL_FLY | FL_SWIM ) ) )
	{
		DROP_TO_FLOOR( ENT( pev ) );//make sure monster is on the floor!
	}

	//pev->origin.z = vecStartPos.z;//!!!HACKHACK

	//pev->origin = vecStart;

/*
	if( flDist > 1024 )
	{
		// !!!PERFORMANCE - this operation may be too CPU intensive to try checks this large.
		// We don't lose much here, because a distance this great is very likely
		// to have something in the way.

		// since we've actually moved the monster during the check, undo the move.
		pev->origin = vecStartPos;
		return FALSE;
	}
*/
	// this loop takes single steps to the goal.
	for( flStep = 0; flStep < flDist; flStep += LOCAL_STEP_SIZE )
	{
		stepSize = LOCAL_STEP_SIZE;

		if( ( flStep + LOCAL_STEP_SIZE ) >= ( flDist - 1 ) )
			stepSize = ( flDist - flStep ) - 1;

		//UTIL_ParticleEffect( pev->origin, g_vecZero, 255, 25 );

		if( !WALK_MOVE( ENT( pev ), flYaw, stepSize, WALKMOVE_CHECKONLY ) )
		{
			// can't take the next step, fail!
			if( pflDist != NULL )
			{
				*pflDist = flStep;
			}
			if( pTarget && pTarget->edict() == gpGlobals->trace_ent )
			{
				// if this step hits target ent, the move is legal.
				iReturn = LOCALMOVE_VALID;
				break;
			}
			else
			{
				if(m_ignoreFail_OFF == 0)
				{
					if(m_cleardally_enemy > 0 || m_hTargetEnt != NULL)
					{
						if ( (m_ignoreFail >= 0 && m_ignoreFail <= m_ignoreFail_MAX) )
						{
							if(m_ignoreFail != 1919)
								m_ignoreFail += 1;
						}
						else
						{
							iReturn = LOCALMOVE_INVALID;
						}
					}
					else
					{
						iReturn = LOCALMOVE_INVALID;
					}
				}
				else
				{
					iReturn = LOCALMOVE_INVALID;
				}
				break;
			}
		}
	}

	if( iReturn == LOCALMOVE_VALID && !( pev->flags & ( FL_FLY | FL_SWIM ) ) && ( !pTarget || ( pTarget->pev->flags & FL_ONGROUND ) ) )
	{
		// The monster can move to a spot UNDER the target, but not to it. Don't try to triangulate, go directly to the node graph.
		// UNDONE: Magic # 64 -- this used to be pev->size.z but that won't work for small creatures like the headcrab
		if( fabs( vecEnd.z - pev->origin.z ) > 128.0f && !m_MoveFail_FuckRoad && m_ctmod != 2 )
		{
			if(m_ignoreFail_OFF == 0)
			{
				if ( (m_ignoreFail >= 0 && m_ignoreFail <= m_ignoreFail_MAX) || m_ignoreFail == 1919 )
				{
					if(m_ignoreFail != 1919)
					{
						m_ignoreFail += 1;
					}
				}
				else
				{
					iReturn = LOCALMOVE_INVALID_DONT_TRIANGULATE;
				}
			}
			else
			{
				iReturn = LOCALMOVE_INVALID_DONT_TRIANGULATE;
			}
		}
	}
	/*
	// uncommenting this block will draw a line representing the nearest legal move.
	WRITE_BYTE( MSG_BROADCAST, SVC_TEMPENTITY );
	WRITE_BYTE( MSG_BROADCAST, TE_SHOWLINE );
	WRITE_COORD( MSG_BROADCAST, pev->origin.x );
	WRITE_COORD( MSG_BROADCAST, pev->origin.y );
	WRITE_COORD( MSG_BROADCAST, pev->origin.z );
	WRITE_COORD( MSG_BROADCAST, vecStart.x );
	WRITE_COORD( MSG_BROADCAST, vecStart.y );
	WRITE_COORD( MSG_BROADCAST, vecStart.z );
	*/

	// since we've actually moved the monster during the check, undo the move.
	UTIL_SetOrigin( pev, vecStartPos );


	if(iReturn != LOCALMOVE_VALID && m_MoveFail_FuckRoad)
	{
		if(m_MoveFail_FuckRoad_Mode == 1)
		{
			if(m_MoveFailNum >= 30)
			{
				iReturn = LOCALMOVE_VALID;
			}
		}
		else if(m_MoveFailNum >= 10)
		{
			iReturn = LOCALMOVE_VALID;
		}

		TraceResult tr;
		UTIL_MakeVectors(pev->angles);
		Vector vecSrc = pev->origin + Vector(0,0,18);
		Vector vecEnd	= vecSrc + gpGlobals->v_forward * 36;
		UTIL_TraceLine( vecSrc, vecEnd, ignore_monsters, ENT( pev ), &tr );
		if ( tr.flFraction == 1.0 )
		{
			m_MoveFailNum += 1;
		}
		else
		{
			m_MoveFailNum = 0;
		}

	}
	else if(m_MoveFailNum > 0)
	{
		m_MoveFailNum = 0;
	}

	return iReturn;
}

float CBaseMonster::OpenDoorAndWait( entvars_t *pevDoor )
{
	float flTravelTime = 0;

	//ALERT( at_aiconsole, "A door. " );
	CBaseEntity *pcbeDoor = CBaseEntity::Instance( pevDoor );
	if( pcbeDoor && !pcbeDoor->IsLockedByMaster() )
	{
		//ALERT( at_aiconsole, "unlocked! " );
		pcbeDoor->Use( this, this, USE_ON, 0.0 );
		//ALERT( at_aiconsole, "pevDoor->nextthink = %d ms\n", (int)( 1000 * pevDoor->nextthink ) );
		//ALERT( at_aiconsole, "pevDoor->ltime = %d ms\n", (int)( 1000 * pevDoor->ltime ) );
		//ALERT( at_aiconsole, "pev-> nextthink = %d ms\n", (int)( 1000 * pev->nextthink ) );
		//ALERT( at_aiconsole, "pev->ltime = %d ms\n", (int)( 1000 * pev->ltime ) );
		flTravelTime = pevDoor->nextthink - pevDoor->ltime;
		//ALERT( at_aiconsole, "Waiting %d ms\n", (int)( 1000 * flTravelTime ) );
		if( pcbeDoor->pev->targetname )
		{
			edict_t *pentTarget = NULL;
			for( ; ; )
			{
				pentTarget = FIND_ENTITY_BY_TARGETNAME( pentTarget, STRING(pcbeDoor->pev->targetname ) );

				if( VARS( pentTarget ) != pcbeDoor->pev )
				{
					if( FNullEnt( pentTarget ) )
						break;

					if( FClassnameIs( pentTarget, STRING( pcbeDoor->pev->classname ) ) )
					{
						CBaseEntity *pDoor = Instance( pentTarget );
						if( pDoor )
							pDoor->Use( this, this, USE_ON, 0.0 );
					}
				}
			}
		}
	}

	return gpGlobals->time + flTravelTime;
}

//=========================================================
// AdvanceRoute - poorly named function that advances the 
// m_iRouteIndex. If it goes beyond ROUTE_SIZE, the route 
// is refreshed. 
//=========================================================
void CBaseMonster::AdvanceRoute( float distance )
{
	if( m_iRouteIndex == ROUTE_SIZE - 1 )
	{
		// time to refresh the route.
		if( !FRefreshRoute() )
		{
			ALERT( at_aiconsole, "Can't Refresh Route!!\n" );
		}
	}
	else
	{
		if( !( m_Route[m_iRouteIndex].iType & bits_MF_IS_GOAL ) )
		{
			// If we've just passed a path_corner, advance m_pGoalEnt
			if( ( m_Route[m_iRouteIndex].iType & ~bits_MF_NOT_TO_MASK ) == bits_MF_TO_PATHCORNER )
				m_pGoalEnt = m_pGoalEnt->GetNextTarget();

			// IF both waypoints are nodes, then check for a link for a door and operate it.
			//
			if( ( m_Route[m_iRouteIndex].iType & bits_MF_TO_NODE ) == bits_MF_TO_NODE
			   && ( m_Route[m_iRouteIndex + 1].iType & bits_MF_TO_NODE ) == bits_MF_TO_NODE )
			{
				//ALERT( at_aiconsole, "SVD: Two nodes. " );

				int iSrcNode  = WorldGraph.FindNearestNode( m_Route[m_iRouteIndex].vecLocation, this );
				int iDestNode = WorldGraph.FindNearestNode( m_Route[m_iRouteIndex + 1].vecLocation, this );

				int iLink;
				WorldGraph.HashSearch( iSrcNode, iDestNode, iLink );

				if( iLink >= 0 && WorldGraph.m_pLinkPool[iLink].m_pLinkEnt != NULL )
				{
					//ALERT( at_aiconsole, "A link. " );
					if( WorldGraph.HandleLinkEnt( iSrcNode, WorldGraph.m_pLinkPool[iLink].m_pLinkEnt, m_afCapability, CGraph::NODEGRAPH_DYNAMIC ) )
					{
						//ALERT( at_aiconsole, "usable." );
						entvars_t *pevDoor = WorldGraph.m_pLinkPool[iLink].m_pLinkEnt;
						if( pevDoor )
						{
							m_flMoveWaitFinished = OpenDoorAndWait( pevDoor );
							//ALERT( at_aiconsole, "Wating for door %.2f\n", m_flMoveWaitFinished-gpGlobals->time );
						}
					}
				}
				//ALERT( at_aiconsole, "\n" );
			}
			m_iRouteIndex++;
		}
		else	// At goal!!!
		{
			if( distance < m_flGroundSpeed * 0.2f /* FIX */ )
			{
				MovementComplete();
			}
		}
	}
}

int CBaseMonster::RouteClassify( int iMoveFlag )
{
	int movementGoal;

	movementGoal = MOVEGOAL_NONE;

	if( iMoveFlag & bits_MF_TO_TARGETENT )
		movementGoal = MOVEGOAL_TARGETENT;
	else if( iMoveFlag & bits_MF_TO_ENEMY )
		movementGoal = MOVEGOAL_ENEMY;
	else if( iMoveFlag & bits_MF_TO_PATHCORNER )
		movementGoal = MOVEGOAL_PATHCORNER;
	else if( iMoveFlag & bits_MF_TO_NODE )
		movementGoal = MOVEGOAL_NODE;
	else if( iMoveFlag & bits_MF_TO_LOCATION )
		movementGoal = MOVEGOAL_LOCATION;

	return movementGoal;
}

//=========================================================
// BuildRoute
//=========================================================
BOOL CBaseMonster::BuildRoute( const Vector &vecGoal, int iMoveFlag, CBaseEntity *pTarget )
{
	float flDist;
	Vector vecApex;
	int iLocalMove;

	RouteNew();
	m_movementGoal = RouteClassify( iMoveFlag );

	// so we don't end up with no moveflags
	m_Route[0].vecLocation = vecGoal;
	m_Route[0].iType = iMoveFlag | bits_MF_IS_GOAL;

	// check simple local move
	iLocalMove = CheckLocalMove( pev->origin, vecGoal, pTarget, &flDist, 1 );

	if( iLocalMove == LOCALMOVE_VALID )
	{
		// monster can walk straight there!
		return TRUE;
	}

	// try to triangulate around any obstacles.
	else if( iLocalMove != LOCALMOVE_INVALID_DONT_TRIANGULATE && FTriangulate( pev->origin, vecGoal, flDist, pTarget, &vecApex ) )
	{
		// there is a slightly more complicated path that allows the monster to reach vecGoal
		m_Route[0].vecLocation = vecApex;
		m_Route[0].iType = (iMoveFlag | bits_MF_TO_DETOUR);

		m_Route[1].vecLocation = vecGoal;
		m_Route[1].iType = iMoveFlag | bits_MF_IS_GOAL;
			/*
			WRITE_BYTE( MSG_BROADCAST, SVC_TEMPENTITY );
			WRITE_BYTE( MSG_BROADCAST, TE_SHOWLINE );
			WRITE_COORD( MSG_BROADCAST, vecApex.x );
			WRITE_COORD( MSG_BROADCAST, vecApex.y );
			WRITE_COORD( MSG_BROADCAST, vecApex.z );
			WRITE_COORD( MSG_BROADCAST, vecApex.x );
			WRITE_COORD( MSG_BROADCAST, vecApex.y );
			WRITE_COORD( MSG_BROADCAST, vecApex.z + 128 );
			*/
		RouteSimplify( pTarget );
		return TRUE;
	}

	// last ditch, try nodes
	if( FGetNodeRoute( vecGoal ) )
	{
		//ALERT( at_console, "Can get there on nodes\n" );
		m_vecMoveGoal = vecGoal;
		RouteSimplify( pTarget );
		return TRUE;
	}

	// b0rk
	return FALSE;
}

//=========================================================
// InsertWaypoint - Rebuilds the existing route so that the
// supplied vector and moveflags are the first waypoint in
// the route, and fills the rest of the route with as much
// of the pre-existing route as possible
//=========================================================
void CBaseMonster::InsertWaypoint( Vector vecLocation, int afMoveFlags )
{
	int i, type;

	// we have to save some Index and Type information from the real
	// path_corner or node waypoint that the monster was trying to reach. This makes sure that data necessary 
	// to refresh the original path exists even in the new waypoints that don't correspond directy to a path_corner
	// or node. 
	type = afMoveFlags | ( m_Route[m_iRouteIndex].iType & ~bits_MF_NOT_TO_MASK );

	for( i = ROUTE_SIZE - 1; i > 0; i-- )
		m_Route[i] = m_Route[i - 1];

	m_Route[m_iRouteIndex].vecLocation = vecLocation;
	m_Route[m_iRouteIndex].iType = type;
}

//=========================================================
// FTriangulate - tries to overcome local obstacles by 
// triangulating a path around them.
//
// iApexDist is how far the obstruction that we are trying
// to triangulate around is from the monster.
//=========================================================
BOOL CBaseMonster::FTriangulate( const Vector &vecStart, const Vector &vecEnd, float flDist, CBaseEntity *pTargetEnt, Vector *pApex )
{
	Vector		vecDir;
	Vector		vecForward;
	Vector		vecLeft;// the spot we'll try to triangulate to on the left
	Vector		vecRight;// the spot we'll try to triangulate to on the right
	Vector		vecTop;// the spot we'll try to triangulate to on the top
	Vector		vecBottom;// the spot we'll try to triangulate to on the bottom
	Vector		vecFarSide;// the spot that we'll move to after hitting the triangulated point, before moving on to our normal goal.
	int		i;
	float		sizeX, sizeZ;

	// If the hull width is less than 24, use 24 because CheckLocalMove uses a min of
	// 24.
	sizeX = pev->size.x;
	if( sizeX < 24.0f )
		sizeX = 24.0f;
	else if( sizeX > 48.0f )
		sizeX = 48.0f;
	sizeZ = pev->size.z;
	//if( sizeZ < 24.0f )
	//	sizeZ = 24.0f;

	vecForward = ( vecEnd - vecStart ).Normalize();

	Vector vecDirUp( 0, 0, 1 );
	vecDir = CrossProduct( vecForward, vecDirUp );

	// start checking right about where the object is, picking two equidistant starting points, one on
	// the left, one on the right. As we progress through the loop, we'll push these away from the obstacle, 
	// hoping to find a way around on either side. pev->size.x is added to the ApexDist in order to help select
	// an apex point that insures that the monster is sufficiently past the obstacle before trying to turn back
	// onto its original course.

	vecLeft = pev->origin + ( vecForward * ( flDist + sizeX ) ) - vecDir * ( sizeX * 3 );
	vecRight = pev->origin + ( vecForward * ( flDist + sizeX ) ) + vecDir * ( sizeX * 3 );
	if( pev->movetype == MOVETYPE_FLY )
	{
		vecTop = pev->origin + ( vecForward * flDist ) + ( vecDirUp * sizeZ * 3 );
		vecBottom = pev->origin + ( vecForward * flDist ) - ( vecDirUp *  sizeZ * 3 );
	}

	vecFarSide = m_Route[m_iRouteIndex].vecLocation;

	vecDir = vecDir * sizeX * 2;
	if( pev->movetype == MOVETYPE_FLY )
		vecDirUp = vecDirUp * sizeZ * 2;

	for( i = 0; i < 8; i++ )
	{
// Debug, Draw the triangulation
#if 0
		MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
			WRITE_BYTE( TE_SHOWLINE);
			WRITE_COORD( pev->origin.x );
			WRITE_COORD( pev->origin.y );
			WRITE_COORD( pev->origin.z );
			WRITE_COORD( vecRight.x );
			WRITE_COORD( vecRight.y );
			WRITE_COORD( vecRight.z );
		MESSAGE_END();

		MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
			WRITE_BYTE( TE_SHOWLINE );
			WRITE_COORD( pev->origin.x );
			WRITE_COORD( pev->origin.y );
			WRITE_COORD( pev->origin.z );
			WRITE_COORD( vecLeft.x );
			WRITE_COORD( vecLeft.y );
			WRITE_COORD( vecLeft.z );
		MESSAGE_END();
#endif
#if 0
		if( pev->movetype == MOVETYPE_FLY )
		{
			MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
				WRITE_BYTE( TE_SHOWLINE );
				WRITE_COORD( pev->origin.x );
				WRITE_COORD( pev->origin.y );
				WRITE_COORD( pev->origin.z );
				WRITE_COORD( vecTop.x );
				WRITE_COORD( vecTop.y );
				WRITE_COORD( vecTop.z );
			MESSAGE_END();

			MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
				WRITE_BYTE( TE_SHOWLINE );
				WRITE_COORD( pev->origin.x );
				WRITE_COORD( pev->origin.y );
				WRITE_COORD( pev->origin.z );
				WRITE_COORD( vecBottom.x );
				WRITE_COORD( vecBottom.y );
				WRITE_COORD( vecBottom.z );
			MESSAGE_END();
		}
#endif
		if( CheckLocalMove( pev->origin, vecRight, pTargetEnt, NULL, 0 ) == LOCALMOVE_VALID )
		{
			if( CheckLocalMove( vecRight, vecFarSide, pTargetEnt, NULL, 0 ) == LOCALMOVE_VALID )
			{
				if( pApex )
				{
					*pApex = vecRight;
				}

				return TRUE;
			}
		}
		if( CheckLocalMove( pev->origin, vecLeft, pTargetEnt, NULL, 0 ) == LOCALMOVE_VALID )
		{
			if( CheckLocalMove( vecLeft, vecFarSide, pTargetEnt, NULL, 0 ) == LOCALMOVE_VALID )
			{
				if( pApex )
				{
					*pApex = vecLeft;
				}

				return TRUE;
			}
		}

		if( pev->movetype == MOVETYPE_FLY )
		{
			if( CheckLocalMove( pev->origin, vecTop, pTargetEnt, NULL, 0 ) == LOCALMOVE_VALID)
			{
				if( CheckLocalMove ( vecTop, vecFarSide, pTargetEnt, NULL, 0 ) == LOCALMOVE_VALID )
				{
					if( pApex )
					{
						*pApex = vecTop;
						//ALERT(at_aiconsole, "triangulate over\n");
					}

					return TRUE;
				}
			}
#if 1
			if( CheckLocalMove( pev->origin, vecBottom, pTargetEnt, NULL, 0 ) == LOCALMOVE_VALID )
			{
				if( CheckLocalMove( vecBottom, vecFarSide, pTargetEnt, NULL, 0 ) == LOCALMOVE_VALID )
				{
					if( pApex )
					{
						*pApex = vecBottom;
						//ALERT(at_aiconsole, "triangulate under\n");
					}

					return TRUE;
				}
			}
#endif
		}

		vecRight = vecRight + vecDir;
		vecLeft = vecLeft - vecDir;
		if( pev->movetype == MOVETYPE_FLY )
		{
			vecTop = vecTop + vecDirUp;
			vecBottom = vecBottom - vecDirUp;
		}
	}

	return FALSE;
}

//=========================================================
// Move - take a single step towards the next ROUTE location
//=========================================================
#define DIST_TO_CHECK	200

void CBaseMonster::Move( float flInterval ) 
{
	float		flWaypointDist;
	float		flCheckDist;
	float		flDist;// how far the lookahead check got before hitting an object.
	Vector		vecDir;
	Vector		vecApex;
	CBaseEntity	*pTargetEnt;

	// Don't move if no valid route
	if( FRouteClear() )
	{
		// If we still have a movement goal, then this is probably a route truncated by SimplifyRoute()
		// so refresh it.
		if( m_movementGoal == MOVEGOAL_NONE || !FRefreshRoute() )
		{
			ALERT( at_aiconsole, "Tried to move with no route!\n" );
			TaskFail();
			return;
		}
	}

	if( m_flMoveWaitFinished > gpGlobals->time || m_playerguardian_mode >= 1 )
		return;

// Debug, test movement code
#if 0
//	if( CVAR_GET_FLOAT("stopmove" ) != 0 )
	{
		if( m_movementGoal == MOVEGOAL_ENEMY )
			RouteSimplify( m_hEnemy );
		else
			RouteSimplify( m_hTargetEnt );
		FRefreshRoute();
		return;
	}
#else
// Debug, draw the route
//	DrawRoute( pev, m_Route, m_iRouteIndex, 0, 200, 0 );
#endif

	if(m_MoveFail_FuckRoad)
	{
		if(m_MoveFail_FuckRoad_Mode == 1)
		{
			if(m_MoveFailNum >= 50)
			{
				m_MoveFailNum = 0;
				TaskFail();
				return;
			}
		}
		else if(m_MoveFailNum >= 30)
		{
			m_MoveFailNum = 0;
			TaskFail();
			return;
		}
	}
	// if the monster is moving directly towards an entity (enemy for instance), we'll set this pointer
	// to that entity for the CheckLocalMove and Triangulate functions.
	pTargetEnt = NULL;

	// local move to waypoint.
	vecDir = ( m_Route[m_iRouteIndex].vecLocation - pev->origin ).Normalize();
	flWaypointDist = ( m_Route[m_iRouteIndex].vecLocation - pev->origin ).Length2D();

	MakeIdealYaw( m_Route[m_iRouteIndex].vecLocation );
	ChangeYaw( pev->yaw_speed );

	// if the waypoint is closer than CheckDist, CheckDist is the dist to waypoint
	if( flWaypointDist < DIST_TO_CHECK )
	{
		flCheckDist = flWaypointDist;
	}
	else
	{
		flCheckDist = DIST_TO_CHECK;
	}

	if( ( m_Route[m_iRouteIndex].iType & ( ~bits_MF_NOT_TO_MASK ) ) == bits_MF_TO_ENEMY )
	{
		// only on a PURE move to enemy ( i.e., ONLY MF_TO_ENEMY set, not MF_TO_ENEMY and DETOUR )
		pTargetEnt = m_hEnemy;
	}
	else if( ( m_Route[m_iRouteIndex].iType & ~bits_MF_NOT_TO_MASK ) == bits_MF_TO_TARGETENT )
	{
		pTargetEnt = m_hTargetEnt;
	}

	// !!!BUGBUG - CheckDist should be derived from ground speed.
	// If this fails, it should be because of some dynamic entity blocking this guy.
	// We've already checked this path, so we should wait and time out if the entity doesn't move
	flDist = 0;
	if( CheckLocalMove( pev->origin, pev->origin + vecDir * flCheckDist, pTargetEnt, &flDist, 1 ) != LOCALMOVE_VALID )
	{
		CBaseEntity *pBlocker;

		// Can't move, stop
		Stop();

		// Blocking entity is in global trace_ent
		pBlocker = CBaseEntity::Instance( gpGlobals->trace_ent );
		if( pBlocker )
		{
			DispatchBlocked( edict(), pBlocker->edict() );
		}

		if( pBlocker && m_moveWaitTime > 0 && pBlocker->IsMoving() && !pBlocker->IsPlayer() && ( gpGlobals->time-m_flMoveWaitFinished ) > 3.0f )
		{
			// Can we still move toward our target?
			if( flDist < m_flGroundSpeed )
			{
				// No, Wait for a second
				m_flMoveWaitFinished = gpGlobals->time + m_moveWaitTime;
				return;
			}
			// Ok, still enough room to take a step
		}
		else 
		{
			// try to triangulate around whatever is in the way.
			if( FTriangulate( pev->origin, m_Route[m_iRouteIndex].vecLocation, flDist, pTargetEnt, &vecApex ) )
			{
				InsertWaypoint( vecApex, bits_MF_TO_DETOUR );
				RouteSimplify( pTargetEnt );
			}
			else
			{
				//ALERT( at_aiconsole, "Couldn't Triangulate\n" );
				Stop();

				// Only do this once until your route is cleared
				if( m_moveWaitTime > 0 && !( m_afMemory & bits_MEMORY_MOVE_FAILED ) )
				{
					FRefreshRoute();
					if( FRouteClear() )
					{
						TaskFail();
					}
					else
					{
						// Don't get stuck
						if( ( gpGlobals->time - m_flMoveWaitFinished ) < 0.2f )
							Remember( bits_MEMORY_MOVE_FAILED );

						m_flMoveWaitFinished = gpGlobals->time + 0.1f;
					}
				}
				else
				{
					if(m_MoveFail_SimpleRoad)
					{
						if ( m_movementGoal == MOVEGOAL_ENEMY )
						{
							if(m_hEnemy != NULL && m_hEnemy->IsAlive())
							{
								RouteSimplify( m_hEnemy );
								FRefreshRoute();
								return;
							}
						}
						else if ( m_movementGoal == MOVEGOAL_TARGETENT )
						{
							if(m_hTargetEnt != NULL && m_hTargetEnt->IsAlive())
							{
								RouteSimplify( m_hTargetEnt );
								FRefreshRoute();
								return;
							}
						}
					}
					TaskFail();
					//ALERT( at_aiconsole, "%s Failed to move (%d)!\n", STRING( pev->classname ), HasMemory( bits_MEMORY_MOVE_FAILED ) );
					//ALERT( at_aiconsole, "%f, %f, %f\n", pev->origin.z, ( pev->origin + ( vecDir * flCheckDist ) ).z, m_Route[m_iRouteIndex].vecLocation.z );
				}
				return;
			}
		}
	}

	// close enough to the target, now advance to the next target. This is done before actually reaching
	// the target so that we get a nice natural turn while moving.
	if( ShouldAdvanceRoute( flWaypointDist ) )///!!!BUGBUG- magic number
	{
		AdvanceRoute( flWaypointDist );
	}

	// Might be waiting for a door
	if( m_flMoveWaitFinished > gpGlobals->time )
	{
		Stop();
		return;
	}

	// UNDONE: this is a hack to quit moving farther than it has looked ahead.
	if( flCheckDist < m_flGroundSpeed * flInterval )
	{
		flInterval = flCheckDist / m_flGroundSpeed;
		// ALERT( at_console, "%.02f\n", flInterval );
	}
	MoveExecute( pTargetEnt, vecDir, flInterval );

	if( MovementIsComplete() )
	{
		Stop();
		RouteClear();
	}
}

BOOL CBaseMonster::ShouldAdvanceRoute( float flWaypointDist )
{
	if( flWaypointDist <= MONSTER_CUT_CORNER_DIST )
	{
		// ALERT( at_console, "cut %f\n", flWaypointDist );
		return TRUE;
	}

	return FALSE;
}

void CBaseMonster::MoveExecute( CBaseEntity *pTargetEnt, const Vector &vecDir, float flInterval )
{
	//float flYaw = UTIL_VecToYaw( m_Route[m_iRouteIndex].vecLocation - pev->origin );// build a yaw that points to the goal.
	//WALK_MOVE( ENT( pev ), flYaw, m_flGroundSpeed * flInterval, WALKMOVE_NORMAL );
	if( m_IdealActivity != m_movementActivity )
		m_IdealActivity = m_movementActivity;

	float flTotal = m_flGroundSpeed * pev->framerate * flInterval;
	float flStep;
	while( flTotal > 0.001f )
	{
		// don't walk more than 16 units or stairs stop working
		flStep = Q_min( 16.0f, flTotal );
		UTIL_MoveToOrigin( ENT( pev ), m_Route[m_iRouteIndex].vecLocation, flStep, MOVE_NORMAL );
		flTotal -= flStep;
	}
	// ALERT( at_console, "dist %f\n", m_flGroundSpeed * pev->framerate * flInterval );
}

//=========================================================
// MonsterInit - after a monster is spawned, it needs to 
// be dropped into the world, checked for mobility problems,
// and put on the proper path, if any. This function does
// all of those things after the monster spawns. Any
// initialization that should take place for all monsters
// goes here.
//=========================================================
void CBaseMonster::MonsterInit( void )
{
	/*if( !g_pGameRules->FAllowMonsters() )
	{
		pev->flags |= FL_KILLME;		// Post this because some monster code modifies class data after calling this function
		//REMOVE_ENTITY( ENT( pev ) );
		return;
	}*/

	// Set fields common to all monsters
	pev->effects		= 0;
	pev->friction		= 0.7f;
	pev->takedamage		= DAMAGE_AIM;
	pev->ideal_yaw		= pev->angles.y;
	pev->max_health		= pev->health;
	pev->deadflag		= DEAD_NO;

	if(pev->impulse == 1426)
	{
		m_IdealMonsterState	= MONSTERSTATE_HUNT;
		ResetSequenceInfo( );
		pev->frame = 0;
	}
	else
	{
		m_IdealActivity = ACT_IDLE;
		m_IdealMonsterState	= MONSTERSTATE_IDLE;// Assume monster will be idle, until proven otherwise
	}

	SetBits( pev->flags, FL_MONSTER );
	if( pev->spawnflags & SF_MONSTER_HITMONSTERCLIP )
		pev->flags |= FL_MONSTERCLIP;

	ClearSchedule();
	RouteClear();
	InitBoneControllers( ); // FIX: should be done in Spawn

	m_iHintNode = NO_NODE;

	m_afMemory = MEMORY_CLEAR;

	m_hEnemy = NULL;

	m_flDistTooFar = 1024.0f;
	m_flDistLook = 2048.0f;

	m_hHitEnemy			= NULL;

	m_hTeamMate1		= NULL;
	m_hTeamMate2		= NULL;
	m_hTeamMate3		= NULL;
	m_hTeamMate4		= NULL;

	m_noidleseq			= FALSE;

	m_flDistTooFar		= 2560.0;
	m_flDistLook		= 4096.0;

	m_aimflag_dist      = 40.0;

	m_fightmode			= 0;
	m_die_for_back      = 0;

	m_ignoreFail_MAX    = 0;
	m_ignoreFail_OFF	= 1;

	m_groundElev		= FALSE;
	m_groundElev2		= FALSE;

	m_MoveFail_SimpleRoad = FALSE;

	m_forcefuckdoor     = FALSE;
	m_MoveFail_FuckRoad = FALSE;
	m_MoveFail_FuckRoad_Mode = 0;

	m_flPlayerDamage_exp = 0;

	m_barnacleres_time  = 0;
	m_freeze_def = 0;
	m_killedbydmg = 0;

	m_follow_mode		= 0;
	m_killed_exp		= 0;
	m_die				= 0;
	m_dieseq			= 0;
	m_cover_dist		= 256;
	m_attack_dist		= 256;
	m_playerguardian_mode = 0;

	m_rpgms_skill1_learn = 15;
	m_rpgms_skill2_learn = 15;
	m_rpgms_skill3_learn = 15;
	m_rpgms_skill4_learn = 15;
	m_rpgms_skill5_learn = 15;
	m_rpgms_skill6_learn = 15;
	m_rpgms_skill7_learn = 15;
	m_rpgms_skill8_learn = 15;
	m_rpgms_skill9_learn = 15;
	m_rpgms_skill10_learn = 15;
	m_rpgms_skill11_learn = 15;
	m_rpgms_skill12_learn = 15;

	if(m_singdelay_max == 0)
	{
		m_singdelay_max = 2;
		m_singdelay_use = m_singdelay_max;
	}

	// set eye position
	SetEyePosition();

	SetThink( &CBaseMonster::MonsterInitThink );
	pev->nextthink = gpGlobals->time + 0.1f;
	SetUse( &CBaseMonster::MonsterUse );

	if (FClassnameIs(pev, "monster_tyant_boss"))
	{
		pev->nextthink = gpGlobals->time + 1.2;
	}

	if ( !FNullEnt(pev->owner))
	{
		if(VARS(pev->owner)->flags & FL_MONSTER)
		{
			if (FClassnameIs(pev, "monster_zombie") || FClassnameIs(pev, "monster_zombie_barney") ||
			FClassnameIs(pev, "monster_zombie_soldier"))
			{
				pev->nextthink = gpGlobals->time + 1.2;
				pev->owner = NULL;
			}
		}
	}
}

//=========================================================
// MonsterInitThink - Calls StartMonster. Startmonster is 
// virtual, but this function cannot be 
//=========================================================
void CBaseMonster::MonsterInitThink( void )
{
	if(m_fightmode)
	{

		MESSAGE_BEGIN(MSG_BROADCAST, SVC_TEMPENTITY,pev->origin);
			WRITE_BYTE(3);
			WRITE_COORD( pev->origin.x );
			WRITE_COORD( pev->origin.y );
			WRITE_COORD( pev->origin.z);
			WRITE_SHORT(g_sModelIndexCteleport);
			WRITE_BYTE(15);
			WRITE_BYTE(15);
			WRITE_BYTE(4);
		MESSAGE_END();
						
		EMIT_SOUND_DYN( ENT(pev), CHAN_AUTO, "debris/beamstart2old.wav", 1, ATTN_NORM, 0, 100 );
		m_diefadeout = 1;
		m_undropgun = TRUE;
	}

	StartMonster();
}

//=========================================================
// StartMonster - final bit of initization before a monster 
// is turned over to the AI. 
//=========================================================
void CBaseMonster::StartMonster( void )
{
	// update capabilities
	if( LookupActivity( ACT_RANGE_ATTACK1 ) != ACTIVITY_NOT_AVAILABLE )
	{
		m_afCapability |= bits_CAP_RANGE_ATTACK1;
	}
	if( LookupActivity( ACT_RANGE_ATTACK2 ) != ACTIVITY_NOT_AVAILABLE )
	{
		m_afCapability |= bits_CAP_RANGE_ATTACK2;
	}
	if( LookupActivity( ACT_MELEE_ATTACK1 ) != ACTIVITY_NOT_AVAILABLE )
	{
		m_afCapability |= bits_CAP_MELEE_ATTACK1;
	}
	if( LookupActivity( ACT_MELEE_ATTACK2 ) != ACTIVITY_NOT_AVAILABLE )
	{
		m_afCapability |= bits_CAP_MELEE_ATTACK2;
	}

	// Raise monster off the floor one unit, then drop to floor
	if( pev->movetype != MOVETYPE_FLY && !FBitSet( pev->spawnflags, SF_MONSTER_FALL_TO_GROUND ) )
	{
		pev->velocity.z -= 1;
		/*DROP_TO_FLOOR( ENT( pev ) );

		// Try to move the monster to make sure it's not stuck in a brush.
		if( !WALK_MOVE( ENT( pev ), 0, 0, WALKMOVE_NORMAL ) )
		{
			ALERT( at_error, "Monster %s stuck in wall--level design error\n", STRING( pev->classname ) );

			if( g_psv_developer && g_psv_developer->value )
				pev->effects = EF_BRIGHTFIELD;
		}*/
	}
	else 
	{
		pev->flags &= ~FL_ONGROUND;
	}
	
	if( !FStringNull( pev->target ) )// this monster has a target
	{
		// Find the monster's initial target entity, stash it
		m_pGoalEnt = CBaseEntity::Instance( FIND_ENTITY_BY_TARGETNAME( NULL, STRING( pev->target ) ) );

		if( !m_pGoalEnt )
		{
			ALERT( at_error, "ReadyMonster()--%s couldn't find target %s\n", STRING( pev->classname ), STRING( pev->target ) );
		}
		else
		{
			// Monster will start turning towards his destination
			MakeIdealYaw( m_pGoalEnt->pev->origin );

			// JAY: How important is this error message?  Big Momma doesn't obey this rule, so I took it out.
#if 0
			// At this point, we expect only a path_corner as initial goal
			if( !FClassnameIs( m_pGoalEnt->pev, "path_corner" ) )
			{
				ALERT( at_warning, "ReadyMonster--monster's initial goal '%s' is not a path_corner\n", STRING( pev->target ) );
			}
#endif
			// set the monster up to walk a path corner path. 
			// !!!BUGBUG - this is a minor bit of a hack.
			// JAYJAY
			m_movementGoal = MOVEGOAL_PATHCORNER;

			if( pev->movetype == MOVETYPE_FLY )
				m_movementActivity = ACT_FLY;
			else
				m_movementActivity = ACT_WALK;

			if( !FRefreshRoute() )
			{
				ALERT( at_aiconsole, "Can't Create Route!\n" );
			}
			SetState( MONSTERSTATE_IDLE );
			ChangeSchedule( GetScheduleOfType( SCHED_IDLE_WALK ) );
		}
	}

	//SetState( m_IdealMonsterState );
	//SetActivity( m_IdealActivity );

	Use_PathWalk();

	// Delay drop to floor to make sure each door in the level has had its chance to spawn
	// Spread think times so that they don't all happen at the same time (Carmack)
	SetThink( &CBaseMonster::CallMonsterThink );
	pev->nextthink += RANDOM_FLOAT( 0.1f, 0.3f ); // spread think times.

	if( pev->impulse != 1426 && !FStringNull(pev->targetname) )// wait until triggered
	{
		SetState( MONSTERSTATE_IDLE );
		// UNDONE: Some scripted sequence monsters don't have an idle?
		SetActivity( ACT_IDLE );
		ChangeSchedule( GetScheduleOfType( SCHED_WAIT_TRIGGER ) );
	}
}

void CBaseMonster :: Use_PathWalk( void ) 
{
	if ( !FStringNull(pev->target) )// this monster has a target
	{
		// Find the monster's initial target entity, stash it
		m_pGoalEnt = CBaseEntity::Instance( FIND_ENTITY_BY_TARGETNAME ( NULL, STRING( pev->target ) ) );

		if ( !m_pGoalEnt )
		{
			ALERT(at_error, "ReadyMonster()--%s couldn't find target %s", STRING(pev->classname), STRING(pev->target));
		}
		else
		{
			// Monster will start turning towards his destination
			MakeIdealYaw ( m_pGoalEnt->pev->origin );

			// JAY: How important is this error message?  Big Momma doesn't obey this rule, so I took it out.

			// set the monster up to walk a path corner path. 
			// !!!BUGBUG - this is a minor bit of a hack.
			// JAYJAY
			m_movementGoal = MOVEGOAL_PATHCORNER;
					
			if ( pev->movetype == MOVETYPE_FLY )
			{
				m_movementActivity = ACT_FLY;
			}
			else if ( LookupActivity( ACT_RUN ) != ACTIVITY_NOT_AVAILABLE && m_alwaysrunpath )
			{
				m_movementActivity = ACT_RUN;
			}
			else
			{
				m_movementActivity = ACT_WALK;
			}

			if ( !FRefreshRoute() )
			{
				ALERT ( at_aiconsole, "Can't Create Route!\n" );
			}

			m_pathtrack_reuse = 1;

			m_alert = 0;
			m_hEnemy = NULL;
			m_hOldEnemy[0] = NULL;
			m_hOldEnemy[1] = NULL;
			m_hOldEnemy[2] = NULL;
			m_hOldEnemy[3] = NULL;

			SetState( MONSTERSTATE_IDLE );
			ChangeSchedule( GetScheduleOfType( SCHED_IDLE_WALK ) );
		}
	}
}

void CBaseMonster::MovementComplete( void ) 
{ 
	switch( m_iTaskStatus )
	{
	case TASKSTATUS_NEW:
	case TASKSTATUS_RUNNING:
		m_iTaskStatus = TASKSTATUS_RUNNING_TASK;
		break;
	case TASKSTATUS_RUNNING_MOVEMENT:
		TaskComplete();
		break;
	case TASKSTATUS_RUNNING_TASK:
		ALERT( at_error, "Movement completed twice!\n" );
		break;
	case TASKSTATUS_COMPLETE:		
		break;
	}
	m_movementGoal = MOVEGOAL_NONE;
}

int CBaseMonster::TaskIsRunning( void )
{
	if( m_iTaskStatus != TASKSTATUS_COMPLETE && 
		 m_iTaskStatus != TASKSTATUS_RUNNING_MOVEMENT )
		 return 1;

	return 0;
}

//=========================================================
// IRelationship - returns an integer that describes the 
// relationship between two types of monster.
//=========================================================
int CBaseMonster::IRelationship( CBaseEntity *pTarget )
{
	if(Classify() == pTarget->Classify())
		return R_NO;
	
	if(m_ignorePlayer > 0 && pTarget->Classify() == CLASS_PLAYER)
		return R_NO;
	

	if(Classify() == CLASS_ALIEN_PREY)
	{
		if(pTarget->Classify() == CLASS_HUMAN_ASS 
		|| pTarget->Classify() == CLASS_HUMAN_MILITARY
		|| pTarget->Classify() == CLASS_ALIEN_MONSTER 
		|| pTarget->Classify() == CLASS_ALIEN_MILITARY
		|| pTarget->Classify() == CLASS_PLAYER
		|| pTarget->Classify() == CLASS_PLAYER_ALLY
		|| pTarget->Classify() == CLASS_HUMAN_PASSIVE)
		{
			return R_DL;
		}
	}
	if(Classify() == CLASS_MACHINE_BLACK)
	{
		if(pTarget->Classify() == CLASS_HUMAN_ASS 
		|| pTarget->Classify() == CLASS_HUMAN_MILITARY
		|| pTarget->Classify() == CLASS_ALIEN_MONSTER 
		|| pTarget->Classify() == CLASS_ALIEN_MILITARY
		|| pTarget->Classify() == CLASS_MACHINE 
		|| pTarget->Classify() == CLASS_MACHINE_ASS
		|| pTarget->Classify() == CLASS_HUMAN_PASSIVE)
		{
			return R_DL;
		}
	}

	if(Classify() == CLASS_HUMAN_PASSIVE)
	{
		if (FClassnameIs( pTarget->pev, "npc_attack_flag" ) && pTarget->pev->frags == 5)
		{
			return R_NM;
		}
		if(pTarget->Classify() == CLASS_HUMAN_ASS 
		|| pTarget->Classify() == CLASS_HUMAN_MILITARY
		|| pTarget->Classify() == CLASS_ALIEN_MONSTER 
		|| pTarget->Classify() == CLASS_ALIEN_MILITARY
		|| pTarget->Classify() == CLASS_MACHINE 
		|| pTarget->Classify() == CLASS_MACHINE_ASS)
		{
			return R_HT;
		}
		if(pTarget->Classify() == CLASS_ALIEN_PREY )
		{
			return R_DL;
		}
		if (FClassnameIs( pTarget->pev, "npc_aim_flag" ) && pTarget->pev->frags == 5)
		{
			return R_DL;
		}
		if(pTarget->Classify() == CLASS_PLAYER || pTarget->Classify() == CLASS_PLAYER_ALLY)
		{
			return R_AL;
		}
	}

	if(Classify() == CLASS_PLAYER_ALLY)
	{
		if (m_enemyfollower == 1 && (FClassnameIs( pTarget->pev, "player_aim_flag" ) || FClassnameIs( pTarget->pev, "player_attack_flag" )) )
		{
			return R_NM;
		}
		if(pTarget->Classify() == CLASS_HUMAN_ASS 
		|| pTarget->Classify() == CLASS_HUMAN_MILITARY
		|| pTarget->Classify() == CLASS_ALIEN_MONSTER 
		|| pTarget->Classify() == CLASS_ALIEN_MILITARY
		|| pTarget->Classify() == CLASS_MACHINE 
		|| pTarget->Classify() == CLASS_MACHINE_ASS)
		{
			return R_HT;
		}
		if (FClassnameIs( pTarget->pev, "npc_attack_flag" ) && pTarget->pev->frags >= 5)
		{
			return R_HT;
		}
		if(pTarget->Classify() == CLASS_ALIEN_PREY )
		{
			return R_DL;
		}
		if (FClassnameIs( pTarget->pev, "npc_aim_flag" ) && (pTarget->pev->frags >= 5 || pTarget->pev->frags == 3) )
		{
			return R_DL;
		}
		if (pTarget->Classify() == CLASS_PLAYER && m_lovehate <= 0)
		{
			if(m_enemyfollower == 1)
			{
				m_enemyfollower = 0;
			}
			else
			{
				return R_HT;
			}
		}

		//pTarget->IsPlayer()
		if ( (pTarget->Classify() == CLASS_PLAYER && m_enemyfollower == 1 && m_enemyfollower_combat == 0)
		|| (pTarget->Classify() == CLASS_PLAYER && m_playerguardian_mode == 1 && m_follow_mode == 1) )
		{
			return R_DL;
		}

		if(pTarget->Classify() == CLASS_PLAYER)
		{
			return R_AL;
		}
	}

	if(Classify() == CLASS_HUMAN_ASS || Classify() == CLASS_MACHINE_ASS)
	{
		if (FClassnameIs( pTarget->pev, "npc_attack_flag" ) && pTarget->pev->frags == 4)
		{
			return R_NM;
		}
		if(m_fightmode >= 2 && (pTarget->Classify() == CLASS_ALIEN_MILITARY || pTarget->Classify() == CLASS_ALIEN_MONSTER) )
		{
			return R_NO;
		}
		if(pTarget->Classify() == CLASS_PLAYER
		|| pTarget->Classify() == CLASS_PLAYER_ALLY
		|| pTarget->Classify() == CLASS_HUMAN_MILITARY
		|| pTarget->Classify() == CLASS_ALIEN_MONSTER 
		|| pTarget->Classify() == CLASS_ALIEN_MILITARY
		|| pTarget->Classify() == CLASS_MACHINE 
		|| pTarget->Classify() == CLASS_MACHINE_BLACK
		|| pTarget->Classify() == CLASS_HUMAN_PASSIVE
		|| pTarget->Classify() == CLASS_HUMAN_BIOWEAPON)
		{
			return R_HT;
		}
		if(pTarget->Classify() == CLASS_ALIEN_PREY )
		{
			return R_HT;
		}
		if (FClassnameIs( pTarget->pev, "npc_aim_flag" ) && pTarget->pev->frags == 4)
		{
			return R_DL;
		}
	}

	if(Classify() == CLASS_HUMAN_MILITARY || Classify() == CLASS_MACHINE)
	{
		if (FClassnameIs( pTarget->pev, "npc_attack_flag" ) && pTarget->pev->frags == 2)
		{
			return R_NM;
		}
		if(pTarget->Classify() == CLASS_HUMAN_ASS
		|| pTarget->Classify() == CLASS_MACHINE_ASS
		|| pTarget->Classify() == CLASS_ALIEN_MILITARY)
		{
			return R_NM;
		}
		if(pTarget->Classify() == CLASS_PLAYER
		|| pTarget->Classify() == CLASS_PLAYER_ALLY
		|| pTarget->Classify() == CLASS_MACHINE_BLACK
		|| pTarget->Classify() == CLASS_ALIEN_PREY 
		|| pTarget->Classify() == CLASS_ALIEN_MONSTER 
		|| pTarget->Classify() == CLASS_HUMAN_PASSIVE
		|| pTarget->Classify() == CLASS_HUMAN_BIOWEAPON)
		{
			return R_HT;
		}
		if(pTarget->Classify() == CLASS_PLAYER_BIOWEAPON 
		|| pTarget->Classify() == CLASS_ALIEN_BIOWEAPON)
		{
			return R_DL;
		}
		if (FClassnameIs( pTarget->pev, "npc_aim_flag" ) && pTarget->pev->frags == 2)
		{
			if(m_elseuseful == 2 || m_elseuseful == 3)
			{
				return R_NM;
			}
			else
			{
				return R_DL;
			}
		}
	}
	if(Classify() == CLASS_PLAYER)
	{
		return R_AL;
	}
	if(Classify() == CLASS_ALIEN_MONSTER || Classify() == CLASS_ALIEN_MILITARY)
	{
		if (FClassnameIs( pTarget->pev, "npc_attack_flag" ) && pTarget->pev->frags == 1)
		{
			return R_NM;
		}
		if (FClassnameIs( pTarget->pev, "npc_aim_flag" ) && pTarget->pev->frags == 1)
		{
			return R_DL;
		}
		if(m_fightmode >= 2 && pTarget->Classify() == CLASS_HUMAN_ASS)
		{
			return R_NO;
		}
		if(pTarget->Classify() == CLASS_PLAYER
		|| pTarget->Classify() == CLASS_PLAYER_ALLY
		|| pTarget->Classify() == CLASS_MACHINE
		|| pTarget->Classify() == CLASS_MACHINE_BLACK
		|| pTarget->Classify() == CLASS_MACHINE_ASS
		|| pTarget->Classify() == CLASS_HUMAN_BIOWEAPON)
		{
			return R_HT;
		}
		if(pTarget->Classify() == CLASS_HUMAN_ASS
		|| pTarget->Classify() == CLASS_HUMAN_MILITARY 
		|| pTarget->Classify() == CLASS_HUMAN_PASSIVE)\
		{
			return R_NM;
		}
	}
	if(Classify() == CLASS_INSECT)
	{
		if(pTarget->Classify() == CLASS_PLAYER_BIOWEAPON 
		|| pTarget->Classify() == CLASS_ALIEN_BIOWEAPON)
		{
			return R_NO;
		}
		return R_FR;
	}
	if(Classify() == CLASS_PLAYER_BIOWEAPON )
	{
		if(pTarget->Classify() == CLASS_HUMAN_ASS
		|| pTarget->Classify() == CLASS_HUMAN_MILITARY
		|| pTarget->Classify() == CLASS_ALIEN_MONSTER 
		|| pTarget->Classify() == CLASS_ALIEN_MILITARY
		|| pTarget->Classify() == CLASS_MACHINE
		|| pTarget->Classify() == CLASS_MACHINE_BLACK
		|| pTarget->Classify() == CLASS_MACHINE_ASS)
		{
			return R_HT;
		}
		if(pTarget->Classify() == CLASS_PLAYER_ALLY
		|| pTarget->Classify() == CLASS_HUMAN_PASSIVE
		|| pTarget->Classify() == CLASS_PLAYER)
		{
			return R_DL;
		}
	}
	if(Classify() == CLASS_ALIEN_BIOWEAPON )
	{
		if(pTarget->Classify() == CLASS_HUMAN_ASS
		|| pTarget->Classify() == CLASS_HUMAN_MILITARY
		|| pTarget->Classify() == CLASS_PLAYER 
		|| pTarget->Classify() == CLASS_PLAYER_ALLY
		|| pTarget->Classify() == CLASS_MACHINE
		|| pTarget->Classify() == CLASS_MACHINE_BLACK
		|| pTarget->Classify() == CLASS_MACHINE_ASS
		|| pTarget->Classify() == CLASS_HUMAN_PASSIVE
		|| pTarget->Classify() == CLASS_HUMAN_BIOWEAPON)
		{
			return R_HT;
		}
	}
	if(Classify() == CLASS_HUMAN_BIOWEAPON )
	{
		
		if(pTarget->Classify() == CLASS_HUMAN_ASS
		|| pTarget->Classify() == CLASS_HUMAN_MILITARY
		|| pTarget->Classify() == CLASS_ALIEN_MONSTER 
		|| pTarget->Classify() == CLASS_ALIEN_MILITARY
		|| pTarget->Classify() == CLASS_MACHINE
		|| pTarget->Classify() == CLASS_MACHINE_BLACK
		|| pTarget->Classify() == CLASS_MACHINE_ASS)
		{
			return R_HT;
		}
	}
	return R_NO;
}

//=========================================================
// FindCover - tries to find a nearby node that will hide
// the caller from its enemy. 
//
// If supplied, search will return a node at least as far
// away as MinDist, but no farther than MaxDist. 
// if MaxDist isn't supplied, it defaults to a reasonable 
// value
//=========================================================
// UNDONE: Should this find the nearest node?

//float CGraph::PathLength( int iStart, int iDest, int iHull, int afCapMask )

BOOL CBaseMonster::FindCover( Vector vecThreat, Vector vecViewOffset, float flMinDist, float flMaxDist )
{
	int i;
	int iMyHullIndex;
	int iMyNode;
	int iThreatNode;
	float flDist;
	Vector vecLookersOffset;
	TraceResult tr;

	if( !flMaxDist )
	{
		// user didn't supply a MaxDist, so work up a crazy one.
		flMaxDist = 784;
	}

	if( flMinDist > 0.5f * flMaxDist )
	{
#if _DEBUG
		ALERT( at_console, "FindCover MinDist (%.0f) too close to MaxDist (%.0f)\n", flMinDist, flMaxDist );
#endif
		flMinDist = 0.5f * flMaxDist;
	}

	if( !WorldGraph.m_fGraphPresent || !WorldGraph.m_fGraphPointersSet )
	{
		ALERT( at_aiconsole, "Graph not ready for findcover!\n" );
		return FALSE;
	}

	iMyNode = WorldGraph.FindNearestNode( pev->origin, this );
	iThreatNode = WorldGraph.FindNearestNode ( vecThreat, this );
	iMyHullIndex = WorldGraph.HullIndex( this );

	if( iMyNode == NO_NODE )
	{
		ALERT( at_aiconsole, "FindCover() - %s has no nearest node!\n", STRING( pev->classname ) );
		return FALSE;
	}
	if( iThreatNode == NO_NODE )
	{
		// ALERT( at_aiconsole, "FindCover() - Threat has no nearest node!\n" );
		iThreatNode = iMyNode;
		// return FALSE;
	}

	vecLookersOffset = vecThreat + vecViewOffset;// calculate location of enemy's eyes

	// we'll do a rough sample to find nodes that are relatively nearby
	for( i = 0; i < WorldGraph.m_cNodes; i++ )
	{
		int nodeNumber = ( i + WorldGraph.m_iLastCoverSearch ) % WorldGraph.m_cNodes;

		CNode &node = WorldGraph.Node( nodeNumber );
		WorldGraph.m_iLastCoverSearch = nodeNumber + 1; // next monster that searches for cover node will start where we left off here.

		// could use an optimization here!!
		flDist = ( pev->origin - node.m_vecOrigin ).Length();

		// DON'T do the trace check on a node that is farther away than a node that we've already found to 
		// provide cover! Also make sure the node is within the mins/maxs of the search.
		if( flDist >= flMinDist && flDist < flMaxDist )
		{
			UTIL_TraceLine( node.m_vecOrigin + vecViewOffset, vecLookersOffset, ignore_monsters, ignore_glass,  ENT( pev ), &tr );

			// if this node will block the threat's line of sight to me...
			if( tr.flFraction != 1.0f )
			{
				// ..and is also closer to me than the threat, or the same distance from myself and the threat the node is good.
				if( ( iMyNode == iThreatNode ) || WorldGraph.PathLength( iMyNode, nodeNumber, iMyHullIndex, m_afCapability ) <= WorldGraph.PathLength( iThreatNode, nodeNumber, iMyHullIndex, m_afCapability ) )
				{
					if( FValidateCover( node.m_vecOrigin ) && MoveToLocation( ACT_RUN, 0, node.m_vecOrigin ) )
					{
						/*
						MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
							WRITE_BYTE( TE_SHOWLINE );

							WRITE_COORD( node.m_vecOrigin.x );
							WRITE_COORD( node.m_vecOrigin.y );
							WRITE_COORD( node.m_vecOrigin.z );

							WRITE_COORD( vecLookersOffset.x );
							WRITE_COORD( vecLookersOffset.y );
							WRITE_COORD( vecLookersOffset.z );
						MESSAGE_END();
						*/

						return TRUE;
					}
				}
			}
		}
	}
	return FALSE;
}

//=========================================================
// BuildNearestRoute - tries to build a route as close to the target
// as possible, even if there isn't a path to the final point.
//
// If supplied, search will return a node at least as far
// away as MinDist from vecThreat, but no farther than MaxDist. 
// if MaxDist isn't supplied, it defaults to a reasonable 
// value
//=========================================================
BOOL CBaseMonster::BuildNearestRoute( Vector vecThreat, Vector vecViewOffset, float flMinDist, float flMaxDist )
{
	int i;
	int iMyHullIndex;
	int iMyNode;
	float flDist;
	Vector vecLookersOffset;
	TraceResult tr;

	if( !flMaxDist )
	{
		// user didn't supply a MaxDist, so work up a crazy one.
		flMaxDist = 784;
	}

	if( flMinDist > 0.5f * flMaxDist )
	{
#if _DEBUG
		ALERT( at_console, "FindCover MinDist (%.0f) too close to MaxDist (%.0f)\n", flMinDist, flMaxDist );
#endif
		flMinDist = 0.5f * flMaxDist;
	}

	if( !WorldGraph.m_fGraphPresent || !WorldGraph.m_fGraphPointersSet )
	{
		ALERT( at_aiconsole, "Graph not ready for BuildNearestRoute!\n" );
		return FALSE;
	}

	iMyNode = WorldGraph.FindNearestNode( pev->origin, this );
	iMyHullIndex = WorldGraph.HullIndex( this );

	if( iMyNode == NO_NODE )
	{
		ALERT( at_aiconsole, "BuildNearestRoute() - %s has no nearest node!\n", STRING( pev->classname ) );
		return FALSE;
	}

	vecLookersOffset = vecThreat + vecViewOffset;// calculate location of enemy's eyes

	// we'll do a rough sample to find nodes that are relatively nearby
	for( i = 0; i < WorldGraph.m_cNodes; i++ )
	{
		int nodeNumber = ( i + WorldGraph.m_iLastCoverSearch ) % WorldGraph.m_cNodes;

		CNode &node = WorldGraph.Node( nodeNumber );
		WorldGraph.m_iLastCoverSearch = nodeNumber + 1; // next monster that searches for cover node will start where we left off here.

		// can I get there?
		if( WorldGraph.NextNodeInRoute( iMyNode, nodeNumber, iMyHullIndex, 0 ) != iMyNode )
		{
			flDist = ( vecThreat - node.m_vecOrigin ).Length();

			// is it close?
			if( flDist > flMinDist && flDist < flMaxDist )
			{
				// can I see where I want to be from there?
				UTIL_TraceLine( node.m_vecOrigin + pev->view_ofs, vecLookersOffset, ignore_monsters, edict(), &tr );

				if( tr.flFraction == 1.0f )
				{
					// try to actually get there
					if( BuildRoute( node.m_vecOrigin, bits_MF_TO_LOCATION, NULL ) )
					{
						// flMaxDist = flDist;
						m_vecMoveGoal = node.m_vecOrigin;
						return TRUE; // UNDONE: keep looking for something closer!
					}
				}
			}
		}
	}

	return FALSE;
}

//=========================================================
// BestVisibleEnemy - this functions searches the link
// list whose head is the caller's m_pLink field, and returns
// a pointer to the enemy entity in that list that is nearest the 
// caller.
//
// !!!UNDONE - currently, this only returns the closest enemy.
// we'll want to consider distance, relationship, attack types, back turned, etc.
//=========================================================
CBaseEntity *CBaseMonster::BestVisibleEnemy( void )
{
	CBaseEntity	*pReturn;
	CBaseEntity	*pNextEnt;
	int iNearest;
	int	iDist;
	int	iBestRelationship;

	iNearest = 8192;// so first visible entity will become the closest.
	if(m_enemyget_mode == 1)
	{
		iNearest = 0;
	}
	pNextEnt = m_pLink;
	pReturn = NULL;
	iBestRelationship = R_NO;

	while( pNextEnt != NULL )
	{
		if( pNextEnt->IsAlive() )
		{
			if( pNextEnt->pev->team == 666)
			{
				if(IRelationship( pNextEnt) >= R_DL)
				{
					iBestRelationship = 4;
					iNearest = ( pNextEnt->pev->origin - pev->origin ).Length();
					pReturn = pNextEnt;
				}
			}
			else if( IRelationship( pNextEnt) > iBestRelationship )
			{
				// this entity is disliked MORE than the entity that we 
				// currently think is the best visible enemy. No need to do 
				// a distance check, just get mad at this one for now.
				iBestRelationship = IRelationship ( pNextEnt );
				iNearest = ( pNextEnt->pev->origin - pev->origin ).Length();
				pReturn = pNextEnt;
			}
			else if( IRelationship( pNextEnt) == iBestRelationship )
			{
				// this entity is disliked just as much as the entity that
				// we currently think is the best visible enemy, so we only
				// get mad at it if it is closer.
				iDist = ( pNextEnt->pev->origin - pev->origin ).Length();


				if(m_enemyget_mode == 1)
				{
					if ( iDist >= iNearest )
					{
						iNearest = iDist;
						iBestRelationship = IRelationship ( pNextEnt );
						pReturn = pNextEnt;
					}
				}
				else 
				{
					if( iDist <= iNearest )
					{
						iNearest = iDist;
						iBestRelationship = IRelationship( pNextEnt );
						pReturn = pNextEnt;
					}
				}
			}
		}

		pNextEnt = pNextEnt->m_pLink;
	}

	return pReturn;
}

//=========================================================
// MakeIdealYaw - gets a yaw value for the caller that would
// face the supplied vector. Value is stuffed into the monster's
// ideal_yaw
//=========================================================
void CBaseMonster::MakeIdealYaw( Vector vecTarget )
{
	Vector vecProjection;

	// strafing monster needs to face 90 degrees away from its goal
	if( m_movementActivity == ACT_STRAFE_LEFT )
	{
		vecProjection.x = -vecTarget.y;
		vecProjection.y = vecTarget.x;

		pev->ideal_yaw = UTIL_VecToYaw( vecProjection - pev->origin );
	}
	else if( m_movementActivity == ACT_STRAFE_RIGHT )
	{
		vecProjection.x = vecTarget.y;
		vecProjection.y = vecTarget.x;

		pev->ideal_yaw = UTIL_VecToYaw( vecProjection - pev->origin );
	}
	else
	{
		pev->ideal_yaw = UTIL_VecToYaw( vecTarget - pev->origin );
	}
}

//=========================================================
// FlYawDiff - returns the difference ( in degrees ) between
// monster's current yaw and ideal_yaw
//
// Positive result is left turn, negative is right turn
//=========================================================
float CBaseMonster::FlYawDiff( void )
{
	float flCurrentYaw;

	flCurrentYaw = UTIL_AngleMod( pev->angles.y );

	if( flCurrentYaw == pev->ideal_yaw )
	{
		return 0;
	}

	return UTIL_AngleDiff( pev->ideal_yaw, flCurrentYaw );
}

//=========================================================
// Changeyaw - turns a monster towards its ideal_yaw
//=========================================================
float CBaseMonster::ChangeYaw( int yawSpeed )
{
	float		ideal, current, move, speed;

	current = UTIL_AngleMod( pev->angles.y );
	ideal = pev->ideal_yaw;
	if( current != ideal )
	{
		if( monsteryawspeedfix.value )
		{
			if( m_flLastYawTime == 0.f )
				m_flLastYawTime = gpGlobals->time - gpGlobals->frametime;

			float delta = Q_min( gpGlobals->time - m_flLastYawTime, 0.25f );

			m_flLastYawTime = gpGlobals->time;

			speed = (float)yawSpeed * delta * 2;
		}
		else
			speed = (float)yawSpeed * gpGlobals->frametime * 10;

		move = ideal - current;

		if( ideal > current )
		{
			if( move >= 180 )
				move = move - 360;
		}
		else
		{
			if( move <= -180 )
				move = move + 360;
		}

		if( move > 0 )
		{
			// turning to the monster's left
			if( move > speed )
				move = speed;
		}
		else
		{
			// turning to the monster's right
			if( move < -speed )
				move = -speed;
		}

		pev->angles.y = UTIL_AngleMod( current + move );

		// turn head in desired direction only if they have a turnable head
		if( m_afCapability & bits_CAP_TURN_HEAD )
		{
			float yaw = pev->ideal_yaw - pev->angles.y;
			if( yaw > 180 )
				yaw -= 360;
			if( yaw < -180 )
				yaw += 360;
			// yaw *= 0.8;
			SetBoneController( 0, yaw );
		}
	}
	else
		move = 0;

	return move;
}

//=========================================================
// VecToYaw - turns a directional vector into a yaw value
// that points down that vector.
//=========================================================
float CBaseMonster::VecToYaw( Vector vecDir )
{
	if( vecDir.x == 0 && vecDir.y == 0 && vecDir.z == 0 )
		return pev->angles.y;

	return UTIL_VecToYaw( vecDir );
}

//=========================================================
// SetEyePosition
//
// queries the monster's model for $eyeposition and copies
// that vector to the monster's view_ofs
//
//=========================================================
void CBaseMonster::SetEyePosition( void )
{
	Vector  vecEyePosition;
	void	*pmodel = GET_MODEL_PTR( ENT(pev) );

	GetEyePosition( pmodel, vecEyePosition );

	pev->view_ofs = vecEyePosition;

	if( pev->view_ofs == g_vecZero )
	{
		ALERT( at_aiconsole, "%s has no view_ofs!\n", STRING( pev->classname ) );
	}
}

void CBaseMonster::HandleAnimEvent( MonsterEvent_t *pEvent )
{
	switch( pEvent->event )
	{
	case SCRIPT_EVENT_DEAD:
		if( m_MonsterState == MONSTERSTATE_SCRIPT )
		{
			pev->deadflag = DEAD_DYING;
			// Kill me now! (and fade out when CineCleanup() is called)
#if _DEBUG
			ALERT( at_aiconsole, "Death event: %s\n", STRING( pev->classname ) );
#endif
			pev->health = 0;
		}
#if _DEBUG
		else
			ALERT( at_aiconsole, "INVALID death event:%s\n", STRING( pev->classname ) );
#endif
		break;
	case SCRIPT_EVENT_NOT_DEAD:
		if( m_MonsterState == MONSTERSTATE_SCRIPT )
		{
			pev->deadflag = DEAD_NO;

			// This is for life/death sequences where the player can determine whether a character is dead or alive after the script 
			pev->health = pev->max_health;
		}
		break;
	case SCRIPT_EVENT_SOUND:			// Play a named wave file
		EMIT_SOUND( edict(), CHAN_BODY, pEvent->options, 1.0, ATTN_IDLE );
		break;
	case SCRIPT_EVENT_SOUND_VOICE:
		EMIT_SOUND( edict(), CHAN_VOICE, pEvent->options, 1.0, ATTN_IDLE );
		break;
	case SCRIPT_EVENT_SENTENCE_RND1:		// Play a named sentence group 33% of the time
		if( RANDOM_LONG( 0, 2 ) == 0 )
			break;
		// fall through...
	case SCRIPT_EVENT_SENTENCE:			// Play a named sentence group
		SENTENCEG_PlayRndSz( edict(), pEvent->options, 1.0, ATTN_IDLE, 0, 100 );
		break;
	case SCRIPT_EVENT_FIREEVENT:		// Fire a trigger
		FireTargets( pEvent->options, this, this, USE_TOGGLE, 0 );
		break;
	case SCRIPT_EVENT_NOINTERRUPT:		// Can't be interrupted from now on
		if( m_pCine )
			m_pCine->AllowInterrupt( FALSE );
		break;
	case SCRIPT_EVENT_CANINTERRUPT:		// OK to interrupt now
		if( m_pCine )
			m_pCine->AllowInterrupt( TRUE );
		break;
#if 0
	case SCRIPT_EVENT_INAIR:			// Don't DROP_TO_FLOOR()
	case SCRIPT_EVENT_ENDANIMATION:		// Set ending animation sequence to
		break;
#endif
	case MONSTER_EVENT_BODYDROP_HEAVY:
		if( pev->flags & FL_ONGROUND )
		{
			if( RANDOM_LONG( 0, 1 ) == 0 )
			{
				EMIT_SOUND_DYN( ENT( pev ), CHAN_BODY, "common/bodydrop3.wav", 1, ATTN_NORM, 0, 90 );
			}
			else
			{
				EMIT_SOUND_DYN( ENT( pev ), CHAN_BODY, "common/bodydrop4.wav", 1, ATTN_NORM, 0, 90 );
			}
		}
		break;
	case MONSTER_EVENT_BODYDROP_LIGHT:
		if( pev->flags & FL_ONGROUND )
		{
			if( RANDOM_LONG( 0, 1 ) == 0 )
			{
				EMIT_SOUND( ENT( pev ), CHAN_BODY, "common/bodydrop3.wav", 1, ATTN_NORM );
			}
			else
			{
				EMIT_SOUND( ENT( pev ), CHAN_BODY, "common/bodydrop4.wav", 1, ATTN_NORM );
			}
		}
		break;
	case MONSTER_EVENT_BODYSIZE_DOWN:	
				if ( pev->flags & FL_ONGROUND )
				{
					if ( RANDOM_LONG( 0, 1 ) == 0 )
					{
						EMIT_SOUND( ENT(pev), CHAN_BODY, "common/bodydrop3.wav", 1, ATTN_NORM );
					}
					else
					{
						EMIT_SOUND( ENT(pev), CHAN_BODY, "common/bodydrop4.wav", 1, ATTN_NORM );
					}
				}
				if(m_die_corpse_solid == 1)
				{
					UTIL_SetSize ( pev, Vector ( pev->mins.x, pev->mins.y, pev->mins.z ), Vector ( pev->maxs.x, pev->maxs.y, 1 ) );
				}
				else if ( !BBoxFlat() || m_crouchmode != 0 )
				{
					UTIL_SetSize ( pev, Vector ( pev->mins.x, pev->mins.y, pev->mins.z ), Vector ( pev->maxs.x, pev->maxs.y, pev->mins.z + 2 ) );
				}
				else
				{
					UTIL_SetSize ( pev, Vector ( pev->mins.x, pev->mins.y, pev->mins.z ), Vector ( pev->maxs.x, pev->maxs.y, pev->mins.z + 4 ) );
				}	
		break;
	case MONSTER_EVENT_SWISHSOUND:
		{
			// NO MONSTER may use this anim event unless that monster's precache precaches this sound!!!
			EMIT_SOUND( ENT( pev ), CHAN_BODY, "zombie/claw_miss2.wav", 1, ATTN_NORM );
			break;
		}
	default:
		ALERT( at_aiconsole, "Unhandled animation event %d for %s\n", pEvent->event, STRING( pev->classname ) );
		break;
	}
}

// Combat
Vector CBaseMonster::GetGunPosition()
{
	UTIL_MakeVectors( pev->angles );

	// Vector vecSrc = pev->origin + gpGlobals->v_forward * 10;
	//vecSrc.z = pevShooter->absmin.z + pevShooter->size.z * 0.7;
	//vecSrc.z = pev->origin.z + (pev->view_ofs.z - 4);
	Vector vecSrc = pev->origin 
					+ gpGlobals->v_forward * m_HackedGunPos.y 
					+ gpGlobals->v_right * m_HackedGunPos.x 
					+ gpGlobals->v_up * m_HackedGunPos.z;

	return vecSrc;
}

//=========================================================
// NODE GRAPH
//=========================================================

//=========================================================
// FGetNodeRoute - tries to build an entire node path from
// the callers origin to the passed vector. If this is 
// possible, ROUTE_SIZE waypoints will be copied into the
// callers m_Route. TRUE is returned if the operation 
// succeeds (path is valid) or FALSE if failed (no path 
// exists )
//=========================================================
BOOL CBaseMonster::FGetNodeRoute( Vector vecDest )
{
	int iPath[ MAX_PATH_SIZE ];
	int iSrcNode, iDestNode;
	int iResult;
	int i;
	int iNumToCopy;

	iSrcNode = WorldGraph.FindNearestNode( pev->origin, this );
	iDestNode = WorldGraph.FindNearestNode( vecDest, this );

	if( iSrcNode == -1 )
	{
		// no node nearest self
		//ALERT( at_aiconsole, "FGetNodeRoute: No valid node near self!\n" );
		return FALSE;
	}
	else if( iDestNode == -1 )
	{
		// no node nearest target
		//ALERT( at_aiconsole, "FGetNodeRoute: No valid node near target!\n" );
		return FALSE;
	}

	// valid src and dest nodes were found, so it's safe to proceed with
	// find shortest path
	int iNodeHull = WorldGraph.HullIndex( this ); // make this a monster virtual function
	iResult = WorldGraph.FindShortestPath( iPath, iSrcNode, iDestNode, iNodeHull, m_afCapability );

	if( !iResult )
	{
#if 1
		ALERT( at_aiconsole, "No Path from %d to %d!\n", iSrcNode, iDestNode );
		return FALSE;
#else
		BOOL bRoutingSave = WorldGraph.m_fRoutingComplete;
		WorldGraph.m_fRoutingComplete = FALSE;
		iResult = WorldGraph.FindShortestPath( iPath, iSrcNode, iDestNode, iNodeHull, m_afCapability );
		WorldGraph.m_fRoutingComplete = bRoutingSave;
		if( !iResult )
		{
			ALERT( at_aiconsole, "No Path from %d to %d!\n", iSrcNode, iDestNode );
			return FALSE;
		}
		else
		{
			ALERT( at_aiconsole, "Routing is inconsistent!" );
		}
#endif
	}

	// there's a valid path within iPath now, so now we will fill the route array
	// up with as many of the waypoints as it will hold.
	
	// don't copy ROUTE_SIZE entries if the path returned is shorter
	// than ROUTE_SIZE!!!
	if( iResult < ROUTE_SIZE )
	{
		iNumToCopy = iResult;
	}
	else
	{
		iNumToCopy = ROUTE_SIZE;
	}

	for( i = 0 ; i < iNumToCopy; i++ )
	{
		m_Route[i].vecLocation = WorldGraph.m_pNodes[iPath[i]].m_vecOrigin;
		m_Route[i].iType = bits_MF_TO_NODE;
	}

	if( iNumToCopy < ROUTE_SIZE )
	{
		m_Route[iNumToCopy].vecLocation = vecDest;
		m_Route[iNumToCopy].iType |= bits_MF_IS_GOAL;
	}

	return TRUE;
}

//=========================================================
// FindHintNode
//=========================================================
int CBaseMonster::FindHintNode( void )
{
	int i;
	TraceResult tr;

	if( !WorldGraph.m_fGraphPresent )
	{
		ALERT( at_aiconsole, "find_hintnode: graph not ready!\n" );
		return NO_NODE;
	}

	if( WorldGraph.m_iLastActiveIdleSearch >= WorldGraph.m_cNodes )
	{
		WorldGraph.m_iLastActiveIdleSearch = 0;
	}

	for( i = 0; i < WorldGraph.m_cNodes; i++ )
	{
		int nodeNumber = ( i + WorldGraph.m_iLastActiveIdleSearch) % WorldGraph.m_cNodes;
		CNode &node = WorldGraph.Node( nodeNumber );

		if( node.m_sHintType )
		{
			// this node has a hint. Take it if it is visible, the monster likes it, and the monster has an animation to match the hint's activity.
			if( FValidateHintType( node.m_sHintType ) )
			{
				if( !node.m_sHintActivity || LookupActivity( node.m_sHintActivity ) != ACTIVITY_NOT_AVAILABLE )
				{
					UTIL_TraceLine( pev->origin + pev->view_ofs, node.m_vecOrigin + pev->view_ofs, ignore_monsters, ENT( pev ), &tr );

					if( tr.flFraction == 1.0f )
					{
						WorldGraph.m_iLastActiveIdleSearch = nodeNumber + 1; // next monster that searches for hint nodes will start where we left off.
						return nodeNumber;// take it!
					}
				}
			}
		}
	}

	WorldGraph.m_iLastActiveIdleSearch = 0;// start at the top of the list for the next search.

	return NO_NODE;
}		

void CBaseMonster::ReportAIState( void )
{
	ALERT_TYPE level = at_console;

	static const char *pStateNames[] = { "None", "Idle", "Combat", "Alert", "Hunt", "Prone", "Scripted", "Dead" };

	ALERT( level, "%s: ", STRING(pev->classname) );
	if( (int)m_MonsterState < ARRAYSIZE( pStateNames ) )
		ALERT( level, "State: %s, ", pStateNames[m_MonsterState] );
	int i = 0;
	while( activity_map[i].type != 0 )
	{
		if( activity_map[i].type == (int)m_Activity )
		{
			ALERT( level, "Activity %s, ", activity_map[i].name );
			break;
		}
		i++;
	}

	if( m_pSchedule )
	{
		const char *pName = NULL;
		pName = m_pSchedule->pName;
		if( !pName )
			pName = "Unknown";
		ALERT( level, "Schedule %s, ", pName );
		Task_t *pTask = GetTask();
		if( pTask )
			ALERT( level, "Task %d (#%d), ", pTask->iTask, m_iScheduleIndex );
	}
	else
		ALERT( level, "No Schedule, " );

	if( m_hEnemy != 0 )
		ALERT( level, "\nEnemy is %s", STRING( m_hEnemy->pev->classname ) );
	else
		ALERT( level, "No enemy" );

	if( IsMoving() )
	{
		ALERT( level, " Moving " );
		if( m_flMoveWaitFinished > gpGlobals->time )
			ALERT( level, ": Stopped for %.2f. ", (double)(m_flMoveWaitFinished - gpGlobals->time) );
		else if( m_IdealActivity == GetStoppedActivity() )
			ALERT( level, ": In stopped anim. " );
	}

	CSquadMonster *pSquadMonster = MySquadMonsterPointer();

	if( pSquadMonster )
	{
		if( !pSquadMonster->InSquad() )
		{
			ALERT( level, "not " );
		}

		ALERT( level, "In Squad, " );

		if( !pSquadMonster->IsLeader() )
		{
			ALERT( level, "not " );
		}

		ALERT( level, "Leader." );
	}

	ALERT( level, "\n" );
	ALERT( level, "Yaw speed:%3.1f,Health: %3.1f\n", (double)pev->yaw_speed, (double)pev->health );
	if( pev->spawnflags & SF_MONSTER_PRISONER )
		ALERT( level, " PRISONER! " );
	if( pev->spawnflags & SF_MONSTER_PREDISASTER )
		ALERT( level, " Pre-Disaster! " );
	ALERT( level, "\n" );
}

//=========================================================
// KeyValue
//
// !!! netname entvar field is used in squadmonster for groupname!!!
//=========================================================
void CBaseMonster::KeyValue( KeyValueData *pkvd )
{
	if( FStrEq( pkvd->szKeyName, "TriggerTarget" ) )
	{
		m_iszTriggerTarget = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "TriggerFTS") )
	{
		m_FTSmod = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "ElseFunction") )
	{
		if(atoi( pkvd->szValue ) == 1)
		{
			m_walkaround = TRUE;
		}
		else if(atoi( pkvd->szValue ) == 2)
		{
			m_walkaround = TRUE;
			m_alwaysrunpath = TRUE;
		}
		else if(atoi( pkvd->szValue ) == 3)
		{
			m_alwaysrunpath = TRUE;
		}
		else if(atoi( pkvd->szValue ) == 4)
		{
			m_walkaround = TRUE;
			m_selfmode = TRUE;
		}
		else if(atoi( pkvd->szValue ) == 5)
		{
			m_selfmode = TRUE;
		}
		else if(atoi( pkvd->szValue ) == 6)
		{
			m_walkaround = TRUE;
			m_ctmod = 1;
		}
		else if(atoi( pkvd->szValue ) == 7)
		{
			m_longming = 1;
		}
		else if(atoi( pkvd->szValue ) == 8)
		{
			m_selfmode = TRUE;
			m_longming = 1;
		}
		else if(atoi( pkvd->szValue ) == 9)
		{
			m_selfmode = TRUE;
			m_longming = 1;
			m_godmode = TRUE;
		}
		else if(atoi( pkvd->szValue ) == 10)
		{
			pev->flags		|= FL_NOTARGET;
		}
		else if(atoi( pkvd->szValue ) == 11)
		{
			m_MoveFail_SimpleRoad = TRUE;
			m_ignoreFail_MAX = 50;
			m_ignoreFail_OFF = 0;
		}
		else if(atoi( pkvd->szValue ) == 12)
		{
			m_no_pov_limit = 1;
		}
		else if(atoi( pkvd->szValue ) == 13)
		{
			m_EyeMod = 1;
		}
		else if(atoi( pkvd->szValue ) == 14)
		{
			m_EyeMod = 2;
		}
		else if(atoi( pkvd->szValue ) == 15)
		{
			m_walkaround = TRUE;
			m_walkaroundFail = TRUE;
		}
		else if(atoi( pkvd->szValue ) == 16)
		{
			m_EyeMod = 1;
			m_ctmod = 1;
		}
		else if(atoi( pkvd->szValue ) == 17)
		{
			m_FTSmod = 8;
		}
		else if(atoi( pkvd->szValue ) == 18)
		{
			m_FTSmod = 8;
			m_no_pov_limit = 1;
		}
		else if(atoi( pkvd->szValue ) == 19)
		{
			m_user_aimflag = TRUE;
			m_longming = 1;
		}
		else if(atoi( pkvd->szValue ) == 20)
		{
			m_undropgun = TRUE;
		}
		else if(atoi( pkvd->szValue ) == 21)
		{
			m_selfmode = TRUE;
			m_longming = 1;
			m_godmode = TRUE;
			pev->flags		|= FL_NOTARGET;
		}
		else if(atoi( pkvd->szValue ) == 22)
		{
			m_singdelay_max = 5;
			m_singdelay_use = m_singdelay_max;
			m_flFieldOfView	= 0.5;
		}
		else if(atoi( pkvd->szValue ) == 23)
		{
			m_rpgms_inteam = 5;
		}
		else if(atoi( pkvd->szValue ) == 24)
		{
			m_selfmode = TRUE;
			m_rpgms_inteam = 5;
			m_godmode = TRUE;
			pev->flags |= FL_NOTARGET;
		}
		
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "TriggerCondition" ) )
	{
		m_iTriggerCondition = atoi( pkvd->szValue );

		if(m_iTriggerCondition == 1000)
		{
			m_flFieldOfView		= -1;
			m_iTriggerCondition = 0;
		}

		if(m_iTriggerCondition == 1419)
		{
			m_FTSmod = 4;
			m_iTriggerCondition = 4;
			m_no_pov_limit = 1;
		}
		if(m_iTriggerCondition == 1834)
		{
			m_running_rangeattack = 1;
			m_iTriggerCondition = 0;
			m_no_pov_limit = 1;
		}
		if(m_iTriggerCondition == 8757)
		{
			m_FTSmod = 1;
			m_chase_mode = 2;
			m_iTriggerCondition = 0;
			m_no_pov_limit = 1;
		}
		if(m_iTriggerCondition == 1919)
		{
			m_ignoreFail_OFF = 1;
			m_iTriggerCondition = 0;
		}

		if(m_iTriggerCondition == 1024)
		{
			m_flFieldOfView		= -1;
			m_iTriggerCondition = 6547;
		}

		pkvd->fHandled = TRUE;
	}
	else
	{
		CBaseToggle::KeyValue( pkvd );
	}
}

//=========================================================
// FCheckAITrigger - checks the monster's AI Trigger Conditions,
// if there is a condition, then checks to see if condition is 
// met. If yes, the monster's TriggerTarget is fired.
//
// Returns TRUE if the target is fired.
//=========================================================
BOOL CBaseMonster::FCheckAITrigger( void )
{
	BOOL fFireTarget;

	if( m_iTriggerCondition == AITRIGGER_NONE )
	{
		// no conditions, so this trigger is never fired.
		return FALSE; 
	}

	fFireTarget = FALSE;

	switch( m_iTriggerCondition )
	{
	case AITRIGGER_SEEPLAYER_ANGRY_AT_PLAYER:
		if( m_hEnemy != 0 && m_hEnemy->IsPlayer() && HasConditions( bits_COND_SEE_ENEMY ) )
		{
			fFireTarget = TRUE;
		}
		break;
	case 7456:
		if(m_lovehate > 0)
		{
			m_lovehate = 810;
			m_longming = 1;
			m_no_pov_limit = 1;
		}
		break;
	case 1919:
		if ( m_cleardally >= 25 )
		{
			if ( !FStringNull(pev->message) )// this monster has a target
			{
				// Find the monster's initial target entity, stash it
				m_pGoalEnt = CBaseEntity::Instance( FIND_ENTITY_BY_TARGETNAME ( NULL, STRING( pev->message ) ) );

				if ( !m_pGoalEnt )
				{
					ALERT(at_error, "ReadyMonster()--%s couldn't find target %s", STRING(pev->classname), STRING(pev->target));
				}
				else
				{
					// Monster will start turning towards his destination
					MakeIdealYaw ( m_pGoalEnt->pev->origin );

					m_movementGoal = MOVEGOAL_PATHCORNER;
					
					if ( pev->movetype == MOVETYPE_FLY )
					{
						m_movementActivity = ACT_FLY;
					}
					else if ( LookupActivity( ACT_RUN ) != ACTIVITY_NOT_AVAILABLE && m_alwaysrunpath )
					{
						m_movementActivity = ACT_RUN;
					}
					else{
						m_movementActivity = ACT_WALK;
					}
					if ( !FRefreshRoute() )
					{
						ALERT ( at_aiconsole, "Can't Create Route!\n" );
					}
					SetState( MONSTERSTATE_IDLE );
					ChangeSchedule( GetScheduleOfType( SCHED_IDLE_WALK ) );
				}
			}
			m_iTriggerCondition = AITRIGGER_NONE;
			return FALSE; 
		}
		break;
	case AITRIGGER_SEEPLAYER_UNCONDITIONAL:
		if( HasConditions( bits_COND_SEE_CLIENT ) )
		{
			fFireTarget = TRUE;
		}
		break;
	case 123:
		{
			CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "player" );
			if ( pEntity )
			{
				float flDist = ( pEntity->pev->origin - pev->origin).Length();
				if(flDist < 250)
				{
					m_boltpoison = 40;
					pev->sequence = LookupActivity ( ACT_MELEE_ATTACK1 );
					ResetSequenceInfo( );
					pev->frame = 0;
					m_iTriggerCondition = AITRIGGER_NONE;
				}
			}
		}
		break;
	case AITRIGGER_SEEPLAYER_NOT_IN_COMBAT:
		if( HasConditions( bits_COND_SEE_CLIENT ) && 
			 m_MonsterState != MONSTERSTATE_COMBAT	&& 
			 m_MonsterState != MONSTERSTATE_PRONE	&& 
			 m_MonsterState != MONSTERSTATE_SCRIPT)
		{
			fFireTarget = TRUE;
		}
		break;
	case AITRIGGER_TAKEDAMAGE:
		if( m_afConditions & ( bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE ) )
		{
			fFireTarget = TRUE;
		}
		break;
	case AITRIGGER_DEATH:
		if( pev->deadflag != DEAD_NO )
		{
			fFireTarget = TRUE;
		}
		break;
	case AITRIGGER_HALFHEALTH:
		if( IsAlive() && pev->health <= ( pev->max_health / 2 ) )
		{
			fFireTarget = TRUE;
		}
		break;
/*

  // !!!UNDONE - no persistant game state that allows us to track these two. 

	case AITRIGGER_SQUADMEMBERDIE:
		break;
	case AITRIGGER_SQUADLEADERDIE:
		break;
*/
	case AITRIGGER_HEARWORLD:
		if( m_afConditions & bits_COND_HEAR_SOUND && m_afSoundTypes & bits_SOUND_WORLD )
		{
			fFireTarget = TRUE;
		}
		break;
	case AITRIGGER_HEARPLAYER:
		if( m_afConditions & bits_COND_HEAR_SOUND && m_afSoundTypes & bits_SOUND_PLAYER )
		{
			fFireTarget = TRUE;
		}
		break;
	case AITRIGGER_HEARCOMBAT:
		if( m_afConditions & bits_COND_HEAR_SOUND && m_afSoundTypes & bits_SOUND_COMBAT )
		{
			fFireTarget = TRUE;
		}
		break;
	case AITRIGGER_SEEPLAYER_OR_TAKEDAMAGE:
		if ( pev->deadflag != DEAD_NO )
		{
			fFireTarget = TRUE;
		}
		else if ( m_afConditions & ( bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE ) )
		{
			fFireTarget = TRUE;
		}
		else if ( HasConditions ( bits_COND_SEE_CLIENT ) || HasConditions ( bits_COND_SEE_ENEMY ) )
		{
			fFireTarget = TRUE;
		}
		break;
	case 13:
		if ( m_afConditions & ( bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE ) )
		{
			fFireTarget = TRUE;
		} 
		else if(m_cleardally >= 10){
			fFireTarget = TRUE;
		}
		break;
	case 6547:
		if(m_cleardally >= 1){
		fFireTarget = TRUE;
		}
		break;
	}

	if( fFireTarget )
	{
		// fire the target, then set the trigger conditions to NONE so we don't fire again
		ALERT( at_aiconsole, "AI Trigger Fire Target\n" );
		FireTargets( STRING( m_iszTriggerTarget ), this, this, USE_TOGGLE, 0 );
		m_iTriggerCondition = AITRIGGER_NONE;
		return TRUE;
	}

	return FALSE;
}

//=========================================================	
// CanPlaySequence - determines whether or not the monster
// can play the scripted sequence or AI sequence that is 
// trying to possess it. If DisregardState is set, the monster
// will be sucked into the script no matter what state it is
// in. ONLY Scripted AI ents should allow this.
//=========================================================	
int CBaseMonster::CanPlaySequence( BOOL fDisregardMonsterState, int interruptLevel )
{
	if( m_pCine || !IsAlive() || m_MonsterState == MONSTERSTATE_PRONE )
	{
		// monster is already running a scripted sequence or dead!
		return FALSE;
	}
	
	if( fDisregardMonsterState )
	{
		// ok to go, no matter what the monster state. (scripted AI)
		return TRUE;
	}

	if( m_MonsterState == MONSTERSTATE_NONE || m_MonsterState == MONSTERSTATE_IDLE || m_IdealMonsterState == MONSTERSTATE_IDLE )
	{
		// ok to go, but only in these states
		return TRUE;
	}
	
	if( m_MonsterState == MONSTERSTATE_ALERT )
		return TRUE;

	// unknown situation
	return FALSE;
}

//=========================================================
// FindLateralCover - attempts to locate a spot in the world
// directly to the left or right of the caller that will
// conceal them from view of pSightEnt
//=========================================================
#define	COVER_CHECKS	5// how many checks are made
#define COVER_DELTA		48// distance between checks

BOOL CBaseMonster::FindLateralCover( const Vector &vecThreat, const Vector &vecViewOffset )
{
	TraceResult tr;
	Vector	vecBestOnLeft;
	Vector	vecBestOnRight;
	Vector	vecLeftTest;
	Vector	vecRightTest;
	Vector	vecStepRight;
	int	i;

	UTIL_MakeVectors( pev->angles );
	vecStepRight = gpGlobals->v_right * COVER_DELTA;
	vecStepRight.z = 0; 

	vecLeftTest = vecRightTest = pev->origin;

	for( i = 0; i < COVER_CHECKS; i++ )
	{
		vecLeftTest = vecLeftTest - vecStepRight;
		vecRightTest = vecRightTest + vecStepRight;

		// it's faster to check the SightEnt's visibility to the potential spot than to check the local move, so we do that first.
		UTIL_TraceLine( vecThreat + vecViewOffset, vecLeftTest + pev->view_ofs, ignore_monsters, ignore_glass, ENT( pev )/*pentIgnore*/, &tr );

		if( tr.flFraction != 1.0f )
		{
			if( FValidateCover( vecLeftTest ) && CheckLocalMove( pev->origin, vecLeftTest, NULL, NULL, 0 ) == LOCALMOVE_VALID )
			{
				if( MoveToLocation( ACT_RUN, 0, vecLeftTest ) )
				{
					return TRUE;
				}
			}
		}
		
		// it's faster to check the SightEnt's visibility to the potential spot than to check the local move, so we do that first.
		UTIL_TraceLine( vecThreat + vecViewOffset, vecRightTest + pev->view_ofs, ignore_monsters, ignore_glass, ENT(pev)/*pentIgnore*/, &tr );

		if( tr.flFraction != 1.0f )
		{
			if( FValidateCover( vecRightTest ) && CheckLocalMove( pev->origin, vecRightTest, NULL, NULL, 0 ) == LOCALMOVE_VALID )
			{
				if( MoveToLocation( ACT_RUN, 0, vecRightTest ) )
				{
					return TRUE;
				}
			}
		}
	}

	return FALSE;
}

Vector CBaseMonster::ShootAtEnemy( const Vector &shootOrigin )
{
	CBaseEntity *pEnemy = m_hEnemy;

	if( pEnemy )
	{
		Vector duckheight = Vector(0,0,0);

		CBaseMonster *pEnemyMonster;
		pEnemyMonster = pEnemy->MyMonsterPointer();
		if(pEnemyMonster && pEnemyMonster->m_duckseq == 1)
		{
			duckheight = Vector(0,0,-10);
		}
		else if(pEnemyMonster && pEnemyMonster->m_duckseq == 2)
		{
			duckheight = Vector(0,0,-24);
		}
		else if(pEnemyMonster && pEnemyMonster->m_duckseq == 3)
		{
			duckheight = Vector(0,0,-27);
		}
		else if(pEnemyMonster && pEnemyMonster->m_duckseq == 4)
		{
			duckheight = Vector(0,0,-32);
		}
		else if(pEnemyMonster && pEnemyMonster->m_duckseq == 5)
		{
			duckheight = Vector(0,0,-15);
		}

		if(m_aimenemy_mod == 9)
		{
			return ( (pEnemy->BodyTarget_d( shootOrigin ) - pEnemy->pev->origin + duckheight) + m_vecEnemyLKP - shootOrigin ).Normalize();
		}
		else if(m_aimenemy_mod == 8)
		{
			return ( (pEnemy->BodyTarget_b( shootOrigin ) - pEnemy->pev->origin + duckheight) + m_vecEnemyLKP - shootOrigin ).Normalize();
		}
		else if(m_aimenemy_mod == 7)
		{
			return ( (pEnemy->BodyTarget_l( shootOrigin ) - pEnemy->pev->origin + duckheight) + m_vecEnemyLKP - shootOrigin ).Normalize();
		}
		else if(m_aimenemy_mod == 6)
		{
			return ( (pEnemy->BodyTarget_o( shootOrigin ) - pEnemy->pev->origin + duckheight) + m_vecEnemyLKP - shootOrigin ).Normalize();
		}
		else if(m_aimenemy_mod == 5)
		{
			return ( (pEnemy->BodyTarget( shootOrigin ) - pEnemy->pev->origin + duckheight) + m_vecEnemyLKP - shootOrigin ).Normalize();
		}
		else if(m_aimenemy_mod == 4)
		{
			return ( (pEnemy->BodyTarget_e( shootOrigin ) - pEnemy->pev->origin + duckheight) + m_vecEnemyLKP - shootOrigin ).Normalize();
		}
		else if(m_aimenemy_mod == 3)
		{
			return ( (pEnemy->BodyTarget_h( shootOrigin ) - pEnemy->pev->origin + duckheight) + m_vecEnemyLKP - shootOrigin ).Normalize();
		}
		else if(m_aimenemy_mod == 2)
		{
			int aim_mod = RANDOM_LONG(1,5);
			if(aim_mod == 1)
			{
				return ( (pEnemy->BodyTarget( shootOrigin ) - pEnemy->pev->origin + duckheight) + m_vecEnemyLKP - shootOrigin ).Normalize();
			}
			else if(aim_mod == 5)
			{
				return ( (pEnemy->BodyTarget_e( shootOrigin ) - pEnemy->pev->origin + duckheight) + m_vecEnemyLKP - shootOrigin ).Normalize();
			}
			else
			{
				return ( (pEnemy->BodyTarget_h( shootOrigin ) - pEnemy->pev->origin + duckheight) + m_vecEnemyLKP - shootOrigin ).Normalize();
			}
		}
		else if(m_aimenemy_mod == 1)
		{
			return ( (pEnemy->BodyTarget_c( shootOrigin ) - pEnemy->pev->origin + duckheight) + m_vecEnemyLKP - shootOrigin ).Normalize();
		}
		else
		{
			return ( (pEnemy->BodyTarget( shootOrigin ) - pEnemy->pev->origin + duckheight) + m_vecEnemyLKP - shootOrigin ).Normalize();
		}
	}
	else
		return gpGlobals->v_forward;
}

//=========================================================
// FacingIdeal - tells us if a monster is facing its ideal
// yaw. Created this function because many spots in the 
// code were checking the yawdiff against this magic
// number. Nicer to have it in one place if we're gonna
// be stuck with it.
//=========================================================
BOOL CBaseMonster::FacingIdeal( void )
{
	if( m_playerguardian_mode == 1 || m_facing_fucking_mode == 1 || fabs( FlYawDiff() ) <= 0.006 )//!!!BUGBUG - no magic numbers!!!
	{
		return TRUE;
	}

	return FALSE;
}

//=========================================================
// FCanActiveIdle
//=========================================================
BOOL CBaseMonster::FCanActiveIdle( void )
{
	if( m_MonsterState == MONSTERSTATE_IDLE && m_IdealMonsterState == MONSTERSTATE_IDLE && !IsMoving() )
	{
		return TRUE;
	}
	return FALSE;
}

#if !SPEAKABLE_TARGETS
void CBaseMonster::PlaySentence( const char *pszSentence, float duration, float volume, float attenuation )
{
	if( pszSentence && IsAlive() )
	{
		if( pszSentence[0] == '!' )
			EMIT_SOUND_DYN( edict(), CHAN_VOICE, pszSentence, volume, attenuation, 0, PITCH_NORM );
		else
			SENTENCEG_PlayRndSz( edict(), pszSentence, volume, attenuation, 0, PITCH_NORM );
	}
}

void CBaseMonster::PlayScriptedSentence( const char *pszSentence, float duration, float volume, float attenuation, BOOL bConcurrent, CBaseEntity *pListener )
{
	PlaySentence( pszSentence, duration, volume, attenuation );
}

void CBaseMonster::SentenceStop( void )
{
	EMIT_SOUND( edict(), CHAN_VOICE, "common/null.wav", 1.0, ATTN_IDLE );
}
#endif
void CBaseMonster::CorpseFallThink( void )
{
	pev->movetype = MOVETYPE_TOSS;
	SetThink( NULL );

	SetSequenceBox( );
	UTIL_SetOrigin( pev, pev->origin );// link into world.
	UTIL_SetSize(pev, Vector(-5,-5,0), Vector(5,5,2));
}

// Call after animation/pose is set up
void CBaseMonster::MonsterInitDead( void )
{
	InitBoneControllers();

	pev->solid		= SOLID_BBOX;
	pev->movetype		= MOVETYPE_TOSS;// so he'll fall to ground
	pev->takedamage     = DAMAGE_YES;

	pev->frame = 0;
	ResetSequenceInfo();
	pev->framerate = 0;

	// Copy health
	pev->max_health		= pev->health;
	pev->deadflag		= DEAD_DEAD;

	UTIL_SetSize( pev, g_vecZero, g_vecZero );
	UTIL_SetOrigin( pev, pev->origin );

	// Setup health counters, etc.
	BecomeDead();
	SetThink( &CBaseMonster::CorpseFallThink );
	pev->nextthink = gpGlobals->time + 0.5f;
}

//=========================================================
// BBoxIsFlat - check to see if the monster's bounding box
// is lying flat on a surface (traces from all four corners
// are same length.)
//=========================================================
BOOL CBaseMonster::BBoxFlat( void )
{
	TraceResult tr;
	Vector vecSrc = pev->origin + Vector(0,0,1);
	Vector vecEnd = vecSrc + Vector(0,0,40);
	UTIL_TraceLine( vecSrc, vecEnd, ignore_monsters, ENT( pev ), &tr );
	if( tr.flFraction != 1.0 )
	{
		return FALSE;
	}
	return TRUE;
}

//=========================================================
// Get Enemy - tries to find the best suitable enemy for the monster.
//=========================================================
BOOL CBaseMonster::GetEnemy( void )
{
	CBaseEntity *pNewEnemy;

	if( HasConditions( bits_COND_SEE_HATE | bits_COND_SEE_DISLIKE | bits_COND_SEE_NEMESIS ) )
	{
		pNewEnemy = BestVisibleEnemy();

		if( pNewEnemy != m_hEnemy && pNewEnemy != NULL )
		{
			// DO NOT mess with the monster's m_hEnemy pointer unless the schedule the monster is currently running will be interrupted
			// by COND_NEW_ENEMY. This will eliminate the problem of monsters getting a new enemy while they are in a schedule that doesn't care,
			// and then not realizing it by the time they get to a schedule that does. I don't feel this is a good permanent fix. 

			if( m_pSchedule )
			{
				if( m_pSchedule->iInterruptMask & bits_COND_NEW_ENEMY )
				{
					PushEnemy( m_hEnemy, m_vecEnemyLKP );
					SetConditions( bits_COND_NEW_ENEMY );
					m_hEnemy = pNewEnemy;
					m_vecEnemyLKP = m_hEnemy->pev->origin;
				}
				// if the new enemy has an owner, take that one as well
				if( pNewEnemy->pev->owner != NULL )
				{
					CBaseEntity *pOwner = GetMonsterPointer( pNewEnemy->pev->owner );
					if( pOwner && ( pOwner->pev->flags & FL_MONSTER ) && IRelationship( pOwner ) != R_NO )
						PushEnemy( pOwner, m_vecEnemyLKP );
				}
			}
		}
	}

	// remember old enemies
	if( m_hEnemy == 0 && PopEnemy() )
	{
		if( m_pSchedule )
		{
			if( m_pSchedule->iInterruptMask & bits_COND_NEW_ENEMY )
			{
				SetConditions( bits_COND_NEW_ENEMY );
			}
		}
	}

	if( m_hEnemy != 0 )
	{
		if(m_thinkspeed == -1)
		{
			m_thinkspeed = 0;
		}
		// monster has an enemy.
		return TRUE;
	}

	return FALSE;// monster has no enemy
}

//=========================================================
// DropItem - dead monster drops named item 
//=========================================================
CBaseEntity *CBaseMonster::DropItem( const char *pszItemName, const Vector &vecPos, const Vector &vecAng )
{
	if( !pszItemName )
	{
		ALERT( at_console, "DropItem() - No item name!\n" );
		return NULL;
	}

	CBaseEntity *pItem = CBaseEntity::Create( pszItemName, vecPos, vecAng, edict() );

	if( pItem )
	{
		// do we want this behavior to be default?! (sjb)
		pItem->pev->velocity = pev->velocity;
		pItem->pev->avelocity = Vector( 0, RANDOM_FLOAT( 0, 100 ), 0 );

		// Dropped items should never respawn (unless this rule changes in the future). - Solokiller
		pItem->pev->spawnflags |= SF_NORESPAWN;
		return pItem;
	}
	else
	{
		ALERT( at_console, "DropItem() - Didn't create!\n" );
		return FALSE;
	}
}

BOOL CBaseMonster::ShouldFadeOnDeath( void )
{
	// if flagged to fade out or I have an owner (I came from a monster spawner)
	if( ( pev->spawnflags & SF_MONSTER_FADECORPSE ) || m_diefadeout >= 1 )
		return TRUE;

	return FALSE;
}
