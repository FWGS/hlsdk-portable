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
#if !OEM_BUILD && !HLDEMO_BUILD

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"
#include "soundent.h"
#include "gamerules.h"

enum w_toad_e {
	WTOAD_IDLE1 = 0,
	WTOAD_FIDGET,
	WTOAD_JUMP,
	WTOAD_RUN
};

enum toad_e {
	TOAD_IDLE1 = 0,
	TOAD_IDLE2,
	TOAD_FIDGETFIT,
	TOAD_FIDGETNIP,
	TOAD_DOWN,
	TOAD_UP,
	TOAD_THROW
};

#if !CLIENT_DLL
class CToadGrenade : public CGrenade
{
	void Spawn( void );
	void Precache( void );
	int  Classify( void );
	void EXPORT SuperBounceTouch( CBaseEntity *pOther );
	void EXPORT HuntThink( void );
	int  BloodColor( void ) { return BLOOD_COLOR_YELLOW; }
	void Killed( entvars_t *pevAttacker, int iGib );
	void GibMonster( void );

	virtual int		Save( CSave &save ); 
	virtual int		Restore( CRestore &restore );
	
	static	TYPEDESCRIPTION m_SaveData[];

	static float m_flNextBounceSoundTime;

	// CBaseEntity *m_pTarget;
	float m_flDie;
	Vector m_vecTarget;
	float m_flNextHunt;
	float m_flNextHit;
	Vector m_posPrev;
	EHANDLE m_hOwner;
	int  m_iMyClass;
};

extern int gEvilImpulse101;
float CToadGrenade::m_flNextBounceSoundTime = 0;

