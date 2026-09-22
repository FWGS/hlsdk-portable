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
#include	"game.h"
#include	"player.h"
#include	"soundent.h"

extern DLL_GLOBAL int		g_iSkillLevel;

//=========================================================
// Monster's Anim Events Go Here
//=========================================================

class CGonome : public CBaseMonster
{
public:
	void Spawn( void );
	void Precache( void );
	void SetYawSpeed( void );
	int  Classify ( void );
	void HandleAnimEvent( MonsterEvent_t *pEvent );

	float m_flNextFlinch;
	float m_flNextPainTime;

	int IgnoreConditions ( void );
	void Killed( entvars_t *pevAttacker, int iGib );

	void EXPORT LeapTouch ( CBaseEntity *pOther );

	void PainSound( void );
	void DeathSound( void );
	void IdleSound( void );
	void AttackSound( void );
	void RunAI( void );
	BOOL CheckMeleeAttack1 ( float flDot, float flDist );
	BOOL CheckRangeAttack1 ( float flDot, float flDist );
	BOOL CheckRangeAttack2 ( float flDot, float flDist );

	BOOL FCanCheckAttacks ( void );

	void TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType);

	static const char *pAttackSounds[];
	static const char *pIdleSounds[];
	static const char *pDeathSounds[];
	static const char *pPainSounds[];
	static const char *pAttackHitSounds[];
	static const char *pAttackMissSounds[];

	int TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType );
	void StartTask ( Task_t *pTask );

	int	Save( CSave &save ); 
	int Restore( CRestore &restore );

	static TYPEDESCRIPTION m_SaveData[];

	float m_flNextJumpTime;// we keep track of this, because if something hurts a squid, it will forget about its love of headcrabs for a while.
	float m_flNextSpitTime;// last time the bullsquid used the spit attack.
};

LINK_ENTITY_TO_CLASS( monster_gonome, CGonome );

TYPEDESCRIPTION	CGonome::m_SaveData[] = 
{
	DEFINE_FIELD( CGonome, m_flNextJumpTime, FIELD_TIME ),
	DEFINE_FIELD( CGonome, m_flNextSpitTime, FIELD_TIME ),
};

IMPLEMENT_SAVERESTORE( CGonome, CBaseMonster );

const char *CGonome::pAttackHitSounds[] = 
{
	"zombie/claw_strike1.wav",
	"zombie/claw_strike2.wav",
	"zombie/claw_strike3.wav",
};

const char *CGonome::pAttackMissSounds[] = 
{
	"zombie/claw_miss1.wav",
	"zombie/claw_miss2.wav",
};

const char *CGonome::pAttackSounds[] = 
{
	"gonome/gonome_melee1.wav",
	"gonome/gonome_melee2.wav",
};

const char *CGonome::pIdleSounds[] = 
{
	"gonome/gonome_idle1.wav",
	"gonome/gonome_idle2.wav",
	"gonome/gonome_idle3.wav",
};

const char *CGonome::pDeathSounds[] = 
{
	"gonome/gonome_death2.wav",
	"gonome/gonome_death3.wav",
};

const char *CGonome::pPainSounds[] = 
{
	"gonome/gonome_pain1.wav",
	"gonome/gonome_pain2.wav",
	"gonome/gonome_pain3.wav",
};


//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int	CGonome :: Classify ( void )
{
	return	CLASS_ALIEN_MONSTER;
}

void CGonome :: LeapTouch ( CBaseEntity *pOther )
{
	if ( !pOther->pev->takedamage || pev->deadflag != DEAD_NO){
		return;
	}

	if ( pOther->Classify() == Classify() ){
		return;
	}

	if ( pOther->pev->flags & (FL_MONSTER|FL_CLIENT) )
	{
		if ( pOther->pev->flags & FL_MONSTER){
			if(pev->origin.z - 64 >= pOther->pev->origin.z){
			pOther->TakeDamage ( pev, pev, 100, DMG_CRUSH );
			pOther->pev->velocity.x += 1000;
			}
		}
		else{
			if(pev->origin.z - 32 >= pOther->pev->origin.z){
			pOther->TakeDamage ( pev, pev, 100, DMG_CRUSH );
			pOther->pev->velocity.x += 1000;
			}
		}
	}

	SetTouch( NULL );
}


