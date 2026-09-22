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

===== h_cycler.cpp ========================================================

  The Halflife Cycler Monsters

*/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "animation.h"
#include "weapons.h"
#include "player.h"

#define TEMP_FOR_SCREEN_SHOTS	1
#if TEMP_FOR_SCREEN_SHOTS //===================================================

class CCycler : public CBaseMonster
{
public:
	void GenericCyclerSpawn( const char *szModel, Vector vecMin, Vector vecMax );
	virtual int ObjectCaps( void ) { return ( CBaseEntity::ObjectCaps() | FCAP_IMPULSE_USE ); }
	int TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType );
	void Spawn( void );
	void Think( void );
	//void Pain( float flDamage );
	void Use ( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );

	// Don't treat as a live target
	virtual BOOL IsAlive( void ) { return FALSE; }
	int BloodColor( void ) { return DONT_BLEED; }
#if SPEAKABLE_TARGETS
	BOOL IsAllowedToSpeak( void ) { return TRUE; }
#endif
	virtual int Save( CSave &save );
	virtual int Restore( CRestore &restore );
	static TYPEDESCRIPTION m_SaveData[];

	int m_animate;
};

TYPEDESCRIPTION	CCycler::m_SaveData[] =
{
	DEFINE_FIELD( CCycler, m_animate, FIELD_INTEGER ),
};

IMPLEMENT_SAVERESTORE( CCycler, CBaseMonster )

//
// we should get rid of all the other cyclers and replace them with this.
//
class CGenericCycler : public CCycler
{
public:
	void Spawn( void )
	{
		GenericCyclerSpawn( STRING( pev->model ), Vector( -16, -16, 0 ), Vector( 16, 16, 72 ) );
	}
};

LINK_ENTITY_TO_CLASS( cycler, CGenericCycler )

// Probe droid imported for tech demo compatibility
//
// PROBE DROID
//
class CCyclerProbe : public CCycler
{
public:	
	void Spawn( void );
};

LINK_ENTITY_TO_CLASS( cycler_prdroid, CCyclerProbe )

void CCyclerProbe::Spawn( void )
{
	pev->origin = pev->origin + Vector( 0, 0, 16 );
	GenericCyclerSpawn( "models/prdroid.mdl", Vector( -16, -16, -16 ), Vector( 16, 16, 16 ) );
}

// Cycler member functions
void CCycler::GenericCyclerSpawn( const char *szModel, Vector vecMin, Vector vecMax )
{
	if( !szModel || !*szModel )
	{
		ALERT( at_error, "cycler at %.0f %.0f %0.f missing modelname\n", (double)pev->origin.x, (double)pev->origin.y, (double)pev->origin.z );
		REMOVE_ENTITY( ENT( pev ) );
		return;
	}

	pev->classname = MAKE_STRING( "cycler" );
	PRECACHE_MODEL( szModel );
	SET_MODEL( ENT( pev ), szModel );

	CCycler::Spawn();

	UTIL_SetSize( pev, vecMin, vecMax );
}

void CCycler::Spawn()
{
	InitBoneControllers();
	pev->solid		= SOLID_NOT;
	pev->movetype		= MOVETYPE_NONE;
	pev->takedamage		= DAMAGE_NO;
	pev->effects		= 0;
	pev->health		= 80000;// no cycler should die
	pev->yaw_speed		= 5;
	pev->ideal_yaw		= pev->angles.y;
	ChangeYaw( 360 );

	m_flFrameRate		= 75;
	m_flGroundSpeed		= 0;

	pev->nextthink		+= 1.0f;

	ResetSequenceInfo();

	if( /*pev->sequence != 0 || */pev->frame != 0 )
	{
		m_animate = 0;
		pev->framerate = 0;
	}
	else
	{
		m_animate = 1;
	}

	if(pev->armortype == 1)
	{
		pev->effects = EF_DIMLIGHT | EF_BRIGHTFIELD;
		UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);
		pev->solid = SOLID_SLIDEBOX;
	}
	else if(pev->armortype == 2)
	{
		UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);
		pev->solid = SOLID_SLIDEBOX;
	}
	else if(pev->armortype == 3 || pev->armortype == 4)
	{
		UTIL_SetSize(pev, Vector( -8, -8, 0 ), Vector(8,8,72) );
		pev->solid = SOLID_SLIDEBOX;
	}
}

//
// cycler think
//
void CCycler::Think( void )
{
	pev->nextthink = gpGlobals->time + 0.1f;

	if( m_animate )
	{
		StudioFrameAdvance();
	}
	if( m_fSequenceFinished && !m_fSequenceLoops )
	{
		// ResetSequenceInfo();
		// hack to avoid reloading model every frame
		pev->animtime = gpGlobals->time;
		pev->framerate = 1.0;
		m_fSequenceFinished = FALSE;
		m_flLastEventCheck = gpGlobals->time;
		pev->frame = 0;
		if( !m_animate )
			pev->framerate = 0.0f;	// FIX: don't reset framerate
	}
}

