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
// bullsquid - big, spotty tentacle-mouthed meanie.
//=========================================================

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"schedule.h"
#include	"nodes.h"
#include	"effects.h"
#include	"decals.h"
#include	"soundent.h"
#include	"game.h"
#include	"weapons.h"

#define		SQUID_SPRINT_DIST	256 // how close the squid has to get before starting to sprint and refusing to swerve

int			   iSquidSpitSprite;
extern DLL_GLOBAL int		g_iSkillLevel;

//=========================================================
// monster-specific schedule types
//=========================================================
enum
{
	SCHED_SQUID_HURTHOP = LAST_COMMON_SCHEDULE + 1,
	SCHED_SQUID_SMELLFOOD,
	SCHED_SQUID_SEECRAB,
	SCHED_SQUID_EAT,
	SCHED_SQUID_SNIFF_AND_EAT,
	SCHED_SQUID_WALLOW,
};

//=========================================================
// monster-specific tasks
//=========================================================
enum 
{
	TASK_SQUID_HOPTURN = LAST_COMMON_TASK + 1,
};

//=========================================================
// Bullsquid's spit projectile
//=========================================================
class CSquidSpit : public CBaseEntity
{
public:
	void Spawn( void );

	static void Shoot( entvars_t *pevOwner, Vector vecStart, Vector vecVelocity );
	void Touch( CBaseEntity *pOther );
	void EXPORT Animate( void );

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	int  m_maxFrame;
};

LINK_ENTITY_TO_CLASS( squidspit, CSquidSpit );

TYPEDESCRIPTION	CSquidSpit::m_SaveData[] = 
{
	DEFINE_FIELD( CSquidSpit, m_maxFrame, FIELD_INTEGER ),
};

IMPLEMENT_SAVERESTORE( CSquidSpit, CBaseEntity );

void CSquidSpit:: Spawn( void )
{
	pev->movetype = MOVETYPE_FLY;
	pev->classname = MAKE_STRING( "squidspit" );
	
	pev->solid = SOLID_BBOX;
	pev->rendermode = kRenderTransAlpha;
	pev->renderamt = 255;

	SET_MODEL(ENT(pev), "sprites/bigspit.spr");
	pev->frame = 0;
	pev->scale = 0.5;

	UTIL_SetSize( pev, Vector( 0, 0, 0), Vector(0, 0, 0) );

	m_maxFrame = (float) MODEL_FRAMES( pev->modelindex ) - 1;
}

void CSquidSpit::Animate( void )
{
	pev->nextthink = gpGlobals->time + 0.1;

	if ( pev->frame++ )
	{
		if ( pev->frame > m_maxFrame )
		{
			pev->frame = 0;
		}
	}
}

void CSquidSpit::Shoot( entvars_t *pevOwner, Vector vecStart, Vector vecVelocity )
{
	CSquidSpit *pSpit = GetClassPtr( (CSquidSpit *)NULL );
	pSpit->Spawn();
	
	UTIL_SetOrigin( pSpit->pev, vecStart );
	pSpit->pev->velocity = vecVelocity;
	pSpit->pev->owner = ENT(pevOwner);

	pSpit->SetThink ( &CSquidSpit::Animate );
	pSpit->pev->nextthink = gpGlobals->time + 0.1;
}

void CSquidSpit :: Touch ( CBaseEntity *pOther )
{
	TraceResult tr;
	int		iPitch;

	// splat sound
	iPitch = RANDOM_FLOAT( 90, 110 );

	EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "bullchicken/bc_acid1.wav", 1, ATTN_NORM, 0, iPitch );	

	switch ( RANDOM_LONG( 0, 1 ) )
	{
	case 0:
		EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, "bullchicken/bc_spithit1.wav", 1, ATTN_NORM, 0, iPitch );	
		break;
	case 1:
		EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, "bullchicken/bc_spithit2.wav", 1, ATTN_NORM, 0, iPitch );	
		break;
	}


	Vector vecSrc = pev->origin;
	Vector vecEnd	= vecSrc + pev->velocity * 10;
	UTIL_TraceLine( vecSrc, vecEnd, dont_ignore_monsters, ENT( pev ), &tr );

	int tex = (int)TEXTURETYPE_Trace(&tr, pev->origin, vecEnd);
	int surface = (int)SURFACETYPE_Trace(&tr, pev->origin, vecEnd,Classify(),0);
	FX_ImpBullet( tr.vecEndPos, tr.vecPlaneNormal, pev->origin, surface, BULLET_CROWBAR, (float)tex );


	if ( !pOther->pev->takedamage )
	{
		// make some flecks
		MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, tr.vecEndPos );
			WRITE_BYTE( TE_SPRITE_SPRAY );
			WRITE_COORD( tr.vecEndPos.x);	// pos
			WRITE_COORD( tr.vecEndPos.y);	
			WRITE_COORD( tr.vecEndPos.z);	
			WRITE_COORD( tr.vecPlaneNormal.x);	// dir
			WRITE_COORD( tr.vecPlaneNormal.y);	
			WRITE_COORD( tr.vecPlaneNormal.z);	
			WRITE_SHORT( iSquidSpitSprite );	// model
			WRITE_BYTE ( 5 );			// count
			WRITE_BYTE ( 30 );			// speed
			WRITE_BYTE ( 80 );			// noise ( client will divide by 100 )
		MESSAGE_END();
		// make a splat on the wall
	//	UTIL_TraceLine( pev->origin, pev->origin + pev->velocity * 10, dont_ignore_monsters, ENT( pev ), &tr );
		UTIL_DecalTrace(&tr, DECAL_SPIT1 + RANDOM_LONG(0,1));
	}
	else
	{
		int dmg3;
		dmg3 = 20;

		ClearMultiDamage( );
		pOther->TraceAttack(pev, dmg3, gpGlobals->v_forward, &tr, DMG_GENERIC ); 
		ApplyMultiDamage( pev, pev );

	//	pOther->TakeDamage ( pev, pev, dmg3, DMG_GENERIC );
	}

	SetThink ( &CSquidSpit::SUB_Remove );
	pev->nextthink = gpGlobals->time;
}

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define		BSQUID_AE_SPIT		( 1 )
#define		BSQUID_AE_BITE		( 2 )
#define		BSQUID_AE_BLINK		( 3 )
#define		BSQUID_AE_TAILWHIP	( 4 )
#define		BSQUID_AE_HOP		( 5 )
#define		BSQUID_AE_THROW		( 6 )

