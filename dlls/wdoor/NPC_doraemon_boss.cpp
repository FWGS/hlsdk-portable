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
#include    "gamerules.h"
#include    "soundent.h"
#include    "player.h"
#include    "decals.h"

extern DLL_GLOBAL int		g_iSkillLevel;

class CDoraemonRocket : public CBaseEntity
{
public:
	void Spawn( void );

	void EXPORT Aim_think( void );
	void Touch( CBaseEntity *pOther );
};

LINK_ENTITY_TO_CLASS( doraemon_rocket, CDoraemonRocket );

void CDoraemonRocket:: Spawn( void )
{
	pev->movetype = MOVETYPE_NOCLIP;

	pev->classname = MAKE_STRING( "doraemon_rocket" );
	
	SET_MODEL(ENT(pev), "models/HVR.mdl");
	pev->dmg = 80;

	UTIL_SetSize(pev, Vector( -4, -4, -16), Vector(4, 4, 0));
	UTIL_SetOrigin( pev, pev->origin );

	pev->solid = SOLID_BBOX;

	pev->velocity.z = 2000;
	pev->angles = UTIL_VecToAngles (pev->velocity);

	MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
	WRITE_BYTE( TE_BEAMFOLLOW );
	WRITE_SHORT( entindex() );		// entity, attachment
	WRITE_SHORT(g_sModelIndexTrail );	// model
	WRITE_BYTE( 4 ); // life
	WRITE_BYTE( 8 );  // width
	WRITE_BYTE( 255 );	// R
	WRITE_BYTE( 255 );	// G
	WRITE_BYTE( 255 );	// B
	WRITE_BYTE( 192 );	// brightness
	MESSAGE_END();  // move PHS/PVS data sending into here (SEND_ALL, SEND_PVS, SEND_PHS)

	SetThink ( &CDoraemonRocket::Aim_think );
	pev->nextthink = gpGlobals->time + 2.0;
}

void CDoraemonRocket :: Touch ( CBaseEntity *pOther )
{
	TraceResult tr;
	Vector		vecSpot;// trace starts here!

	pev->enemy = pOther->edict();

	vecSpot = pev->origin - pev->velocity.Normalize() * 32;
	UTIL_TraceLine( vecSpot, vecSpot + pev->velocity.Normalize() * 64, ignore_monsters, ENT(pev), &tr );
	vecSpot = (tr.vecEndPos + tr.vecPlaneNormal * 15);

	FX_Explosion( vecSpot, 132 );
	
	MESSAGE_BEGIN( MSG_PAS, SVC_TEMPENTITY, pev->origin );
		WRITE_BYTE( TE_EXPLOSION );		// This makes a dynamic light and the explosion sprites/sound
		WRITE_COORD( pev->origin.x );	// Send to PAS because of the sound
		WRITE_COORD( pev->origin.y );
		WRITE_COORD( pev->origin.z );
		WRITE_SHORT( g_sModelIndexFireball );
		WRITE_BYTE( 0  ); // scale * 10
		WRITE_BYTE( 15  ); // framerate
		WRITE_BYTE( 10 );
	MESSAGE_END();

		CSoundEnt::InsertSound ( bits_SOUND_COMBAT, pev->origin, NORMAL_EXPLOSION_VOLUME, 3.0 );

	entvars_t *pevOwner;
	if ( pev->owner )
		pevOwner = VARS( pev->owner );
	else
		pevOwner = NULL;

	pev->owner = NULL; // can't traceline attack owner if this is set

	if ( pOther->pev->flags & FL_MONSTER ){
		CBaseMonster *pEnemyMonster;
		pEnemyMonster = pOther->MyMonsterPointer();
		if(pEnemyMonster){
		pEnemyMonster->m_trouch_full_radiusdmg += 1;
		}
	}

	::RadiusDamage_limit( vecSpot, pev, pevOwner, pev->dmg, pev->dmg * 2.5, CLASS_HUMAN_MILITARY, DMG_BLAST );

	if ( RANDOM_FLOAT( 0 , 1 ) < 0.5 )
	{
		UTIL_DecalTrace( &tr, DECAL_SCORCH1 );
	}
	else
	{
		UTIL_DecalTrace( &tr, DECAL_SCORCH2 );
	}

	UTIL_Remove(this);
}

