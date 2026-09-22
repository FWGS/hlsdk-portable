/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/
/*

===== combat.cpp ========================================================

  functions dealing with damage infliction & death

*/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "soundent.h"
#include "decals.h"
#include "animation.h"
#include "weapons.h"
#include "func_break.h"
#include "game.h"
#include "player.h"
#include "trains.h"
#include "gamerules.h"
#include "game.h"

#include "pm_materials.h"

extern DLL_GLOBAL Vector		g_vecAttackDir;
extern DLL_GLOBAL int			g_iSkillLevel;
extern DLL_GLOBAL int			g_gibexp_max;
extern DLL_GLOBAL int			g_causality_add;

extern Vector VecBModelOrigin( entvars_t *pevBModel );
extern entvars_t *g_pevLastInflictor;

#define GERMAN_GIB_COUNT		4
#define	HUMAN_GIB_COUNT			6
#define ALIEN_GIB_COUNT			4


// HACKHACK -- The gib velocity equations don't work
void CGib::LimitVelocity( void )
{
	float length = pev->velocity.Length();

	// ceiling at 1500.  The gib velocity equation is not bounded properly.  Rather than tune it
	// in 3 separate places again, I'll just limit it here.
	if( length > 1500.0f )
		pev->velocity = pev->velocity.Normalize() * 1500.0f;		// This should really be sv_maxvelocity * 0.75 or something
}

void RadiusFlash(Vector vecSrc, entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage)
{
	CBaseEntity *pEntity = NULL;
	TraceResult tr;
	float flAdjustedDamage, falloff;
	Vector vecSpot;
	float flRadius = 1500;

	if (flRadius)
		falloff = flDamage / flRadius;
	else
		falloff = 1;

	int bInWater = (UTIL_PointContents(vecSrc) == CONTENTS_WATER);

	vecSrc.z += 1;

	while ((pEntity = UTIL_FindEntityInSphere(pEntity, vecSrc, flRadius)) != NULL)
	{
		TraceResult tr2;
		Vector vecLOS;
		float flDot;
		float fadeTime;
		float fadeHold;
		int alpha;
		CBasePlayer *pPlayer;
		float currentHoldTime;

		if ( !(pEntity->pev->flags & (FL_MONSTER | FL_CLIENT)) )
			break;

		if (pEntity->pev->takedamage == DAMAGE_NO || pEntity->pev->deadflag != DEAD_NO)
			continue;

		if (bInWater && !pEntity->pev->waterlevel)
			continue;

		if (!bInWater && pEntity->pev->waterlevel == 3)
			continue;

		vecSpot = pEntity->BodyTarget(vecSrc);
		UTIL_TraceLine(vecSrc, vecSpot, dont_ignore_monsters, ENT(pevInflictor), &tr);

		if (tr.flFraction == 1 || tr.pHit == pEntity->edict())
		{
			UTIL_TraceLine(vecSpot, vecSrc, dont_ignore_monsters, tr.pHit, &tr2);

			if (tr2.flFraction < 1)
				continue;

			if (tr.fStartSolid)
			{
				tr.vecEndPos = vecSrc;
				tr.flFraction = 0;
			}

			flAdjustedDamage = (vecSrc - tr2.vecEndPos).Length() * falloff;
			flAdjustedDamage = flDamage - flAdjustedDamage;

			if (flAdjustedDamage < 0)
				flAdjustedDamage = 0;

			UTIL_MakeVectors(pEntity->pev->angles);
			vecLOS = vecSrc - pEntity->EarPosition();
			flDot = DotProduct(vecLOS, gpGlobals->v_forward);

			if (flDot >= 0)
			{
				fadeTime = flAdjustedDamage * 3;
				fadeHold = flAdjustedDamage / 2.0;
				alpha = 255;
			}
			else
			{
				fadeTime = flAdjustedDamage * 1.75;
				fadeHold = flAdjustedDamage / 4.0;
				alpha = 188;
			}

			if(pEntity->IsPlayer())
			{
				pPlayer = (CBasePlayer *)pEntity;

				currentHoldTime = pPlayer->m_blindStartTime + pPlayer->m_blindHoldTime - gpGlobals->time;

				if (currentHoldTime > 0 && alpha == 255)
					fadeHold += currentHoldTime;

				if (pPlayer->m_blindStartTime != 0 && pPlayer->m_blindFadeTime != 0)
				{
					if (pPlayer->m_blindStartTime + pPlayer->m_blindFadeTime + pPlayer->m_blindHoldTime > gpGlobals->time)
					{
						if (pPlayer->m_blindFadeTime > fadeTime)
							fadeTime = pPlayer->m_blindFadeTime;

						if (pPlayer->m_blindAlpha > alpha)
							alpha = pPlayer->m_blindAlpha;
					}
				}

				if(pPlayer->m_darkposion == 0){
				UTIL_ScreenFade(pPlayer, Vector(255, 255, 255), fadeTime, fadeHold, alpha, 0);
				}

				currentHoldTime = 1;

				pPlayer->Blind(fadeTime / 3, fadeHold, fadeTime, alpha);
			}
			else
			{
				CBaseMonster *pEnemyMonster;
				pEnemyMonster = pEntity->MyMonsterPointer();
				if(pEnemyMonster)
				{
					if(pEnemyMonster->m_selfmode == FALSE)
					{
						if(flAdjustedDamage >= 10)
						{
							pEnemyMonster->Freeze_Monster(40);
						}
						else if(flAdjustedDamage >= 6)
						{
							pEnemyMonster->Freeze_Monster(30);
						}
						else if(flAdjustedDamage >= 3)
						{
							pEnemyMonster->Freeze_Monster(20);
						}
						else if(flAdjustedDamage > 0)
						{
							pEnemyMonster->Freeze_Monster(10);
						}
					}
				}
			}
		}
	}
}

void CGib::SpawnStickyGibs( entvars_t *pevVictim, Vector vecOrigin, int cGibs )
{
	int i;

	for( i = 0; i < cGibs; i++ )
	{
		CGib *pGib = GetClassPtr( (CGib *)NULL );

		pGib->Spawn( "models/stickygib.mdl" );
		pGib->pev->body = RANDOM_LONG( 0, 2 );

		if( pevVictim )
		{
			pGib->pev->origin.x = vecOrigin.x + RANDOM_FLOAT( -3.0f, 3.0f );
			pGib->pev->origin.y = vecOrigin.y + RANDOM_FLOAT( -3.0f, 3.0f );
			pGib->pev->origin.z = vecOrigin.z + RANDOM_FLOAT( -3.0f, 3.0f );

			/*
			pGib->pev->origin.x = pevVictim->absmin.x + pevVictim->size.x * ( RANDOM_FLOAT( 0, 1 ) );
			pGib->pev->origin.y = pevVictim->absmin.y + pevVictim->size.y * ( RANDOM_FLOAT( 0, 1 ) );
			pGib->pev->origin.z = pevVictim->absmin.z + pevVictim->size.z * ( RANDOM_FLOAT( 0, 1 ) );
			*/

			// make the gib fly away from the attack vector
			pGib->pev->velocity = g_vecAttackDir * -1.0f;

			// mix in some noise
			pGib->pev->velocity.x += RANDOM_FLOAT( -0.15f, 0.15f );
			pGib->pev->velocity.y += RANDOM_FLOAT( -0.15f, 0.15f );
			pGib->pev->velocity.z += RANDOM_FLOAT( -0.15f, 0.15f );

			pGib->pev->velocity = pGib->pev->velocity * 900.0f;

			pGib->pev->avelocity.x = RANDOM_FLOAT( 250.0f, 400.0f );
			pGib->pev->avelocity.y = RANDOM_FLOAT( 250.0f, 400.0f );

			// copy owner's blood color
			pGib->m_bloodColor = ( CBaseEntity::Instance( pevVictim ) )->BloodColor();

			if( pevVictim->health > -150 )
			{
				pGib->pev->velocity = pGib->pev->velocity * 0.7f;
			}
			else if( pevVictim->health > -400 )
			{
				pGib->pev->velocity = pGib->pev->velocity * 2.0f;
			}
			else
			{
				pGib->pev->velocity = pGib->pev->velocity * 4.0f;
			}

			pGib->pev->movetype = MOVETYPE_TOSS;
			pGib->pev->solid = SOLID_BBOX;
			UTIL_SetSize( pGib->pev, Vector( 0, 0, 0 ), Vector( 0, 0, 0 ) );
			pGib->SetTouch( &CGib::StickyGibTouch );
			pGib->SetThink( NULL );
		}
		pGib->LimitVelocity();
	}
}

void CGib::SpawnHeadGib( entvars_t *pevVictim, int body, int fastfade )
{
	CGib *pGib = GetClassPtr( (CGib *)NULL );

	pGib->Spawn( "models/gibs_all.mdl" );// throw one head
	pGib->pev->body = body;

	if( pevVictim )
	{
		pGib->pev->origin = pevVictim->origin + pevVictim->view_ofs;

		edict_t *pentPlayer = FIND_CLIENT_IN_PVS( pGib->edict() );

		if( RANDOM_LONG( 0, 100 ) <= 5 && pentPlayer )
		{
			// 5% chance head will be thrown at player's face.
			entvars_t *pevPlayer;

			pevPlayer = VARS( pentPlayer );
			pGib->pev->velocity = ( ( pevPlayer->origin + pevPlayer->view_ofs ) - pGib->pev->origin ).Normalize() * 300.0f;
			pGib->pev->velocity.z += 100.0f;
		}
		else
		{
			pGib->pev->velocity = Vector( RANDOM_FLOAT( -100.0f, 100.0f ), RANDOM_FLOAT( -100.0f, 100.0f ), RANDOM_FLOAT( 200.0f, 300.0f ) );
			pGib->pev->velocity.z += 50;
		}

		pGib->pev->avelocity.x = RANDOM_FLOAT( 100.0f, 200.0f );
		pGib->pev->avelocity.y = RANDOM_FLOAT( 100.0f, 300.0f );

		// copy owner's blood color
		pGib->m_bloodColor = ( CBaseEntity::Instance( pevVictim ) )->BloodColor();

		/*if( pevVictim->health > -50 )
		{
			pGib->pev->velocity = pGib->pev->velocity * 0.7f;
		}
		else if( pevVictim->health > -200 )
		{
			pGib->pev->velocity = pGib->pev->velocity * 2.0f;
		}
		else
		{
			pGib->pev->velocity = pGib->pev->velocity * 4.0f;
		}*/

		pGib->pev->velocity = pGib->pev->velocity * 1;

		if(fastfade >= 1)
		{
			pGib->pev->armorvalue = 18;
		}
	}
	pGib->LimitVelocity();
}

void CGib::SpawnRandomGibs( entvars_t *pevVictim, int cGibs, int human, int fastfade )
{
	int cSplat;

	if( ( pevVictim->spawnflags & SF_MONSTER_FADECORPSE ) && fastfade == 0 )
		fastfade = 1;

	for( cSplat = 0; cSplat < cGibs; cSplat++ )
	{
		CGib *pGib = GetClassPtr( (CGib *)NULL );

		/*if( human )
		{
			// human pieces
			pGib->Spawn( "models/hgibs.mdl" );
			pGib->pev->body = RANDOM_LONG( 1, HUMAN_GIB_COUNT - 1 );// start at one to avoid throwing random amounts of skulls (0th gib)
		}
		else
		{
			// aliens
			pGib->Spawn( "models/agibs.mdl" );
			pGib->pev->body = RANDOM_LONG( 0, ALIEN_GIB_COUNT - 1 );
		}*/

		pGib->Spawn( "models/gibs_all.mdl" );
		pGib->pev->body = human;

		if( pevVictim )
		{
			// spawn the gib somewhere in the monster's bounding volume
			pGib->pev->origin.x = pevVictim->absmin.x + pevVictim->size.x * ( RANDOM_FLOAT( 0.0f, 1.0f ) );
			pGib->pev->origin.y = pevVictim->absmin.y + pevVictim->size.y * ( RANDOM_FLOAT( 0.0f, 1.0f ) );
			pGib->pev->origin.z = pevVictim->absmin.z + pevVictim->size.z * ( RANDOM_FLOAT( 0.0f, 1.0f ) ) + 1.0f;	// absmin.z is in the floor because the engine subtracts 1 to enlarge the box

			// make the gib fly away from the attack vector
			pGib->pev->velocity = g_vecAttackDir * -1.0f;

			// mix in some noise
			pGib->pev->velocity.x += RANDOM_FLOAT( -0.25f, 0.25f );
			pGib->pev->velocity.y += RANDOM_FLOAT( -0.25f, 0.25f );
			pGib->pev->velocity.z += RANDOM_FLOAT( -0.25f, 0.25f );

			pGib->pev->velocity = pGib->pev->velocity * RANDOM_FLOAT( 300.0f, 400.0f );
			pGib->pev->velocity.z += 50;

			pGib->pev->avelocity.x = RANDOM_FLOAT( 100.0f, 200.0f );
			pGib->pev->avelocity.y = RANDOM_FLOAT( 100.0f, 300.0f );

			// copy owner's blood color
			pGib->m_bloodColor = ( CBaseEntity::Instance( pevVictim ) )->BloodColor();

			/*if( pevVictim->health > -50 )
			{
				pGib->pev->velocity = pGib->pev->velocity * 0.7f;
			}
			else if( pevVictim->health > -200 )
			{
				pGib->pev->velocity = pGib->pev->velocity * 2.0f;
			}
			else
			{
				pGib->pev->velocity = pGib->pev->velocity * 4.0f;
			}*/

			if( FClassnameIs( pevVictim,"hydra_spore_fun" ) )
			{
				pGib->pev->velocity = pGib->pev->velocity * 3;
			}
			else if( FClassnameIs( pevVictim,"monster_bigmomma" ) )
			{
				pGib->pev->velocity = pGib->pev->velocity * 2;
			}
			else
			{
				pGib->pev->velocity = pGib->pev->velocity * 1;
			}

			pGib->pev->solid = SOLID_BBOX;
			UTIL_SetSize( pGib->pev, Vector( 0, 0, 0 ), Vector( 0, 0, 0 ) );

			if( fastfade >= 1 )
				pGib->pev->armorvalue = 20;
		}
		pGib->LimitVelocity();
	}
}

BOOL CBaseMonster::HasHumanGibs( void )
{
	int myClass = Classify();

	if( myClass == CLASS_HUMAN_MILITARY ||
		myClass == CLASS_PLAYER_ALLY ||
		myClass == CLASS_HUMAN_PASSIVE ||
		myClass == CLASS_HUMAN_ASS ||
		myClass == CLASS_PLAYER )

		 return TRUE;

	return FALSE;
}

BOOL CBaseMonster::HasAlienGibs( void )
{
	int myClass = Classify();

	if( myClass == CLASS_ALIEN_MILITARY ||
		myClass == CLASS_ALIEN_MONSTER ||
		myClass == CLASS_ALIEN_PASSIVE ||
		myClass == CLASS_INSECT ||
		myClass == CLASS_ALIEN_PREDATOR ||
		myClass == CLASS_ALIEN_PREY )

		return TRUE;

	return FALSE;
}

void CBaseMonster::FadeMonster( void )
{
	StopAnimation();
	pev->velocity = g_vecZero;
	pev->movetype = MOVETYPE_NONE;
	pev->avelocity = g_vecZero;
	pev->animtime = gpGlobals->time;
	pev->effects |= EF_NOINTERP;
	SUB_StartFadeOut();
}

