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
/*

===== turret.cpp ========================================================

*/

//
// TODO: 
//		Take advantage of new monster fields like m_hEnemy and get rid of that OFFSET() stuff
//		Revisit enemy validation stuff, maybe it's not necessary with the newest monster code
//

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "effects.h"
#include "player.h"
#include "gamerules.h"

extern Vector VecBModelOrigin( entvars_t* pevBModel );

extern DLL_GLOBAL int		g_iSkillLevel;

#define TURRET_SHOTS	2
#define TURRET_RANGE	2000
#define TURRET_SPREAD	Vector( 0.02, 0.02, 0.02 )
#define TURRET_TURNRATE	30		//angles per 0.1 second
#define TURRET_MAXWAIT	15		// seconds turret will stay active w/o a target
#define TURRET_MAXSPIN	5		// seconds turret barrel will spin w/o a target
#define TURRET_MACHINE_VOLUME	0.5

typedef enum
{
	TURRET_ANIM_NONE = 0,
	TURRET_ANIM_FIRE,
	TURRET_ANIM_SPIN,
	TURRET_ANIM_DEPLOY,
	TURRET_ANIM_RETIRE,
	TURRET_ANIM_DIE,
} TURRET_ANIM;

class CBaseTurret : public CBaseMonster
{
public:
	void Spawn(void);
	virtual void Precache(void);
	void KeyValue( KeyValueData *pkvd );
	void EXPORT TurretUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );

	int IRelationship ( CBaseEntity *pTarget );

	virtual void TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType);
	virtual int	 TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType );
	virtual int	 Classify(void);

	int BloodColor( void ) { return DONT_BLEED; }
	void GibMonster( void ) {}	// UNDONE: Throw turret gibs?

	// Think functions

	void EXPORT ActiveThink(void);
	void EXPORT SearchThink(void);
	void EXPORT AutoSearchThink(void);
	void EXPORT TurretDeath(void);

	virtual void EXPORT SpinDownCall(void) { m_iSpin = 0; }
	virtual void EXPORT SpinUpCall(void) { m_iSpin = 1; }

	// void SpinDown(void);
	// float EXPORT SpinDownCall( void ) { return SpinDown(); }

	// virtual float SpinDown(void) { return 0;}
	// virtual float Retire(void) { return 0;}

	void EXPORT Deploy(void);
	void EXPORT Retire(void);
	
	void EXPORT Initialize(void);

	virtual void Ping(void);
	virtual void EyeOn(void);
	virtual void EyeOff(void);

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	
	static	TYPEDESCRIPTION m_SaveData[];

	// other functions
	void SetTurretAnim(TURRET_ANIM anim);
	int MoveTurret(void);
	virtual void Shoot(Vector &vecSrc, Vector &vecDirToEnemy) { };

	float m_flMaxSpin;		// Max time to spin the barrel w/o a target
	int m_iSpin;

	CSprite *m_pEyeGlow;
	int		m_eyeBrightness;

	int	m_iDeployHeight;
	int	m_iRetractHeight;
	int m_iMinPitch;

	int m_iBaseTurnRate;	// angles per second
	float m_fTurnRate;		// actual turn rate
	int m_iOrientation;		// 0 = floor, 1 = Ceiling
	int	m_iOn;
	int m_fBeserk;			// Sometimes this bitch will just freak out
	int m_iAutoStart;		// true if the turret auto deploys when a target
							// enters its range

	Vector m_vecLastSight;
	float m_flLastSight;	// Last time we saw a target
	float m_flMaxWait;		// Max time to seach w/o a target
	int m_iSearchSpeed;		// Not Used!

	// movement
	float	m_flStartYaw;
	Vector	m_vecCurAngles;
	Vector	m_vecGoalAngles;


	float	m_flPingTime;	// Time until the next ping, used when searching
	float	m_flSpinUpTime;	// Amount of time until the barrel should spin down when searching
};


TYPEDESCRIPTION	CBaseTurret::m_SaveData[] = 
{
	DEFINE_FIELD( CBaseTurret, m_flMaxSpin, FIELD_FLOAT ),
	DEFINE_FIELD( CBaseTurret, m_iSpin, FIELD_INTEGER ),

	DEFINE_FIELD( CBaseTurret, m_pEyeGlow, FIELD_CLASSPTR ),
	DEFINE_FIELD( CBaseTurret, m_eyeBrightness, FIELD_INTEGER ),
	DEFINE_FIELD( CBaseTurret, m_iDeployHeight, FIELD_INTEGER ),
	DEFINE_FIELD( CBaseTurret, m_iRetractHeight, FIELD_INTEGER ),
	DEFINE_FIELD( CBaseTurret, m_iMinPitch, FIELD_INTEGER ),

	DEFINE_FIELD( CBaseTurret, m_iBaseTurnRate, FIELD_INTEGER ),
	DEFINE_FIELD( CBaseTurret, m_fTurnRate, FIELD_FLOAT ),
	DEFINE_FIELD( CBaseTurret, m_iOrientation, FIELD_INTEGER ),
	DEFINE_FIELD( CBaseTurret, m_iOn, FIELD_INTEGER ),
	DEFINE_FIELD( CBaseTurret, m_fBeserk, FIELD_INTEGER ),
	DEFINE_FIELD( CBaseTurret, m_iAutoStart, FIELD_INTEGER ),


	DEFINE_FIELD( CBaseTurret, m_vecLastSight, FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( CBaseTurret, m_flLastSight, FIELD_TIME ),
	DEFINE_FIELD( CBaseTurret, m_flMaxWait, FIELD_FLOAT ),
	DEFINE_FIELD( CBaseTurret, m_iSearchSpeed, FIELD_INTEGER ),

	DEFINE_FIELD( CBaseTurret, m_flStartYaw, FIELD_FLOAT ),
	DEFINE_FIELD( CBaseTurret, m_vecCurAngles, FIELD_VECTOR ),
	DEFINE_FIELD( CBaseTurret, m_vecGoalAngles, FIELD_VECTOR ),

	DEFINE_FIELD( CBaseTurret, m_flPingTime, FIELD_TIME ),
	DEFINE_FIELD( CBaseTurret, m_flSpinUpTime, FIELD_TIME ),
};

IMPLEMENT_SAVERESTORE( CBaseTurret, CBaseMonster );

class CTurret : public CBaseTurret
{
public:
	void Spawn(void);
	void Precache(void);
	// Think functions
	void SpinUpCall(void);
	void SpinDownCall(void);

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	
	static	TYPEDESCRIPTION m_SaveData[];

	// other functions
	void Shoot(Vector &vecSrc, Vector &vecDirToEnemy);

private:
	int m_iStartSpin;

};
TYPEDESCRIPTION	CTurret::m_SaveData[] = 
{
	DEFINE_FIELD( CTurret, m_iStartSpin, FIELD_INTEGER ),
};

IMPLEMENT_SAVERESTORE( CTurret, CBaseTurret );


class CMiniTurret : public CBaseTurret
{
public:
	void Spawn( );
	void Precache(void);
	// other functions
	void Shoot(Vector &vecSrc, Vector &vecDirToEnemy);
};

LINK_ENTITY_TO_CLASS( monster_turret, CTurret );
LINK_ENTITY_TO_CLASS( monster_miniturret, CMiniTurret );

void CBaseTurret::KeyValue( KeyValueData *pkvd )
{
	if (FStrEq(pkvd->szKeyName, "maxsleep"))
	{
		m_flMaxWait = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "orientation"))
	{
		m_iOrientation = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;

	}
	else if (FStrEq(pkvd->szKeyName, "searchspeed"))
	{
		m_iSearchSpeed = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;

	}
	else if (FStrEq(pkvd->szKeyName, "turnrate"))
	{
		m_iBaseTurnRate = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "style") ||
			 FStrEq(pkvd->szKeyName, "height") ||
			 FStrEq(pkvd->szKeyName, "value1") ||
			 FStrEq(pkvd->szKeyName, "value2") ||
			 FStrEq(pkvd->szKeyName, "value3"))
		pkvd->fHandled = TRUE;
	else
		CBaseMonster::KeyValue( pkvd );
}


