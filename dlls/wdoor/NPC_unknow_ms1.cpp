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
#include	"player.h"

extern DLL_GLOBAL int		g_iSkillLevel;

//=========================================================
// Monster's Anim Events Go Here
//=========================================================

class CUnknowMs1 : public CBaseMonster
{
public:
	void Spawn( void );
	void Precache( void );
	void SetYawSpeed( void );
	int  Classify ( void );
	void HandleAnimEvent( MonsterEvent_t *pEvent );

	void Killed( entvars_t *pevAttacker, int iGib );

	void RunAI( void );

	EHANDLE	m_hEnemyBite;

	int bite_origin_move;

	int	Save( CSave &save ); 
	int Restore( CRestore &restore );

	static TYPEDESCRIPTION m_SaveData[];

	// No range attacks
	BOOL CheckRangeAttack1 ( float flDot, float flDist ) { return FALSE; }
	BOOL CheckRangeAttack2 ( float flDot, float flDist ) { return FALSE; }
	BOOL CheckMeleeAttack1 ( float flDot, float flDist );
	int TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType );

};

LINK_ENTITY_TO_CLASS( monster_else_doorhand, CUnknowMs1 );


TYPEDESCRIPTION	CUnknowMs1::m_SaveData[] = 
{
	DEFINE_FIELD( CUnknowMs1, m_hEnemyBite, FIELD_EHANDLE ),
};

IMPLEMENT_SAVERESTORE( CUnknowMs1, CBaseMonster );


void CUnknowMs1 :: RunAI( void )
{
	CBaseMonster :: RunAI();

	if(m_hEnemyBite == NULL && pev->takedamage && pev->sequence == LookupActivity ( ACT_IDLE )){
	pev->takedamage = DAMAGE_NO;
	pev->flags |= FL_NOTARGET;
	UTIL_SetSize( pev, Vector(0, 0, 0), Vector(0, 0, 0) );
	}

	if(m_hEnemyBite != NULL){
		if(pev->sequence != LookupActivity ( ACT_EAT )){
		SetActivity ( ACT_EAT );
		}
		else if(bite_origin_move == 0){
			m_hEnemyBite->pev->velocity.z = 0;
			UTIL_MakeVectors ( pev->angles ); 
			Vector vecArmPos,vecArmDir;
			GetAttachment( 1, vecArmPos, vecArmDir );
			if ( m_hEnemyBite->pev->flags & FL_CLIENT ){
			vecArmPos = vecArmPos + gpGlobals->v_forward * 10;
			}
			else{
			bite_origin_move += 1;
			}
			UTIL_SetOrigin ( m_hEnemyBite->pev, Vector(vecArmPos.x,vecArmPos.y,m_hEnemyBite->pev->origin.z + 2) );
		}

				if ( m_hEnemyBite->pev->flags & FL_CLIENT ){
								CBasePlayer *player = GetClassPtr((CBasePlayer *)m_hEnemyBite->pev);
								if( !IsAlive() ){
								m_hEnemyBite = NULL;
								player->m_barnacle_RTP_relase = 1;
								player->m_barnacle_RTP = 0;
								player->m_barnacle_RTP_bar = 0;
								player->m_barnacle_Level = 0;
								player->m_barnacle_catchme = NULL;
								}
								else if( !player->IsAlive() ){
								m_hEnemyBite = NULL;
								SetActivity ( ACT_SMALL_FLINCH );
								}
								else if(player->m_barnacle_RTP_bar >= 255){
									m_hEnemyBite = NULL;
									player->m_barnacle_RTP_relase = 1;
									player->m_barnacle_RTP = 0;
									player->m_barnacle_RTP_bar = 0;
									player->m_barnacle_Level = 0;
									player->m_barnacle_catchme = NULL;
									player->pev->velocity = player->pev->velocity + (player->pev->origin - pev->origin).Normalize() * 150;
									player->pev->velocity.z += 100;
									if(pev->health > 20){
									pev->health -= 20;
									SetActivity ( ACT_SMALL_FLINCH );
									}
									else{
									Killed( pev, GIB_NORMAL );
									}
								}
				}
				else{
					CBaseMonster *pEnemyMonster;
					pEnemyMonster = m_hEnemyBite->MyMonsterPointer();
					if( !pEnemyMonster->IsAlive() ){
					m_hEnemyBite = NULL;
					SetActivity ( ACT_SMALL_FLINCH );
					}
					else if( !IsAlive() ){
					m_hEnemyBite = NULL;
					pEnemyMonster->BarnacleVictimReleased();
					}
					else{
					m_hEnemyBite->TakeDamage ( pev, pev, 1, DMG_SLASH );
					}
				}
		}
	

}