void CDoraemonRocket::Aim_think( void )
{
	entvars_t *pevOwner;
	if ( pev->owner ){
		pevOwner = VARS( pev->owner );
	}

	if(pev->owner == NULL || pev->frags >= 13){
		SetThink ( NULL );
		UTIL_Remove( this );
		return;
	}

	if(pev->frags <= 5){
		pev->effects |= EF_NODRAW;
		pev->velocity = g_vecZero;
		if ( pevOwner->flags & FL_MONSTER ){
			CBaseMonster *pEnemyMonster;
			pEnemyMonster = Instance( pev->owner )->MyMonsterPointer();
			if(pEnemyMonster){
					TraceResult tr;
					UTIL_TraceLine ( pevOwner->origin, pevOwner->origin + Vector(0,0,8192), ignore_monsters, ENT(pevOwner), &tr );
					pev->origin = tr.vecEndPos;
					pev->origin.x = pEnemyMonster->m_vecEnemyLKP.x;
					pev->origin.y = pEnemyMonster->m_vecEnemyLKP.y;

					if(pev->frags == 5){//���դ�����
					pev->origin.x += RANDOM_FLOAT( -64, 64 );
					pev->origin.y += RANDOM_FLOAT( -64, 64 );
					pev->origin.z -= 16;
					UTIL_SetOrigin( pev, pev->origin );
					}
			}
		}
	}
	else if(pev->frags == 6){
		pev->effects &= ~EF_NODRAW;
		pev->movetype = MOVETYPE_FLY;
		pev->velocity.z = -2000;
		pev->angles = UTIL_VecToAngles (pev->velocity);

		MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
		WRITE_BYTE( TE_BEAMFOLLOW );
		WRITE_SHORT( entindex() );		// entity, attachment
		WRITE_SHORT(g_sModelIndexTrail );	// model
		WRITE_BYTE( 4 ); // life
		WRITE_BYTE( 8 );  // width
		WRITE_BYTE( 255 );	// R
		WRITE_BYTE( 255 );	// G
		WRITE_BYTE( 255 );	// B
		WRITE_BYTE( 192 );	// brightness
		MESSAGE_END();  // move PHS/PVS data sending into here (SEND_ALL, SEND_PVS, SEND_PHS)
	}

	pev->frags += 1;

	pev->nextthink = gpGlobals->time + 0.5;
}

class CDoraemonGun : public CBaseEntity
{
public:
	void Spawn( void );

	void EXPORT Thinking( void );
	void Touch( CBaseEntity *pOther );
	static void Shoot( entvars_t *pevOwner, Vector vecStart, Vector vecVelocity );
};

LINK_ENTITY_TO_CLASS( doraemon_gun, CDoraemonGun );

void CDoraemonGun:: Spawn( void )
{
	pev->movetype = MOVETYPE_FLY;

	pev->classname = MAKE_STRING( "doraemon_gun" );
	
	SET_MODEL(ENT(pev), "models/camera_rocket.mdl");
	pev->dmg = 80;

	UTIL_SetSize(pev, Vector( 0, 0, 0), Vector(0, 0, 0));
	UTIL_SetOrigin( pev, pev->origin );

	pev->solid = SOLID_BBOX;

	MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
	WRITE_BYTE( TE_BEAMFOLLOW );
	WRITE_SHORT( entindex() );		// entity, attachment
	WRITE_SHORT(g_sModelIndexTrail );	// model
	WRITE_BYTE( 4 ); // life
	WRITE_BYTE( 8 );  // width
	WRITE_BYTE( 255 );	// R
	WRITE_BYTE( 255 );	// G
	WRITE_BYTE( 255 );	// B
	WRITE_BYTE( 192 );	// brightness
	MESSAGE_END();  // move PHS/PVS data sending into here (SEND_ALL, SEND_PVS, SEND_PHS)
}

