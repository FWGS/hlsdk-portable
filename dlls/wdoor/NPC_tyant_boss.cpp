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
#include	"game.h"

extern DLL_GLOBAL int		g_iSkillLevel;

//=========================================================
// Monster's Anim Events Go Here
//=========================================================

class CTyantBOSS : public CBaseMonster
{
public:
	void Spawn( void );
	void Precache( void );
	void SetYawSpeed( void );
	int  Classify ( void );
	void HandleAnimEvent( MonsterEvent_t *pEvent );

	float m_flNextFlinch;
	float m_flNextPainTime;

	void Killed( entvars_t *pevAttacker, int iGib );
	void RunAI( void );

	Schedule_t *GetScheduleOfType ( int Type );
	Schedule_t *GetSchedule ( void );

	void AlertSound( void );

	void TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType);
	void BloodSonic( int damage );
	
	void EXPORT LeapTouch ( CBaseEntity *pOther );

	BOOL FCanCheckAttacks ( void );

	// No range attacks
	BOOL CheckRangeAttack1 ( float flDot, float flDist );
	BOOL CheckRangeAttack2 ( float flDot, float flDist );
	BOOL CheckMeleeAttack1 ( float flDot, float flDist );
	BOOL CheckMeleeAttack2 ( float flDot, float flDist ) { return FALSE; }

	int TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType );

	int IRelationship ( CBaseEntity *pTarget );

	int	Save( CSave &save ); 
	int Restore( CRestore &restore );

	static TYPEDESCRIPTION m_SaveData[];

	int m_iSpriteTexture;

	EHANDLE	m_hEnemyBite;

	int m_JumpUse;
	int m_Miss;
	float m_flNextShakeTime;
	float m_flNextCatchTime;
	float m_flPlayerDamage;
	float m_flPlayerDamage2;

	Vector m_henemyoldorigin;
};

LINK_ENTITY_TO_CLASS( monster_tyant_boss, CTyantBOSS );

TYPEDESCRIPTION	CTyantBOSS::m_SaveData[] = 
{
	DEFINE_FIELD( CTyantBOSS, m_JumpUse, FIELD_INTEGER ),
	DEFINE_FIELD( CTyantBOSS, m_Miss, FIELD_INTEGER ),
	DEFINE_FIELD( CTyantBOSS, m_flNextShakeTime, FIELD_TIME ),
	DEFINE_FIELD( CTyantBOSS, m_flNextCatchTime, FIELD_TIME ),
	DEFINE_FIELD( CTyantBOSS, m_hEnemyBite, FIELD_EHANDLE ),
	DEFINE_FIELD( CTyantBOSS, m_flPlayerDamage, FIELD_FLOAT ),
	DEFINE_FIELD( CTyantBOSS, m_flPlayerDamage2, FIELD_FLOAT ),
	DEFINE_FIELD( CTyantBOSS, m_henemyoldorigin, FIELD_VECTOR ),
};

IMPLEMENT_SAVERESTORE( CTyantBOSS, CBaseMonster );

void CTyantBOSS :: AlertSound( void )
{
	EMIT_SOUND_DYN ( ENT(pev), CHAN_VOICE, "tyant_boss/roar.wav", 1.0, ATTN_NORM, 0, 100 );
}

int CTyantBOSS::IRelationship ( CBaseEntity *pTarget )
{
	if ( FClassnameIs( pTarget->pev, "player" ) && m_flPlayerDamage2 >= 60 && pev->weapons != 1 )
	{
		if(m_alert == 0)
		m_alert = 100;

		return R_NM;
	}

	return CBaseMonster::IRelationship( pTarget );
}


BOOL CTyantBOSS :: FCanCheckAttacks ( void )
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

