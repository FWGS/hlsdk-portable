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
// barnacle - stationary ceiling mounted 'fishing' monster
//=========================================================

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"schedule.h"
#include	"player.h"
#include	"weapons.h"

#define	BARNACLE_BODY_HEIGHT	36 // how 'tall' the barnacle's model is.
#define BARNACLE_KILL_VICTIM_DELAY	3 // how many seconds after pulling prey in to gib them. 

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define	BARNACLE_AE_PUKEGIB	2

class CBarnacle_Holy : public CBaseMonster
{
public:
	void Spawn( void );
	void Precache( void );
	CBaseEntity *TongueTouchEnt ( float *pflLength );
	int  Classify ( void );
	void EXPORT BarnacleThink ( void );
	void EXPORT WaitTillDead ( void );
	void Killed( entvars_t *pevAttacker, int iGib );
	int TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType );
	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	float m_flAltitude;
	float m_flKillVictimTime;
	int	  m_cGibs;// barnacle loads up on gibs each time it kills something.
	BOOL  m_fTongueExtended;
	BOOL  m_fLiftingPrey;
	float m_flTongueAdj;
	BOOL  m_ion;
	BOOL  m_eatkey;
};
LINK_ENTITY_TO_CLASS( monster_barnacle_holy, CBarnacle_Holy );

TYPEDESCRIPTION	CBarnacle_Holy::m_SaveData[] = 
{
	DEFINE_FIELD( CBarnacle_Holy, m_eatkey, FIELD_BOOLEAN ),
	DEFINE_FIELD( CBarnacle_Holy, m_ion, FIELD_BOOLEAN ),
	DEFINE_FIELD( CBarnacle_Holy, m_flAltitude, FIELD_FLOAT ),
	DEFINE_FIELD( CBarnacle_Holy, m_flKillVictimTime, FIELD_TIME ),
	DEFINE_FIELD( CBarnacle_Holy, m_cGibs, FIELD_INTEGER ),// barnacle loads up on gibs each time it kills something.
	DEFINE_FIELD( CBarnacle_Holy, m_fTongueExtended, FIELD_BOOLEAN ),
	DEFINE_FIELD( CBarnacle_Holy, m_fLiftingPrey, FIELD_BOOLEAN ),
	DEFINE_FIELD( CBarnacle_Holy, m_flTongueAdj, FIELD_FLOAT ),
};

IMPLEMENT_SAVERESTORE( CBarnacle_Holy, CBaseMonster );


//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int	CBarnacle_Holy :: Classify ( void )
{
	return	CLASS_ALIEN_MONSTER;
}

//=========================================================
// Spawn
//=========================================================
void CBarnacle_Holy :: Spawn()
{
	Precache( );

	SET_MODEL(ENT(pev), "models/barnacle_holy.mdl");
	UTIL_SetSize( pev, Vector(-16, -16, -32), Vector(16, 16, 0) );

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_FLY;
	pev->takedamage		= DAMAGE_AIM;
	m_bloodColor		= BLOOD_COLOR_YELLOW;
	pev->effects		= EF_INVLIGHT; // take light from the ceiling 
	pev->health			= 40;
	m_flFieldOfView		= 0.5;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState		= MONSTERSTATE_NONE;
	m_flKillVictimTime	= 0;
	m_cGibs				= 0;
	m_fLiftingPrey		= FALSE;
	m_flTongueAdj		= -100;

	InitBoneControllers();

	SetActivity ( ACT_IDLE );

	pev->renderfx		= 255;
	pev->armortype		= 1;

	SetThink ( &CBarnacle_Holy::BarnacleThink );
	pev->nextthink = gpGlobals->time + 0.5;

	UTIL_SetOrigin ( pev, pev->origin );
	pev->netname = MAKE_STRING( "Holy.Barnacle" );
}

int CBarnacle_Holy::TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	return CBaseMonster::TakeDamage( pevInflictor, pevAttacker, flDamage, bitsDamageType );
}