void CBaseTurret::Spawn()
{ 
	Precache( );
	pev->nextthink		= gpGlobals->time + 1;
	pev->movetype		= MOVETYPE_FLY;
	pev->sequence		= 0;
	pev->frame			= 0;
	pev->solid			= SOLID_SLIDEBOX;
	pev->takedamage		= DAMAGE_AIM;

	SetBits (pev->flags, FL_MONSTER);
	SetUse( &CBaseTurret::TurretUse );

	if (( pev->spawnflags & SF_MONSTER_TURRET_AUTOACTIVATE ) 
		 && !( pev->spawnflags & SF_MONSTER_TURRET_STARTINACTIVE ))
	{
		m_iAutoStart = TRUE;
	}

	ResetSequenceInfo( );
	SetBoneController( 0, 0 );
	SetBoneController( 1, 0 );
	m_flFieldOfView = VIEW_FIELD_FULL;
	// m_flSightRange = TURRET_RANGE;
}

int CBaseTurret::IRelationship ( CBaseEntity *pTarget )
{
//	if ( FClassnameIs( pev, "monster_turret_tr" ) ){
		if(pTarget->Classify() == CLASS_PLAYER && pev->frags == 72){
		return R_NM;
		}
//	}
	return CBaseMonster::IRelationship( pTarget );
}

void CBaseTurret::Precache( )
{
	PRECACHE_SOUND ("turret/tu_fire1.wav");
	PRECACHE_SOUND ("turret/tu_ping.wav");
	PRECACHE_SOUND ("turret/tu_active2.wav");
	PRECACHE_SOUND ("turret/tu_die.wav");
	PRECACHE_SOUND ("turret/tu_die2.wav");
	PRECACHE_SOUND ("turret/tu_die3.wav");
	// PRECACHE_SOUND ("turret/tu_retract.wav"); // just use deploy sound to save memory
	PRECACHE_SOUND ("turret/tu_deploy.wav");
	PRECACHE_SOUND ("turret/tu_spinup.wav");
	PRECACHE_SOUND ("turret/tu_spindown.wav");
	PRECACHE_SOUND ("turret/tu_search.wav");
	PRECACHE_SOUND ("turret/tu_alert.wav");
}

#define TURRET_GLOW_SPRITE "sprites/flare3.spr"

void CTurret::Spawn()
{ 
	Precache( );
	SET_MODEL(ENT(pev), "models/turret.mdl");

	if (g_iSkillLevel == SKILL_HARD){
	pev->health			= 100;
	}
	else{
	pev->health			= 80;
	}

	m_HackedGunPos		= Vector( 0, 0, 12.75 );
	m_flMaxSpin =		TURRET_MAXSPIN;
	pev->view_ofs.z = 12.75;

	CBaseTurret::Spawn( );

	m_iRetractHeight = 16;
	m_iDeployHeight = 32;
	m_iMinPitch	= -15;
	UTIL_SetSize(pev, Vector(-32, -32, -m_iRetractHeight), Vector(32, 32, m_iRetractHeight));
	
	SetThink(&CTurret::Initialize);	

	m_pEyeGlow = CSprite::SpriteCreate( TURRET_GLOW_SPRITE, pev->origin, FALSE );
	m_pEyeGlow->SetTransparency( kRenderGlow, 255, 0, 0, 0, kRenderFxNoDissipation );
	m_pEyeGlow->SetAttachment( edict(), 2 );
	m_eyeBrightness = 0;

	pev->renderfx = 255;

	pev->nextthink = gpGlobals->time + 0.3; 
	m_killed_exp = 60;
	m_rpgms_level = 40;
	pev->netname = MAKE_STRING( "Turret" );

	if(pev->team == 2){
	pev->health = 800;//�Ѿ�������ǿ��
	m_killed_exp = 1;
	m_rpgms_inteam = 5;
	}
}

void CTurret::Precache()
{
	CBaseTurret::Precache( );
	PRECACHE_MODEL ("models/turret.mdl");	
	PRECACHE_MODEL (TURRET_GLOW_SPRITE);
}

void CMiniTurret::Spawn()
{ 
	Precache( );
	SET_MODEL(ENT(pev), "models/miniturret.mdl");

	if (g_iSkillLevel == SKILL_HARD){
	pev->health			= 70;
	}
	else{
	pev->health			= 60;
	}

	m_HackedGunPos		= Vector( 0, 0, 12.75 );
	m_flMaxSpin = 0;
	pev->view_ofs.z = 12.75;

	CBaseTurret::Spawn( );
	m_iRetractHeight = 16;
	m_iDeployHeight = 32;
	m_iMinPitch	= -15;
	UTIL_SetSize(pev, Vector(-16, -16, -m_iRetractHeight), Vector(16, 16, m_iRetractHeight));
	
	pev->renderfx = 255;

	SetThink(&CMiniTurret::Initialize);	
	pev->nextthink = gpGlobals->time + 0.3; 
	m_killed_exp = 40;
	m_rpgms_level = 20;
	pev->netname = MAKE_STRING( "Mini.Turret" );
}


