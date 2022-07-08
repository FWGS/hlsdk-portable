/************************************************************
*															*
*			Diablo, par Julien								*
*															*
************************************************************/


#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"schedule.h"
#include    "weapons.h"


//=====================================================
// Monster's anim events
// Constantes associ
//=====================================================

#define DIABLO_AE_KICK_NORMAL			( 1 )
#define DIABLO_AE_KICK_BAS				( 2 )
#define DIABLO_AE_KICK_LOIN				( 3 )
#define DIABLO_AE_STEP					( 4 )


//=====================================================
//  Schedule types :
//	Attitudes 
//=====================================================
enum
{
	SCHED_DIABLO_RANGE_ATTACK1,
	SCHED_DIABLO_RANGE_ATTACK2,
};



//=====================================================
//D
//=====================================================


class CDiablo : public CBaseMonster
{
public:
	void Spawn( void );					// initialisation
	void Precache( void );				// on pr
	void SetYawSpeed( void );			// vitesse de rotation
	int  Classify ( void );				// "camp" du monstre : alien ou humain

	void HandleAnimEvent( MonsterEvent_t *pEvent );
	Schedule_t	*GetSchedule( void );						// analyse des bit_COND_ ...
	Schedule_t  *GetScheduleOfType ( int Type );			// ... et en retourne un comportement

	BOOL CheckRangeAttack1 ( float flDot , float flDist );
	BOOL CheckRangeAttack2 ( float flDot , float flDist );
	BOOL CheckMeleeAttack1 ( float flDot , float flDist );
	BOOL CheckMeleeAttack2 ( float flDot , float flDist );

	CBaseEntity	*KickNormal( void );
	CBaseEntity	*KickBas( void );
	CBaseEntity	*KickLoin( void );

	void SonFrappe ( BOOL Touche );

	void TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType);

	CUSTOM_SCHEDULES;										// d

	//===
};
LINK_ENTITY_TO_CLASS( monster_panther, CDiablo );


//====================================================
// Classification
//====================================================

int	CDiablo :: Classify ( void )
{
	return	CLASS_ALIEN_MONSTER;	// ami avec les aliens
}

//====================================================
// Vitesse de rotation
//====================================================

void CDiablo :: SetYawSpeed ( void )
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
	pev->yaw_speed = 64;	// rotation en degr
}


//====================================================
// Initialisation
//====================================================

void CDiablo :: Spawn()
{
	Precache( );

	SET_MODEL(ENT(pev), "models/diablo.mdl");				// model
//	UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);  // taille de la "boite" : constante pour taille standart
	UTIL_SetSize(pev, Vector (-32, -16, 0 ), Vector ( 32, 16, 60) );  

	pev->solid			= SOLID_SLIDEBOX;	// la "boite" est solide
	pev->movetype		= MOVETYPE_STEP;	// il ne vole pas
	m_bloodColor		= BLOOD_COLOR_YELLOW;	// couleur du sang
	pev->health			= 200;				// vie (plus tard avec le skill.cfg)
//	pev->view_ofs		= Vector ( 0, 0, 6 );// position of the eyes relative to monster's origin.
	m_flFieldOfView		= VIEW_FIELD_FULL;  // c
	m_MonsterState		= MONSTERSTATE_NONE;// ?

	//=========
	m_afCapability		= bits_CAP_HEAR |
						  bits_CAP_RANGE_ATTACK1 |	// capable d'entendre, et deux attaques
						  //bits_CAP_RANGE_ATTACK2 |	// plus une de proximit
						  bits_CAP_MELEE_ATTACK1 |
						  bits_CAP_MELEE_ATTACK2 ;
	pev->effects		= 0;				// pour le muzzleflash ?
	//=========

	MonsterInit(); // ?
}

//==================
//==================

void CDiablo :: Precache()
{
	PRECACHE_MODEL("models/diablo.mdl");

	PRECACHE_SOUND( "zombie/claw_miss1.wav" );
	PRECACHE_SOUND( "zombie/claw_miss2.wav" );

	PRECACHE_SOUND( "zombie/claw_strike1.wav" );
	PRECACHE_SOUND( "zombie/claw_strike2.wav" );
	PRECACHE_SOUND( "zombie/claw_strike3.wav" );


}


