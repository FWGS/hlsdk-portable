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
// Generic Monster - purely for scripted sequence work.
//=========================================================
#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"schedule.h"
#include	"animation.h"
#include	"weapons.h"
#include	"soundent.h"
#include	"hornet.h"
#include	"shake.h"

// For holograms, make them not solid so the player can walk through them
#define	SF_GENERICMONSTER_NOTSOLID					4 

//=========================================================
// Monster's Anim Events Go Here
//=========================================================

class CDomaHuman : public CBaseMonster
{
public:
	void RunAI( void );
	void Spawn( void );
	void Precache( void );
	void SetYawSpeed( void );
	void SetActivity ( Activity NewActivity );
	int  Classify ( void );

	void BarneyFirePistol( void );
	void doma_laser_fire( void );

	void HandleAnimEvent( MonsterEvent_t *pEvent );
	int ISoundMask ( void );

	int m_iTrail;

	BOOL CheckRangeAttack1 ( float flDot, float flDist ) { return FALSE; }
	BOOL CheckRangeAttack2 ( float flDot, float flDist ) { return FALSE; }

	int TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType );
};
LINK_ENTITY_TO_CLASS( monster_doma, CDomaHuman );
//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int	CDomaHuman :: Classify ( void )
{
	return	CLASS_HUMAN_ASS;
}

void CDomaHuman :: doma_laser_fire ( void )
{
	

//	if(m_hEnemy != NULL){
	Vector org,vecdir;
	GetAttachment( 0, org,vecdir);
	FX_Trail(org, entindex(), 142);

	m_HackedGunPos = org;

	Vector vecShootDir = ShootAtEnemy( m_HackedGunPos );

	UTIL_VecToAngles( vecShootDir );

	FireBullets(1, m_HackedGunPos, vecShootDir, g_vecZero, 16384, BULLET_DOMA_LASER,0);

	FireBeam(m_HackedGunPos, vecShootDir, BEAM_TAUCANNON, 100, pev);

	EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, "doma/doma_fire2.wav", 1, 0.6, 0, 100);

	CSoundEnt::InsertSound ( bits_SOUND_COMBAT, pev->origin, 384, 0.3 );
	//}
}

void CDomaHuman :: RunAI( void )
{
	CBaseMonster :: RunAI();
	if(pev->frags == 1){
	m_flGroundSpeed = 300;
	}
	if(pev->weapons == 2){
		if(pev->sequence != LookupActivity ( ACT_RUN_SCARED )){
		SetActivity( ACT_RUN_SCARED );
		}
	}
	if(pev->sequence == LookupActivity ( ACT_WALK )){
	m_flGroundSpeed = 45;
	}
	if(pev->sequence == LookupActivity ( ACT_RUN )){
	m_flGroundSpeed = 255;
	}
	if(pev->frags == 1){
	m_flGroundSpeed = 300;
	}

	if(pev->impulse == 13){//Òþ²ØÏûÊ§¤ÎDoma£¬ÏÅ»£Íæ¼ÒÓÃ£¿
		if ( !FNullEnt( FIND_CLIENT_IN_PVS( edict() ) ) ){
			CBaseMonster *pClient;
			CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "player" );
			if ( pEntity ){
			pClient = pEntity->MyMonsterPointer();
			}
			if (pClient)
			{
				if (FInViewCone(pEntity) && FVisible(pEntity)){
					if (pClient->FInViewCone(this)){

						if(m_singdelay_use == 0){//ÏûÊ§
						UTIL_ScreenFade( pClient, Vector(0,0,0), 0.3, 0.3, 255, FFADE_IN );//ÉÁÏ¹Íæ¼Ò
						UTIL_Remove( this );
						return;
						}

						m_singdelay_use--;
					}
				}
			}
		}
	}

}

int CDomaHuman :: TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	if ( (pev->spawnflags & SF_MONSTER_GAG) ){
	pev->sequence = LookupSequence( "doma_sword_hit" );
	ResetSequenceInfo( );
	pev->frame = 0;
	return 0;
	}

	return CBaseMonster::TakeDamage( pevInflictor, pevAttacker, flDamage, bitsDamageType );
}

