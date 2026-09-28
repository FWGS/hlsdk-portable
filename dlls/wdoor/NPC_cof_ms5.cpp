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
#include	"soundent.h"

extern DLL_GLOBAL int		g_iSkillLevel;
//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define		ISLAVE_MAX_BEAMS	8

class CCofMs5 : public CBaseMonster
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

	Schedule_t *GetScheduleOfType ( int Type );
	Schedule_t *GetSchedule ( void );

	int	Save( CSave &save ); 
	int Restore( CRestore &restore );
	static TYPEDESCRIPTION m_SaveData[];

	void ClearBeams( );
	void ArmBeam( int side );
	void WackBeam( int side, CBaseEntity *pEntity );
	void ZapBeam( int side );
	void BeamGlow( void );

	int m_iBravery;

	CBeam *m_pBeam[ISLAVE_MAX_BEAMS];

	int m_iBeams;
	float m_flNextAttack;

	void PainSound( void );
	void AlertSound( void );
	void IdleSound( void );
	void AttackSound( void );
	void TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType);

	void DeathSound( void );
	void RunAI( void );

	// No range attacks
	BOOL CheckRangeAttack1 ( float flDot, float flDist );
	BOOL CheckRangeAttack2 ( float flDot, float flDist ) { return FALSE; }
	BOOL CheckMeleeAttack1 ( float flDot, float flDist ) { return FALSE; }
	int TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType );
};

LINK_ENTITY_TO_CLASS( monster_cof_ms5, CCofMs5 );

TYPEDESCRIPTION	CCofMs5::m_SaveData[] = 
{
	DEFINE_FIELD( CCofMs5, m_iBravery, FIELD_INTEGER ),

	DEFINE_ARRAY( CCofMs5, m_pBeam, FIELD_CLASSPTR, ISLAVE_MAX_BEAMS ),
	DEFINE_FIELD( CCofMs5, m_iBeams, FIELD_INTEGER ),
	DEFINE_FIELD( CCofMs5, m_flNextAttack, FIELD_TIME ),
};

IMPLEMENT_SAVERESTORE( CCofMs5, CBaseMonster );

void CCofMs5 :: RunAI( void )
{
	CBaseMonster :: RunAI();
	if(pev->sequence == LookupActivity ( ACT_WALK )
	|| pev->sequence == LookupActivity ( ACT_WALK_SCARED )){
    m_flGroundSpeed = 60;
	}
}

void CCofMs5 :: DeathSound ( void )
{
	EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "cof/st_death.wav", 1, 0.6, 0, 100);
}

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int	CCofMs5 :: Classify ( void )
{
	return	CLASS_HUMAN_ASS;
}

BOOL CCofMs5 :: CheckRangeAttack1 ( float flDot, float flDist )
{
	if (m_flNextAttack > gpGlobals->time)
	{
		if(pev->sequence == LookupActivity ( ACT_WALK_SCARED )){
		m_facing_fucking_mode = 0;
		ClearBeams( );
		pev->sequence = LookupActivity ( ACT_WALK );
		ResetSequenceInfo( );
		pev->frame = 0;
		}
		return FALSE;
	}

			if (flDist <= 1000 && m_hEnemy != NULL && flDot >= 0.5)
			{
				m_ignoreFail = 0;
				if (m_hEnemy->IsAlive() && FVisible( m_hEnemy ) ){
					if(pev->sequence == LookupActivity ( ACT_WALK )  ){
					m_facing_fucking_mode = 1;
					pev->sequence = LookupActivity ( ACT_WALK_SCARED );
					ResetSequenceInfo( );
					pev->frame = 0;
					}
				}
				else if(pev->sequence == LookupActivity ( ACT_WALK_SCARED )){
					m_facing_fucking_mode = 0;
					ClearBeams( );
					pev->sequence = LookupActivity ( ACT_WALK );
					ResetSequenceInfo( );
					pev->frame = 0;
				}
			}
			else if (flDist >= 1250 || m_hEnemy == NULL)
			{
					if(pev->sequence == LookupActivity ( ACT_WALK_SCARED )){
					m_facing_fucking_mode = 0;
					ClearBeams( );
					pev->sequence = LookupActivity ( ACT_WALK );
					ResetSequenceInfo( );
					pev->frame = 0;
					}
			}

	return FALSE;
}

