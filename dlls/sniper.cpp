/************************************************************
*															*
*			Sniper, par Julien								*
*															*
************************************************************/


#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"schedule.h"
#include    "weapons.h"
#include	"soundent.h"
#include	"animation.h"
#include	"effects.h"



#define SNIPER_MAX_CLIP			5

// d

#define HEAD_GROUP		1
#define ARM_L_GROUP		3
#define ARM_R_GROUP		4
#define LEG_L_GROUP		5
#define LEG_R_GROUP		6

#define NO_MEMBRE		1

#define HEAD1			0
#define HEAD2			1
#define HEAD3			2
#define NO_HEAD			3

//=====================================================
// Monster's anim events
// Constantes associ
//=====================================================

#define SNIPER_AE_RELOAD				( 1 )
#define SNIPER_AE_MELEE_ATTACK1			( 2 )
#define SNIPER_AE_SHOOT					( 3 )
#define SNIPER_AE_DROPGUN				( 4 )

//=====================================================
//  Schedule types :
//	Attitudes 
//=====================================================
enum
{
	SCHED_SNIPER_RECHARGEMENT = LAST_COMMON_SCHEDULE + 1,
	SCHED_SNIPER_COVER,
};

//=====================================================
//D
//=====================================================


class CSniper : public CBaseMonster
{
public:
	void	Spawn			( void );						// initialisation
	void	Precache		( void );						// on pr
	void	SetYawSpeed		( void );						//vitesse de rotation
	int		Classify		( void );						// "camp" du monstre : alien ou humain
	int		IRelationship	( CBaseEntity *pTarget );

	void	 Shoot			(void);							// le tir !	
	CBaseEntity	*Kick		( void );						// trace un vecteur et voit s'il touche qqn

	BOOL CheckRangeAttack1 ( float flDot, float flDist );
	BOOL CheckMeleeAttack1 ( float flDot, float flDist );

	void	CheckAmmo		( void );						// v

	void HandleAnimEvent( MonsterEvent_t *pEvent );
	void RunTask ( Task_t *pTask );

	void GibMonster			( void );

	Schedule_t	*GetSchedule( void );						// analyse des bit_COND_ ...
	Schedule_t  *GetScheduleOfType ( int Type );			// ... et en retourne un comportement

	int m_cAmmoLoaded;										// nbre de balles dans le chargeur du sniper
	int m_iShell;

	CUSTOM_SCHEDULES;										// d
	
	static TYPEDESCRIPTION m_SaveData[];
	int	Save( CSave &save ); 
	int Restore( CRestore &restore );

	void MakeGib ( int body, entvars_t *pevAttacker );
	void TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType);

	virtual Activity GetDeathActivity ( void );
};
LINK_ENTITY_TO_CLASS( monster_sniper, CSniper );


TYPEDESCRIPTION	CSniper::m_SaveData[] = 
{
	DEFINE_FIELD( CSniper, m_cAmmoLoaded, FIELD_INTEGER ),
};
IMPLEMENT_SAVERESTORE( CSniper, CBaseMonster );


int	CSniper :: Classify ( void )
{
	return	CLASS_HUMAN_MILITARY;	// copain avec les grunts
}

//====================

void CSniper :: SetYawSpeed ( void )
{
	/*int ys;

	switch ( m_Activity )
	{
	case ACT_IDLE:
	default:
		ys = 50;
		break;
	}

	pev->yaw_speed = ys;*/
	pev->yaw_speed = 360;	// rotation en degr
}

//-----------------------------------------
//
// Spawn / Precache
//
//------------------------------------------