void CDoraemonGun::Shoot( entvars_t *pevOwner, Vector vecStart, Vector vecVelocity )
{
	CDoraemonGun *pSpit = GetClassPtr( (CDoraemonGun *)NULL );
	pSpit->Spawn();
	
	UTIL_SetOrigin( pSpit->pev, vecStart );
	pSpit->pev->velocity = vecVelocity;
	pSpit->pev->owner = ENT(pevOwner);
	pSpit->pev->angles = UTIL_VecToAngles (pSpit->pev->velocity);
}

void CDoraemonGun :: Touch ( CBaseEntity *pOther )
{
	TraceResult tr;
	Vector		vecSpot;// trace starts here!

	pev->enemy = pOther->edict();

	vecSpot = pev->origin - pev->velocity.Normalize() * 32;
	UTIL_TraceLine( vecSpot, vecSpot + pev->velocity.Normalize() * 64, ignore_monsters, ENT(pev), &tr );
	vecSpot = (tr.vecEndPos + tr.vecPlaneNormal * 15);

	FX_Explosion( vecSpot, 132 );
	
	MESSAGE_BEGIN( MSG_PAS, SVC_TEMPENTITY, pev->origin );
		WRITE_BYTE( TE_EXPLOSION );		// This makes a dynamic light and the explosion sprites/sound
		WRITE_COORD( pev->origin.x );	// Send to PAS because of the sound
		WRITE_COORD( pev->origin.y );
		WRITE_COORD( pev->origin.z );
		WRITE_SHORT( g_sModelIndexFireball );
		WRITE_BYTE( 0  ); // scale * 10
		WRITE_BYTE( 15  ); // framerate
		WRITE_BYTE( 10 );
	MESSAGE_END();

		CSoundEnt::InsertSound ( bits_SOUND_COMBAT, pev->origin, NORMAL_EXPLOSION_VOLUME, 3.0 );

	entvars_t *pevOwner;
	if ( pev->owner )
		pevOwner = VARS( pev->owner );
	else
		pevOwner = NULL;

	pev->owner = NULL; // can't traceline attack owner if this is set

	if ( pOther->pev->flags & FL_MONSTER ){
		CBaseMonster *pEnemyMonster;
		pEnemyMonster = pOther->MyMonsterPointer();
		if(pEnemyMonster){
		pEnemyMonster->m_trouch_full_radiusdmg += 1;
		}
	}

	::RadiusDamage_limit( vecSpot, pev, pevOwner, pev->dmg, pev->dmg * 2.5, CLASS_HUMAN_MILITARY, DMG_BLAST );

	if ( RANDOM_FLOAT( 0 , 1 ) < 0.5 )
	{
		UTIL_DecalTrace( &tr, DECAL_SCORCH1 );
	}
	else
	{
		UTIL_DecalTrace( &tr, DECAL_SCORCH2 );
	}

	UTIL_Remove(this);
}

void CDoraemonGun::Thinking( void )
{
	SetThink ( NULL );
	UTIL_Remove( this );
	return;
}


//=========================================================
// Monster's Anim Events Go Here
//=========================================================

class CDoraemonBoss : public CBaseMonster
{
public:
	void Spawn( void );
	void Precache( void );
	void SetYawSpeed( void );
	int  Classify ( void );
	void HandleAnimEvent( MonsterEvent_t *pEvent );

	float m_flNextFlinch;
	float m_flNextPainTime;

	void Doraemon_laser_fire( void );

	void Killed( entvars_t *pevAttacker, int iGib );