BOOL CUnknowMs1 :: CheckMeleeAttack1 ( float flDot, float flDist )
{
	int dist = 80;
	if(pev->frags == 1){
	dist = 120;
	}
	// Decent fix to keep folks from kicking/punching hornets and snarks is to check the onground flag(sjb)
	if ( flDist <= 80 && flDot >= 0.5 && m_hEnemy != NULL && m_hEnemyBite == NULL && pev->sequence == LookupActivity ( ACT_IDLE ) )
	{
		return TRUE;
	}
	return FALSE;
}

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int	CUnknowMs1 :: Classify ( void )
{
	return	CLASS_HUMAN_ASS;
}


//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CUnknowMs1 :: SetYawSpeed ( void )
{
	pev->yaw_speed = 0;
}

int CUnknowMs1 :: TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	return CBaseMonster::TakeDamage( pevInflictor, pevAttacker, flDamage, bitsDamageType );
}

void CUnknowMs1::Killed( entvars_t *pevAttacker, int iGib )
{
	CBaseMonster::Killed( pevAttacker, GIB_NEVER );
}



//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CUnknowMs1 :: HandleAnimEvent( MonsterEvent_t *pEvent )
{
	switch( pEvent->event )
	{
		case 1:
		{

			if(!pev->takedamage && pev->sequence != LookupActivity ( ACT_IDLE )){
			pev->takedamage = DAMAGE_AIM;
			pev->flags &= ~FL_NOTARGET;
			UTIL_SetSize( pev, Vector(-32, -32, 48), Vector(32, 32, 72) );
			}

			//===============抓捕.直球=================//
			TraceResult tr;
			UTIL_MakeVectors(pev->angles);
			Vector vecSrc	= pev->origin + EyePosition();
			Vector vecEnd	= vecSrc + gpGlobals->v_forward * 80;

			if(pev->frags == 1){
			vecEnd	= vecSrc + gpGlobals->v_forward * 128;
			}

			UTIL_TraceLine( vecSrc, vecEnd, dont_ignore_monsters, ENT( pev ), &tr );

			CBaseEntity *pEntity = CBaseEntity::Instance(tr.pHit);

			if ( tr.flFraction < 1.0 ){
				if(pEntity){
					if( (pEntity->pev->flags & (FL_MONSTER|FL_CLIENT)) && pEntity->pev->deadflag == DEAD_NO
						&& pEntity->pev->takedamage){
						if(IRelationship( pEntity ) > R_NO){
							if(pEntity->Classify() != CLASS_PLAYER_BIOWEAPON
							&& pEntity->Classify() != CLASS_ALIEN_BIOWEAPON
							&& pEntity->Classify() != CLASS_MACHINE 
							&& pEntity->Classify() != CLASS_MACHINE_ASS
							&& pEntity->Classify() != CLASS_MACHINE_BLACK){
								if ( pEntity->IsPlayer() ){
									CBasePlayer *player = GetClassPtr((CBasePlayer *)pEntity->pev);
									if(player->pev->health > 0 && player->m_barnacle_RTP == 0 
									&& player->m_barnacle_god_time <= gpGlobals->time && player->pev->movetype == MOVETYPE_WALK){
												m_hEnemyBite = pEntity;
												SetActivity ( ACT_EAT );
												player->m_barnacle_RTP = 1;
												player->m_barnacle_Level = 2;
												player->m_barnacle_catchme = this;
												player->pev->punchangle.x += RANDOM_FLOAT(-25, 25);
												player->pev->punchangle.y += RANDOM_FLOAT(-25, 25);
												player->pev->punchangle.z += RANDOM_FLOAT(-25, 25);
												bite_origin_move = 0;
												return;
									}
								}
								else{
										CBaseMonster *pEnemyMonster;
										pEnemyMonster = pEntity->MyMonsterPointer();
										if(pEnemyMonster->pev->health > 0 && pEnemyMonster->m_IdealMonsterState != MONSTERSTATE_PRONE
										&& pEnemyMonster->m_MonsterState != MONSTERSTATE_PRONE){
										m_hEnemyBite = pEntity;
										pEnemyMonster->BarnacleVictimBitten( pev );
										pEnemyMonster->FBecomeProne();
										SetActivity ( ACT_EAT );
										bite_origin_move = 0;
										return;
										}
								}
							}
						}
					}
				}
			}


			//===============抓捕.范围=================//
			Vector vecArmPos,vecArmDir;
			GetAttachment( 1, vecArmPos, vecArmDir );

			Vector mins = vecArmPos - Vector( 16, 16, 16 );
			Vector maxs = vecArmPos + Vector( 16, 16, 16 );

			if(pev->frags == 1){
			mins = vecArmPos - Vector( 24, 24, 24 );
			maxs = vecArmPos + Vector( 24, 24, 24 );
			}

			CBaseEntity *pList[10];
			int count = UTIL_EntitiesInBox( pList, 10, mins, maxs, (FL_CLIENT|FL_MONSTER) );
			if ( count )
			{
				for ( int i = 0; i < count; i++ )
				{
					if ( pList[i] != this && IRelationship( pList[i] ) > R_NO && pList[ i ]->pev->deadflag == DEAD_NO )	// this ent is one of our enemies. Barnacle tries to eat it.
					{
						if(pList[i]->Classify() != CLASS_PLAYER_BIOWEAPON
						&& pList[i]->Classify() != CLASS_ALIEN_BIOWEAPON
						&& pList[i]->Classify() != CLASS_MACHINE 
						&& pList[i]->Classify() != CLASS_MACHINE_ASS
						&& pList[i]->Classify() != CLASS_MACHINE_BLACK){
							if ( pList[i]->IsPlayer() ){
								CBasePlayer *player = GetClassPtr((CBasePlayer *)pList[i]->pev);
								if(player->pev->health > 0 && player->m_barnacle_RTP == 0 
								&& player->m_barnacle_god_time <= gpGlobals->time && player->pev->movetype == MOVETYPE_WALK){
											m_hEnemyBite = pList[i];
											SetActivity ( ACT_EAT );
											player->m_barnacle_RTP = 1;
											player->m_barnacle_Level = 2;
											player->m_barnacle_catchme = this;
											player->pev->punchangle.x += RANDOM_FLOAT(-25, 25);
											player->pev->punchangle.y += RANDOM_FLOAT(-25, 25);
											player->pev->punchangle.z += RANDOM_FLOAT(-25, 25);
											bite_origin_move = 0;
											return;
								}
							}
							else{
									CBaseMonster *pEnemyMonster;
									pEnemyMonster = pList[i]->MyMonsterPointer();
									if(pEnemyMonster->pev->health > 0 && pEnemyMonster->m_IdealMonsterState != MONSTERSTATE_PRONE
									&& pEnemyMonster->m_MonsterState != MONSTERSTATE_PRONE){
									m_hEnemyBite = pList[i];
									pEnemyMonster->BarnacleVictimBitten( pev );
									pEnemyMonster->FBecomeProne();
									SetActivity ( ACT_EAT );
									bite_origin_move = 0;
									return;
									}
							}
						}
					}
				}
			}
			//====================================//
		}
		break;

		case 2:
			{
				Vector vecGunPos,vecGunAngles;
				GetAttachment( 0, vecGunPos, vecGunAngles );
				FX_Explosion( vecGunPos, 236 );
				SpawnBlood(vecGunPos, BloodColor(), 100);
				FX_Trail(vecGunPos, entindex(), PROJ_GUTS );
				EMIT_SOUND(ENT(pev), CHAN_BODY, "newadd/zom_headburst.wav", 1, ATTN_NORM);	
				UTIL_SetSize( pev, Vector(0, 0, 0), Vector(0, 0, 0) );
			}
			break;

		case 3:
			{
				if(pev->weapons == 1){
				FireTargets( "break_door_hand_all", this, this, USE_TOGGLE, 0 );
				}
				FadeMonster();
			}
			break;

		case 4:
			{
				EMIT_SOUND(ENT(pev), CHAN_WEAPON, "newadd/handstrike1.wav", 0.9, ATTN_NORM);
			}
			break;

		case 5:
			{
				if(pev->sequence != LookupActivity ( ACT_EAT )){
				SetActivity ( ACT_IDLE );
				}
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
void CUnknowMs1 :: Spawn()
{
	Precache( );
	SET_MODEL(ENT(pev), "models/handstrike.mdl");
	UTIL_SetSize( pev, Vector(0, 0, 0), Vector(0, 0, 0) );

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_NONE;
	m_bloodColor		= BLOOD_COLOR_RED;

	if (g_iSkillLevel == SKILL_HARD){
	pev->health			= 60;
	}
	else{
	pev->health			= 40;
	}

	pev->view_ofs		= VEC_VIEW;// position of the eyes relative to monster's origin.
	m_flFieldOfView		= 0;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState		= MONSTERSTATE_NONE;
	m_afCapability		= bits_CAP_DOORS_GROUP;

	MonsterInit();

	m_ignoredamage = 1;
	m_longming = 1;
	m_facing_fucking_mode = 1;
	m_killed_exp = 10;

	if(pev->frags == 1){//加强版
	pev->health	= 60;
	}
	m_rpgms_level = 15;

	pev->netname = MAKE_STRING( "Black.Hand" );

	m_singdelay_max = 0;//0反应
	m_singdelay_use = m_singdelay_max;
}
//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CUnknowMs1 :: Precache()
{
	PRECACHE_MODEL("models/handstrike.mdl");
	PRECACHE_SOUND ("newadd/handstrike1.wav");
}	
