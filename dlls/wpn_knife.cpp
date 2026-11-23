#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"
#include "buildslave_assert.h"

#define ASSERT_FILE BUILDSLAVE_PATH( L"czero\\dlls\\wpn_knife.cpp" )

enum knife_e
{
	KNIFE_IDLE,
	KNIFE_ATTACK1HIT,
	KNIFE_ATTACK2HIT,
	KNIFE_DRAW,
	KNIFE_STABHIT,
	KNIFE_STABMISS,
	KNIFE_MIDATTACK1HIT,
	KNIFE_MIDATTACK2HIT
};

LINK_ENTITY_TO_CLASS( weapon_knife, CKnife );

void FindHullIntersection( const Vector& source, TraceResult& trace, float* mins, float* maxs, edict_t* entity )
{
	float* minmaxs[2] = { mins, maxs };
	TraceResult test;
	Vector hullEnd = source + ( trace.vecEndPos - source ) * 2;
	UTIL_TraceLine( source, hullEnd, dont_ignore_monsters, entity, &test );
	if ( test.flFraction < 1.0 )
	{
		trace = test;
		return;
	}

	float distance = 1000000;
	for ( int i = 0; i < 2; i++ )
	{
		for ( int j = 0; j < 2; j++ )
		{
			for ( int k = 0; k < 2; k++ )
			{
				Vector end;
				end.x = hullEnd.x + minmaxs[i][0];
				end.y = hullEnd.y + minmaxs[j][1];
				end.z = hullEnd.z + minmaxs[k][2];
				UTIL_TraceLine( source, end, dont_ignore_monsters, entity, &test );
				if ( test.flFraction < 1.0 )
				{
					float hitDistance = ( test.vecEndPos - source ).Length();
					if ( hitDistance < distance )
					{
						trace = test;
						distance = hitDistance;
					}
				}
			}
		}
	}
}

void CKnife::Spawn()
{
	Precache();
	m_iId = ACCESSORY_KNIFE;
	SET_MODEL( edict(), "models/w_knife.mdl" );
	m_iClip = -1;
	m_bIsAccessory = true;
	FallInit();
}

void CKnife::Precache()
{
	PRECACHE_MODEL( "models/v_knife.mdl" );
	PRECACHE_MODEL( "models/w_knife.mdl" );
	PRECACHE_SOUND( "weapons/knife_deploy1.wav" );
	PRECACHE_SOUND( "weapons/knife_hit1.wav" );
	PRECACHE_SOUND( "weapons/knife_hit2.wav" );
	PRECACHE_SOUND( "weapons/knife_hit3.wav" );
	PRECACHE_SOUND( "weapons/knife_hit4.wav" );
	PRECACHE_SOUND( "weapons/knife_slash1.wav" );
	PRECACHE_SOUND( "weapons/knife_slash2.wav" );
	PRECACHE_SOUND( "weapons/knife_stab.wav" );
	PRECACHE_SOUND( "weapons/knife_hitwall1.wav" );
	m_usKnife = PRECACHE_EVENT( 1, "events/knife.sc" );
}

int CKnife::GetItemInfo( ItemInfo* p )
{
	p->pszName = STRING( pev->classname );
	p->pszAmmo1 = NULL;
	p->iMaxAmmo1 = -1;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = WEAPON_NOCLIP;
	p->iSlot = 2;
	p->iPosition = 1;
	p->iId = ACCESSORY_KNIFE;
	p->iWeight = 0;
	p->bIsAccessory = true;
	return 1;
}

BOOL CKnife::Deploy()
{
	EMIT_SOUND_DYN( m_pPlayer->edict(), CHAN_ITEM, "weapons/knife_deploy1.wav", 0.3f, 2.4f, 0, PITCH_NORM );
	m_iSwing = 0;
	return DefaultDeploy( "models/v_knife.mdl", NULL, KNIFE_DRAW, "knife", UseDecrement() );
}

void CKnife::Holster( int )
{
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;
}

void CKnife::PrimaryAttack()
{
	Swing( 1 );
}

void CKnife::SecondaryAttack()
{
	Stab( 1 );
	pev->nextthink = UTIL_WeaponTimeBase() + 0.35;
}

void CKnife::SwingAgain()
{
	Swing( 0 );
}

void CKnife::Smack()
{
}

void CKnife::WeaponAnimation( int )
{
}