LINK_ENTITY_TO_CLASS( monster_toad, CToadGrenade );
TYPEDESCRIPTION	CToadGrenade::m_SaveData[] = 
{
	DEFINE_FIELD( CToadGrenade, m_flDie, FIELD_TIME ),
	DEFINE_FIELD( CToadGrenade, m_vecTarget, FIELD_VECTOR ),
	DEFINE_FIELD( CToadGrenade, m_flNextHunt, FIELD_TIME ),
	DEFINE_FIELD( CToadGrenade, m_flNextHit, FIELD_TIME ),
	DEFINE_FIELD( CToadGrenade, m_posPrev, FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( CToadGrenade, m_hOwner, FIELD_EHANDLE ),
};

IMPLEMENT_SAVERESTORE( CToadGrenade, CGrenade );

#define TOAD_DETONATE_DELAY	15.0f

int CToadGrenade :: Classify ( void )
{
	if (m_iMyClass != 0)
		return m_iMyClass; // protect against recursion

	if (m_hEnemy != 0)
	{
		m_iMyClass = CLASS_INSECT; // no one cares about it
		switch( m_hEnemy->Classify( ) )
		{
			case CLASS_PLAYER:
			case CLASS_HUMAN_PASSIVE:
			case CLASS_HUMAN_MILITARY:
				m_iMyClass = 0;
				return CLASS_ALIEN_MILITARY; // barney's get mad, grunts get mad at it
		}
		m_iMyClass = 0;
	}

	return CLASS_ALIEN_BIOWEAPON;
}

void CToadGrenade :: Spawn( void )
{
	Precache( );
	// motor
	pev->movetype = MOVETYPE_BOUNCE;
	pev->solid = SOLID_BBOX;

	SET_MODEL(ENT(pev), "models/w_toad.mdl");
	UTIL_SetSize(pev, Vector( -4, -4, 0), Vector(4, 4, 8));
	UTIL_SetOrigin( pev, pev->origin );

	SetTouch( &CToadGrenade::SuperBounceTouch );
	SetThink( &CToadGrenade::HuntThink );
	pev->nextthink = gpGlobals->time + 0.1f;
	m_flNextHunt = gpGlobals->time + (float)1E6;

	pev->flags |= FL_MONSTER;
	pev->takedamage		= DAMAGE_AIM;
	pev->health			= gSkillData.toadHealth;
	pev->gravity		= 0.5f;
	pev->friction		= 0.5f;

	pev->dmg = gSkillData.toadDmgPop;

	m_flDie = gpGlobals->time + TOAD_DETONATE_DELAY;

	m_flFieldOfView = 0; // 180 degrees

	if ( pev->owner )
		m_hOwner = Instance( pev->owner );

	m_flNextBounceSoundTime = gpGlobals->time;// reset each time a snark is spawned.

	pev->sequence = WTOAD_RUN;
	ResetSequenceInfo( );
}

void CToadGrenade::Precache( void )
{
	PRECACHE_MODEL("models/w_toad.mdl");
	PRECACHE_SOUND("toad/toad_blast1.wav");
	PRECACHE_SOUND("common/bodysplat.wav");
	PRECACHE_SOUND("toad/toad_die1.wav");
	PRECACHE_SOUND("toad/toad_hunt1.wav");
	PRECACHE_SOUND("toad/toad_hunt2.wav");
	PRECACHE_SOUND("toad/toad_hunt3.wav");
	PRECACHE_SOUND("toad/toad_deploy1.wav");
}


void CToadGrenade :: Killed( entvars_t *pevAttacker, int iGib )
{
	pev->model = iStringNull;// make invisible
	SetThink( &CToadGrenade::SUB_Remove );
//	ResetTouch( );
	SetTouch( NULL );
	pev->nextthink = gpGlobals->time + 0.1f;

	// since toad grenades never leave a body behind, clear out their takedamage now.
	// Squeaks do a bit of radius damage when they pop, and that radius damage will
	// continue to call this function unless we acknowledge the Squeak's death now. (sjb)
	pev->takedamage = DAMAGE_NO;

	// play squeek blast
	EMIT_SOUND_DYN(ENT(pev), CHAN_ITEM, "toad/toad_blast1.wav", 1, 0.5, 0, PITCH_NORM);	

	CSoundEnt::InsertSound ( bits_SOUND_COMBAT, pev->origin, SMALL_EXPLOSION_VOLUME, 3.0 );

	UTIL_BloodDrips( pev->origin, g_vecZero, BloodColor(), 80 );

	if (m_hOwner != 0)
		RadiusDamage ( pev, m_hOwner->pev, pev->dmg, CLASS_NONE, DMG_BLAST );
	else
		RadiusDamage ( pev, pev, pev->dmg, CLASS_NONE, DMG_BLAST );

	// reset owner so death message happens
	if (m_hOwner != 0)
		pev->owner = m_hOwner->edict();

	CBaseMonster :: Killed( pevAttacker, GIB_ALWAYS );
}

void CToadGrenade :: GibMonster( void )
{
	EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "common/bodysplat.wav", 0.75, ATTN_NORM, 0, 200);		
}