void CSniper :: Spawn()
{
	Precache( );

	SET_MODEL(ENT(pev), "models/sniper.mdl");				// model
	UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);  // taille de la "boite" : constante pour taille standart

	pev->solid			= SOLID_SLIDEBOX;		// la "boite" est solide
	pev->movetype		= MOVETYPE_STEP;		// il ne vole pas
	m_bloodColor		= BLOOD_COLOR_RED;		// couleur du sang
	pev->health			= 75;					// vie (plus tard avec le skill.cfg)
	pev->view_ofs		= Vector ( 0, 0, 73 );	// position of the eyes relative to monster's origin.
	m_flFieldOfView		= VIEW_FIELD_FULL;		// c
	m_MonsterState		= MONSTERSTATE_NONE;	// ?

	m_HackedGunPos		= Vector( 0, 24, 48 );	// position du gun
	m_cAmmoLoaded		= SNIPER_MAX_CLIP;		// nbre de balles dans le chargeur (provisoire)
	ClearConditions(bits_COND_NO_AMMO_LOADED);
	
	m_afCapability		= bits_CAP_HEAR |
						  bits_CAP_RANGE_ATTACK1 |	// capable d'entendre, et deux attaques
						  bits_CAP_MELEE_ATTACK1 ;
	pev->effects		= 0;					// pour le muzzleflash ?

	MonsterInit();								// ?

	m_flDistTooFar		= 4096.0;
	m_flDistLook		= 4096.0;

	SetBodygroup ( HEAD_GROUP, RANDOM_LONG(0,2) );	// t

}


void CSniper :: Precache()
{
	PRECACHE_MODEL("models/sniper.mdl");
	PRECACHE_MODEL("models/sniper_gibs.mdl");

	PRECACHE_SOUND("weapons/fsniper_shoot1.wav");

	m_iShell = PRECACHE_MODEL ("models/sniper_shell.mdl");

}	




//-----------------------------------------
//
// Shoot / Kick
//
//------------------------------------------



void CSniper :: Shoot ( void )
{
	if (m_hEnemy == NULL)
	{
		return;
	}

	Vector vecShootOrigin = GetGunPosition();
	Vector vecShootDir = ShootAtEnemy( vecShootOrigin );

	float m_flDiviation = 0;		// perfect shot 
	UTIL_MakeVectors ( pev->angles );

	Vector	vecShellVelocity = gpGlobals->v_right * RANDOM_FLOAT(40,90) + gpGlobals->v_up * RANDOM_FLOAT(75,200) + gpGlobals->v_forward * RANDOM_FLOAT(-40, 40);
	EjectBrass ( pev->origin + gpGlobals->v_up * 32 + gpGlobals->v_forward * 12, vecShellVelocity, pev->angles.y, m_iShell, TE_BOUNCE_SHELL);

	FireBullets( 1, vecShootOrigin, vecShootDir, VECTOR_CONE_3DEGREES, 4096, 0, 0, 45 );

	EMIT_SOUND(ENT(pev), CHAN_WEAPON, "weapons/fsniper_shoot1.wav", RANDOM_FLOAT(0.6, 0.8), ATTN_NORM);

	m_cAmmoLoaded -- ;
}



CBaseEntity *CSniper :: Kick( void )
{
	TraceResult tr;

	UTIL_MakeVectors( pev->angles );		// == d
	Vector vecStart = pev->origin;							// le vecteur part de l'origine
	vecStart.z += pev->size.z * 0.5;						// du monstre et de la moiti
															// de la hauteur de la "boite"
	Vector vecEnd = vecStart + (gpGlobals->v_forward * 70); // jusqu'

	UTIL_TraceHull( vecStart, vecEnd, dont_ignore_monsters, head_hull, ENT(pev), &tr );  // on trace ce vecteur
	
	if ( tr.pHit )		// 
	{
		CBaseEntity *pEntity = CBaseEntity::Instance( tr.pHit );
		return pEntity;
	}

	return NULL;		// si c'est un coup 
}


//-----------------------------------------
// IRelationship
//
//-----------------------------------------

int CSniper::IRelationship ( CBaseEntity *pTarget )
{
	if ( FClassnameIs( pTarget->pev,  "monster_gargantua" ) )
	{
		return R_NM;
	}

	if ( FClassnameIs( pTarget->pev, "vehicle_tank" ) )
	{
			return R_NO;
	}

	return CBaseMonster::IRelationship( pTarget );
}


//-----------------------------------------
//
// CheckAttacks - 
//------------------------------------------.