void CTyantBOSS :: RunAI( void )
{
	CBaseMonster :: RunAI();
	if(m_flPlayerDamage2 > 0){
		if(m_flPlayerDamage2 > 90){//��ҳ���Ч��
		m_flPlayerDamage2 = 90;
		}
		else{
		m_flPlayerDamage2 -= 1;
		}
	}

	if(pev->yaw_speed == 0 && !IsMoving()){
	SetYawSpeed();//BUG Fix?
	}

	// flying?
	if ( pev->movetype == MOVETYPE_TOSS)
	{
		if (pev->flags & FL_ONGROUND)
		{
			pev->movetype = MOVETYPE_STEP;
		}
	}

	if(pev->sequence == LookupActivity ( ACT_WALK ) || pev->sequence == LookupActivity ( ACT_WALK_SCARED )){
		m_flGroundSpeed = 125;
		if (!(pev->flags & FL_ONGROUND)){
		DROP_TO_FLOOR ( ENT(pev) );
		}
	}

	if(m_hEnemyBite != NULL){
		if(pev->sequence != LookupActivity ( ACT_MELEE_ATTACK2 )){
		SetActivity ( ACT_MELEE_ATTACK2 );
		}
		UTIL_MakeVectors(pev->angles);

		DROP_TO_FLOOR( ENT( pev ) );
		if(m_hEnemyBite->pev->flags & FL_CLIENT ){
		Vector CatchOrigin = Center() + gpGlobals->v_forward * 48 + Vector(0,0,16);
		TraceResult tr;
		UTIL_TraceLine(Center() + Vector(0,0,36), CatchOrigin + gpGlobals->v_forward * 16, ignore_monsters, edict(), &tr);
		if (tr.flFraction < 1.0){
		CatchOrigin = tr.vecEndPos + (tr.vecPlaneNormal * 16);
		pev->velocity = gpGlobals->v_forward * -64;
		}
		UTIL_SetOrigin ( m_hEnemyBite->pev, CatchOrigin );
		//m_hEnemyBite->pev->origin = CatchOrigin;
		m_hEnemyBite->pev->velocity = g_vecZero;

						CBasePlayer *player = GetClassPtr((CBasePlayer *)m_hEnemyBite->pev);
						if( !IsAlive() ){
						m_hEnemyBite = NULL;
						player->m_barnacle_RTP_relase = 1;
						player->m_barnacle_draw_time = gpGlobals->time + 0.5;//0.5����ǹ���
						player->m_barnacle_RTP = 0;
						player->m_barnacle_RTP_bar = 0;
						player->m_barnacle_Level = 0;
						player->m_barnacle_catchme = NULL;
						}
						else if(player->m_barnacle_RTP_bar >= 255){
						m_hEnemyBite = NULL;
						player->m_barnacle_RTP_relase = 1;
						player->m_barnacle_draw_time = gpGlobals->time + 0.5;//0.5����ǹ���
						player->m_barnacle_RTP = 0;
						player->m_barnacle_RTP_bar = 0;
						player->m_barnacle_Level = 0;
						player->m_barnacle_catchme = NULL;
						SetActivity ( ACT_SMALL_FLINCH );
						player->pev->velocity = (player->pev->origin - pev->origin).Normalize() * 200;
						player->m_flVelocityModifier = 0;
						player->pev->velocity.z += 100;
						}
						else if(!player->IsAlive()){
						m_hEnemyBite = NULL;
						player->m_barnacle_RTP = 0;
						player->m_barnacle_RTP_bar = 0;
						player->m_barnacle_Level = 0;
						player->m_barnacle_catchme = NULL;
						}
		}
		else{

		Vector CatchOrigin = Center() + gpGlobals->v_forward * 48 + Vector(0,0,16);
		TraceResult tr;
		UTIL_TraceLine(Center() + Vector(0,0,36), CatchOrigin + gpGlobals->v_forward * 16, ignore_monsters, edict(), &tr);
		if (tr.flFraction < 1.0){
		CatchOrigin = tr.vecEndPos + (tr.vecPlaneNormal * 16);
		pev->velocity = gpGlobals->v_forward * -64;
		}

		CatchOrigin.z -= 40;

		UTIL_SetOrigin ( m_hEnemyBite->pev, CatchOrigin );
		//m_hEnemyBite->pev->origin = CatchOrigin;
		m_hEnemyBite->pev->velocity = g_vecZero;

			CBaseMonster *pEnemyMonster;
			pEnemyMonster = m_hEnemyBite->MyMonsterPointer();
			if( !pEnemyMonster->IsAlive() ){
			m_hEnemyBite = NULL;
			}
			else if( !IsAlive() ){
			m_hEnemyBite = NULL;
			pEnemyMonster->BarnacleVictimReleased();
			}
			else if( m_flPlayerDamage >= 150 ){
			m_hEnemyBite = NULL;
			pEnemyMonster->BarnacleVictimReleased();
			SetActivity ( ACT_SMALL_FLINCH );
			m_flPlayerDamage = 0;
			}
		}

	}
	else if(pev->sequence == LookupActivity ( ACT_RANGE_ATTACK1 )){
		if(m_hEnemyBite == NULL && m_JumpUse == 1){
			UTIL_MakeVectors ( pev->angles );
			if ( m_hEnemy != NULL ){

			//===============ץ��=================//
			Vector vecArmPos,vecArmDir;
			GetAttachment( 0, vecArmPos, vecArmDir );

			UTIL_Sparks( vecArmPos );

			Vector mins = vecArmPos - Vector( 32, 32, 32 );
			Vector maxs = vecArmPos + Vector( 32, 32, 32 );

			CBaseEntity *pList[10];
			int count = UTIL_EntitiesInBox( pList, 10, mins, maxs, (FL_CLIENT|FL_MONSTER) );
			if ( count )
			{
				for ( int i = 0; i < count; i++ )
				{
					if ( pList[i] != this && IRelationship( pList[i] ) > R_NO && pList[ i ]->pev->deadflag == DEAD_NO
					&& pList[ i ]->pev->takedamage)	// this ent is one of our enemies. Barnacle tries to eat it.
					{
						if(pList[i]->Classify() != CLASS_PLAYER_BIOWEAPON
						&& pList[i]->Classify() != CLASS_ALIEN_BIOWEAPON
						&& pList[i]->Classify() != CLASS_MACHINE 
						&& pList[i]->Classify() != CLASS_MACHINE_ASS
						&& pList[i]->Classify() != CLASS_MACHINE_BLACK){
							if ( pList[i]->IsPlayer() ){
								CBasePlayer *player = GetClassPtr((CBasePlayer *)pList[i]->pev);
								if(player->pev->origin.z - 90 > pev->origin.z){
								continue;//��ץ�ߴ��ĵ���
								}
								if(player->pev->health > 0 && player->m_barnacle_RTP == 0 
								&& player->m_barnacle_god_time <= gpGlobals->time && player->pev->movetype == MOVETYPE_WALK){
											m_hEnemyBite = pList[i];
											SetActivity ( ACT_MELEE_ATTACK2 );
											player->m_barnacle_RTP = 1;
											player->m_barnacle_Level = 0;
											player->m_barnacle_catchme = this;
											player->pev->punchangle.x += RANDOM_FLOAT(-30, 30);
											player->pev->punchangle.y += RANDOM_FLOAT(-30, 30);
											player->pev->punchangle.z += RANDOM_FLOAT(-30, 30);
											player->m_flNextAttack = UTIL_WeaponTimeBase() + 0.3;
											m_flPlayerDamage = 0;
											m_flNextCatchTime = gpGlobals->time + 10.0;
											m_henemyoldorigin = player->pev->origin;

											pev->movetype = MOVETYPE_STEP;
											pev->velocity = gpGlobals->v_forward * -200;
											break;
								}
							}
							else{
									CBaseMonster *pEnemyMonster;
									pEnemyMonster = pList[i]->MyMonsterPointer();
									if(pEnemyMonster->pev->origin.z - 50 > pev->origin.z){
									continue;//��ץ�ߴ��ĵ���
									}
									if(pEnemyMonster->pev->health > 0 && pEnemyMonster->m_IdealMonsterState != MONSTERSTATE_PRONE
									&& pEnemyMonster->m_MonsterState != MONSTERSTATE_PRONE
									&& pEnemyMonster->m_canbarnacle_mode == 1){
									m_hEnemyBite = pList[i];
									pEnemyMonster->BarnacleVictimBitten( pev );
									pEnemyMonster->FBecomeProne();
									SetActivity ( ACT_MELEE_ATTACK2 );
									m_flPlayerDamage = 0;
									m_flNextCatchTime = gpGlobals->time + 10.0;
									m_henemyoldorigin = pEnemyMonster->pev->origin;

									pev->movetype = MOVETYPE_STEP;
									pev->velocity = gpGlobals->v_forward * -200;
									break;
									}
							}
						}
					}
				}
			}
			//====================================//
			

			}
		}
	}

}

