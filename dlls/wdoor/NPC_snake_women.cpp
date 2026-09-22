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

extern DLL_GLOBAL int		g_iSkillLevel;


//=========================================================
// Monster's Anim Events Go Here
//=========================================================

class CSnakeScythe : public CBaseEntity
{
public:
	void Spawn( void );

	static void Shoot( entvars_t *pevOwner, Vector vecStart, Vector vecVelocity );
	void Touch( CBaseEntity *pOther );
	void EXPORT Remove_Thinking( void );
};

LINK_ENTITY_TO_CLASS( snake_scythe, CSnakeScythe );

void CSnakeScythe:: Spawn( void )
{
	pev->movetype = MOVETYPE_FLY;
	pev->classname = MAKE_STRING( "snake_scythe" );
	
	pev->solid = SOLID_BBOX;
	
	SET_MODEL(ENT(pev), "models/snake_scythe.mdl");
	pev->body = 0;
	pev->frame = 0;
	pev->framerate = 1.0;

	pev->effects		= EF_DIMLIGHT;

	MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
	WRITE_BYTE( TE_BEAMFOLLOW );
	WRITE_SHORT( entindex() + 0x1000 * 1 );		// entity, attachment
	WRITE_SHORT(g_sModelIndexTrail);	// model
	WRITE_BYTE( 6 ); // life
	WRITE_BYTE( 4 );  // width
	WRITE_BYTE( 255 );	// R
	WRITE_BYTE( 255 );	// G
	WRITE_BYTE( 192 );	// B
	WRITE_BYTE( 192 );	// brightness
	MESSAGE_END();  // move PHS/PVS data sending into here (SEND_ALL, SEND_PVS, SEND_PHS)

	MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
	WRITE_BYTE( TE_BEAMFOLLOW );
	WRITE_SHORT( entindex() + 0x1000 * 2 );		// entity, attachment
	WRITE_SHORT(g_sModelIndexTrail);	// model
	WRITE_BYTE( 6 ); // life
	WRITE_BYTE( 4 );  // width
	WRITE_BYTE( 255 );	// R
	WRITE_BYTE( 255 );	// G
	WRITE_BYTE( 192 );	// B
	WRITE_BYTE( 192 );	// brightness
	MESSAGE_END();  // move PHS/PVS data sending into here (SEND_ALL, SEND_PVS, SEND_PHS)

	UTIL_SetSize( pev, Vector( -1, -1, 1), Vector(1, 1, 1) );
}

void CSnakeScythe::Remove_Thinking( void )
{
	MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
	WRITE_BYTE( TE_KILLBEAM );
	WRITE_SHORT( entindex() + 0x1000 * 1 );		// entity, attachment
	MESSAGE_END();

	MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
	WRITE_BYTE( TE_KILLBEAM );
	WRITE_SHORT( entindex() + 0x1000 * 2 );		// entity, attachment
	MESSAGE_END();

	UTIL_Remove( this );
	return;
}

void CSnakeScythe::Shoot( entvars_t *pevOwner, Vector vecStart, Vector vecVelocity )
{
	CSnakeScythe *pSpit = GetClassPtr( (CSnakeScythe *)NULL );
	pSpit->Spawn();
	
	UTIL_SetOrigin( pSpit->pev, vecStart );
	pSpit->pev->velocity = vecVelocity;
	pSpit->pev->owner = ENT(pevOwner);
	pSpit->pev->angles = UTIL_VecToAngles (pSpit->pev->velocity);
}