//=================================================
//	Check Attacks
//=================================================

BOOL CDiablo :: CheckRangeAttack1 ( float flDot , float flDist )
{
	if ( flDist >= 128 )
	{
		return TRUE;
	}
	return FALSE;
}


BOOL CDiablo :: CheckRangeAttack2 ( float flDot , float flDist )
{
	if ( flDist < 128 )
	{
		return TRUE;
	}
	return FALSE;
}


BOOL CDiablo :: CheckMeleeAttack1 ( float flDot, float flDist )
{
	if ( flDist <= 64 && flDot >= 0.7 && m_hEnemy != 0 && FBitSet ( m_hEnemy->pev->flags, FL_ONGROUND ) )
	{
		return TRUE;
	}
	return FALSE;
}


BOOL CDiablo :: CheckMeleeAttack2 ( float flDot, float flDist )
{
	if ( flDist <= 92 && flDot >= 0.7 && m_hEnemy != 0 && FBitSet ( m_hEnemy->pev->flags, FL_ONGROUND ) )
	{
		return TRUE;
	}
	return FALSE;
}

//==================================================
// Coups
//==================================================

CBaseEntity *CDiablo :: KickNormal( void )
{
	TraceResult tr;

	UTIL_MakeVectors( pev->angles );		// == d
	Vector vecStart = pev->origin;							// le vecteur part de l'origine
	vecStart.z += 42;									// du monstre + 42 unit
															// de haut
	Vector vecEnd = vecStart + (gpGlobals->v_forward * 64); // jusqu'

	UTIL_TraceHull( vecStart, vecEnd, dont_ignore_monsters, head_hull, ENT(pev), &tr );  // on trace ce vecteur
	
	if ( tr.pHit )		// 
	{
		CBaseEntity *pEntity = CBaseEntity::Instance( tr.pHit );
		return pEntity;
	}

	return NULL;		// si c'est un coup 
}


CBaseEntity *CDiablo :: KickBas( void )
{
	TraceResult tr;

	UTIL_MakeVectors( pev->angles );		
	Vector vecStart = pev->origin;
	vecStart.z += 30;

	Vector vecEnd = vecStart + (gpGlobals->v_forward * 64); 

	UTIL_TraceHull( vecStart, vecEnd, dont_ignore_monsters, head_hull, ENT(pev), &tr );
	
	if ( tr.pHit )
	{
		CBaseEntity *pEntity = CBaseEntity::Instance( tr.pHit );
		return pEntity;
	}

	return NULL;
}


CBaseEntity *CDiablo :: KickLoin( void )
{
	TraceResult tr;

	UTIL_MakeVectors( pev->angles );		// == d
	Vector vecStart = pev->origin;							// le vecteur part de l'origine
	vecStart.z += 42;									// du monstre + 42 unit
															// de haut
	Vector vecEnd = vecStart + (gpGlobals->v_forward * 92); // jusqu'

	UTIL_TraceHull( vecStart, vecEnd, dont_ignore_monsters, head_hull, ENT(pev), &tr );  // on trace ce vecteur
	
	if ( tr.pHit )
	{
		CBaseEntity *pEntity = CBaseEntity::Instance( tr.pHit );
		return pEntity;
	}

	return NULL;
}

//================================================
// Sons du monstre 
//================================================

void CDiablo :: SonFrappe ( BOOL Touche )
{
	if ( Touche )
	{
		switch ( RANDOM_LONG(0,2) )
		{
		case 0:	
			EMIT_SOUND( ENT(pev), CHAN_WEAPON, "zombie/claw_strike1.wav", 1, ATTN_STATIC );	
			break;
		case 1:
			EMIT_SOUND( ENT(pev), CHAN_WEAPON, "zombie/claw_strike2.wav", 1, ATTN_STATIC );	
			break;
		case 2:
			EMIT_SOUND( ENT(pev), CHAN_WEAPON, "zombie/claw_strike3.wav", 1, ATTN_STATIC );	
			break;
		}
	}
	else if ( !Touche )
	{
		switch ( RANDOM_LONG(0,1) )
		{
		case 0:	
			EMIT_SOUND( ENT(pev), CHAN_WEAPON, "zombie/claw_miss1.wav", 1, ATTN_STATIC );	
			break;
		case 1:
			EMIT_SOUND( ENT(pev), CHAN_WEAPON, "zombie/claw_miss2.wav", 1, ATTN_STATIC );	
			break;
		}
	}
}