//=========================================================
// RunAI
//=========================================================
void CGonome :: RunAI( void )
{
	CBaseMonster :: RunAI();
	if(pev->sequence == LookupActivity ( ACT_RUN )
	|| pev->sequence == LookupActivity ( ACT_RUN_SCARED )){
	m_flGroundSpeed = 260;
	}
}

void CGonome::TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType)
{
	if(ptr->iHitgroup == 1)
	{
		m_zombiehead_health -= flDamage;
		if(m_zombiehead_health <= 0 && pev->health > flDamage){
		pev->health = 0;
		}
		flDamage *= 0.5;
	}
	CBaseMonster::TraceAttack( pevAttacker, flDamage, vecDir, ptr, bitsDamageType );
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CGonome :: SetYawSpeed ( void )
{
	pev->yaw_speed = 180;
}

BOOL CGonome :: FCanCheckAttacks ( void )
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


BOOL CGonome :: CheckMeleeAttack1 ( float flDot, float flDist )
{
	if (flDist <= 150 && m_hEnemy != NULL && flDot >= 0.6)
	{
		if (m_hEnemy->IsAlive() ){
			if(pev->sequence == LookupActivity ( ACT_RUN )){
			pev->sequence = LookupActivity ( ACT_RUN_SCARED );
			ResetSequenceInfo( );
			pev->frame = 0;
			}
		}
	}
	else if (flDist > 200)
	{
			if(pev->sequence == LookupActivity ( ACT_RUN_SCARED )){
			pev->sequence = LookupActivity ( ACT_RUN );
			ResetSequenceInfo( );
			pev->frame = 0;
			}
	}
	return FALSE;
}

BOOL CGonome :: CheckRangeAttack2 ( float flDot, float flDist )
{
	return FALSE;
}

BOOL CGonome :: CheckRangeAttack1 ( float flDot, float flDist )
{
	if(GetBodygroup( 1 ) == 3){
	return FALSE;
	}
	int limit_dist = 256;
	int long_dist = 1024;
	if ( m_hEnemy != NULL )
	{
		if ( fabs( pev->origin.z - m_hEnemy->pev->origin.z ) > 128 )
		{
		limit_dist = 128;
		long_dist = 1280;
		}
	}

	if(pev->impulse > 0){//Ͷ��ģʽ?
		if(gpGlobals->time >= m_flNextSpitTime){
			return TRUE;
		}
	}

	if ( flDist > limit_dist && flDist <= long_dist && flDot >= 0.5 && gpGlobals->time >= m_flNextSpitTime)
	{
		if ( IsMoving() )
		{
			// don't spit again for a long time, resume chasing enemy.
			m_flNextSpitTime = gpGlobals->time + 5.0;
		}
		else
		{
			// not moving, so spit again pretty soon.
			m_flNextSpitTime = gpGlobals->time + 0.5;
		}

		TraceResult	tr;

		Vector vecSrc = BodyTarget(pev->origin);

		// verify that a bullet fired from the gun will hit the enemy before the world.
		UTIL_TraceLine( vecSrc, m_hEnemy->BodyTarget(vecSrc), ignore_monsters, ignore_glass, ENT(pev), &tr);

		if ( tr.flFraction == 1.0 )
		{
			return TRUE;
		}
	}

	return FALSE;
}

int CGonome :: TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	return CBaseMonster::TakeDamage( pevInflictor, pevAttacker, flDamage, bitsDamageType );
}