BOOL CTyantBOSS :: CheckMeleeAttack1 ( float flDot, float flDist )
{
	float dist = 75;
	if(m_hEnemy != NULL){
		if ( (m_hEnemy->pev->flags & FL_MONSTER) && fabs( pev->origin.z - m_hEnemy->pev->origin.z ) > 40 ){
			dist += 25;
		}
		else if ( (m_hEnemy->pev->flags & FL_CLIENT) && fabs( pev->origin.z - m_hEnemy->pev->origin.z ) > 70 ){
			dist += 25;
			TraceResult tr;
			UTIL_MakeVectors(pev->angles);
			Vector vecSrc = Center();
			Vector vecEnd	= vecSrc + gpGlobals->v_forward * 50;
			UTIL_TraceLine( vecSrc, vecEnd, dont_ignore_monsters, ENT( pev ), &tr );

			CBaseEntity *pEntity = CBaseEntity::Instance(tr.pHit);
			if ( tr.flFraction < 1.0 && pEntity->pev->takedamage ){
				if ( FClassnameIs( pEntity->pev, "func_breakable" )){
					if(pev->sequence == LookupActivity ( ACT_RANGE_ATTACK1 ) && m_JumpUse == 1){
					ClearMultiDamage( );
					pEntity->TraceAttack(pev, 5, gpGlobals->v_forward, &tr, DMG_SLASH ); 
					ApplyMultiDamage( pev, pev );
					}
					dist += 150;
				}
			}
		}
	}

		

	if ( flDist <= dist && flDot >= 0.75 && m_hEnemy != NULL )
	{
		return TRUE;
	}
	return FALSE;
}

BOOL CTyantBOSS :: CheckRangeAttack1 ( float flDot, float flDist )
{
	if ( flDist >= 120 && flDist <= 960 && flDot >= 0.6 && m_hEnemy != NULL && m_flNextCatchTime <= gpGlobals->time)
	{
		return TRUE;
	}

	return FALSE;
}