void CCofMs5::TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType)
{
	if(ptr->iHitgroup != 1){
			m_bloodColor		= BLOOD_COLOR_RED;
	}
	else{
			m_zombiehead_health -= flDamage;
			if(m_zombiehead_health <= 0 && pev->health > flDamage){
			pev->health = 0;
			}

			m_bloodColor		= DONT_BLEED;
			UTIL_WhiteSparks( ptr->vecEndPos, ptr->vecPlaneNormal, 9, 5, 5, 100 );//puntos
			UTIL_Sparks( ptr->vecEndPos );

			ptr->iHitgroup = HITGROUP_GENERIC;
	}
	CBaseMonster::TraceAttack( pevAttacker, flDamage, vecDir, ptr, bitsDamageType );
}

Schedule_t* CCofMs5 :: GetScheduleOfType ( int Type )
{
	return CBaseMonster::GetScheduleOfType( Type );
}

//=========================================================
// GetSchedule - Decides which type of schedule best suits
// the monster's current state and conditions. Then calls
// monster's member function to get a pointer to a schedule
// of the proper type.
//=========================================================
Schedule_t *CCofMs5 :: GetSchedule ( void )
{
	return CBaseMonster::GetSchedule();
}
//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CCofMs5 :: SetYawSpeed ( void )
{
	pev->yaw_speed = 90;
}

int CCofMs5 :: TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	if(m_crouchmode == 1){
	return 0;
	}

	return CBaseMonster::TakeDamage( pevInflictor, pevAttacker, flDamage, bitsDamageType );
}

void CCofMs5::Killed( entvars_t *pevAttacker, int iGib )
{
	ClearBeams( );
	CBaseMonster::Killed( pevAttacker, GIB_NEVER );
}

void CCofMs5 :: PainSound( void )
{

}

void CCofMs5 :: AlertSound( void )
{
	
}

void CCofMs5 :: IdleSound( void )
{

}

void CCofMs5 :: AttackSound( void )
{
	
}


