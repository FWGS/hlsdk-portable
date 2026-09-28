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

class CRevenant : public CBaseMonster
{
public:
	void Spawn( void );
	void Precache( void );
	void SetYawSpeed( void );
	int  Classify ( void );
	void HandleAnimEvent( MonsterEvent_t *pEvent );

	float m_flNextSkill;
	float m_flNextPainTime;
	float m_flKillBeamTime;

	void AlertSound( void );
	void PainSound( void );
	void DeathSound( void );

	void Killed( entvars_t *pevAttacker, int iGib );

	Schedule_t *GetScheduleOfType ( int Type );
	Schedule_t *GetSchedule ( void );

	void TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType);

	void SetActivity ( Activity NewActivity );

	void RunAI( void );

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	// No range attacks
	BOOL CheckRangeAttack1 ( float flDot, float flDist ) { return FALSE; }
	BOOL CheckRangeAttack2 ( float flDot, float flDist ) { return FALSE; }
	BOOL CheckMeleeAttack1 ( float flDot, float flDist );
	BOOL CheckMeleeAttack2 ( float flDot, float flDist );
	int TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType );

};

LINK_ENTITY_TO_CLASS( monster_revenant, CRevenant );

TYPEDESCRIPTION	CRevenant::m_SaveData[] = 
{
	DEFINE_FIELD( CRevenant, m_flNextSkill, FIELD_TIME ),
	DEFINE_FIELD( CRevenant, m_flNextPainTime, FIELD_TIME ),
	DEFINE_FIELD( CRevenant, m_flKillBeamTime, FIELD_TIME ),
};

IMPLEMENT_SAVERESTORE( CRevenant, CBaseMonster );

void CRevenant :: DeathSound ( void )
{
	EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "cof/taller_die.wav", 1, 0.6, 0, 100);
}

void CRevenant :: AlertSound ( void )
{
	EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "cof/taller_alert.wav", 1, 0.6, 0, 100);
}

void CRevenant :: PainSound ( void )
{
	if ( m_flNextPainTime > gpGlobals->time )
	{
		return;
	}

	m_flNextPainTime = gpGlobals->time + 1.5;

	EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "cof/taller_pain.wav", 1, 0.6, 0, 100);
}


void CRevenant :: RunAI( void )
{
	CBaseMonster :: RunAI();

	if ( pev->movetype == MOVETYPE_TOSS && !m_groundElev )
	{
		if (pev->flags & FL_ONGROUND)
		{
			pev->movetype = MOVETYPE_STEP;
		}
	}

	if(m_flKillBeamTime > 0 && m_flKillBeamTime < gpGlobals->time){
	MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
	WRITE_BYTE( TE_KILLBEAM );
	WRITE_SHORT( entindex() + 0x1000 * 2 );		// entity, attachment
	MESSAGE_END();
	m_flKillBeamTime = 0;
	}

	if(pev->weapons == 1){//黑水模式，未发现敌人前隐身，不被突袭
		m_flDistLook = 768.0;
		m_flFieldOfView = -1;
		m_singdelay_max = 0;
		m_singdelay_use = m_singdelay_max;
		pev->weapons = 2;
		pev->takedamage = DAMAGE_NO;
		pev->effects |= EF_NODRAW;
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
		Killed( pev, GIB_ALWAYS );//碎尸!
		return;
		}
	}

	
	//if(pev->sequence == LookupActivity ( ACT_WALK )){
	//m_flGroundSpeed = 120;
	//}
}

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int	CRevenant :: Classify ( void )
{
	return	CLASS_HUMAN_ASS;
}

BOOL CRevenant :: CheckMeleeAttack1 ( float flDot, float flDist )
{
	float dist = 96;

	if (flDist <= dist && m_hEnemy != NULL && flDot >= 0.5)
	{
		return TRUE;
	}

	return FALSE;
}

BOOL CRevenant :: CheckMeleeAttack2 ( float flDot, float flDist )
{
	float dist = 192;

	if(m_hEnemy != NULL){
		if ( fabs( pev->origin.z - m_hEnemy->pev->origin.z ) > 72 )
		{
			if (m_hEnemy->pev->flags & FL_ONGROUND){
			dist += 64;
			}
		}
	}

	if (flDist <= dist && m_hEnemy != NULL && m_flNextSkill <= gpGlobals->time)
	{
		m_facing_fucking_mode = 1;
		return TRUE;
	}

	m_facing_fucking_mode = 0;
	return FALSE;
}