//=========================================================
// GibMonster - create some gore and get rid of a monster's
// model.
//=========================================================
void CBaseMonster::GibMonster( void )
{
	TraceResult	tr;
	BOOL		gibbed = FALSE;

	EMIT_SOUND( ENT( pev ), CHAN_BODY, "common/bodysplat.wav", 1, ATTN_NORM );
	m_gibed = 1;

	// only humans throw skulls !!!UNDONE - eventually monsters will have their own sets of gibs
	if( HasHumanGibs() )
	{
		/*if( CVAR_GET_FLOAT( "violence_hgibs" ) != 0 )	// Only the player will ever get here
		{
			CGib::SpawnHeadGib( pev );
			CGib::SpawnRandomGibs( pev, 4, 1 );	// throw some human gibs.
		}*/

		if(g_gibexp_max < 100)
		{
			if ( FClassnameIs(pev,"monster_revenant") )
			{
				CGib::SpawnRandomGibs( pev, 1, 21,m_diefadeout );
				CGib::SpawnRandomGibs( pev, 1, 21,m_diefadeout );
				g_gibexp_max += 1;
			}
			else if ( FClassnameIs(pev,"monster_thrower") )
			{
				CGib::SpawnRandomGibs( pev, 1, 22,m_diefadeout );
			}
			else if ( FClassnameIs(pev,"monster_bloodsucker") )
			{
				CGib::SpawnRandomGibs( pev, 1, 20,m_diefadeout );
			}
			else if ( FClassnameIs(pev,"monster_unknow_melon") )
			{
				CGib::SpawnRandomGibs( pev, 1, 19,m_diefadeout );
			}
			else
			{
				CGib::SpawnHeadGib( pev,0,m_diefadeout );
			}
			
			CGib::SpawnRandomGibs( pev, 1, 1,m_diefadeout );
			CGib::SpawnRandomGibs( pev, 1, 2,m_diefadeout );
			CGib::SpawnRandomGibs( pev, 1, 3,m_diefadeout );
			CGib::SpawnRandomGibs( pev, 1, 4,m_diefadeout );

			if ( FClassnameIs(pev,"monster_human_grunt") 
			|| FClassnameIs(pev,"monster_human_grunt_medic")
			|| FClassnameIs(pev,"monster_human_grunt_torch") 
			|| FClassnameIs(pev,"monster_human_assault")
			|| FClassnameIs(pev,"monster_human_grunt_shape"))
			{
				CGib::SpawnRandomGibs( pev, 1, 17,m_diefadeout );
			}
			else if ( FClassnameIs(pev,"monster_majo") )
			{
				CGib::SpawnRandomGibs( pev, 1, 18,m_diefadeout );
			}
			else
			{
				CGib::SpawnRandomGibs( pev, 1, 5,m_diefadeout );
			}

			g_gibexp_max += 6;

			SpawnBlood(Center(), BloodColor(), 150);
		}
		gibbed = TRUE;
	}
	else if( HasAlienGibs() )
	{
		/*if( CVAR_GET_FLOAT( "violence_agibs" ) != 0 )	// Should never get here, but someone might call it directly
		{
			CGib::SpawnRandomGibs( pev, 4, 0 );	// Throw alien gibs
		}*/

		if(g_gibexp_max < 100)
		{
			if ( !FClassnameIs(pev,"monster_headcrab") && !FClassnameIs(pev,"monster_houndeye"))
			{
				CGib::SpawnHeadGib( pev,6,m_diefadeout );
				g_gibexp_max += 1;
			}
			
			if ( !FClassnameIs(pev,"monster_headcrab"))
			{
				CGib::SpawnRandomGibs( pev, 1, 7,m_diefadeout );
				g_gibexp_max += 1;
			}
			
			if ( FClassnameIs(pev,"monster_bigmomma"))
			{
				CGib::SpawnRandomGibs( pev, 1, 13,m_diefadeout );
				CGib::SpawnRandomGibs( pev, 1, 13,m_diefadeout );
				CGib::SpawnRandomGibs( pev, 1, 13,m_diefadeout );
				CGib::SpawnRandomGibs( pev, 1, 13,m_diefadeout );
				CGib::SpawnRandomGibs( pev, 1, 14,m_diefadeout );
				CGib::SpawnRandomGibs( pev, 1, 15,m_diefadeout );
				g_gibexp_max += 6;
			}
			
			if ( FClassnameIs(pev,"hydra_spore_fun"))
			{
				CGib::SpawnRandomGibs( pev, 1, 16,m_diefadeout );
				CGib::SpawnRandomGibs( pev, 1, 16,m_diefadeout );
				CGib::SpawnRandomGibs( pev, 1, 16,m_diefadeout );
				CGib::SpawnRandomGibs( pev, 1, 16,m_diefadeout );
				CGib::SpawnRandomGibs( pev, 1, 16,m_diefadeout );
				g_gibexp_max += 5;
			}

			CGib::SpawnRandomGibs( pev, 1, 8,m_diefadeout );
			CGib::SpawnRandomGibs( pev, 1, 9,m_diefadeout );
			CGib::SpawnRandomGibs( pev, 1, 10,m_diefadeout );
			CGib::SpawnRandomGibs( pev, 1, 11,m_diefadeout );
			g_gibexp_max += 4;

			SpawnBlood(Center(), BloodColor(), 150);
		}
		gibbed = TRUE;
	}

	if( !IsPlayer() )
	{
		if( gibbed )
		{
			if(pev->impulse != 810)
			{
				// don't remove players!
				SetThink( &CBaseEntity::SUB_Remove );
				pev->nextthink = gpGlobals->time;
			}
		}
		else
		{
			FadeMonster();
		}
	}
}

//=========================================================
// GetDeathActivity - determines the best type of death
// anim to play.
//=========================================================
Activity CBaseMonster::GetDeathActivity( void )
{
	Activity	deathActivity;
	BOOL		fTriedDirection;
	float		flDot;
	TraceResult	tr;
	Vector		vecSrc;

	if( pev->deadflag != DEAD_NO )
	{
		// don't run this while dying.
		return m_IdealActivity;
	}

	vecSrc = Center();

	fTriedDirection = FALSE;
	deathActivity = ACT_DIESIMPLE;// in case we can't find any special deaths to do.

	UTIL_MakeVectors( pev->angles );
	flDot = DotProduct( gpGlobals->v_forward, g_vecAttackDir * -1.0f );

	switch( m_LastHitGroup )
	{
		// try to pick a region-specific death.
	case HITGROUP_HEAD:
		deathActivity = ACT_DIE_HEADSHOT;
		break;
	case HITGROUP_STOMACH:
		deathActivity = ACT_DIE_GUTSHOT;
		break;
	case HITGROUP_GENERIC:
		// try to pick a death based on attack direction
		fTriedDirection = TRUE;
		if( flDot > 0.3f )
		{
			deathActivity = ACT_DIEFORWARD;
		}
		else if( flDot <= -0.3f )
		{
			deathActivity = ACT_DIEBACKWARD;
		}
		break;
	default:
		// try to pick a death based on attack direction
		fTriedDirection = TRUE;

		if( flDot > 0.3f )
		{
			deathActivity = ACT_DIEFORWARD;
		}
		else if( flDot <= -0.3f )
		{
			deathActivity = ACT_DIEBACKWARD;
		}
		break;
	}

	// can we perform the prescribed death?
	if( LookupActivity( deathActivity ) == ACTIVITY_NOT_AVAILABLE )
	{
		// no! did we fail to perform a directional death? 
		if( fTriedDirection )
		{
			// if yes, we're out of options. Go simple.
			deathActivity = ACT_DIESIMPLE;
		}
		else
		{
			// cannot perform the ideal region-specific death, so try a direction.
			if( flDot > 0.3f )
			{
				deathActivity = ACT_DIEFORWARD;
			}
			else if( flDot <= -0.3f )
			{
				deathActivity = ACT_DIEBACKWARD;
			}
		}
	}

	if( LookupActivity( deathActivity ) == ACTIVITY_NOT_AVAILABLE )
	{
		// if we're still invalid, simple is our only option.
		deathActivity = ACT_DIESIMPLE;
	}

	if( deathActivity == ACT_DIEFORWARD )
	{
		// make sure there's room to fall forward
		UTIL_TraceHull( vecSrc, vecSrc + gpGlobals->v_forward * 64.0f, dont_ignore_monsters, head_hull, edict(), &tr );

		if( tr.flFraction != 1.0f )
		{
			deathActivity = ACT_DIESIMPLE;
		}
	}

	if( deathActivity == ACT_DIEBACKWARD )
	{
		// make sure there's room to fall backward
		UTIL_TraceHull( vecSrc, vecSrc - gpGlobals->v_forward * 64.0f, dont_ignore_monsters, head_hull, edict(), &tr );

		if( tr.flFraction != 1.0f )
		{
			deathActivity = ACT_DIESIMPLE;
		}
	}

	if(m_killbyheadcrab >= 1)
	{
		deathActivity = ACT_FALL;
	}
	if ( FClassnameIs(pev,"monster_misaliya") && pev->frags == 1 )
	{
		deathActivity = ACT_DIEVIOLENT;
	}
	if ( FClassnameIs(pev,"monster_zombie_soldier2") )
	{
		deathActivity = ACT_DIEVIOLENT;
	}
	if ( FClassnameIs(pev,"monster_majo") && pev->body == 1 )
	{
		deathActivity = ACT_DIEVIOLENT;
	}
	if ( FClassnameIs(pev,"monster_dengor") && m_crouchmode == 1 )
	{
		deathActivity = ACT_DIEVIOLENT;
	}
	if ( FClassnameIs(pev,"monster_tyant_boss") && pev->weapons == 1 )
	{
		deathActivity = ACT_DIEVIOLENT;
	}
	if ( FClassnameIs(pev,"monster_cof_ms4") )
	{
		if(pev->frags == 8)
		{
			deathActivity = ACT_CROUCH;
		}
		else if(m_crouchmode == 1)
		{
			deathActivity = ACT_DIE_GUTSHOT;
		}
		else
		{
			if ( IsMoving() )
			{
				deathActivity = ACT_DIESIMPLE;
			}
			else
			{
				deathActivity = ACT_DIE_HEADSHOT;
			}
		}
	}

	return deathActivity;
}

//=========================================================
// GetSmallFlinchActivity - determines the best type of flinch
// anim to play.
//=========================================================
Activity CBaseMonster::GetSmallFlinchActivity( void )
{
	Activity	flinchActivity;
	// BOOL		fTriedDirection;
	//float		flDot;

	// fTriedDirection = FALSE;
	UTIL_MakeVectors( pev->angles );
	//flDot = DotProduct( gpGlobals->v_forward, g_vecAttackDir * -1.0f );

	switch( m_LastHitGroup )
	{
		// pick a region-specific flinch
	case HITGROUP_HEAD:
		flinchActivity = ACT_FLINCH_HEAD;
		break;
	case HITGROUP_STOMACH:
		flinchActivity = ACT_FLINCH_STOMACH;
		break;
	case HITGROUP_LEFTARM:
		flinchActivity = ACT_FLINCH_LEFTARM;
		break;
	case HITGROUP_RIGHTARM:
		flinchActivity = ACT_FLINCH_RIGHTARM;
		break;
	case HITGROUP_LEFTLEG:
		flinchActivity = ACT_FLINCH_LEFTLEG;
		break;
	case HITGROUP_RIGHTLEG:
		flinchActivity = ACT_FLINCH_RIGHTLEG;
		break;
	case HITGROUP_GENERIC:
	default:
		// just get a generic flinch.
		flinchActivity = ACT_SMALL_FLINCH;
		break;
	}

	// do we have a sequence for the ideal activity?
	if( LookupActivity( flinchActivity ) == ACTIVITY_NOT_AVAILABLE )
	{
		flinchActivity = ACT_SMALL_FLINCH;
	}

	return flinchActivity;
}

void CBaseMonster::DeadTouch( CBaseEntity *pOther )
{
	if(pev->deadflag == DEAD_NO)
	{
		// Did the player touch me?
		if ( m_new_ally_type && pOther->IsPlayer() )
		{
			// Ignore if pissed at player
			if ( m_groundElev || pev->takedamage == DAMAGE_NO || m_lovehate <= 0 || m_MonsterState == MONSTERSTATE_SCRIPT || m_IdealMonsterState == MONSTERSTATE_SCRIPT || IsMoving() || pev->gravity >= 1.6 || m_selfmode == TRUE)
				return;

			Vector vecSrc, vecDest;
			vecDest = (pev->origin - pOther->pev->origin);
			vecDest.z = pev->origin.z - pev->origin.z;
			vecDest = vecDest.Normalize() * 50;
			pev->velocity = pev->velocity + vecDest;
			vecSrc = pev->origin;
			vecDest = vecSrc + (pev->velocity.Normalize() * 1000);
			MakeIdealYaw( pOther->pev->origin );
		}	
	}
}

void CBaseMonster::BecomeDead( void )
{
	pev->takedamage = DAMAGE_YES;// don't let autoaim aim at corpses.

	// give the corpse half of the monster's original maximum health. 
	//pev->health = pev->max_health / 2;
	//pev->max_health = 5; // max_health now becomes a counter for how many blood decals the corpse can place.

	// make the corpse fly away from the attack vector
	if(m_candrownwater != 2)
		pev->movetype = MOVETYPE_TOSS;
	if (corpsephysics.value &&
			// affect only dying monsters, not initially dead ones
			m_IdealMonsterState == MONSTERSTATE_DEAD)
	{
		pev->flags &= ~FL_ONGROUND;
		pev->origin.z += 2.0f;
		pev->velocity = g_vecAttackDir * -1.0f;
		pev->velocity = pev->velocity * RANDOM_FLOAT( 300.0f, 400.0f );
	}

	if(m_new_ally_type)
		SetTouch( &CBaseMonster::DeadTouch );

	m_die = 1;
}

BOOL CBaseMonster::ShouldGibMonster( int iGib )
{
	/*if( ( iGib == GIB_NORMAL && pev->health < GIB_HEALTH_VALUE ) || ( iGib == GIB_ALWAYS ) )
		return TRUE;*/

	if( iGib == GIB_NEVER || m_nevergibmode == TRUE )
		return FALSE;

	if( iGib == GIB_ALWAYS )
		return TRUE;

	float limip = -pev->max_health * 0.6;

	if(limip > -60)
		limip = -60;

	if ( pev->health <= limip )
		return TRUE;

	return FALSE;
}

void CBaseMonster::CallGibMonster( void )
{
	BOOL fade = FALSE;

	/*if( HasHumanGibs() )
	{
		if( CVAR_GET_FLOAT( "violence_hgibs" ) == 0.0f )
			fade = TRUE;
	}
	else if( HasAlienGibs() )
	{
		if( CVAR_GET_FLOAT( "violence_agibs" ) == 0.0f )
			fade = TRUE;
	}

	pev->takedamage = DAMAGE_NO;
	pev->solid = SOLID_NOT;// do something with the body. while monster blows up

	if( fade )
	{
		FadeMonster();
	}
	else
	{
		pev->effects = EF_NODRAW; // make the model invisible.
		GibMonster();
	}

	pev->deadflag = DEAD_DEAD;
	FCheckAITrigger();

	// don't let the status bar glitch for players.with <0 health.
	if( pev->health < -99 )
	{
		pev->health = 0;
	}

	if( ShouldFadeOnDeath() && !fade )
		UTIL_Remove( this );*/
	
	SetThink ( NULL );
	SetTouch ( NULL );

	pev->velocity = g_vecZero;
	pev->movetype = MOVETYPE_NONE;
	pev->avelocity = g_vecZero;

	pev->takedamage = DAMAGE_NO;
	pev->solid = SOLID_NOT;// do something with the body. while monster blows up

	pev->effects = EF_NODRAW; // make the model invisible.
	GibMonster();

	pev->deadflag = DEAD_DEAD;
	FCheckAITrigger();
}