BOOL CTyantBOSS :: CheckRangeAttack2 ( float flDot, float flDist )
{
	if(pev->weapons == 1){
	return FALSE;
	}

	float dist = 300;
	if(m_hEnemy != NULL){
		if ( (m_hEnemy->pev->flags & FL_MONSTER) && fabs( pev->origin.z - m_hEnemy->pev->origin.z ) > 40 ){
			dist += 60;
		}
		else if ( (m_hEnemy->pev->flags & FL_CLIENT) && fabs( pev->origin.z - m_hEnemy->pev->origin.z ) > 70 ){
			dist += 60;
		}
	}

	if(flDist <= dist && m_flNextShakeTime <= gpGlobals->time 
	&& m_hEnemy != NULL && !HasConditions(bits_COND_CAN_RANGE_ATTACK1) ){
		m_facing_fucking_mode = 1;
		return TRUE;
	}

	m_facing_fucking_mode = 0;
	return FALSE;
}


//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int	CTyantBOSS :: Classify ( void )
{
	return	CLASS_HUMAN_ASS;
}

void CTyantBOSS::TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType)
{
	if(ptr->iHitgroup == 1){
	m_bloodColor		= BLOOD_COLOR_RED;
	}
	else if(ptr->iHitgroup == 4 || ptr->iHitgroup == 5){
	m_bloodColor		= BLOOD_COLOR_RED;
		if ( (bitsDamageType & DMG_BULLET) || (bitsDamageType & DMG_CLUB) 
		|| (bitsDamageType & DMG_ENERGYBEAM)){
		flDamage *= 0.5;
		}
	}
	else{
		if ( (bitsDamageType & DMG_BULLET) || (bitsDamageType & DMG_CLUB) 
		|| (bitsDamageType & DMG_ENERGYBEAM) || (bitsDamageType & DMG_SLASH)){
			if(flDamage <= 60){
			flDamage *= 0.1;
			m_bloodColor		= DONT_BLEED;
			}
			else{//�Ʒ�
			flDamage *= 0.3;
			m_bloodColor		= BLOOD_COLOR_RED;
			}
		}
		else{
			if(flDamage <= 60){
			m_bloodColor		= DONT_BLEED;
			flDamage *= 0.5;
			}
			else{//�Ʒ�
			m_bloodColor		= BLOOD_COLOR_RED;
			}
		}

		UTIL_Ricochet( ptr->vecEndPos, RANDOM_FLOAT( 1, 2));
	}

	CBaseMonster::TraceAttack( pevAttacker, flDamage, vecDir, ptr, bitsDamageType );
}

Schedule_t* CTyantBOSS :: GetScheduleOfType ( int Type )
{
	return CBaseMonster::GetScheduleOfType( Type );
}

//=========================================================
// GetSchedule - Decides which type of schedule best suits
// the monster's current state and conditions. Then calls
// monster's member function to get a pointer to a schedule
// of the proper type.
//=========================================================
Schedule_t *CTyantBOSS :: GetSchedule ( void )
{
	return CBaseMonster::GetSchedule();
}
//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CTyantBOSS :: SetYawSpeed ( void )
{
	int ys;

	switch ( m_Activity )
	{
	case ACT_MELEE_ATTACK2:	
		ys = 0;		
		break;
	case ACT_RANGE_ATTACK1:	
		ys = 180;	
		break;
	default:
		ys = 120;
		break;
	}

	pev->yaw_speed = ys;
}

int CTyantBOSS :: TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	if ( bitsDamageType == DMG_ENERGYBLAST || bitsDamageType == DMG_BLAST)
	{
			if( bitsDamageType & DMG_BLAST ){
				m_flPlayerDamage += flDamage * 2.0;
			}
			else if( bitsDamageType == DMG_ENERGYBLAST){
				m_flPlayerDamage += flDamage * 1.5;
			}
	}
	else{
		m_flPlayerDamage += flDamage;
	}

	if ( pevAttacker->flags & FL_CLIENT )
		m_flPlayerDamage2 += flDamage;

			if(pev->sequence == LookupActivity ( ACT_WALK )){
				if(m_flPlayerDamage >= 100){
					if(pev->weapons != 1){
					m_movementActivity = ACT_WALK_SCARED;
					pev->sequence = LookupActivity ( ACT_WALK_SCARED );
					ResetSequenceInfo( );
					pev->frame = 0;
					}
					m_flPlayerDamage = 0;
				}
			}

	return CBaseMonster::TakeDamage( pevInflictor, pevAttacker, flDamage, bitsDamageType );
}

void CTyantBOSS::Killed( entvars_t *pevAttacker, int iGib )
{
	CBaseMonster::Killed( pevAttacker, GIB_NEVER );
}