class CBullsquid : public CBaseMonster
{
public:
	void Spawn( void );
	void Precache( void );
	void SetYawSpeed( void );
	int  ISoundMask( void );
	int  Classify ( void );
//	void Killed( entvars_t *pevAttacker, int iGib );
	void HandleAnimEvent( MonsterEvent_t *pEvent );
	void IdleSound( void );
	void PainSound( void );
	void DeathSound( void );
	void AlertSound ( void );
	void AttackSound( void );
	void StartTask ( Task_t *pTask );
	void RunTask ( Task_t *pTask );
	BOOL CheckMeleeAttack1 ( float flDot, float flDist );
	BOOL CheckMeleeAttack2 ( float flDot, float flDist );
	BOOL CheckRangeAttack1 ( float flDot, float flDist );
	void RunAI( void );
	BOOL FValidateHintType ( short sHint );
	Schedule_t *GetSchedule( void );
	Schedule_t *GetScheduleOfType ( int Type );
	int TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType );
	int IRelationship ( CBaseEntity *pTarget );
	int IgnoreConditions ( void );
	MONSTERSTATE GetIdealState ( void );

	int	Save( CSave &save ); 
	int Restore( CRestore &restore );

	CUSTOM_SCHEDULES;
	static TYPEDESCRIPTION m_SaveData[];

	BOOL m_fCanThreatDisplay;// this is so the squid only does the "I see a headcrab!" dance one time. 

	float m_flLastHurtTime;// we keep track of this, because if something hurts a squid, it will forget about its love of headcrabs for a while.
	float m_flNextSpitTime;// last time the bullsquid used the spit attack.
};
LINK_ENTITY_TO_CLASS( monster_bullchicken, CBullsquid );
LINK_ENTITY_TO_CLASS( monster_bullchicken_big, CBullsquid );

TYPEDESCRIPTION	CBullsquid::m_SaveData[] = 
{
	DEFINE_FIELD( CBullsquid, m_fCanThreatDisplay, FIELD_BOOLEAN ),
	DEFINE_FIELD( CBullsquid, m_flLastHurtTime, FIELD_TIME ),
	DEFINE_FIELD( CBullsquid, m_flNextSpitTime, FIELD_TIME ),
};

IMPLEMENT_SAVERESTORE( CBullsquid, CBaseMonster );

//=========================================================
// IgnoreConditions 
//=========================================================
int CBullsquid::IgnoreConditions ( void )
{
	int iIgnore = CBaseMonster::IgnoreConditions();

	if ( gpGlobals->time - m_flLastHurtTime <= 20 )
	{
		// haven't been hurt in 20 seconds, so let the squid care about stink. 
		iIgnore = bits_COND_SMELL | bits_COND_SMELL_FOOD;
	}

	if ( m_hEnemy != NULL )
	{
		if ( FClassnameIs( m_hEnemy->pev, "monster_headcrab" ) )
		{
			// (Unless after a tasty headcrab)
			iIgnore = bits_COND_SMELL | bits_COND_SMELL_FOOD;
		}
	}


	return iIgnore;
}

//=========================================================
// IRelationship - overridden for bullsquid so that it can
// be made to ignore its love of headcrabs for a while.
//=========================================================
int CBullsquid::IRelationship ( CBaseEntity *pTarget )
{
	/*
	if ( FClassnameIs ( pTarget->pev, "monster_headcrab" ) )
	{
		// if squid has been hurt in the last 5 seconds, and is getting relationship for a headcrab, 
		// tell squid to disregard crab. 
		return R_DL;
	}
	*/

	return CBaseMonster :: IRelationship ( pTarget );
}

//=========================================================
// TakeDamage - overridden for bullsquid so we can keep track
// of how much time has passed since it was last injured
//=========================================================
int CBullsquid :: TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	/* ���ܵ�����������
	float flDist;
	Vector vecApex;

	// if the squid is running, has an enemy, was hurt by the enemy, hasn't been hurt in the last 3 seconds, and isn't too close to the enemy,
	// it will swerve. (whew).
	
	if ( m_hEnemy != NULL && IsMoving() && pevAttacker == m_hEnemy->pev && gpGlobals->time - m_flLastHurtTime > 3 )
	{
		flDist = ( pev->origin - m_hEnemy->pev->origin ).Length2D();
		
		if ( flDist > SQUID_SPRINT_DIST )
		{
			flDist = ( pev->origin - m_Route[ m_iRouteIndex ].vecLocation ).Length2D();// reusing flDist. 

			if ( FTriangulate( pev->origin, m_Route[ m_iRouteIndex ].vecLocation, flDist * 0.5, m_hEnemy, &vecApex ) )
			{
				InsertWaypoint( vecApex, bits_MF_TO_DETOUR | bits_MF_DONT_SIMPLIFY );
			}
		}
	}
	*/

	if ( !FClassnameIs ( pevAttacker, "monster_headcrab" ) )
	{
		// don't forget about headcrabs if it was a headcrab that hurt the squid.
		m_flLastHurtTime = gpGlobals->time;
	}

	return CBaseMonster :: TakeDamage ( pevInflictor, pevAttacker, flDamage, bitsDamageType );
}

//=========================================================
// CheckRangeAttack1
//=========================================================
BOOL CBullsquid :: CheckRangeAttack1 ( float flDot, float flDist )
{
	if ( FClassnameIs(pev, "monster_bullchicken_big") ){
	return FALSE;
	}

	if ( IsMoving() && flDist <= 512 )
	{
		// squid will far too far behind if he stops running to spit at this distance from the enemy.
		return FALSE;
	}

	if ( flDist > 256 && flDist <= 1536 && flDot >= 0.5 && gpGlobals->time >= m_flNextSpitTime )
	{
		if ( m_hEnemy != NULL )
		{
			if ( fabs( pev->origin.z - m_hEnemy->pev->origin.z ) > 256 )
			{
				// don't try to spit at someone up really high or down really low.
				return FALSE;
			}
		}

		if ( IsMoving() )
		{
			// don't spit again for a long time, resume chasing enemy.
			m_flNextSpitTime = gpGlobals->time + 5;
		}
		else
		{
			// not moving, so spit again pretty soon.
			m_flNextSpitTime = gpGlobals->time + 0.5;
		}

		return TRUE;
	}

	return FALSE;
}