//=========================================================
//=========================================================
void CBarnacle_Holy :: BarnacleThink ( void )
{
	CBaseEntity *pTouchEnt;
	CBaseMonster *pVictim;
	float flLength;

	pev->nextthink = gpGlobals->time + 0.03;

	/*
	if(m_hEnemy != NULL && !m_fLiftingPrey){
		UTIL_MakeVectors ( pev->angles );
		TraceResult tr;
		Vector vecspot = pev->origin + Vector(0,0,30);
		UTIL_TraceLine( vecspot, vecspot + gpGlobals->v_forward * 20, dont_ignore_monsters, ENT( pev ), &tr );
		if ( tr.flFraction != 1.0 )
		{
			pev->velocity = g_vecZero;
		}
		else{
			pev->velocity = gpGlobals->v_forward * 250;
		}
	}
	else{
		UTIL_MakeVectors ( pev->angles );
		TraceResult tr;
		Vector vecspot = pev->origin + Vector(0,0,30);
		UTIL_TraceLine( vecspot, vecspot + gpGlobals->v_forward * -20, dont_ignore_monsters, ENT( pev ), &tr );
		if ( tr.flFraction != 1.0 )
		{
			pev->velocity = g_vecZero;
		}
		else{
			pev->velocity = gpGlobals->v_forward * -500;
		}
	}
	*/

	if ( m_hEnemy != NULL )
	{
// barnacle has prey.
		if ( !m_hEnemy->IsAlive() )
		{
			m_ion = 1;
			// someone (maybe even the barnacle) killed the prey. Reset barnacle.
			m_fLiftingPrey = FALSE;// indicate that we're not lifting prey.
			m_hEnemy = NULL;
			return;
		}
		
				if(m_hEnemy->pev->flags & FL_CLIENT ){
					CBasePlayer *player = GetClassPtr((CBasePlayer *)m_hEnemy->pev);
						if(player->m_barnacle_RTP_bar >= 255){
						TakeDamage ( pev, pev, 300, DMG_GENERIC );
						m_fLiftingPrey = FALSE;// indicate that we're not lifting prey.
						m_hEnemy = NULL;
						player->m_barnacle_RTP_relase = 1;
						player->m_barnacle_draw_time = gpGlobals->time + 0.5;//0.5����ǹ���
						player->m_barnacle_RTP = 0;
						player->m_barnacle_RTP_bar = 0;
						player->m_barnacle_Level = 0;
						player->m_barnacle_catchme = NULL;
						return;
						}
				}

		if ( m_fLiftingPrey )
		{
			if ( m_hEnemy != NULL && m_hEnemy->pev->deadflag != DEAD_NO )
			{
				// crap, someone killed the prey on the way up.
				m_hEnemy = NULL;
				m_fLiftingPrey = FALSE;
				return;
			}

	// still pulling prey.
			Vector vecNewEnemyOrigin = m_hEnemy->pev->origin;
			vecNewEnemyOrigin.x = pev->origin.x;
			vecNewEnemyOrigin.y = pev->origin.y;

			// guess as to where their neck is
			vecNewEnemyOrigin.x -= 6 * cos(m_hEnemy->pev->angles.y * M_PI/180.0);	
			vecNewEnemyOrigin.y -= 6 * sin(m_hEnemy->pev->angles.y * M_PI/180.0);

			m_flAltitude -= 3;
			vecNewEnemyOrigin.z += 3;
			
				Vector vecArmPos,vecArmDir;
				GetBonePosition( 0, vecArmPos, vecArmDir );
				vecNewEnemyOrigin.x = vecArmPos.x;
				vecNewEnemyOrigin.y = vecArmPos.y;

			if ( fabs( pev->origin.z - ( vecNewEnemyOrigin.z + m_hEnemy->pev->view_ofs.z ) ) < BARNACLE_BODY_HEIGHT )
			{
		// prey has just been lifted into position ( if the victim origin + eye height + 8 is higher than the bottom of the barnacle, it is assumed that the head is within barnacle's body )
				m_fLiftingPrey = FALSE;

				EMIT_SOUND( ENT(pev), CHAN_WEAPON, "barnacle/bcl_bite3.wav", 1, ATTN_NORM );	

				pVictim = m_hEnemy->MyMonsterPointer();

				m_flKillVictimTime = gpGlobals->time + 20;// now that the victim is in place, the killing bite will be administered in 10 seconds.

				if ( pVictim )
				{
					pVictim->BarnacleVictimBitten( pev );
					SetActivity ( ACT_EAT );

					if ( !(pev->effects & EF_NODRAW) ){
					pVictim->TakeDamage ( pev, pev, 25, DMG_SLASH);
					}

					if ( !m_eatkey && FClassnameIs( pVictim->pev,"monster_eatkey")){
					UTIL_Sparks( pVictim->pev->origin );
					pVictim->Killed( NULL, GIB_NEVER );
					//UTIL_SetOrigin ( pVictim->pev, vecNewEnemyOrigin + Vector(0,0,8192) );
					//m_hEnemy = NULL;
					m_eatkey = TRUE;
					return;
					}
				}
			}

			UTIL_SetOrigin ( m_hEnemy->pev, vecNewEnemyOrigin );
		}
		else
		{
	// prey is lifted fully into feeding position and is dangling there.

			pVictim = m_hEnemy->MyMonsterPointer();

			Vector vecArmPos,vecArmDir;
			GetBonePosition( 0, vecArmPos, vecArmDir );
			Vector vecNewEnemyOrigin = m_hEnemy->pev->origin;
			vecNewEnemyOrigin.x = vecArmPos.x;
			vecNewEnemyOrigin.y = vecArmPos.y;
			UTIL_SetOrigin ( m_hEnemy->pev, vecNewEnemyOrigin );

			if ( m_flKillVictimTime != -1 && gpGlobals->time > m_flKillVictimTime )
			{
				// kill!
				if ( pVictim )
				{
					pVictim->TakeDamage ( pev, pev, pVictim->pev->health, DMG_SLASH | DMG_ALWAYSGIB );
					m_cGibs = 3;
				}

				return;
			}

			// bite prey every once in a while
			if ( pVictim )
			{
				pVictim->BarnacleVictimBitten( pev );
			}

		}
	}
	else
	{
// barnacle has no prey right now, so just idle and check to see if anything is touching the tongue.

		// If idle and no nearby client, don't think so often
		if ( FNullEnt( FIND_CLIENT_IN_PVS( edict() ) ) )
			pev->nextthink = gpGlobals->time + RANDOM_FLOAT(0.5,0.75);	// Stagger a bit to keep barnacles from thinking on the same frame

		if ( m_fSequenceFinished )
		{// this is done so barnacle will fidget.
			SetActivity ( ACT_IDLE );
			m_flTongueAdj = -100;
		}

		pTouchEnt = TongueTouchEnt( &flLength );

		if ( pTouchEnt != NULL && m_fTongueExtended )
		{
			// tongue is fully extended, and is touching someone.
			if ( pTouchEnt->FBecomeProne() )
			{
				if(pTouchEnt->pev->flags & FL_CLIENT ){
					CBasePlayer *player = GetClassPtr((CBasePlayer *)pTouchEnt->pev);
					if(player->m_barnacle_RTP == 0 && player->m_barnacle_god_time <= gpGlobals->time){
					player->m_barnacle_RTP = 1;
					player->m_barnacle_Level = 1;
					player->m_barnacle_catchme = this;
					player->pev->punchangle.x += RANDOM_FLOAT(-25, 25);
					player->pev->punchangle.y += RANDOM_FLOAT(-25, 25);
					player->pev->punchangle.z += RANDOM_FLOAT(-25, 25);
					}
				}
				else{
					CBaseMonster *pVictim;
					pVictim = pTouchEnt->MyMonsterPointer();
					if(pVictim){
					pVictim->m_movementGoal = MOVEGOAL_NONE;
						if(FClassnameIs( pVictim->pev,"monster_eatkey")){
						pVictim->pev->angles.x = 90;
						}
					}
				}
				EMIT_SOUND( ENT(pev), CHAN_WEAPON, "barnacle/bcl_alert2.wav", 1, ATTN_NORM );	

				SetSequenceByName ( "attack1" );
				m_flTongueAdj = -20;

				m_hEnemy = pTouchEnt;

				pTouchEnt->pev->movetype = MOVETYPE_FLY;
				pTouchEnt->pev->velocity = g_vecZero;
				pTouchEnt->pev->basevelocity = g_vecZero;
				pTouchEnt->pev->origin.x = pev->origin.x;
				pTouchEnt->pev->origin.y = pev->origin.y;

				m_fLiftingPrey = TRUE;// indicate that we should be lifting prey.
				m_flKillVictimTime = -1;// set this to a bogus time while the victim is lifted.

				m_flAltitude = (pev->origin.z - pTouchEnt->EyePosition().z);
			}
		}
		else
		{
			// calculate a new length for the tongue to be clear of anything else that moves under it. 
			if ( m_flAltitude < flLength )
			{
				// if tongue is higher than is should be, lower it kind of slowly.
				m_flAltitude += 2;
				m_fTongueExtended = FALSE;
			}
			else
			{
				m_flAltitude = flLength;
				m_fTongueExtended = TRUE;
			}

		}

	}

	// ALERT( at_console, "tounge %f\n", m_flAltitude + m_flTongueAdj );
	SetBoneController( 0, -(m_flAltitude + m_flTongueAdj) );
	StudioFrameAdvance( 0.1 );
}