void CToadGrenade::HuntThink( void )
{
	// ALERT( at_console, "think\n" );

	if (!IsInWorld())
	{
	//	ResetTouch( );
		SetTouch( NULL );
		UTIL_Remove( this );
		return;
	}
	
	StudioFrameAdvance( );
	pev->nextthink = gpGlobals->time + 0.1f;

	// explode when ready
	if (gpGlobals->time >= m_flDie)
	{
		g_vecAttackDir = pev->velocity.Normalize( );
		pev->health = -1;
		Killed( pev, 0 );
		return;
	}

	// float
	if (pev->waterlevel != 0)
	{
		if (pev->movetype == MOVETYPE_BOUNCE)
		{
			pev->movetype = MOVETYPE_FLY;
		}
		pev->velocity = pev->velocity * 0.9f;
		pev->velocity.z += 8.0f;
	}
	else if (pev->movetype == MOVETYPE_FLY)
	{
		pev->movetype = MOVETYPE_BOUNCE;
	}

	// return if not time to hunt
	if (m_flNextHunt > gpGlobals->time)
		return;

	m_flNextHunt = gpGlobals->time + 2.0f;

	CBaseEntity *pOther = 0;
	Vector vecDir;
	TraceResult tr;

	Vector vecFlat = pev->velocity;
	vecFlat.z = 0;
	vecFlat = vecFlat.Normalize( );

	UTIL_MakeVectors( pev->angles );

	if (m_hEnemy == 0 || !m_hEnemy->IsAlive())
	{
		// find target, bounce a bit towards it.
		Look( 512 );
		m_hEnemy = BestVisibleEnemy( );
	}

	// squeek if it's about time blow up
	if ((m_flDie - gpGlobals->time <= 0.5f) && (m_flDie - gpGlobals->time >= 0.3f))
	{
		EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "toad/toad_die1.wav", 1, ATTN_NORM, 0, 100 + RANDOM_LONG(0,0x3F));
		CSoundEnt::InsertSound ( bits_SOUND_COMBAT, pev->origin, 256, 0.25 );
	}

	// higher pitch as squeeker gets closer to detonation time
	float flpitch = 155.0f - 60.0f * ((m_flDie - gpGlobals->time) / TOAD_DETONATE_DELAY);
	if (flpitch < 80)
		flpitch = 80;

	if (m_hEnemy != 0)
	{
		if (FVisible( m_hEnemy ))
		{
			vecDir = m_hEnemy->EyePosition() - pev->origin;
			m_vecTarget = vecDir.Normalize( );
		}

		float flVel = pev->velocity.Length();
		float flAdj = 50.0f / (flVel + 10.0f);

		if (flAdj > 1.2f)
			flAdj = 1.2f;
		
		// ALERT( at_console, "think : enemy\n");

		// ALERT( at_console, "%.0f %.2f %.2f %.2f\n", flVel, m_vecTarget.x, m_vecTarget.y, m_vecTarget.z );

		pev->velocity = pev->velocity * flAdj + m_vecTarget * 300;
	}

	if (pev->flags & FL_ONGROUND)
	{
		pev->avelocity = Vector( 0, 0, 0 );
	}
	else
	{
		if (pev->avelocity == Vector( 0, 0, 0))
		{
			pev->avelocity.x = RANDOM_FLOAT( -100, 100 );
			pev->avelocity.z = RANDOM_FLOAT( -100, 100 );
		}
	}

	if ((pev->origin - m_posPrev).Length() < 1.0f)
	{
		pev->velocity.x = RANDOM_FLOAT( -100, 100 );
		pev->velocity.y = RANDOM_FLOAT( -100, 100 );
	}
	m_posPrev = pev->origin;

	pev->angles = UTIL_VecToAngles( pev->velocity );
	pev->angles.z = 0;
	pev->angles.x = 0;
}