void CGonome::Killed( entvars_t *pevAttacker, int iGib )
{
	if(m_die == 0 && !ShouldGibMonster( iGib )){
		Vector vecGunPos;
		Vector vecGunAngles;
		CBaseEntity *pCrab;

		if(m_zombiehead_health > 30){
		m_zombiehead_health = 30;//ͷм�ļ���Ѫ��
		}

		if ( GetBodygroup( 0 ) == 1 )
		{// ���ϵ�ͷз
			SetBodygroup( 0, 2 );
			GetAttachment( 0, vecGunPos, vecGunAngles );
			pCrab = CBaseEntity::Create( "monster_headcrab", vecGunPos, pev->angles, edict() );
			UTIL_MakeVectors ( pev->angles ); 
			pCrab->pev->velocity = gpGlobals->v_forward * 50;

			if(m_diefadeout == 1){
			SetBits( pCrab->pev->spawnflags, SF_MONSTER_FADECORPSE );
			}
		}
		if ( m_zombiehead_health > 0 && GetBodygroup( 0 ) != 2 )
		{// ͷ�ϵ�ͷз
			SetBodygroup( 0, 2 );
			GetAttachment( 1, vecGunPos, vecGunAngles );
			pCrab = CBaseEntity::Create( "monster_headcrab", vecGunPos, pev->angles, edict() );
			pCrab->pev->health = m_zombiehead_health;
			UTIL_MakeVectors ( pev->angles ); 
			pCrab->pev->velocity = gpGlobals->v_forward * 50;

			if(m_diefadeout == 1){
			SetBits( pCrab->pev->spawnflags, SF_MONSTER_FADECORPSE );
			}
		}
		else{
			m_killed_exp = 110;
		}

		if ( GetBodygroup( 1 ) == 0 )
		{// ���ϵ�ͷз 3
			SetBodygroup( 1, 3 );
			GetAttachment( 2, vecGunPos, vecGunAngles );
			pCrab = CBaseEntity::Create( "monster_headcrab", vecGunPos, pev->angles, edict() );
			UTIL_MakeVectors (vecGunAngles); 
			pCrab->pev->velocity = gpGlobals->v_up * 100;
			pCrab = CBaseEntity::Create( "monster_headcrab", vecGunPos + Vector(0,0,20) , pev->angles, edict() );
			pCrab->pev->velocity = gpGlobals->v_right * -100;
			pCrab = CBaseEntity::Create( "monster_headcrab", vecGunPos - Vector(0,0,20) , pev->angles, edict() );
			pCrab->pev->velocity = gpGlobals->v_right * 100;

			if(m_diefadeout == 1){
			SetBits( pCrab->pev->spawnflags, SF_MONSTER_FADECORPSE );
			}
		}
		else if ( GetBodygroup( 1 ) == 1 )
		{// ���ϵ�ͷз 2
			SetBodygroup( 1, 3 );
			GetAttachment( 2, vecGunPos, vecGunAngles );
			pCrab = CBaseEntity::Create( "monster_headcrab", vecGunPos, pev->angles, edict() );
			UTIL_MakeVectors (vecGunAngles); 
			pCrab->pev->velocity = gpGlobals->v_up * 100;
			pCrab = CBaseEntity::Create( "monster_headcrab", vecGunPos - Vector(0,0,20) , pev->angles, edict() );
			pCrab->pev->velocity = gpGlobals->v_right * 100;

			if(m_diefadeout == 1){
			SetBits( pCrab->pev->spawnflags, SF_MONSTER_FADECORPSE );
			}
		}
		else if ( GetBodygroup( 1 ) == 2 )
		{// ���ϵ�ͷз 1
			SetBodygroup( 1, 3 );
			GetAttachment( 2, vecGunPos, vecGunAngles );
			pCrab = CBaseEntity::Create( "monster_headcrab", vecGunPos, pev->angles, edict() );
			UTIL_MakeVectors (vecGunAngles); 
			pCrab->pev->velocity = gpGlobals->v_up * 100;

			if(m_diefadeout == 1){
			SetBits( pCrab->pev->spawnflags, SF_MONSTER_FADECORPSE );
			}
		}
	}

	CBaseMonster::Killed( pevAttacker, iGib );
}