//=========================================================
// Killed.
//=========================================================
void CBarnacle_Holy :: Killed( entvars_t *pevAttacker, int iGib )
{
	CBaseMonster *pVictim;

	pev->solid = SOLID_NOT;
	pev->takedamage = DAMAGE_NO;

	if(m_eatkey){
	m_eatkey = FALSE;
	CBaseEntity *pChild = CBaseEntity::Create( "monster_eatkey", pev->origin, pev->angles, edict() );
	pChild->pev->owner = ENT( pev );
	pChild->pev->spawnflags |= SF_MONSTER_FALL_TO_GROUND;
	}

	if ( m_hEnemy != NULL )
	{
		pVictim = m_hEnemy->MyMonsterPointer();

		if ( pVictim )
		{
			pVictim->BarnacleVictimReleased();
		}

				if(m_hEnemy->pev->flags & FL_CLIENT ){
					CBasePlayer *player = GetClassPtr((CBasePlayer *)m_hEnemy->pev);
					if(player->m_barnacle_RTP != 0){
						player->m_barnacle_RTP_relase = 1;
						player->m_barnacle_draw_time = gpGlobals->time + 0.5;//0.5����ǹ���
						player->m_barnacle_RTP = 0;
						player->m_barnacle_RTP_bar = 0;
						player->m_barnacle_Level = 0;
						player->m_barnacle_catchme = NULL;
					}
				}
	}

	switch ( RANDOM_LONG ( 0, 1 ) )
	{
	case 0:	EMIT_SOUND( ENT(pev), CHAN_WEAPON, "barnacle/bcl_die1.wav", 1, ATTN_NORM );	break;
	case 1:	EMIT_SOUND( ENT(pev), CHAN_WEAPON, "barnacle/bcl_die3.wav", 1, ATTN_NORM );	break;
	}
	
	SetActivity ( ACT_DIESIMPLE );
	SetBoneController( 0, 0 );

	StudioFrameAdvance( 0.1 );

	pev->nextthink = gpGlobals->time + 0.1;
	SetThink ( &CBarnacle_Holy::WaitTillDead );
}