void CToadGrenade::SuperBounceTouch( CBaseEntity *pOther )
{
	float	flpitch;

	TraceResult tr = UTIL_GetGlobalTrace( );

	// don't hit the guy that launched this grenade
	if ( pev->owner && pOther->edict() == pev->owner )
		return;

	// at least until we've bounced once
	pev->owner = 0;

	pev->angles.x = 0;
	pev->angles.z = 0;

	// avoid bouncing too much
	if (m_flNextHit > gpGlobals->time)
		return;

	// higher pitch as squeeker gets closer to detonation time
	flpitch = 155.0f - 60.0f * ((m_flDie - gpGlobals->time) / TOAD_DETONATE_DELAY);

	if ( pOther->pev->takedamage && m_flNextAttack < gpGlobals->time )
	{
		// attack!

		// make sure it's me who has touched them
		if (tr.pHit == pOther->edict())
		{
			// and it's not another toadgrenade
			if (tr.pHit->v.modelindex != pev->modelindex)
			{
				// ALERT( at_console, "hit enemy\n");
				ClearMultiDamage( );
				pOther->TraceAttack(pev, gSkillData.toadDmgBite, gpGlobals->v_forward, &tr, DMG_SLASH ); 
				if (m_hOwner != 0)
					ApplyMultiDamage( pev, m_hOwner->pev );
				else
					ApplyMultiDamage( pev, pev );

				pev->dmg += gSkillData.toadDmgPop; // add more explosion damage
				// m_flDie += 2.0; // add more life

				// make bite sound
				EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, "toad/toad_deploy1.wav", 1.0, ATTN_NORM, 0, (int)flpitch);
				m_flNextAttack = gpGlobals->time + 0.5f;
			}
		}
		else
		{
			// ALERT( at_console, "been hit\n");
		}
	}

	m_flNextHit = gpGlobals->time + 0.1f;
	m_flNextHunt = gpGlobals->time;

	if ( g_pGameRules->IsMultiplayer() )
	{
		// in multiplayer, we limit how often snarks can make their bounce sounds to prevent overflows.
		if ( gpGlobals->time < m_flNextBounceSoundTime )
		{
			// too soon!
			return;
		}
	}

	if (!(pev->flags & FL_ONGROUND))
	{
		// play bounce sound
		float flRndSound = RANDOM_FLOAT ( 0 , 1 );

		if ( flRndSound <= 0.33f )
			EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "toad/toad_hunt1.wav", 1, ATTN_NORM, 0, (int)flpitch);		
		else if (flRndSound <= 0.66f)
			EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "toad/toad_hunt2.wav", 1, ATTN_NORM, 0, (int)flpitch);
		else 
			EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "toad/toad_hunt3.wav", 1, ATTN_NORM, 0, (int)flpitch);
		CSoundEnt::InsertSound ( bits_SOUND_COMBAT, pev->origin, 256, 0.25 );
	}
	else
	{
		// skittering sound
		CSoundEnt::InsertSound ( bits_SOUND_COMBAT, pev->origin, 100, 0.1 );
	}

	m_flNextBounceSoundTime = gpGlobals->time + 0.5f;// half second.
}

#endif

LINK_ENTITY_TO_CLASS( weapon_toad, CToad );


void CToad::Spawn( )
{
	Precache( );
	m_iId = WEAPON_TOAD;
	SET_MODEL(ENT(pev), "models/toad_nest.mdl");

	FallInit();//get ready to fall down.

	m_iDefaultAmmo = TOAD_DEFAULT_GIVE;
		
	pev->sequence = 1;
	pev->animtime = gpGlobals->time;
	pev->framerate = 1.0f;

	SetThink( &CToad::ToadIdle );
	pev->nextthink = gpGlobals->time + 0.2f;
	m_flNextTrace = 0;
	m_flNextHunt = 0;
	m_flDie = 0;
}


void CToad::Precache( void )
{
	PRECACHE_MODEL("models/toad_nest.mdl");
	PRECACHE_MODEL("models/v_toad.mdl");
	PRECACHE_MODEL("models/p_toad.mdl");
	PRECACHE_SOUND("toad/toad_hunt2.wav");
	PRECACHE_SOUND("toad/toad_hunt3.wav");
	UTIL_PrecacheOther("monster_toad");
}


int CToad::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "Toads";
	p->iMaxAmmo1 = TOAD_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = WEAPON_NOCLIP;
	p->iSlot = 4;
	p->iPosition = 5;
	p->iId = m_iId = WEAPON_TOAD;
	p->iWeight = SNARK_WEIGHT;
	p->iFlags = ITEM_FLAG_LIMITINWORLD | ITEM_FLAG_EXHAUSTIBLE;

	return 1;
}



BOOL CToad::Deploy( )
{
	if( !gEvilImpulse101 )
	{
		// play hunt sound
		float flRndSound = RANDOM_FLOAT( 0, 1 );

		if( flRndSound <= 0.5f )
			EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "toad/toad_hunt2.wav", 1, ATTN_NORM, 0, 100);
		else 
			EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "toad/toad_hunt3.wav", 1, ATTN_NORM, 0, 100);

		m_pPlayer->m_iWeaponVolume = QUIET_GUN_VOLUME;
	}

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1.1f;
	return DefaultDeploy( "models/v_toad.mdl", "models/p_toad.mdl", TOAD_UP, "toad" );
}