void CDomaHuman :: SetActivity ( Activity NewActivity )
{
	int	iSequence = ACTIVITY_NOT_AVAILABLE;
	void *pmodel = GET_MODEL_PTR( ENT(pev) );

	switch ( NewActivity)
	{
	case ACT_RUN:
		// grunt is either shooting standing or shooting crouched
		if (pev->weapons == 4)
		{
			iSequence = LookupSequence( "run_flash" );
		}
		else if (pev->weapons == 2)
		{
			iSequence = LookupActivity ( ACT_RUN_SCARED );
		}
		else
		{
			iSequence = LookupActivity ( NewActivity );
		}
		break;
	case ACT_WALK:
		// grunt is either shooting standing or shooting crouched
		if (pev->weapons == 4)
		{
			iSequence = LookupSequence( "walk_flash" );
		}
		else
		{
			iSequence = LookupActivity ( NewActivity );
		}
		break;
	case ACT_IDLE:
		// grunt is either shooting standing or shooting crouched
		if (pev->weapons == 1)
		{
			iSequence = LookupSequence( "dying_friendidle" );
		}
		else if (pev->weapons == 3)
		{
			iSequence = LookupActivity ( ACT_COMBAT_IDLE );
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
	m_IdealActivity = m_Activity;

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
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CDomaHuman :: SetYawSpeed ( void )
{
	pev->yaw_speed = 90;
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CDomaHuman :: HandleAnimEvent( MonsterEvent_t *pEvent )
{
	switch( pEvent->event )
	{
	case 1:
		{//Á¬Ðø³æÂÑÉä»÷
			Vector vecArmPos, vecArmDir;
			Vector vecDirToEnemy;
			Vector angDir;

			if (HasConditions( bits_COND_SEE_ENEMY))
			{
				vecDirToEnemy = ( ( m_vecEnemyLKP ) - pev->origin );
				angDir = UTIL_VecToAngles( vecDirToEnemy );
				vecDirToEnemy = vecDirToEnemy.Normalize();
			}
			else
			{
				angDir = pev->angles;
				UTIL_MakeAimVectors( angDir );
				vecDirToEnemy = gpGlobals->v_forward;
			}

			GetAttachment( 0, vecArmPos, vecArmDir );

			vecArmPos = vecArmPos + vecDirToEnemy * 8 + gpGlobals->v_right * RANDOM_LONG(-6,6) + gpGlobals->v_up * RANDOM_LONG(-4,4);

			CBaseEntity *pHornet = CBaseEntity::Create( "hornet_doma", vecArmPos, UTIL_VecToAngles( vecDirToEnemy ), edict() );
			UTIL_MakeVectors ( pHornet->pev->angles );
			pHornet->pev->velocity = gpGlobals->v_forward * 3000;
			pHornet->pev->health = 15;
	
			EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, "doma/doma_fire1.wav", 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-10,10) );
		}
		break;

	case 2:
		{//ÌÍ³öÎäÆ÷
			SetBodygroup( 4, 0 );
		}
		break;

	case 3:
		{//ÎäÆ÷×°Ìî
			
		}
		break;

	case 4:
		{//Ð¶ÏÂÎäÆ÷
		//	SetBodygroup( 4, 1 );
		}
		break;
	case 5:
		{//Ä§ÉñÒ»»÷¡¤ÐîÁ¦
			Vector vecStart, angleGun;
			GetAttachment( 0, vecStart, angleGun );
			FX_Trail(pev->origin, entindex(), 141);
		}
		break;
	case 6:
		{//Ä§ÉñÒ»»÷¡¤Éä»÷
			doma_laser_fire();
		}
		break;
	case 7:
		{//±äÉí
			SetBodygroup( 0, 0 );
			SetBodygroup( 1, 0 );
			SetBodygroup( 2, 0 );
			SetBodygroup( 3, 0 );
			SetBodygroup( 4, 0 );
			SetBodygroup( 5, 0 );

			pev->renderfx = kRenderFxExplode;
			pev->rendercolor.x = 255;
			pev->rendercolor.y = 255;
			pev->rendercolor.z = 255;
			FX_Explosion( Center(), 127);
			EMIT_SOUND_DYN ( ENT(pev), CHAN_STREAM, "newadd/exp2_frost.wav", 1.0, 0.6, 0, 100);
		}
		break;

	case 8://ÍÏÎ²
		{
			MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
			WRITE_BYTE( TE_BEAMFOLLOW );
			WRITE_SHORT( entindex() + 0x1000 * 1 );		// entity, attachment
			WRITE_SHORT(m_iTrail );	// model
			WRITE_BYTE( 8 ); // life
			WRITE_BYTE( 4 );  // width
			WRITE_BYTE( 255 );	// R
			WRITE_BYTE( 32 );	// G
			WRITE_BYTE( 32 );	// B
			WRITE_BYTE( 188 );	// brightness
			MESSAGE_END();  // move PHS/PVS data sending into here (SEND_ALL, SEND_PVS, SEND_PHS)
		}
		break;
	
		case 9://»÷ÖÐKadoma
		{
				CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_kadoma" );
				if ( pEntity )//¶ÔKadomaÌØ¹¥
				{
					
					CBaseMonster *pEnemyMonster = pEntity->MyMonsterPointer();

					pEnemyMonster->SetBodygroup( 1, 3);

					EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, "mario/mario_skill_hit.wav", 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
					FX_Explosion( pEntity->Center(), 47);

					FX_Explosion( pEntity->Center(), 236 );
					SpawnBlood(pEntity->Center(), BloodColor(), 200);
					FX_Trail(pEntity->Center(), entindex(), PROJ_GUTS );
					EMIT_SOUND(ENT(pEntity->pev), CHAN_BODY, "newadd/zom_headburst.wav", 1, ATTN_NORM);	
				}
		}
		break;

	default:
		CBaseMonster::HandleAnimEvent( pEvent );
		break;
	}
}

void CDomaHuman :: BarneyFirePistol ( void )
{
	Vector vecShootOrigin;

	UTIL_MakeVectors(pev->angles);

	vecShootOrigin = pev->origin + Vector( 0, 0, 65 );
	
	Vector vecShootDir = ShootAtEnemy( vecShootOrigin );

	pev->effects = EF_MUZZLEFLASH;

	int pitchShift = RANDOM_LONG( 0, 20 );
	
	// Only shift about half the time
	if ( pitchShift > 10 )
		pitchShift = 0;
	else
		pitchShift -= 5;
	

	int iBulletType;
	iBulletType = BULLET_12MM;

	FireBullets(1, vecShootOrigin, vecShootDir, Vector(0.0,0.0,0.0), 2048, iBulletType,1);

	EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, "weapons/pl_gun3.wav", 1, ATTN_NORM, 0, 100 + pitchShift );

	CSoundEnt::InsertSound ( bits_SOUND_COMBAT, pev->origin, 384, 0.3 );
}

//=========================================================
// ISoundMask - generic monster can't hear.
//=========================================================
int CDomaHuman :: ISoundMask ( void )
{
	return	NULL;
}

//=========================================================
// Spawn
//=========================================================
void CDomaHuman :: Spawn()
{
	Precache();

	SET_MODEL(ENT(pev), "models/doma.mdl");

	UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	m_bloodColor		= BLOOD_COLOR_RED;
	pev->health			= 10000;
	m_flFieldOfView		= 0.5;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState		= MONSTERSTATE_NONE;

	SetBodygroup( 0, 1 );
	SetBodygroup( 1, 0 );
	SetBodygroup( 2, 1 );
	SetBodygroup( 3, 1 );
	SetBodygroup( 4, 0 );
	SetBodygroup( 5, 1 );

	MonsterInit();
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CDomaHuman :: Precache()
{
	PRECACHE_MODEL("models/doma.mdl");
	PRECACHE_SOUND("doma/doma_fire1.wav");
	PRECACHE_SOUND("doma/doma_fire2.wav");

	UTIL_PrecacheOther( "monster_doma_boss" );

	m_iTrail = PRECACHE_MODEL("sprites/streak.spr");
}	

//=========================================================
// AI Schedules Specific to this monster
//=========================================================