void CMiniTurret::Precache()
{
	CBaseTurret::Precache( );
	PRECACHE_MODEL ("models/miniturret.mdl");	
	PRECACHE_SOUND("weapons/hks4.wav");
}

void CBaseTurret::Initialize(void)
{
	m_iOn = 0;
	m_fBeserk = 0;
	m_iSpin = 0;

	SetBoneController( 0, 0 );
	SetBoneController( 1, 0 );

	if (m_iBaseTurnRate == 0) m_iBaseTurnRate = TURRET_TURNRATE;
	if (m_flMaxWait == 0) m_flMaxWait = TURRET_MAXWAIT;
	m_flStartYaw = pev->angles.y;
	if (m_iOrientation == 1)
	{
		pev->idealpitch = 180;
		pev->angles.x = 180;
		pev->view_ofs.z = -pev->view_ofs.z;
		pev->effects |= EF_INVLIGHT;
		pev->angles.y = pev->angles.y + 180;
		if (pev->angles.y > 360)
			pev->angles.y = pev->angles.y - 360;
	}

	m_vecGoalAngles.x = 0;

	if (m_iAutoStart)
	{
		m_flLastSight = gpGlobals->time + m_flMaxWait;
		SetThink(&CBaseTurret::AutoSearchThink);		
		pev->nextthink = gpGlobals->time + .1;
	}
	else
		SetThink(&CBaseTurret::SUB_DoNothing);
}

void CBaseTurret::TurretUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if ( !ShouldToggle( useType, m_iOn ) )
		return;

	if (m_iOn)
	{
		m_hEnemy = NULL;
		pev->nextthink = gpGlobals->time + 0.1;
		m_iAutoStart = FALSE;// switching off a turret disables autostart
		//!!!! this should spin down first!!BUGBUG
		SetThink(&CBaseTurret::Retire);
	}
	else 
	{
		pev->nextthink = gpGlobals->time + 0.1; // turn on delay

		// if the turret is flagged as an autoactivate turret, re-enable it's ability open self.
		if ( pev->spawnflags & SF_MONSTER_TURRET_AUTOACTIVATE )
		{
			m_iAutoStart = TRUE;
		}
		
		SetThink(&CBaseTurret::Deploy);
	}
}


void CBaseTurret::Ping( void )
{
	// make the pinging noise every second while searching
	if (m_flPingTime == 0)
		m_flPingTime = gpGlobals->time + 1;
	else if (m_flPingTime <= gpGlobals->time)
	{
		m_flPingTime = gpGlobals->time + 1;
		EMIT_SOUND(ENT(pev), CHAN_ITEM, "turret/tu_ping.wav", 1, ATTN_NORM);
		EyeOn( );
	}
	else if (m_eyeBrightness > 0)
	{
		EyeOff( );
	}
}


void CBaseTurret::EyeOn( )
{
	if (m_pEyeGlow)
	{
		if (m_eyeBrightness != 255)
		{
			m_eyeBrightness = 255;
		}
		m_pEyeGlow->SetBrightness( m_eyeBrightness );
	}
}


void CBaseTurret::EyeOff( )
{
	if (m_pEyeGlow)
	{
		if (m_eyeBrightness > 0)
		{
			m_eyeBrightness = Q_max( 0, m_eyeBrightness - 30 );
			m_pEyeGlow->SetBrightness( m_eyeBrightness );
		}
	}
}


void CBaseTurret::ActiveThink(void)
{
	int fAttack = 0;
	Vector vecDirToEnemy;

	if ( FClassnameIs( pev, "monster_sentry_user" )  ){
		entvars_t *pevOwner = VARS( pev->owner );
		if(FNullEnt(pev->owner)){
			SetTurretAnim(TURRET_ANIM_NONE);
			return;
		}
		else if(pevOwner->deadflag != DEAD_NO){
			SetTurretAnim(TURRET_ANIM_NONE);
			return;
		}
	}


	pev->nextthink = gpGlobals->time + 0.1;
	StudioFrameAdvance( );

	if ((!m_iOn) || (m_hEnemy == NULL))
	{
		m_hEnemy = NULL;
		m_flLastSight = gpGlobals->time + m_flMaxWait;
		SetThink(&CBaseTurret::SearchThink);
		return;
	}
	
	// if it's dead, look for something new
	if ( !m_hEnemy->IsAlive() )
	{
		if (!m_flLastSight)
		{
			m_flLastSight = gpGlobals->time + 0.5; // continue-shooting timeout
		}
		else
		{
			if (gpGlobals->time > m_flLastSight)
			{	
				m_hEnemy = NULL;
				m_flLastSight = gpGlobals->time + m_flMaxWait;
				SetThink(&CBaseTurret::SearchThink);
				return;
			}
		}
	}

	Vector vecMid = pev->origin + pev->view_ofs;
	Vector vecMidEnemy = m_hEnemy->BodyTarget_h( vecMid );

	// Look for our current enemy
	int fEnemyVisible = FBoxVisible(pev, m_hEnemy->pev, vecMidEnemy );	

	vecDirToEnemy = vecMidEnemy - vecMid;	// calculate dir and dist to enemy
	float flDistToEnemy = vecDirToEnemy.Length();

	Vector vec = UTIL_VecToAngles(vecMidEnemy - vecMid);	

	// Current enmey is not visible.
	if (!fEnemyVisible || (flDistToEnemy > TURRET_RANGE))
	{
		if (!m_flLastSight)
			m_flLastSight = gpGlobals->time + 0.5;
		else
		{
			// Should we look for a new target?
			if (gpGlobals->time > m_flLastSight)
			{
				m_hEnemy = NULL;
				m_flLastSight = gpGlobals->time + m_flMaxWait;
				SetThink(&CBaseTurret::SearchThink);
				return;
			}
		}
		fEnemyVisible = 0;
	}
	else
	{
		m_vecLastSight = vecMidEnemy;
	}

	UTIL_MakeAimVectors(m_vecCurAngles);	

	/*
	ALERT( at_console, "%.0f %.0f : %.2f %.2f %.2f\n", 
		m_vecCurAngles.x, m_vecCurAngles.y,
		gpGlobals->v_forward.x, gpGlobals->v_forward.y, gpGlobals->v_forward.z );
	*/
	
	Vector vecLOS = vecDirToEnemy; //vecMid - m_vecLastSight;
	vecLOS = vecLOS.Normalize();

	// Is the Gun looking at the target
	if (DotProduct(vecLOS, gpGlobals->v_forward) <= 0.866) // 30 degree slop
		fAttack = FALSE;
	else
		fAttack = TRUE;

	// fire the gun
	if (m_iSpin && ((fAttack) || (m_fBeserk)))
	{
		Vector vecSrc, vecAng;
		GetAttachment( 0, vecSrc, vecAng );
		SetTurretAnim(TURRET_ANIM_FIRE);
		Shoot(vecSrc, gpGlobals->v_forward );
	} 
	else
	{
		SetTurretAnim(TURRET_ANIM_SPIN);
	}

	//move the gun
	if (m_fBeserk)
	{
		if (RANDOM_LONG(0,9) == 0)
		{
			m_vecGoalAngles.y = RANDOM_FLOAT(0,360);
			m_vecGoalAngles.x = RANDOM_FLOAT(0,90) - 90 * m_iOrientation;
			TakeDamage(pev,pev,1, DMG_GENERIC); // don't beserk forever
			return;
		}
	} 
	else if (fEnemyVisible)
	{
		if (vec.y > 360)
			vec.y -= 360;

		if (vec.y < 0)
			vec.y += 360;

		//ALERT(at_console, "[%.2f]", vec.x);
		
		if (vec.x < -180)
			vec.x += 360;

		if (vec.x > 180)
			vec.x -= 360;

		// now all numbers should be in [1...360]
		// pin to turret limitations to [-90...15]

		if (m_iOrientation == 0)
		{
			if (vec.x > 90)
				vec.x = 90;
			else if (vec.x < m_iMinPitch)
				vec.x = m_iMinPitch;
		}
		else
		{
			if (vec.x < -90)
				vec.x = -90;
			else if (vec.x > -m_iMinPitch)
				vec.x = -m_iMinPitch;
		}

		// ALERT(at_console, "->[%.2f]\n", vec.x);

		m_vecGoalAngles.y = vec.y;
		m_vecGoalAngles.x = vec.x;

	}

	SpinUpCall();
	MoveTurret();
}