//=========================================================
// CheckMeleeAttack1 - bullsquid is a big guy, so has a longer
// melee range than most monsters. This is the tailwhip attack
//=========================================================
BOOL CBullsquid :: CheckMeleeAttack2 ( float flDot, float flDist )
{
	if ( FClassnameIs(pev, "monster_bullchicken_big")){
	return FALSE;
	}

	if ( m_hEnemy != NULL && flDist <= 128)
	{
		if (m_hEnemy->pev->flags && FL_CLIENT){
			if(m_hEnemy->pev->origin.z - 72 >= pev->origin.z){
			return TRUE;
			}
			else if(!FBitSet ( m_hEnemy->pev->flags, FL_ONGROUND ) && m_hEnemy->pev->origin.z - 48 >= pev->origin.z && flDist <= 96){
			return TRUE;
			}
		}
		else if(m_hEnemy->pev->origin.z - 36 >= pev->origin.z){
			return TRUE;
		}
	}

	return FALSE;
}

BOOL CBullsquid :: CheckMeleeAttack1 ( float flDot, float flDist )
{
	if ( FClassnameIs(pev, "monster_bullchicken_big") ){
			float dist = 110;
			if(m_hEnemy != NULL){
				if ( fabs( pev->origin.z - m_hEnemy->pev->origin.z ) > 60 )
				{
					if (m_hEnemy->pev->flags & FL_ONGROUND)
					{
					dist += 60;
					}
				}
			}

			if (flDist <= dist && m_hEnemy != NULL && flDot >= 0.5)
			{
				if (m_hEnemy->IsAlive() ){
					if(pev->sequence == LookupActivity ( ACT_RUN )){
					pev->sequence = LookupActivity ( ACT_RUN_SCARED );
					ResetSequenceInfo( );
					pev->frame = 0;
					}
				}
			}
			else if (flDist >= dist + 50 )
			{
					if(pev->sequence == LookupActivity ( ACT_RUN_SCARED )){
					pev->sequence = LookupActivity ( ACT_RUN );
					ResetSequenceInfo( );
					pev->frame = 0;
					}
			}
			return FALSE;
	}


	if ( flDist <= 85 && flDot >= 0.7 && !HasConditions( bits_COND_CAN_MELEE_ATTACK2 ) )		// The player & bullsquid can be as much as their bboxes 
	{										// apart (48 * sqrt(3)) and he can still attack (85 is a little more than 48*sqrt(3))
		return TRUE;
	}
	return FALSE;
}  

//=========================================================
//  FValidateHintType 
//=========================================================
BOOL CBullsquid :: FValidateHintType ( short sHint )
{
	int i;

	static short sSquidHints[] =
	{
		HINT_WORLD_HUMAN_BLOOD,
	};

	for ( i = 0 ; i < ARRAYSIZE ( sSquidHints ) ; i++ )
	{
		if ( sSquidHints[ i ] == sHint )
		{
			return TRUE;
		}
	}

	ALERT ( at_aiconsole, "Couldn't validate hint type" );
	return FALSE;
}

//=========================================================
// ISoundMask - returns a bit mask indicating which types
// of sounds this monster regards. In the base class implementation,
// monsters care about all sounds, but no scents.
//=========================================================
int CBullsquid :: ISoundMask ( void )
{
	return	bits_SOUND_WORLD	|
			bits_SOUND_COMBAT	|
			bits_SOUND_CARCASS	|
			bits_SOUND_MEAT		|
			bits_SOUND_GARBAGE	|
			bits_SOUND_PLAYER;
}

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int	CBullsquid :: Classify ( void )
{
	return	CLASS_ALIEN_MONSTER;
}

//=========================================================
// IdleSound 
//=========================================================
#define SQUID_ATTN_IDLE	(float)1.5
void CBullsquid :: IdleSound ( void )
{
	switch ( RANDOM_LONG(0,4) )
	{
	case 0:	
		EMIT_SOUND( ENT(pev), CHAN_VOICE, "bullchicken/bc_idle1.wav", 1, SQUID_ATTN_IDLE );	
		break;
	case 1:	
		EMIT_SOUND( ENT(pev), CHAN_VOICE, "bullchicken/bc_idle2.wav", 1, SQUID_ATTN_IDLE );	
		break;
	case 2:	
		EMIT_SOUND( ENT(pev), CHAN_VOICE, "bullchicken/bc_idle3.wav", 1, SQUID_ATTN_IDLE );	
		break;
	case 3:	
		EMIT_SOUND( ENT(pev), CHAN_VOICE, "bullchicken/bc_idle4.wav", 1, SQUID_ATTN_IDLE );	
		break;
	case 4:	
		EMIT_SOUND( ENT(pev), CHAN_VOICE, "bullchicken/bc_idle5.wav", 1, SQUID_ATTN_IDLE );	
		break;
	}
}

//=========================================================
// PainSound 
//=========================================================
void CBullsquid :: PainSound ( void )
{
	int iPitch = RANDOM_LONG( 85, 120 );

	switch ( RANDOM_LONG(0,3) )
	{
	case 0:	
		EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "bullchicken/bc_pain1.wav", 1, ATTN_NORM, 0, iPitch );	
		break;
	case 1:	
		EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "bullchicken/bc_pain2.wav", 1, ATTN_NORM, 0, iPitch );	
		break;
	case 2:	
		EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "bullchicken/bc_pain3.wav", 1, ATTN_NORM, 0, iPitch );	
		break;
	case 3:	
		EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "bullchicken/bc_pain4.wav", 1, ATTN_NORM, 0, iPitch );	
		break;
	}
}