//===============================================
// Ricochets
//===============================================

void CDiablo :: TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType)
{
	ALERT ( at_console , "vie :%d" , pev->health );
	if ( ptr->iHitgroup != 2 )			// le tir n'est pas dans l'oeil
	{
		flDamage -= 30;					// absorbe les dommages
		if (flDamage <= 0)				// aucun d
		{
			UTIL_Ricochet( ptr->vecEndPos, 1.0 );
			flDamage = 0;
		}
	}
	else
	{ 		ALERT ( at_console , "DANS L'OEIL !!!!\n" ); }	// 

	CBaseMonster::TraceAttack( pevAttacker, flDamage, vecDir, ptr, bitsDamageType );
}
		




//================================================
// Handle Anim Event :
// 
//================================================

void CDiablo :: HandleAnimEvent( MonsterEvent_t *pEvent )
{

	switch (pEvent->event)
	{

	case DIABLO_AE_KICK_NORMAL:
		{
			CBaseEntity *pHurtNormal = KickNormal();
			if ( pHurtNormal )
			{
				UTIL_MakeVectors( pev->angles );
				pHurtNormal->pev->velocity = pHurtNormal->pev->velocity + gpGlobals->v_forward * 100 + gpGlobals->v_up * 200;
				pHurtNormal->TakeDamage( pev, pev,25, DMG_CLUB );
				SonFrappe ( TRUE ) ;
			}
			else
			{
				SonFrappe ( FALSE );
			}
			break;
		}

	case DIABLO_AE_KICK_BAS:
		{
			CBaseEntity *pHurtBas = KickBas();
			if ( pHurtBas )
			{
				UTIL_MakeVectors( pev->angles );
				pHurtBas->pev->velocity = pHurtBas->pev->velocity + gpGlobals->v_forward * 75 + gpGlobals->v_up * 75;
				pHurtBas->TakeDamage( pev, pev,15, DMG_BULLET );
				SonFrappe ( TRUE ) ;
			}
			else
			{
				SonFrappe ( FALSE );
			}
			break;
		}

	case DIABLO_AE_KICK_LOIN:
		{
			CBaseEntity *pHurtLoin = KickLoin();
			if ( pHurtLoin )
			{
				UTIL_MakeVectors( pev->angles );
				pHurtLoin->pev->velocity = pHurtLoin->pev->velocity + gpGlobals->v_forward * 100 + gpGlobals->v_up * 200;
				pHurtLoin->TakeDamage( pev, pev,25, DMG_CLUB );
				SonFrappe ( TRUE ) ;
			}
			else
			{
				SonFrappe ( FALSE );
			}
			break;
		}

	case DIABLO_AE_STEP:
		{
			switch ( RANDOM_LONG(0,3) )
			{
				case 0:	EMIT_SOUND_DYN ( ENT(pev), CHAN_BODY, "player/pl_step1.wav", 1, ATTN_NORM, 0, 70 );	break;
				case 1:	EMIT_SOUND_DYN ( ENT(pev), CHAN_BODY, "player/pl_step2.wav", 1, ATTN_NORM, 0, 70 );	break;
				case 2:	EMIT_SOUND_DYN ( ENT(pev), CHAN_BODY, "player/pl_step3.wav", 1, ATTN_NORM, 0, 70 );	break;
				case 3:	EMIT_SOUND_DYN ( ENT(pev), CHAN_BODY, "player/pl_step4.wav", 1, ATTN_NORM, 0, 70 );	break;
			}
		}
	}
}



//=================================================
// AI Schedules
//=================================================

//=== court vers l'ennemi lorsqu' il en est loin ===

Task_t	tlDiabloRangeAttack1[] =
{
	{ TASK_SET_FAIL_SCHEDULE,	(float)SCHED_TAKE_COVER_FROM_ENEMY	},// S'il ne peut pas aller vers le joueur
	{ TASK_GET_PATH_TO_ENEMY,	(float)0	},	// localise l'ennemi, trace un chemin,
	{ TASK_RUN_PATH,			(float)0	},	// se pr
	{ TASK_WAIT_FOR_MOVEMENT,	(float)0	},	// et se lance

};

