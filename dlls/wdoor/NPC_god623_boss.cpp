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
// Alien slave monster
//=========================================================

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"squadmonster.h"
#include	"schedule.h"
#include	"effects.h"
#include	"weapons.h"
#include	"soundent.h"
#include	"decals.h"

class CGodBOSSHolyBall : public CBaseEntity
{
public:
	void Spawn( void );

	static void Shoot( entvars_t *pevOwner, Vector vecStart, Vector vecVelocity,int type );
	void Touch( CBaseEntity *pOther );
};

LINK_ENTITY_TO_CLASS( god_holy_ball, CGodBOSSHolyBall );

void CGodBOSSHolyBall:: Spawn( void )
{
	pev->movetype = MOVETYPE_FLY;
	pev->classname = MAKE_STRING( "god_holy_ball" );
	
	pev->solid = SOLID_BBOX;

	SET_MODEL(ENT(pev), "models/camera_rocket.mdl");
	pev->frame = 0;

	UTIL_SetSize( pev, Vector( 0, 0, 0), Vector(0, 0, 0) );

	MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
	WRITE_BYTE( TE_BEAMFOLLOW );
	WRITE_SHORT( entindex() );		// entity, attachment
	WRITE_SHORT(g_sModelIndexTrail);	// model
	WRITE_BYTE( 6 ); // life
	WRITE_BYTE( 8 );  // width
	WRITE_BYTE( 255 );	// R
	WRITE_BYTE( 255 );	// G
	WRITE_BYTE( 255 );	// B
	WRITE_BYTE( 192 );	// brightness
	MESSAGE_END();  // move PHS/PVS data sending into here (SEND_ALL, SEND_PVS, SEND_PHS)
}

void CGodBOSSHolyBall::Shoot( entvars_t *pevOwner, Vector vecStart, Vector vecVelocity,int type )
{
	CGodBOSSHolyBall *pSpit = GetClassPtr( (CGodBOSSHolyBall *)NULL );
	pSpit->Spawn();
	
	UTIL_SetOrigin( pSpit->pev, vecStart );
	pSpit->pev->velocity = vecVelocity;
	pSpit->pev->owner = ENT(pevOwner);
	pSpit->pev->angles = UTIL_VecToAngles (pSpit->pev->velocity);

	if(type == 2){//��
		pSpit->pev->frags = 2;
		FX_Trail(pSpit->pev->origin, pSpit->entindex(), 155);
	}
	else{//��
		pSpit->pev->frags = 1;
		FX_Trail(pSpit->pev->origin, pSpit->entindex(), 154);
	}

	pSpit->pev->dmg = 100;
}

void CGodBOSSHolyBall :: Touch ( CBaseEntity *pOther )
{
	entvars_t *pevOwner = VARS( pev->owner );

	FX_Trail( pev->origin, entindex(), PROJ_REMOVE );

	if(pev->frags == 1){
		if(pOther->pev->takedamage){
		pOther->TakeDamage ( pev, pevOwner, 32, DMG_BURN );
		}

		::RadiusDamage_limit( pev->origin, pev, pevOwner, 64, 160, CLASS_HUMAN_ASS, DMG_BURN );

		FX_Trail( pev->origin, entindex(), PROJ_FLAME_DETONATE );
	}
	else{
		if(pOther->pev->takedamage){
		pOther->TakeDamage ( pev, pevOwner, 32, DMG_FREEZE );
		}

		::RadiusDamage_limit( pev->origin, pev, pevOwner, 64, 160, CLASS_HUMAN_ASS, DMG_FREEZE );

		EMIT_SOUND(ENT(pev), CHAN_WEAPON, "misaliya/frostnova.wav", VOL_NORM, ATTN_NORM); 
		FX_Trail( pev->origin, entindex(), PROJ_ICE_DETONATE );
	}

		SetThink ( &CGodBOSSHolyBall::SUB_Remove );
		pev->nextthink = gpGlobals->time;
}


//=========================================================
// God Holy Sword
//=========================================================
class CGodBOSSHolySword : public CBaseEntity
{
public:
	void Spawn( void );

	void EXPORT FlyThink( void );
};

LINK_ENTITY_TO_CLASS( god_holy_sword, CGodBOSSHolySword );