/*
============
Killed
============
*/
void CBaseMonster::Killed( entvars_t *pevAttacker, int iGib )
{
	//unsigned int	cCount = 0;
	//BOOL		fDone = FALSE;

	/*if( HasMemory( bits_MEMORY_KILLED ) )
	{
		if( ShouldGibMonster( iGib ) )
			CallGibMonster();
		return;
	}*/

	if(m_die == 0)
	{
		CBaseEntity *pEntity = GetClassPtr((CBaseEntity *)pevAttacker);
		if ( (pevAttacker->flags & FL_CLIENT) )
		{
			if (pevAttacker)
			{
				CBasePlayer *pPlayer = GetClassPtr((CBasePlayer *)pevAttacker);
				if(pPlayer)
				{
					if(pev->impulse == 278)
					{
						m_killed_exp = 1;
						pPlayer->TeamMate_expadd(NULL,this);
						char text[256];
						sprintf( text, "Hits: %d\n",pPlayer->m_enemy_kills);
						UTIL_CenterPrintAll( text );
						pPlayer->TakeHealth(10, DMG_GENERIC);
					}
					else
					{
						if(m_killed_exp >= 1)
						{
							if(m_killed_exp >= 10)
							{
								pPlayer->TeamMate_expadd(NULL,this);
							}
							g_pGameRules->DeathNotice( this, pevAttacker, g_pevLastInflictor,m_killedbydmg );
						}
						if(Classify() == CLASS_PLAYER_ALLY)
						{
							pPlayer->m_ending_frags -= 3;
						}
					}
				}
			}
		}
		else if ( (pevAttacker->flags & FL_MONSTER) && pEntity->Classify() == CLASS_PLAYER_ALLY )
		{
			if(m_killed_exp >= 1)
			{
				g_pGameRules->DeathNotice( this, pevAttacker, g_pevLastInflictor,m_killedbydmg );
				CBaseMonster *pEnemyMonster;
				pEnemyMonster = pEntity->MyMonsterPointer();
				if(pEnemyMonster)
				{
					if(pEnemyMonster->m_hPlayer != NULL)
					{
						if(pEnemyMonster->m_rpgms_inteam >= 1 && pEnemyMonster->m_rpgms_inteam <= 4)
						{
							CBasePlayer *pPlayer = GetClassPtr((CBasePlayer *)pEnemyMonster->m_hPlayer->pev);
							if(pPlayer)
							{
								if(m_killed_exp >= 10)
								{
									pPlayer->TeamMate_expadd(pEnemyMonster,this);
								}
							}
						}
					}
				}
			}
		}
		else if ( pEntity->Classify() == CLASS_VEHICLE )
		{
			CBaseEntity *pDriver = ((CFuncVehicle *)pEntity)->m_pDriver;
			if (pDriver)
			{
				pevAttacker = pDriver->pev;
				if (pevAttacker)
				{
					CBasePlayer *pPlayer = GetClassPtr((CBasePlayer *)pevAttacker);	
					if(m_killed_exp >= 1)
					{
						if(m_killed_exp >= 10)
						{
							pPlayer->TeamMate_expadd(NULL,this);
						}
						g_pGameRules->DeathNotice( this, pevAttacker, g_pevLastInflictor,m_killedbydmg );
					}
				}
			}
		}
		else
		{
			if(Classify() == CLASS_PLAYER_ALLY && m_rpgms_inteam >= 1)
			{
				g_pGameRules->DeathNotice( this, pevAttacker, g_pevLastInflictor,m_killedbydmg );
			}
			else if( m_killed_exp >= 1 && !FNullEnt(FIND_CLIENT_IN_PVS(edict())) )
			{
				g_pGameRules->DeathNotice( this, pevAttacker, g_pevLastInflictor,m_killedbydmg );
			}
		}

	}

	if ( ShouldGibMonster( iGib ) && m_gibed == 0 )
	{
		CallGibMonster();
	}
	else if(m_die == 0)
	{
		if(m_grenadekilled == 1)
		{
			pev->velocity = m_MSblastvec * (1 / m_MSblastvec.Length()) * 250;
			pev->velocity.z = 100;
		}
		else if(m_grenadekilled == 2)
		{
			pev->velocity = m_MSblastvec * (1 / m_MSblastvec.Length()) * 300;
			pev->velocity.z = 150;
		}
		else if(m_grenadekilled == 3)
		{
			pev->velocity = m_MSblastvec * (1 / m_MSblastvec.Length()) * 400;
			pev->velocity.z = 200;
		}
		else if(m_grenadekilled == 4)
		{
			pev->velocity = m_MSblastvec * (1 / m_MSblastvec.Length()) * 500;
			pev->velocity.z = 250;
		}
		else if(m_grenadekilled == 5)
		{
			pev->velocity = m_MSblastvec * (1 / m_MSblastvec.Length()) * 600;
			pev->velocity.z = 300;
		}
		else if(m_grenadekilled == 6)
		{
			pev->velocity = m_MSblastvec * (1 / m_MSblastvec.Length()) * 800;
			pev->velocity.z = 400;
		}
		else if(m_grenadekilled == 7)
		{
			pev->velocity = m_MSblastvec * (1 / m_MSblastvec.Length()) * 1000;
			pev->velocity.z = 500;
		}
	
		if(m_grenadekilled >= 1)
		{
			pev->flags &= ~FL_ONGROUND;
		}
        
		m_grenadekilled = 0;

		if(pev->health < -pev->max_health)
		{
			pev->health = -pev->max_health;
		}
	}
	else
	{
		Remember( bits_MEMORY_KILLED );
		m_IdealMonsterState = MONSTERSTATE_DEAD;
		return;
	}

	m_die += 1;

	Remember( bits_MEMORY_KILLED );

	// clear the deceased's sound channels.(may have been firing or reloading when killed)
	EMIT_SOUND( ENT( pev ), CHAN_WEAPON, "common/null.wav", 1, ATTN_NORM );
	m_IdealMonsterState = MONSTERSTATE_DEAD;
	// Make sure this condition is fired too (TakeDamage breaks out before this happens on death)
	SetConditions( bits_COND_LIGHT_DAMAGE );

	// tell owner ( if any ) that we're dead.This is mostly for MonsterMaker functionality.
	CBaseEntity *pOwner = CBaseEntity::Instance( pev->owner );
	if( pOwner )
	{
		pOwner->DeathNotice( pev );
	}

	/*if( ShouldGibMonster( iGib ) )
	{
		CallGibMonster();
		return;
	}
	else if( pev->flags & FL_MONSTER )
	{
		SetTouch( NULL );
		BecomeDead();
	}*/

	if ( pev->flags & FL_MONSTER )
	{
		//SetTouch( NULL );
		BecomeDead();
	}

	// don't let the status bar glitch for players.with <0 health.
	if( pev->health < -99 )
	{
		pev->health = 0;
	}

	//pev->enemy = ENT( pevAttacker );//why? (sjb)

	m_IdealMonsterState = MONSTERSTATE_DEAD;
}

//
// fade out - slowly fades a entity out, then removes it.
//
// DON'T USE ME FOR GIBS AND STUFF IN MULTIPLAYER! 
// SET A FUTURE THINK AND A RENDERMODE!!
void CBaseEntity::SUB_StartFadeOut( void )
{
	if( pev->rendermode == kRenderNormal )
	{
		pev->renderamt = 255;
		pev->rendermode = kRenderTransTexture;
	}

	pev->solid = SOLID_NOT;
	pev->avelocity = g_vecZero;

	pev->nextthink = gpGlobals->time + 0.1f;
	SetThink( &CBaseEntity::SUB_FadeOut );
}

void CBaseEntity::SUB_FadeOut( void )
{
	if( pev->renderamt > 7 )
	{
		pev->renderamt -= 7;
		pev->nextthink = gpGlobals->time + 0.1f;
	}
	else 
	{
		pev->renderamt = 0;
		pev->nextthink = gpGlobals->time + 0.2f;
		SetThink( &CBaseEntity::SUB_Remove );
	}
}

void CBaseEntity :: SUB_StartFadeOut2 ( void )
{
	pev->nextthink = gpGlobals->time + 0.1f;
	SetThink ( &CBaseEntity::SUB_FadeOut2 );
}

void CBaseEntity :: SUB_FadeOut2 ( void  )
{
	if (pev->rendermode == kRenderNormal)
	{
		pev->renderamt = 255;
		pev->rendermode = kRenderTransTexture;
	}

	if ( pev->renderamt > 3 )
	{
		pev->renderamt -= 3;
		pev->nextthink = gpGlobals->time + 0.05f;
	}
	else 
	{
		pev->renderamt = 0;
		pev->nextthink = gpGlobals->time + 0.1f;
		SetThink ( &CBaseEntity::SUB_Remove );
	}
}

void CBaseEntity :: SUB_StartFadeOut3 ( void )
{
	pev->nextthink = gpGlobals->time + 0.1;
	SetThink ( &CBaseEntity::SUB_FadeOut3 );
}

void CBaseEntity :: SUB_FadeOut3 ( void  )
{
	if (pev->rendermode == kRenderNormal)
	{
		pev->renderamt = 255;
		pev->rendermode = kRenderTransTexture;
	}

	if ( pev->renderamt > 3 )
	{
		pev->renderamt -= 3;
		pev->origin.z -= 0.3f;
		pev->nextthink = gpGlobals->time + 0.05f;
	}
	else 
	{
		pev->nextthink = gpGlobals->time + 0.1f;
		SetThink ( &CBaseEntity::SUB_Remove );
	}
}

//=========================================================
// WaitTillLand - in order to emit their meaty scent from
// the proper location, gibs should wait until they stop 
// bouncing to emit their scent. That's what this function
// does.
//=========================================================
void CGib::WaitTillLand( void )
{
	if( !IsInWorld() )
	{
		UTIL_Remove( this );
		return;
	}

	if( pev->armorvalue >= 32 )
	{
		pev->armorvalue = 0;
		SetThink( &CBaseEntity::SUB_StartFadeOut );
		pev->nextthink = gpGlobals->time + 0.1f;

		// If you bleed, you stink!
		/*if( m_bloodColor != DONT_BLEED )
		{
			// ok, start stinkin!
			CSoundEnt::InsertSound( bits_SOUND_MEAT, pev->origin, 384, 25 );
		}*/
	}
	else
	{
		// wait and check again in another half second.
		pev->armorvalue += 1;
		pev->nextthink = gpGlobals->time + 0.5f;
	}
}

//
// Gib bounces on the ground or wall, sponges some blood down, too!
//
void CGib::BounceGibTouch( CBaseEntity *pOther )
{
	Vector	vecSpot;
	TraceResult	tr;

	//if( RANDOM_LONG( 0, 1 ) )
	//	return;// don't bleed everytime

	TraceResult trsky;
	UTIL_TraceLine ( pev->origin, pev->origin + Vector ( 0, 0, -2 ),  ignore_monsters, ENT(pev), &trsky);
	if ( UTIL_PointContents(trsky.vecEndPos) == CONTENT_SKY || UTIL_PointContents(pev->origin) == CONTENT_SKY )
	{
		SetTouch( NULL );
		SetThink ( &CBaseEntity::SUB_Remove );
		pev->nextthink = gpGlobals->time;
		return;
	}

	if( pev->flags & FL_ONGROUND )
	{
		pev->velocity = pev->velocity * 0.9f;
		pev->angles.x = 0.0f;
		pev->angles.z = 0.0f;
		pev->avelocity.x = 0.0f;
		pev->avelocity.z = 0.0f;
	}
	else
	{
		if( m_cBloodDecals > 0 && m_bloodColor != DONT_BLEED )
		{
			vecSpot = pev->origin + Vector( 0.0f, 0.0f, 8.0f );//move up a bit, and trace down.
			UTIL_TraceLine( vecSpot, vecSpot + Vector( 0.0f, 0.0f, -24.0f ), ignore_monsters, ENT( pev ), &tr );

			UTIL_BloodDecalTrace( &tr, m_bloodColor );

			m_cBloodDecals--; 
		}

		if( m_material != matNone && RANDOM_LONG( 0, 2 ) == 0 )
		{
			float volume;
			float zvel = fabs( pev->velocity.z );

			volume = 0.8f * Q_min( 1.0f, zvel / 450.0f );

			CBreakable::MaterialSoundRandom( edict(), (Materials)m_material, volume );
		}
	}
}

//
// Sticky gib puts blood on the wall and stays put. 
//
void CGib::StickyGibTouch( CBaseEntity *pOther )
{
	Vector	vecSpot;
	TraceResult	tr;

	SetThink( &CBaseEntity::SUB_Remove_fx );
	pev->nextthink = gpGlobals->time + 10.0f;

	if( !FClassnameIs( pOther->pev, "worldspawn" ) )
	{
		pev->nextthink = gpGlobals->time;
		return;
	}

	UTIL_TraceLine( pev->origin, pev->origin + pev->velocity * 32.0f,  ignore_monsters, ENT( pev ), &tr );

	UTIL_BloodDecalTrace( &tr, m_bloodColor );

	pev->velocity = tr.vecPlaneNormal * -1.0f;
	pev->angles = UTIL_VecToAngles( pev->velocity );
	pev->velocity = g_vecZero;
	pev->avelocity = g_vecZero;
	pev->movetype = MOVETYPE_NONE;
}

//
// Throw a chunk
//
void CGib::Spawn( const char *szGibModel )
{
	pev->movetype = MOVETYPE_BOUNCE;
	pev->friction = 0.55f; // deading the bounce a bit

	// sometimes an entity inherits the edict from a former piece of glass,
	// and will spawn using the same render FX or rendermode! bad!
	pev->renderamt = 255;
	pev->rendermode = kRenderNormal;
	pev->renderfx = kRenderFxNone;
	pev->solid = SOLID_SLIDEBOX;/// hopefully this will fix the VELOCITY TOO LOW crap
	pev->classname = MAKE_STRING( "gib" );

	SET_MODEL( ENT( pev ), szGibModel );
	UTIL_SetSize( pev, Vector( 0, 0, 0 ), Vector( 0, 0, 0 ) );

	pev->nextthink = gpGlobals->time + 4.0f;
	m_lifeTime = 25;
	SetThink( &CGib::WaitTillLand );
	SetTouch( &CGib::BounceGibTouch );

	m_material = matNone;
	m_cBloodDecals = 4;// how many blood decals this gib can place (1 per bounce until none remain). 
}

// take health
int CBaseMonster::TakeHealth( float flHealth, int bitsDamageType )
{
	/*if( !pev->takedamage )
		return 0;*/

	// clear out any damage types we healed.
	// UNDONE: generic health should not heal any
	// UNDONE: time-based damage

	m_bitsDamageType &= ~( bitsDamageType & ~DMG_TIMEBASED );

	return CBaseEntity::TakeHealth( flHealth, bitsDamageType );
}

/*
============
TakeDamage

The damage is coming from inflictor, but get mad at attacker
This should be the only function that ever reduces health.
bitsDamageType indicates the type of damage sustained, ie: DMG_SHOCK

Time-based damage: only occurs while the monster is within the trigger_hurt.
When a monster is poisoned via an arrow etc it takes all the poison damage at once.

GLOBALS ASSUMED SET:  g_iSkillLevel
============
*/
void MovetoTarget_darkhole( entvars_t *pevFucker, Vector vecTarget,float power )
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

	if (flSpeed > 1000)
	{
		m_vecIdeal = m_vecIdeal.Normalize( ) * 1000;
	}

	m_vecIdeal = m_vecIdeal + (vecTarget - pevFucker->origin).Normalize() * power;

	pevFucker->velocity = m_vecIdeal;
}

