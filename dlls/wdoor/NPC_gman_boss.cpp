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
#include	"game.h"

extern DLL_GLOBAL int		g_iSkillLevel;

Task_t	tlGman_Red[] =
{
	{ TASK_STOP_MOVING,					0				},
	{ TASK_PLAY_SEQUENCE_FACE_ENEMY,	(float) ACT_GUARD },
};

Schedule_t slGman_Red[] = 
{
	{
		tlGman_Red,
		ARRAYSIZE ( tlGman_Red ),
		0,
		0,
		"Gman Red"
	}
};


Task_t	tlGman_Arm[] =
{
	{ TASK_STOP_MOVING,					0				},
	{ TASK_PLAY_SEQUENCE_FACE_ENEMY,	(float) ACT_ARM },
};

Schedule_t slGman_Arm[] = 
{
	{
		tlGman_Arm,
		ARRAYSIZE ( tlGman_Arm ),
		0,
		0,
		"Gman Arm Def"
	}
};

//=========================================================
// Monster's Anim Events Go Here
//=========================================================

class CGmanBoss : public CBaseMonster
{
public:
	void Spawn( void );
	void Precache( void );
	void SetYawSpeed( void );
	int  Classify ( void );
	void HandleAnimEvent( MonsterEvent_t *pEvent );

	float m_flNextFlinch;
	float m_flNextPainTime;

	void gman_laser_fire( void );

	void Killed( entvars_t *pevAttacker, int iGib );

	Schedule_t *GetScheduleOfType ( int Type );
	Schedule_t *GetSchedule ( void );

	void EXPORT LeapTouch ( CBaseEntity *pOther );

	void TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType);

	void SetActivity ( Activity NewActivity );

	void RunAI( void );
	void RunTask( Task_t *pTask );

	BOOL FCanCheckAttacks ( void );

	void SetObjectCollisionBox( void )
	{
		pev->absmin = pev->origin + Vector( -40, -40, 0 );
		pev->absmax = pev->origin + Vector( 40, 40, 160 );
	}

	// No range attacks
	BOOL CheckRangeAttack1 ( float flDot, float flDist );//��֮����
	BOOL CheckRangeAttack2 ( float flDot, float flDist );//˫����
	BOOL CheckMeleeAttack1 ( float flDot, float flDist );//�չ�
	BOOL CheckMeleeAttack2 ( float flDot, float flDist );//�ػ�
	int TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType );

	int	Save( CSave &save ); 
	int Restore( CRestore &restore );

	static TYPEDESCRIPTION m_SaveData[];

	Vector m_teleportorigin;

	float m_SkillTime1;
	float m_SkillTime2;
	float m_SkillTime3;
	float m_SkillTime4;
	float m_SkillTime5;
	float m_SkillTime6;
	float m_ArmDefTime;
	float m_flKillBeamTime;
	float m_flFlyingTime;

	BOOL	m_fGunDrawn;
	//EHANDLE	m_childguy;

	CUSTOM_SCHEDULES;
};

LINK_ENTITY_TO_CLASS( monster_gman_boss, CGmanBoss );
LINK_ENTITY_TO_CLASS( monster_gman_boss_render, CGmanBoss );