void CToad::Holster( int skiplocal /* = 0 */ )
{
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5f;

	if( !m_pPlayer->m_rgAmmo[ m_iPrimaryAmmoType ] )
	{
		m_pPlayer->pev->weapons &= ~(1<<WEAPON_TOAD);
		SetThink( &CToad::DestroyItem );
		pev->nextthink = gpGlobals->time + 0.1f;
		return;
	}
	
	DefaultHolster( TOAD_DOWN, 1.5 );
	EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "common/null.wav", 1.0, ATTN_NORM);
}


void CToad::PrimaryAttack()
{
	if( m_pPlayer->m_bIsHolster )
        {
                WeaponIdle();
                return;
        }

	if( m_pPlayer->m_rgAmmo[ m_iPrimaryAmmoType ] )
	{
		UTIL_MakeVectors( m_pPlayer->pev->v_angle );
		TraceResult tr;
		Vector trace_origin;

		// HACK HACK:  Ugly hacks to handle change in origin based on new physics code for players
		// Move origin up if crouched and start trace a bit outside of body ( 20 units instead of 16 )
		trace_origin = m_pPlayer->pev->origin;
		if( m_pPlayer->pev->flags & FL_DUCKING )
		{
			trace_origin = trace_origin - ( VEC_HULL_MIN - VEC_DUCK_HULL_MIN );
		}

		// find place to toss monster
		UTIL_TraceLine( trace_origin + gpGlobals->v_forward * 20, trace_origin + gpGlobals->v_forward * 64, dont_ignore_monsters, NULL, &tr );

		if( tr.fAllSolid == 0 && tr.fStartSolid == 0 && tr.flFraction > 0.25f )
		{
			SendWeaponAnim( TOAD_THROW );

			// player "shoot" animation
			m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

#if !CLIENT_DLL
			CBaseEntity *pToad = CBaseEntity::Create( "monster_toad", tr.vecEndPos, m_pPlayer->pev->v_angle, m_pPlayer->edict() );
			pToad->pev->velocity = gpGlobals->v_forward * 200 + m_pPlayer->pev->velocity;
#endif

			// play hunt sound
			float flRndSound = RANDOM_FLOAT( 0 , 1 );

			if( flRndSound <= 0.5f )
				EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "toad/toad_hunt2.wav", 1, ATTN_NORM, 0, 105);
			else 
				EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "toad/toad_hunt3.wav", 1, ATTN_NORM, 0, 105);

			m_pPlayer->m_iWeaponVolume = QUIET_GUN_VOLUME;

			m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType]--;

			m_fJustThrown = 1;

			m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.3f;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1.0f;
		}
	}
}


void CToad::SecondaryAttack( void )
{
	if( m_pPlayer->m_bIsHolster )
        {
                WeaponIdle();
                return;
        }
}


void CToad::WeaponIdle( void )
{
	if( m_pPlayer->m_bIsHolster )
        {
                if( m_flTimeWeaponIdle <= UTIL_WeaponTimeBase() )
                {
                        m_pPlayer->m_bIsHolster = FALSE;
                        Deploy();
                }
		return;
        }

	if ( m_flTimeWeaponIdle > UTIL_WeaponTimeBase() )
		return;

	if (m_fJustThrown)
	{
		m_fJustThrown = 0;

		if ( !m_pPlayer->m_rgAmmo[PrimaryAmmoIndex()] )
		{
			RetireWeapon();
			return;
		}

		SendWeaponAnim( TOAD_UP );
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
		return;
	}

	int iAnim, iRand = RANDOM_LONG( 0, 5 );
	switch( iRand )
	{
	case 0:
		iAnim = TOAD_IDLE2;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 2.2f;
		break;
	case 1:
		iAnim = TOAD_FIDGETFIT;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 2.18f;
		break;
	case 2:
		iAnim = TOAD_FIDGETNIP;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 2.9f;
		break;
	default:
		iAnim = TOAD_IDLE1;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 2.2f;
		break;
	}
	SendWeaponAnim( iAnim );
}