void CTurret::Shoot(Vector &vecSrc, Vector &vecDirToEnemy)
{
	int iBulletType;
	iBulletType = BULLET_12MM;

	FireBullets( 1, vecSrc, vecDirToEnemy, TURRET_SPREAD, TURRET_RANGE, iBulletType, 1 );

	EMIT_SOUND(ENT(pev), CHAN_WEAPON, "turret/tu_fire1.wav", 1, 0.5);
	pev->effects = pev->effects | EF_MUZZLEFLASH;
}


void CMiniTurret::Shoot(Vector &vecSrc, Vector &vecDirToEnemy)
{
	int iBulletType;
	iBulletType = BULLET_MONSTER_9MM;

	if(pev->frags >= 72){
	iBulletType = BULLET_9MM;//ѵ�������˺�����
	}

	FireBullets( 1, vecSrc, vecDirToEnemy, TURRET_SPREAD, TURRET_RANGE, iBulletType, 1 );

	EMIT_SOUND(ENT(pev), CHAN_WEAPON, "weapons/hks4.wav", 1, ATTN_NORM);
	
	pev->effects = pev->effects | EF_MUZZLEFLASH;
}


void CBaseTurret::Deploy(void)
{
	pev->nextthink = gpGlobals->time + 0.1;
	StudioFrameAdvance( );

	if (pev->sequence != TURRET_ANIM_DEPLOY)
	{
		m_iOn = 1;
		SetTurretAnim(TURRET_ANIM_DEPLOY);
		EMIT_SOUND(ENT(pev), CHAN_BODY, "turret/tu_deploy.wav", TURRET_MACHINE_VOLUME, ATTN_NORM);
		SUB_UseTargets( this, USE_ON, 0 );
	}

	if (m_fSequenceFinished)
	{
		pev->maxs.z = m_iDeployHeight;
		pev->mins.z = -m_iDeployHeight;
		UTIL_SetSize(pev, pev->mins, pev->maxs);

		m_vecCurAngles.x = 0;

		if (m_iOrientation == 1)
		{
			m_vecCurAngles.y = UTIL_AngleMod( pev->angles.y + 180 );
		}
		else
		{
			m_vecCurAngles.y = UTIL_AngleMod( pev->angles.y );
		}

		SetTurretAnim(TURRET_ANIM_SPIN);
		pev->framerate = 0;
		SetThink(&CBaseTurret::SearchThink);
	}

	m_flLastSight = gpGlobals->time + m_flMaxWait;
}

void CBaseTurret::Retire(void)
{
	// make the turret level
	m_vecGoalAngles.x = 0;
	m_vecGoalAngles.y = m_flStartYaw;

	pev->nextthink = gpGlobals->time + 0.1;

	StudioFrameAdvance( );

	EyeOff( );

	if (!MoveTurret())
	{
		if (m_iSpin)
		{
			SpinDownCall();
		}
		else if (pev->sequence != TURRET_ANIM_RETIRE)
		{
			SetTurretAnim(TURRET_ANIM_RETIRE);
			EMIT_SOUND_DYN(ENT(pev), CHAN_BODY, "turret/tu_deploy.wav", TURRET_MACHINE_VOLUME, ATTN_NORM, 0, 120);
			SUB_UseTargets( this, USE_OFF, 0 );
		}
		else if (m_fSequenceFinished) 
		{	
			m_iOn = 0;
			m_flLastSight = 0;
			SetTurretAnim(TURRET_ANIM_NONE);
			pev->maxs.z = m_iRetractHeight;
			pev->mins.z = -m_iRetractHeight;
			UTIL_SetSize(pev, pev->mins, pev->maxs);
			if (m_iAutoStart)
			{
				SetThink(&CBaseTurret::AutoSearchThink);		
				pev->nextthink = gpGlobals->time + .1;
			}
			else
				SetThink(&CBaseTurret::SUB_DoNothing);
		}
	}
	else
	{
		SetTurretAnim(TURRET_ANIM_SPIN);
	}
}


void CTurret::SpinUpCall(void)
{
	StudioFrameAdvance( );
	pev->nextthink = gpGlobals->time + 0.1;

	// Are we already spun up? If not start the two stage process.
	if (!m_iSpin)
	{
		SetTurretAnim( TURRET_ANIM_SPIN );
		// for the first pass, spin up the the barrel
		if (!m_iStartSpin)
		{
			pev->nextthink = gpGlobals->time + 1.0; // spinup delay
			EMIT_SOUND(ENT(pev), CHAN_BODY, "turret/tu_spinup.wav", TURRET_MACHINE_VOLUME, ATTN_NORM);
			m_iStartSpin = 1;
			pev->framerate = 0.1;
		}
		// after the barrel is spun up, turn on the hum
		else if (pev->framerate >= 1.0)
		{
			pev->nextthink = gpGlobals->time + 0.1; // retarget delay
			EMIT_SOUND(ENT(pev), CHAN_STATIC, "turret/tu_active2.wav", TURRET_MACHINE_VOLUME, ATTN_NORM);
			SetThink(&CBaseTurret::ActiveThink);
			m_iStartSpin = 0;
			m_iSpin = 1;
		} 
		else
		{
			pev->framerate += 0.075;
		}
	}

	if (m_iSpin)
	{
		SetThink(&CBaseTurret::ActiveThink);
	}
}