Schedule_t	slDiabloRangeAttack1[] =
{
	{ 
		tlDiabloRangeAttack1,
		ARRAYSIZE ( tlDiabloRangeAttack1 ), 
		bits_COND_NEW_ENEMY			|	
		bits_COND_ENEMY_DEAD		|	// ne s'arrete dans sa cours que sous 
		bits_COND_LIGHT_DAMAGE		|	// certaines conditions :
		bits_COND_HEAVY_DAMAGE		|	// peut frapper l'ennemi, entre autres ...
		bits_COND_ENEMY_OCCLUDED	|
		bits_COND_NO_AMMO_LOADED	|
		bits_COND_CAN_MELEE_ATTACK1	|
		bits_COND_HEAR_SOUND,
		
		//bits_SOUND_DANGER,
		//"Range Attack1"
	},
};

//=== marche vers l'ennemi lorsqu'il en est proche ===

Task_t	tlDiabloRangeAttack2[] =
{

	{ TASK_GET_PATH_TO_ENEMY,	(float)0	},
	{ TASK_WALK_PATH,			(float)0	},
	{ TASK_WAIT_FOR_MOVEMENT,	(float)0	},

};

Schedule_t	slDiabloRangeAttack2[] =
{
	{ 
		tlDiabloRangeAttack2,
		ARRAYSIZE ( tlDiabloRangeAttack2 ), 
		bits_COND_NEW_ENEMY			|
		bits_COND_ENEMY_DEAD		|
		bits_COND_LIGHT_DAMAGE		|
		bits_COND_HEAVY_DAMAGE		|
		bits_COND_ENEMY_OCCLUDED	|
		bits_COND_NO_AMMO_LOADED	|
		bits_COND_CAN_RANGE_ATTACK1	|
		bits_COND_CAN_MELEE_ATTACK1	|
		bits_COND_HEAR_SOUND,
		
		//bits_SOUND_DANGER,
		//"Range Attack1"
	},
};



DEFINE_CUSTOM_SCHEDULES( CDiablo )
{
	slDiabloRangeAttack1,
};

IMPLEMENT_CUSTOM_SCHEDULES( CDiablo, CBaseMonster );


//==================================


Schedule_t *CDiablo :: GetSchedule( void )
{
	switch	( m_MonsterState )
	{
	case MONSTERSTATE_COMBAT:
		{
			if ( HasConditions ( bits_COND_CAN_MELEE_ATTACK1 ) )
			{
				return GetScheduleOfType ( SCHED_MELEE_ATTACK1 );
			}

			if ( HasConditions ( bits_COND_CAN_MELEE_ATTACK2 ) )
			{
				return GetScheduleOfType ( SCHED_MELEE_ATTACK2 );
			}

			if ( HasConditions(bits_COND_CAN_RANGE_ATTACK1) )
			{
				return GetScheduleOfType( SCHED_DIABLO_RANGE_ATTACK1 );
			}

			if ( HasConditions(bits_COND_CAN_RANGE_ATTACK2) )
			{
				return GetScheduleOfType( SCHED_DIABLO_RANGE_ATTACK2 );
			}

			if ( pev->health <= 75 )
			{
				return GetScheduleOfType( SCHED_TAKE_COVER_FROM_ENEMY );
			}

		}	// fin du MS_COMBAT
	}		// fin du switch

	return CBaseMonster :: GetSchedule();
	/* si aucune des solutions ici 
	ne convient,on appelle le GetSchedule
	de la classe de base */

}

//===========================


Schedule_t* CDiablo :: GetScheduleOfType ( int Type ) 
{
	switch	( Type )
	{
	case SCHED_DIABLO_RANGE_ATTACK1:
		{
			return &slDiabloRangeAttack1[ 0 ];
		}

		case SCHED_DIABLO_RANGE_ATTACK2:
		{
			return &slDiabloRangeAttack2[ 0 ];
		}


	default:
		{
			return CBaseMonster :: GetScheduleOfType ( Type );	// la classe de base s'occuppe du reste
		}
	}
}



	