void CRevenant::TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType)
{
	if ( ptr->iHitgroup == 1){
	flDamage *= 0.75;//头部超硬化，伤害只有1.5倍
	}

	if ( ptr->iHitgroup == 10)
	{
		flDamage -= 60;//防御力很高的护甲

		if ( pev->dmgtime != gpGlobals->time || (RANDOM_LONG(0,100) < 20))
		{
			pev->dmgtime = gpGlobals->time;

			if (RANDOM_LONG(0, 1))
				EMIT_SOUND(ENT(pev), CHAN_BODY, "weapons/ric_metal-1.wav", 1, ATTN_NORM);
			else
				EMIT_SOUND(ENT(pev), CHAN_BODY, "weapons/ric_metal-2.wav", 1, ATTN_NORM);

			UTIL_Sparks(ptr->vecEndPos);
		}

		if(flDamage < 1){
		return;
		}
	}

	if ( ptr->iHitgroup == 8)
	{
		if(pev->skin == 1 && pev->body == 0 && pev->deadflag != DEAD_NO){//眼睛打开
			Vector vecGunPos,vecGunAngles;
			GetAttachment( 0, vecGunPos, vecGunAngles );
			FX_Explosion( vecGunPos, 236 );
			SpawnBlood(vecGunPos, BloodColor(), 100);
			EMIT_SOUND(ENT(pev), CHAN_BODY, "cof/bodysplat.wav", 1, ATTN_NORM);
			pev->body = 1;
			UTIL_SetSize ( pev, Vector ( pev->mins.x, pev->mins.y, pev->mins.z ), Vector ( pev->maxs.x, pev->maxs.y, pev->mins.z + 4 ) );
		}
	}

	CBaseMonster::TraceAttack( pevAttacker, flDamage, vecDir, ptr, bitsDamageType );
}

Schedule_t* CRevenant :: GetScheduleOfType ( int Type )
{
	return CBaseMonster::GetScheduleOfType( Type );
}

//=========================================================
// GetSchedule - Decides which type of schedule best suits
// the monster's current state and conditions. Then calls
// monster's member function to get a pointer to a schedule
// of the proper type.
//=========================================================
Schedule_t *CRevenant :: GetSchedule ( void )
{
	return CBaseMonster::GetSchedule();
}
//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CRevenant :: SetYawSpeed ( void )
{
	int ys;

	switch ( m_Activity )
	{
	case ACT_MELEE_ATTACK2:	
		ys = 450;		
		break;
	case ACT_MELEE_ATTACK1:	
		ys = 300;	
		break;
	default:
		ys = 150;
		break;
	}

	pev->yaw_speed = ys;
}

int CRevenant :: TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	return CBaseMonster::TakeDamage( pevInflictor, pevAttacker, flDamage, bitsDamageType );
}

void CRevenant::Killed( entvars_t *pevAttacker, int iGib )
{
	if(m_die == 0){
	pev->skin = 1;//开眼
	}
	CBaseMonster::Killed( pevAttacker, iGib );
}