TYPEDESCRIPTION	CGmanBoss::m_SaveData[] = 
{
	DEFINE_FIELD( CGmanBoss, m_SkillTime1, FIELD_TIME ),
	DEFINE_FIELD( CGmanBoss, m_SkillTime2, FIELD_TIME ),
	DEFINE_FIELD( CGmanBoss, m_SkillTime3, FIELD_TIME ),
	DEFINE_FIELD( CGmanBoss, m_SkillTime4, FIELD_TIME ),
	DEFINE_FIELD( CGmanBoss, m_SkillTime5, FIELD_TIME ),
	DEFINE_FIELD( CGmanBoss, m_SkillTime6, FIELD_TIME ),
	DEFINE_FIELD( CGmanBoss, m_ArmDefTime, FIELD_TIME ),
	DEFINE_FIELD( CGmanBoss, m_flFlyingTime, FIELD_TIME ),
	DEFINE_FIELD( CGmanBoss, m_flKillBeamTime, FIELD_TIME ),
	DEFINE_FIELD( CGmanBoss, m_teleportorigin, FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( CGmanBoss, m_fGunDrawn, FIELD_BOOLEAN ),
//	DEFINE_FIELD( CGmanBoss, m_childguy, FIELD_EHANDLE ),
};

IMPLEMENT_SAVERESTORE( CGmanBoss, CBaseMonster );

DEFINE_CUSTOM_SCHEDULES( CGmanBoss )
{
	slGman_Red,
	slGman_Arm,
};

IMPLEMENT_CUSTOM_SCHEDULES( CGmanBoss, CBaseMonster );

Schedule_t* CGmanBoss :: GetScheduleOfType ( int Type )
{
	switch( Type )
	{
		case SCHED_COWER:{
		return slGman_Red;
		}
		break;

		case SCHED_ARM_WEAPON:{
		return slGman_Arm;
		}
		break;
	}

	return CBaseMonster::GetScheduleOfType( Type );
}

//=========================================================
// GetSchedule - Decides which type of schedule best suits
// the monster's current state and conditions. Then calls
// monster's member function to get a pointer to a schedule
// of the proper type.
//=========================================================
Schedule_t *CGmanBoss :: GetSchedule ( void )
{
	if (!m_fGunDrawn && pev->health <= pev->max_health * 0.6){
		pev->takedamage = DAMAGE_NO;
		if(m_freezetime >= 1){
		m_freezetime = -1;
		}
		return GetScheduleOfType( SCHED_COWER );//����
	}

	if(m_fGunDrawn && m_SkillTime5 <= gpGlobals->time && m_freezetime == 0){
		return GetScheduleOfType( SCHED_ARM_WEAPON );//���֮��
	}
/*
	if(m_SkillTime6 <= gpGlobals->time && m_freezetime == 0 && m_cleardally_enemy_long > 0
	&& pev->health <= pev->max_health * 0.6){
		return GetScheduleOfType( SCHED_RELOAD_DEEP );//�����
	}
*/
	return CBaseMonster::GetSchedule();
}

void CGmanBoss :: LeapTouch ( CBaseEntity *pOther )
{
	if(pev->frags != 1){
		return;
	}

	if ( pOther->pev->deadflag != DEAD_NO || pev->deadflag != DEAD_NO){
		return;
	}

	if ( pOther->Classify() == Classify() ){
		return;
	}
	
	if ( pOther->pev->flags & (FL_MONSTER|FL_CLIENT) ){
		if(pOther == m_hEnemy){
		pev->frags = 2;//ץס��!
		pev->velocity = -pev->velocity;
		pOther->TakeDamage( pev, pev, 0, DMG_CONCUSSION);//ץȡѣ��!
		}
		else if(pev->impulse == 0){//���ٳ�ײ!ײ���˱��Ŀ�ꣿ
		pev->impulse = 1;
		pOther->TakeDamage( pev, pev, 360, DMG_GENERIC | DMG_CONCUSSION);
		}
	}
}

BOOL CGmanBoss :: FCanCheckAttacks ( void )
{
	return TRUE;
}

// ����Ŀ��
void MovetoTarget_3( entvars_t *pevFucker, Vector vecTarget,float speed )
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

	if (flSpeed > (speed + 900))
	{
		m_vecIdeal = m_vecIdeal.Normalize( ) * (speed + 900);
	}

	m_vecIdeal = m_vecIdeal + (vecTarget - pevFucker->origin).Normalize() * speed;

	pevFucker->velocity = m_vecIdeal;
}

void CGmanBoss :: gman_laser_fire ( void )
{
	Vector org,vecdir;
	GetAttachment( 0, org,vecdir);
	FX_Trail(org, entindex(), 144);

	m_HackedGunPos = org;

	if ( HasConditions( bits_COND_SEE_ENEMY ) ){
		CBaseEntity *pEnemy = m_hEnemy;
		if ( pEnemy )
		{//��׼����μӳ�
		m_vecEnemyLKP = pEnemy->pev->origin;
		}
	}

	Vector vecShootDir = ShootAtEnemy( m_HackedGunPos );

	UTIL_VecToAngles( vecShootDir );

	FireBullets(1, m_HackedGunPos, vecShootDir, g_vecZero, 16384, BULLET_GMAN_LASER,0);

	FireBeam(m_HackedGunPos, vecShootDir, BEAM_TAUCANNON2, 100, pev);

	EMIT_SOUND_DYN( ENT(pev), CHAN_STREAM, "doma/doma_fire2.wav", 1, 0.6, 0, 100);
}

void CGmanBoss :: RunAI( void )
{
	CBaseMonster :: RunAI();

	if(pev->sequence == LookupActivity ( ACT_WALK )){
		m_flGroundSpeed = 140;
	}
	else if(pev->sequence == LookupActivity ( ACT_WALK_SCARED )
	|| pev->sequence == LookupActivity ( ACT_WALK_HURT )){
		m_flGroundSpeed = 420;
	}

	if(m_hEnemy != NULL && m_SkillTime4 < gpGlobals->time && m_freezetime == 0){
		if(pev->sequence == LookupActivity ( ACT_WALK ) || pev->sequence == LookupActivity ( ACT_IDLE )){
			pev->armorvalue++;
			if(pev->armorvalue >= 40){
			ClearSchedule();
			SetActivity ( ACT_HOP );
			pev->armorvalue = 0;
			}
		}
		else if(pev->armorvalue > 0){
			pev->armorvalue--;
		}
	}

	if(m_flKillBeamTime > 0 && m_flKillBeamTime < gpGlobals->time){
	MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
	WRITE_BYTE( TE_KILLBEAM );
	WRITE_SHORT( entindex() + 0x1000 * 2 );		// entity, attachment
	MESSAGE_END();

	MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
	WRITE_BYTE( TE_KILLBEAM );
	WRITE_SHORT( entindex() + 0x1000 * 3 );		// entity, attachment
	MESSAGE_END();
	m_flKillBeamTime = 0;
	}

	if(m_flFlyingTime > gpGlobals->time){
		if(m_hEnemy != NULL){
			if(pev->frags == 1){
			MovetoTarget_3(pev,m_vecEnemyLKP,750);
			}
			else if(pev->frags == 2){
				if(pev->velocity.Length() <= 750){
				pev->velocity.x += RANDOM_LONG(-900,900);
				pev->velocity.y += RANDOM_LONG(-900,900);
				}
				if(pev->velocity.z < 300){
				pev->velocity.z = 300;
				}

				MovetoTarget_3(m_hEnemy->pev,Center(),750);
			}
		}
	}
	else if(m_flFlyingTime > 0){
		m_groundElev2 = FALSE;
		pev->movetype = MOVETYPE_STEP;
		pev->gravity  = 1.6;
		m_flFlyingTime = 0;
		pev->frags = 0;
		pev->impulse = 0;
		SetTouch ( NULL );
	}

	if(m_ArmDefTime > 0 && m_ArmDefTime < gpGlobals->time){
	m_ArmDefTime = 0;
	pev->body = 1;
	pev->takedamage = DAMAGE_AIM;
	ClearBits(pev->effects, EF_DIMLIGHT);
	m_enemyget_mode = 0;
	}
}

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int	CGmanBoss :: Classify ( void )
{
	return	CLASS_HUMAN_ASS;
}