void CSniper :: GibMonster ( void )
{
	if ( GetBodygroup( 2 ) != 0 )
		return;

	Vector	vecGunPos = GetGunPosition();
	Vector	vecGunAngles;

	Vector zeroVector(0,0,0);
	GetAttachment( 0, zeroVector, vecGunAngles );
	DropItem( "weapon_fsniper", vecGunPos, vecGunAngles );

	CBaseMonster :: GibMonster();
}


//-----------------------------------------
//
// D
//------------------------------------------.


void CSniper :: TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType)
{
	CBaseMonster :: TraceAttack( pevAttacker, flDamage, vecDir, ptr, bitsDamageType);

	if ( gMultiDamage.pEntity != this )
		return;
	
	if ( ( pev->health - ( gMultiDamage.amount ) <= 0 )  && IsAlive() && m_iHasGibbed == 0 )
	{
		if ( ptr->iHitgroup == HITGROUP_CHEST )
		{
			ptr->iHitgroup = RANDOM_LONG ( 1,3 );
			ptr->iHitgroup = ptr->iHitgroup == 2 ? HITGROUP_LEFTARM : ptr->iHitgroup;
			ptr->iHitgroup = ptr->iHitgroup == 3 ? HITGROUP_RIGHTARM : ptr->iHitgroup;
		}

		switch ( ptr->iHitgroup )
		{
		case HITGROUP_RIGHTARM:
			SetBodygroup( ARM_R_GROUP, NO_MEMBRE);
			MakeGib ( 2, pevAttacker );
			break;
		case HITGROUP_LEFTARM:
			SetBodygroup( ARM_L_GROUP, NO_MEMBRE);
			MakeGib ( 2, pevAttacker );
			break;
		case HITGROUP_RIGHTLEG:
			SetBodygroup( LEG_R_GROUP, NO_MEMBRE);
			MakeGib ( 1, pevAttacker );
			break;
		case HITGROUP_LEFTLEG:
			SetBodygroup( LEG_L_GROUP, NO_MEMBRE);
			MakeGib ( 1, pevAttacker );
			break;
		case HITGROUP_HEAD:
			SetBodygroup( HEAD_GROUP, NO_HEAD);
			MakeGib ( 0, pevAttacker );
			break;
		}
	}

}

void CSniper :: MakeGib ( int body, entvars_t *pevAttacker )
{

	if ( m_iHasGibbed == 1 )
		return;
	m_iHasGibbed = 1;

	CGib *pGib = GetClassPtr( (CGib *)NULL );
	pGib->Spawn( "models/sniper_gibs.mdl" );
	pGib->m_bloodColor = BLOOD_COLOR_RED;
	pGib->pev->body = body;

	pGib->pev->origin = pev->origin + Vector ( 0, 0, 40 );
	pGib->pev->velocity = ( Center() - pevAttacker->origin).Normalize() * 300;
	
	pGib->pev->avelocity.x = RANDOM_FLOAT ( 100, 200 );
	pGib->pev->avelocity.y = RANDOM_FLOAT ( 100, 300 );

}

Activity CSniper :: GetDeathActivity ( void )
{

	Activity	deathActivity = ACT_DIESIMPLE;

	if ( pev->deadflag != DEAD_NO )
		return m_IdealActivity;


	switch ( m_LastHitGroup )
	{
	case HITGROUP_HEAD:
		deathActivity = ACT_DIE_HEADSHOT;
		break;

	case HITGROUP_STOMACH:
	case HITGROUP_CHEST:
	case HITGROUP_LEFTLEG:
	case HITGROUP_RIGHTLEG:
		deathActivity = ACT_DIE_GUTSHOT;
		break;

	case HITGROUP_LEFTARM:
	case HITGROUP_RIGHTARM:
		deathActivity = ACT_DIE_BACKSHOT ;
		break;
	}

	if ( LookupActivity ( deathActivity ) == ACTIVITY_NOT_AVAILABLE )
		deathActivity = ACT_DIESIMPLE;

	return deathActivity;
}