int CKnife::Swing( int first )
{
	UTIL_MakeVectors( m_pPlayer->pev->v_angle );
	Vector source = m_pPlayer->GetGunPosition();
	Vector end = source + gpGlobals->v_forward * 48;
	TraceResult trace;
	UTIL_TraceLine( source, end, dont_ignore_monsters, m_pPlayer->edict(), &trace );
	if ( trace.flFraction >= 1.0 )
	{
		UTIL_TraceHull( source, end, dont_ignore_monsters, head_hull, m_pPlayer->edict(), &trace );
		if ( trace.flFraction < 1.0 )
		{
			CBaseEntity* hit = CBaseEntity::Instance( trace.pHit );
			if ( !hit || hit->IsBSPModel() )
			{
				Vector mins = VEC_DUCK_HULL_MIN;
				Vector maxs = VEC_DUCK_HULL_MAX;
				FindHullIntersection( source, trace, &mins.x, &maxs.x, m_pPlayer->edict() );
			}
			end = trace.vecEndPos;
		}
	}

	if ( trace.flFraction >= 1.0 )
	{
		if ( first )
		{
			switch ( m_iSwing++ % 2 )
			{
			case 0:
				SetWeaponState( KNIFE_MIDATTACK1HIT );
				break;
			case 1:
				SetWeaponState( KNIFE_MIDATTACK2HIT );
				break;
			}
			const char* sound = RANDOM_LONG( 0, 1 ) ? "weapons/knife_slash1.wav" : "weapons/knife_slash2.wav";
			EMIT_SOUND_DYN( m_pPlayer->edict(), CHAN_WEAPON, sound, 1, ATTN_NORM, 0, 94 );
			m_pPlayer->SetAnimation( PLAYER_ATTACK1 );
		}
		return 0;
	}

	switch ( m_iSwing++ % 2 )
	{
	case 0:
		SetWeaponState( KNIFE_MIDATTACK1HIT );
		break;
	case 1:
		SetWeaponState( KNIFE_MIDATTACK2HIT );
		break;
	}
	float volume = 1;
#ifndef CLIENT_DLL
	CBaseEntity* hit = CBaseEntity::Instance( trace.pHit );
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );
	ClearMultiDamage();
	double nextAttack = m_flNextPrimaryAttack + 0.4;
	float damage = UTIL_WeaponTimeBase() <= nextAttack ? 15.0f : 20.0f;
	hit->TraceAttack( m_pPlayer->pev, damage, gpGlobals->v_forward, &trace, DMG_BULLET | DMG_NEVERGIB );
	ApplyMultiDamage( m_pPlayer->pev, m_pPlayer->pev );
	if ( !hit || hit->Classify() == CLASS_NONE || hit->Classify() == CLASS_MACHINE )
	{
		Vector soundEnd = source + ( end - source ) * 2;
		TEXTURETYPE_PlaySound( &trace, source, soundEnd, BULLET_PLAYER_CROWBAR );
		if ( (unsigned int)RANDOM_LONG( 0, 1 ) <= 1 )
		{
			int pitch = RANDOM_LONG( 0, 3 ) + 98;
			EMIT_SOUND_DYN( m_pPlayer->edict(), CHAN_ITEM, "weapons/knife_hitwall1.wav", 1, ATTN_NORM, 0, pitch );
		}
		volume = 1;
	}
	else
	{
		switch ( RANDOM_LONG( 0, 3 ) )
		{
		case 0:
			EMIT_SOUND_DYN( m_pPlayer->edict(), CHAN_WEAPON, "weapons/knife_hit1.wav", 1, ATTN_NORM, 0, PITCH_NORM );
			break;
		case 1:
			EMIT_SOUND_DYN( m_pPlayer->edict(), CHAN_WEAPON, "weapons/knife_hit2.wav", 1, ATTN_NORM, 0, PITCH_NORM );
			break;
		case 2:
			EMIT_SOUND_DYN( m_pPlayer->edict(), CHAN_WEAPON, "weapons/knife_hit3.wav", 1, ATTN_NORM, 0, PITCH_NORM );
			break;
		case 3:
			EMIT_SOUND_DYN( m_pPlayer->edict(), CHAN_WEAPON, "weapons/knife_hit4.wav", 1, ATTN_NORM, 0, PITCH_NORM );
			break;
		}
		m_pPlayer->m_iWeaponVolume = 128;
		if ( !hit->IsAlive() )
			return 1;
		volume = 0.1f;
	}
#endif
	m_trHit = trace;
	SetThink( &CKnife::Smack );
	pev->nextthink = UTIL_WeaponTimeBase() + 0.2;
	m_pPlayer->m_iWeaponVolume = (int)( volume * 512 );
	return 1;
}