void CTurret::SpinDownCall(void)
{
	if (m_iSpin)
	{
		SetTurretAnim( TURRET_ANIM_SPIN );
		if (pev->framerate == 1.0)
		{
			EMIT_SOUND_DYN(ENT(pev), CHAN_STATIC, "turret/tu_active2.wav", 0, 0, SND_STOP, 100);
			EMIT_SOUND(ENT(pev), CHAN_ITEM, "turret/tu_spindown.wav", TURRET_MACHINE_VOLUME, ATTN_NORM);
		}
		pev->framerate -= 0.02;
		if (pev->framerate <= 0)
		{
			pev->framerate = 0;
			m_iSpin = 0;
		}
	}
}


void CBaseTurret::SetTurretAnim(TURRET_ANIM anim)
{
	if (pev->sequence != anim)
	{
		switch(anim)
		{
		case TURRET_ANIM_FIRE:
		case TURRET_ANIM_SPIN:
			if (pev->sequence != TURRET_ANIM_FIRE && pev->sequence != TURRET_ANIM_SPIN)
			{
				pev->frame = 0;
			}
			break;
		default:
			pev->frame = 0;
			break;
		}

		pev->sequence = anim;
		ResetSequenceInfo( );

		switch(anim)
		{
		case TURRET_ANIM_RETIRE:
			pev->frame			= 255;
			pev->framerate		= -1.0;
			break;
		case TURRET_ANIM_DIE:
			pev->framerate		= 1.0;
			break;
		}
		//ALERT(at_console, "Turret anim #%d\n", anim);
	}
}


//
// This search function will sit with the turret deployed and look for a new target. 
// After a set amount of time, the barrel will spin down. After m_flMaxWait, the turret will
// retact.
//
void CBaseTurret::SearchThink(void)
{
	if ( FClassnameIs( pev, "monster_sentry_user" )  ){
		entvars_t *pevOwner = VARS( pev->owner );
		if(FNullEnt(pev->owner)){
			SetTurretAnim(TURRET_ANIM_NONE);
			return;
		}
		else if(pevOwner->deadflag != DEAD_NO){
			SetTurretAnim(TURRET_ANIM_NONE);
			return;
		}
	}


	// ensure rethink
	SetTurretAnim(TURRET_ANIM_SPIN);
	StudioFrameAdvance( );
	pev->nextthink = gpGlobals->time + 0.1;

	if (m_flSpinUpTime == 0 && m_flMaxSpin)
		m_flSpinUpTime = gpGlobals->time + m_flMaxSpin;

	Ping( );

	// If we have a target and we're still healthy
	if (m_hEnemy != NULL)
	{
		if (!m_hEnemy->IsAlive() )
			m_hEnemy = NULL;// Dead enemy forces a search for new one
	}


	// Acquire Target
	if (m_hEnemy == NULL)
	{
		Look(TURRET_RANGE);
		m_hEnemy = BestVisibleEnemy();
	}

	// If we've found a target, spin up the barrel and start to attack
	if (m_hEnemy != NULL)
	{
		m_flLastSight = 0;
		m_flSpinUpTime = 0;
		SetThink(&CBaseTurret::ActiveThink);
	}
	else
	{
		// Are we out of time, do we need to retract?
 		if (gpGlobals->time > m_flLastSight)
		{
			//Before we retrace, make sure that we are spun down.
			m_flLastSight = 0;
			m_flSpinUpTime = 0;
			SetThink(&CBaseTurret::Retire);
		}
		// should we stop the spin?
		else if ((m_flSpinUpTime) && (gpGlobals->time > m_flSpinUpTime))
		{
			SpinDownCall();
		}

		if ( !FClassnameIs( pev, "monster_sentry_user" ) ){
			// generic hunt for new victims
			m_vecGoalAngles.y = (m_vecGoalAngles.y + 0.1 * m_fTurnRate);
			if (m_vecGoalAngles.y >= 360)
				m_vecGoalAngles.y -= 360;
			MoveTurret();
		}
	}
}


// 
// This think function will deploy the turret when something comes into range. This is for
// automatically activated turrets.
//
void CBaseTurret::AutoSearchThink(void)
{
	if ( FClassnameIs( pev, "monster_sentry_user" )  ){
		entvars_t *pevOwner = VARS( pev->owner );
		if(FNullEnt(pev->owner)){
			SetTurretAnim(TURRET_ANIM_NONE);
			return;
		}
		else if(pevOwner->deadflag != DEAD_NO){
			SetTurretAnim(TURRET_ANIM_NONE);
			return;
		}
	}


	// ensure rethink
	StudioFrameAdvance( );
	pev->nextthink = gpGlobals->time + 0.3;

	// If we have a target and we're still healthy

	if (m_hEnemy != NULL)
	{
		if (!m_hEnemy->IsAlive() )
			m_hEnemy = NULL;// Dead enemy forces a search for new one
	}

	// Acquire Target

	if (m_hEnemy == NULL)
	{
		Look( TURRET_RANGE );
		m_hEnemy = BestVisibleEnemy();
	}

	if (m_hEnemy != NULL)
	{
		SetThink(&CBaseTurret::Deploy);
		EMIT_SOUND(ENT(pev), CHAN_BODY, "turret/tu_alert.wav", TURRET_MACHINE_VOLUME, ATTN_NORM);
	}
}