//-----------------------------------------
//
// CheckAttacks
//------------------------------------------


BOOL CSniper :: CheckRangeAttack1 ( float flDot, float flDist )
{
	if ( flDist <= 4096 )
	{
		if ( flDist <= 64 )
			return FALSE;

		TraceResult	tr;
		Vector vecSrc = GetGunPosition();
		Vector vecTarget = m_hEnemy->BodyTarget(vecSrc);

		UTIL_TraceLine( vecSrc, vecTarget, ignore_monsters, ignore_glass, ENT(pev), &tr);

		if ( tr.flFraction == 1.0 )
			return TRUE;

		else if ( !FNullEnt ( tr.pHit ) && VARS( tr.pHit )->takedamage == DAMAGE_YES )
		{
			return TRUE;
		}

	  }

	return FALSE;
}

BOOL CSniper :: CheckMeleeAttack1 ( float flDot, float flDist )
{
	CBaseMonster *pEnemy;

	if ( m_hEnemy != NULL )
	{
		pEnemy = m_hEnemy->MyMonsterPointer();

		if ( !pEnemy )
		{
			return FALSE;
		}
	}

	if ( flDist <= 64 )
		return TRUE;

	return FALSE;
}



void CSniper :: CheckAmmo ( void )
{
	if ( m_cAmmoLoaded <= 0 )
	{
		SetConditions(bits_COND_NO_AMMO_LOADED);
	}
}

//===============


void CSniper :: HandleAnimEvent( MonsterEvent_t *pEvent )
{
	switch (pEvent->event)
	{
	case SNIPER_AE_SHOOT:

		Shoot (); break;

	case SNIPER_AE_RELOAD:

		m_cAmmoLoaded = SNIPER_MAX_CLIP;
		ClearConditions(bits_COND_NO_AMMO_LOADED);
		break;

	case SNIPER_AE_MELEE_ATTACK1:
		{
			CBaseEntity *pHurt = Kick();
			if ( pHurt )
			{
				UTIL_MakeVectors( pev->angles );
				pHurt->pev->punchangle.z = 25;
				pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_forward * 200 + gpGlobals->v_up * 100;
				pHurt->TakeDamage( pev, pev,25, DMG_CLUB );
			}
			break;
		}
	case SNIPER_AE_DROPGUN:
		{
			SetBodygroup( 2, 1);

			Vector	vecGunPos = GetGunPosition();
			Vector	vecGunAngles;

			Vector zeroVector(0,0,0);
			GetAttachment( 0, zeroVector, vecGunAngles );
			DropItem( "weapon_fsniper", vecGunPos, vecGunAngles );
			break;
		}
	}
}

void CSniper :: RunTask ( Task_t *pTask )
{
	switch ( pTask->iTask )
	{
		case TASK_RANGE_ATTACK1:
		{
			if (m_hEnemy != NULL)
			{
				Vector vecShootDir = m_hEnemy->Center() - Center();
				Vector angDir = UTIL_VecToAngles( vecShootDir );
				SetBlending( 0, angDir.x );
			}

			MakeIdealYaw ( m_vecEnemyLKP );
			ChangeYaw ( pev->yaw_speed );

			CBaseMonster :: RunTask( pTask );
			break;
		}

	default:
		{
			CBaseMonster :: RunTask( pTask );
			break;
		}
	}

}





//----------------------------------------------------
//
// AI Schedules
//----------------------------------------------------


//-------------------
// Rechargement

Task_t	tlSniperRechargement[] =
{
	{ TASK_PLAY_SEQUENCE,			(float)ACT_RELOAD			},
};

Schedule_t slSniperRechargement[] = 
{
	{
		tlSniperRechargement,
		ARRAYSIZE ( tlSniperRechargement ),
		bits_COND_HEAVY_DAMAGE	|
		bits_COND_HEAR_SOUND,

		0,
		"SniperRechargement"
	}
};

//-------------------
// abri des grenades

Task_t	tlSniperCover[] =
{
	{ TASK_PLAY_SEQUENCE,			(float)ACT_COWER			},
};