//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CCofMs5 :: HandleAnimEvent( MonsterEvent_t *pEvent )
{
	switch( pEvent->event )
	{
		case 3:
		{
			UTIL_MakeAimVectors( pev->angles );

			if (m_iBeams == 0)
			{
				Vector vecSrc = pev->origin + gpGlobals->v_forward * 2;
				MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, vecSrc );
					WRITE_BYTE(TE_DLIGHT);
					WRITE_COORD(vecSrc.x);	// X
					WRITE_COORD(vecSrc.y);	// Y
					WRITE_COORD(vecSrc.z);	// Z
					WRITE_BYTE( 12 );		// radius * 0.1
					WRITE_BYTE( 255 );		// r
					WRITE_BYTE( 255 );		// g
					WRITE_BYTE( 255 );		// b
					WRITE_BYTE( 20 / pev->framerate );		// time * 10
					WRITE_BYTE( 0 );		// decay * 0.1
				MESSAGE_END( );

			}

				ArmBeam( -1 );
				ArmBeam( 1 );
				BeamGlow( );

			EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, "debris/zap4.wav", 1, ATTN_NORM, 0, 100 + m_iBeams * 10 );
		}
		break;

		case 4:
		{
			ClearBeams( );

			ClearMultiDamage();

			UTIL_MakeAimVectors( pev->angles );

			ZapBeam( -1 );
			ZapBeam( 1 );

			EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, "hassault/hw_shoot1.wav", 1, ATTN_NORM, 0, RANDOM_LONG( 130, 160 ) );
			ApplyMultiDamage(pev, pev);

			m_flNextAttack = gpGlobals->time + RANDOM_FLOAT( 1.0, 3.0 );
		}
		break;

		case 5:
		{
			ClearBeams( );
		}
		break;

		case 6:
		{
			FX_Trail(EyePosition(), entindex(), (UTIL_PointContents(pev->origin) == CONTENT_WATER)?PROJ_M203_DETONATE_WATER:PROJ_M203_DETONATE );
			::RadiusDamage2( EyePosition(), pev, pev, 80, 200, CLASS_HUMAN_ASS, DMG_BLAST);

		//pev->takedamage = DAMAGE_NO;
		//pev->solid = SOLID_NOT;

		//pev->effects = EF_NODRAW;

			MESSAGE_BEGIN( MSG_PAS, SVC_TEMPENTITY, pev->origin );
				WRITE_BYTE( TE_EXPLOSION);
				WRITE_COORD( pev->origin.x );
				WRITE_COORD( pev->origin.y );
				WRITE_COORD( pev->origin.z );
				WRITE_SHORT( g_sModelIndexFireball );
				WRITE_BYTE( 0 ); // no sprite
				WRITE_BYTE( 15  ); // framerate
				WRITE_BYTE( TE_EXPLFLAG_NONE );
			MESSAGE_END();

			SetThink ( &CCofMs5::SUB_Remove );
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
void CCofMs5 :: Spawn()
{
	Precache( );
	SET_MODEL(ENT(pev), "models/stranger.mdl");
	UTIL_SetSize( pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX );

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	m_bloodColor		= BLOOD_COLOR_RED;
	
	if (g_iSkillLevel == SKILL_HARD){
	pev->health			= 150;
	m_zombiehead_health = 50;
	}
	else{
	pev->health			= 120;
	m_zombiehead_health = 40;
	}

	pev->view_ofs		= VEC_VIEW;// position of the eyes relative to monster's origin.
	m_flFieldOfView		= 0;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState		= MONSTERSTATE_NONE;
	m_afCapability		= bits_CAP_DOORS_GROUP;

	pev->body           = 0;
	m_aimenemy_mod		= 1;

	MonsterInit();

	m_ignoreFail_MAX = 30;
	m_ignoreFail_OFF = 0;
	m_forcefuckdoor  = TRUE;
	m_MoveFail_FuckRoad = TRUE;
	m_MoveFail_SimpleRoad = TRUE;

	m_killed_exp = 80;
	m_rpgms_level = 40;
	pev->netname = MAKE_STRING( "Dark.TV" );
}
//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CCofMs5 :: Precache()
{
	PRECACHE_MODEL("models/stranger.mdl");
	PRECACHE_MODEL("sprites/lgtning_dark.spr");
	PRECACHE_SOUND("debris/zap1.wav");
	PRECACHE_SOUND("debris/zap4.wav");
	PRECACHE_SOUND("weapons/electro4.wav");
	PRECACHE_SOUND("hassault/hw_shoot1.wav");
	PRECACHE_SOUND("cof/st_death.wav");
}	

int CCofMs5::IgnoreConditions ( void )
{
	int iIgnore = CBaseMonster::IgnoreConditions();

	return iIgnore;
}

//=========================================================
// ArmBeam - small beam from arm to nearby geometry
//=========================================================

void CCofMs5 :: ArmBeam( int side )
{
	TraceResult tr;
	float flDist = 1.0;
	
	if (m_iBeams >= ISLAVE_MAX_BEAMS)
		return;

	UTIL_MakeAimVectors( pev->angles );
	Vector vecSrc = pev->origin + gpGlobals->v_up * 36 + gpGlobals->v_right * side * 16 + gpGlobals->v_forward * 32;

	for (int i = 0; i < 3; i++)
	{
		Vector vecAim = gpGlobals->v_right * side * RANDOM_FLOAT( 0, 1 ) + gpGlobals->v_up * RANDOM_FLOAT( -1, 1 );
		TraceResult tr1;
		UTIL_TraceLine ( vecSrc, vecSrc + vecAim * 768, dont_ignore_monsters, ENT( pev ), &tr1);
		if (flDist > tr1.flFraction)
		{
			tr = tr1;
			flDist = tr.flFraction;
		}
	}

	// Couldn't find anything close enough
	if ( flDist == 1.0 )
		return;

	//DecalGunshot( &tr, BULLET_PLAYER_CROWBAR );

	m_pBeam[m_iBeams] = CBeam::BeamCreate( "sprites/lgtning_dark.spr", 30 );
	if (!m_pBeam[m_iBeams])
		return;

	m_pBeam[m_iBeams]->PointEntInit( tr.vecEndPos, entindex( ) );
	m_pBeam[m_iBeams]->SetEndAttachment( side < 0 ? 2 : 1 );
	m_pBeam[m_iBeams]->SetColor( 255, 255, 255 );
	m_pBeam[m_iBeams]->SetBrightness( 64 );
	m_pBeam[m_iBeams]->SetNoise( 80 );
	m_iBeams++;
}


//=========================================================
// BeamGlow - brighten all beams
//=========================================================
void CCofMs5 :: BeamGlow( )
{
	int b = m_iBeams * 32;
	if (b > 255)
		b = 255;

	for (int i = 0; i < m_iBeams; i++)
	{
		if (m_pBeam[i]->GetBrightness() != 255) 
		{
			m_pBeam[i]->SetBrightness( b );
		}
	}
}


//=========================================================
// WackBeam - regenerate dead colleagues
//=========================================================
void CCofMs5 :: WackBeam( int side, CBaseEntity *pEntity )
{
	Vector vecDest;
	float flDist = 1.0;
	
	if (m_iBeams >= ISLAVE_MAX_BEAMS)
		return;

	if (pEntity == NULL)
		return;

	m_pBeam[m_iBeams] = CBeam::BeamCreate( "sprites/lgtning_dark.spr", 30 );
	if (!m_pBeam[m_iBeams])
		return;

	m_pBeam[m_iBeams]->PointEntInit( pEntity->Center(), entindex( ) );
	m_pBeam[m_iBeams]->SetEndAttachment( side < 0 ? 2 : 1 );
	m_pBeam[m_iBeams]->SetColor( 255, 255, 255 );
	m_pBeam[m_iBeams]->SetBrightness( 255 );
	m_pBeam[m_iBeams]->SetNoise( 80 );
	m_iBeams++;
}

//=========================================================
// ZapBeam - heavy damage directly forward
//=========================================================
void CCofMs5 :: ZapBeam( int side )
{
	Vector vecSrc, vecAim;
	TraceResult tr;
	CBaseEntity *pEntity;

	if (m_iBeams >= ISLAVE_MAX_BEAMS)
		return;

	vecSrc = pev->origin + gpGlobals->v_up * 36;
	vecAim = ShootAtEnemy( vecSrc );
	float deflection = 0.01;
	vecAim = vecAim + side * gpGlobals->v_right * RANDOM_FLOAT( 0, deflection ) + gpGlobals->v_up * RANDOM_FLOAT( -deflection, deflection );
	UTIL_TraceLine ( vecSrc, vecSrc + vecAim * 1200, dont_ignore_monsters, ENT( pev ), &tr);

	m_pBeam[m_iBeams] = CBeam::BeamCreate( "sprites/lgtning_dark.spr", 50 );
	if (!m_pBeam[m_iBeams])
		return;

	m_pBeam[m_iBeams]->PointEntInit( tr.vecEndPos, entindex( ) );
	m_pBeam[m_iBeams]->SetEndAttachment( side < 0 ? 2 : 1 );
	m_pBeam[m_iBeams]->SetColor( 255, 255, 255 );
	m_pBeam[m_iBeams]->SetBrightness( 255 );
	m_pBeam[m_iBeams]->SetNoise( 20 );
	m_iBeams++;

	int dmg;
	dmg = 12;

	pEntity = CBaseEntity::Instance(tr.pHit);
	if (pEntity != NULL && pEntity->pev->takedamage)
	{
		pEntity->TraceAttack( pev, dmg, vecAim, &tr, DMG_SHOCK );
	}
	UTIL_EmitAmbientSound( ENT(pev), tr.vecEndPos, "weapons/electro4.wav", 0.5, ATTN_NORM, 0, RANDOM_LONG( 140, 160 ) );
}


//=========================================================
// ClearBeams - remove all beams
//=========================================================
void CCofMs5 :: ClearBeams( )
{
	for (int i = 0; i < ISLAVE_MAX_BEAMS; i++)
	{
		if (m_pBeam[i])
		{
			UTIL_Remove( m_pBeam[i] );
			m_pBeam[i] = NULL;
		}
	}
	m_iBeams = 0;
	pev->skin = 0;

	STOP_SOUND( ENT(pev), CHAN_WEAPON, "debris/zap4.wav" );
}
