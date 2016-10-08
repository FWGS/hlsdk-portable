/***
*
*	Copyright (c) 1996-2001, Valve LLC. All rights reserved.
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
#include	"bullsquid.h"

extern int iSquidSpitSprite;

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define		MSQUID_AE_SPIT		( 1 )
#define		MSQUID_AE_BITE		( 2 )
#define		MSQUID_AE_BLINK		( 3 )
#define		MSQUID_AE_TAILWHIP	( 4 )
#define		MSQUID_AE_HOP		( 5 )
#define		MSQUID_AE_THROW		( 6 )

class CMegasquid : public CBullsquid
{
public:
	void Spawn(void);
	void Precache(void);
	void HandleAnimEvent(MonsterEvent_t *pEvent);

	BOOL CheckMeleeAttack1(float flDot, float flDist) { return FALSE; }
	BOOL CheckMeleeAttack2(float flDot, float flDist);
};

LINK_ENTITY_TO_CLASS(monster_th_megasquid, CMegasquid);

//=========================================================
// Spawn
//=========================================================
void CMegasquid::Spawn()
{
	Precache();

	SET_MODEL(ENT(pev), "models/megasquid.mdl");
	UTIL_SetSize(pev, Vector(-160, -160, 0), Vector(160, 160, 256));

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	m_bloodColor = BLOOD_COLOR_RED;
	pev->effects = 0;
	pev->health = gSkillData.megasquidHealth;
	m_flFieldOfView = 0.2;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState = MONSTERSTATE_NONE;

	m_fCanThreatDisplay = FALSE;
	m_flNextSpitTime = gpGlobals->time;

	pev->view_ofs = Vector(0, 0, 128);

	MonsterInit();

	// Remove tail whip attack.
	m_afCapability &= ~bits_CAP_MELEE_ATTACK1;
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CMegasquid::Precache()
{
	CBullsquid::Precache();

	PRECACHE_MODEL("models/megasquid.mdl");
}

//=========================================================
// CheckMeleeAttack2 - bullsquid is a big guy, so has a longer
// melee range than most monsters. This is the bite attack.
// this attack will not be performed if the tailwhip attack
// is valid.
//=========================================================
BOOL CMegasquid::CheckMeleeAttack2(float flDot, float flDist)
{
	if (flDist <= 200 && flDot >= 0.7)		// The player & bullsquid can be as much as their bboxes 
	{										// apart (48 * sqrt(3)) and he can still attack (85 is a little more than 48*sqrt(3))
		return TRUE;
	}

	return FALSE;
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CMegasquid::HandleAnimEvent(MonsterEvent_t *pEvent)
{
	switch (pEvent->event)
	{
	case MSQUID_AE_SPIT:
	{
		Vector	vecSpitOffset;
		Vector	vecSpitDir;

		UTIL_MakeVectors ( pev->angles );

		// !!!HACKHACK - the spot at which the spit originates (in front of the mouth) was measured in 3ds and hardcoded here.
		// we should be able to read the position of bones at runtime for this info.
		vecSpitOffset = ( gpGlobals->v_right * 40 + gpGlobals->v_forward * 185 + gpGlobals->v_up * 115 );		 // 8  37   23
		vecSpitOffset = ( pev->origin + vecSpitOffset );
		vecSpitDir = ( ( m_hEnemy->pev->origin + m_hEnemy->pev->view_ofs ) - vecSpitOffset ).Normalize();

		vecSpitDir.x += RANDOM_FLOAT( -0.05, 0.05 );
		vecSpitDir.y += RANDOM_FLOAT( -0.05, 0.05 );
		vecSpitDir.z += RANDOM_FLOAT( -0.05, 0 );


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

		CSquidSpit::Shoot( pev, vecSpitOffset, vecSpitDir * 900 );
	}
	break;

	case MSQUID_AE_BITE:
	{
		// SOUND HERE!
		CBaseEntity *pHurt = CheckTraceHullAttack(220, gSkillData.bullsquidDmgBite, DMG_SLASH);

		if (pHurt)
		{
			//pHurt->pev->punchangle.z = -15;
			//pHurt->pev->punchangle.x = -45;
			pHurt->pev->velocity = pHurt->pev->velocity - gpGlobals->v_forward * 100;
			pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_up * 100;
		}
	}
	break;

	case MSQUID_AE_THROW:
	{
		int iPitch;

		// squid throws its prey IF the prey is a client. 
		CBaseEntity *pHurt = CheckTraceHullAttack(220, 0, 0);


		if (pHurt)
		{
			// croonchy bite sound
			iPitch = RANDOM_FLOAT(90, 110);
			switch (RANDOM_LONG(0, 1))
			{
			case 0:
				EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, "bullchicken/bc_bite2.wav", 1, ATTN_NORM, 0, iPitch);
				break;
			case 1:
				EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, "bullchicken/bc_bite3.wav", 1, ATTN_NORM, 0, iPitch);
				break;
			}


			//pHurt->pev->punchangle.x = RANDOM_LONG(0,34) - 5;
			//pHurt->pev->punchangle.z = RANDOM_LONG(0,49) - 25;
			//pHurt->pev->punchangle.y = RANDOM_LONG(0,89) - 45;

			// screeshake transforms the viewmodel as well as the viewangle. No problems with seeing the ends of the viewmodels.
			UTIL_ScreenShake(pHurt->pev->origin, 25.0, 1.5, 0.7, 2);

			if (pHurt->IsPlayer())
			{
				UTIL_MakeVectors(pev->angles);
				pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_forward * 300 + gpGlobals->v_up * 300;
			}
		}
	}
	break;

	default:
		CBullsquid::HandleAnimEvent(pEvent);
	}
}