void CBaseTurret ::	TurretDeath( void )
{
	BOOL iActive = FALSE;

	StudioFrameAdvance( );
	pev->nextthink = gpGlobals->time + 0.1;

	if (pev->deadflag != DEAD_DEAD)
	{
		pev->deadflag = DEAD_DEAD;

		float flRndSound = RANDOM_FLOAT ( 0 , 1 );

		if ( flRndSound <= 0.33 )
			EMIT_SOUND(ENT(pev), CHAN_BODY, "turret/tu_die.wav", 1.0, ATTN_NORM);
		else if ( flRndSound <= 0.66 )
			EMIT_SOUND(ENT(pev), CHAN_BODY, "turret/tu_die2.wav", 1.0, ATTN_NORM);
		else 
			EMIT_SOUND(ENT(pev), CHAN_BODY, "turret/tu_die3.wav", 1.0, ATTN_NORM);

		EMIT_SOUND_DYN(ENT(pev), CHAN_STATIC, "turret/tu_active2.wav", 0, 0, SND_STOP, 100);

		if (m_iOrientation == 0)
			m_vecGoalAngles.x = -15;
		else
			m_vecGoalAngles.x = -90;

		SetTurretAnim(TURRET_ANIM_DIE); 

		EyeOn( );	
	}

	EyeOff( );

	if (pev->dmgtime + RANDOM_FLOAT( 0, 2 ) > gpGlobals->time)
	{
		// lots of smoke
		MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
			WRITE_BYTE( TE_SMOKE );
			WRITE_COORD( RANDOM_FLOAT( pev->absmin.x, pev->absmax.x ) );
			WRITE_COORD( RANDOM_FLOAT( pev->absmin.y, pev->absmax.y ) );
			WRITE_COORD( pev->origin.z - m_iOrientation * 64 );
			WRITE_SHORT( g_sModelIndexSmoke );
			WRITE_BYTE( 25 ); // scale * 10
			WRITE_BYTE( 10 - m_iOrientation * 5); // framerate
		MESSAGE_END();
	}
	
	if (pev->dmgtime + RANDOM_FLOAT( 0, 5 ) > gpGlobals->time)
	{
		Vector vecSrc = Vector( RANDOM_FLOAT( pev->absmin.x, pev->absmax.x ), RANDOM_FLOAT( pev->absmin.y, pev->absmax.y ), 0 );
		if (m_iOrientation == 0)
			vecSrc = vecSrc + Vector( 0, 0, RANDOM_FLOAT( pev->origin.z, pev->absmax.z ) );
		else
			vecSrc = vecSrc + Vector( 0, 0, RANDOM_FLOAT( pev->absmin.z, pev->origin.z ) );

		UTIL_Sparks( vecSrc );
	}

	if (m_fSequenceFinished && pev->dmgtime + 4 < gpGlobals->time)
	{
		if(pev->team != 2){
			FX_Explosion( Vector(pev->origin.x,pev->origin.y,pev->origin.z - m_iOrientation * 64), EXPLOSION_WHL_SHARD );
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
				if (m_pEyeGlow)
				{
					UTIL_Remove( m_pEyeGlow );
					m_pEyeGlow = NULL;
				}
			UTIL_Remove( this );
			pev->framerate = 0;
			SetThink( NULL );
		}
		else if(pev->dmgtime + 15 < gpGlobals->time){//����!
			pev->health = pev->max_health;
			pev->deadflag = DEAD_NO;
			SetThink( &CBaseTurret::Deploy );
			SetUse( NULL );
			pev->nextthink = gpGlobals->time + 0.1;
		}
	}
}



void CBaseTurret :: TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType)
{
	if ( ptr->iHitgroup == 10 )
	{
		UTIL_Ricochet( ptr->vecEndPos, 1 );
		if(flDamage > 20){
		flDamage -= 20;
		}
		else{
		return;
		}
	}

	if ( !pev->takedamage )
		return;

	UTIL_Sparks( ptr->vecEndPos );

	AddMultiDamage( pevAttacker, this, flDamage, bitsDamageType );
}

// take damage. bitsDamageType indicates type of damage sustained, ie: DMG_BULLET

int CBaseTurret::TakeDamage(entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType)
{
	if ( !pev->takedamage )
		return 0;

	if (!m_iOn || pev->sequence == TURRET_ANIM_DEPLOY)
		flDamage /= 4.0;

	if (bitsDamageType & DMG_BLAST)
	{
		flDamage *= 2.0;
	}

	if (pev->frags >= 72)//ѵ���ã������˺�����
	{
		flDamage *= 1.2;
	}

	pev->health -= flDamage;
	if (pev->health <= 0)
	{
		m_killedbydmg = bitsDamageType;
		pev->health = 0;
		pev->takedamage = DAMAGE_NO;
		pev->dmgtime = gpGlobals->time;

	//	ClearBits (pev->flags, FL_MONSTER); // why are they set in the first place???

	if(m_die == 0){
		CBaseEntity *pEntity = GetClassPtr((CBaseEntity *)pevAttacker);
		if ( (pevAttacker->flags & FL_CLIENT) )//��һ�ɱ
		{
			if (pevAttacker){
			CBasePlayer *pPlayer = GetClassPtr((CBasePlayer *)pevAttacker);
				if(pPlayer){
				pPlayer->TeamMate_expadd(NULL,this);
					if(m_killed_exp >= 1){
					g_pGameRules->DeathNotice( this, pevAttacker, NULL,m_killedbydmg );
					}
				}
			}
		}
		else if ( (pevAttacker->flags & FL_MONSTER) && pEntity->Classify() == CLASS_PLAYER_ALLY )//�����ɱ������Ѿ�
		{
			CBaseMonster *pEnemyMonster;
			pEnemyMonster = pEntity->MyMonsterPointer();
			if(pEnemyMonster){
				if(pEnemyMonster->m_hPlayer != NULL && pEnemyMonster->m_lovehate > 0){

					if(pEnemyMonster->m_rpgms_type == 1 && 
					(pEnemyMonster->m_rpgms_inteam == 0 || pEnemyMonster->m_rpgms_inteam == 5) ){
					pEnemyMonster->m_rpgms_exp += m_killed_exp;
					pEnemyMonster->m_rpgms_maxexp += m_killed_exp;
							if(pEnemyMonster->m_rpgms_inteam == 5 && m_killed_exp >= 1){
							g_pGameRules->DeathNotice( this, pevAttacker, NULL,m_killedbydmg );
							}
					}
					else if(pEnemyMonster->m_rpgms_inteam > 0){
						CBasePlayer *pPlayer = GetClassPtr((CBasePlayer *)pEnemyMonster->m_hPlayer->pev);
						if(pPlayer){
						pPlayer->TeamMate_expadd(pEnemyMonster,this);
							if(m_killed_exp >= 1){
							g_pGameRules->DeathNotice( this, pevAttacker, NULL,m_killedbydmg );
							}
						}
					}	
				}
			}
		}
	}
	m_die = 1;

		if(pev->frags >= 72){//ѵ����
		FireTargets( "turret_kill_count", this, this, USE_TOGGLE, 0 );
		}

		SetUse(NULL);
		SetThink(&CBaseTurret::TurretDeath);
		SUB_UseTargets( this, USE_ON, 0 ); // wake up others
		pev->nextthink = gpGlobals->time + 0.1;

		return 0;
	}

	return 1;
}