void CToad::ToadIdle()
{
	int i;
	float flDist;
	CBaseEntity *pPlayer = UTIL_PlayerByIndex( 1 );

	if( !pPlayer )
		return;

	flDist = ( pPlayer->Center() - this->Center() ).Length();
	if( !pPlayer->FVisible( this ) || flDist >= 256.0f )
	{
		if( pev->sequence != 1 )
		{
			pev->sequence = 1;
			pev->framerate = 0.5;
			ResetSequenceInfo();
		}

		if( !RANDOM_LONG( 0, 10 ) )
			HuntSound( true );
		pev->nextthink = gpGlobals->time + 0.2f;
		return;
	}

	if( flDist >= 50.0f )
	{
		TraceResult tr;

		m_flDie = 1;
		pev->movetype = MOVETYPE_STEP;
		StudioFrameAdvance( 0.0 );
		if( pev->sequence != 1 )
		{
			if( m_flNextTrace <= gpGlobals->time )
			{
				Vector vecSrc, vecEnd;
				Vector vecDist2D = ( Center() - pPlayer->Center() ).Normalize();
				vecDist2D.z = 0;
				m_posNext = pev->origin + vecDist2D * 128;
				vecSrc = EyePosition();
				vecEnd = vecSrc + vecDist2D * 68;
				UTIL_TraceLine( vecSrc, vecEnd, ignore_monsters, 0, &tr );

				if( tr.flFraction < 1.0f )
				{
					Vector vecDist, vecForward, vecAngle;
					vecDist = ( EyePosition() - tr.vecEndPos ).Normalize();
					vecAngle = Vector( 0, UTIL_AngleMod( UTIL_VecToAngles( vecDist ).y ) + 195.0f, 0 );
					UTIL_MakeVectorsPrivate( vecAngle, vecForward, 0, 0 );
					m_posNext = pev->origin - vecForward * 128;
				}
				m_flNextTrace = gpGlobals->time + 0.5f;
			}
			Vector newPos = m_posNext - Center();
			newPos.z = 0;
			pev->angles = UTIL_VecToAngles( newPos );
			UTIL_MoveToOrigin( ENT( pev ), newPos * 256.0f, 200.0f / m_flGroundSpeed, MOVE_STRAFE );
			HuntSound( false );
			pev->nextthink = gpGlobals->time + 0.2f;
			return;
		}
		pev->sequence = 0;
		pev->frame = 0;
		ResetSequenceInfo();
		pev->framerate = 3.0;
		HuntSound( true );
		pev->nextthink = gpGlobals->time + 0.2f;
		return;
	}

	if( m_flDie == 1 )
	{
		for( i = 0; i < m_iDefaultAmmo; i++ )
			( (CBasePlayer*)pPlayer )->GiveNamedItem( "weapon_toad" );
		SetThink( &CBaseEntity::SUB_Remove );
		pev->nextthink = gpGlobals->time + 0.1f;
	}
}

void CToad::HuntSound( bool force )
{
	const char *pszSound;

	if( ( m_flNextHunt <= gpGlobals->time ) || force )
	{
		int iRand = RANDOM_LONG( 0, 2 );
		if( iRand == 2 )
		{
			pszSound = "toad/toad_hunt3.wav";
		}
		else if( iRand == 1 )
		{
			pszSound = "toad/toad_hunt2.wav";
		}
		else
		{
			pszSound = "toad/toad_hunt1.wav";
		}
		EMIT_SOUND( ENT( pev ), CHAN_VOICE, pszSound, RANDOM_FLOAT( 0.5, 0.8 ), ATTN_NORM );
		m_flNextHunt = gpGlobals->time + RANDOM_FLOAT( 3.0, 4.0 );
	}
}
#endif