	void TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType);

	void RunAI( void );

	BOOL FCanCheckAttacks ( void );

	// No range attacks
	BOOL CheckRangeAttack1 ( float flDot, float flDist );//������
	BOOL CheckRangeAttack2 ( float flDot, float flDist );//�����
	BOOL CheckMeleeAttack1 ( float flDot, float flDist );//����
	BOOL CheckMeleeAttack2 ( float flDot, float flDist );//�ڴ�
	int TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType );

	int	Save( CSave &save ); 
	int Restore( CRestore &restore );

	static TYPEDESCRIPTION m_SaveData[];

	Vector m_teleportorigin;

	float m_SkillTime1;
	float m_SkillTime2;
	float m_SkillTime3;
	float m_SkillTime4;
//	float m_SkillTime5;
	float m_ArmDefTime;
	float m_flKillBeamTime;
	float m_flFlyingTime;

	BOOL	m_fGunDrawn;
	EHANDLE	m_childguy;
};

LINK_ENTITY_TO_CLASS( monster_doraemon_boss, CDoraemonBoss );

TYPEDESCRIPTION	CDoraemonBoss::m_SaveData[] = 
{
	DEFINE_FIELD( CDoraemonBoss, m_SkillTime1, FIELD_TIME ),
	DEFINE_FIELD( CDoraemonBoss, m_SkillTime2, FIELD_TIME ),
	DEFINE_FIELD( CDoraemonBoss, m_SkillTime3, FIELD_TIME ),
	DEFINE_FIELD( CDoraemonBoss, m_SkillTime4, FIELD_TIME ),
//	DEFINE_FIELD( CDoraemonBoss, m_SkillTime5, FIELD_TIME ),
	DEFINE_FIELD( CDoraemonBoss, m_ArmDefTime, FIELD_TIME ),
	DEFINE_FIELD( CDoraemonBoss, m_flFlyingTime, FIELD_TIME ),
	DEFINE_FIELD( CDoraemonBoss, m_flKillBeamTime, FIELD_TIME ),
	DEFINE_FIELD( CDoraemonBoss, m_teleportorigin, FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( CDoraemonBoss, m_fGunDrawn, FIELD_BOOLEAN ),
	DEFINE_FIELD( CDoraemonBoss, m_childguy, FIELD_EHANDLE ),
};

IMPLEMENT_SAVERESTORE( CDoraemonBoss, CBaseMonster );

BOOL CDoraemonBoss :: FCanCheckAttacks ( void )
{
	return TRUE;
}

void CDoraemonBoss :: Doraemon_laser_fire ( void )
{

}

void CDoraemonBoss :: RunAI( void )
{
	CBaseMonster :: RunAI();

	if(pev->sequence == LookupActivity ( ACT_WALK )){
	m_flGroundSpeed = 140;
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
		if(m_freezetime > 0){
		m_flFlyingTime = 0;
		}

		UTIL_MakeVectors ( pev->angles );
		if ( m_hEnemy != NULL ){
		pev->velocity = gpGlobals->v_forward * 400;
		}

		Vector	vecSpitOffset,vangle;
		Vector	vecSpitDir;
		GetAttachment( RANDOM_LONG( 1, 2 ), vecSpitOffset, vangle );
		UTIL_Sparks( vecSpitOffset );
		::RadiusDamage_limit( vecSpitOffset, pev, pev, 60, 90, CLASS_HUMAN_MILITARY, DMG_CRUSH);
	}
	else if(pev->movetype != MOVETYPE_STEP){
		pev->movetype = MOVETYPE_STEP;
		m_groundElev2 = FALSE;
		ClearSchedule();
		SetYawSpeed();
	}

	if(!FBitSet( pev->spawnflags, SF_MONSTER_PRISONER ) && pev->takedamage == DAMAGE_NO){
		if(m_freezetime <= 1){
			EMIT_SOUND( ENT(pev), CHAN_WEAPON, "weapons/displacer_self.wav", 1, 0.6 );
			FX_Explosion(Center(), 134 );
			pev->flags &= ~FL_NOTARGET;
			pev->effects &= ~EF_NODRAW;
			pev->takedamage = DAMAGE_AIM;
		}
	}

	if(m_enemyget_mode == 1 && pev->health <= pev->max_health * 0.5){
	m_enemyget_mode = 0;//���ۣ���Ϊ������!
	}
}

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int	CDoraemonBoss :: Classify ( void )
{
	return	CLASS_MACHINE;
}

//=========================================================
// ���������
//=========================================================
BOOL CDoraemonBoss::CheckRangeAttack1( float flDot, float flDist )
{
	if ( gpGlobals->time > m_SkillTime1)
	{
		return TRUE;
	}
	return FALSE;
}

//=========================================================
// ���������
//=========================================================
BOOL CDoraemonBoss::CheckRangeAttack2( float flDot, float flDist )
{
	if ( gpGlobals->time > m_SkillTime2)
	{
		return TRUE;
	}

	return FALSE;
}

//�ڴ�ʹ��
BOOL CDoraemonBoss :: CheckMeleeAttack2 ( float flDot, float flDist )
{
	if ( gpGlobals->time > m_SkillTime3 && m_childguy == NULL )//��������
	{
		return TRUE;
	}

	return FALSE;
}

//��ͷ�͹�
BOOL CDoraemonBoss :: CheckMeleeAttack1 ( float flDot, float flDist )
{
	if ( gpGlobals->time > m_SkillTime4 && flDist <= 512)
	{
		return TRUE;
	}

	return FALSE;
}


void CDoraemonBoss::TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType)
{
	if ( pev->dmgtime != gpGlobals->time || (RANDOM_LONG(0,100) < 20))
	{
		pev->dmgtime = gpGlobals->time;

		UTIL_WhiteSparks( ptr->vecEndPos, ptr->vecPlaneNormal, 9, 6, 5, 100 );//puntos
		UTIL_Sparks(ptr->vecEndPos);
	}

	CBaseMonster::TraceAttack( pevAttacker, flDamage, vecDir, ptr, bitsDamageType );
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CDoraemonBoss :: SetYawSpeed ( void )
{
	int ys;

	switch ( m_Activity )
	{
	case ACT_MELEE_ATTACK1:	
		ys = 120;	
		break;
	default:
		ys = 180;
		break;
	}

	pev->yaw_speed = ys;
}

int CDoraemonBoss :: TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	if(pevAttacker != NULL){
		CBaseEntity *pEntity = GetClassPtr((CBaseEntity *)pevAttacker);
		if(pEntity == this){
		return 0;
		}
	}
	return CBaseMonster::TakeDamage( pevInflictor, pevAttacker, flDamage, bitsDamageType );
}