//
// CyclerUse - starts a rotation trend
//
void CCycler::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	/*m_animate = !m_animate;
	if( m_animate )
		pev->framerate = 1.0f;
	else
		pev->framerate = 0.0f;*/
	
	if(pev->armortype == 1 && pev->body == 0)
	{

		if( pActivator != NULL)
		{
			if ( pActivator->IsPlayer() )
				return;
		}

		pev->body = 1;

		MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
		WRITE_BYTE( TE_LARGEFUNNEL );
		WRITE_COORD( pev->origin.x );
		WRITE_COORD( pev->origin.y );
		WRITE_COORD( pev->origin.z );
		WRITE_SHORT( g_sModelIndexFlareGlow );
		WRITE_SHORT( 1 );
		MESSAGE_END();

		CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "player" );
		if(pEntity->pev->deadflag == DEAD_NO)
		{
			pEntity->pev->health = pEntity->pev->max_health;
			FX_Explosion( pEntity->Center(), 45);
			CBasePlayer *pPlayer = GetClassPtr((CBasePlayer *)pEntity->pev);

			if(pPlayer->m_fNextClearTextTime < gpGlobals->time)
			{
				pPlayer->m_fNextClearTextTime = gpGlobals->time + 6.0;		
			}
			else
			{
				pPlayer->m_fNextClearTextTime += 4.0;			
			}

			char text[256];
			sprintf( text, "- All recover health!\n");
			UTIL_SayTextAll( text,this );

			if (pPlayer->m_team_npc1 != NULL && pPlayer->m_team_npc1->pev->deadflag == DEAD_NO)
			{
				pPlayer->m_team_npc1->pev->health = pPlayer->m_team_npc1->pev->max_health;
				FX_Explosion( pPlayer->m_team_npc1->Center(), 45);
			}
			
			if (pPlayer->m_team_npc2 != NULL && pPlayer->m_team_npc2->pev->deadflag == DEAD_NO)
			{
				pPlayer->m_team_npc2->pev->health = pPlayer->m_team_npc2->pev->max_health;
				FX_Explosion( pPlayer->m_team_npc2->Center(), 45);
			}
			
			if (pPlayer->m_team_npc3 != NULL && pPlayer->m_team_npc3->pev->deadflag == DEAD_NO)
			{
				pPlayer->m_team_npc3->pev->health = pPlayer->m_team_npc3->pev->max_health;
				FX_Explosion( pPlayer->m_team_npc3->Center(), 45);
			}
			
			if (pPlayer->m_team_npc4 != NULL && pPlayer->m_team_npc4->pev->deadflag == DEAD_NO)
			{
				pPlayer->m_team_npc4->pev->health = pPlayer->m_team_npc4->pev->max_health;
				FX_Explosion( pPlayer->m_team_npc4->Center(), 45);
			}
		}

		pev->effects = 0;
	}

	if(pev->armortype == 2)
	{
		if( pActivator == NULL)
			return;
	
		if ( pActivator->IsPlayer() )
		{
			CBasePlayer *pPlayer = GetClassPtr((CBasePlayer *)pActivator->pev);
			if(pPlayer)
			{
				if(pev->body == 1)
				{
					if(pPlayer->m_fValve)
					{
						pPlayer->m_fValve = FALSE;
						pPlayer->MenuItem_remove(14);
						pev->body = 0;
						EMIT_SOUND(ENT(pPlayer->pev), CHAN_ITEM, "items/gunpickup2.wav", 1, ATTN_NORM);
					}
				}
				else if(pev->body == 0)
				{
					pPlayer->GiveNamedItem( "weapon_valvesword" );
					FireTargets( "wdoor_get_valvesword", this, this, USE_TOGGLE, 0 );
					UTIL_Remove( this );
					return;
				}
			}
		}
	}

	if(pev->armortype == 3)
	{
		if(pev->impulse >= 1)
		{
			if ( pev->dmgtime < gpGlobals->time )
			{
				pev->dmgtime = gpGlobals->time + 6.0;
				if(pev->impulse >= 1)
				{
					char text[256];
					CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "player" );
					if(pEntity->pev->deadflag == DEAD_NO)
					{
						CBasePlayer *pPlayer = GetClassPtr((CBasePlayer *)pEntity->pev);
								
						if(pev->impulse == 1)
						{
							
							sprintf( text, "???: This is the bonus level room. You can press the E button to look around or enter the painting to take on the challenges.\n");
					
						}
						else if(pev->impulse == 2)
						{
							sprintf( text, "Plant: The vending machine for drinks is out of order.\n");
						}
						else if(pev->impulse == 3)
						{
							sprintf( text, "Plant: The snack vending machine is out of order.\n");
						}
						else if(pev->impulse == 4)
						{
							sprintf( text, "Book: With the invisibility skill on, equipping the Holy Sword and charging can cause critical damage.\n");
						}
						else if(pev->impulse == 5)
						{
							sprintf( text, "Book: After clearing the game once, the fist can be used to perform a special attack in the air to climb walls!\n");
							
						}
						else if(pev->impulse == 6)
						{
								
							sprintf( text, "Book: When attacking enemies with melee weapons during the high-speed long jump, greater damage will be caused!\n");
							
						}
						else if(pev->impulse == 7)
						{
							sprintf( text, "Book: I can't tell you anything.\n");
						
						}
						pPlayer->m_fNextClearTextTime = gpGlobals->time + 6.0;
						UTIL_SayTextAll( text,this );
					}
				}
			}
		}
	}

	if(pev->armortype == 4)
	{
		if ( pev->dmgtime < gpGlobals->time )
		{
			pev->dmgtime = gpGlobals->time + 4.0;
			if( !FStringNull(pev->message) )
			{
				CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "player" );
				if(pEntity->pev->deadflag == DEAD_NO)
				{
					CBasePlayer *pPlayer = GetClassPtr((CBasePlayer *)pEntity->pev);

					pPlayer->m_fNextClearTextTime = gpGlobals->time + 4.0;
					UTIL_SayTextAll( STRING(pev->message),this );
				}
			}
		}
	}
}