void CGonome :: PainSound( void )
{
	if ( m_flNextPainTime > gpGlobals->time )
	{
		return;
	}

	m_flNextPainTime = gpGlobals->time + 0.75;

	int pitch = 95 + RANDOM_LONG(0,9);

	EMIT_SOUND_DYN ( ENT(pev), CHAN_VOICE, pPainSounds[ RANDOM_LONG(0,ARRAYSIZE(pPainSounds)-1) ], 1.0, ATTN_NORM, 0, pitch );
}

void CGonome :: DeathSound( void )
{
	int pitch = 95 + RANDOM_LONG(0,9);

	EMIT_SOUND_DYN ( ENT(pev), CHAN_VOICE, pDeathSounds[ RANDOM_LONG(0,ARRAYSIZE(pDeathSounds)-1) ], 1.0, ATTN_NORM, 0, pitch );
}

void CGonome :: IdleSound( void )
{
	int pitch = 95 + RANDOM_LONG(0,9);

	// Play a random idle sound
	EMIT_SOUND_DYN ( ENT(pev), CHAN_VOICE, pIdleSounds[ RANDOM_LONG(0,ARRAYSIZE(pIdleSounds)-1) ], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
}

void CGonome :: AttackSound( void )
{
	// Play a random attack sound
	EMIT_SOUND_DYN ( ENT(pev), CHAN_VOICE, pAttackSounds[ RANDOM_LONG(0,ARRAYSIZE(pAttackSounds)-1) ], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
}



void CGonome :: StartTask ( Task_t *pTask )
{
	m_iTaskStatus = TASKSTATUS_RUNNING;

	switch ( pTask->iTask )
	{
	case TASK_RANGE_ATTACK2:
		{
			m_IdealActivity = ACT_RANGE_ATTACK2;
			break;
		}
	default:
		{
			CBaseMonster :: StartTask( pTask );
		}
	}
}


void CGonome :: HandleAnimEvent( MonsterEvent_t *pEvent )
{
	int dmg1,dmg2;
	dmg1 = 25;
	dmg2 = 12;

	switch( pEvent->event )
	{
		case 1:
		{
			TraceResult tr;
			UTIL_MakeVectors(pev->angles);
			Vector vecSrc	= BodyTarget_d(pev->origin);
			Vector vecEnd	= vecSrc + gpGlobals->v_forward * 105;
			UTIL_TraceLine( vecSrc, vecEnd, dont_ignore_monsters, ENT( pev ), &tr );

			CBaseEntity *pEntity = CBaseEntity::Instance(tr.pHit);

			if ( tr.flFraction < 1.0 && pEntity->pev->takedamage ){
			int tex = (int)TEXTURETYPE_Trace(&tr, vecSrc, vecEnd);
			int surface = (int)SURFACETYPE_Trace(&tr, vecSrc, vecEnd,Classify(),0);
			FX_ImpBullet( tr.vecEndPos, tr.vecPlaneNormal, vecSrc, surface, BULLET_CROWBAR, (float)tex );

			ClearMultiDamage( );
			pEntity->TraceAttack(pev, dmg1, gpGlobals->v_forward, &tr, DMG_SLASH ); 
				if ( pEntity->pev->flags & (FL_MONSTER|FL_CLIENT) )
				{
				pEntity->pev->velocity = pEntity->pev->velocity + (pEntity->pev->origin - pev->origin).Normalize() * 200;
				}
			ApplyMultiDamage( pev, pev );
			EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, pAttackHitSounds[ RANDOM_LONG(0,ARRAYSIZE(pAttackHitSounds)-1) ], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
			}
			else{
				CBaseEntity *pHurt = CheckTraceHullAttack( 105, dmg1, DMG_SLASH );
				if ( pHurt )
				{
					if ( pHurt->pev->flags & (FL_MONSTER|FL_CLIENT) )
					{
					pHurt->pev->velocity = pHurt->pev->velocity + (pHurt->pev->origin - pev->origin).Normalize() * 200;
					}
				EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, pAttackHitSounds[ RANDOM_LONG(0,ARRAYSIZE(pAttackHitSounds)-1) ], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
				}
				else{
				EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, pAttackMissSounds[ RANDOM_LONG(0,ARRAYSIZE(pAttackMissSounds)-1) ], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
				}
			}
		}
		break;

		case 2:
		{
			TraceResult tr;
			UTIL_MakeVectors(pev->angles);
			Vector vecSrc	= BodyTarget_d(pev->origin);
			Vector vecEnd	= vecSrc + gpGlobals->v_forward * 105;
			UTIL_TraceLine( vecSrc, vecEnd, dont_ignore_monsters, ENT( pev ), &tr );

			CBaseEntity *pEntity = CBaseEntity::Instance(tr.pHit);

			if ( tr.flFraction < 1.0 && pEntity->pev->takedamage ){
			int tex = (int)TEXTURETYPE_Trace(&tr, vecSrc, vecEnd);
			int surface = (int)SURFACETYPE_Trace(&tr, vecSrc, vecEnd,Classify(),0);
			FX_ImpBullet( tr.vecEndPos, tr.vecPlaneNormal, vecSrc, surface, BULLET_CROWBAR, (float)tex );

			ClearMultiDamage( );
			pEntity->TraceAttack(pev, dmg1, gpGlobals->v_forward, &tr, DMG_SLASH ); 
				if ( pEntity->pev->flags & (FL_MONSTER|FL_CLIENT) )
				{
				pEntity->pev->velocity = pEntity->pev->velocity + (pEntity->pev->origin - pev->origin).Normalize() * 200;
				}
			ApplyMultiDamage( pev, pev );
			EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, pAttackHitSounds[ RANDOM_LONG(0,ARRAYSIZE(pAttackHitSounds)-1) ], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
			}
			else{
				CBaseEntity *pHurt = CheckTraceHullAttack( 105, dmg1, DMG_SLASH );
				if ( pHurt )
				{
					if ( pHurt->pev->flags & (FL_MONSTER|FL_CLIENT) )
					{
					pHurt->pev->velocity = pHurt->pev->velocity + (pHurt->pev->origin - pev->origin).Normalize() * 200;
					}
				EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, pAttackHitSounds[ RANDOM_LONG(0,ARRAYSIZE(pAttackHitSounds)-1) ], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
				}
				else{
				EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, pAttackMissSounds[ RANDOM_LONG(0,ARRAYSIZE(pAttackMissSounds)-1) ], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
				}
			}

		}
		break;

		case 3:
		{
			
			if ( GetBodygroup( 0 ) == 0 ){//����û��ͷз

				if(pev->impulse == 0){
					if ( GetBodygroup( 1 ) == 0 )
					{// ���ϵ�ͷз 3
						SetBodygroup( 1, 1 );
					}
					else if ( GetBodygroup( 1 ) == 1 )
					{// ���ϵ�ͷз 2
						SetBodygroup( 1, 2 );
					}
					else if ( GetBodygroup( 1 ) == 2 )
					{// ���ϵ�ͷз 1
						SetBodygroup( 1, 3 );
					}
				}
				else{//Bug Fix 3.0 ͷзͶ�֤�ģʽ
					if( pev->impulse >= 20 && (pev->flags & FL_ONGROUND) ){

						MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
						WRITE_BYTE( TE_BEAMFOLLOW );
						WRITE_SHORT( entindex() );		// entity, attachment
						WRITE_SHORT(g_sModelIndexTrail);	// model
						WRITE_BYTE( 10 ); // life
						WRITE_BYTE( 10 );  // width
						WRITE_BYTE( 255 );	// R
						WRITE_BYTE( 255 );	// G
						WRITE_BYTE( 255 );	// B
						WRITE_BYTE( 255 );	// brightness
						MESSAGE_END();

						if( pev->impulse <= 40){
						pev->velocity.z += RANDOM_FLOAT( 0, 900 );
						}
						else if( pev->impulse <= 60){
						pev->velocity.z += RANDOM_FLOAT( 0, 1500 );
						}
						else if(pev->weapons != 0){
						UTIL_SetOrigin (pev, m_vecOldMovePoint + Vector (0,RANDOM_LONG(-1280,1280),RANDOM_LONG(0,1536)));
						}

						if(pev->weapons == 0 && pev->impulse > 60){//TM�ι���չ��ģʽ!
						CBaseEntity *pEntity = Create( "monster_kadoma2", pev->origin, pev->angles, NULL );
						SET_MODEL(ENT(pEntity->pev), "models/zdeadeye.mdl");
						pEntity->pev->movetype = MOVETYPE_FOLLOW;
						pEntity->pev->aiment = edict();
						pev->effects = EF_LIGHT;
						pev->movetype = MOVETYPE_FLY;
						pev->weapons = 1;
						FX_Explosion( Center(), EXPLOSION_CHRONOCLIP );
						EMIT_SOUND_DYN( ENT(pev), CHAN_STREAM, "weapons/chronoclip_explode.wav", 1, 0.4, 0, 100);
						SERVER_COMMAND("mp3 loop media/Half-Life05.mp3\n");
						m_vecOldMovePoint = pev->origin;
						ClearSchedule();
						SetActivity( ACT_IDLE );
						m_flNextSpitTime = gpGlobals->time + 3.0;
						return;
						}

						ClearBits( pev->flags, FL_ONGROUND );
					}
				}

				SetBodygroup( 0, 1 );//���ϵ�ͷз
			}
		}
		break;

		case 4:
		{
			if(m_hEnemy != NULL){
				SetBodygroup( 0, 0 );//���ϵ�ͷз

				Vector	vecSpitOffset;
				Vector	vecSpitDir,vecdir;

				UTIL_MakeVectors ( pev->angles );

				GetAttachment( 0, vecSpitOffset,vecdir);
				vecSpitOffset = pev->origin + Vector(0,0,64);
				vecSpitOffset = vecSpitOffset + gpGlobals->v_forward * 16;

				Vector enemy_origin = m_hEnemy->pev->origin;
				float flDist = ( pev->origin - m_hEnemy->pev->origin).Length();
				float height = flDist * 0.12 - 40;

				if(pev->impulse == 0 && height > 0){
				enemy_origin.z += height;
				}

				vecSpitDir = ( ( enemy_origin + m_hEnemy->pev->view_ofs ) - vecSpitOffset ).Normalize();

				
				CBaseEntity *pCrab = CBaseEntity::Create( "monster_headcrab_throw", vecSpitOffset, pev->angles, edict() );
				if(pev->impulse <= 0){
					pCrab->pev->velocity = vecSpitDir * 2000;
					pCrab->pev->sequence = 21;
					pCrab->pev->frame = 0;
					pCrab->pev->movetype = MOVETYPE_TOSS;
				}
				else{//Bug Fix 3.0 ͷзͶ�֤�ģʽ
					float tsped = 160 + (pev->impulse * 40);
					float tasped = 10 + (pev->impulse * 5);
					pev->impulse += 1;

					//pCrab->pev->effects = EF_DIMLIGHT;

					if(pev->impulse <= 5){
					m_flNextSpitTime = gpGlobals->time + 6.0;
					}
					else if(pev->impulse <= 20){
					m_flNextSpitTime = gpGlobals->time + 5.0;
					}
					else if(pev->impulse <= 60){
					m_flNextSpitTime = gpGlobals->time + 4.0;
					}
					else if(pev->impulse <= 100){//����ģʽ!
						if(pev->weapons != 0){
						UTIL_SetOrigin (pev, m_vecOldMovePoint);
						pev->velocity = g_vecZero;
						tsped = 1000 + (pev->impulse * 50);
						pCrab->pev->effects = EF_LIGHT;
						tasped = 200 + (pev->impulse * 10);
						}
						m_flNextSpitTime = gpGlobals->time + 3.0;
					}
					else{//����ȫ��
						UTIL_SetOrigin (pev, m_vecOldMovePoint);
						pCrab->pev->effects = EF_LIGHT;
						tsped = 9000;//MAX SPEED
						tasped = 1500;
					}

					pCrab->pev->velocity = vecSpitDir * tsped;
					if(RANDOM_LONG(0,1)){
					pCrab->pev->avelocity.z = tasped;
					}
					else{
					pCrab->pev->avelocity.z = -tasped;
					}
					pCrab->pev->sequence = 21;
					pCrab->pev->frame = 0;
					CBaseMonster *pEnemyMonster = pCrab->MyMonsterPointer();
					pEnemyMonster->m_IdealMonsterState	= MONSTERSTATE_HUNT;
					pCrab->pev->movetype = MOVETYPE_FLY;
					pCrab->pev->spawnflags |= SF_MONSTER_FADECORPSE;
					pCrab->pev->impulse = 278;//��ɱ��ʾ
					
					pCrab->pev->gravity = 0.2;
					pCrab->pev->health = 10;

					MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
					WRITE_BYTE( TE_BEAMFOLLOW );
					WRITE_SHORT( pCrab->entindex() );		// entity, attachment
					WRITE_SHORT(g_sModelIndexTrail);	// model
					WRITE_BYTE( 10 ); // life
					WRITE_BYTE( 5 );  // width
					WRITE_BYTE( 255 );	// R
					WRITE_BYTE( 255 );	// G
					WRITE_BYTE( 255 );	// B
					WRITE_BYTE( 255 );	// brightness
					MESSAGE_END();  // move PHS/PVS data sending into here (SEND_ALL, SEND_PVS, SEND_PHS)
				}

				ClearBits( pCrab->pev->flags, FL_ONGROUND );
			}
		}
		break;

		case 5:
		{
			TraceResult tr;
			UTIL_MakeVectors(pev->angles);
			Vector vecSrc	= BodyTarget_d(pev->origin);
			Vector vecEnd	= vecSrc + gpGlobals->v_forward * 90;
			UTIL_TraceLine( vecSrc, vecEnd, dont_ignore_monsters, ENT( pev ), &tr );

			CBaseEntity *pEntity = CBaseEntity::Instance(tr.pHit);

			if ( tr.flFraction < 1.0 && pEntity->pev->takedamage ){
			int tex = (int)TEXTURETYPE_Trace(&tr, vecSrc, vecEnd);
			int surface = (int)SURFACETYPE_Trace(&tr, vecSrc, vecEnd,Classify(),0);
			FX_ImpBullet( tr.vecEndPos, tr.vecPlaneNormal, vecSrc, surface, BULLET_CROWBAR, (float)tex );

			ClearMultiDamage( );
			pEntity->TraceAttack(pev, dmg2, gpGlobals->v_forward, &tr, DMG_SLASH ); 
			ApplyMultiDamage( pev, pev );
			EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, pAttackHitSounds[ RANDOM_LONG(0,ARRAYSIZE(pAttackHitSounds)-1) ], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
			}
			else{
				CBaseEntity *pHurt = CheckTraceHullAttack( 90, dmg2, DMG_SLASH );
				if ( pHurt )
				{
				EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, pAttackHitSounds[ RANDOM_LONG(0,ARRAYSIZE(pAttackHitSounds)-1) ], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
				}
				else{
				EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, pAttackMissSounds[ RANDOM_LONG(0,ARRAYSIZE(pAttackMissSounds)-1) ], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
				}
			}

		}
		break;

		case 6:
		{
			Vector vecJumpDir;
			ClearBits( pev->flags, FL_ONGROUND );
			UTIL_SetOrigin (pev, pev->origin + Vector ( 0 , 0 , 1) );// take him off ground so engine doesn't instantly reset onground 
			m_alert = 100;
			m_boltpoison = 20;

			EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "headcrab/hc_attack1.wav", 1, ATTN_NORM, 0, 100);

			FireTargets( "gon_break_glass", this, this, USE_TOGGLE, 0 );

			UTIL_MakeVectors ( pev->angles );
			pev->velocity = pev->velocity + gpGlobals->v_forward * 100 + gpGlobals->v_up * 100;

			SetTouch ( &CGonome::LeapTouch );
		}
		break;

		case 7:
		{
			if( (pev->velocity.x < 60 || pev->velocity.x > -60) && (pev->velocity.y < 60 || pev->velocity.y > -60) ){
			UTIL_MakeVectors ( pev->angles );
			pev->velocity = pev->velocity + gpGlobals->v_up * 200;
			}
		}
		break;

		case 8:
		{
			if( (pev->velocity.x < 60 || pev->velocity.x > -60) && (pev->velocity.y < 60 || pev->velocity.y > -60) ){
			UTIL_MakeVectors ( pev->angles );
			pev->velocity = pev->velocity + gpGlobals->v_forward * 200;
			}
		}
		break;

		case 9:
		{
			EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "gonome/gonome_melee1.wav", 1, ATTN_NORM, 0, 100);
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
void CGonome :: Spawn()
{
	Precache( );

	SET_MODEL(ENT(pev), "models/gonome.mdl");
	UTIL_SetSize( pev, Vector( -16, -16, 0 ), Vector( 16, 16, 80 ) );

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	m_bloodColor		= BLOOD_COLOR_GREEN;

	if (g_iSkillLevel == SKILL_HARD){
	pev->health			= 250;
	m_zombiehead_health = 80;
	}
	else{
	pev->health			= 200;
	m_zombiehead_health = 60;
	}

	pev->view_ofs		= VEC_VIEW;// position of the eyes relative to monster's origin.
	m_flFieldOfView		= 0.3;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState		= MONSTERSTATE_NONE;
	m_afCapability		= bits_CAP_DOORS_GROUP;
	pev->body			= 0;
	pev->gravity		= 1.5;

	m_flNextJumpTime = gpGlobals->time;
	m_flNextSpitTime = gpGlobals->time;

	m_victoryeat		= TRUE;
	m_ignoredamage		= 1;

	MonsterInit();
	m_chase_mode = 2;
	m_chase_failed_max = 4;
	m_EyeMod = 2;//Ŀ��ģʽ
	m_killed_exp = 90;
	m_rpgms_level = 50;
	pev->netname = MAKE_STRING( "Gonome.Zombie" );
}
//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CGonome :: Precache()
{
	int i;

	PRECACHE_MODEL("models/gonome.mdl");
	PRECACHE_SOUND("bullchicken/bc_spithit1.wav" );
	PRECACHE_SOUND("bullchicken/bc_spithit2.wav" );
	PRECACHE_SOUND("headcrab/hc_attack1.wav" );

	UTIL_PrecacheOther( "monster_headcrab" );

	for ( i = 0; i < ARRAYSIZE( pAttackHitSounds ); i++ )
		PRECACHE_SOUND((char *)pAttackHitSounds[i]);

	for ( i = 0; i < ARRAYSIZE( pAttackMissSounds ); i++ )
		PRECACHE_SOUND((char *)pAttackMissSounds[i]);

	for ( i = 0; i < ARRAYSIZE( pAttackSounds ); i++ )
		PRECACHE_SOUND((char *)pAttackSounds[i]);

	for ( i = 0; i < ARRAYSIZE( pIdleSounds ); i++ )
		PRECACHE_SOUND((char *)pIdleSounds[i]);

	for ( i = 0; i < ARRAYSIZE( pDeathSounds ); i++ )
		PRECACHE_SOUND((char *)pDeathSounds[i]);

	for ( i = 0; i < ARRAYSIZE( pPainSounds ); i++ )
		PRECACHE_SOUND((char *)pPainSounds[i]);
}	

int CGonome::IgnoreConditions ( void )
{
	int iIgnore = CBaseMonster::IgnoreConditions();

	if ( m_hEnemy != NULL ){
	iIgnore |= (bits_COND_LIGHT_DAMAGE|bits_COND_HEAVY_DAMAGE);
	}

	return iIgnore;
}