void CTyantBOSS :: BloodSonic ( int damage )
{
	EMIT_SOUND(ENT(pev), CHAN_WEAPON, "tyant_boss/blast.wav", 1, 0.6);

	// blast circles
	MESSAGE_BEGIN( MSG_PAS, SVC_TEMPENTITY, pev->origin );
		WRITE_BYTE( TE_BEAMCYLINDER );
		WRITE_COORD( pev->origin.x);
		WRITE_COORD( pev->origin.y);
		WRITE_COORD( pev->origin.z + 16);
		WRITE_COORD( pev->origin.x);
		WRITE_COORD( pev->origin.y);
		WRITE_COORD( pev->origin.z + 16 + 384 / .2); // reach damage radius over .3 seconds
		WRITE_SHORT( m_iSpriteTexture );
		WRITE_BYTE( 0 ); // startframe
		WRITE_BYTE( 0 ); // framerate
		WRITE_BYTE( 2 ); // life
		WRITE_BYTE( 16 );  // width
		WRITE_BYTE( 0 );   // noise

		WRITE_BYTE( 255   );
		WRITE_BYTE( 32 );
		WRITE_BYTE( 32  );

		WRITE_BYTE( 255 ); //brightness
		WRITE_BYTE( 0 );		// speed
	MESSAGE_END();

	MESSAGE_BEGIN( MSG_PAS, SVC_TEMPENTITY, pev->origin );
		WRITE_BYTE( TE_BEAMCYLINDER );
		WRITE_COORD( pev->origin.x);
		WRITE_COORD( pev->origin.y);
		WRITE_COORD( pev->origin.z + 16);
		WRITE_COORD( pev->origin.x);
		WRITE_COORD( pev->origin.y);
		WRITE_COORD( pev->origin.z + 16 + ( 384 / 2 ) / .2); // reach damage radius over .3 seconds
		WRITE_SHORT( m_iSpriteTexture );
		WRITE_BYTE( 0 ); // startframe
		WRITE_BYTE( 0 ); // framerate
		WRITE_BYTE( 2 ); // life
		WRITE_BYTE( 16 );  // width
		WRITE_BYTE( 0 );   // noise
		
		WRITE_BYTE( 255   );
		WRITE_BYTE( 32 );
		WRITE_BYTE( 32  );

		WRITE_BYTE( 255 ); //brightness
		WRITE_BYTE( 0 );		// speed
	MESSAGE_END();

	::RadiusDamage2( pev->origin, pev, pev, damage, 384, CLASS_HUMAN_ASS, DMG_SONIC);
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CTyantBOSS :: HandleAnimEvent( MonsterEvent_t *pEvent )
{
	int dmg1,dmg2,dmg3,dmg4;
	dmg1 = 40;//��ȭ
	dmg2 = 60;//��צ
	dmg3 = 120;//�󴩴�
	dmg4 = 40;//�𵴲�

	switch( pEvent->event )
	{
		case 1:
		{

			if(m_hEnemy != NULL){//ͷ�����˹���
				
				TraceResult tr;
				UTIL_MakeVectors(pev->angles);
				Vector vecSrc	= BodyTarget_o(pev->origin);
				Vector vecEnd	= m_hEnemy->Center();
				UTIL_TraceLine( vecSrc, vecEnd, dont_ignore_monsters, ENT( pev ), &tr );
				CBaseEntity *pEntity = CBaseEntity::Instance(tr.pHit);

				if ( tr.flFraction < 1.0 && pEntity->pev->takedamage ){
					if(( vecSrc - tr.vecEndPos).Length() <= 80){
							int tex = (int)TEXTURETYPE_Trace(&tr, vecSrc, vecEnd);
							int surface = (int)SURFACETYPE_Trace(&tr, vecSrc, vecEnd,Classify(),1);
							FX_ImpBullet( tr.vecEndPos, tr.vecPlaneNormal, vecSrc, surface, BULLET_CROWBAR, (float)tex );

							if ( pEntity->pev->flags & (FL_MONSTER|FL_CLIENT) ){
							pEntity->pev->velocity = pEntity->pev->velocity + (pEntity->pev->origin - pev->origin).Normalize() * 300;
							}

							ClearMultiDamage( );
							pEntity->TraceAttack(pev, dmg1, gpGlobals->v_forward, &tr, DMG_SLASH); 
							ApplyMultiDamage( pev, pev );
							EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, "newadd/fist_hitbod2.wav", 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
					}
				}

			}

		}
		break;

		case 2:
		{
			TraceResult tr;
			UTIL_MakeVectors(pev->angles);
			Vector vecSrc	= BodyTarget_o(pev->origin);
			Vector vecEnd	= vecSrc + gpGlobals->v_forward * 90;
			UTIL_TraceLine( vecSrc, vecEnd, dont_ignore_monsters, ENT( pev ), &tr );

			CBaseEntity *pEntity = CBaseEntity::Instance(tr.pHit);

			if ( tr.flFraction < 1.0 && pEntity->pev->takedamage ){
			int tex = (int)TEXTURETYPE_Trace(&tr, vecSrc, vecEnd);
			int surface = (int)SURFACETYPE_Trace(&tr, vecSrc, vecEnd,Classify(),1);
			FX_ImpBullet( tr.vecEndPos, tr.vecPlaneNormal, vecSrc, surface, BULLET_CROWBAR, (float)tex );

			ClearMultiDamage( );
			pEntity->TraceAttack(pev, dmg2, gpGlobals->v_forward, &tr, DMG_SLASH ); 
			ApplyMultiDamage( pev, pev );

				EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, "tyant_boss/slash.wav", 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );

				if ( pEntity->pev->flags & (FL_MONSTER|FL_CLIENT) ){
				pEntity->pev->velocity = (pEntity->pev->origin - pev->origin).Normalize() * 150;
				}
			}
			else{
				CBaseEntity *pHurt = CheckTraceHullAttack( 90, dmg2, DMG_SLASH );
				if(pHurt){
					if (pHurt->Classify() == CLASS_PLAYER || pHurt->Classify() == CLASS_PLAYER_ALLY
					|| pHurt->Classify() == CLASS_HUMAN_ASS || pHurt->Classify() == CLASS_HUMAN_PASSIVE
					|| pHurt->Classify() == CLASS_HUMAN_MILITARY){
					FX_Explosion( pHurt->Center(), 236 );
					}
					else if (pHurt->Classify() == CLASS_ALIEN_MONSTER || pHurt->Classify() == CLASS_ALIEN_MILITARY){
					FX_Explosion( pHurt->Center(), 237 );
					}
					EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, "tyant_boss/slash.wav", 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );

					if ( pHurt->pev->flags & (FL_MONSTER|FL_CLIENT) ){
					pHurt->pev->velocity = pHurt->pev->velocity + (pHurt->pev->origin - pev->origin).Normalize() * 150;
					}
				}
			}

		}
		break;

		case 3:
		{
			if(m_flNextShakeTime <= gpGlobals->time){
			BloodSonic(dmg4);
			m_flNextShakeTime = gpGlobals->time + 6.0;

			if(m_hEnemy != NULL){//ͷ�����˹���
				
				TraceResult tr;
				UTIL_MakeVectors(pev->angles);
				Vector vecSrc	= BodyTarget(pev->origin);
				Vector vecEnd	= m_hEnemy->Center();
				UTIL_TraceLine( vecSrc, vecEnd, dont_ignore_monsters, ENT( pev ), &tr );
				CBaseEntity *pEntity = CBaseEntity::Instance(tr.pHit);

				if ( tr.flFraction < 1.0 && pEntity->pev->takedamage ){
					if(( vecSrc - tr.vecEndPos).Length() <= 60){
							int tex = (int)TEXTURETYPE_Trace(&tr, vecSrc, vecEnd);
							int surface = (int)SURFACETYPE_Trace(&tr, vecSrc, vecEnd,Classify(),1);
							FX_ImpBullet( tr.vecEndPos, tr.vecPlaneNormal, vecSrc, surface, BULLET_CROWBAR, (float)tex );

							if ( pEntity->pev->flags & (FL_MONSTER|FL_CLIENT) ){
							pEntity->pev->velocity = pEntity->pev->velocity + (pEntity->pev->origin - pev->origin).Normalize() * 300;
							}

							ClearMultiDamage( );
							pEntity->TraceAttack(pev, dmg1, gpGlobals->v_forward, &tr, DMG_SLASH); 
							ApplyMultiDamage( pev, pev );
							EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, "newadd/fist_hitbod2.wav", 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
					}
				}

			}

			}
		}
		break;

		case 4:
		{
		MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
		WRITE_BYTE( TE_BEAMFOLLOW );
		WRITE_SHORT( entindex() + 0x1000 * 2 );		// entity, attachment
		WRITE_SHORT(g_sModelIndexTrail );	// model
		WRITE_BYTE( 3 ); // life
		WRITE_BYTE( 1 );  // width
		WRITE_BYTE( 255 );	// R
		WRITE_BYTE( 32 );	// G
		WRITE_BYTE( 32 );	// B
		WRITE_BYTE( 128 );	// brightness
	    MESSAGE_END();  // move PHS/PVS data sending into here (SEND_ALL, SEND_PVS, SEND_PHS)

		MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
		WRITE_BYTE( TE_BEAMFOLLOW );
		WRITE_SHORT( entindex() + 0x1000 * 3 );		// entity, attachment
		WRITE_SHORT(g_sModelIndexTrail );	// model
		WRITE_BYTE( 3 ); // life
		WRITE_BYTE( 1 );  // width
		WRITE_BYTE( 255 );	// R
		WRITE_BYTE( 32 );	// G
		WRITE_BYTE( 32 );	// B
		WRITE_BYTE( 128 );	// brightness
	    MESSAGE_END();  // move PHS/PVS data sending into here (SEND_ALL, SEND_PVS, SEND_PHS)

		MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
		WRITE_BYTE( TE_BEAMFOLLOW );
		WRITE_SHORT( entindex() + 0x1000 * 4 );		// entity, attachment
		WRITE_SHORT(g_sModelIndexTrail );	// model
		WRITE_BYTE( 3 ); // life
		WRITE_BYTE( 1 );  // width
		WRITE_BYTE( 255 );	// R
		WRITE_BYTE( 32 );	// G
		WRITE_BYTE( 32 );	// B
		WRITE_BYTE( 128 );	// brightness
	    MESSAGE_END();  // move PHS/PVS data sending into here (SEND_ALL, SEND_PVS, SEND_PHS)
		}
		break;

		case 5:
		{
			if(m_hEnemyBite != NULL){
				m_hEnemyBite->TakeDamage ( pev, pev, dmg3, DMG_SLASH);

				if (m_hEnemyBite->Classify() == CLASS_PLAYER || m_hEnemyBite->Classify() == CLASS_PLAYER_ALLY
				|| m_hEnemyBite->Classify() == CLASS_HUMAN_ASS || m_hEnemyBite->Classify() == CLASS_HUMAN_PASSIVE
				|| m_hEnemyBite->Classify() == CLASS_HUMAN_MILITARY){
				SpawnBlood(m_hEnemyBite->Center(), BLOOD_COLOR_RED, 200);
				FX_Explosion( m_hEnemyBite->Center(), 236 );
				}
				else if (m_hEnemyBite->Classify() == CLASS_ALIEN_MONSTER || m_hEnemyBite->Classify() == CLASS_ALIEN_MILITARY){
				SpawnBlood(m_hEnemyBite->Center(), BLOOD_COLOR_YELLOW, 200);
				FX_Explosion( m_hEnemyBite->Center(), 237 );
				}

				if(m_hEnemyBite->pev->flags & FL_CLIENT ){
					CBasePlayer *player = GetClassPtr((CBasePlayer *)m_hEnemyBite->pev);
					if(player->pev->health > 0){
								player->m_barnacle_RTP_relase = 1;
								player->m_barnacle_draw_time = gpGlobals->time + 0.5;//0.5����ǹ���
								player->m_barnacle_RTP = 0;
								player->m_barnacle_RTP_bar = 0;
								player->m_barnacle_Level = 0;
								player->m_barnacle_catchme = NULL;
						player->pev->velocity = player->pev->velocity + (player->pev->origin - pev->origin).Normalize() * 300;
						player->pev->velocity.z += 300;
						player->pev->punchangle.x += -60;
					}
				}
				else if(m_hEnemyBite->pev->health > 0){
					CBaseMonster *pEnemyMonster;
					pEnemyMonster = m_hEnemyBite->MyMonsterPointer();
					pEnemyMonster->BarnacleVictimReleased();
					pEnemyMonster->pev->velocity = pEnemyMonster->pev->velocity + (pEnemyMonster->pev->origin - pev->origin).Normalize() * 300;
					pEnemyMonster->pev->velocity.z += 300;
					pEnemyMonster->m_alert	= 100;
				}

				m_hEnemyBite = NULL;
				m_alert	= 100;

				Vector vecArmPos,vecArmDir;
				GetAttachment( 2, vecArmPos, vecArmDir );
				FX_Trail(vecArmPos, entindex(), PROJ_GUTS );

				EMIT_SOUND(ENT(pev), CHAN_WEAPON, "newadd/zom_headburst.wav", 1, 0.7);	
				m_flNextCatchTime = gpGlobals->time + 10.0;
			}
			
		}
		break;

		case 6:
		{
			m_JumpUse = 1;
			m_flNextCatchTime = gpGlobals->time + 10.0;

			MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
			WRITE_BYTE( TE_BEAMFOLLOW );
			WRITE_SHORT( entindex() + 0x1000 * 1 );		// entity, attachment
			WRITE_SHORT(g_sModelIndexTrail );	// model
			WRITE_BYTE( 5 ); // life
			WRITE_BYTE( 5 );  // width
			WRITE_BYTE( 255 );	// R
			WRITE_BYTE( 255 );	// G
			WRITE_BYTE( 255 );	// B
			WRITE_BYTE( 192 );	// brightness
			MESSAGE_END();  // move PHS/PVS data sending into here (SEND_ALL, SEND_PVS, SEND_PHS)

			UTIL_MakeVectors(pev->angles);
			ClearBits( pev->flags, FL_ONGROUND );
			//UTIL_SetOrigin (pev, pev->origin + Vector ( 0 , 0 , 1) );
			pev->movetype = MOVETYPE_FLY;
			pev->velocity = gpGlobals->v_forward * 2000;
		}
		break;

		case 7:
		{
			m_JumpUse = 0;
		}
		break;

		case 8:
		{
			EMIT_SOUND_DYN ( edict(), CHAN_WEAPON, "tyant_boss/drop.wav", 1.0, 0.5, 0, PITCH_NORM + RANDOM_LONG(-10,10) );
			FX_Explosion( pev->origin + Vector(0,0,4), EXPLOSION_SPARKSHOWER );
			pev->gravity = 0;
			pev->movetype = MOVETYPE_TOSS;
			pev->velocity.z += 800;
		}
		break;

		case 9:
		{
			pev->effects |= EF_NODRAW;
			FadeMonster();
		}
		break;

		case 10:
		{
			pev->movetype = MOVETYPE_TOSS;
			pev->velocity = g_vecZero;

			ClearSchedule();
			SetConditions( bits_COND_SCHEDULE_DONE );
			m_iTaskStatus = TASKSTATUS_COMPLETE;
		}
		break;

		case 11:
		{
			EMIT_SOUND_DYN ( edict(), CHAN_WEAPON, "tyant_boss/drop.wav", 1.0, 0.5, 0, PITCH_NORM + RANDOM_LONG(-10,10) );
			FX_Explosion( pev->origin + Vector(0,0,4), EXPLOSION_SPARKSHOWER );
		}
		break;

		case 12:
		{
				CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_saintna" );
				if ( pEntity )//��ʥ���ع�
				{
					CBaseMonster *pEnemyMonster = pEntity->MyMonsterPointer();
					if(pEnemyMonster->pev->solid != SOLID_NOT){
						pEnemyMonster->pev->flags &= ~FL_ONGROUND;
						pEnemyMonster->pev->velocity.y = -750;
						pEnemyMonster->pev->velocity.x = -550;
						pEnemyMonster->pev->velocity.z = 350;
						pEnemyMonster->pev->angles.y = 45;
						pEnemyMonster->m_thinkspeed = 3;
						pEnemyMonster->pev->sequence = pEnemyMonster->LookupActivity ( ACT_FALL );
						pEnemyMonster->ResetSequenceInfo( );
						pEnemyMonster->pev->frame = 0;
						Vector vecArmPos,vecArmDir;
						GetAttachment( 2, vecArmPos, vecArmDir );
						FX_Trail(vecArmPos, entindex(), PROJ_GUTS );
						EMIT_SOUND(ENT(pev), CHAN_WEAPON, "newadd/zom_headburst.wav", 1, 0.7);	
						SpawnBlood(pEnemyMonster->Center(), BLOOD_COLOR_RED, 200);
						FX_Explosion( pEnemyMonster->Center(), 236 );
					}
				}
		}
		break;

		default:
			CBaseMonster::HandleAnimEvent( pEvent );
			break;
	}
}