int CBaseMonster::TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	float	flTake;
	Vector	vecDir;

	if( !pev->takedamage || m_godmode == TRUE || m_playerguardian_mode >= 1 )
		return 0;
	
	if( m_flGodTime > gpGlobals->time )
		return 0;

	if( !IsAlive() )
	{
		return DeadTakeDamage( pevInflictor, pevAttacker, flDamage, bitsDamageType );
	}

	CBaseEntity *pEntity = NULL;

	if ( (pev->flags & FL_MONSTER) && pev->deadflag == DEAD_NO)
	{

		if(pevAttacker)
		{
			if( (pevAttacker->flags & FL_CLIENT) )
			{
				CBasePlayer *pPlayer = GetClassPtr((CBasePlayer *)pevAttacker);
				if(pPlayer->m_fequip6 == TRUE)
				{
					flDamage *= 2.0;
				}

				if(pPlayer->m_skill_reload && g_causality_add > 0)
				{
					flDamage *= 1.0 + (0.4 * g_causality_add);
				}

				if(pPlayer->m_skill_punch && (m_cleardally_enemy_long == 0 || pPlayer->m_skill_darkhide_on) )
				{
					flDamage *= 1.5;
				}
			}

			if(bitsDamageType == DMG_VALVE_SWORD)
			{
				Freeze_Monster(70);
			}
			else if(bitsDamageType & DMG_UNKNOWBLAST)
			{
				Freeze_Monster(40);
			}
			else if( (bitsDamageType & DMG_CONCUSSION) || (bitsDamageType & DMG_DARK) )
			{
				Freeze_Monster(20);
			}

			pEntity = GetClassPtr((CBaseEntity *)pevAttacker);
						
			if ( pEntity->Classify() == Classify() && Classify() != CLASS_NONE )
			{
				return 0;
			}
			else if ( (pEntity->pev->flags & FL_CLIENT) )
			{
				if(m_rpgms_inteam > 0)
				{
					return 0;
				}
				else if(Classify() == CLASS_PLAYER_ALLY)
				{
					if(m_lovehate > 0)
					{
						int dmg = (int)flDamage;

						if(bitsDamageType & DMG_CLUB)
							dmg += 5;
										
						if ( m_hEnemy == NULL )
						{
							dmg *= 3.0;
						}					
						else if(m_hEnemy->IsPlayer())
						{
							dmg *= 3.0;				
						}
						else if(dmg >= 5)
						{
							dmg -= 5;
						}
						else if(m_MonsterState == MONSTERSTATE_PRONE || m_IdealMonsterState == MONSTERSTATE_PRONE)
						{
							dmg *= 0.5;
						}

						if(dmg > 60)
							dmg = 60;
							
						if(dmg > 0)
							m_lovehate -= dmg;
										
					}
				}
			}
		}

		if( bitsDamageType & DMG_BLAST )
		{
			flDamage *= 2.0;
		}
		else if( bitsDamageType & DMG_ENERGYBLAST)
		{
			flDamage *= 1.5;
		}
	}

	if( pev->deadflag == DEAD_NO )
	{
		// no pain sound during death animation.
		if(flDamage > 0)
			PainSound();// "Ouch!"
	}

	//!!!LATER - make armor consideration here!
	flTake = flDamage;

	if( (pev->flags & FL_MONSTER) )
	{

		if(m_hPortecter != NULL && flDamage > 0 && m_playerguardian_mode == 0)
		{
			if(m_hPortecter->pev->deadflag == DEAD_NO && m_hPortecter->pev->weapons > 0)
			{
				m_hPortecter->TakeDamage ( pevInflictor, pevAttacker, flDamage, bitsDamageType);
				return 0;
			}
			else
			{
				m_hPortecter = NULL;
			}
		}

		m_flNPC_Pain += flTake;

		if ( (pevAttacker->flags & FL_CLIENT) )
		{
			if(pev->health < flTake)
			{
				m_flPlayerDamage_exp += pev->health;
			}
			else
			{
				m_flPlayerDamage_exp += flTake;
				m_flPlayerDamage_hate += flTake;
			}
		}
		else if ( pEntity && (pEntity->pev->flags & FL_MONSTER) )
		{
			CBaseMonster *pEnemyMonster;
			pEnemyMonster = pEntity->MyMonsterPointer();
			if(pEnemyMonster)
			{
				pEnemyMonster->m_hHitEnemy = this;
				if(pEnemyMonster->m_rpgms_inteam == 1)
				{
					m_flPlayerTeamMateDamage_exp1 += flTake;
				}
				else if(pEnemyMonster->m_rpgms_inteam == 2)
				{
					m_flPlayerTeamMateDamage_exp2 += flTake;
				}
				else if(pEnemyMonster->m_rpgms_inteam == 3)
				{
					m_flPlayerTeamMateDamage_exp3 += flTake;
				}
				else if(pEnemyMonster->m_rpgms_inteam == 4)
				{
					m_flPlayerTeamMateDamage_exp4 += flTake;
				}
			}
		}
	}

	// set damage type sustained
	m_bitsDamageType |= bitsDamageType;

	// grab the vector of the incoming attack. ( pretend that the inflictor is a little lower than it really is, so the body will tend to fly upward a bit).
	vecDir = Vector( 0, 0, 0 );
	if( !FNullEnt( pevInflictor ) )
	{
		CBaseEntity *pInflictor = CBaseEntity::Instance( pevInflictor );
		if( pInflictor )
		{
			m_MSblastvec = pev->origin - pevInflictor->origin;
			vecDir = ( pInflictor->Center() - Vector ( 0, 0, 10 ) - Center() ).Normalize();
			vecDir = g_vecAttackDir = vecDir.Normalize();
		}
	}

	// add to the damage total for clients, which will be sent as a single
	// message at the end of the frame
	// todo: remove after combining shotgun blasts?
	if( IsPlayer() )
	{
		if( pevInflictor )
			pev->dmg_inflictor = ENT( pevInflictor );

		pev->dmg_take += flTake;

		// check for godmode or invincibility
		if( pev->flags & FL_GODMODE )
		{
			return 0;
		}
	}

	// if this is a player, move him around!
	if( ( !FNullEnt( pevInflictor ) ) && ( pev->movetype == MOVETYPE_WALK ) && ( !pevAttacker || pevAttacker->solid != SOLID_TRIGGER ) && flDamage >= 5 )
	{
		Vector Damage_force = vecDir * -DamageForce( flDamage );

		Damage_force.x *= 0.5;
		Damage_force.y *= 0.5;
		Damage_force.z = 0;

		pev->velocity = pev->velocity + Damage_force;
	}

	// HACKHACK Don't kill monsters in a script.  Let them break their scripts first
	if( m_MonsterState == MONSTERSTATE_SCRIPT )
	{
		if(m_can_kill_script == 0)
		{
			SetConditions( bits_COND_LIGHT_DAMAGE );
			return 0;
		}
		else
		{
			pev->health -= flTake;
			SetConditions( bits_COND_LIGHT_DAMAGE );
			if(pev->health < 1)
			{
				m_killedbydmg = bitsDamageType;
				Killed( pevAttacker, GIB_NEVER );
			}
			else
				return 0;
		}
	}

	if(pev->solid == SOLID_NOT && pev->deadflag != DEAD_NO)
		return 0;

	if(pev->deadflag == DEAD_NO)
	{
		// do the damage
		pev->health -= flTake;
	}

	if ( pev->health < 1 )
	{
		if(flDamage >= 600)
		{
			m_grenadekilled = 7;
		}
		else if(flDamage >= 480)
		{
			m_grenadekilled = 6;
		}
	    else if(flDamage >= 360)
		{
			m_grenadekilled = 5;
		}
		else if(flDamage >= 180)
		{
			m_grenadekilled = 4;
		}
		else if(flDamage >= 120)
		{
			m_grenadekilled = 3;
		}
		else if(flDamage >= 60)
		{
			m_grenadekilled = 2;
		}
		else if(flDamage >= 30)
		{
			m_grenadekilled = 1;
		}
		else if(m_LastHitGroup == 1 && flDamage >= 15)
		{
			m_grenadekilled = 1;
		}

		if(bitsDamageType & DMG_ENERGYBLAST)
		{
			if(flDamage >= 400)
			{
				m_grenadekilled = 7;
			}
			else if(flDamage >= 300)
			{
				m_grenadekilled = 6;
			}
			else if(flDamage >= 240)
			{
				m_grenadekilled = 5;
			}
			else if(flDamage >= 160)
			{
				m_grenadekilled = 4;
			}
			else if(flDamage >= 120)
			{
				m_grenadekilled = 3;
			}
        	else if(flDamage >= 80)
			{
				m_grenadekilled = 2;
			}
			else if(flDamage >= 40)
			{
            	m_grenadekilled = 1;
			}
		}
		else if( bitsDamageType & DMG_BLAST )
		{
			if(flDamage >= 300)
			{
				m_grenadekilled = 7;
			}
			else if(flDamage >= 200)
			{
				m_grenadekilled = 6;
			}
			else if(flDamage >= 120)
			{
				m_grenadekilled = 5;
			}
			else if(flDamage >= 80)
			{
				m_grenadekilled = 4;
			}
			else if(flDamage >= 60)
			{
				m_grenadekilled = 3;
			}
        	else if(flDamage >= 40)
			{
				m_grenadekilled = 2;
			}
			else if(flDamage >= 20)
			{
            	m_grenadekilled = 1;
			}
		}
		
		g_pevLastInflictor = pevInflictor;

		if(bitsDamageType & DMG_AIR)
		{
			if(m_grenadekilled < 4)
			{
				m_grenadekilled = 4;
			}
		}

		if ( bitsDamageType & DMG_ALWAYSGIB )
		{
			m_killedbydmg = bitsDamageType;
			Killed( pevAttacker, GIB_ALWAYS );
		}
		else if ( (bitsDamageType & DMG_NEVERGIB))
		{
			if(m_grenadekilled > 4)
			{
				m_grenadekilled = 4;
			}
			m_killedbydmg = bitsDamageType;
			Killed( pevAttacker, GIB_NEVER );
		}
		else
		{
			m_killedbydmg = bitsDamageType;
			Killed( pevAttacker, GIB_NORMAL );
		}

		g_pevLastInflictor = NULL;
		
		return 0;
	}

	// react to the damage (get mad)
	if( ( pev->flags & FL_MONSTER ) && !FNullEnt( pevAttacker ) )
	{
		if( pevAttacker->flags & ( FL_MONSTER | FL_CLIENT ) )
		{
			if(m_movementGoal == MOVEGOAL_PATHCORNER || flDamage == 0 || m_walkaround == TRUE)
			{
				m_alert = 100;
				if(m_movementGoal == MOVEGOAL_PATHCORNER)
				{
					m_cleardally_enemy_long = 60;
					ClearSchedule();
				}
			}

			if(m_ignoredamage == 0)
			{
				// only if the attack was a monster or client!
				// enemy's last known position is somewhere down the vector that the attack came from.
				if( pevInflictor )
				{
					if( m_hEnemy == 0 || pevInflictor == m_hEnemy->pev || !HasConditions( bits_COND_SEE_ENEMY ) )
					{
						m_vecEnemyLKP = pevInflictor->origin;
					}
				}
				else
				{
					m_vecEnemyLKP = pev->origin + ( g_vecAttackDir * 64.0f );
				}

				MakeIdealYaw( m_vecEnemyLKP );

				// add pain to the conditions 
				// !!!HACKHACK - fudged for now. Do we want to have a virtual function to determine what is light and 
				// heavy damage per monster class?
				if( flDamage > 0.0f )
				{
					SetConditions( bits_COND_LIGHT_DAMAGE );
				}

				if( flDamage >= 30.0f )
				{
					SetConditions( bits_COND_HEAVY_DAMAGE );
				}
			}
			else if(m_ignoredamage == 2 && flDamage >= 30)
			{
				m_alert = 100;

				if( pevInflictor )
				{
					if( m_hEnemy == 0 || pevInflictor == m_hEnemy->pev || !HasConditions( bits_COND_SEE_ENEMY ) )
					{
							m_vecEnemyLKP = pevInflictor->origin;
					}
				}
				else
				{
					m_vecEnemyLKP = pev->origin + ( g_vecAttackDir * 64 ); 
				}

				MakeIdealYaw( m_vecEnemyLKP );

				SetConditions(bits_COND_LIGHT_DAMAGE);
			}
			else
			{
				m_alert = 100;
			}
		}
	}

	return 1;
}

//=========================================================
// DeadTakeDamage - takedamage function called when a monster's
// corpse is damaged.
//=========================================================
int CBaseMonster::DeadTakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	Vector vecDir;

	// grab the vector of the incoming attack. ( pretend that the inflictor is a little lower than it really is, so the body will tend to fly upward a bit).
	vecDir = Vector( 0, 0, 0 );
	if( !FNullEnt( pevInflictor ) )
	{
		CBaseEntity *pInflictor = CBaseEntity::Instance( pevInflictor );
		if( pInflictor )
		{
			vecDir = ( pInflictor->Center() - Vector ( 0.0f, 0.0f, 10.0f ) - Center() ).Normalize();
			vecDir = g_vecAttackDir = vecDir.Normalize();
		}
	}

#if 0// turn this back on when the bounding box issues are resolved.

	pev->flags &= ~FL_ONGROUND;
	pev->origin.z += 1.0f;

	// let the damage scoot the corpse around a bit.
	if( !FNullEnt( pevInflictor ) && ( pevAttacker->solid != SOLID_TRIGGER ) )
	{
		pev->velocity = pev->velocity + vecDir * -DamageForce( flDamage );
	}
#endif
	// kill the corpse if enough damage was done to destroy the corpse and the damage is of a type that is allowed to destroy the corpse.
	/*if( bitsDamageType & DMG_GIB_CORPSE )
	{
		if( pev->health <= flDamage )
		{
			pev->health = -50;
			Killed( pevAttacker, GIB_ALWAYS );
			return 0;
		}
		// Accumulate corpse gibbing damage, so you can gib with multiple hits
		pev->health -= flDamage * 0.1f;
	}*/



	if( bitsDamageType & DMG_GIB_CORPSE )
	{
		pev->health -= flDamage;
	}

	if( bitsDamageType & DMG_ALWAYSGIB )
	{
		Killed( pevAttacker, GIB_ALWAYS );
	}
	else if( (bitsDamageType & DMG_NEVERGIB)  )
	{
		Killed( pevAttacker, GIB_NEVER );
	}
	else
	{
		Killed( pevAttacker, GIB_NORMAL );
	}

	return 0;
}

float CBaseMonster::DamageForce( float damage )
{ 
	float force = damage * ( ( 32.0f * 32.0f * 72.0f ) / ( pev->size.x * pev->size.y * pev->size.z ) ) * 5.0f;

	if( force > 1000.0f ) 
	{
		force = 1000.0f;
	}

	return force;
}

//
// RadiusDamage - this entity is exploding, or otherwise needs to inflict damage upon entities within a certain range.
// 
// only damage ents that can clearly be seen by the explosion!
void RadiusDamage3(Vector vecSrc, entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, float flRadius, int iClassIgnore, int bitsDamageType)
{
	const char *killer_weapon_name = "world";
		
	CBaseEntity *pEntity = NULL;
	TraceResult tr;
	float flAdjustedDamage, falloff;
	Vector vecSpot;

	if (flRadius)
		falloff = flDamage / flRadius;
	else
		falloff = 1;

	int bInWater = (UTIL_PointContents(vecSrc) == CONTENTS_WATER);

	vecSrc.z += 1;

	if (!pevAttacker)
		pevAttacker = pevInflictor;

	while ((pEntity = UTIL_FindEntityInSphere(pEntity, vecSrc, flRadius)) != NULL)
	{
		//if (pEntity->pev->takedamage == DAMAGE_NO)
		//	continue;

		if (iClassIgnore != CLASS_NONE && pEntity->Classify() == iClassIgnore)
			continue;

		vecSpot = pEntity->BodyTarget( vecSrc );
			
		UTIL_TraceLine ( vecSrc, vecSpot, ignore_monsters,ENT(pevInflictor), &tr );
				
		// decrease damage for an ent that's farther from the bomb.
		flAdjustedDamage = ( vecSrc - tr.vecEndPos ).Length() * falloff;
		flAdjustedDamage = flDamage - flAdjustedDamage;
			
		if ( flAdjustedDamage < 0 )
		{
			flAdjustedDamage = 0;
		}

		if((bitsDamageType & DMG_DARK))
		{
			if(flAdjustedDamage >= flDamage * 0.5)
			{
				if( pEntity->pev->takedamage )
				{
							pEntity->TakeDamage ( pevInflictor, pevAttacker, flAdjustedDamage, bitsDamageType );
				}
				if ( FClassnameIs(pEntity->pev, "gib") )
				{
					UTIL_Remove( pEntity );
					continue;
				}
			}

			if(pEntity->pev->gravity <= 1.5)
			{
				if( (pEntity->pev->flags & FL_MONSTER) || (pEntity->pev->flags & FL_CLIENT) )
				{
					if(pEntity->Classify() != CLASS_PLAYER_ALLY && pEntity->Classify() != CLASS_PLAYER)
					{
						MovetoTarget_darkhole(pEntity->pev,vecSrc + Vector(0,0,60),flAdjustedDamage * 15);
					}
				}
			}

			if ( FClassnameIs(pEntity->pev, "gib") )
			{
				MovetoTarget_darkhole(pEntity->pev,vecSrc + Vector(0,0,60),flAdjustedDamage * 15);
			}
		}
		else
		{
			if((bitsDamageType & DMG_BLAST))
			{
				if( !(pEntity->pev->flags & FL_NOTARGET) && pEntity->pev->solid != SOLID_BSP )
				{
					MovetoTarget_darkhole(pEntity->pev,vecSrc,-flAdjustedDamage * 10);
				}
			}
			else
			{
				if( (pEntity->pev->flags & FL_MONSTER) )
				{
					if(pEntity->pev->gravity <= 1.5 && pEntity->Classify() != CLASS_PLAYER_ALLY && pEntity->Classify() != CLASS_PLAYER)
					{
						MovetoTarget_darkhole(pEntity->pev,vecSrc,-flAdjustedDamage * 10);
					}
				}
				else
				{
					killer_weapon_name = STRING( pEntity->pev->classname );
					if(pEntity->pev->health == 0 && pEntity->pev->movetype == MOVETYPE_BOUNCE)
					{
						if ( strncmp( killer_weapon_name, "weapon_", 7 ) == 0 
						|| FClassnameIs(pEntity->pev, "grenade")
						|| FClassnameIs(pEntity->pev, "monster_satchel") 
						|| FClassnameIs(pEntity->pev, "monster_satchel_green") 
						|| FClassnameIs(pEntity->pev, "gib") )
						{
							MovetoTarget_darkhole(pEntity->pev,vecSrc,-flAdjustedDamage * 10);
						}
					}
					else if ( strncmp( killer_weapon_name, "ammo_", 5 ) == 0 ) 
					{
						MovetoTarget_darkhole(pEntity->pev,vecSrc,-flAdjustedDamage * 10);
					}
				}
			}

					//pEntity->TakeDamage ( pevInflictor, pevAttacker, flAdjustedDamage, bitsDamageType );
		}
	}
}