void CSnakeScythe :: Touch ( CBaseEntity *pOther )
{
	if(pev->frags == 1){
	return;
	}

	if ( UTIL_PointContents(pev->origin) == CONTENT_SKY )
	{
		UTIL_Remove( this );
		return;
	}

	// splat sound
	int iPitch = RANDOM_FLOAT( 90, 110 );	

	TraceResult tr = UTIL_GetGlobalTrace( );
	if (tr.pHit == pOther->edict())
	{
		EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, "snakewomen/attack_hit2.wav", 1, ATTN_NORM, 0, iPitch );	

		Vector vecSrc = Center();
		Vector vecEnd	= vecSrc + pev->velocity * 10;
		int tex = (int)TEXTURETYPE_Trace(&tr, vecSrc, vecEnd);
		int surface = (int)SURFACETYPE_Trace(&tr, vecSrc, vecEnd,0,CLASS_PLAYER);
		FX_ImpBullet( tr.vecEndPos, tr.vecPlaneNormal, vecSrc, surface, BULLET_CROWBAR, (float)tex );

		ClearMultiDamage( );
		pOther->TraceAttack(pev, 90, gpGlobals->v_forward, &tr, DMG_ENERGYBEAM); 

		if(!FNullEnt(pev->owner)){
		entvars_t	*pevOwner;
		pevOwner = VARS( pev->owner );
		ApplyMultiDamage( pev, pevOwner );
		}
		else{
		ApplyMultiDamage( pev, pev );
		}
	}

	if ( pOther->pev->flags & (FL_MONSTER|FL_CLIENT) )
	{
		if(pOther->pev->gravity <= 1.5){//����Ч��
		pOther->pev->velocity = (pOther->pev->origin - pev->origin).Normalize() + pev->velocity * 0.1;
		}
	}

	pev->solid = SOLID_NOT;
	FX_Explosion( pev->origin, 50);
	pev->frags = 1;
	SetThink ( &CSnakeScythe::Remove_Thinking );
	pev->nextthink = gpGlobals->time + 0.5;
}


class CSnakeWomen : public CBaseMonster
{
public:
	void Spawn( void );
	void Precache( void );
	void SetYawSpeed( void );
	int  Classify ( void );
	void HandleAnimEvent( MonsterEvent_t *pEvent );

	float m_flKillBeamTime;
	float m_flNextSkillTime;

	void Killed( entvars_t *pevAttacker, int iGib );

	Schedule_t *GetScheduleOfType ( int Type );
	Schedule_t *GetSchedule ( void );

	void RunAI( void );

	void DeathSound( void );

	// No range attacks
	BOOL CheckRangeAttack1 ( float flDot, float flDist );
	BOOL CheckMeleeAttack1 ( float flDot, float flDist );

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	int TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType );

};

LINK_ENTITY_TO_CLASS( monster_snake_women, CSnakeWomen );

TYPEDESCRIPTION	CSnakeWomen::m_SaveData[] = 
{
	DEFINE_FIELD( CSnakeWomen, m_flNextSkillTime, FIELD_TIME ),
	DEFINE_FIELD( CSnakeWomen, m_flKillBeamTime, FIELD_TIME ),
};

IMPLEMENT_SAVERESTORE( CSnakeWomen, CBaseMonster );

BOOL CSnakeWomen :: CheckRangeAttack1 ( float flDot, float flDist )
{
	float dist = 1800;

	if(m_flNextSkillTime > gpGlobals->time){
	return FALSE;
	}
	
	if ( !HasConditions( bits_COND_ENEMY_OCCLUDED ) && flDist <= dist && flDot >= 0.5)
	{
	return TRUE;
	}

	return FALSE;
}

void CSnakeWomen :: DeathSound( void )
{
	EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "snakewomen/die1.wav", 1, 0.6, 0, 100);
}

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int	CSnakeWomen :: Classify ( void )
{
	return	CLASS_HUMAN_ASS;
}