int CBaseTurret::MoveTurret(void)
{
	int state = 0;
	// any x movement?
	
	if (m_vecCurAngles.x != m_vecGoalAngles.x)
	{
		float flDir = m_vecGoalAngles.x > m_vecCurAngles.x ? 1 : -1 ;

		m_vecCurAngles.x += 0.1 * m_fTurnRate * flDir;

		// if we started below the goal, and now we're past, peg to goal
		if (flDir == 1)
		{
			if (m_vecCurAngles.x > m_vecGoalAngles.x)
				m_vecCurAngles.x = m_vecGoalAngles.x;
		} 
		else
		{
			if (m_vecCurAngles.x < m_vecGoalAngles.x)
				m_vecCurAngles.x = m_vecGoalAngles.x;
		}

		if (m_iOrientation == 0)
			SetBoneController(1, -m_vecCurAngles.x);
		else
			SetBoneController(1, m_vecCurAngles.x);
		state = 1;
	}

	if (m_vecCurAngles.y != m_vecGoalAngles.y)
	{
		float flDir = m_vecGoalAngles.y > m_vecCurAngles.y ? 1 : -1 ;
		float flDist = fabs(m_vecGoalAngles.y - m_vecCurAngles.y);
		
		if (flDist > 180)
		{
			flDist = 360 - flDist;
			flDir = -flDir;
		}
		if (flDist > 30)
		{
			if (m_fTurnRate < m_iBaseTurnRate * 10)
			{
				m_fTurnRate += m_iBaseTurnRate;
			}
		}
		else if (m_fTurnRate > 45)
		{
			m_fTurnRate -= m_iBaseTurnRate;
		}
		else
		{
			m_fTurnRate += m_iBaseTurnRate;
		}

		m_vecCurAngles.y += 0.1 * m_fTurnRate * flDir;

		if (m_vecCurAngles.y < 0)
			m_vecCurAngles.y += 360;
		else if (m_vecCurAngles.y >= 360)
			m_vecCurAngles.y -= 360;

		if (flDist < (0.05 * m_iBaseTurnRate))
			m_vecCurAngles.y = m_vecGoalAngles.y;

		//ALERT(at_console, "%.2f -> %.2f\n", m_vecCurAngles.y, y);
		if (m_iOrientation == 0)
			SetBoneController(0, m_vecCurAngles.y - pev->angles.y );
		else 
			SetBoneController(0, pev->angles.y - 180 - m_vecCurAngles.y );
		state = 1;
	}

	if (!state)
		m_fTurnRate = m_iBaseTurnRate;

	//ALERT(at_console, "(%.2f, %.2f)->(%.2f, %.2f)\n", m_vecCurAngles.x, 
	//	m_vecCurAngles.y, m_vecGoalAngles.x, m_vecGoalAngles.y);
	return state;
}

//
// ID as a machine
//
int	CBaseTurret::Classify ( void )
{
	if (m_iOn || m_iAutoStart){
		if ( FClassnameIs( pev, "monster_sentry" ) || FClassnameIs( pev, "monster_sentry_user" ) ){
		return	CLASS_MACHINE;
		}
		else if ( pev->team == 1 ){
		return	CLASS_MACHINE;
		}
		else if ( pev->team == 2 ){
		return	CLASS_MACHINE_BLACK;
		}
		else{
		return	CLASS_MACHINE_ASS;
		}
	}
	return CLASS_NONE;
}




//=========================================================
// Sentry gun - smallest turret, placed near grunt entrenchments
//=========================================================
class CSentry : public CBaseTurret
{
public:
	void Spawn( );
	void Precache(void);
	// other functions
	void Shoot(Vector &vecSrc, Vector &vecDirToEnemy);
	int TakeDamage(entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType);
	void EXPORT SentryTouch( CBaseEntity *pOther );
	void EXPORT SentryDeath( void );

};

LINK_ENTITY_TO_CLASS( monster_sentry, CSentry );
LINK_ENTITY_TO_CLASS( monster_sentry_user, CSentry );

void CSentry::Precache()
{
	CBaseTurret::Precache( );
	PRECACHE_MODEL ("models/sentry.mdl");	
//	PRECACHE_MODEL ("models/sentry_user.mdl");
	PRECACHE_SOUND ("weapons/357_shoot1.wav");
	PRECACHE_SOUND("weapons/hks4.wav");
}

void CSentry::Spawn()
{ 
	Precache( );
	SET_MODEL(ENT(pev), "models/sentry.mdl");

	if (g_iSkillLevel == SKILL_HARD){
	pev->health			= 70;
	}
	else{
	pev->health			= 60;
	}

	m_HackedGunPos		= Vector( 0, 0, 48 );
	pev->view_ofs.z		= 48;
	m_flMaxWait = 1E6;
	m_flMaxSpin	= 1E6;

	CBaseTurret::Spawn();
	m_iRetractHeight = 64;
	m_iDeployHeight = 64;
	m_iMinPitch	= -60;
	UTIL_SetSize(pev, Vector(-16, -16, 0), Vector(16, 16, m_iRetractHeight));

	if ( FClassnameIs( pev, "monster_sentry_user" ) ){
	SET_MODEL(ENT(pev), "models/sentry_user.mdl");
	pev->takedamage = DAMAGE_NO;
	pev->view_ofs.z		= 24;
	pev->flags |= FL_FROZEN;
	m_iRetractHeight = 32;
	m_iDeployHeight = 32;
	UTIL_SetSize(pev, Vector(-16, -16, 0), Vector(16, 16, 16));
	}

	SetTouch(&CSentry::SentryTouch);
	SetThink(&CSentry::Initialize);	
	pev->nextthink = gpGlobals->time + 0.3; 
	m_killed_exp = 40;
	m_rpgms_level = 30;
	pev->netname = MAKE_STRING( "Sentry" );
}

void CSentry::Shoot(Vector &vecSrc, Vector &vecDirToEnemy)
{
	if ( FClassnameIs( pev, "monster_sentry_user" ) ){
		int iBulletType;
		iBulletType = BULLET_MONSTER_9MM;

		FireBullets( 1, vecSrc, vecDirToEnemy, Vector(0.03,0.03,0.03), 4096, iBulletType, 1 );
		
		EMIT_SOUND(ENT(pev), CHAN_WEAPON, "weapons/357_shoot1.wav", 1, 0.3); 

		pev->effects = pev->effects | EF_MUZZLEFLASH;
	}
	else{
		int iBulletType;
		iBulletType = BULLET_MONSTER_9MM;

		FireBullets( 1, vecSrc, vecDirToEnemy, TURRET_SPREAD, TURRET_RANGE, iBulletType, 1 );
		EMIT_SOUND(ENT(pev), CHAN_WEAPON, "weapons/hks4.wav", 1, ATTN_NORM);
		pev->effects = pev->effects | EF_MUZZLEFLASH;
	}
}