int CKnife::Stab( int first )
{
	UTIL_MakeVectors( m_pPlayer->pev->v_angle );
	Vector source = m_pPlayer->GetGunPosition();
	Vector end = source + gpGlobals->v_forward * 32;
	TraceResult trace;
	UTIL_TraceLine( source, end, dont_ignore_monsters, m_pPlayer->edict(), &trace );
	if ( trace.flFraction >= 1.0 )
	{
		UTIL_TraceHull( source, end, dont_ignore_monsters, head_hull, m_pPlayer->edict(), &trace );
		if ( trace.flFraction < 1.0 )
		{
			CBaseEntity* hit = CBaseEntity::Instance( trace.pHit );
			if ( !hit || hit->IsBSPModel() )
			{
				Vector mins = VEC_DUCK_HULL_MIN;
				Vector maxs = VEC_DUCK_HULL_MAX;
				FindHullIntersection( source, trace, &mins.x, &maxs.x, m_pPlayer->edict() );
			}
			end = trace.vecEndPos;
		}
	}

	if ( trace.flFraction >= 1.0 )
	{
		if ( first )
		{
			SetWeaponState( KNIFE_STABMISS );
			const char* sound = RANDOM_LONG( 0, 1 ) ? "weapons/knife_slash1.wav" : "weapons/knife_slash2.wav";
			EMIT_SOUND_DYN( m_pPlayer->edict(), CHAN_WEAPON, sound, 1, ATTN_NORM, 0, 94 );
			m_pPlayer->SetAnimation( PLAYER_ATTACK1 );
		}
		return 0;
	}

	SetWeaponState( KNIFE_STABHIT );
	float volume = 1;
#ifndef CLIENT_DLL
	CBaseEntity* hit = CBaseEntity::Instance( trace.pHit );
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );
	ClearMultiDamage();
	hit->TraceAttack( m_pPlayer->pev, 65, gpGlobals->v_forward, &trace, DMG_BULLET | DMG_NEVERGIB );
	ApplyMultiDamage( m_pPlayer->pev, m_pPlayer->pev );
	if ( hit && hit->Classify() != CLASS_NONE && hit->Classify() != CLASS_MACHINE )
	{
		EMIT_SOUND_DYN( m_pPlayer->edict(), CHAN_WEAPON, "weapons/knife_stab.wav", 1, ATTN_NORM, 0, PITCH_NORM );
		m_pPlayer->m_iWeaponVolume = 128;
		if ( !hit->IsAlive() )
			return 1;
		volume = 0.1f;
	}
	else
	{
		Vector soundEnd = source + ( end - source ) * 2;
		TEXTURETYPE_PlaySound( &trace, source, soundEnd, BULLET_PLAYER_CROWBAR );
		if ( (unsigned int)RANDOM_LONG( 0, 1 ) <= 1 )
		{
			int pitch = RANDOM_LONG( 0, 3 ) + 98;
			EMIT_SOUND_DYN( m_pPlayer->edict(), CHAN_ITEM, "weapons/knife_hitwall1.wav", 1, ATTN_NORM, 0, pitch );
		}
		volume = 1;
	}
#endif
	m_trHit = trace;
	SetThink( &CKnife::Smack );
	pev->nextthink = UTIL_WeaponTimeBase() + 0.2;
	m_pPlayer->m_iWeaponVolume = (int)( volume * 512 );
	return 1;
}

void CKnife::WeaponIdle()
{
	if ( m_flTimeWeaponIdle <= UTIL_WeaponTimeBase() && GetWeaponState() <= KNIFE_MIDATTACK2HIT )
		SetWeaponState( KNIFE_IDLE );
}

void CKnife::SetWeaponState( unsigned short state )
{
	float duration = 0;
	float nextAttack = 0;
	float nextSecondary = 0;
	switch ( state )
	{
	case KNIFE_IDLE:
		duration = 0.133f;
		nextAttack = nextSecondary = 0;
		break;
	case KNIFE_ATTACK1HIT:
	case KNIFE_ATTACK2HIT:
	case KNIFE_STABMISS:
		duration = nextAttack = nextSecondary = 0.81f;
		break;
	case KNIFE_DRAW:
		duration = nextAttack = nextSecondary = 1;
		break;
	case KNIFE_STABHIT:
		duration = nextAttack = nextSecondary = 1.189f;
		break;
	case KNIFE_MIDATTACK1HIT:
	case KNIFE_MIDATTACK2HIT:
		duration = nextAttack = 0.675f;
		nextSecondary = 0.825f;
		break;
	default:
		ASSERT_AT( false, 494 );
		break;
	}
	SendWeaponAnim( state, UseDecrement() );
	m_WeaponState = state;
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + duration;
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + nextAttack;
	m_flNextSecondaryAttack = nextSecondary;
}

int CKnife::iItemSlot()
{
	return 3;
}

float CKnife::GetMaxSpeed()
{
	return 225;
}

BOOL CKnife::CanDrop()
{
	return FALSE;
}

BOOL CKnife::UseDecrement()
{
	return TRUE;
}