//=========================================================
// ��֮����
//=========================================================
BOOL CGmanBoss::CheckRangeAttack1( float flDot, float flDist )
{
	if ( !HasConditions( bits_COND_ENEMY_OCCLUDED ) && gpGlobals->time > m_SkillTime1)
	{
		return TRUE;
	}
	return FALSE;
}

//=========================================================
// ˫����
//=========================================================
BOOL CGmanBoss::CheckRangeAttack2( float flDot, float flDist )
{
	if ( gpGlobals->time > m_SkillTime2)
	{
		m_facing_fucking_mode = 1;
		return TRUE;
	}

	m_facing_fucking_mode = 0;
	return FALSE;
}

//�����ش�
BOOL CGmanBoss :: CheckMeleeAttack2 ( float flDot, float flDist )
{
	float dist = 120;

	if(m_hEnemy != NULL){
		if ( fabs( pev->origin.z - m_hEnemy->pev->origin.z ) >= 80 )
		{
			if (m_hEnemy->pev->flags & FL_ONGROUND)
			{
			dist += 60;
			}
		}
	}

	if(flDist <= dist && m_SkillTime3 <= gpGlobals->time && m_hEnemy != NULL){
		return TRUE;
	}

	return FALSE;
}

//�չ�
BOOL CGmanBoss :: CheckMeleeAttack1 ( float flDot, float flDist )
{
	float dist = 100;

	if(m_hEnemy != NULL){
		if ( fabs( pev->origin.z - m_hEnemy->pev->origin.z ) >= 80 )
		{
			if (m_hEnemy->pev->flags & FL_ONGROUND)
			{
			dist += 60;
			}
		}
	}

	if(pev->body >= 1){
		dist += 20;
		if (flDist <= dist && m_hEnemy != NULL && flDot >= 0.5)
		{
			if (m_hEnemy->IsAlive() ){
				if(pev->sequence == LookupActivity ( ACT_WALK_HURT )){
				pev->sequence = LookupActivity ( ACT_WALK_SCARED );
				ResetSequenceInfo( );
				pev->frame = 0;
				}
			}
		}
		else if (flDist >= dist + 60 )
		{
				if(pev->sequence == LookupActivity ( ACT_WALK_SCARED )){
				pev->sequence = LookupActivity ( ACT_WALK_HURT );
				ResetSequenceInfo( );
				pev->frame = 0;
				}
		}
		return FALSE;
	}

	if (flDist <= dist && m_hEnemy != NULL && flDot >= 0.5 && !HasConditions(bits_COND_CAN_MELEE_ATTACK2))
	{
		return TRUE;
	}

	return FALSE;
}


void CGmanBoss::TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType)
{
	if ( pev->dmgtime != gpGlobals->time || (RANDOM_LONG(0,100) < 20))
	{
	pev->dmgtime = gpGlobals->time;
	UTIL_Sparks(ptr->vecEndPos);
	}

	if (ptr->iHitgroup == 1)
	{
		flDamage *= 0.75;
		if ( pev->dmgtime != gpGlobals->time || (RANDOM_LONG(0,100) < 20))
		{
			UTIL_WhiteSparks( ptr->vecEndPos, ptr->vecPlaneNormal, 9, 5, 5, 100 );//puntos
		}
	}

	if (ptr->iHitgroup == 10)
	{
		flDamage *= 1.5;//����!
		if ( pev->dmgtime != gpGlobals->time || (RANDOM_LONG(0,100) < 20))
		{
			UTIL_WhiteSparks( ptr->vecEndPos, ptr->vecPlaneNormal, 9, 5, 5, 100 );//puntos
		}
	}

	CBaseMonster::TraceAttack( pevAttacker, flDamage, vecDir, ptr, bitsDamageType );
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CGmanBoss :: SetYawSpeed ( void )
{
	int ys;

	switch ( m_Activity )
	{
	case ACT_RANGE_ATTACK2:	
		ys = 360;	
		break;
	default:
		ys = 180;
		break;
	}

	if(pev->body >= 1){
	ys = 360;
	}

	pev->yaw_speed = ys;
}

int CGmanBoss :: TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	if(pev->sequence == LookupActivity ( ACT_GUARD )){
	return 0;//�����޵�״̬
	}

	if ( (bitsDamageType & DMG_ENERGYBEAM) 
	|| (bitsDamageType & DMG_BURN)
	|| (bitsDamageType & DMG_BULLET)){
	flDamage *= 0.8;
	}

	return CBaseMonster::TakeDamage( pevInflictor, pevAttacker, flDamage, bitsDamageType );
}