Schedule_t slSniperCover[] = 
{
	{
		tlSniperCover,
		ARRAYSIZE ( tlSniperRechargement ),
		bits_COND_HEAVY_DAMAGE,

		0,
		"SniperCover"
	}
};


//-------------------
// tir
// red
// ne tire pas sans voir l'ennemi



Task_t	tlSniperShoot[] =
{
	{ TASK_STOP_MOVING,			0				},
	{ TASK_FACE_ENEMY,			(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
};

Schedule_t slSniperShoot[] = 
{
	{
		tlSniperShoot,
		ARRAYSIZE ( tlSniperShoot ),
		bits_COND_NEW_ENEMY			|
		bits_COND_ENEMY_DEAD		|
		bits_COND_LIGHT_DAMAGE		|
		bits_COND_HEAVY_DAMAGE		|
		bits_COND_NO_AMMO_LOADED	|
		bits_COND_HEAR_SOUND,

		bits_SOUND_DANGER,
		"SniperShoot"
	}
};


//-------------------
// tableau des AI

DEFINE_CUSTOM_SCHEDULES( CSniper )
{
	slSniperRechargement,
	slSniperCover,
	slSniperShoot,
};

IMPLEMENT_CUSTOM_SCHEDULES( CSniper, CBaseMonster );



//----------------------------------------------------
//
//	Gestion des comportements
//----------------------------------------------------


Schedule_t *CSniper :: GetSchedule( void )
{
	switch	( m_MonsterState )
	{
	
	if ( HasConditions(bits_COND_HEAR_SOUND) )
	{
		CSound *pSound = PBestSound();

		ASSERT( pSound != NULL );
		if ( pSound)
		{
			if (pSound->m_iType & bits_SOUND_DANGER)
			{
				return GetScheduleOfType( SCHED_SNIPER_COVER );
			}
		}
	}


	if ( HasConditions( bits_COND_LIGHT_DAMAGE ) )
	{
		if ( RANDOM_LONG(0,99) <= 75 )
		{
			ClearConditions( bits_COND_LIGHT_DAMAGE );
			return GetScheduleOfType( SCHED_SMALL_FLINCH );
		}
	}

	case MONSTERSTATE_COMBAT:
		{
			if ( HasConditions ( bits_COND_NO_AMMO_LOADED ))
			{
				return GetScheduleOfType ( SCHED_SNIPER_RECHARGEMENT );
			}

			if ( HasConditions ( bits_COND_CAN_MELEE_ATTACK1 ) )
			{
				return GetScheduleOfType ( SCHED_MELEE_ATTACK1 );
			}


			UTIL_MakeVectors ( pev->angles );
			Vector2D vec2LOS = ( m_hEnemy->pev->origin - pev->origin ).Make2D();
			vec2LOS = vec2LOS.Normalize();
			float flDot = DotProduct (vec2LOS , gpGlobals->v_forward.Make2D() );

			if ( CheckRangeAttack1 ( DotProduct (vec2LOS , gpGlobals->v_forward.Make2D() ), (m_hEnemy->pev->origin - pev->origin).Length() ) )
			{
				return GetScheduleOfType ( SCHED_RANGE_ATTACK1 );
			}

		}	// fin du MS_COMBAT
	}		// fin du switch

	return CBaseMonster :: GetSchedule();
}

//----------------------------------------------------
//
//	Direction des comportements vers un tableau d'AI
//----------------------------------------------------


Schedule_t* CSniper :: GetScheduleOfType ( int Type ) 
{
	switch	( Type )
	{
	case SCHED_SNIPER_RECHARGEMENT:
		{
			return &slSniperRechargement[ 0 ];
		}
	case SCHED_SNIPER_COVER:
		{
			return &slSniperCover[ 0 ];	
		}
	case SCHED_RANGE_ATTACK1:
		{
			return &slSniperShoot[ 0 ];	
		}

	default:
		{
			return CBaseMonster :: GetScheduleOfType ( Type );
		}
	}
}