//
// CyclerPain , changes sequences when shot
//
//void CCycler::Pain( float flDamage )
int CCycler::TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	/*if( m_animate )
	{
		pev->sequence++;

		ResetSequenceInfo();

		if( m_flFrameRate == 0.0f )
		{
			pev->sequence = 0;
			ResetSequenceInfo();
		}
		pev->frame = 0;
	}
	else
	{
		pev->framerate = 1.0f;
		StudioFrameAdvance( 0.1f );
		pev->framerate = 0;
		ALERT( at_console, "sequence: %d, frame %.0f\n", pev->sequence, (double)pev->frame );
	}*/

	return 0;
}
#endif

class CCyclerSprite : public CBaseEntity
{
public:
	void Spawn( void );
	void Think( void );
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	virtual int ObjectCaps( void ) { return ( CBaseEntity :: ObjectCaps() | FCAP_DONT_SAVE | FCAP_IMPULSE_USE ); }
	virtual int TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType );
	void Animate( float frames );

	virtual int Save( CSave &save );
	virtual int Restore( CRestore &restore );
	static TYPEDESCRIPTION m_SaveData[];

	inline int ShouldAnimate( void )
	{
		return m_animate && m_maxFrame > 1.0f;
	}

	int m_animate;
	float m_lastTime;
	float m_maxFrame;
};

LINK_ENTITY_TO_CLASS( cycler_sprite, CCyclerSprite )

TYPEDESCRIPTION	CCyclerSprite::m_SaveData[] =
{
	DEFINE_FIELD( CCyclerSprite, m_animate, FIELD_INTEGER ),
	DEFINE_FIELD( CCyclerSprite, m_lastTime, FIELD_TIME ),
	DEFINE_FIELD( CCyclerSprite, m_maxFrame, FIELD_FLOAT ),
};

IMPLEMENT_SAVERESTORE( CCyclerSprite, CBaseEntity )

void CCyclerSprite::Spawn( void )
{
	pev->solid		= SOLID_NOT;
	pev->movetype		= MOVETYPE_NONE;
	pev->takedamage		= DAMAGE_YES;
	pev->effects		= 0;

	pev->frame		= 0;
	pev->nextthink		= gpGlobals->time + 0.1f;
	m_animate		= 1;
	m_lastTime		= gpGlobals->time;

	PRECACHE_MODEL( STRING( pev->model ) );
	SET_MODEL( ENT( pev ), STRING( pev->model ) );

	m_maxFrame = (float)MODEL_FRAMES( pev->modelindex ) - 1;
}

void CCyclerSprite::Think( void )
{
	if( ShouldAnimate() )
		Animate( pev->framerate * ( gpGlobals->time - m_lastTime ) );

	pev->nextthink = gpGlobals->time + 0.1f;
	m_lastTime = gpGlobals->time;
}

void CCyclerSprite::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	m_animate = !m_animate;
	ALERT( at_console, "Sprite: %s\n", STRING( pev->model ) );
}

int CCyclerSprite::TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	if( m_maxFrame > 1.0f )
	{
		Animate( 1.0f );
	}
	return 1;
}

void CCyclerSprite::Animate( float frames )
{ 
	pev->frame += frames;
	if( m_maxFrame > 0 )
		pev->frame = fmod( pev->frame, m_maxFrame );
}