void CGmanBoss::Killed( entvars_t *pevAttacker, int iGib )
{
	m_ArmDefTime = 0;
	ClearBits(pev->effects, EF_DIMLIGHT);
	FX_Trail(pev->origin, entindex(), PROJ_REMOVE);
	pev->movetype = MOVETYPE_STEP;
	pev->gravity  = 1.6;
	m_flFlyingTime = 0;
	pev->frags = 0;
	pev->impulse = 0;
	SetTouch ( NULL );
	pev->body = 0;
	CBaseMonster::Killed( pevAttacker, GIB_NEVER );
}


//=========================================================
// SetActivity 
//=========================================================
void CGmanBoss :: SetActivity ( Activity NewActivity )
{
	int	iSequence = ACTIVITY_NOT_AVAILABLE;
	void *pmodel = GET_MODEL_PTR( ENT(pev) );

	switch ( NewActivity)
	{
	case ACT_WALK:
		if ( pev->body >= 1  )//���±���
		{
			iSequence = LookupActivity ( ACT_WALK_HURT );
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
	//	pev->sequence		= 0;	// Set to the reset anim (if it's there)
	}
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CGmanBoss :: HandleAnimEvent( MonsterEvent_t *pEvent )
{
	int dmg1,dmg2;

	dmg1 = 180;
	dmg2 = 360;

	switch( pEvent->event )
	{
		case 1://��β1
		{
			
			MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
			WRITE_BYTE( TE_BEAMFOLLOW );
			WRITE_SHORT( entindex() + 0x1000 * 3 );		// entity, attachment
			WRITE_SHORT(g_sModelIndexTrail);	// model
			if(pev->weapons == 520){
			WRITE_BYTE( 6 ); // life
			WRITE_BYTE( 9 );  // width
			}
			else{
			WRITE_BYTE( 4 ); // life
			WRITE_BYTE( 6 );  // width
			}
			WRITE_BYTE( 255 );	// R
			WRITE_BYTE( 255 );	// G
			WRITE_BYTE( 255 );	// B
			WRITE_BYTE( 192 );	// brightness
			MESSAGE_END();  // move PHS/PVS data sending into here (SEND_ALL, SEND_PVS, SEND_PHS)

			m_flKillBeamTime = gpGlobals->time + 3.0;
		}
		break;

		case 2://�Ϲ�ȭ
		{
			if(pev->weapons == 520){
					CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_vanlve_combat" );
					if ( pEntity )//��Vanlve�ع�
					{
					pEntity->pev->velocity.z = 100;
					pEntity->pev->velocity.y = 400;
					//FX_Explosion( pEntity->Center(), 126 );
					SpawnBlood(pEntity->Center(), BLOOD_COLOR_RED, 200);
					pEntity->TakeDamage ( pev, pev, dmg2, DMG_CRUSH );
					EMIT_SOUND_DYN ( ENT(pev), CHAN_STREAM,"newadd/fist_hearvy_hit1.wav", 1.0, 0.7, 0, 100 + RANDOM_LONG(-5,5) );
					}
			}

			if(m_hEnemy != NULL){
				TraceResult tr;
				UTIL_MakeVectors(pev->angles);
				
				//Vector vecGunPos,vecGunAngles;
				//GetAttachment( 2, vecGunPos, vecGunAngles );

				Vector vecSrc	= Center();
				Vector vecEnd	= m_hEnemy->Center();
				UTIL_TraceLine( vecSrc, vecEnd, dont_ignore_monsters, ENT( pev ), &tr );
				CBaseEntity *pEntity = CBaseEntity::Instance(tr.pHit);

				if ( tr.flFraction < 1.0 ){
					if(( vecSrc - tr.vecEndPos).Length() <= 140){
					int tex = (int)TEXTURETYPE_Trace(&tr, vecSrc, vecEnd);
					int surface = (int)SURFACETYPE_Trace(&tr, vecSrc, vecEnd,Classify(),1);
					FX_ImpBullet( tr.vecEndPos, tr.vecPlaneNormal, vecSrc, surface, BULLET_CROWBAR, (float)tex );

					if ( pEntity->pev->flags & (FL_MONSTER|FL_CLIENT) )
					{
					pEntity->pev->velocity.z += 300;
					}

					ClearMultiDamage( );
					pEntity->TraceAttack(pev, dmg1, gpGlobals->v_forward, &tr, DMG_CRUSH); 
					ApplyMultiDamage( pev, pev );
					EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, "newadd/fist_hearvy_hit1.wav", 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
					}
				}
			}

		}
		break;

		case 3://��β2
		{
			MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
			WRITE_BYTE( TE_BEAMFOLLOW );
			WRITE_SHORT( entindex() + 0x1000 * 2 );		// entity, attachment
			WRITE_SHORT(g_sModelIndexTrail);	// model
			if(pev->weapons == 520){
			WRITE_BYTE( 6 ); // life
			WRITE_BYTE( 9 );  // width
			}
			else{
			WRITE_BYTE( 4 ); // life
			WRITE_BYTE( 6 );  // width
			}
			WRITE_BYTE( 255 );	// R
			WRITE_BYTE( 255 );	// G
			WRITE_BYTE( 255 );	// B
			WRITE_BYTE( 192 );	// brightness
			MESSAGE_END();  // move PHS/PVS data sending into here (SEND_ALL, SEND_PVS, SEND_PHS)

			m_flKillBeamTime = gpGlobals->time + 3.0;
		}
		break;

		case 4://��ͨȭ
		{
			if(pev->weapons == 520){
					CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_vanlve_combat" );
					if ( pEntity )//��Vanlve�ع�
					{
					pEntity->pev->velocity.z = 0;
					pEntity->pev->velocity.y = 650;
					pev->velocity.y = 600;
					//FX_Explosion( pEntity->Center(), 126 );
					SpawnBlood(pEntity->Center(), BLOOD_COLOR_RED, 200);
					pEntity->TakeDamage ( pev, pev, dmg2, DMG_CRUSH );
					EMIT_SOUND_DYN ( ENT(pEntity->pev), CHAN_WEAPON,"newadd/fist_hearvy_hit1.wav", 1.0, 0.7, 0, 100 + RANDOM_LONG(-5,5) );
					}
			}

			if(m_hEnemy != NULL){
				TraceResult tr;
				UTIL_MakeVectors(pev->angles);
				
				Vector vecSrc	= BodyTarget_o(pev->origin);
				Vector vecEnd	= m_hEnemy->Center();
				UTIL_TraceLine( vecSrc, vecEnd, dont_ignore_monsters, ENT( pev ), &tr );
				CBaseEntity *pEntity = CBaseEntity::Instance(tr.pHit);

				if ( tr.flFraction < 1.0){
					if(( vecSrc - tr.vecEndPos).Length() <= 140){
					int tex = (int)TEXTURETYPE_Trace(&tr, vecSrc, vecEnd);
					int surface = (int)SURFACETYPE_Trace(&tr, vecSrc, vecEnd,Classify(),1);
					FX_ImpBullet( tr.vecEndPos, tr.vecPlaneNormal, vecSrc, surface, BULLET_CROWBAR, (float)tex );

					if ( pEntity->pev->flags & (FL_MONSTER|FL_CLIENT) )
					{
					pEntity->pev->velocity = pEntity->pev->velocity + (pEntity->pev->origin - pev->origin).Normalize() * 600;
					}

					ClearMultiDamage( );
					pEntity->TraceAttack(pev, dmg1, gpGlobals->v_forward, &tr, DMG_CRUSH); 
					ApplyMultiDamage( pev, pev );
					EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, "newadd/fist_hearvy_hit1.wav", 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
					}
				}
			}

		}
		break;

		case 5:
		{//��β��ʧ
			MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
			WRITE_BYTE( TE_KILLBEAM );
			WRITE_SHORT( entindex() + 0x1000 * 2 );		// entity, attachment
			MESSAGE_END();

			MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
			WRITE_BYTE( TE_KILLBEAM );
			WRITE_SHORT( entindex() + 0x1000 * 3 );		// entity, attachment
			MESSAGE_END();

			m_flKillBeamTime = 0;
		}
		break;

		case 12:
		{//��β1+2
			MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
			WRITE_BYTE( TE_BEAMFOLLOW );
			WRITE_SHORT( entindex() + 0x1000 * 2 );		// entity, attachment
			WRITE_SHORT(g_sModelIndexTrail);	// model
			WRITE_BYTE( 4 ); // life
			WRITE_BYTE( 6 );  // width
			WRITE_BYTE( 255 );	// R
			WRITE_BYTE( 255 );	// G
			WRITE_BYTE( 255 );	// B
			WRITE_BYTE( 192 );	// brightness
			MESSAGE_END();  // move PHS/PVS data sending into here (SEND_ALL, SEND_PVS, SEND_PHS)

			MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
			WRITE_BYTE( TE_BEAMFOLLOW );
			WRITE_SHORT( entindex() + 0x1000 * 3 );		// entity, attachment
			WRITE_SHORT(g_sModelIndexTrail);	// model
			WRITE_BYTE( 4 ); // life
			WRITE_BYTE( 6 );  // width
			WRITE_BYTE( 255 );	// R
			WRITE_BYTE( 255 );	// G
			WRITE_BYTE( 255 );	// B
			WRITE_BYTE( 192 );	// brightness
			MESSAGE_END();  // move PHS/PVS data sending into here (SEND_ALL, SEND_PVS, SEND_PHS)

			m_flKillBeamTime = gpGlobals->time + 3.0;
		}
		break;

		case 13://�����ش�
		{
			if(m_SkillTime3 < gpGlobals->time && m_hEnemy != NULL){
				
				m_SkillTime3 = gpGlobals->time + RANDOM_FLOAT( 6.0, 10.0 );

				if(m_SkillTime6 < gpGlobals->time && pev->body >= 1){
					TraceResult trace;

					UTIL_MakeVectors( pev->angles );
					Vector vecStart = pev->origin + Vector(0,0,16) + 32 * gpGlobals->v_forward;
					Vector vecAim = ShootAtEnemy( vecStart );
					Vector vecEnd = (vecAim * 2560) + vecStart;

					UTIL_TraceLine( vecStart, vecEnd, ignore_monsters, edict(), &trace );

					CBaseEntity *pStomp = Create( "gman_stomp", vecStart, pev->angles, edict() );
					Vector dir = (trace.vecEndPos - pStomp->pev->origin);
					pStomp->pev->scale = dir.Length();
					pStomp->pev->movedir = dir.Normalize();
					pStomp->pev->speed = 1200;
					pStomp->pev->owner = ENT(pev);

					m_SkillTime6 = gpGlobals->time + RANDOM_FLOAT( 8.0, 12.0 );
				}

				TraceResult tr;
				UTIL_MakeVectors(pev->angles);
				
				Vector vecSrc	= BodyTarget(pev->origin);
				Vector vecEnd	= m_hEnemy->Center();
				UTIL_TraceLine( vecSrc, vecEnd, dont_ignore_monsters, ENT( pev ), &tr );
				CBaseEntity *pEntity = CBaseEntity::Instance(tr.pHit);

				if ( tr.flFraction < 1.0){
					if(( vecSrc - tr.vecEndPos).Length() <= 200){
					int tex = (int)TEXTURETYPE_Trace(&tr, vecSrc, vecEnd);
					int surface = (int)SURFACETYPE_Trace(&tr, vecSrc, vecEnd,Classify(),1);
					FX_ImpBullet( tr.vecEndPos, tr.vecPlaneNormal, vecSrc, surface, BULLET_CROWBAR, (float)tex );

					if ( pEntity->pev->flags & (FL_MONSTER|FL_CLIENT) )
					{
					pEntity->pev->velocity = pEntity->pev->velocity + (pEntity->pev->origin - pev->origin).Normalize() * 1200;
					}
					FX_Explosion( tr.vecEndPos + (tr.vecPlaneNormal * 20), 126 );
					::RadiusDamage_limit( tr.vecEndPos + (tr.vecPlaneNormal * 20), pev, pev, 240, 360, CLASS_HUMAN_ASS, DMG_ENERGYBLAST | DMG_CONCUSSION);

					ClearMultiDamage( );
					pEntity->TraceAttack(pev, dmg2, gpGlobals->v_forward, &tr, DMG_CRUSH); 
					ApplyMultiDamage( pev, pev );
					EMIT_SOUND_DYN ( ENT(pev), CHAN_STREAM,"newadd/fist_hearvy_hit2.wav", 1.0, 0.7, 0, 100 + RANDOM_LONG(-5,5) );
					}
				}
				else if(( pev->origin - m_hEnemy->pev->origin).Length() <= 180){
					m_hEnemy->pev->velocity = m_hEnemy->pev->velocity + (m_hEnemy->pev->origin - pev->origin).Normalize() * 1200;
					FX_Explosion( m_hEnemy->Center(), 126 );
					m_hEnemy->TakeDamage ( pev, pev, dmg2, DMG_CRUSH );
					::RadiusDamage_limit( m_hEnemy->Center(), pev, pev, 240, 360, CLASS_HUMAN_ASS, DMG_ENERGYBLAST | DMG_CONCUSSION);
					EMIT_SOUND_DYN ( ENT(pev), CHAN_STREAM,"newadd/fist_hearvy_hit2.wav", 1.0, 0.7, 0, 100 + RANDOM_LONG(-5,5) );
				}

			}

		}
		break;

		case 6:
		{//��֮���ߡ�����
			if(m_SkillTime1 > gpGlobals->time){
			ClearSchedule();
			}
			else{
			FX_Trail(pev->origin, entindex(), PROJ_REMOVE);
			FX_Trail(pev->origin, entindex(), 143);
			}
		}
		break;

		case 7:
		{//��֮���ߡ����
			if(m_SkillTime1 < gpGlobals->time){
			FX_Trail(pev->origin, entindex(), PROJ_REMOVE);

			m_SkillTime1 = gpGlobals->time + RANDOM_FLOAT( 30.0, 40.0 );

			gman_laser_fire();
			}
		}
		break;

		case 8://��ɤ�׼��
		{
			if(pev->weapons == 520){
			return;
			}

				if(m_SkillTime2 > gpGlobals->time){
					ClearSchedule();
				}
				else{
					pev->velocity.x = 0;
					pev->velocity.y = 0;
					pev->velocity.z = 30;
					pev->gravity = -0.3;
					pev->movetype = MOVETYPE_BOUNCE;
					ClearBits( pev->flags, FL_ONGROUND );
					m_flFlyingTime = gpGlobals->time + 3.0;
					m_groundElev2 = TRUE;//��ֹ��������?
				}

		}
		break;

		case 9:
		{
			if(pev->weapons == 520){
			StopAnimation();
			return;
			}

			MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
			WRITE_BYTE( TE_BEAMFOLLOW );
			WRITE_SHORT( entindex() + 0x1000 * 2 );		// entity, attachment
			WRITE_SHORT(g_sModelIndexTrail);	// model
			WRITE_BYTE( 4 ); // life
			WRITE_BYTE( 6 );  // width
			WRITE_BYTE( 255 );	// R
			WRITE_BYTE( 255 );	// G
			WRITE_BYTE( 255 );	// B
			WRITE_BYTE( 192 );	// brightness
			MESSAGE_END();  // move PHS/PVS data sending into here (SEND_ALL, SEND_PVS, SEND_PHS)

			MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
			WRITE_BYTE( TE_BEAMFOLLOW );
			WRITE_SHORT( entindex() + 0x1000 * 3 );		// entity, attachment
			WRITE_SHORT(g_sModelIndexTrail);	// model
			WRITE_BYTE( 4 ); // life
			WRITE_BYTE( 6 );  // width
			WRITE_BYTE( 255 );	// R
			WRITE_BYTE( 255 );	// G
			WRITE_BYTE( 255 );	// B
			WRITE_BYTE( 192 );	// brightness
			MESSAGE_END();  // move PHS/PVS data sending into here (SEND_ALL, SEND_PVS, SEND_PHS)

			ClearBits( pev->flags, FL_ONGROUND );
			pev->gravity = 0.0;
			pev->frags = 1;
			pev->movetype = MOVETYPE_BOUNCE;

			SetTouch ( &CGmanBoss::LeapTouch );

			m_flKillBeamTime = gpGlobals->time + 3.0;

			m_flFlyingTime = gpGlobals->time + 1.2;
		}
		break;

		case 10://˫����
		{
			if(m_SkillTime2 < gpGlobals->time && m_hEnemy != NULL){
				m_SkillTime2 = gpGlobals->time + RANDOM_FLOAT( 10.0, 14.0 );

				TraceResult tr;
				UTIL_MakeVectors(pev->angles);
				
				Vector vecSrc	= Center();
				Vector vecEnd	= m_hEnemy->Center();
				UTIL_TraceLine( vecSrc, vecEnd, dont_ignore_monsters, ENT( pev ), &tr );
				CBaseEntity *pEntity = CBaseEntity::Instance(tr.pHit);

				if(pev->frags != 2){
					if ( pEntity->pev->flags & (FL_MONSTER|FL_CLIENT) ){
						if(tr.flFraction < 1.0 && (vecSrc - tr.vecEndPos).Length() <= 96){//����Ͻ����ж�
						pev->frags = 2;
						}
					}
				}

				if (pev->frags == 2){
					int tex = (int)TEXTURETYPE_Trace(&tr, vecSrc, vecEnd);
					int surface = (int)SURFACETYPE_Trace(&tr, vecSrc, vecEnd,Classify(),1);
					FX_ImpBullet( tr.vecEndPos, tr.vecPlaneNormal, vecSrc, surface, BULLET_CROWBAR, (float)tex );

					if ( pEntity->pev->flags & (FL_MONSTER|FL_CLIENT) )
					{
					pEntity->pev->velocity = pEntity->pev->velocity + (pEntity->pev->origin - pev->origin).Normalize() * 1200;
					}

					FX_Explosion( tr.vecEndPos + (tr.vecPlaneNormal * 20), 126 );
					::RadiusDamage_limit( tr.vecEndPos + (tr.vecPlaneNormal * 20), pev, pev, 240, 360, CLASS_HUMAN_ASS, DMG_ENERGYBLAST | DMG_CONCUSSION);

					ClearMultiDamage( );
					pEntity->TraceAttack(pev, dmg2, gpGlobals->v_forward, &tr, DMG_CRUSH); 
					ApplyMultiDamage( pev, pev );
					EMIT_SOUND_DYN ( ENT(pev), CHAN_STREAM, "newadd/fist_hearvy_hit2.wav", 1.0, 0.7, 0, 100 + RANDOM_LONG(-5,5) );
				}

				pev->frags = 0;
				pev->impulse = 0;

			}

		}
		break;

		case 11://������ը
		{
		//	FX_Explosion( Center(), EXPLOSION_CHRONOCLIP );
		//	EMIT_SOUND_DYN( ENT(pev), CHAN_STREAM, "weapons/chronoclip_explode.wav", 1, 0.4, 0, 100);
		}
		break;

		case 14://��Ծ
		{
			ClearBits( pev->flags, FL_ONGROUND );

			//UTIL_SetOrigin (pev, pev->origin + Vector ( 0 , 0 , 1) );// take him off ground so engine doesn't instantly reset onground 
			UTIL_MakeVectors ( pev->angles );

			Vector vecJumpDir;
			if (m_hEnemy != NULL)
			{
				float gravity = g_psv_gravity->value;
				if (gravity <= 1)
					gravity = 1;

				// How fast does the headcrab need to travel to reach that height given gravity?
				float height = (m_vecEnemyLKP.z + m_hEnemy->pev->view_ofs.z - pev->origin.z);
				if (height < 60){
				height = 60;
				}
				float speed = sqrt( 2 * gravity * height );
				float time = speed / gravity;

				// Scale the sideways velocity to get there at the right time
				vecJumpDir = (m_vecEnemyLKP + m_hEnemy->pev->view_ofs - pev->origin);
				vecJumpDir = vecJumpDir * ( 1.0 / time );

				// Speed to offset gravity at the desired height
				vecJumpDir.z = speed;

				// Don't jump too far/fast
				float distance = vecJumpDir.Length();
				
				if (distance > 750)
				{
					vecJumpDir = vecJumpDir * ( 750.0 / distance );
				}
			}
			else
			{
				// jump hop, don't care where
				vecJumpDir = Vector( gpGlobals->v_forward.x, gpGlobals->v_forward.y, gpGlobals->v_up.z ) * 750;
			}
			vecJumpDir.z *= 1.5;
			pev->velocity = vecJumpDir;

			m_SkillTime4 = gpGlobals->time + 4.0;
		}
		break;

		case 15://������
		{
			pev->body = 1;
			m_fGunDrawn = TRUE;
			m_ignoreFail_MAX = 30;//����ģʽ
			m_ignoreFail_OFF = 0;
			m_MoveFail_FuckRoad = TRUE;
			m_MoveFail_SimpleRoad = TRUE;
			FX_Explosion( Center(), EXPLOSION_HEVCHARGER);
			EMIT_SOUND_DYN ( ENT(pev), CHAN_STREAM, "newadd/exp2_frost.wav", 1.0, 0.6, 0, 100);
			pev->takedamage	= DAMAGE_AIM;

			//�������״̬
			if(m_freezetime == -1){
			ChangeSchedule( GetScheduleOfType( SCHED_ALERT_STAND ) );
			SetActivity( ACT_IDLE );
			SetYawSpeed();
			m_freezetime = 0;
			}

			m_freeze_def = 3;//���Ό��LV3!!!
		}
		break;

		case 16://���֮��
		{
			if(m_SkillTime5 <= gpGlobals->time){
				pev->body = 2;
				pev->takedamage = DAMAGE_NO;
				m_ArmDefTime = gpGlobals->time + 10.0;
				m_SkillTime5 = gpGlobals->time + RANDOM_FLOAT( 40.0, 50.0 );
				EMIT_SOUND_DYN( ENT(pev), CHAN_STREAM, "newadd/PowerShield.wav", 1, 0.6, 0, 100);
				SetBits(pev->effects, EF_DIMLIGHT);

				if(m_freezetime >= 1){//�������״̬
				ChangeSchedule( GetScheduleOfType( SCHED_ALERT_STAND ) );
				SetActivity( ACT_IDLE );
				SetYawSpeed();
				m_freezetime = 0;
				}

				m_hEnemy = NULL;//���õ���
				m_enemyget_mode = 1;//�޵Ф�ʱ��Զ����
			}
		}
		break;

		case 17:
		{
			//m_SkillTime6 = gpGlobals->time + RANDOM_FLOAT( 18.0, 27.0 );
		}
		break;

		default:
			CBaseMonster::HandleAnimEvent( pEvent );
			break;
	}
}

void CGmanBoss :: RunTask( Task_t *pTask )
{
	switch ( pTask->iTask )
	{
	case TASK_MELEE_ATTACK1:
		if (pev->body >= 1)
		{
			pev->framerate = 1.5;
		}
		CBaseMonster::RunTask( pTask );
		break;
	case TASK_MELEE_ATTACK2:
		if (pev->body >= 1)
		{
			pev->framerate = 1.5;
		}
		CBaseMonster::RunTask( pTask );
		break;
	default:
		CBaseMonster::RunTask( pTask );
		break;
	}
}


//=========================================================
// Spawn
//=========================================================
void CGmanBoss :: Spawn()
{
	Precache( );
	SET_MODEL(ENT(pev), "models/gman_boss.mdl");
	UTIL_SetSize(pev, Vector(-32,-32,0), Vector(32,32,128));

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	m_bloodColor		= DONT_BLEED;

	if (g_iSkillLevel == SKILL_HARD){
	pev->health			= 72000;
	}
	else{
	pev->health			= 60000;
	}

	pev->view_ofs		= VEC_VIEW;// position of the eyes relative to monster's origin.
	m_flFieldOfView		= -1;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState		= MONSTERSTATE_NONE;
	m_afCapability		= bits_CAP_DOORS_GROUP;
	
	MonsterInit();

	pev->gravity		= 1.6;

	m_SkillTime1 = 0;
	m_SkillTime2 = 0;
	m_SkillTime3 = 0;
	m_SkillTime4 = 0;
	m_SkillTime5 = 0;
	m_SkillTime6 = 0;
	m_flKillBeamTime = 0;
	m_flFlyingTime = 0;

	m_ignoredamage = 1;
	m_headdef	   = 2;
	m_longming	   = 1;

	m_selfmode = TRUE;
	m_fGunDrawn = FALSE;
	pev->body = 0;

	m_MoveFail_SimpleRoad = TRUE;

	m_aimenemy_mod = 6;

	m_killed_exp = 6000;
	m_is_the_boss = TRUE;
	m_rpgms_level = 120;
	pev->netname = MAKE_STRING( "Giga.Man" );

	m_freeze_def = 2;//���Ό��LV2

	m_singdelay_max = 0;//0��Ӧ
	m_singdelay_use = m_singdelay_max;

	m_no_pov_limit = 1;
//	m_childguy = NULL;
}
//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CGmanBoss :: Precache()
{
	PRECACHE_MODEL("models/gman_boss.mdl");
	PRECACHE_SOUND("doma/doma_fire1.wav");
	PRECACHE_SOUND("doma/doma_fire2.wav");
	PRECACHE_SOUND("weapons/chronoclip_explode.wav" );
	PRECACHE_SOUND("newadd/fist_hearvy_hit1.wav");
	PRECACHE_SOUND("newadd/fist_hearvy_hit2.wav");
	PRECACHE_SOUND("newadd/exp2_frost.wav");
	PRECACHE_SOUND("newadd/dragonball_dash.wav");
	PRECACHE_SOUND("newadd/PowerShield.wav");
	PRECACHE_SOUND("weapons/mortarhit.wav");

	UTIL_PrecacheOther( "gman_stomp" );
}	