void RadiusDamage2( Vector vecSrc, entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, float flRadius, int iClassIgnore, int bitsDamageType )
{
	CBaseEntity *pEntity = NULL;
	TraceResult	tr;
	float		flAdjustedDamage, falloff;
	Vector		vecSpot;

	if ( flRadius )
		falloff = flDamage / flRadius;
	else
		falloff = 1.0;

	int bInWater = (UTIL_PointContents ( vecSrc ) == CONTENTS_WATER);

	vecSrc.z += 1;// in case grenade is lying on the ground

	if ( !pevAttacker )
		pevAttacker = pevInflictor;

	// iterate on all entities in the vicinity.
	while ((pEntity = UTIL_FindEntityInSphere( pEntity, vecSrc, flRadius )) != NULL)
	{
		if ( pEntity->pev->takedamage != DAMAGE_NO )
		{
			// UNDONE: this should check a damage mask, not an ignore
			if ( iClassIgnore != CLASS_NONE && pEntity->Classify() == iClassIgnore )
			{// houndeyes don't hurt other houndeyes with their attack
				continue;
			}

			// blast's don't tavel into or out of water
			if (bInWater && pEntity->pev->waterlevel == 0)
				continue;
			if (!bInWater && pEntity->pev->waterlevel == 3)
				continue;

			vecSpot = pEntity->BodyTarget( vecSrc );
			
			UTIL_TraceLine ( vecSrc, vecSpot, ignore_monsters,ignore_glass, ENT(pevInflictor), &tr );

			if ( tr.flFraction == 1.0 || tr.pHit == pEntity->edict() )
			{// the explosion can 'see' this entity, so hurt them!
				if (tr.fStartSolid)
				{
					// if we're stuck inside them, fixup the position and distance
				//	tr.vecEndPos = vecSrc; ȫ���bug
					tr.flFraction = 0.0;
				}
				
				// decrease damage for an ent that's farther from the bomb.
				flAdjustedDamage = ( vecSrc - tr.vecEndPos ).Length() * falloff;
				flAdjustedDamage = flDamage - flAdjustedDamage;
			
				if ( flAdjustedDamage < 0 )
				{
					flAdjustedDamage = 0;
				}
			
				if ( pEntity->pev->flags & FL_MONSTER ){
					CBaseMonster *pEnemyMonster;
					pEnemyMonster = pEntity->MyMonsterPointer();
					if(pEnemyMonster){
						if(pEnemyMonster->m_trouch_full_radiusdmg > 0)
						{
							pEnemyMonster->m_trouch_full_radiusdmg = 0;
							flAdjustedDamage = flDamage;
						}
					}
				}
				UTIL_TraceLine ( vecSrc, vecSpot, dont_ignore_monsters, ENT(pevInflictor), &tr );
				// ALERT( at_console, "hit %s\n", STRING( pEntity->pev->classname ) );
				if (flAdjustedDamage >= 10 && tr.flFraction != 1.0)
				{
					ClearMultiDamage( );
					pEntity->TraceAttack( pevInflictor, flAdjustedDamage, (tr.vecEndPos - vecSrc).Normalize( ), &tr, bitsDamageType );
					ApplyMultiDamage( pevInflictor, pevAttacker );
				}
				else if (flAdjustedDamage >= 1)
				{
					pEntity->TakeDamage ( pevInflictor, pevAttacker, flAdjustedDamage, bitsDamageType );
				}
			}
			else
			{
				// decrease damage for an ent that's farther from the bomb.
				flAdjustedDamage = ( vecSrc - tr.vecEndPos ).Length() * falloff;
				flAdjustedDamage = flDamage - flAdjustedDamage;

				if(flDamage >= 200)
				{
					flAdjustedDamage -= flDamage * 0.5;
				}
				else
				{
					flAdjustedDamage = 0;
				}

				if ( flAdjustedDamage >= 1 )
				{
					pEntity->TakeDamage(pevInflictor, pevAttacker, flAdjustedDamage, bitsDamageType);
				}
			}
		}
	}
}

void RadiusDamage_limit( Vector vecSrc, entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, float flRadius, int iClassIgnore, int bitsDamageType )
{
	CBaseEntity *pEntity = NULL;
	TraceResult	tr;
	float		flAdjustedDamage, falloff;
	Vector		vecSpot;

	if ( flRadius )
		falloff = flDamage / flRadius;
	else
		falloff = 1.0;

	int bInWater = (UTIL_PointContents ( vecSrc ) == CONTENTS_WATER);

	vecSrc.z += 1;// in case grenade is lying on the ground

	if ( !pevAttacker )
		pevAttacker = pevInflictor;

	// iterate on all entities in the vicinity.
	while ((pEntity = UTIL_FindEntityInSphere( pEntity, vecSrc, flRadius )) != NULL)
	{
		if ( pEntity->pev->takedamage != DAMAGE_NO )
		{
			// UNDONE: this should check a damage mask, not an ignore
			if ( iClassIgnore != CLASS_NONE && pEntity->Classify() == iClassIgnore )
			{// houndeyes don't hurt other houndeyes with their attack
				continue;
			}

			if ( iClassIgnore == 623 && 
				(pEntity->Classify() == CLASS_PLAYER
				|| pEntity->Classify() == CLASS_PLAYER_ALLY
				|| pEntity->Classify() == CLASS_HUMAN_PASSIVE) )
			{
				continue;
			}

			// blast's don't tavel into or out of water
			if (bInWater && pEntity->pev->waterlevel == 0)
				continue;
			if (!bInWater && pEntity->pev->waterlevel == 3)
				continue;

			vecSpot = pEntity->BodyTarget( vecSrc );
			
			UTIL_TraceLine ( vecSrc, vecSpot, ignore_monsters,ignore_glass, ENT(pevInflictor), &tr );

			if ( tr.flFraction == 1.0 || tr.pHit == pEntity->edict() )
			{// the explosion can 'see' this entity, so hurt them!
				if (tr.fStartSolid)
				{
					// if we're stuck inside them, fixup the position and distance
				//	tr.vecEndPos = vecSrc;
					tr.flFraction = 0.0;
				}
				
				// decrease damage for an ent that's farther from the bomb.
				flAdjustedDamage = ( vecSrc - tr.vecEndPos ).Length() * falloff;
				flAdjustedDamage = flDamage - flAdjustedDamage;
			
				if(iClassIgnore == 999)
				{
					flAdjustedDamage = flDamage;
				}
				else
				{
					flAdjustedDamage += flDamage * 0.25;

					if(flAdjustedDamage > flDamage)
					{
						flAdjustedDamage = flDamage;
					}
				}

				UTIL_TraceLine ( vecSrc, vecSpot, dont_ignore_monsters, ENT(pevInflictor), &tr );
				// ALERT( at_console, "hit %s\n", STRING( pEntity->pev->classname ) );
				if (flAdjustedDamage >= 10 && tr.flFraction != 1.0)
				{
					ClearMultiDamage( );
					pEntity->TraceAttack( pevInflictor, flAdjustedDamage, (tr.vecEndPos - vecSrc).Normalize( ), &tr, bitsDamageType );
					ApplyMultiDamage( pevInflictor, pevAttacker );
				}
				else if (flAdjustedDamage >= 1)
				{
					pEntity->TakeDamage ( pevInflictor, pevAttacker, flAdjustedDamage, bitsDamageType );
				}
			}
			else
			{	
				// decrease damage for an ent that's farther from the bomb.
				flAdjustedDamage = ( vecSrc - tr.vecEndPos ).Length() * falloff;
				flAdjustedDamage = flDamage - flAdjustedDamage;

				if(flDamage >= 200)
				{
					flAdjustedDamage -= flDamage * 0.5;
				}
				else
				{
					flAdjustedDamage = 0;
				}

				if ( flAdjustedDamage >= 1 )
				{
					pEntity->TakeDamage(pevInflictor, pevAttacker, flAdjustedDamage, bitsDamageType);
				}
			}
		}
	}
}

void RadiusDamage( Vector vecSrc, entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, float flRadius, int iClassIgnore, int bitsDamageType )
{
	CBaseEntity *pEntity = NULL;
	TraceResult	tr;
	float		flAdjustedDamage, falloff;
	Vector		vecSpot;

	if( flRadius )
		falloff = flDamage / flRadius;
	else
		falloff = 1.0f;

	int bInWater = ( UTIL_PointContents( vecSrc ) == CONTENTS_WATER );

	vecSrc.z += 1.0f;// in case grenade is lying on the ground

	if( !pevAttacker )
		pevAttacker = pevInflictor;

	// iterate on all entities in the vicinity.
	while( ( pEntity = UTIL_FindEntityInSphere( pEntity, vecSrc, flRadius ) ) != NULL )
	{
		if( pEntity->pev->takedamage != DAMAGE_NO )
		{
			// UNDONE: this should check a damage mask, not an ignore
			if( iClassIgnore != CLASS_NONE && pEntity->Classify() == iClassIgnore )
			{
				// houndeyes don't hurt other houndeyes with their attack
				continue;
			}

			// blast's don't tavel into or out of water
			if( bInWater && pEntity->pev->waterlevel == 0 )
				continue;
			if( !bInWater && pEntity->pev->waterlevel == 3 )
				continue;

			vecSpot = pEntity->BodyTarget( vecSrc );

			UTIL_TraceLine( vecSrc, vecSpot, dont_ignore_monsters, ENT( pevInflictor ), &tr );

			if( tr.flFraction == 1.0f || tr.pHit == pEntity->edict() )
			{
				// the explosion can 'see' this entity, so hurt them!
				if( tr.fStartSolid )
				{
					// if we're stuck inside them, fixup the position and distance
					tr.vecEndPos = vecSrc;
					tr.flFraction = 0.0f;
				}

				// decrease damage for an ent that's farther from the bomb.
				flAdjustedDamage = ( vecSrc - tr.vecEndPos ).Length() * falloff;
				flAdjustedDamage = flDamage - flAdjustedDamage;

				if( flAdjustedDamage < 0.0f )
				{
					flAdjustedDamage = 0.0f;
				}

				// ALERT( at_console, "hit %s\n", STRING( pEntity->pev->classname ) );
				if( tr.flFraction != 1.0f )
				{
					ClearMultiDamage();
					pEntity->TraceAttack( pevInflictor, flAdjustedDamage, ( tr.vecEndPos - vecSrc ).Normalize(), &tr, bitsDamageType );
					ApplyMultiDamage( pevInflictor, pevAttacker );
				}
				else
				{
					pEntity->TakeDamage ( pevInflictor, pevAttacker, flAdjustedDamage, bitsDamageType );
				}
			}
		}
	}
}

void CBaseMonster::RadiusDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int iClassIgnore, int bitsDamageType )
{
	::RadiusDamage2( pev->origin, pevInflictor, pevAttacker, flDamage, flDamage * 2.5f, iClassIgnore, bitsDamageType );
}

void CBaseMonster::RadiusDamage( Vector vecSrc, entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int iClassIgnore, int bitsDamageType )
{
	::RadiusDamage2( pev->origin, pevInflictor, pevAttacker, flDamage, flDamage * 2.5f, iClassIgnore, bitsDamageType );
}

//=========================================================
// CheckTraceHullAttack - expects a length to trace, amount 
// of damage to do, and damage type. Returns a pointer to
// the damaged entity in case the monster wishes to do
// other stuff to the victim (punchangle, etc)
//
// Used for many contact-range melee attacks. Bites, claws, etc.
//=========================================================
CBaseEntity* CBaseMonster::CheckTraceHullAttack( float flDist, int iDamage, int iDmgType )
{
	TraceResult tr;

	if( IsPlayer() )
		UTIL_MakeVectors( pev->angles );
	else
		UTIL_MakeAimVectors( pev->angles );

	Vector vecStart = pev->origin;
	vecStart.z += pev->size.z * 0.5f;
	Vector vecEnd = vecStart + ( gpGlobals->v_forward * flDist );

	UTIL_TraceHull( vecStart, vecEnd, dont_ignore_monsters, head_hull, ENT( pev ), &tr );

	if( tr.pHit )
	{
		CBaseEntity *pEntity = CBaseEntity::Instance( tr.pHit );

		if( iDamage > 0 )
		{
			pEntity->TakeDamage( pev, pev, iDamage, iDmgType );
		}

		return pEntity;
	}

	return NULL;
}