void CGodBOSSHolySword:: Spawn( void )
{
	pev->movetype = MOVETYPE_NOCLIP;
	pev->classname = MAKE_STRING( "god_holy_sword" );
	
	pev->solid = SOLID_NOT;

	SET_MODEL(ENT(pev), "models/holysword_skill.mdl");
	pev->sequence	= 1;
	pev->frame		= 0;
	pev->framerate = 1.0;

	UTIL_SetSize( pev, Vector( -32, -32, -64), Vector(32, 32, 384) );

	pev->effects		= EF_DIMLIGHT;

	SetThink( &CGodBOSSHolySword::FlyThink );
	pev->nextthink = gpGlobals->time + 0.1;

	pev->velocity.z = 1500;
	pev->angles.x = 180;

						MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
						WRITE_BYTE( TE_BEAMFOLLOW );
						WRITE_SHORT( entindex() + 0x1000 * 1 );		// entity, attachment
						WRITE_SHORT(g_sModelIndexTrail);	// model
						WRITE_BYTE( 30 ); // life
						WRITE_BYTE( 15 );  // width
						WRITE_BYTE( 255 );	// R
						WRITE_BYTE( 255 );	// G
						WRITE_BYTE( 255 );	// B
						WRITE_BYTE( 192 );	// brightness
						MESSAGE_END();  // move PHS/PVS data sending into here (SEND_ALL, SEND_PVS, SEND_PHS)

						MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
						WRITE_BYTE( TE_BEAMFOLLOW );
						WRITE_SHORT( entindex() + 0x1000 * 2 );		// entity, attachment
						WRITE_SHORT(g_sModelIndexTrail);	// model
						WRITE_BYTE( 30 ); // life
						WRITE_BYTE( 15 );  // width
						WRITE_BYTE( 255 );	// R
						WRITE_BYTE( 255 );	// G
						WRITE_BYTE( 255 );	// B
						WRITE_BYTE( 192 );	// brightness
						MESSAGE_END();  // move PHS/PVS data sending into here (SEND_ALL, SEND_PVS, SEND_PHS)

						MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
						WRITE_BYTE( TE_BEAMFOLLOW );
						WRITE_SHORT( entindex() + 0x1000 * 3 );		// entity, attachment
						WRITE_SHORT(g_sModelIndexTrail);	// model
						WRITE_BYTE( 30 ); // life
						WRITE_BYTE( 15 );  // width
						WRITE_BYTE( 255 );	// R
						WRITE_BYTE( 255 );	// G
						WRITE_BYTE( 255 );	// B
						WRITE_BYTE( 192 );	// brightness
						MESSAGE_END();  // move PHS/PVS data sending into here (SEND_ALL, SEND_PVS, SEND_PHS)

						MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
						WRITE_BYTE( TE_BEAMFOLLOW );
						WRITE_SHORT( entindex() + 0x1000 * 4 );		// entity, attachment
						WRITE_SHORT(g_sModelIndexTrail);	// model
						WRITE_BYTE( 30 ); // life
						WRITE_BYTE( 15 );  // width
						WRITE_BYTE( 255 );	// R
						WRITE_BYTE( 255 );	// G
						WRITE_BYTE( 255 );	// B
						WRITE_BYTE( 192 );	// brightness
						MESSAGE_END();  // move PHS/PVS data sending into here (SEND_ALL, SEND_PVS, SEND_PHS)
}