class CWeaponCycler : public CBasePlayerWeapon
{
public:
	void Spawn( void );
	int iItemSlot( void ) { return 1; }
	int GetItemInfo(ItemInfo *p) {return 0; }

	void PrimaryAttack( void );
	void SecondaryAttack( void );
	BOOL Deploy( void );
	void Holster( int skiplocal = 0 );
	int m_iszModel;
	int m_iModel;
};

LINK_ENTITY_TO_CLASS( cycler_weapon, CWeaponCycler )

void CWeaponCycler::Spawn()
{
	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_NONE;

	PRECACHE_MODEL( STRING( pev->model ) );
	SET_MODEL( ENT( pev ), STRING( pev->model ) );
	m_iszModel = pev->model;
	m_iModel = pev->modelindex;

	UTIL_SetOrigin( pev, pev->origin );
	UTIL_SetSize( pev, Vector( -16, -16, 0 ), Vector( 16, 16, 16 ) );
	SetTouch( &CBasePlayerItem::DefaultTouch );
}

BOOL CWeaponCycler::Deploy()
{
	m_pPlayer->pev->viewmodel = m_iszModel;
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 1.0f;
	SendWeaponAnim( 0 );
	m_iClip = 0;
	return TRUE;
}

void CWeaponCycler::Holster( int skiplocal /* = 0 */ )
{
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5f;
}

void CWeaponCycler::PrimaryAttack()
{
	SendWeaponAnim( pev->sequence );

	m_flNextPrimaryAttack = gpGlobals->time + 0.3f;
}

void CWeaponCycler::SecondaryAttack( void )
{
	float flFrameRate, flGroundSpeed;

	pev->sequence = ( pev->sequence + 1 ) % 8;

	pev->modelindex = m_iModel;
	void *pmodel = GET_MODEL_PTR( ENT( pev ) );
	GetSequenceInfo( pmodel, pev, &flFrameRate, &flGroundSpeed );
	pev->modelindex = 0;

	if( flFrameRate == 0.0f )
	{
		pev->sequence = 0;
	}

	SendWeaponAnim( pev->sequence );

	m_flNextSecondaryAttack = gpGlobals->time + 0.3f;
}

// Flaming Wreakage
class CWreckage : public CBaseMonster
{
	int Save( CSave &save );
	int Restore( CRestore &restore );
	static TYPEDESCRIPTION m_SaveData[];

	void Spawn( void );
	void Precache( void );
	void Think( void );

	int m_flStartTime;
};

TYPEDESCRIPTION	CWreckage::m_SaveData[] =
{
	DEFINE_FIELD( CWreckage, m_flStartTime, FIELD_TIME ),
};

IMPLEMENT_SAVERESTORE( CWreckage, CBaseMonster )

LINK_ENTITY_TO_CLASS( cycler_wreckage, CWreckage )

void CWreckage::Spawn( void )
{
	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_NONE;
	pev->takedamage = 0;
	pev->effects = 0;

	pev->frame = 0;
	pev->nextthink = gpGlobals->time + 0.1f;

	if( pev->model )
	{
		PRECACHE_MODEL( STRING( pev->model ) );
		SET_MODEL( ENT( pev ), STRING( pev->model ) );
	}
	// pev->scale = 5.0;

	m_flStartTime = (int)gpGlobals->time;
}

void CWreckage::Precache()
{
	if( pev->model )
		PRECACHE_MODEL( STRING( pev->model ) );
}

void CWreckage::Think( void )
{
	StudioFrameAdvance();
	pev->nextthink = gpGlobals->time + 0.2f;

	if( pev->dmgtime )
	{
		if( pev->dmgtime < gpGlobals->time )
		{
			UTIL_Remove( this );
			return;
		}
		else if( RANDOM_FLOAT( 0, pev->dmgtime - m_flStartTime ) > pev->dmgtime - gpGlobals->time )
		{
			return;
		}
	}

	Vector VecSrc;

	VecSrc.x = RANDOM_FLOAT( pev->absmin.x, pev->absmax.x );
	VecSrc.y = RANDOM_FLOAT( pev->absmin.y, pev->absmax.y );
	VecSrc.z = RANDOM_FLOAT( pev->absmin.z, pev->absmax.z );

	MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, VecSrc );
		WRITE_BYTE( TE_SMOKE );
		WRITE_COORD( VecSrc.x );
		WRITE_COORD( VecSrc.y );
		WRITE_COORD( VecSrc.z );
		WRITE_SHORT( g_sModelIndexSmoke );
		WRITE_BYTE( RANDOM_LONG( 0,49 ) + 50 ); // scale * 10
		WRITE_BYTE( RANDOM_LONG( 0, 3 ) + 8  ); // framerate
	MESSAGE_END();
}