int CSentry::TakeDamage(entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType)
{
	if ( !pev->takedamage )
		return 0;

	if (!m_iOn)
	{
		SetThink( &CSentry::Deploy );
		SetUse( NULL );
		pev->nextthink = gpGlobals->time + 0.1;
	}

	if (bitsDamageType & DMG_BLAST)
	{
		flDamage *= 2.0;
	}

	pev->health -= flDamage;
	if (pev->health <= 0)
	{
		pev->health = 0;
		pev->takedamage = DAMAGE_NO;
		pev->dmgtime = gpGlobals->time;

	//	ClearBits (pev->flags, FL_MONSTER); // why are they set in the first place???

	if(m_die == 0){
		CBaseEntity *pEntity = GetClassPtr((CBaseEntity *)pevAttacker);
		if ( (pevAttacker->flags & FL_CLIENT) )//��һ�ɱ
		{
			if (pevAttacker){
			CBasePlayer *pPlayer = GetClassPtr((CBasePlayer *)pevAttacker);
				if(pPlayer){
				pPlayer->TeamMate_expadd(NULL,this);
					if(m_killed_exp >= 1){
					g_pGameRules->DeathNotice( this, pevAttacker, NULL,m_killedbydmg );
					}
				}
			}
		}
		else if ( (pevAttacker->flags & FL_MONSTER) && pEntity->Classify() == CLASS_PLAYER_ALLY )//�����ɱ������Ѿ�
		{
			CBaseMonster *pEnemyMonster;
			pEnemyMonster = pEntity->MyMonsterPointer();
			if(pEnemyMonster){
				if(pEnemyMonster->m_hPlayer != NULL && pEnemyMonster->m_lovehate > 0){

					if(pEnemyMonster->m_rpgms_type == 1 && 
					(pEnemyMonster->m_rpgms_inteam == 0 || pEnemyMonster->m_rpgms_inteam == 5) ){
					pEnemyMonster->m_rpgms_exp += m_killed_exp;
					pEnemyMonster->m_rpgms_maxexp += m_killed_exp;
							if(pEnemyMonster->m_rpgms_inteam == 5 && m_killed_exp >= 1){
							g_pGameRules->DeathNotice( this, pevAttacker, NULL,m_killedbydmg );
							}
					}
					else if(pEnemyMonster->m_rpgms_inteam > 0){
						CBasePlayer *pPlayer = GetClassPtr((CBasePlayer *)pEnemyMonster->m_hPlayer->pev);
						if(pPlayer){
						pPlayer->TeamMate_expadd(pEnemyMonster,this);
							if(m_killed_exp >= 1){
							g_pGameRules->DeathNotice( this, pevAttacker, NULL,m_killedbydmg );
							}
						}
					}	
				}
			}
		}
	}
	m_die = 1;

		SetUse(NULL);
		SetThink(&CSentry::SentryDeath);
		SUB_UseTargets( this, USE_ON, 0 ); // wake up others
		pev->nextthink = gpGlobals->time + 0.1;

		return 0;
	}

	return 1;
}


void CSentry::SentryTouch( CBaseEntity *pOther )
{
	if ( pOther && (pOther->IsPlayer() || (pOther->pev->flags & FL_MONSTER)) )
	{
		TakeDamage(pOther->pev, pOther->pev, 0, 0 );
	}
}


void CSentry ::	SentryDeath( void )
{
	BOOL iActive = FALSE;

	StudioFrameAdvance( );
	pev->nextthink = gpGlobals->time + 0.1;

	if (pev->deadflag != DEAD_DEAD)
	{
		pev->deadflag = DEAD_DEAD;

		float flRndSound = RANDOM_FLOAT ( 0 , 1 );

		if ( flRndSound <= 0.33 )
			EMIT_SOUND(ENT(pev), CHAN_BODY, "turret/tu_die.wav", 1.0, ATTN_NORM);
		else if ( flRndSound <= 0.66 )
			EMIT_SOUND(ENT(pev), CHAN_BODY, "turret/tu_die2.wav", 1.0, ATTN_NORM);
		else 
			EMIT_SOUND(ENT(pev), CHAN_BODY, "turret/tu_die3.wav", 1.0, ATTN_NORM);

		EMIT_SOUND_DYN(ENT(pev), CHAN_STATIC, "turret/tu_active2.wav", 0, 0, SND_STOP, 100);

		SetBoneController( 0, 0 );
		SetBoneController( 1, 0 );

		SetTurretAnim(TURRET_ANIM_DIE); 

		pev->solid = SOLID_NOT;

		if(pev->impulse == 1){
		pev->angles.y = 0;//���½Ƕȹ̶�0
		}
		else if(pev->impulse == 2){
		pev->angles.y = 90;//���½Ƕȹ̶�90
		}
		else{//���½Ƕ����
		pev->angles.y = UTIL_AngleMod( pev->angles.y + RANDOM_LONG( 0, 2 ) * 120 );
		}

		EyeOn( );
	}

	EyeOff( );

	Vector vecSrc, vecAng;
	GetAttachment( 1, vecSrc, vecAng );

	if (pev->dmgtime + RANDOM_FLOAT( 0, 2 ) > gpGlobals->time)
	{
		// lots of smoke
		MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
			WRITE_BYTE( TE_SMOKE );
			WRITE_COORD( vecSrc.x + RANDOM_FLOAT( -16, 16 ) );
			WRITE_COORD( vecSrc.y + RANDOM_FLOAT( -16, 16 ) );
			WRITE_COORD( vecSrc.z - 32 );
			WRITE_SHORT( g_sModelIndexSmoke );
			WRITE_BYTE( 15 ); // scale * 10
			WRITE_BYTE( 8 ); // framerate
		MESSAGE_END();
	}
	
	if (pev->dmgtime + RANDOM_FLOAT( 0, 8 ) > gpGlobals->time)
	{
		UTIL_Sparks( vecSrc );
	}

	if (m_fSequenceFinished && pev->dmgtime + 4 < gpGlobals->time)
	{
		FX_Explosion( pev->origin + Vector(0,0,5), EXPLOSION_WHL_SHARD );
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

		UTIL_Remove( this );
		pev->framerate = 0;
		SetThink( NULL );
	}
}