void CSnakeWomen :: RunAI( void )
{
	if(pev->sequence == LookupActivity ( ACT_RUN )
	|| pev->sequence == LookupActivity ( ACT_RUN_SCARED )
	|| pev->sequence == LookupActivity ( ACT_WALK_SCARED )){
	m_flGroundSpeed = 300;
	}

	if(m_flKillBeamTime > 0 && m_flKillBeamTime < gpGlobals->time){
	MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
	WRITE_BYTE( TE_KILLBEAM );
	WRITE_SHORT( entindex() + 0x1000 * 1 );		// entity, attachment
	MESSAGE_END();
	m_flKillBeamTime = 0;
	}

	if(pev->weapons == 1){//��ˮģʽ��δ���ֵ���ǰ����������ͻϮ
			m_flDistLook = 1280.0;
			m_singdelay_max = 0;
			m_singdelay_use = m_singdelay_max;
			pev->weapons = 2;
			pev->takedamage = DAMAGE_NO;
			pev->effects |= EF_NODRAW;
			m_longming = 1;
			m_selfmode = TRUE;
			pev->solid	= SOLID_NOT;
	}
	else if(pev->weapons >= 2){
			if(pev->weapons == 2 && m_hEnemy != NULL){
			pev->weapons = 3;
			pev->takedamage = DAMAGE_AIM;
			pev->effects &= ~EF_NODRAW;
			pev->solid	= SOLID_SLIDEBOX;
			}
			if ( pev->waterlevel >= 1){
			Killed( pev, GIB_ALWAYS );//��ʬ!
			return;
			}
	}

	CBaseMonster :: RunAI();
}


BOOL CSnakeWomen :: CheckMeleeAttack1 ( float flDot, float flDist )
{
	float dist = 120;
	int height_attack = 0;
	if(m_hEnemy != NULL){
		if ( fabs( pev->origin.z - m_hEnemy->pev->origin.z ) >= 80 )
		{
			if (m_hEnemy->pev->flags & FL_ONGROUND)
			{
			dist += 80;
			height_attack = 1;
			}
		}
	}

			if (flDist <= dist && m_hEnemy != NULL && flDot >= 0.5)
			{
				if (m_hEnemy->IsAlive()){
					if(height_attack == 1 && pev->sequence != LookupActivity ( ACT_WALK_SCARED )){
						pev->sequence = LookupActivity ( ACT_WALK_SCARED );
						ResetSequenceInfo( );
						pev->frame = 0;
					}
					else if(height_attack == 0 && pev->sequence != LookupActivity ( ACT_RUN_SCARED )){
						pev->sequence = LookupActivity ( ACT_RUN_SCARED );
						ResetSequenceInfo( );
						pev->frame = 0;
					}
				}
			}
			else if (flDist >= dist + 60 )
			{
					if(pev->sequence == LookupActivity ( ACT_RUN_SCARED ) 
					|| pev->sequence == LookupActivity ( ACT_WALK_SCARED )){
					pev->sequence = LookupActivity ( ACT_RUN );
					ResetSequenceInfo( );
					pev->frame = 0;
					}
			}

	return FALSE;
}

Schedule_t* CSnakeWomen :: GetScheduleOfType ( int Type )
{
	return CBaseMonster::GetScheduleOfType( Type );
}

//=========================================================
// GetSchedule - Decides which type of schedule best suits
// the monster's current state and conditions. Then calls
// monster's member function to get a pointer to a schedule
// of the proper type.
//=========================================================
Schedule_t *CSnakeWomen :: GetSchedule ( void )
{
	return CBaseMonster::GetSchedule();
}
//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CSnakeWomen :: SetYawSpeed ( void )
{
	pev->yaw_speed = 180;
}

int CSnakeWomen :: TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	//ħ������
	if ( (bitsDamageType & DMG_SHOCK) || (bitsDamageType & DMG_ENERGYBLAST) 
	|| (bitsDamageType & DMG_FREEZE) || (bitsDamageType & DMG_BURN)
	|| (bitsDamageType & DMG_ENERGYBEAM) || (bitsDamageType & DMG_DARK)
	|| (bitsDamageType & DMG_SONIC)){
		flDamage *= 0.8;
	}

	return CBaseMonster::TakeDamage( pevInflictor, pevAttacker, flDamage, bitsDamageType );
}