//=========================================================
// FInViewCone - returns true is the passed ent is in
// the caller's forward view cone. The dot product is performed
// in 2d, making the view cone infinitely tall. 
//=========================================================
BOOL CBaseMonster::FInViewCone( CBaseEntity *pEntity )
{
	Vector2D	vec2LOS;
	float	flDot;

	UTIL_MakeVectors( pev->angles );

	vec2LOS = ( pEntity->pev->origin - pev->origin ).Make2D();
	vec2LOS = vec2LOS.Normalize();

	flDot = DotProduct( vec2LOS, gpGlobals->v_forward.Make2D() );

	if( flDot > m_flFieldOfView )
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

BOOL CBaseMonster :: FInViewCone2 ( CBaseEntity *pEntity )
{
	Vector2D	vec2LOS;
	float	flDot;

	UTIL_MakeVectors ( pev->angles );

	if(!pEntity)
		return FALSE;

	vec2LOS = ( pEntity->pev->origin - pev->origin ).Make2D();
	vec2LOS = vec2LOS.Normalize();

	flDot = DotProduct (vec2LOS , gpGlobals->v_forward.Make2D() );

	if ( flDot > 0.8 )
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

BOOL CBaseMonster :: FInViewCone3 ( CBaseEntity *pEntity )
{
	Vector2D	vec2LOS;
	float	flDot;

	UTIL_MakeVectors ( pev->angles );
	
	if(!pEntity)
		return FALSE;

	vec2LOS = ( pEntity->pev->origin - pev->origin ).Make2D();
	vec2LOS = vec2LOS.Normalize();

	flDot = DotProduct (vec2LOS , gpGlobals->v_forward.Make2D() );

	if ( flDot > 0.5 )
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

BOOL CBaseMonster :: FInViewCone4 ( CBaseEntity *pEntity )
{
	Vector2D	vec2LOS;
	float	flDot;

	UTIL_MakeVectors ( pev->angles );
	
	if(!pEntity)
		return FALSE;

	vec2LOS = ( pEntity->pev->origin - pev->origin ).Make2D();
	vec2LOS = vec2LOS.Normalize();

	flDot = DotProduct (vec2LOS , gpGlobals->v_forward.Make2D() );

	if ( flDot > 0.2 )
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

//=========================================================
// FInViewCone - returns true is the passed vector is in
// the caller's forward view cone. The dot product is performed
// in 2d, making the view cone infinitely tall. 
//=========================================================
BOOL CBaseMonster::FInViewCone( Vector *pOrigin )
{
	Vector2D	vec2LOS;
	float		flDot;

	UTIL_MakeVectors( pev->angles );

	vec2LOS = ( *pOrigin - pev->origin ).Make2D();
	vec2LOS = vec2LOS.Normalize();

	flDot = DotProduct( vec2LOS, gpGlobals->v_forward.Make2D() );

	if( flDot > m_flFieldOfView )
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

//=========================================================
// FVisible - returns true if a line can be traced from
// the caller's eyes to the target
//=========================================================
BOOL CBaseEntity::FVisible( CBaseEntity *pEntity )
{
	TraceResult tr;
	Vector		vecLookerOrigin;
	Vector		vecTargetOrigin;

	if( !pEntity )
		return FALSE;
	if( !pEntity->pev )
		return FALSE;

	if( FBitSet( pEntity->pev->flags, FL_NOTARGET ) )
		return FALSE;

	if( (pev->flags & FL_FLY) && (pev->flags & FL_SWIM) )
	{
	}
	else
	{
		// don't look through water
		if( ( pev->waterlevel == 0 && pEntity->pev->waterlevel == 3 ) 
			|| ( pev->waterlevel >= 3 && pEntity->pev->waterlevel == 0 ) )
			return FALSE;
	}

	vecLookerOrigin = pev->origin + pev->view_ofs;//look through the caller's 'eyes'
	vecTargetOrigin = pEntity->EyePosition();


	CBaseMonster *pEnemyMonster;
	pEnemyMonster = this->MyMonsterPointer();
	if(pEnemyMonster)
	{
		if(pEnemyMonster->m_EyeMod == 1)
		{
			UTIL_TraceLine( vecLookerOrigin, vecTargetOrigin, ignore_monsters, ENT( pev )/*pentIgnore*/, &tr );
		}
		else
		{
			UTIL_TraceLine( vecLookerOrigin, vecTargetOrigin, ignore_monsters, ignore_glass, ENT( pev )/*pentIgnore*/, &tr );
		}
	}
	else
	{
		UTIL_TraceLine( vecLookerOrigin, vecTargetOrigin, ignore_monsters, ignore_glass, ENT( pev )/*pentIgnore*/, &tr );
	}

	if( tr.flFraction != 1.0f )
	{

		if(pEnemyMonster)
		{
			if(pEnemyMonster->m_EyeMod >= 2)
			{
				vecTargetOrigin = pEntity->pev->origin;//ֱ�ӿ��ŵ�
				vecTargetOrigin.x += RANDOM_LONG(pEntity->pev->mins.x,pEntity->pev->maxs.x);
				vecTargetOrigin.y += RANDOM_LONG(pEntity->pev->mins.y,pEntity->pev->maxs.y);
				vecTargetOrigin.z += RANDOM_LONG(pEntity->pev->mins.z,pEntity->pev->view_ofs.z) + 1;
			}
			else
				return FALSE;
		}
		else
			return FALSE;

		UTIL_TraceLine(vecLookerOrigin, vecTargetOrigin, ignore_monsters, ignore_glass, ENT(pev)/*pentIgnore*/, &tr);

		if (tr.flFraction == 1.0)
			return TRUE;
		
		return FALSE;// Line of sight is not established
	}
	else
	{
		return TRUE;// line of sight is valid.
	}
}

//=========================================================
// FVisible - returns true if a line can be traced from
// the caller's eyes to the target vector
//=========================================================
BOOL CBaseEntity::FVisible( const Vector &vecOrigin )
{
	TraceResult tr;
	Vector		vecLookerOrigin;

	vecLookerOrigin = EyePosition();//look through the caller's 'eyes'

	UTIL_TraceLine( vecLookerOrigin, vecOrigin, ignore_monsters, ignore_glass, ENT( pev )/*pentIgnore*/, &tr );

	if( tr.flFraction != 1.0f )
	{
		return FALSE;// Line of sight is not established
	}
	else
	{
		return TRUE;// line of sight is valid.
	}
}

/*
================
TraceAttack
================
*/
void CBaseEntity::TraceAttack(entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType)
{
	Vector vecOrigin = ptr->vecEndPos - vecDir * 4.0f;

	if( pev->takedamage )
	{
		AddMultiDamage( pevAttacker, this, flDamage, bitsDamageType );

		int blood = BloodColor();

		if( blood != DONT_BLEED )
		{
			SpawnBlood( vecOrigin, blood, flDamage );// a little surface blood.
			TraceBleed( flDamage, vecDir, ptr, bitsDamageType );
		}
	}
}

/*
//=========================================================
// TraceAttack
//=========================================================
void CBaseMonster::TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType )
{
	Vector vecOrigin = ptr->vecEndPos - vecDir * 4.0f;

	ALERT( at_console, "%d\n", ptr->iHitgroup );

	if( pev->takedamage )
	{
		AddMultiDamage( pevAttacker, this, flDamage, bitsDamageType );

		int blood = BloodColor();

		if( blood != DONT_BLEED )
		{
			SpawnBlood( vecOrigin, blood, flDamage );// a little surface blood.
		}
	}
}
*/

//=========================================================
// TraceAttack
//=========================================================
void CBaseMonster::TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType )
{
	if( m_flGodTime > gpGlobals->time )
		return;
		
	if( pev->takedamage && m_godmode == FALSE )
	{
		m_LastHitGroup = ptr->iHitgroup;

		if (pevAttacker && pev->deadflag == DEAD_NO)
		{
			CBaseEntity *pEntity = GetClassPtr((CBaseEntity *)pevAttacker);
			
			if ( pEntity->Classify() == Classify() )
			{
				if(Classify() == CLASS_PLAYER_ALLY)
					return;
			}
			else if ( (pEntity->pev->flags & FL_CLIENT) )
			{
				if(m_rpgms_inteam > 0)
					return;
			}
			else if ( (pEntity->pev->team == CLASS_PLAYER || pEntity->Classify() == CLASS_PLAYER_BIOWEAPON) &&  Classify() == CLASS_PLAYER_ALLY && m_rpgms_inteam > 0)
				return;
		}

		if( bitsDamageType & (DMG_ENERGYBEAM|DMG_BULLET|DMG_CLUB) )
		{
			switch( ptr->iHitgroup )
			{
			case HITGROUP_GENERIC:
				break;
			case HITGROUP_HEAD:
				{
					if ( m_headdef == 2 )
					{
						flDamage *= 2;
					}
					else if ( m_headdef == 1 )
					{
						flDamage *= 4;
					}
					else
					{
						flDamage *= 3;
					}
						
					if(flDamage >= 10)
					{
						int blood = BloodColor();
						if (blood == BLOOD_COLOR_RED)
						{
							UTIL_BloodStream( ptr->vecEndPos,gpGlobals->v_forward * -5 + gpGlobals->v_up * 2,70, 60 );
						}
						if (blood == BLOOD_COLOR_YELLOW || blood == BLOOD_COLOR_GREEN)
						{
							UTIL_BloodStream( ptr->vecEndPos,gpGlobals->v_forward * -5 + gpGlobals->v_up * 2,58, 60 );
						}
					}

				}
				//flDamage *= gSkillData.monHead;
				break;
			case HITGROUP_CHEST:
				//flDamage *= gSkillData.monChest;
				break;
			case HITGROUP_STOMACH:
				//flDamage *= gSkillData.monStomach;
				break;
			case HITGROUP_LEFTARM:
			case HITGROUP_RIGHTARM:
				//flDamage *= gSkillData.monArm;
				flDamage *= 1;
				break;
			case HITGROUP_LEFTLEG:
			case HITGROUP_RIGHTLEG:
				//flDamage *= gSkillData.monLeg;
				flDamage *= 0.75;
				break;
			default:
				break;
			}
		}
		else
		{
			if( bitsDamageType & (DMG_SHOCK | DMG_GENERIC | DMG_SLASH | DMG_BURN | DMG_SONIC ) )
			{
				if(ptr->iHitgroup == 1)
				{
					if ( m_headdef == 2 )
					{
						flDamage *= 1.25;
					}
					else if ( m_headdef == 1 )
					{
						flDamage *= 2.0;
					}
					else
					{
						flDamage *= 1.5;
					}
				
					if(flDamage >= 10)
					{
						int blood = BloodColor();
						if (blood == BLOOD_COLOR_RED)
						{
							UTIL_BloodStream( ptr->vecEndPos,gpGlobals->v_forward * -5 + gpGlobals->v_up * 2,70, 60 );
						}
						if (blood == BLOOD_COLOR_YELLOW || blood == BLOOD_COLOR_GREEN)
						{
							UTIL_BloodStream( ptr->vecEndPos,gpGlobals->v_forward * -5 + gpGlobals->v_up * 2,58, 60 );
						}
					}
				}
			}
		}

		if(flDamage >= 10 && pev->deadflag == DEAD_NO)
		{
			if ( m_bloodColor == BLOOD_COLOR_YELLOW)
			{
				if(flDamage >= 40)
				{
					FX_Explosion( ptr->vecEndPos, 237 );
				}
				else if(flDamage >= 20)
				{
					FX_Explosion( ptr->vecEndPos, 235 );
				}
				else if(flDamage >= 10)
				{
					FX_Explosion( ptr->vecEndPos, 233 );
				}
			}
			else if ( m_bloodColor == BLOOD_COLOR_RED)
			{
				if(flDamage >= 40)
				{
					FX_Explosion( ptr->vecEndPos, 236 );
				}
				else if(flDamage >= 20)
				{
					FX_Explosion( ptr->vecEndPos, 234 );
				}
				else if(flDamage >= 10)
				{
					FX_Explosion( ptr->vecEndPos, 232 );
				}
			}
		}

		if(flDamage >= 3 && bitsDamageType != DMG_BLOOD)
		{
			SpawnBlood( ptr->vecEndPos, BloodColor(), flDamage );// a little surface blood.
			TraceBleed( flDamage, vecDir, ptr, bitsDamageType );
		}
	}
	AddMultiDamage( pevAttacker, this, flDamage, bitsDamageType );
}

/*
================
FireBeam
Penetrating wall beams
================
*/
Vector CBaseEntity::FireBeam (Vector vecSrc, Vector vecDirShooting, int iBeamType, float flDamage, entvars_t *pevAttacker)
{
	TraceResult tr, beam_tr, beam_tr_end;
	Vector vecEnd = vecSrc + vecDirShooting * 16384;
	if(flDamage == 623)
		vecEnd = vecDirShooting;

	UTIL_TraceLine(vecSrc, vecEnd, dont_ignore_monsters, ENT(pev), &tr);
	int iPiercePower = 0; 
	int iDmgType = 0;
	float iDamage = 0;
	float iRadiusDamage = 0;
	int iImpactEffect = 0;

	if (pevAttacker == NULL)
		pevAttacker = pev;

	CBaseEntity *pEntity = CBaseEntity::Instance(tr.pHit);
	int tex = (int)TEXTURETYPE_Trace(&tr, vecSrc, vecEnd);
	int surface = (int)SURFACETYPE_Trace(&tr, vecSrc, vecEnd,Classify(),1);

	switch(iBeamType)
	{
		case BEAM_BLASTER:		
			iDamage = flDamage;
			iDmgType = DMG_ENERGYBEAM | DMG_NEVERGIB;
			iPiercePower = iDamage;
			iImpactEffect = IMPBEAM_BLASTER;
		break;

		case BEAM_TAUCANNON:		
			iDamage = flDamage;
			iDmgType = DMG_ENERGYBEAM | DMG_NEVERGIB;
			iPiercePower = iDamage;
			iImpactEffect = IMPBEAM_TAUCANNON;
		break;

	//	case BEAM_TAUCANNON2:		
	//		iDamage = flDamage;
	//		iDmgType = DMG_ENERGYBEAM | DMG_NEVERGIB;
	//		iPiercePower = iDamage;
	//		iImpactEffect = IMPBEAM_TAUCANNON2;
	//	break;

		case BEAM_IONTURRET:		
			iDamage = flDamage;
			iDmgType = DMG_ENERGYBEAM | DMG_NEVERGIB;
			iPiercePower = iDamage/2;
			iImpactEffect = IMPBEAM_IONTURRET;
		break;

		case BEAM_GAUSS:		
			iDamage = flDamage;
			iDmgType = DMG_ENERGYBEAM | DMG_SHOCK | DMG_NEVERGIB;
			iPiercePower = iDamage;
			iImpactEffect = IMPBEAM_GAUSS;
		break;

		case BEAM_GAUSSCHARGED:		
			iDamage = flDamage;
			iDmgType = DMG_ENERGYBEAM | DMG_SHOCK | DMG_NEVERGIB;
			iPiercePower = flDamage/2;
			iImpactEffect = IMPBEAM_GAUSSCHARGED;
		break;

	}
	if (iImpactEffect > 0 && UTIL_PointContents(tr.vecEndPos) != CONTENTS_SKY)
		FX_ImpBeam( tr.vecEndPos, tr.vecPlaneNormal, surface, iImpactEffect );

	FX_FireBeam( vecSrc, tr.vecEndPos, tr.vecPlaneNormal, iBeamType );

	if (tex == CHAR_TEX_ENERGYSHIELD || iPiercePower < 1) 
		return Vector(0,0,0);

	UTIL_TraceLine( tr.vecEndPos + vecDirShooting*8, vecEnd, dont_ignore_monsters, ENT( pev ), &beam_tr);
	if (!beam_tr.fAllSolid)
	{
		UTIL_TraceLine( beam_tr.vecEndPos, tr.vecEndPos, dont_ignore_monsters, ENT( pev ), &beam_tr);

		if((beam_tr.vecEndPos - tr.vecEndPos).Length( ) > iPiercePower) //if walls are ticker than gun-DMG units
			return Vector(0,0,0);

		if (iImpactEffect > 0)
			FX_ImpBeam( beam_tr.vecEndPos, tr.vecPlaneNormal, surface, iImpactEffect );

		UTIL_TraceLine( beam_tr.vecEndPos, vecEnd, dont_ignore_monsters, ENT(pev), &beam_tr_end);
		CBaseEntity *pExitEntity = CBaseEntity::Instance(beam_tr_end.pHit);
		int surf_exit_end = (int)SURFACETYPE_Trace(&beam_tr_end, beam_tr.vecEndPos, vecEnd,Classify(),0);

		if (iImpactEffect > 0 && UTIL_PointContents(beam_tr_end.vecEndPos) != CONTENTS_SKY)
			FX_ImpBeam( beam_tr_end.vecEndPos, tr.vecPlaneNormal, surf_exit_end, iImpactEffect );

		FX_FireBeam( beam_tr.vecEndPos, beam_tr_end.vecEndPos, beam_tr_end.vecPlaneNormal, iBeamType+1 );
	}
	return Vector(0,0,0);
}

/*
================
FireBullets

Go to the trouble of combining multiple pellets into a single damage call.

This version is used by Monsters.
================
*/
void CBaseEntity::FireBullets( ULONG cShots, Vector vecSrc, Vector vecDirShooting, Vector vecSpread, float flDistance, int iBulletType, int iTracerFreq, int iDamage, entvars_t *pevAttacker )
{
	static int tracerCount;
	int tracer;
	TraceResult tr,tr2,tr3,tr4;
	Vector vecRight = gpGlobals->v_right;
	Vector vecUp = gpGlobals->v_up;

	if( pevAttacker == NULL )
		pevAttacker = pev;  // the default attacker is ourselves

	ClearMultiDamage();
	gMultiDamage.type = DMG_BULLET | DMG_NEVERGIB;

	for( ULONG iShot = 1; iShot <= cShots; iShot++ )
	{
		// get circular gaussian spread
		float x, y, z;
		do {
			x = RANDOM_FLOAT( -0.5f, 0.5f ) + RANDOM_FLOAT( -0.5f, 0.5f );
			y = RANDOM_FLOAT( -0.5f, 0.5f ) + RANDOM_FLOAT( -0.5f, 0.5f );
			z = x * x + y * y;
		} while (z > 1);

		Vector vecDir = vecDirShooting +
						x * vecSpread.x * vecRight +
						y * vecSpread.y * vecUp;
		Vector vecEnd;

		vecEnd = vecSrc + vecDir * flDistance;
		UTIL_TraceLine( vecSrc, vecEnd, dont_ignore_monsters, ENT( pev )/*pentIgnore*/, &tr );

		tracer = 0;
		if( iTracerFreq != 0 && ( tracerCount++ % iTracerFreq ) == 0 )
		{
			Vector vecTracerSrc;

			if( IsPlayer() )
			{
				// adjust tracer position for player
				vecTracerSrc = vecSrc + Vector( 0.0f, 0.0f, -4.0f ) + gpGlobals->v_right * 2.0f + gpGlobals->v_forward * 16.0f;
			}
			else
			{
				vecTracerSrc = vecSrc;
			}

			if( iTracerFreq != 1 )		// guns that always trace also always decal
				tracer = 1;
			switch( iBulletType )
			{
			case BULLET_MONSTER_MP5:
			case BULLET_MONSTER_9MM:
			case BULLET_MONSTER_12MM:
			default:
				MESSAGE_BEGIN( MSG_PAS, SVC_TEMPENTITY, vecTracerSrc );
					WRITE_BYTE( TE_TRACER );
					WRITE_COORD( vecTracerSrc.x );
					WRITE_COORD( vecTracerSrc.y );
					WRITE_COORD( vecTracerSrc.z );
					WRITE_COORD( tr.vecEndPos.x );
					WRITE_COORD( tr.vecEndPos.y );
					WRITE_COORD( tr.vecEndPos.z );
				MESSAGE_END();
				break;
			}
		}
		// do damage, paint decals
		if( tr.flFraction != 1.0f )
		{
			CBaseEntity *pEntity = CBaseEntity::Instance( tr.pHit );

			int tex = (int)TEXTURETYPE_Trace(&tr, vecSrc, vecEnd);
			int ThroughWall = 0;
			float damage_level;
			int HearvyHit = 0;
			if(iBulletType == BULLET_14MM || iBulletType == BULLET_30mm 
			|| iBulletType == BULLET_338Magnum || iBulletType == BULLET_GAUSS_HEV
			|| iBulletType == BULLET_IONTURRET || iBulletType == BULLET_DOMA_LASER
			|| iBulletType == BULLET_GMAN_LASER)
				HearvyHit = 1;
			int surface = (int)SURFACETYPE_Trace(&tr, vecSrc, vecEnd,Classify(),HearvyHit);
			if(iBulletType != 810 && iBulletType != 811 && iBulletType != 814)
				FX_ImpBullet( tr.vecEndPos, tr.vecPlaneNormal, vecSrc, surface, iBulletType, (float)tex );

			/*vecDir = vecDir.Normalize();

			if( iDamage )
			{
				pEntity->TraceAttack( pevAttacker, iDamage, vecDir, &tr, DMG_BULLET | ( ( iDamage > 16 ) ? DMG_ALWAYSGIB : DMG_NEVERGIB ) );

				TEXTURETYPE_PlaySound( &tr, vecSrc, vecEnd, iBulletType );
				DecalGunshot( &tr, iBulletType );
			}*/
			switch( iBulletType )
			{
			default:
			case 810:	
				{
					ThroughWall = 0;
					damage_level = 0;
					FX_Explosion( tr.vecEndPos + (tr.vecPlaneNormal * 20), EXPLOSION_TRIPMINE );
					FX_Explosion( tr.vecEndPos + (tr.vecPlaneNormal * 25), EXPLOSION_WHL_SHARD );
					::RadiusDamage2( tr.vecEndPos, pev, pevAttacker, 150, 360, CLASS_NONE, DMG_BLAST);
					MESSAGE_BEGIN( MSG_PAS, SVC_TEMPENTITY, tr.vecEndPos );
							WRITE_BYTE( TE_EXPLOSION);
							WRITE_COORD( tr.vecEndPos.x );
							WRITE_COORD( tr.vecEndPos.y );
							WRITE_COORD( tr.vecEndPos.z );
							WRITE_SHORT( g_sModelIndexFireball );
							WRITE_BYTE( 0 ); // no sprite
							WRITE_BYTE( 15  ); // framerate
							WRITE_BYTE( TE_EXPLFLAG_NONE );
						MESSAGE_END();
					CSoundEnt::InsertSound ( bits_SOUND_COMBAT, pev->origin, NORMAL_EXPLOSION_VOLUME, 3.0 );
					if ( RANDOM_FLOAT( 0 , 1 ) < 0.5 )
					{
						UTIL_DecalTrace( &tr, DECAL_SCORCH1 );
					}
					else
					{
						UTIL_DecalTrace( &tr, DECAL_SCORCH2 );
					}
				}
				break;
			case 811:	
				{
					ThroughWall = 0;
					damage_level = 0;
				
					if(iDamage == 114){
						FX_Trail( tr.vecEndPos + (tr.vecPlaneNormal * 15), entindex(), PROJ_M203_DETONATE );
						::RadiusDamage_limit( tr.vecEndPos, pev, pevAttacker, 150, 250, CLASS_NONE, DMG_BLAST);
					}
					else if(iDamage == 514){
						FX_Trail( tr.vecEndPos + (tr.vecPlaneNormal * 15), entindex(), PROJ_M203_DETONATE );
						::RadiusDamage_limit( tr.vecEndPos, pev, pevAttacker, 100, 250, CLASS_NONE, DMG_BLAST);
					}
					else{
						FX_Explosion( tr.vecEndPos + (tr.vecPlaneNormal * 15), EXPLOSION_SATCHEL );
						::RadiusDamage_limit( tr.vecEndPos, pev, pevAttacker, 120, 250, CLASS_NONE, DMG_BLAST);
					}

					ClearMultiDamage();//�౶���?
					//pEntity->TraceAttack(pevAttacker, 40, vecDir, &tr, DMG_BULLET);

					MESSAGE_BEGIN( MSG_PAS, SVC_TEMPENTITY, tr.vecEndPos );
							WRITE_BYTE( TE_EXPLOSION);
							WRITE_COORD( tr.vecEndPos.x );
							WRITE_COORD( tr.vecEndPos.y );
							WRITE_COORD( tr.vecEndPos.z );
							WRITE_SHORT( g_sModelIndexFireball );
							WRITE_BYTE( 0 ); // no sprite
							WRITE_BYTE( 15  ); // framerate
							WRITE_BYTE( TE_EXPLFLAG_NONE );
						MESSAGE_END();
					CSoundEnt::InsertSound ( bits_SOUND_COMBAT, pev->origin, NORMAL_EXPLOSION_VOLUME, 3.0 );
					if ( RANDOM_FLOAT( 0 , 1 ) < 0.5 )
					{
						UTIL_DecalTrace( &tr, DECAL_SCORCH1 );
					}
					else
					{
						UTIL_DecalTrace( &tr, DECAL_SCORCH2 );
					}
				}
				break;
			case BULLET_9MM:	
				ThroughWall = 0;
				damage_level = 3;
				pEntity->TraceAttack(pevAttacker, 3, vecDir, &tr, DMG_BULLET);
				TEXTURETYPE_PlaySound(&tr, vecSrc, vecEnd, iBulletType);
				DecalGunshot( &tr, iBulletType );
				break;

			case BULLET_57mm:	
				ThroughWall = 0;
				damage_level = 4;
				pEntity->TraceAttack(pevAttacker, 4, vecDir, &tr, DMG_BULLET);
				TEXTURETYPE_PlaySound(&tr, vecSrc, vecEnd, iBulletType);
				DecalGunshot( &tr, iBulletType );
				break;
			case BULLET_MONSTER_9MM:
				ThroughWall = 0;
				if(iDamage == 623)
				{
					damage_level = 16;
					pEntity->TraceAttack(pevAttacker, 16, vecDir, &tr, DMG_BULLET);
				}
				else if(iDamage == 625)
				{
					damage_level = 10;
					pEntity->TraceAttack(pevAttacker, 10, vecDir, &tr, DMG_BULLET);
				}
				else
				{
					damage_level = 8;
					pEntity->TraceAttack(pevAttacker, 8, vecDir, &tr, DMG_BULLET);
				}
				TEXTURETYPE_PlaySound(&tr, vecSrc, vecEnd, iBulletType);
				DecalGunshot( &tr, iBulletType );
				break;

			case BULLET_MONSTER_MP5:
				ThroughWall = 0;
				damage_level = 5;
				pEntity->TraceAttack( pevAttacker, 5, vecDir, &tr, DMG_BULLET );

				TEXTURETYPE_PlaySound( &tr, vecSrc, vecEnd, iBulletType );
				DecalGunshot( &tr, iBulletType );
				break;
			case BULLET_8mm:	
				ThroughWall = 0;
				if(iDamage == 623)
				{
					damage_level = 15;
					pEntity->TraceAttack(pevAttacker, 15, vecDir, &tr, DMG_BULLET);
				}
				else	
				{
					damage_level = 6;
					pEntity->TraceAttack(pevAttacker, 6, vecDir, &tr, DMG_BULLET);
				}
				TEXTURETYPE_PlaySound(&tr, vecSrc, vecEnd, iBulletType);
				DecalGunshot( &tr, iBulletType );
				break;

			case BULLET_PLAYER_BUCKSHOT:	
				ThroughWall = 0;
				damage_level = 7;
				pEntity->TraceAttack(pevAttacker, 7, vecDir, &tr, DMG_BULLET);
				TEXTURETYPE_PlaySound(&tr, vecSrc, vecEnd, iBulletType);
				DecalGunshot( &tr, iBulletType );
				break;
			case BULLET_12MM:		
				ThroughWall = 0;
				if(iDamage == 623)
				{
					damage_level = 25;
					pEntity->TraceAttack(pevAttacker, 25, vecDir, &tr, DMG_BULLET);
				}
				else if(iDamage == 114)
				{
					damage_level = 15;
					pEntity->TraceAttack(pevAttacker, 13, vecDir, &tr, DMG_BULLET);
				}
				else if(iDamage == 625)
				{
					damage_level = 13;
					pEntity->TraceAttack(pevAttacker, 13, vecDir, &tr, DMG_BULLET);
				}
				else
				{
					damage_level = 10;
					pEntity->TraceAttack(pevAttacker, 10, vecDir, &tr, DMG_BULLET);
				}
				TEXTURETYPE_PlaySound(&tr, vecSrc, vecEnd, iBulletType);
				DecalGunshot( &tr, iBulletType );
				break;
			case BULLET_MONSTER_12MM:
				ThroughWall = 0;
				if(iDamage == 623)
				{
					damage_level = 36;
					pEntity->TraceAttack(pevAttacker, 36, vecDir, &tr, DMG_BULLET);
				}
				else if(iDamage == 625)
				{
					damage_level = 16;
					pEntity->TraceAttack(pevAttacker, 16, vecDir, &tr, DMG_BULLET);
				}
				else
				{
					damage_level = 12;
					pEntity->TraceAttack(pevAttacker, 12, vecDir, &tr, DMG_BULLET);
				}
				TEXTURETYPE_PlaySound(&tr, vecSrc, vecEnd, iBulletType);
				DecalGunshot( &tr, iBulletType );
				break;
			case BULLET_556:	
				ThroughWall = 0;
				damage_level = 20;
				pEntity->TraceAttack(pevAttacker, 20, vecDir, &tr, DMG_BULLET); 
				TEXTURETYPE_PlaySound(&tr, vecSrc, vecEnd, iBulletType);
				DecalGunshot( &tr, iBulletType );
				break;
			case BULLET_14MM:
				ThroughWall = 0;
				damage_level = 25;
				pEntity->TraceAttack(pevAttacker, 25, vecDir, &tr, DMG_BULLET); 
				TEXTURETYPE_PlaySound(&tr, vecSrc, vecEnd, iBulletType);
				DecalGunshot( &tr, iBulletType );
				break;
			
			case BULLET_30mm:	
				ThroughWall = 0;
				if(iDamage == 623)
				{
					damage_level = 90;
					pEntity->TraceAttack(pevAttacker, 90, vecDir, &tr, DMG_BULLET); 
				}
				else
				{
					damage_level = 30;
					pEntity->TraceAttack(pevAttacker, 30, vecDir, &tr, DMG_BULLET); 
				}
				TEXTURETYPE_PlaySound(&tr, vecSrc, vecEnd, iBulletType);
				DecalGunshot( &tr, iBulletType );
				break;

			case 812:	
				ThroughWall = 1;
				damage_level = 180;
				pEntity->TraceAttack(pevAttacker, 180, vecDir, &tr, DMG_BULLET); 
				TEXTURETYPE_PlaySound(&tr, vecSrc, vecEnd, iBulletType);
				DecalGunshot( &tr, iBulletType );
				break;

			case 814:	
				{
					if ( (pEntity->pev->flags & FL_MONSTER)  )
					{
						CBaseMonster *pEnemyMonster;
						pEnemyMonster = pEntity->MyMonsterPointer();
						if(pEnemyMonster)
						{
							pEnemyMonster->Freeze_Monster(40);
							FX_Explosion(tr.vecEndPos, 48);
						}
					}
				}
				break;

			case BULLET_338Magnum:	
				ThroughWall = 1;
				if(iDamage == 623)
				{
					damage_level = 135;
					pEntity->TraceAttack(pevAttacker, 135, vecDir, &tr, DMG_BULLET); 
				}
				else
				{
					damage_level = 45;
					pEntity->TraceAttack(pevAttacker, 45, vecDir, &tr, DMG_BULLET); 
				}
				TEXTURETYPE_PlaySound(&tr, vecSrc, vecEnd, iBulletType);
				DecalGunshot( &tr, iBulletType );
				break;

				/*
			case BULLET_338Magnum_EX:	
				ThroughWall = 0;
				damage_level = 50;
				pEntity->TraceAttack(pevAttacker, 50, vecDir, &tr, DMG_BULLET); 
				TEXTURETYPE_PlaySound(&tr, vecSrc, vecEnd, iBulletType);
				DecalGunshot( &tr, iBulletType );
				break;*/

			case BULLET_GAUSS_HEV:	
				ThroughWall = 3;
				damage_level = 120;
				gMultiDamage.type = 0;
				pEntity->TraceAttack(pevAttacker, 120, vecDir, &tr, DMG_ENERGYBEAM); 
				FX_Explosion( tr.vecEndPos, EXPLOSION_BEAM_FLESHIMPACT);
				FX_Explosion( tr.vecEndPos+(tr.vecPlaneNormal * 12), EXPLOSION_LIGHTSABER );
				TEXTURETYPE_PlaySound(&tr, vecSrc, vecEnd, iBulletType);
				DecalGunshot( &tr, iBulletType );
				break;

			case BULLET_IONTURRET:	
				if(iDamage == 623)
				{
					ThroughWall = 4;
					damage_level = 600;
					gMultiDamage.type = 0;
					pEntity->TraceAttack(pevAttacker, damage_level, vecDir, &tr, DMG_ENERGYBLAST | DMG_UNKNOWBLAST); 
					FX_Explosion( tr.vecEndPos+(tr.vecPlaneNormal * 15), 138);
				}
				else{
					ThroughWall = 3;
					damage_level = 150;
					gMultiDamage.type = 0;
					pEntity->TraceAttack(pevAttacker, 150, vecDir, &tr, DMG_ENERGYBEAM); 
					FX_Explosion( tr.vecEndPos, EXPLOSION_BEAM_FLESHIMPACT);
					FX_Explosion( tr.vecEndPos+(tr.vecPlaneNormal * 12), EXPLOSION_LIGHTSABER );
					TEXTURETYPE_PlaySound(&tr, vecSrc, vecEnd, iBulletType);
					DecalGunshot( &tr, iBulletType );
				}
				break;

			case BULLET_DOMA_LASER:	
				ThroughWall = 4;
				damage_level = 600;
				gMultiDamage.type = 0;
				pEntity->TraceAttack(pevAttacker, 600, vecDir, &tr, DMG_ENERGYBLAST | DMG_UNKNOWBLAST); 
				FX_Explosion( tr.vecEndPos+(tr.vecPlaneNormal * 15), 150);
				TEXTURETYPE_PlaySound(&tr, vecSrc, vecEnd, iBulletType);
				DecalGunshot( &tr, iBulletType );
				break;

			case BULLET_GMAN_LASER:	
				ThroughWall = 4;
				damage_level = 600;
				gMultiDamage.type = 0;
				pEntity->TraceAttack(pevAttacker, 600, vecDir, &tr, DMG_ENERGYBLAST | DMG_UNKNOWBLAST); 
				FX_Explosion( tr.vecEndPos+(tr.vecPlaneNormal * 15), 125);
				TEXTURETYPE_PlaySound(&tr, vecSrc, vecEnd, iBulletType);
				DecalGunshot( &tr, iBulletType );
				break;
			case BULLET_NONE: // FIX
				pEntity->TraceAttack( pevAttacker, 50, vecDir, &tr, DMG_CLUB );
				TEXTURETYPE_PlaySound( &tr, vecSrc, vecEnd, iBulletType );
				// only decal glass
				if( !FNullEnt( tr.pHit ) && VARS( tr.pHit )->rendermode != 0 )
				{
					UTIL_DecalTrace( &tr, DECAL_GLASSBREAK1 + RANDOM_LONG( 0, 2 ) );
				}

				break;
			}

			if(ThroughWall >= 1)
			{
				UTIL_TraceLine(tr.vecEndPos + vecDir * 8, tr.vecEndPos + vecDir * flDistance * 0.15, dont_ignore_monsters, ENT(pEntity->pev)/*pentIgnore*/, &tr2);
				if (tr2.flFraction != 1.0)
				{
					CBaseEntity *pEntity2 = CBaseEntity::Instance(tr2.pHit);
					pEntity2->TraceAttack(pevAttacker, damage_level * 0.5, vecDirShooting, &tr2, DMG_BULLET); 
					TEXTURETYPE_PlaySound(&tr2, vecSrc, vecEnd, iBulletType);
					DecalGunshot( &tr2, iBulletType );

					if(iBulletType == BULLET_GMAN_LASER)
					{
						if(pEntity2->pev->takedamage)
						{
							FX_Explosion( tr2.vecEndPos+(tr2.vecPlaneNormal * 12), EXPLOSION_LIGHTSABER );
						}
					}

					if(ThroughWall >= 2)
					{
						UTIL_TraceLine(tr2.vecEndPos + vecDir * 8, tr2.vecEndPos + vecDir * flDistance * 0.1, dont_ignore_monsters, ENT(pEntity2->pev)/*pentIgnore*/, &tr3);
						if (tr3.flFraction != 1.0)
						{
							CBaseEntity *pEntity3 = CBaseEntity::Instance(tr3.pHit);
							pEntity3->TraceAttack(pevAttacker, damage_level * 0.25, vecDirShooting, &tr3, DMG_BULLET); 
							TEXTURETYPE_PlaySound(&tr3, vecSrc, vecEnd, iBulletType);
							DecalGunshot( &tr3, iBulletType );

							if(iBulletType == BULLET_GMAN_LASER)
							{
								if(pEntity3->pev->takedamage)
								{
									FX_Explosion( tr3.vecEndPos+(tr3.vecPlaneNormal * 12), EXPLOSION_LIGHTSABER );
								}
							}

							if(ThroughWall >= 3)
							{
								UTIL_TraceLine(tr3.vecEndPos + vecDir * 8, tr3.vecEndPos + vecDir * flDistance * 0.05, dont_ignore_monsters, ENT(pEntity3->pev)/*pentIgnore*/, &tr4);
								if (tr4.flFraction != 1.0)
								{
									CBaseEntity *pEntity4 = CBaseEntity::Instance(tr4.pHit);
									pEntity4->TraceAttack(pevAttacker, damage_level * 0.1, vecDirShooting, &tr4, DMG_BULLET); 
									TEXTURETYPE_PlaySound(&tr4, vecSrc, vecEnd, iBulletType);
									DecalGunshot( &tr4, iBulletType );

									if(iBulletType == BULLET_GMAN_LASER)
									{
										if(pEntity4->pev->takedamage)
										{
											FX_Explosion( tr4.vecEndPos+(tr4.vecPlaneNormal * 12), EXPLOSION_LIGHTSABER );
										}
									}

								}
							}
						}
					}
				}
			}
		}
		// make bullet trails
		UTIL_BubbleTrail( vecSrc, tr.vecEndPos, (int)( ( flDistance * tr.flFraction ) / 64.0f ) );
	}
	ApplyMultiDamage( pev, pevAttacker );
}

/*
================
FireBullets

Go to the trouble of combining multiple pellets into a single damage call.

This version is used by Players, uses the random seed generator to sync client and server side shots.
================
*/
Vector CBaseEntity::FireBulletsPlayer( ULONG cShots, Vector vecSrc, Vector vecDirShooting, Vector vecSpread, float flDistance, int iBulletType, int iTracerFreq, int iDamage, entvars_t *pevAttacker, int shared_rand )
{
	static int tracerCount;
	TraceResult tr,tr2,tr3,tr4;
	Vector vecRight = gpGlobals->v_right;
	Vector vecUp = gpGlobals->v_up;
	float x, y, z;
	//float z;

	if( pevAttacker == NULL )
		pevAttacker = pev;  // the default attacker is ourselves

	ClearMultiDamage();
	gMultiDamage.type = DMG_BULLET | DMG_NEVERGIB;

	for( ULONG iShot = 1; iShot <= cShots; iShot++ )
	{
		//Use player's random seed.
		// get circular gaussian spread
		x = UTIL_SharedRandomFloat( shared_rand + iShot, -0.5f, 0.5f ) + UTIL_SharedRandomFloat( shared_rand + ( 1 + iShot ) , -0.5f, 0.5f );
		y = UTIL_SharedRandomFloat( shared_rand + ( 2 + iShot ), -0.5f, 0.5f ) + UTIL_SharedRandomFloat( shared_rand + ( 3 + iShot ), -0.5f, 0.5f );
		//z = x * x + y * y;

		Vector vecDir = vecDirShooting +
						x * vecSpread.x * vecRight +
						y * vecSpread.y * vecUp;
		Vector vecEnd;

		vecEnd = vecSrc + vecDir * flDistance;
		UTIL_TraceLine( vecSrc, vecEnd, dont_ignore_monsters, ENT( pev )/*pentIgnore*/, &tr );

		// do damage, paint decals
		if( tr.flFraction != 1.0f )
		{
			CBaseEntity *pEntity = CBaseEntity::Instance( tr.pHit );

			/*vecDir = vecDir.Normalize();

			if( iDamage )
			{
				pEntity->TraceAttack( pevAttacker, iDamage, vecDir, &tr, DMG_BULLET | ( ( iDamage > 16 ) ? DMG_ALWAYSGIB : DMG_NEVERGIB ) );

				TEXTURETYPE_PlaySound( &tr, vecSrc, vecEnd, iBulletType );
				DecalGunshot( &tr, iBulletType );
			}*/

			int ThroughWall = 0;
			int tex = (int)TEXTURETYPE_Trace(&tr, vecSrc, vecEnd);
			float damage_level;

			int HearvyHit = 0;
			if(iBulletType == BULLET_PLAYER_357 || iBulletType == BULLET_50AE)
			{
				HearvyHit = 1;
			}
			else if(iBulletType == BULLET_338Magnum)
			{
				HearvyHit = 2;
			}

			int surface = (int)SURFACETYPE_Trace(&tr, vecSrc, vecEnd,Classify(),HearvyHit);

			if(iBulletType != 628)
			{
				FX_ImpBullet( tr.vecEndPos, tr.vecPlaneNormal, vecSrc, surface, iBulletType, (float)tex );
			}
			
			switch( iBulletType )
			{
			default:
			case BULLET_PLAYER_9MM:
				ThroughWall = 0;
				damage_level = 10;
				pEntity->TraceAttack( pevAttacker, 10, vecDir, &tr, DMG_BULLET );
				TEXTURETYPE_PlaySound(&tr, vecSrc, vecEnd, iBulletType);
				DecalGunshot( &tr, iBulletType );
				break;
			case BULLET_PLAYER_MP5:
			ThroughWall = 0;
				damage_level = 8;
				pEntity->TraceAttack( pevAttacker, 8, vecDir, &tr, DMG_BULLET );
				TEXTURETYPE_PlaySound(&tr, vecSrc, vecEnd, iBulletType);
				DecalGunshot( &tr, iBulletType );
				break;
			case BULLET_556Nato:
				ThroughWall = 0;
				damage_level = 12;
				pEntity->TraceAttack(pevAttacker, 12, vecDir, &tr, DMG_BULLET); 
				TEXTURETYPE_PlaySound(&tr, vecSrc, vecEnd, iBulletType);
				DecalGunshot( &tr, iBulletType );
				break;
			case BULLET_PLAYER_BUCKSHOT:
				ThroughWall = 0;
				if(iDamage == 623)
				{
					damage_level = 9;
					pEntity->TraceAttack( pevAttacker, 9, vecDir, &tr, DMG_BULLET ); 
				}
				else
				{
					damage_level = 7;
					pEntity->TraceAttack( pevAttacker, 7, vecDir, &tr, DMG_BULLET ); 
				}
				TEXTURETYPE_PlaySound( &tr, vecSrc, vecEnd, iBulletType );
				DecalGunshot( &tr, iBulletType );
				break;
			case BULLET_556:
				ThroughWall = 0;
				damage_level = 25;
				pEntity->TraceAttack(pevAttacker, 25, vecDir, &tr, DMG_BULLET); 
				TEXTURETYPE_PlaySound(&tr, vecSrc, vecEnd, iBulletType);
				DecalGunshot( &tr, iBulletType );
				break;

			case BULLET_762:
				ThroughWall = 0;
				damage_level = 16;
				pEntity->TraceAttack(pevAttacker, 16, vecDir, &tr, DMG_BULLET); 
				TEXTURETYPE_PlaySound(&tr, vecSrc, vecEnd, iBulletType);
				DecalGunshot( &tr, iBulletType );
				break;

			case BULLET_762Nato:
				ThroughWall = 0;
				damage_level = 20;
				pEntity->TraceAttack(pevAttacker, 20, vecDir, &tr, DMG_BULLET); 
				TEXTURETYPE_PlaySound(&tr, vecSrc, vecEnd, iBulletType);
				DecalGunshot( &tr, iBulletType );
				break;
			case BULLET_PLAYER_357:
				ThroughWall = 0;
				damage_level = 60;
				pEntity->TraceAttack( pevAttacker, 60, vecDir, &tr, DMG_BULLET );
				TEXTURETYPE_PlaySound(&tr, vecSrc, vecEnd, iBulletType);
				DecalGunshot( &tr, iBulletType );
				break;
			case BULLET_50AE:
				ThroughWall = 0;
				damage_level = 45;
				pEntity->TraceAttack(pevAttacker, 45, vecDir, &tr, DMG_BULLET); 
				TEXTURETYPE_PlaySound(&tr, vecSrc, vecEnd, iBulletType);
				DecalGunshot( &tr, iBulletType );
				break;

			case BULLET_338Magnum:
				ThroughWall = 1;
				damage_level = 180;
				pEntity->TraceAttack(pevAttacker, 180, vecDir, &tr, DMG_BULLET); 
				TEXTURETYPE_PlaySound(&tr, vecSrc, vecEnd, iBulletType);
				DecalGunshot( &tr, iBulletType );
				break;

			case 628:
				{
					ThroughWall = 0;
					damage_level = 0;

					FX_Explosion( tr.vecEndPos + (tr.vecPlaneNormal * 15), EXPLOSION_C4 );
					//EMIT_SOUND(ENT(pev), CHAN_STATIC, "weapons/mortarhit.wav", 1.0, 0.3);
					::RadiusDamage_limit( tr.vecEndPos, pev, pevAttacker, 320, 640, CLASS_NONE, DMG_BLAST);
					ClearMultiDamage();

					MESSAGE_BEGIN( MSG_PAS, SVC_TEMPENTITY, tr.vecEndPos );
							WRITE_BYTE( TE_EXPLOSION);
							WRITE_COORD( tr.vecEndPos.x );
							WRITE_COORD( tr.vecEndPos.y );
							WRITE_COORD( tr.vecEndPos.z );
							WRITE_SHORT( g_sModelIndexFireball );
							WRITE_BYTE( 0 ); // no sprite
							WRITE_BYTE( 15  ); // framerate
							WRITE_BYTE( TE_EXPLFLAG_NONE );
						MESSAGE_END();

					CSoundEnt::InsertSound ( bits_SOUND_COMBAT, pev->origin, NORMAL_EXPLOSION_VOLUME, 3.0 );
					if ( RANDOM_FLOAT( 0 , 1 ) < 0.5 )
					{
						UTIL_DecalTrace( &tr, DECAL_SCORCH1 );
					}
					else
					{
						UTIL_DecalTrace( &tr, DECAL_SCORCH2 );
					}
				}
				break;
			case BULLET_NONE: // FIX
				pEntity->TraceAttack( pevAttacker, 50, vecDir, &tr, DMG_CLUB );
				TEXTURETYPE_PlaySound( &tr, vecSrc, vecEnd, iBulletType );
				// only decal glass
				if( !FNullEnt( tr.pHit ) && VARS( tr.pHit )->rendermode != 0 )
				{
					UTIL_DecalTrace( &tr, DECAL_GLASSBREAK1 + RANDOM_LONG( 0, 2 ) );
				}

				break;
			}

			if(ThroughWall >= 1)
			{
				UTIL_TraceLine(tr.vecEndPos + vecDir * 8, tr.vecEndPos + vecDir * flDistance * 0.15, dont_ignore_monsters, ENT(pEntity->pev)/*pentIgnore*/, &tr2);
				if (tr2.flFraction != 1.0)
				{
					CBaseEntity *pEntity2 = CBaseEntity::Instance(tr2.pHit);
					pEntity2->TraceAttack(pevAttacker, damage_level * 0.5, vecDirShooting, &tr2, DMG_BULLET); 
					TEXTURETYPE_PlaySound(&tr2, vecSrc, vecEnd, iBulletType);
					DecalGunshot( &tr2, iBulletType );
					if(ThroughWall >= 2)
					{
						UTIL_TraceLine(tr2.vecEndPos + vecDir * 8, tr2.vecEndPos + vecDir * flDistance * 0.1, dont_ignore_monsters, ENT(pEntity2->pev)/*pentIgnore*/, &tr3);
						if (tr3.flFraction != 1.0)
						{
							CBaseEntity *pEntity3 = CBaseEntity::Instance(tr3.pHit);
							pEntity3->TraceAttack(pevAttacker, damage_level * 0.25, vecDirShooting, &tr3, DMG_BULLET); 
							TEXTURETYPE_PlaySound(&tr3, vecSrc, vecEnd, iBulletType);
							DecalGunshot( &tr3, iBulletType );
						
							if(ThroughWall >= 3)
							{
								UTIL_TraceLine(tr3.vecEndPos + vecDir * 8, tr3.vecEndPos + vecDir * flDistance * 0.05, dont_ignore_monsters, ENT(pEntity3->pev)/*pentIgnore*/, &tr4);
								if (tr4.flFraction != 1.0)
								{
									CBaseEntity *pEntity4 = CBaseEntity::Instance(tr4.pHit);
									pEntity4->TraceAttack(pevAttacker, damage_level * 0.125, vecDirShooting, &tr4, DMG_BULLET); 
									TEXTURETYPE_PlaySound(&tr4, vecSrc, vecEnd, iBulletType);
									DecalGunshot( &tr4, iBulletType );
								}
							}
						}
					}
				}
			}
		}
		// make bullet trails
		UTIL_BubbleTrail( vecSrc, tr.vecEndPos, (int)( ( flDistance * tr.flFraction ) / 64.0f ) );
	}
	ApplyMultiDamage( pev, pevAttacker );

	return Vector( x * vecSpread.x, y * vecSpread.y, 0.0 );
}

void CBaseEntity::TraceBleed( float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType )
{
	if( BloodColor() == DONT_BLEED )
		return;

	if( flDamage == 0 )
		return;

	if( !( bitsDamageType & ( DMG_CRUSH | DMG_BULLET | DMG_SLASH | DMG_BLAST | DMG_CLUB | DMG_BURN ) ) )
		return;

	// make blood decal on the wall! 
	TraceResult Bloodtr;
	Vector vecTraceDir; 
	float flNoise;
	int cCount;
	int i;

/*
	if( !IsAlive() )
	{
		// dealing with a dead monster. 
		if( pev->max_health <= 0 )
		{
			// no blood decal for a monster that has already decalled its limit.
			return; 
		}
		else
		{
			pev->max_health--;
		}
	}
*/
	if( flDamage < 10.0f )
	{
		flNoise = 0.1f;
		cCount = 1;
	}
	else if( flDamage < 25.0f )
	{
		flNoise = 0.2f;
		cCount = 2;
	}
	else
	{
		flNoise = 0.3f;
		cCount = 4;
	}

	for( i = 0; i < cCount; i++ )
	{
		vecTraceDir = vecDir * -1.0f;// trace in the opposite direction the shot came from (the direction the shot is going)

		vecTraceDir.x += RANDOM_FLOAT( -flNoise, flNoise );
		vecTraceDir.y += RANDOM_FLOAT( -flNoise, flNoise );
		vecTraceDir.z += RANDOM_FLOAT( -flNoise, flNoise );

		UTIL_TraceLine( ptr->vecEndPos, ptr->vecEndPos + vecTraceDir * -172.0f, ignore_monsters, ENT( pev ), &Bloodtr );

		if( Bloodtr.flFraction != 1.0f )
		{
			UTIL_BloodDecalTrace( &Bloodtr, BloodColor() );
		}
	}
}

//=========================================================
//=========================================================
void CBaseMonster::MakeDamageBloodDecal( int cCount, float flNoise, TraceResult *ptr, const Vector &vecDir )
{
	// make blood decal on the wall! 
	TraceResult Bloodtr;
	Vector vecTraceDir; 
	int i;

	/*if( !IsAlive() )
	{
		// dealing with a dead monster. 
		if( pev->max_health <= 0 )
		{
			// no blood decal for a monster that has already decalled its limit.
			return; 
		}
		else
		{
			pev->max_health--;
		}
	}*/

	for( i = 0; i < cCount; i++ )
	{
		vecTraceDir = vecDir;

		vecTraceDir.x += RANDOM_FLOAT( -flNoise, flNoise );
		vecTraceDir.y += RANDOM_FLOAT( -flNoise, flNoise );
		vecTraceDir.z += RANDOM_FLOAT( -flNoise, flNoise );

		UTIL_TraceLine( ptr->vecEndPos, ptr->vecEndPos + vecTraceDir * 172.0f, ignore_monsters, ENT( pev ), &Bloodtr );

/*
		MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
			WRITE_BYTE( TE_SHOWLINE);
			WRITE_COORD( ptr->vecEndPos.x );
			WRITE_COORD( ptr->vecEndPos.y );
			WRITE_COORD( ptr->vecEndPos.z );

			WRITE_COORD( Bloodtr.vecEndPos.x );
			WRITE_COORD( Bloodtr.vecEndPos.y );
			WRITE_COORD( Bloodtr.vecEndPos.z );
		MESSAGE_END();
*/

		if( Bloodtr.flFraction != 1.0f )
		{
			UTIL_BloodDecalTrace( &Bloodtr, BloodColor() );
		}
	}
}