void CDoraemonBoss::Killed( entvars_t *pevAttacker, int iGib )
{
	if(m_childguy != NULL){
		if(m_childguy->pev->deadflag == DEAD_NO){
		m_childguy->Killed( pev, GIB_NEVER );
		}
	}
	pev->movetype = MOVETYPE_STEP;
	pev->gravity  = 1.6;
	m_flFlyingTime = 0;
	pev->skin = 1;

	CBaseMonster::Killed( pevAttacker, GIB_NEVER );
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CDoraemonBoss :: HandleAnimEvent( MonsterEvent_t *pEvent )
{
	switch( pEvent->event )
	{
		case 1://�ͳ�������
		{
			pev->body = 0;
			pev->skin = 1;
		}
		break;

		case 2://�����ڿ���!
		{
			if(m_SkillTime1 <= gpGlobals->time && m_hEnemy != NULL){
			Vector	vecSpitOffset,vangle;
			Vector	vecSpitDir;

			UTIL_MakeVectors ( pev->angles );

			// !!!HACKHACK - the spot at which the spit originates (in front of the mouth) was measured in 3ds and hardcoded here.
			// we should be able to read the position of bones at runtime for this info.
			GetAttachment( 0, vecSpitOffset, vangle );

			FX_Explosion( vecSpitOffset, EXPLOSION_SPARKSHOWER );

			vecSpitDir = ( m_hEnemy->Center() - vecSpitOffset ).Normalize();

			EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, "doraemon/ragingbull_shoot.wav", 1.0, 0.3,0,100 + RANDOM_LONG(-5,5) );

			CDoraemonGun::Shoot( pev, vecSpitOffset, vecSpitDir * 6000 );

			m_SkillTime1 = gpGlobals->time + RANDOM_FLOAT( 0.5, 4.0 );

			}
		}
		break;

		case 3://�ͳ�����
		{
			pev->body = 1;
			pev->skin = 2;//Ц
			EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "doraemon/doraemon1.wav", 1.0, 0.2,0,100);
		}
		break;

		case 4://�������β
		{
			MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
			WRITE_BYTE( TE_BEAMFOLLOW );
			WRITE_SHORT( entindex() + 0x1000 * 2 );		// entity, attachment
			WRITE_SHORT(g_sModelIndexTrail);	// model
			WRITE_BYTE( 8 ); // life
			WRITE_BYTE( 4 );  // width
			WRITE_BYTE( 255 );	// R
			WRITE_BYTE( 255 );	// G
			WRITE_BYTE( 255 );	// B
			WRITE_BYTE( 192 );	// brightness
			MESSAGE_END();  // move PHS/PVS data sending into here (SEND_ALL, SEND_PVS, SEND_PHS)

			MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
			WRITE_BYTE( TE_BEAMFOLLOW );
			WRITE_SHORT( entindex() + 0x1000 * 3 );		// entity, attachment
			WRITE_SHORT(g_sModelIndexTrail);	// model
			WRITE_BYTE( 8 ); // life
			WRITE_BYTE( 4 );  // width
			WRITE_BYTE( 255 );	// R
			WRITE_BYTE( 255 );	// G
			WRITE_BYTE( 255 );	// B
			WRITE_BYTE( 192 );	// brightness
			MESSAGE_END();  // move PHS/PVS data sending into here (SEND_ALL, SEND_PVS, SEND_PHS)

			m_flKillBeamTime = gpGlobals->time + 5.0;
		}
		break;

		case 5://����ι�����ʼ
		{
			pev->movetype = MOVETYPE_BOUNCEMISSILE;
			m_groundElev2 = TRUE;//��ֹ��������?
			m_flFlyingTime = gpGlobals->time + 4.0;
			m_SkillTime4 = gpGlobals->time + RANDOM_FLOAT( 18.0, 24.0 );
		}
		break;

		case 6://�ͳ����Ͳ
		{
			pev->body = 2;
			pev->skin = 2;//Ц
			EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "doraemon/doraemon1.wav", 1.0, 0.2,0,100);
		}
		break;

		case 7://���Ͳ4����
		{
			if(m_SkillTime2 <= gpGlobals->time){
			Vector	vecSpitOffset,vangle;
			Vector	vecSpitDir;
			GetAttachment( 3, vecSpitOffset, vangle );
			CBaseEntity::Create( "doraemon_rocket", vecSpitOffset, pev->angles, edict() );
			EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, "doraemon/rlauncher_fire.wav", 1.0, 0.3,0,100 + RANDOM_LONG(-5,5) );
			}
		}
		break;

		case 8://���ͲCD
		{
			pev->body = 3;
			pev->skin = 1;
			m_SkillTime2 = gpGlobals->time + RANDOM_FLOAT( 12.0, 18.0 );
			ClearSchedule();
			SetYawSpeed();
		}
		break;

		case 9://׼���ͳ�ĳ��
		{
			pev->body = 3;
			pev->skin = 2;//Ц
			EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "doraemon/doraemon1.wav", 1.0, 0.2,0,100);
		}
		break;

		case 10://�ͳ�ĳ��
		{
			/*
			if(m_SkillTime5 < gpGlobals->time){//���ⵯ!
				CGrenade::Shoot_Flashbang( pev, Center(), Vector(0,0,64), 3.0 );
				m_SkillTime5 = gpGlobals->time + RANDOM_FLOAT( 30.0, 35.0 );
				ClearSchedule();
				SetYawSpeed();
			}
			else{
			*/
				if(m_childguy == NULL){
				CBaseMonster *pEnemyMonster;

				CBaseEntity *pMonsterEntity;
				pMonsterEntity = Create( "monster_barney_hevshield", pev->origin, pev->angles, edict() );
				pEnemyMonster = pMonsterEntity->MyMonsterPointer();
				pMonsterEntity->pev->team = 1;
				pEnemyMonster->m_alert = 100;
				pEnemyMonster->m_killed_exp = 1;//����û�о���ֵ!
				pEnemyMonster->m_no_cover_mode = 1;//���ֲ���������
				pEnemyMonster->m_guard_mode = TRUE;//����ģʽ
				pEnemyMonster->m_chase_mode = -1;
				pEnemyMonster->m_diefadeout = 1;
				pEnemyMonster->m_undropgun = TRUE;
				pEnemyMonster->m_selfmode = TRUE;

				m_childguy = pMonsterEntity;

				Freeze_Monster(80);//���ִ���
				pev->flags |= FL_NOTARGET;
				pev->effects |= EF_NODRAW;
				pev->takedamage = DAMAGE_NO;

				EMIT_SOUND( ENT(pev), CHAN_WEAPON, "weapons/displacer_self.wav", 1, 0.6 );
				FX_Explosion(Center(), 134 );
				}

				m_SkillTime3 = gpGlobals->time + RANDOM_FLOAT( 20.0, 25.0 );
				ClearSchedule();
				SetYawSpeed();
			//}
		}
		break;

		case 11://׼���Ա�
		{
			pev->effects |= EF_LIGHT;
		}
		break;

		case 12://�Ա�
		{
			FX_Explosion( Center(), EXPLOSION_C4 );
			EMIT_SOUND(ENT(pev), CHAN_STATIC, "weapons/mortarhit.wav", 1.0, 0.3);
			RadiusDamage( Center(), pev, pev, 320, CLASS_NONE, DMG_BLAST );

			SetThink ( &CDoraemonBoss::SUB_Remove );
			pev->nextthink = gpGlobals->time;
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
void CDoraemonBoss :: Spawn()
{
	Precache( );
	SET_MODEL(ENT(pev), "models/doraemon_boss.mdl");
	UTIL_SetSize(pev, Vector(-32,-32,0), Vector(32,32,72));

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	m_bloodColor		= DONT_BLEED;

	if (g_iSkillLevel == SKILL_HARD){
	pev->health			= 36000;
	}
	else{
	pev->health			= 30000;
	}

	pev->view_ofs		= VEC_VIEW;// position of the eyes relative to monster's origin.
	m_flFieldOfView		= 0;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState		= MONSTERSTATE_NONE;
	m_afCapability		= bits_CAP_DOORS_GROUP;
	
	MonsterInit();

	pev->gravity		= 1.6;

	m_SkillTime1 = 0;
	m_SkillTime2 = 0;
	m_SkillTime3 = 0;
	m_SkillTime4 = 0;
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

	m_killed_exp = 4000;
	m_rpgms_level = 100;
	m_is_the_boss = TRUE;
	pev->netname = MAKE_STRING( "Doraemon" );

	m_facing_fucking_mode = 1;

	m_childguy = NULL;

	pev->skin = 1;
	m_enemyget_mode = 1;//Զ����!

	m_freeze_def = 1;//����ο���LV1
}
//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CDoraemonBoss :: Precache()
{
	PRECACHE_MODEL("models/doraemon_boss.mdl");
	PRECACHE_MODEL("models/HVR.mdl");
	PRECACHE_SOUND("doraemon/doraemon1.wav");
	PRECACHE_SOUND("doraemon/ragingbull_shoot.wav");
	PRECACHE_SOUND("doraemon/rlauncher_fire.wav");
	PRECACHE_SOUND("weapons/mortarhit.wav");
	PRECACHE_SOUND("weapons/flashbang_explode.wav");

	UTIL_PrecacheOther( "monster_barney_hevshield" );
}	