void CGodBOSSHolySword::FlyThink( void )
{
	if(pev->frags < 270){
		pev->nextthink = gpGlobals->time + 0.1;
		pev->frags += 1;
	}
	else{
		MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
		WRITE_BYTE( TE_KILLBEAM );
		WRITE_SHORT( entindex() + 0x1000 * 1 );		// entity, attachment
		MESSAGE_END();

		MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
		WRITE_BYTE( TE_KILLBEAM );
		WRITE_SHORT( entindex() + 0x1000 * 2 );		// entity, attachment
		MESSAGE_END();

		MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
		WRITE_BYTE( TE_KILLBEAM );
		WRITE_SHORT( entindex() + 0x1000 * 3 );		// entity, attachment
		MESSAGE_END();

		MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
		WRITE_BYTE( TE_KILLBEAM );
		WRITE_SHORT( entindex() + 0x1000 * 4 );		// entity, attachment
		MESSAGE_END();

		SUB_StartFadeOut3();

		return;
	}

	if(pev->frags >= 30 && pev->frags <= 200){
		entvars_t *pevOwner;
		if ( pev->owner )
			pevOwner = VARS( pev->owner );
		else
			pevOwner = NULL;

		if(pev->frags <= 35){//���궨λ��
			if ( pevOwner->flags & FL_MONSTER ){
				CBaseMonster *pEnemyMonster;
				pEnemyMonster = Instance( pev->owner )->MyMonsterPointer();
				if(pEnemyMonster){
						pev->origin.x = pEnemyMonster->m_vecEnemyLKP.x;
						pev->origin.y = pEnemyMonster->m_vecEnemyLKP.y;

						if(pev->frags == 35){//���դ�����
						pev->origin.x += RANDOM_FLOAT( -128, 128 );
						pev->origin.y += RANDOM_FLOAT( -128, 128 );
						pev->origin.z = pEnemyMonster->m_vecEnemyLKP.z + 3000;
						UTIL_SetOrigin( pev, pev->origin );
						pev->angles.x = 0;
						}
				}
			}
		}
		else if(pev->frags == 36){
						MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
						WRITE_BYTE( TE_BEAMFOLLOW );
						WRITE_SHORT( entindex() + 0x1000 * 1 );		// entity, attachment
						WRITE_SHORT(g_sModelIndexTrail);	// model
						WRITE_BYTE( 30 ); // life
						WRITE_BYTE( 15 );  // width
						WRITE_BYTE( 255 );	// R
						WRITE_BYTE( 255 );	// G
						WRITE_BYTE( 255 );	// B
						WRITE_BYTE( 192 );	// brightness
						MESSAGE_END();  // move PHS/PVS data sending into here (SEND_ALL, SEND_PVS, SEND_PHS)

						MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
						WRITE_BYTE( TE_BEAMFOLLOW );
						WRITE_SHORT( entindex() + 0x1000 * 2 );		// entity, attachment
						WRITE_SHORT(g_sModelIndexTrail);	// model
						WRITE_BYTE( 30 ); // life
						WRITE_BYTE( 15 );  // width
						WRITE_BYTE( 255 );	// R
						WRITE_BYTE( 255 );	// G
						WRITE_BYTE( 255 );	// B
						WRITE_BYTE( 192 );	// brightness
						MESSAGE_END();  // move PHS/PVS data sending into here (SEND_ALL, SEND_PVS, SEND_PHS)

						MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
						WRITE_BYTE( TE_BEAMFOLLOW );
						WRITE_SHORT( entindex() + 0x1000 * 3 );		// entity, attachment
						WRITE_SHORT(g_sModelIndexTrail);	// model
						WRITE_BYTE( 30 ); // life
						WRITE_BYTE( 15 );  // width
						WRITE_BYTE( 255 );	// R
						WRITE_BYTE( 255 );	// G
						WRITE_BYTE( 255 );	// B
						WRITE_BYTE( 192 );	// brightness
						MESSAGE_END();  // move PHS/PVS data sending into here (SEND_ALL, SEND_PVS, SEND_PHS)

						MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
						WRITE_BYTE( TE_BEAMFOLLOW );
						WRITE_SHORT( entindex() + 0x1000 * 4 );		// entity, attachment
						WRITE_SHORT(g_sModelIndexTrail);	// model
						WRITE_BYTE( 30 ); // life
						WRITE_BYTE( 15 );  // width
						WRITE_BYTE( 255 );	// R
						WRITE_BYTE( 255 );	// G
						WRITE_BYTE( 255 );	// B
						WRITE_BYTE( 192 );	// brightness
						MESSAGE_END();  // move PHS/PVS data sending into here (SEND_ALL, SEND_PVS, SEND_PHS)

						pev->velocity.z = -2000;
		}

		TraceResult tr;
		Vector vecSpot = pev->origin;
		UTIL_TraceLine( pev->origin + Vector(0,0,384), vecSpot, ignore_monsters, ENT(pev), &tr );

		if ( tr.flFraction < 1.0 ){
			vecSpot = tr.vecEndPos + Vector(0,0,16);

			if (UTIL_PointContents(tr.vecEndPos) == CONTENTS_SKY){
				pev->frags = 270;
				pev->framerate = 0.0;
				pev->velocity = g_vecZero;
			}
			else{
				pev->frags = 200;
				pev->framerate = 0.0;
				pev->velocity = g_vecZero;

				if(pev->body == 2){//��֮��
				FX_Explosion( vecSpot, 152 );
				::RadiusDamage_limit( vecSpot, pev, pevOwner, 192, 384, CLASS_HUMAN_ASS, DMG_BURN | DMG_BLAST | DMG_CONCUSSION);
				}
				else{//��֮��
				FX_Explosion( vecSpot, 153 );
				::RadiusDamage_limit( vecSpot, pev, pevOwner, 192, 384, CLASS_HUMAN_ASS, DMG_FREEZE | DMG_BLAST | DMG_CONCUSSION);
				}

				EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, "rmxp/160-Skill04.wav", 1.0, 0.5, 0, 100 + RANDOM_LONG(-5,5) );

				if ( RANDOM_FLOAT( 0 , 1 ) < 0.5 )
				{
					UTIL_DecalTrace( &tr, DECAL_SCORCH1 );
				}
				else
				{
					UTIL_DecalTrace( &tr, DECAL_SCORCH2 );
				}
			}

		}
	}
}

extern DLL_GLOBAL int		g_iSkillLevel;


class CGodBoss : public CSquadMonster
{
public:
	void Spawn( void );
	void Precache( void );
	void SetYawSpeed( void );
	int  Classify ( void );

	void HandleAnimEvent( MonsterEvent_t *pEvent );
	BOOL FCanCheckAttacks ( void );

	BOOL CheckRangeAttack1 ( float flDot, float flDist );//����
	BOOL CheckRangeAttack2 ( float flDot, float flDist );//��Ȧ
	BOOL CheckMeleeAttack1 ( float flDot, float flDist );//�����
	BOOL CheckMeleeAttack2 ( float flDot, float flDist );//�ٻ���

	void TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType);
	int TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType);

	void PainSound( void );
	void DeathSound( void );

	void RunAI( void );

	void Killed( entvars_t *pevAttacker, int iGib );

	void SetObjectCollisionBox( void )
	{
		pev->absmin = pev->origin + Vector( -160, -160, -480 );
		pev->absmax = pev->origin + Vector( 160, 160, 480 );
	}

	int	Save( CSave &save ); 
	int Restore( CRestore &restore );
	static TYPEDESCRIPTION m_SaveData[];

	Vector m_teleportorigin;
	Vector m_teleportorigin2;

	float m_flNextSkill1Time;
	float m_flNextSkill2Time;
	float m_flNextSkill3Time;
	float m_flNextSkill4Time;
	float m_flKillBeamTime;

	float m_painTime;
	float m_debugseqTime;

	int god_deadball;
	EHANDLE	m_childguy;
};
LINK_ENTITY_TO_CLASS( monster_god623_boss, CGodBoss );
LINK_ENTITY_TO_CLASS( monster_god625_boss, CGodBoss );

TYPEDESCRIPTION	CGodBoss::m_SaveData[] = 
{
	DEFINE_FIELD( CGodBoss, m_teleportorigin, FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( CGodBoss, m_teleportorigin2, FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( CGodBoss, m_painTime, FIELD_TIME ),
	DEFINE_FIELD( CGodBoss, m_debugseqTime, FIELD_TIME ),
	DEFINE_FIELD( CGodBoss, m_flNextSkill1Time, FIELD_TIME ),
	DEFINE_FIELD( CGodBoss, m_flNextSkill2Time, FIELD_TIME ),
	DEFINE_FIELD( CGodBoss, m_flNextSkill3Time, FIELD_TIME ),
	DEFINE_FIELD( CGodBoss, m_flNextSkill4Time, FIELD_TIME ),
	DEFINE_FIELD( CGodBoss, m_flKillBeamTime, FIELD_TIME ),
	DEFINE_FIELD( CGodBoss, m_childguy, FIELD_EHANDLE ),
};

IMPLEMENT_SAVERESTORE( CGodBoss, CSquadMonster );

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int	CGodBoss :: Classify ( void )
{
	return	CLASS_HUMAN_ASS;
}

BOOL CGodBoss :: FCanCheckAttacks ( void )
{
	return TRUE;
}

void CGodBoss :: PainSound ( void )
{
	if (g_iSkillLevel == SKILL_HARD){//Bug Fix 3.0 ��ս��Ѫ
	m_painTime = gpGlobals->time + 24.0;
	}
	else{
	m_painTime = gpGlobals->time + 30.0;
	}
}

void CGodBoss :: RunAI( void )
{
	CBaseMonster :: RunAI();

	if(pev->deadflag != DEAD_NO){
		pev->gravity = 0.1;
		pev->velocity.x *= 0.1;
		pev->velocity.y *= 0.1;
		pev->velocity.z *= 0.1;
	}
	else{
		//�黯��������� Bug Fix 1.0
		if(m_debugseqTime > 0 && m_debugseqTime <= gpGlobals->time && pev->movetype == MOVETYPE_NOCLIP){
			m_debugseqTime = 0;

			SetYawSpeed();
			pev->movetype = MOVETYPE_FLY;
			pev->velocity = g_vecZero;

			if(pev->impulse == 0){//��������
			UTIL_SetOrigin( pev, m_teleportorigin2 );
			pev->impulse = 1;
			}
			else{
			UTIL_SetOrigin( pev, m_teleportorigin );
			pev->impulse = 0;
			}

			pev->renderfx = 0;
			pev->takedamage = DAMAGE_AIM;
			pev->takedamage = SOLID_BBOX;

			if ( RANDOM_LONG(0,1) ){
			pev->velocity.x = 96;
			}
			else{
			pev->velocity.x = -96;
			}

			m_flNextSkill3Time = 0;
		}

		if(pev->movetype == MOVETYPE_FLY && m_freezetime == 0 ){//x����

				if (gpGlobals->time > m_painTime && pev->health < pev->max_health ){//�Զ��ָ�����ֵ!
					TakeHealth( pev->max_health * 0.001, DMG_GENERIC );
					m_painTime = gpGlobals->time + 1.0;
				}

				if( !(pev->flags & FL_FLY) ){
					pev->flags	 |= FL_FLY;
					m_teleportorigin = pev->origin;

					if ( RANDOM_LONG(0,1) ){
					pev->velocity.x = 96;
					}
					else{
					pev->velocity.x = -96;
					}
				}

				if (m_teleportorigin.x - pev->origin.x > 384){
					if(pev->velocity.x < 96){
					pev->velocity.x += 16;
					}
				}
				if (m_teleportorigin.x - pev->origin.x < -384){
					if(pev->velocity.x > -96){
					pev->velocity.x -= 16;
					}
				}
		}
	}
	


	if(m_flKillBeamTime > 0){
		if(pev->movetype == MOVETYPE_NOCLIP){
		Vector sword_org,vecdir;
		GetAttachment( 0, sword_org,vecdir);
		::RadiusDamage_limit( sword_org, pev, pev, 128, 384, CLASS_HUMAN_ASS, DMG_BURN | DMG_CONCUSSION);
		FX_Explosion( sword_org, 47);

		GetAttachment( 1, sword_org,vecdir);
		::RadiusDamage_limit( sword_org, pev, pev, 128, 384, CLASS_HUMAN_ASS, DMG_FREEZE | DMG_CONCUSSION);
		FX_Explosion( sword_org, 51);

		GetAttachment( 2, sword_org,vecdir);
		::RadiusDamage_limit( sword_org, pev, pev, 128, 384, CLASS_HUMAN_ASS, DMG_SLASH | DMG_CONCUSSION);
		FX_Explosion( sword_org, 47);

		GetAttachment( 3, sword_org,vecdir);
		::RadiusDamage_limit( sword_org, pev, pev, 128, 384, CLASS_HUMAN_ASS, DMG_BURN | DMG_CONCUSSION);
		FX_Explosion( sword_org, 51);

		::RadiusDamage_limit( Center(), pev, pev, 128, 384, CLASS_HUMAN_ASS, DMG_SLASH | DMG_CONCUSSION);
		FX_Explosion( Center(), 43);

		EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, "god623/holysword_dash.wav", 1.0, 0.1, 0, 100 + RANDOM_LONG(-5,5) );
		}

		if(m_flKillBeamTime < gpGlobals->time){
		MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
		WRITE_BYTE( TE_KILLBEAM );
		WRITE_SHORT( entindex() + 0x1000 * 1 );		// entity, attachment
		MESSAGE_END();

		MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
		WRITE_BYTE( TE_KILLBEAM );
		WRITE_SHORT( entindex() + 0x1000 * 2 );		// entity, attachment
		MESSAGE_END();

		MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
		WRITE_BYTE( TE_KILLBEAM );
		WRITE_SHORT( entindex() + 0x1000 * 3 );		// entity, attachment
		MESSAGE_END();

		MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
		WRITE_BYTE( TE_KILLBEAM );
		WRITE_SHORT( entindex() + 0x1000 * 4 );		// entity, attachment
		MESSAGE_END();
		m_flKillBeamTime = 0;
		}
	}
}

//=========================================================
// DieSound
//=========================================================
void CGodBoss :: DeathSound( void )
{
	EMIT_SOUND_DYN ( ENT(pev), CHAN_VOICE, "god623/die.wav", 1.0, 0.1, 0, 100);
}


void CGodBoss::Killed( entvars_t *pevAttacker, int iGib )
{
	if(m_childguy != NULL){
		if(m_childguy->pev->deadflag == DEAD_NO){
		m_childguy->Killed( pev, GIB_NEVER );
		}
	}
	CSquadMonster::Killed( pevAttacker, iGib );
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CGodBoss :: SetYawSpeed ( void )
{
	pev->yaw_speed = 180;
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//
// Returns number of events handled, 0 if none.
//=========================================================
void CGodBoss :: HandleAnimEvent( MonsterEvent_t *pEvent )
{
	// ALERT( at_console, "event %d : %f\n", pEvent->event, pev->frame );
	switch( pEvent->event )
	{
		case 1://����
		{
			if(m_flNextSkill1Time < gpGlobals->time){
			Vector sword_org,vecdir;
			GetAttachment( 0, sword_org,vecdir);
			CBaseEntity *SwordEntity = Create( "god_holy_sword", sword_org, pev->angles, edict() );
			SwordEntity->pev->body = 2;

			GetAttachment( 1, sword_org,vecdir);
			SwordEntity = Create( "god_holy_sword", sword_org, pev->angles, edict() );
			SwordEntity->pev->body = 3;
			SwordEntity->pev->frags = -5;

			GetAttachment( 2, sword_org,vecdir);
			SwordEntity = Create( "god_holy_sword", sword_org, pev->angles, edict() );
			SwordEntity->pev->body = 2;
			SwordEntity->pev->frags = -10;

			GetAttachment( 3, sword_org,vecdir);
			SwordEntity = Create( "god_holy_sword", sword_org, pev->angles, edict() );
			SwordEntity->pev->body = 3;
			SwordEntity->pev->frags = -15;

			m_flNextSkill1Time = gpGlobals->time + RANDOM_FLOAT( 28.0, 35.0 );
			SetBodygroup( 5, 1 );

			EMIT_SOUND_DYN ( ENT(pev), CHAN_VOICE, "god623/holysword_up.wav", 1.0, 0.1, 0, 100 + RANDOM_LONG(-5,5) );
			}
		}
		break;

		case 2:
		{
			if(m_flNextSkill2Time <= gpGlobals->time){
			m_flNextSkill2Time = gpGlobals->time + RANDOM_FLOAT( 4.0, 10.0 );
			}

			Vector vecArmPos,vecArmDir,vecSpitDir;
			GetBonePosition( 44, vecArmPos, vecArmDir );
			vecArmPos.z += 128;

			if(m_hEnemy != NULL){
			UTIL_MakeVectors ( pev->angles );

			vecSpitDir = ( m_hEnemy->Center() - vecArmPos).Normalize();

			CGodBOSSHolyBall::Shoot( pev, vecArmPos, vecSpitDir * 2000,1 );
			//EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, "weapons/shock_fire.wav", 0.7, ATTN_NORM,0,100 + RANDOM_LONG(-5,5) );
			}
		}
		break;

		case 3:
		{
			Vector vecArmPos,vecArmDir,vecSpitDir;
			GetBonePosition( 78, vecArmPos, vecArmDir );
			vecArmPos.z += 128;

			if(m_hEnemy != NULL){
			UTIL_MakeVectors ( pev->angles );

			vecSpitDir = ( m_hEnemy->Center() - vecArmPos).Normalize();

			CGodBOSSHolyBall::Shoot( pev, vecArmPos, vecSpitDir * 2000,2 );
			//EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, "weapons/shock_fire.wav", 0.7, ATTN_NORM,0,100 + RANDOM_LONG(-5,5) );
			}
		}
		break;

		case 4:
		{//�黯
			if(m_flNextSkill3Time <= gpGlobals->time){
			pev->renderfx = kRenderFxGlowShell;
			pev->rendercolor.x = 255;
			pev->rendercolor.y = 255;
			pev->rendercolor.z = 255;
			pev->renderamt = 2;

			pev->takedamage = DAMAGE_NO;
			pev->takedamage = SOLID_NOT;

			pev->yaw_speed   = 0;
			pev->movetype = MOVETYPE_NOCLIP;
			pev->velocity.z  = -200;

			m_debugseqTime = gpGlobals->time + 5.0;
			}
		}
		break;

		case 5:
		{//�黯���
			if(m_flNextSkill3Time <= gpGlobals->time){
						UTIL_MakeVectors ( pev->angles );
						pev->velocity = gpGlobals->v_forward * 4000;
						m_flKillBeamTime = gpGlobals->time + 1.0;

						EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, "god623/holysword_dash.wav", 1.0, 0.1, 0, 100 + RANDOM_LONG(-5,5) );

						MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
						WRITE_BYTE( TE_BEAMFOLLOW );
						WRITE_SHORT( entindex());		// entity, attachment
						WRITE_SHORT(g_sModelIndexTrail);	// model
						WRITE_BYTE( 30 ); // life
						WRITE_BYTE( 30 );  // width
						WRITE_BYTE( 255 );	// R
						WRITE_BYTE( 255 );	// G
						WRITE_BYTE( 255 );	// B
						WRITE_BYTE( 255 );	// brightness
						MESSAGE_END();  // move PHS/PVS data sending into here (SEND_ALL, SEND_PVS, SEND_PHS)

						MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
						WRITE_BYTE( TE_BEAMFOLLOW );
						WRITE_SHORT( entindex() + 0x1000 * 1 );		// entity, attachment
						WRITE_SHORT(g_sModelIndexTrail);	// model
						WRITE_BYTE( 30 ); // life
						WRITE_BYTE( 20 );  // width
						WRITE_BYTE( 255 );	// R
						WRITE_BYTE( 128 );	// G
						WRITE_BYTE( 128 );	// B
						WRITE_BYTE( 255 );	// brightness
						MESSAGE_END();  // move PHS/PVS data sending into here (SEND_ALL, SEND_PVS, SEND_PHS)

						MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
						WRITE_BYTE( TE_BEAMFOLLOW );
						WRITE_SHORT( entindex() + 0x1000 * 2 );		// entity, attachment
						WRITE_SHORT(g_sModelIndexTrail);	// model
						WRITE_BYTE( 30 ); // life
						WRITE_BYTE( 20 );  // width
						WRITE_BYTE( 128 );	// R
						WRITE_BYTE( 128 );	// G
						WRITE_BYTE( 255 );	// B
						WRITE_BYTE( 255 );	// brightness
						MESSAGE_END();  // move PHS/PVS data sending into here (SEND_ALL, SEND_PVS, SEND_PHS)

						MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
						WRITE_BYTE( TE_BEAMFOLLOW );
						WRITE_SHORT( entindex() + 0x1000 * 3 );		// entity, attachment
						WRITE_SHORT(g_sModelIndexTrail);	// model
						WRITE_BYTE( 30 ); // life
						WRITE_BYTE( 20 );  // width
						WRITE_BYTE( 255 );	// R
						WRITE_BYTE( 128 );	// G
						WRITE_BYTE( 128 );	// B
						WRITE_BYTE( 255 );	// brightness
						MESSAGE_END();  // move PHS/PVS data sending into here (SEND_ALL, SEND_PVS, SEND_PHS)

						MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
						WRITE_BYTE( TE_BEAMFOLLOW );
						WRITE_SHORT( entindex() + 0x1000 * 4 );		// entity, attachment
						WRITE_SHORT(g_sModelIndexTrail);	// model
						WRITE_BYTE( 30 ); // life
						WRITE_BYTE( 20 );  // width
						WRITE_BYTE( 128 );	// R
						WRITE_BYTE( 128 );	// G
						WRITE_BYTE( 255 );	// B
						WRITE_BYTE( 255 );	// brightness
						MESSAGE_END();  // move PHS/PVS data sending into here (SEND_ALL, SEND_PVS, SEND_PHS)
			}
		}
		break;

		case 6:
		{//�黯
			m_debugseqTime = 0;

			SetYawSpeed();
			pev->movetype = MOVETYPE_FLY;
			pev->velocity = g_vecZero;

			if(pev->impulse == 0){//��������
			UTIL_SetOrigin( pev, m_teleportorigin2 );
			pev->impulse = 1;
			}
			else{
			UTIL_SetOrigin( pev, m_teleportorigin );
			pev->impulse = 0;
			}

			pev->renderfx = 0;
			pev->takedamage = DAMAGE_AIM;
			pev->takedamage = SOLID_BBOX;

			if ( RANDOM_LONG(0,1) ){
			pev->velocity.x = 96;
			}
			else{
			pev->velocity.x = -96;
			}

			int rdmax = 9;
			if(g_iSkillLevel == SKILL_HARD){
			rdmax -= 1;
			}
			if(pev->health <= pev->max_health * 0.6){
			rdmax -= 1;
			}
			//Bug Fix 3.0 ���ʥ�����и��ʣ�10%-30%
			if (RANDOM_LONG(0,9) >= rdmax){
			m_flNextSkill3Time = gpGlobals->time;
			m_flNextSkill1Time += 3.0;
			m_flNextSkill2Time += 3.0;
			m_flNextSkill4Time += 3.0;
			}
			else{
			m_flNextSkill3Time = gpGlobals->time + RANDOM_FLOAT( 20.0, 25.0 );
			}

		}
		break;

		case 7:
		{//�ٻ�����!
			if(m_flNextSkill4Time <= gpGlobals->time){
				Vector vecArmPos,vecArmDir,vecSpitDir;
				GetBonePosition( 9, vecArmPos, vecArmDir );

				UTIL_MakeVectors ( pev->angles );
				vecArmPos = vecArmPos + gpGlobals->v_forward * 192;

				if(m_childguy == NULL){
				CBaseMonster *pEnemyMonster;
				CBaseEntity *pMonsterEntity;
				pMonsterEntity = Create( "monster_zdeadeye", vecArmPos, pev->angles, edict() );
				pEnemyMonster = pMonsterEntity->MyMonsterPointer();
				pEnemyMonster->m_alert = 100;
				pEnemyMonster->m_killed_exp = 1;//����û�о���ֵ!
				pEnemyMonster->m_selfmode = TRUE;

				if (g_iSkillLevel == SKILL_HARD){//����ģʽ����Ӧ����һЩ!
				pEnemyMonster->m_boltpoison = 50;
				}
				else{
				pEnemyMonster->m_boltpoison = 60;
				}

				pMonsterEntity->pev->frags = 2;
				m_childguy = pMonsterEntity;

				SetBodygroup( 4, 1 );
				}

				m_flNextSkill4Time = gpGlobals->time + RANDOM_FLOAT( 75.0, 90.0 );
			}
		}
		break;

		case 8://������ը
		{
			Vector org = Center();

			MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
			WRITE_BYTE( TE_LARGEFUNNEL );
			WRITE_COORD( org.x );
			WRITE_COORD( org.y );
			WRITE_COORD( org.z );
			WRITE_SHORT( god_deadball );
			WRITE_SHORT( 1 );
			MESSAGE_END();

			FX_Explosion( Center(), EXPLOSION_CHRONOCLIP );

			EMIT_SOUND_DYN( ENT(pev), CHAN_STREAM, "weapons/chronoclip_explode.wav", 1, 0.1, 0, 100);
			SUB_StartFadeOut3();
		}
		break;

		default:
			CSquadMonster::HandleAnimEvent( pEvent );
			break;
	}
}

//=========================================================
// CheckRangeAttack1 - normal beam attack 
//=========================================================
BOOL CGodBoss :: CheckRangeAttack1 ( float flDot, float flDist )
{
	if (m_flNextSkill1Time > gpGlobals->time || pev->movetype == MOVETYPE_NOCLIP)
	{
		return FALSE;
	}

	if ( GetBodygroup( 5 ) == 1 ){
	SetBodygroup( 5, 0 );
	}

	return TRUE;
}

BOOL CGodBoss :: CheckRangeAttack2 ( float flDot, float flDist )
{
	if (m_flNextSkill2Time > gpGlobals->time || pev->movetype == MOVETYPE_NOCLIP)
	{
		return FALSE;
	}
	return TRUE;
}

//=========================================================
// CheckRangeAttack2 - check bravery and try to resurect dead comrades
//=========================================================
BOOL CGodBoss :: CheckMeleeAttack1 ( float flDot, float flDist )
{
	if (m_flNextSkill3Time > gpGlobals->time || pev->movetype == MOVETYPE_NOCLIP)
	{
		return FALSE;
	}
	
	if ( GetBodygroup( 5 ) == 1 ){
	SetBodygroup( 5, 0 );
	}

	return TRUE;
}

BOOL CGodBoss :: CheckMeleeAttack2 ( float flDot, float flDist )
{
	if (m_flNextSkill4Time > gpGlobals->time || m_childguy != NULL 
	|| pev->health >= pev->max_health * 0.7 || pev->movetype == MOVETYPE_NOCLIP)
	{
		return FALSE;
	}
	
	if ( GetBodygroup( 4 ) == 1 ){
	SetBodygroup( 4, 0 );
	}

	return TRUE;
}
//=========================================================
// Spawn
//=========================================================
void CGodBoss :: Spawn()
{
	Precache( );

	SET_MODEL(ENT(pev), "models/god623_boss.mdl");
	UTIL_SetSize(pev, Vector( -160, -160, -480 ), Vector( 160, 160, 480 ));

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_FLY;
	m_bloodColor		= DONT_BLEED;
	pev->effects		= 0;
	
	//pev->flags		   |= FL_FLY;

	if (g_iSkillLevel == SKILL_HARD){
	pev->health			= 100000;
	}
	else{
	pev->health			= 90000;
	}

	pev->view_ofs		= Vector ( 0, 0, 64 );// position of the eyes relative to monster's origin.
	m_flFieldOfView		= 0; // NOTE: we need a wide field of view so npc will notice player and say hello
	m_MonsterState		= MONSTERSTATE_NONE;
	m_afCapability		= bits_CAP_HEAR | bits_CAP_TURN_HEAD | bits_CAP_DOORS_GROUP;

	MonsterInit();

	pev->gravity		= 3.0;

	m_flNextSkill1Time = gpGlobals->time + 18.0;//����
	m_flNextSkill2Time = gpGlobals->time + 2.0;//��Ȧ
	m_flNextSkill3Time = gpGlobals->time + 12.0;//�����
	m_flNextSkill4Time = gpGlobals->time + 36.0;//�ٻ���

	m_debugseqTime = 0;
	m_flKillBeamTime = 0;

	m_ignoredamage = 1;
	m_longming	   = 1;

	m_selfmode = TRUE;
	pev->body = 0;

	m_aimenemy_mod = 6;

	m_killed_exp = 9000;
	m_is_the_boss = TRUE;
	m_rpgms_level = 130;
	pev->netname = MAKE_STRING( "Z.Z.God" );

	m_freeze_def = 3;//���Ό��LV3!!!

	m_singdelay_max = 0;//0��Ӧ
	m_singdelay_use = m_singdelay_max;

	m_facing_fucking_mode = 1;

	pev->effects = EF_BRIGHTLIGHT;

	m_teleportorigin = pev->origin;
	m_teleportorigin2 = m_teleportorigin;
	m_teleportorigin2.y += 1920;

	m_enemyget_mode = 1;//Զ����!

	m_attack_dist = 1536;

	if ( FClassnameIs(pev, "monster_god623_boss")){//����
	pev->movetype	 = MOVETYPE_NONE;
	pev->spawnflags |= SF_MONSTER_PRISONER;
	}

	m_painTime = 0;
	m_childguy = NULL;
	m_FTSmod = 7;
	m_no_pov_limit = 1;
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CGodBoss :: Precache()
{
	PRECACHE_MODEL("models/god623_boss.mdl");
	PRECACHE_MODEL("models/holysword_skill.mdl");
	
	PRECACHE_SOUND("rmxp/160-Skill04.wav");
	PRECACHE_SOUND("god623/holysword_up.wav");
	PRECACHE_SOUND("god623/holysword_dash.wav");
	PRECACHE_SOUND("god623/die.wav");
	
	PRECACHE_SOUND("misaliya/frostnova.wav");
	PRECACHE_SOUND("majo/flame_hitwall.wav");
	PRECACHE_MODEL("sprites/unused_spark1.spr");
	PRECACHE_SOUND("weapons/chronoclip_explode.wav");

	god_deadball =  PRECACHE_MODEL("sprites/anim_spr3.spr");

	UTIL_PrecacheOther( "monster_zdeadeye" );
}	


//=========================================================
// TakeDamage - get provoked when injured
//=========================================================

int CGodBoss :: TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType)
{
	if( (bitsDamageType & DMG_DARK) || (bitsDamageType & DMG_CLUB) ){
		flDamage *= 2.0;
	}
	else if( (bitsDamageType & DMG_BLAST) || (bitsDamageType & DMG_ENERGYBLAST) ){
		flDamage *= 1.5;
	}
	else if( (bitsDamageType & DMG_MORTAR) || (bitsDamageType & DMG_VALVE_SWORD) ){
		flDamage *= 1.25;
	}

	return CBaseMonster::TakeDamage(pevInflictor, pevAttacker, flDamage, bitsDamageType);
}

void CGodBoss::TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType)
{//��Ѫ������������BUG���֣�
	if ( pev->dmgtime != gpGlobals->time || (RANDOM_LONG(0,100) < 20))
	{
	pev->dmgtime = gpGlobals->time;
	UTIL_Sparks(ptr->vecEndPos);
	}

	CBaseMonster::TraceAttack( pevAttacker, flDamage, vecDir, ptr, bitsDamageType );
}