//=========================================================
//=========================================================
void CBarnacle_Holy :: WaitTillDead ( void )
{
	pev->nextthink = gpGlobals->time + 0.1;

	float flInterval = StudioFrameAdvance( 0.1 );
	DispatchAnimEvents ( flInterval );

	if ( m_fSequenceFinished )
	{
		// death anim finished. 
		StopAnimation();
		SetThink ( NULL );
	}
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CBarnacle_Holy :: Precache()
{
	PRECACHE_MODEL("models/barnacle_holy.mdl");
	UTIL_PrecacheOther( "monster_eatkey" );

	PRECACHE_SOUND("barnacle/bcl_alert2.wav");//happy, lifting food up
	PRECACHE_SOUND("barnacle/bcl_bite3.wav");//just got food to mouth
	PRECACHE_SOUND("barnacle/bcl_die1.wav" );
	PRECACHE_SOUND("barnacle/bcl_die3.wav" );
}	

//=========================================================
// TongueTouchEnt - does a trace along the barnacle's tongue
// to see if any entity is touching it. Also stores the length
// of the trace in the int pointer provided.
//=========================================================
#define BARNACLE_CHECK_SPACING	15
CBaseEntity *CBarnacle_Holy :: TongueTouchEnt ( float *pflLength )
{
	TraceResult	tr;
	float		length;

	// trace once to hit architecture and see if the tongue needs to change position.
	UTIL_TraceLine ( pev->origin, pev->origin - Vector ( 0 , 0 , 2048 ), ignore_monsters, ENT(pev), &tr );
	length = fabs( pev->origin.z - tr.vecEndPos.z );
	if ( pflLength )
	{
		*pflLength = length;
	}

	Vector vecArmPos,vecArmDir;
   // GetAttachment( 1, vecArmPos, vecArmDir );
	vecArmPos = pev->origin;

	Vector delta = Vector( BARNACLE_CHECK_SPACING, BARNACLE_CHECK_SPACING, 0 );
	Vector mins = vecArmPos - delta;
	Vector maxs = vecArmPos + delta;

	maxs.z = pev->origin.z;
	mins.z -= length;

	CBaseEntity *pList[10];
	int count = UTIL_EntitiesInBox( pList, 10, mins, maxs, (FL_CLIENT|FL_MONSTER) );
	if ( count )
	{
		for ( int i = 0; i < count; i++ )
		{
			// only clients and monsters
			if ( pList[i] != this && IRelationship( pList[i] ) > R_NO && pList[ i ]->pev->deadflag == DEAD_NO )	// this ent is one of our enemies. Barnacle tries to eat it.
			{
				if(pList[i]->Classify() != CLASS_MACHINE && pList[i]->Classify() != CLASS_MACHINE_ASS
				&& pList[i]->Classify() != CLASS_MACHINE_BLACK){
				return pList[i];
				}
			}
		}
	}

	return NULL;
}