void CSnakeWomen::Killed( entvars_t *pevAttacker, int iGib )
{
	CBaseMonster::Killed( pevAttacker, iGib );
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CSnakeWomen :: HandleAnimEvent( MonsterEvent_t *pEvent )
{
	int dmg;
	dmg = 75;

	switch( pEvent->event )
	{
		case 1:
		{
			TraceResult tr;
			UTIL_MakeVectors(pev->angles);
			Vector vecSrc	= BodyTarget_d(pev->origin);
			Vector vecEnd	= vecSrc + gpGlobals->v_forward * 130;
			UTIL_TraceLine( vecSrc, vecEnd, dont_ignore_monsters, ENT( pev ), &tr );

			CBaseEntity *pEntity = CBaseEntity::Instance(tr.pHit);

			if ( tr.flFraction < 1.0 && pEntity->pev->takedamage ){
			int tex = (int)TEXTURETYPE_Trace(&tr, vecSrc, vecEnd);
			int surface = (int)SURFACETYPE_Trace(&tr, vecSrc, vecEnd,Classify(),1);
			FX_ImpBullet( tr.vecEndPos, tr.vecPlaneNormal, vecSrc, surface, BULLET_CROWBAR, (float)tex );

			ClearMultiDamage( );
			pEntity->TraceAttack(pev, dmg, gpGlobals->v_forward, &tr, DMG_ENERGYBEAM); 
			ApplyMultiDamage( pev, pev );
			EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, "snakewomen/attack_hit.wav", 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
			}
			else{
				CBaseEntity *pHurt = CheckTraceHullAttack( 130, dmg, DMG_ENERGYBEAM);
				if(pHurt){
					if (pHurt->Classify() == CLASS_PLAYER || pHurt->Classify() == CLASS_PLAYER_ALLY
					|| pHurt->Classify() == CLASS_HUMAN_ASS || pHurt->Classify() == CLASS_HUMAN_PASSIVE
					|| pHurt->Classify() == CLASS_HUMAN_MILITARY){
					FX_Explosion( pHurt->Center(), 234 );
					}
					else if (pHurt->Classify() == CLASS_ALIEN_MONSTER || pHurt->Classify() == CLASS_ALIEN_MILITARY){
					FX_Explosion( pHurt->Center(), 235 );
					}
				EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, "snakewomen/attack_hit.wav", 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
				}
			}
			
		}
		break;

		case 2://��β1
		{
			MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
			WRITE_BYTE( TE_BEAMFOLLOW );
			WRITE_SHORT( entindex() + 0x1000 * 1 );		// entity, attachment
			WRITE_SHORT(g_sModelIndexTrail);	// model
			WRITE_BYTE( 4 ); // life
			WRITE_BYTE( 4 );  // width
			WRITE_BYTE( 255 );	// R
			WRITE_BYTE( 255 );	// G
			WRITE_BYTE( 128 );	// B
			WRITE_BYTE( 192 );	// brightness
			MESSAGE_END();  // move PHS/PVS data sending into here (SEND_ALL, SEND_PVS, SEND_PHS)

			m_flKillBeamTime = gpGlobals->time + 1.5;
		}
		break;

		case 3:
		{
			if(m_hEnemy != NULL){
				if(( pev->origin - m_hEnemy->pev->origin).Length() <= 180){
				TraceResult tr;
				UTIL_MakeVectors(pev->angles);
				Vector vecSrc	= BodyTarget(pev->origin);
				Vector vecEnd	= m_hEnemy->Center();
				UTIL_TraceLine( vecSrc, vecEnd, dont_ignore_monsters, ENT( pev ), &tr );
				CBaseEntity *pEntity = CBaseEntity::Instance(tr.pHit);

				if ( tr.flFraction < 1.0 && pEntity->pev->takedamage ){
				int tex = (int)TEXTURETYPE_Trace(&tr, vecSrc, vecEnd);
				int surface = (int)SURFACETYPE_Trace(&tr, vecSrc, vecEnd,Classify(),1);
				FX_ImpBullet( tr.vecEndPos, tr.vecPlaneNormal, vecSrc, surface, BULLET_CROWBAR, (float)tex );

				ClearMultiDamage( );
				pEntity->TraceAttack(pev, dmg, gpGlobals->v_forward, &tr, DMG_ENERGYBEAM ); 
				ApplyMultiDamage( pev, pev );
				EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, "snakewomen/attack_hit.wav", 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
				}

				}
			}

			if(m_flNextSkillTime <= gpGlobals->time && m_hEnemy != NULL){

				Vector	vecSpitOffset,vangle;
				Vector	vecSpitDir;

				UTIL_MakeVectors ( pev->angles );

				// !!!HACKHACK - the spot at which the spit originates (in front of the mouth) was measured in 3ds and hardcoded here.
				// we should be able to read the position of bones at runtime for this info.
				GetAttachment( 0, vecSpitOffset, vangle );

				vecSpitDir = ( m_hEnemy->Center() - vecSpitOffset ).Normalize();

				// do stuff for this event.
				//AttackSound();

				EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, "snakewomen/attack_range.wav", 1.0, 0.7,0,100 + RANDOM_LONG(-5,5) );

				CSnakeScythe::Shoot( pev, vecSpitOffset, vecSpitDir * 3000 );
				m_flNextSkillTime = gpGlobals->time + 6;
				}
		}
		break;

		case 4:
		{
		
			if(m_hEnemy != NULL){
				if(( pev->origin - m_hEnemy->pev->origin).Length() <= 180){
				TraceResult tr;
				UTIL_MakeVectors(pev->angles);
				Vector vecSrc	= BodyTarget(pev->origin);
				Vector vecEnd	= m_hEnemy->Center();
				UTIL_TraceLine( vecSrc, vecEnd, dont_ignore_monsters, ENT( pev ), &tr );
				CBaseEntity *pEntity = CBaseEntity::Instance(tr.pHit);

				if ( tr.flFraction < 1.0 && pEntity->pev->takedamage ){
				int tex = (int)TEXTURETYPE_Trace(&tr, vecSrc, vecEnd);
				int surface = (int)SURFACETYPE_Trace(&tr, vecSrc, vecEnd,Classify(),1);
				FX_ImpBullet( tr.vecEndPos, tr.vecPlaneNormal, vecSrc, surface, BULLET_CROWBAR, (float)tex );

				ClearMultiDamage( );
				pEntity->TraceAttack(pev, dmg, gpGlobals->v_forward, &tr, DMG_ENERGYBEAM ); 
				ApplyMultiDamage( pev, pev );
				EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, "snakewomen/attack_hit.wav", 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
				}

				}
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
void CSnakeWomen :: Spawn()
{
	Precache( );
	SET_MODEL(ENT(pev), "models/snake_women.mdl");
	UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	m_bloodColor		= BLOOD_COLOR_RED;

	if (g_iSkillLevel == SKILL_HARD){
	pev->health			= 360;
	}
	else{
	pev->health			= 300;
	}

	pev->view_ofs		= VEC_VIEW;// position of the eyes relative to monster's origin.
	m_flFieldOfView		= 0.5;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState		= MONSTERSTATE_NONE;
	m_afCapability		= bits_CAP_DOORS_GROUP;
	
	MonsterInit();

	m_headdef	 = 2;//ͷ��Ӳ��

	pev->skin = 0;

	m_ignoredamage		= 2;

	m_MoveFail_SimpleRoad = TRUE;
	m_killed_exp = 130;
	m_rpgms_level = 50;

	m_flNextSkillTime = gpGlobals->time + 2;

	pev->netname = MAKE_STRING( "Snake.Women" );
}
//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CSnakeWomen :: Precache()
{
	PRECACHE_MODEL("models/snake_women.mdl");
	PRECACHE_MODEL("models/snake_scythe.mdl");

	PRECACHE_SOUND("snakewomen/die1.wav");
	PRECACHE_SOUND("snakewomen/attack_range.wav");
	PRECACHE_SOUND("snakewomen/attack_hit.wav");
	PRECACHE_SOUND("snakewomen/attack_hit2.wav");
}	