void CTyantBOSS :: LeapTouch ( CBaseEntity *pOther )
{
	// Don't hit if back on ground
	if ( pev->movetype == MOVETYPE_FLY )
	{
		pev->movetype = MOVETYPE_TOSS;
		pev->velocity = g_vecZero;

		int dmg5;
		dmg5 = 10;

		if ( pOther->pev->takedamage ){
			TraceResult tr = UTIL_GetGlobalTrace( );
			ClearMultiDamage( );
			pOther->TraceAttack(pev, dmg5, gpGlobals->v_forward, &tr, DMG_SLASH ); 
			ApplyMultiDamage( pev, pev );
		}
	}

}

//=========================================================
// Spawn
//=========================================================
void CTyantBOSS :: Spawn()
{
	Precache( );
	SET_MODEL(ENT(pev), "models/tyrant_boss.mdl");
	UTIL_SetSize( pev, Vector( -16, -16, 0 ), Vector( 16, 16, 80 ) );

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	m_bloodColor		= BLOOD_COLOR_RED;
	
	if (g_iSkillLevel == SKILL_HARD){
	pev->health			= 2000;
	}
	else{
	pev->health			= 1800;
	}

	pev->view_ofs		= VEC_VIEW;// position of the eyes relative to monster's origin.
	m_flFieldOfView		= 0.3;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState		= MONSTERSTATE_NONE;
	m_afCapability		= bits_CAP_DOORS_GROUP;

	SetActivity( ACT_FALL );
	MonsterInit();

	pev->gravity        = 1.5;

	m_ignoredamage = 1;
	m_MoveFail_FuckRoad = TRUE;

	m_headdef = 2;//��Ӳͷ��

	m_flPlayerDamage = 0;
	m_flPlayerDamage2 = 0;
	m_JumpUse = 0;
	m_Miss = 0;
	m_flNextShakeTime = gpGlobals->time + 3.0;
	m_flNextCatchTime = gpGlobals->time + 3.0;

	SetTouch ( &CTyantBOSS::LeapTouch );
	m_forcefuckdoor  = TRUE;
	m_killed_exp = 800;
	m_rpgms_level = 45;
	m_is_the_boss = TRUE;
	pev->netname = MAKE_STRING( "Cry.Tyrant" );
}
//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CTyantBOSS :: Precache()
{
	PRECACHE_MODEL("models/tyrant_boss.mdl");

	m_iSpriteTexture = PRECACHE_MODEL( "sprites/shockwave.spr" );

	PRECACHE_SOUND ("tyant_boss/walk.wav");
	PRECACHE_SOUND ("tyant_boss/slash.wav");
	PRECACHE_SOUND ("tyant_boss/blast.wav");
	PRECACHE_SOUND ("tyant_boss/roar.wav");
	PRECACHE_SOUND ("tyant_boss/drop.wav");
	PRECACHE_SOUND ("tyant_boss/swing.wav");
}	