//=========================================================
// SetActivity 
//=========================================================
void CRevenant :: SetActivity ( Activity NewActivity )
{
	int	iSequence = ACTIVITY_NOT_AVAILABLE;
	void *pmodel = GET_MODEL_PTR( ENT(pev) );

	switch ( NewActivity)
	{
	case ACT_RUN:
		iSequence = LookupActivity ( NewActivity );
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
void CRevenant :: HandleAnimEvent( MonsterEvent_t *pEvent )
{
	int dmg1,dmg2,dmg3;

	dmg1 = 90;
	dmg2 = 60;
	dmg3 = 180;

	switch( pEvent->event )
	{
		case 1:
		{
			if(m_hEnemy != NULL){
				if(( pev->origin - m_hEnemy->pev->origin).Length() <= 120){
					TraceResult tr;
					UTIL_MakeVectors(pev->angles);
					Vector vecSrc	= BodyTarget_o(pev->origin);
					Vector vecEnd	= m_hEnemy->Center();
					UTIL_TraceLine( vecSrc, vecEnd, dont_ignore_monsters, ENT( pev ), &tr );
					CBaseEntity *pEntity = CBaseEntity::Instance(tr.pHit);

					if ( tr.flFraction < 1.0 && pEntity->pev->takedamage ){
					int tex = (int)TEXTURETYPE_Trace(&tr, vecSrc, vecEnd);
					int surface = (int)SURFACETYPE_Trace(&tr, vecSrc, vecEnd,Classify(),1);
					FX_ImpBullet( tr.vecEndPos, tr.vecPlaneNormal, vecSrc, surface, BULLET_CROWBAR, (float)tex );

					ClearMultiDamage( );
					pEntity->TraceAttack(pev, dmg1, gpGlobals->v_forward, &tr, DMG_SLASH); 
					ApplyMultiDamage( pev, pev );
					EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, "weapons/knife_hitbody.wav", 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
					}
				}
			}
		}
		break;

		case 2:
		{
			if(m_hEnemy != NULL){
				if(( pev->origin - m_hEnemy->pev->origin).Length() <= 150){
					TraceResult tr;
					UTIL_MakeVectors(pev->angles);
					Vector vecSrc	= BodyTarget_o(pev->origin);
					Vector vecEnd	= m_hEnemy->Center();
					UTIL_TraceLine( vecSrc, vecEnd, dont_ignore_monsters, ENT( pev ), &tr );
					CBaseEntity *pEntity = CBaseEntity::Instance(tr.pHit);

					if ( tr.flFraction < 1.0 && pEntity->pev->takedamage ){
					int tex = (int)TEXTURETYPE_Trace(&tr, vecSrc, vecEnd);
					int surface = (int)SURFACETYPE_Trace(&tr, vecSrc, vecEnd,Classify(),1);
					FX_ImpBullet( tr.vecEndPos, tr.vecPlaneNormal, vecSrc, surface, BULLET_CROWBAR, (float)tex );

					ClearMultiDamage( );
					pEntity->TraceAttack(pev, dmg2, gpGlobals->v_forward, &tr, DMG_SLASH); 
					ApplyMultiDamage( pev, pev );
					EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, "weapons/knife_hitbody.wav", 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
					}
				}
			}
		}
		break;

		case 3:
		{
			EMIT_SOUND_DYN ( ENT(pev), CHAN_BODY, "cof/taller_step.wav", 1, ATTN_NORM, 0, 100 );
		}
		break;

		case 4:
		{
			if(pev->body == 1 || m_killed_exp <= 2){//死透了!
				if(pev->body == 0){
				Vector vecGunPos,vecGunAngles;
				GetAttachment( 0, vecGunPos, vecGunAngles );
				FX_Explosion( vecGunPos, 236 );
				SpawnBlood(vecGunPos, BloodColor(), 100);
				EMIT_SOUND(ENT(pev), CHAN_BODY, "cof/bodysplat.wav", 1, ATTN_NORM);
				pev->body = 1;
				}
				pev->deadflag = DEAD_DEAD;
				m_dieseq = 1;
				UTIL_SetSize ( pev, Vector ( pev->mins.x, pev->mins.y, pev->mins.z ), Vector ( pev->maxs.x, pev->maxs.y, pev->mins.z + 4 ) );
				StopAnimation();

				if ( ShouldFadeOnDeath() )
				{
					SetThink ( NULL );
					SetTouch ( NULL );

					pev->velocity = g_vecZero;
					pev->movetype = MOVETYPE_NONE;
					pev->solid = SOLID_NOT;
					pev->avelocity = g_vecZero;
					// this monster was created by a monstermaker... fade the corpse out.
					SUB_StartFadeOut();
				}
			}
			else{
			pev->skin = 0;//眼球关闭!
			}
		}
		break;

		case 5:
		{//最多可复活两次
				if(m_killed_exp > 10){
				m_killed_exp = 10;
				}
				else{
				m_killed_exp = 1;
				}
				pev->skin = 0;
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
		}
		break;

		case 6:
		{
			pev->movetype = MOVETYPE_TOSS;
			pev->flags &= ~FL_ONGROUND;
			UTIL_MakeVectors(pev->angles);
			if(m_hEnemy != NULL){
				if( (m_hEnemy->pev->origin.z - pev->origin.z) >= 120 
				&& (m_hEnemy->pev->flags & FL_ONGROUND) ){
				pev->velocity = gpGlobals->v_forward * 200;
				pev->velocity.z += 400;
				}
				else{
				pev->velocity = gpGlobals->v_forward * 400;
				pev->velocity.z += 200;
				}
			}
			else{
				pev->velocity = gpGlobals->v_forward * 400;
				pev->velocity.z += 200;
			}
			m_flNextSkill = gpGlobals->time + 6.0;
		}
		break;

		case 7:
		{
			if(m_hEnemy != NULL){
					TraceResult tr;
					UTIL_MakeVectors(pev->angles);
					Vector vecSrc	= BodyTarget_e(pev->origin);
					Vector vecEnd	= m_hEnemy->Center();
					UTIL_TraceLine( vecSrc, vecEnd, dont_ignore_monsters, ENT( pev ), &tr );
					CBaseEntity *pEntity = CBaseEntity::Instance(tr.pHit);

					if(( vecSrc - tr.vecEndPos).Length() <= 190){
						if ( tr.flFraction < 1.0 && pEntity->pev->takedamage 
						&& pEntity->Classify() != Classify()){
						int tex = (int)TEXTURETYPE_Trace(&tr, vecSrc, vecEnd);
						int surface = (int)SURFACETYPE_Trace(&tr, vecSrc, vecEnd,Classify(),1);
						FX_ImpBullet( tr.vecEndPos, tr.vecPlaneNormal, vecSrc, surface, BULLET_CROWBAR, (float)tex );

						ClearMultiDamage( );
						pEntity->TraceAttack(pev, dmg3, gpGlobals->v_forward, &tr, DMG_SLASH); 
						ApplyMultiDamage( pev, pev );
						EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, "newadd/zom_headburst.wav", 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
						}
					}
			}
		}
		break;

		case 8://拖尾
		{
			MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
			WRITE_BYTE( TE_BEAMFOLLOW );
			WRITE_SHORT( entindex() + 0x1000 * 2 );		// entity, attachment
			WRITE_SHORT(g_sModelIndexTrail);	// model
			WRITE_BYTE( 6 ); // life
			WRITE_BYTE( 8 );  // width
			WRITE_BYTE( 255 );	// R
			WRITE_BYTE( 255 );	// G
			WRITE_BYTE( 255 );	// B
			WRITE_BYTE( 192 );	// brightness
			MESSAGE_END();  // move PHS/PVS data sending into here (SEND_ALL, SEND_PVS, SEND_PHS)

			m_flKillBeamTime = gpGlobals->time + 1.5;
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
void CRevenant :: Spawn()
{
	Precache( );
	SET_MODEL(ENT(pev), "models/revenant.mdl");
	UTIL_SetSize(pev, Vector(-32,-32,0), Vector(32,32,160));

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_TOSS;
	m_bloodColor		= BLOOD_COLOR_RED;

	if (g_iSkillLevel == SKILL_HARD){
	pev->health			= 1080;
	}
	else{
	pev->health			= 900;
	}

	pev->view_ofs		= VEC_VIEW;// position of the eyes relative to monster's origin.
	m_flFieldOfView		= 0;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState		= MONSTERSTATE_NONE;
	m_afCapability		= bits_CAP_DOORS_GROUP;
	
	MonsterInit();

	pev->gravity		= 1.6;

	m_ignoredamage = 1;
	m_longming = 1;

	m_selfmode = TRUE;

	m_ignoreFail_MAX = 40;
	m_ignoreFail_OFF = 0;
	m_MoveFail_FuckRoad = TRUE;
	m_MoveFail_SimpleRoad = TRUE;

	m_killed_exp = 180;
	m_rpgms_level = 60;
	m_headdef = 2;//坚硬头部

	pev->netname = MAKE_STRING( "Revenant.Taller" );
}
//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CRevenant :: Precache()
{
	PRECACHE_MODEL("models/revenant.mdl");
	PRECACHE_SOUND("cof/taller_alert.wav");
	PRECACHE_SOUND("cof/taller_pain.wav");
	PRECACHE_SOUND("cof/taller_die.wav");
	PRECACHE_SOUND("cof/taller_step.wav");
	PRECACHE_SOUND("cof/bodysplat.wav");
	
}	