//=========================================================
// AlertSound
//=========================================================
void CBullsquid :: AlertSound ( void )
{
	int iPitch = RANDOM_LONG( 140, 160 );

	switch ( RANDOM_LONG ( 0, 1  ) )
	{
	case 0:
		EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "bullchicken/bc_idle1.wav", 1, ATTN_NORM, 0, iPitch );	
		break;
	case 1:
		EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "bullchicken/bc_idle2.wav", 1, ATTN_NORM, 0, iPitch );	
		break;
	}
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CBullsquid :: SetYawSpeed ( void )
{
	pev->yaw_speed = 120;
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CBullsquid :: HandleAnimEvent( MonsterEvent_t *pEvent )
{
	int dmg1,dmg2;
	dmg1 = 30;
	dmg2 = 45;

	switch( pEvent->event )
	{
		case BSQUID_AE_SPIT:
		{
			if(m_hEnemy != NULL){
			Vector	vecSpitOffset;
			Vector	vecSpitDir;

			UTIL_MakeVectors ( pev->angles );

			// !!!HACKHACK - the spot at which the spit originates (in front of the mouth) was measured in 3ds and hardcoded here.
			// we should be able to read the position of bones at runtime for this info.
			vecSpitOffset = ( gpGlobals->v_right * 8 + gpGlobals->v_forward * 37 + gpGlobals->v_up * 23 );		
			vecSpitOffset = ( pev->origin + vecSpitOffset );
			vecSpitDir = ( ( m_hEnemy->pev->origin + m_hEnemy->pev->view_ofs ) - vecSpitOffset ).Normalize();

			vecSpitDir.x += RANDOM_FLOAT( -0.03, 0.03 );
			vecSpitDir.y += RANDOM_FLOAT( -0.03, 0.03 );
			vecSpitDir.z += RANDOM_FLOAT( -0.03, 0 );


			// do stuff for this event.
			AttackSound();

			// spew the spittle temporary ents.
			MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, vecSpitOffset );
				WRITE_BYTE( TE_SPRITE_SPRAY );
				WRITE_COORD( vecSpitOffset.x);	// pos
				WRITE_COORD( vecSpitOffset.y);	
				WRITE_COORD( vecSpitOffset.z);	
				WRITE_COORD( vecSpitDir.x);	// dir
				WRITE_COORD( vecSpitDir.y);	
				WRITE_COORD( vecSpitDir.z);	
				WRITE_SHORT( iSquidSpitSprite );	// model
				WRITE_BYTE ( 15 );			// count
				WRITE_BYTE ( 210 );			// speed
				WRITE_BYTE ( 25 );			// noise ( client will divide by 100 )
			MESSAGE_END();

			CSquidSpit::Shoot( pev, vecSpitOffset, vecSpitDir * 1250 );
			}
		}
		break;

		case BSQUID_AE_BITE:
		{
			int atk_dist = 90;
			if ( FClassnameIs(pev, "monster_bullchicken_big")){
				atk_dist = 120;
				dmg1 *= 2.0;
			}

					int iPitch = RANDOM_FLOAT( 90, 110 );
					switch ( RANDOM_LONG( 0, 1 ) )
					{
					case 0:
						EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, "bullchicken/bc_bite2.wav", 1, ATTN_NORM, 0, iPitch );	
						break;
					case 1:
						EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, "bullchicken/bc_bite3.wav", 1, ATTN_NORM, 0, iPitch );	
						break;
					}

			TraceResult tr;
			UTIL_MakeVectors(pev->angles);
			Vector vecSrc	= BodyTarget_c(pev->origin);
			Vector vecEnd	= vecSrc + gpGlobals->v_forward * atk_dist;
			UTIL_TraceLine( vecSrc, vecEnd, dont_ignore_monsters, ENT( pev ), &tr );

			CBaseEntity *pEntity = CBaseEntity::Instance(tr.pHit);

			/*
			if(m_hEnemy != NULL && FClassnameIs(pev, "monster_bullchicken_big")){
				if(( pev->origin - m_hEnemy->pev->origin).Length() <= atk_dist){
				m_hEnemy->TakeDamage( pev, pev, dmg1, DMG_SLASH );
				}
			}
			*/

			if ( tr.flFraction < 1.0 && pEntity->pev->takedamage ){
				int tex = (int)TEXTURETYPE_Trace(&tr, vecSrc, vecEnd);
				int surface = (int)SURFACETYPE_Trace(&tr, vecSrc, vecEnd,Classify(),1);
				FX_ImpBullet( tr.vecEndPos, tr.vecPlaneNormal, vecSrc, surface, BULLET_CROWBAR, (float)tex );
				ClearMultiDamage( );
				pEntity->TraceAttack(pev, dmg1, gpGlobals->v_forward, &tr, DMG_SLASH ); 
					if ( pEntity->pev->flags & (FL_MONSTER|FL_CLIENT) )
					{
					
						if ( FClassnameIs(pev, "monster_bullchicken_big")){
						pEntity->pev->velocity = pEntity->pev->velocity + (pEntity->pev->origin - pev->origin).Normalize() * 300;
						}
						else{
						pEntity->pev->velocity = pEntity->pev->velocity + (pEntity->pev->origin - pev->origin).Normalize() * 200;
						pEntity->pev->velocity.z += 200;
						}
					}
				ApplyMultiDamage( pev, pev );
			}
			else{
				CBaseEntity *pHurt = CheckTraceHullAttack( atk_dist, dmg1, DMG_SLASH );
				if ( pHurt )
				{
					if ( pHurt->pev->flags & (FL_MONSTER|FL_CLIENT) )
					{
						if ( FClassnameIs(pev, "monster_bullchicken_big")){
						pHurt->pev->velocity = pHurt->pev->velocity + (pHurt->pev->origin - pev->origin).Normalize() * 300;
						}
						else{
						pHurt->pev->velocity = pHurt->pev->velocity + (pHurt->pev->origin - pev->origin).Normalize() * 200;
						pHurt->pev->velocity.z += 200;
						}
					}
				}
			}

		}
		break;

		case BSQUID_AE_TAILWHIP:
		{
			if ( m_hEnemy != NULL)
			{
				float flDist = ( pev->origin - m_hEnemy->pev->origin).Length();
				if(flDist <= 128){
						if (m_hEnemy->pev->flags && FL_CLIENT){
							if(m_hEnemy->pev->origin.z - 72 >= pev->origin.z){
							m_hEnemy->TakeDamage ( pev, pev, dmg2, DMG_GENERIC );
							m_hEnemy->pev->velocity.x += RANDOM_LONG(-300,300);
							m_hEnemy->pev->velocity.z += 300;
							}
							else if(!FBitSet ( m_hEnemy->pev->flags, FL_ONGROUND ) && m_hEnemy->pev->origin.z - 48 >= pev->origin.z && flDist <= 96){
							m_hEnemy->TakeDamage ( pev, pev, dmg2, DMG_GENERIC );
							m_hEnemy->pev->velocity.x += RANDOM_LONG(-300,300);
							m_hEnemy->pev->velocity.z += 300;
							}
						}
						else if(m_hEnemy->pev->origin.z - 36 >= pev->origin.z){
						m_hEnemy->TakeDamage ( pev, pev, dmg2, DMG_GENERIC );
						m_hEnemy->pev->velocity.x += RANDOM_LONG(-300,300);
						m_hEnemy->pev->velocity.z += 300;
						}
				}
			}
		}
		break;

		case BSQUID_AE_BLINK:
		{
			// close eye. 
			pev->skin = 1;
		}
		break;

		case BSQUID_AE_HOP:
		{
			float flGravity = g_psv_gravity->value;

			// throw the squid up into the air on this frame.
			if ( FBitSet ( pev->flags, FL_ONGROUND ) )
			{
				pev->flags -= FL_ONGROUND;
			}

			// jump into air for 0.8 (24/30) seconds
//			pev->velocity.z += (0.875 * flGravity) * 0.5;
			pev->velocity.z += (0.8 * flGravity) * 0.5;
		}
		break;

		case BSQUID_AE_THROW:
			{
					
			}
		break;

		default:
			CBaseMonster::HandleAnimEvent( pEvent );
	}
}
/*
void CBullsquid::Killed( entvars_t *pevAttacker, int iGib )
{
	CBaseMonster::Killed( pevAttacker, GIB_NEVER );
}
*/
//=========================================================
// Spawn
//=========================================================
void CBullsquid :: Spawn()
{
	Precache( );

	SET_MODEL(ENT(pev), "models/bullsquid.mdl");
	UTIL_SetSize( pev, Vector( -24, -24, 0 ), Vector( 24, 24, 36 ) );

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	m_bloodColor		= BLOOD_COLOR_GREEN;
	pev->effects		= 0;

	if (g_iSkillLevel == SKILL_HARD){
	pev->health			= 100;
	}
	else{
	pev->health			= 80;
	}

	m_flFieldOfView		= 0.2;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState		= MONSTERSTATE_NONE;

	m_fCanThreatDisplay	= TRUE;
	m_flNextSpitTime = gpGlobals->time;

	MonsterInit();

	pev->gravity		= 1.5;

	m_chase_mode = 2;
	m_chase_failed_max = 4;
	m_killed_exp = 60;
	m_rpgms_level = 35;
	pev->netname = MAKE_STRING( "Bullsquid" );

	if ( FClassnameIs(pev, "monster_bullchicken_big")){
		SET_MODEL(ENT(pev), "models/bullsquid_giant.mdl");
		UTIL_SetSize( pev, Vector( -36, -36, 0 ), Vector( 36, 36, 72 ) );
		m_killed_exp = 100;
		m_rpgms_level = 45;

		if (g_iSkillLevel == SKILL_HARD){
		pev->health			= 200;
		}
		else{
		pev->health			= 160;
		}
		pev->max_health		= pev->health;

		m_ignoreFail_MAX = 30;
		m_ignoreFail_OFF = 0;
		m_MoveFail_FuckRoad = TRUE;
		m_MoveFail_SimpleRoad = TRUE;
		m_ignoredamage		= 2;

		pev->netname = MAKE_STRING( "Big.Bullsquid" );
		SetEyePosition();
	}
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CBullsquid :: Precache()
{
	PRECACHE_MODEL("models/bullsquid.mdl");
	PRECACHE_MODEL("models/bullsquid_giant.mdl");

	PRECACHE_MODEL("sprites/bigspit.spr");// spit projectile.
	
	iSquidSpitSprite = PRECACHE_MODEL("sprites/tinyspit.spr");// client side spittle.

	PRECACHE_SOUND("zombie/claw_miss2.wav");// because we use the basemonster SWIPE animation event

	PRECACHE_SOUND("bullchicken/bc_attack2.wav");
	PRECACHE_SOUND("bullchicken/bc_attack3.wav");
	
	PRECACHE_SOUND("bullchicken/bc_die1.wav");
	PRECACHE_SOUND("bullchicken/bc_die2.wav");
	PRECACHE_SOUND("bullchicken/bc_die3.wav");
	
	PRECACHE_SOUND("bullchicken/bc_idle1.wav");
	PRECACHE_SOUND("bullchicken/bc_idle2.wav");
	PRECACHE_SOUND("bullchicken/bc_idle3.wav");
	PRECACHE_SOUND("bullchicken/bc_idle4.wav");
	PRECACHE_SOUND("bullchicken/bc_idle5.wav");
	
	PRECACHE_SOUND("bullchicken/bc_pain1.wav");
	PRECACHE_SOUND("bullchicken/bc_pain2.wav");
	PRECACHE_SOUND("bullchicken/bc_pain3.wav");
	PRECACHE_SOUND("bullchicken/bc_pain4.wav");
	
	PRECACHE_SOUND("bullchicken/bc_attackgrowl.wav");
	PRECACHE_SOUND("bullchicken/bc_attackgrowl2.wav");
	PRECACHE_SOUND("bullchicken/bc_attackgrowl3.wav");

	PRECACHE_SOUND("bullchicken/bc_acid1.wav");

	PRECACHE_SOUND("bullchicken/bc_bite2.wav");
	PRECACHE_SOUND("bullchicken/bc_bite3.wav");

	PRECACHE_SOUND("bullchicken/bc_spithit1.wav");
	PRECACHE_SOUND("bullchicken/bc_spithit2.wav");

}	

//=========================================================
// DeathSound
//=========================================================
void CBullsquid :: DeathSound ( void )
{
	switch ( RANDOM_LONG(0,2) )
	{
	case 0:	
		EMIT_SOUND( ENT(pev), CHAN_VOICE, "bullchicken/bc_die1.wav", 1, ATTN_NORM );	
		break;
	case 1:
		EMIT_SOUND( ENT(pev), CHAN_VOICE, "bullchicken/bc_die2.wav", 1, ATTN_NORM );	
		break;
	case 2:
		EMIT_SOUND( ENT(pev), CHAN_VOICE, "bullchicken/bc_die3.wav", 1, ATTN_NORM );	
		break;
	}
}

//=========================================================
// AttackSound
//=========================================================
void CBullsquid :: AttackSound ( void )
{
	switch ( RANDOM_LONG(0,1) )
	{
	case 0:
		EMIT_SOUND( ENT(pev), CHAN_WEAPON, "bullchicken/bc_attack2.wav", 1, ATTN_NORM );	
		break;
	case 1:
		EMIT_SOUND( ENT(pev), CHAN_WEAPON, "bullchicken/bc_attack3.wav", 1, ATTN_NORM );	
		break;
	}
}


//========================================================
// RunAI - overridden for bullsquid because there are things
// that need to be checked every think.
//========================================================
void CBullsquid :: RunAI ( void )
{
	// first, do base class stuff
	CBaseMonster :: RunAI();

	if(pev->deadflag == DEAD_NO){
		if(pev->sequence == LookupActivity ( ACT_RUN )
		|| pev->sequence == LookupActivity ( ACT_RUN_SCARED )){
			if ( FClassnameIs(pev, "monster_bullchicken_big")){
			m_flGroundSpeed = 430;
			}
			else{
			m_flGroundSpeed = 290;
			}
		}

		if ( pev->skin != 0 )
		{
			// close eye if it was open.
			pev->skin = 0; 
		}

		if ( RANDOM_LONG(0,19) == 0 )
		{
			pev->skin = 1;
		}
	}

}

//========================================================
// AI Schedules Specific to this monster
//=========================================================

// primary range attack
Task_t	tlSquidRangeAttack1[] =
{
	{ TASK_STOP_MOVING,			0				},
	{ TASK_FACE_IDEAL,			(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE	},
};

Schedule_t	slSquidRangeAttack1[] =
{
	{ 
		tlSquidRangeAttack1,
		ARRAYSIZE ( tlSquidRangeAttack1 ), 
		bits_COND_NEW_ENEMY			|
		bits_COND_ENEMY_DEAD		|
		bits_COND_HEAVY_DAMAGE		|
		bits_COND_ENEMY_OCCLUDED	|
		bits_COND_NO_AMMO_LOADED,
		0,
		"Squid Range Attack1"
	},
};

// Chase enemy schedule
Task_t tlSquidChaseEnemy1[] = 
{
	{ TASK_SET_FAIL_SCHEDULE,	(float)SCHED_RANGE_ATTACK1	},// !!!OEM - this will stop nasty squid oscillation.
	{ TASK_GET_PATH_TO_ENEMY,	(float)0					},
	{ TASK_RUN_PATH,			(float)0					},
	{ TASK_WAIT_FOR_MOVEMENT,	(float)0					},
};

Schedule_t slSquidChaseEnemy[] =
{
	{ 
		tlSquidChaseEnemy1,
		ARRAYSIZE ( tlSquidChaseEnemy1 ),
		bits_COND_NEW_ENEMY			|
		bits_COND_ENEMY_DEAD		|
		bits_COND_SMELL_FOOD		|
		bits_COND_CAN_RANGE_ATTACK1	|
		bits_COND_CAN_MELEE_ATTACK1	|
		bits_COND_CAN_MELEE_ATTACK2	|
		bits_COND_TASK_FAILED		|
		bits_COND_HEAR_SOUND,
		
		bits_SOUND_DANGER			|
		bits_SOUND_MEAT,
		"Squid Chase Enemy"
	},
};



// Chase enemy schedule
Task_t tlSquidChaseEnemy2[] = 
{
	{ TASK_SET_FAIL_SCHEDULE,	(float)SCHED_CHASE_ENEMY_FAILED	},// !!!OEM - this will stop nasty squid oscillation.
	{ TASK_GET_PATH_TO_ENEMY,	(float)0					},
	{ TASK_RUN_PATH,			(float)0					},
	{ TASK_WAIT_FOR_MOVEMENT,	(float)0					},
};

Schedule_t slSquidChaseEnemy2[] =
{
	{ 
		tlSquidChaseEnemy2,
		ARRAYSIZE ( tlSquidChaseEnemy2 ),
		bits_COND_NEW_ENEMY			|
		bits_COND_ENEMY_DEAD		|
		bits_COND_SMELL_FOOD		|
		bits_COND_CAN_RANGE_ATTACK1	|
		bits_COND_CAN_MELEE_ATTACK1	|
		bits_COND_CAN_MELEE_ATTACK2	|
		bits_COND_TASK_FAILED		|
		bits_COND_HEAR_SOUND,
		
		bits_SOUND_DANGER			|
		bits_SOUND_MEAT,
		"Squid Chase Enemy"
	},
};

Task_t tlSquidHurtHop[] =
{
	{ TASK_STOP_MOVING,			(float)0		},
	{ TASK_SOUND_WAKE,			(float)0		},
	{ TASK_SQUID_HOPTURN,		(float)0		},
	{ TASK_FACE_ENEMY,			(float)0		},// in case squid didn't turn all the way in the air.
};

Schedule_t slSquidHurtHop[] =
{
	{
		tlSquidHurtHop,
		ARRAYSIZE ( tlSquidHurtHop ),
		0,
		0,
		"SquidHurtHop"
	}
};

Task_t tlSquidSeeCrab[] =
{
	{ TASK_STOP_MOVING,			(float)0		},
	{ TASK_SOUND_WAKE,			(float)0			},
	{ TASK_PLAY_SEQUENCE,		(float)ACT_EXCITED	},
	{ TASK_FACE_ENEMY,			(float)0			},
};

Schedule_t slSquidSeeCrab[] =
{
	{
		tlSquidSeeCrab,
		ARRAYSIZE ( tlSquidSeeCrab ),
		bits_COND_LIGHT_DAMAGE		|
		bits_COND_HEAVY_DAMAGE,
		0,
		"SquidSeeCrab"
	}
};

// squid walks to something tasty and eats it.
Task_t tlSquidEat[] =
{
	{ TASK_STOP_MOVING,				(float)0				},
	{ TASK_EAT,						(float)10				},// this is in case the squid can't get to the food
	{ TASK_STORE_LASTPOSITION,		(float)0				},
	{ TASK_GET_PATH_TO_BESTSCENT,	(float)0				},
	{ TASK_WALK_PATH,				(float)0				},
	{ TASK_WAIT_FOR_MOVEMENT,		(float)0				},
	{ TASK_PLAY_SEQUENCE,			(float)ACT_EAT			},
	{ TASK_PLAY_SEQUENCE,			(float)ACT_EAT			},
	{ TASK_PLAY_SEQUENCE,			(float)ACT_EAT			},
	{ TASK_EAT,						(float)50				},
	{ TASK_GET_PATH_TO_LASTPOSITION,(float)0				},
	{ TASK_WALK_PATH,				(float)0				},
	{ TASK_WAIT_FOR_MOVEMENT,		(float)0				},
	{ TASK_CLEAR_LASTPOSITION,		(float)0				},
};

Schedule_t slSquidEat[] =
{
	{
		tlSquidEat,
		ARRAYSIZE( tlSquidEat ),
		bits_COND_LIGHT_DAMAGE	|
		bits_COND_HEAVY_DAMAGE	|
		bits_COND_NEW_ENEMY	,
		
		// even though HEAR_SOUND/SMELL FOOD doesn't break this schedule, we need this mask
		// here or the monster won't detect these sounds at ALL while running this schedule.
		bits_SOUND_MEAT			|
		bits_SOUND_CARCASS,
		"SquidEat"
	}
};

// this is a bit different than just Eat. We use this schedule when the food is far away, occluded, or behind
// the squid. This schedule plays a sniff animation before going to the source of food.
Task_t tlSquidSniffAndEat[] =
{
	{ TASK_STOP_MOVING,				(float)0				},
	{ TASK_EAT,						(float)10				},// this is in case the squid can't get to the food
	{ TASK_PLAY_SEQUENCE,			(float)ACT_DETECT_SCENT },
	{ TASK_STORE_LASTPOSITION,		(float)0				},
	{ TASK_GET_PATH_TO_BESTSCENT,	(float)0				},
	{ TASK_WALK_PATH,				(float)0				},
	{ TASK_WAIT_FOR_MOVEMENT,		(float)0				},
	{ TASK_PLAY_SEQUENCE,			(float)ACT_EAT			},
	{ TASK_PLAY_SEQUENCE,			(float)ACT_EAT			},
	{ TASK_PLAY_SEQUENCE,			(float)ACT_EAT			},
	{ TASK_EAT,						(float)50				},
	{ TASK_GET_PATH_TO_LASTPOSITION,(float)0				},
	{ TASK_WALK_PATH,				(float)0				},
	{ TASK_WAIT_FOR_MOVEMENT,		(float)0				},
	{ TASK_CLEAR_LASTPOSITION,		(float)0				},
};

Schedule_t slSquidSniffAndEat[] =
{
	{
		tlSquidSniffAndEat,
		ARRAYSIZE( tlSquidSniffAndEat ),
		bits_COND_LIGHT_DAMAGE	|
		bits_COND_HEAVY_DAMAGE	|
		bits_COND_NEW_ENEMY	,
		
		// even though HEAR_SOUND/SMELL FOOD doesn't break this schedule, we need this mask
		// here or the monster won't detect these sounds at ALL while running this schedule.
		bits_SOUND_MEAT			|
		bits_SOUND_CARCASS,
		"SquidSniffAndEat"
	}
};

// squid does this to stinky things. 
Task_t tlSquidWallow[] =
{
	{ TASK_STOP_MOVING,				(float)0				},
	{ TASK_EAT,						(float)10				},// this is in case the squid can't get to the stinkiness
	{ TASK_STORE_LASTPOSITION,		(float)0				},
	{ TASK_GET_PATH_TO_BESTSCENT,	(float)0				},
	{ TASK_WALK_PATH,				(float)0				},
	{ TASK_WAIT_FOR_MOVEMENT,		(float)0				},
	{ TASK_PLAY_SEQUENCE,			(float)ACT_INSPECT_FLOOR},
	{ TASK_EAT,						(float)50				},// keeps squid from eating or sniffing anything else for a while.
	{ TASK_GET_PATH_TO_LASTPOSITION,(float)0				},
	{ TASK_WALK_PATH,				(float)0				},
	{ TASK_WAIT_FOR_MOVEMENT,		(float)0				},
	{ TASK_CLEAR_LASTPOSITION,		(float)0				},
};

Schedule_t slSquidWallow[] =
{
	{
		tlSquidWallow,
		ARRAYSIZE( tlSquidWallow ),
		bits_COND_LIGHT_DAMAGE	|
		bits_COND_HEAVY_DAMAGE	|
		bits_COND_NEW_ENEMY	,
		
		// even though HEAR_SOUND/SMELL FOOD doesn't break this schedule, we need this mask
		// here or the monster won't detect these sounds at ALL while running this schedule.
		bits_SOUND_GARBAGE,

		"SquidWallow"
	}
};

DEFINE_CUSTOM_SCHEDULES( CBullsquid ) 
{
	slSquidRangeAttack1,
	slSquidChaseEnemy,
	slSquidChaseEnemy2,
	slSquidHurtHop,
	slSquidSeeCrab,
	slSquidEat,
	slSquidSniffAndEat,
	slSquidWallow
};

IMPLEMENT_CUSTOM_SCHEDULES( CBullsquid, CBaseMonster );

//=========================================================
// GetSchedule 
//=========================================================
Schedule_t *CBullsquid :: GetSchedule( void )
{
	switch	( m_MonsterState )
	{
	case MONSTERSTATE_ALERT:
		{
			if ( HasConditions(bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE) )
			{
				return GetScheduleOfType ( SCHED_SQUID_HURTHOP );
			}

			if ( HasConditions(bits_COND_SMELL_FOOD) )
			{
				CSound		*pSound;

				pSound = PBestScent();
				
				if ( pSound && (!FInViewCone ( &pSound->m_vecOrigin ) || !FVisible ( pSound->m_vecOrigin )) )
				{
					// scent is behind or occluded
					return GetScheduleOfType( SCHED_SQUID_SNIFF_AND_EAT );
				}

				// food is right out in the open. Just go get it.
				return GetScheduleOfType( SCHED_SQUID_EAT );
			}

			if ( HasConditions(bits_COND_SMELL) )
			{
				// there's something stinky. 
				CSound		*pSound;

				pSound = PBestScent();
				if ( pSound )
					return GetScheduleOfType( SCHED_SQUID_WALLOW);
			}

			break;
		}
	case MONSTERSTATE_COMBAT:
		{
// dead enemy
			if ( HasConditions( bits_COND_ENEMY_DEAD ) )
			{
				// call base class, all code to handle dead enemies is centralized there.
				return CBaseMonster :: GetSchedule();
			}

			if ( HasConditions(bits_COND_NEW_ENEMY) )
			{
				return GetScheduleOfType ( SCHED_WAKE_ANGRY );
			}

			if ( HasConditions( bits_COND_CAN_RANGE_ATTACK1 ) )
			{
				return GetScheduleOfType ( SCHED_RANGE_ATTACK1 );
			}

			if ( HasConditions( bits_COND_CAN_MELEE_ATTACK1 ) )
			{
				return GetScheduleOfType ( SCHED_MELEE_ATTACK1 );
			}

			if ( HasConditions( bits_COND_CAN_MELEE_ATTACK2 ) )
			{
				return GetScheduleOfType ( SCHED_MELEE_ATTACK2 );
			}
			
			return GetScheduleOfType ( SCHED_CHASE_ENEMY );

			break;
		}
	}

	return CBaseMonster :: GetSchedule();
}

//=========================================================
// GetScheduleOfType
//=========================================================
Schedule_t* CBullsquid :: GetScheduleOfType ( int Type ) 
{
	switch	( Type )
	{
	case SCHED_RANGE_ATTACK1:
		return &slSquidRangeAttack1[ 0 ];
		break;
	case SCHED_SQUID_HURTHOP:
		return &slSquidHurtHop[ 0 ];
		break;
	case SCHED_SQUID_SEECRAB:
		return &slSquidSeeCrab[ 0 ];
		break;
	case SCHED_SQUID_EAT:
		return &slSquidEat[ 0 ];
		break;
	case SCHED_SQUID_SNIFF_AND_EAT:
		return &slSquidSniffAndEat[ 0 ];
		break;
	case SCHED_SQUID_WALLOW:
		return &slSquidWallow[ 0 ];
		break;
	case SCHED_CHASE_ENEMY:
		if ( FClassnameIs(pev, "monster_bullchicken_big") ){
		return &slSquidChaseEnemy2[ 0 ];
		}
		else{
		return &slSquidChaseEnemy[ 0 ];
		}
		break;
	}

	return CBaseMonster :: GetScheduleOfType ( Type );
}

//=========================================================
// Start task - selects the correct activity and performs
// any necessary calculations to start the next task on the
// schedule.  OVERRIDDEN for bullsquid because it needs to
// know explicitly when the last attempt to chase the enemy
// failed, since that impacts its attack choices.
//=========================================================
void CBullsquid :: StartTask ( Task_t *pTask )
{
	m_iTaskStatus = TASKSTATUS_RUNNING;

	switch ( pTask->iTask )
	{
	case TASK_MELEE_ATTACK2:
		{
			switch ( RANDOM_LONG ( 0, 2 ) )
			{
			case 0:	
				EMIT_SOUND( ENT(pev), CHAN_VOICE, "bullchicken/bc_attackgrowl.wav", 1, ATTN_NORM );		
				break;
			case 1:	
				EMIT_SOUND( ENT(pev), CHAN_VOICE, "bullchicken/bc_attackgrowl2.wav", 1, ATTN_NORM );	
				break;
			case 2:	
				EMIT_SOUND( ENT(pev), CHAN_VOICE, "bullchicken/bc_attackgrowl3.wav", 1, ATTN_NORM );	
				break;
			}

			CBaseMonster :: StartTask ( pTask );
			break;
		}
	case TASK_SQUID_HOPTURN:
		{
			SetActivity ( ACT_HOP );
			MakeIdealYaw ( m_vecEnemyLKP );
			break;
		}
	case TASK_GET_PATH_TO_ENEMY:
		{
			if ( m_hEnemy != NULL ){//m_hEnemy�޸�! ������ܵ�������������!
				if ( BuildRoute ( m_hEnemy->pev->origin, bits_MF_TO_ENEMY, m_hEnemy ) )
				{
					m_iTaskStatus = TASKSTATUS_COMPLETE;
				}
				else
				{
					ALERT ( at_aiconsole, "GetPathToEnemy failed!!\n" );
					TaskFail();
				}
			}
			else
			{
				ALERT ( at_aiconsole, "GetPathToEnemy failed!!\n" );
				TaskFail();
			}
			break;
		}
	default:
		{
			CBaseMonster :: StartTask ( pTask );
			break;
		}
	}
}

//=========================================================
// RunTask
//=========================================================
void CBullsquid :: RunTask ( Task_t *pTask )
{
	switch ( pTask->iTask )
	{
	case TASK_SQUID_HOPTURN:
		{
			MakeIdealYaw( m_vecEnemyLKP );
			ChangeYaw( pev->yaw_speed );

			if ( m_fSequenceFinished )
			{
				m_iTaskStatus = TASKSTATUS_COMPLETE;
			}
			break;
		}
	default:
		{
			CBaseMonster :: RunTask( pTask );
			break;
		}
	}
}


//=========================================================
// GetIdealState - Overridden for Bullsquid to deal with
// the feature that makes it lose interest in headcrabs for 
// a while if something injures it. 
//=========================================================
MONSTERSTATE CBullsquid :: GetIdealState ( void )
{
	int	iConditions;

	iConditions = IScheduleFlags();
	
	// If no schedule conditions, the new ideal state is probably the reason we're in here.
	switch ( m_MonsterState )
	{
	case MONSTERSTATE_COMBAT:
		/*
		COMBAT goes to ALERT upon death of enemy
		*/
		{
			if ( m_hEnemy != NULL && ( iConditions & bits_COND_LIGHT_DAMAGE || iConditions & bits_COND_HEAVY_DAMAGE ) && FClassnameIs( m_hEnemy->pev, "monster_headcrab" ) )
			{
				// if the squid has a headcrab enemy and something hurts it, it's going to forget about the crab for a while.
				m_hEnemy = NULL;
				m_IdealMonsterState = MONSTERSTATE_ALERT;
			}
			break;
		}
	}

	m_IdealMonsterState = CBaseMonster :: GetIdealState();

	return m_IdealMonsterState;
}

