/****************************************************************************
*																			*
*			Tank.cpp par Julien												*
*																			*
****************************************************************************/


#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "soundent.h"
#include "effects.h"
#include "player.h"
#include "explode.h"
#include "func_break.h"


//========================================
// Fonctions externes
//========================================

extern void EnvSmokeCreate( const Vector &center, int m_iScale, float m_fFrameRate, int m_iTime, int m_iEndTime );

extern int gmsgTankView;

//=========================================
// Variables
//=========================================

#define NEXTTHINK_TIME				0.1
#define BSP_NEXTTHINK_TIME			0.05

#define TANK_TOURELLE_ROT_SPEED		12
#define TANK_ROT_SPEED				50

#define TANK_REFIRE_DELAY			1.5

#define SPRITE_SMOKE				("sprites/muzzleflash.spr")
#define SPRITE_SMOKE_SCALE			1.5
#define SPRITE_MUZ					("sprites/muzzleflash1.spr")
#define SPRITE_MUZ_SCALE			1
#define SPRITE_FEU					("sprites/lflammes02.spr")
#define SPRITE_FEU_SCALE			1
#define SPRITE_SMOKEBALL			("sprites/tank_smokeball.spr")
#define SPRITE_SMOKEBALL_SCALE		2

#define MITRAILLEUSE_SOUND			"tank/mitrailleuse.wav"
#define TIR_SOUND					"tank/tir.wav"
#define TANK_SOUND					"ambience/truck2.wav"
#define CHENILLES_SOUND				"tank/chenilles.wav"
#define CHOC_SOUND					"debris/metal3.wav"
#define ACCELERE_SOUND1				"tank/accelere1.wav"
#define ACCELERE_SOUND2				"tank/accelere2.wav"
#define ACCELERE_SOUND3				"tank/accelere3.wav"
#define DECCELERE_SOUND				"tank/deccelere1.wav"

#define	TANK_EXPLO_SOUND1			"tank/explode.wav"
#define	TANK_EXPLO_SOUND2			"weapons/mortarhit.wav"


#define TOURELLE_MAX_ROT_X			25
#define TOURELLE_MAX_ROT_X2			-10
#define TOURELLE_MAX_ROT_Y			120

#define TANK_SPEED					200
#define TANK_ACCELERATION			5
#define TANK_DECCELERATION			30

#define CAM_DIST_UP					100
#define CAM_DIST_BACK				300

//distances pour l' UTIL_TraceLine ()
#define NEW_ORIGIN					( pev->origin + vecNewVelocity / 10 + gpGlobals->v_up * 2 )

#define	DIST_TOP					60
#define	DIST_FRONT					140
#define	DIST_FRONT_UP				185
#define	DIST_BACK					-150
#define	DIST_BACK_UP				-170
#define	DIST_SIDE					105

#define MOVE_FORWARD				(1<<0)
#define MOVE_BACKWARD				(1<<1)
#define PUSH_FORWARD				(1<<2)
#define PUSH_BACKWARD				(1<<3)

#define TANK_LIFE					1200
#define TANK_RECHARGE				15		// charge du tank_charger par 10e de seconde


//=================================
// classes
//=================================

class CTank;
class CTankCam : public CPointEntity
{
public:
	void Spawn( void );
	void Precache ( void );
	void EXPORT CamThink ( void );

	void SetPlayerTankView ( BOOL setOn );

	virtual int  ObjectCaps( void ) { return FCAP_ACROSS_TRANSITION; };	// traverse les changelevels

	CTank *m_pTankModel;
	float m_skin;
	Vector m_vecTourelleAngle;
	float m_flNextFrameTime;

	int		Save( CSave &save );
	int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];


};

LINK_ENTITY_TO_CLASS( info_tank_camera, CTankCam );

TYPEDESCRIPTION	CTankCam::m_SaveData[] = 
{
	DEFINE_FIELD( CTankCam, m_pTankModel, FIELD_CLASSPTR ),
	DEFINE_FIELD( CTankCam, m_skin, FIELD_FLOAT ),
	DEFINE_FIELD( CTankCam, m_vecTourelleAngle, FIELD_VECTOR ),
	DEFINE_FIELD( CTankCam, m_flNextFrameTime, FIELD_TIME ),
};
IMPLEMENT_SAVERESTORE( CTankCam, CPointEntity );



class CTankBSP : public CBaseEntity/*Monster*/
{
public:
	void	Spawn			( void );
	void	Precache		( void );

	int		BloodColor		( void ) { return DONT_BLEED; };
	int		Classify		( void );
	int		ObjectCaps		( void ) { return CBaseEntity :: ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }

	void	Blocked			( CBaseEntity *pOther );
	void	TraceAttack		( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType);
	int		TakeDamage		( entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType );

	void	EXPORT TankThink	( void );
	void	EXPORT TouchPlayer	( CBaseEntity *pOther );

	CTank *m_pTankModel;

	int		Save( CSave &save );
	int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];


};

LINK_ENTITY_TO_CLASS( vehicle_tank , CTankBSP );

TYPEDESCRIPTION	CTankBSP::m_SaveData[] = 
{
	DEFINE_FIELD( CTankBSP, m_pTankModel, FIELD_CLASSPTR ),
};
IMPLEMENT_SAVERESTORE( CTankBSP, CBaseEntity );



class CTank : public CBaseMonster
{
public:
	
	void Spawn			( void );
	void Precache		( void );
	int  Classify		( void ) { return CLASS_NONE; }
	int  BloodColor		( void ) { return DONT_BLEED; }

	void EXPORT UseTank ( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );

	void EXPORT IdleThink	( void );
	void EXPORT DriveThink	( void );
	void EXPORT StopThink	( void );
	void EXPORT DeadThink	( void );

	void TraceAttack	( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType);
	int	TakeDamage		( entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType );

	void TankDeath		( void );
	void Fire			( int canon );
	void UpdateSound	( void );
	int ModifAngles		( int angle );
	void UpdateCamAngle ( Vector vecNewPosition, float flTime );
	Vector UpdateCam	( void );
	Vector TourelleAngle ( void );

	CTankCam	*m_pCam;
	CBasePlayer *m_pPlayer;
	CTankBSP	*m_pTankBSP;

	int bTankOn;
	int bSetView;
	int bTankDead;

	float	m_flLastAttack1;
	float	m_flNextSound;
	int		bCanon;
	int		m_soundPlaying;

	int		m_iTankmove;

	Vector	m_PlayerAngles;

	Vector	vecCamOrigin ( void ) { Vector origin = pev->origin; origin.z += 150; return origin; };
	Vector	vecCamAim;
	Vector	vecCamTarget;

	float	m_flTempHealth;

	int		Save( CSave &save );
	int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];


private:
	unsigned short m_usAdjustPitch;

};

LINK_ENTITY_TO_CLASS( info_tank_model, CTank );

TYPEDESCRIPTION	CTank::m_SaveData[] = 
{
	DEFINE_FIELD( CTank, m_pCam, FIELD_CLASSPTR ),
	DEFINE_FIELD( CTank, m_pPlayer, FIELD_CLASSPTR ),
	DEFINE_FIELD( CTank, m_pTankBSP, FIELD_CLASSPTR ),

	DEFINE_FIELD( CTank, bTankOn, FIELD_INTEGER ),
	DEFINE_FIELD( CTank, bSetView, FIELD_INTEGER ),
	DEFINE_FIELD( CTank, bTankDead, FIELD_INTEGER ),

	DEFINE_FIELD( CTank, m_iTankmove, FIELD_INTEGER ),

	DEFINE_FIELD( CTank, m_flLastAttack1, FIELD_TIME ),
	DEFINE_FIELD( CTank, m_flNextSound, FIELD_TIME ),

	DEFINE_FIELD( CTank, bCanon, FIELD_INTEGER ),
	DEFINE_FIELD( CTank, m_flTempHealth, FIELD_FLOAT ),

	DEFINE_FIELD( CTank, m_PlayerAngles, FIELD_VECTOR ),
	DEFINE_FIELD( CTank, vecCamAim, FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( CTank, vecCamTarget, FIELD_POSITION_VECTOR ),
};
//IMPLEMENT_SAVERESTORE( CTank, CBaseMonster );







//===================================
// fonctions
//===================================





//=================================
// TankCam
//

void CTankCam::Precache( void )
{
	PRECACHE_MODEL("models/tank_cam.mdl");
}

void CTankCam :: Spawn( void )
{
	Precache();

	SET_MODEL(ENT(pev), "models/tank_cam.mdl");
	Vector zeroVector(0,0,0);
	Vector zeroVector1(0,0,0);
	UTIL_SetSize(pev, zeroVector,zeroVector1 );

	UTIL_SetOrigin( pev, pev->origin );

	pev->movetype = MOVETYPE_NOCLIP;
	pev->solid = SOLID_NOT;

	pev->classname = MAKE_STRING("info_tank_camera");	//necessaire pour le passage a la sauvegarde : getclassptr ne cree pas de pev->classname et donc l entite n est pas prise en compte

	pev->takedamage		= DAMAGE_NO;


	SetThink ( &CTankCam::CamThink );
	pev->nextthink = gpGlobals->time + 1.5;

}

void CTankCam :: CamThink ( void )
{
	pev->nextthink = gpGlobals->time + BSP_NEXTTHINK_TIME;

	// fonction appel
	// le skin des chenilles a besoin d un taux de rafraichissement eleve pour etre realiste

	// skin des chenilles
	// nombre d images : 9

	if ( m_pTankModel == NULL )
		return;


	m_skin += m_pTankModel->pev->velocity.Length() / 20;

	if ( (int)m_skin != m_pTankModel->pev->skin )
	{
		while ( (int)m_skin > 8 )	//de 0 
		{
			m_skin -= 8;
		}
		m_pTankModel->pev->skin = (int)m_skin;
	}


	if ( gpGlobals->time < m_flNextFrameTime )
	{

		float flDifY = m_pTankModel->pev->v_angle.y - m_vecTourelleAngle.y;
		float flDifX = m_pTankModel->pev->v_angle.x - m_vecTourelleAngle.x;

		float flNewAngleY = ( ( gpGlobals->time - ( m_flNextFrameTime - NEXTTHINK_TIME ) ) * flDifY ) / NEXTTHINK_TIME;
		float flNewAngleX = ( ( gpGlobals->time - ( m_flNextFrameTime - NEXTTHINK_TIME ) ) * flDifX ) / NEXTTHINK_TIME;

		m_pTankModel->SetBoneController(1, m_vecTourelleAngle.y + flNewAngleY );
		m_pTankModel->SetBoneController(0, m_vecTourelleAngle.x + flNewAngleX );

	}

//	if ( m_pTankModel->bSetView == TRUE )
//		SetPlayerTankView(TRUE);	// rafraichissement de la positon de la camera pour le client


	if ( m_pTankModel->bTankOn == TRUE )
		SetPlayerTankView(TRUE);	// rafraichissement de la positon de la camera pour le client

}


void	CTankCam :: SetPlayerTankView ( BOOL setOn )
{

	// bug des dommages 

	m_pTankModel->m_flTempHealth = m_pTankModel->m_pTankBSP->pev->health;

	// message client

	MESSAGE_BEGIN( MSG_ONE, gmsgTankView, NULL, m_pTankModel->m_pPlayer->pev );

		WRITE_BYTE	( setOn == TRUE );
		WRITE_COORD ( ENTINDEX ( edict() ) );
		WRITE_LONG ( m_pTankModel->m_pTankBSP->pev->health );

	MESSAGE_END();

}




//==================================
// TankBSP


void CTankBSP :: Precache ( void )
{
	UTIL_PrecacheOther( "info_tank_model" );
}

void CTankBSP :: Spawn( void )
{
	Precache();

	pev->solid			= SOLID_BSP;
	pev->movetype		= MOVETYPE_PUSH;

	SET_MODEL( ENT(pev), STRING(pev->model) );
	
	UTIL_SetSize( pev, pev->mins, pev->maxs );
	UTIL_SetOrigin( pev, pev->origin );

	pev->flags			|= FL_MONSTER;
	pev->takedamage		= DAMAGE_YES;
	pev->rendermode		= kRenderTransTexture;
	pev->renderamt		= 0;
	pev->view_ofs		= Vector ( 0,0,100 );

	pev->health			= TANK_LIFE;

	m_pTankModel = GetClassPtr( (CTank*)NULL );
	UTIL_SetOrigin( m_pTankModel->pev, pev->origin );
	m_pTankModel->pev->angles = pev->angles;
	m_pTankModel->m_pTankBSP = this;
	m_pTankModel->Spawn();


	SetThink ( &CTankBSP::TankThink );
	SetTouch ( &CTankBSP::TouchPlayer );
	pev->nextthink = pev->ltime + 0xFF;

}

int CTankBSP :: Classify( void )
{
	// trouve le model

	edict_t *pent = FIND_ENTITY_BY_CLASSNAME ( NULL, "info_tank_model" );

	if ( pent == NULL )
		return CLASS_NONE;

	CTank *pTank = (CTank*) CBaseEntity::Instance(pent);

	// classe selon l'

	if ( pTank->bTankOn == 1 )
		return CLASS_PLAYER_ALLY;

	return CLASS_NONE;
};



void CTankBSP :: TankThink ( void )
{
	//ne sert strictement 
	pev->nextthink = pev->ltime + 0xFF;
}

void CTankBSP :: Blocked( CBaseEntity *pOther )
{
	pOther->TakeDamage( pev, pev, 0xFF, DMG_CRUSH);
}

void CTankBSP :: TraceAttack ( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType)
{
	CBaseEntity :: TraceAttack(pevAttacker,flDamage,vecDir,ptr,bitsDamageType );

}

int	CTankBSP :: TakeDamage ( entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType )
{
	m_pTankModel->m_flTempHealth = pev->health;

	if ( m_pTankModel->bTankOn == FALSE )
		return 1;

	// le joueur ne le blesse pas - debug
	if ( FClassnameIs(pevInflictor, "player") )
		return 1;

	// que des d
	if ( !(bitsDamageType & DMG_BLAST) )
		return 1;

	// ne se blesse pas lui meme
	if ( pevInflictor == m_pTankModel->pev )
		return 1;

	// d
	if ( pev->health == 0 )
		return 1;

	// mine antichar
	if ( FClassnameIs(pevInflictor,"monster_mine_ac" ) || FClassnameIs(pevAttacker,"monster_mine_ac" ) )
		pev->health = 0;

	pev->health -= flDamage;

	// pas quand le tank est eteint

	if ( m_pTankModel->m_pCam != NULL )
	{
		m_pTankModel->m_pCam->SetPlayerTankView(TRUE);	// rafraichissement de la vie c

		if (pev->health <= 0 )
		{
			pev->health = 0;
			m_pTankModel->TankDeath();
			return 0;
		}
	}

	return 1;
}


void CTankBSP :: TouchPlayer ( CBaseEntity *pOther )
{
	if ( !pOther->IsPlayer() )
		return;

// le joueur n active le tank que s il le touche et qu il  est a peu pres au meme niveau que le tank pour eviter que le joueur sortant sur le toit du tank ne l active a nouveau
	if ( pOther->pev->origin.z > pev->origin.z + 48 )
		return;

//	m_pTankModel->UseTank( pOther, pOther, USE_TOGGLE, 0 );

	edict_t *pent = FIND_ENTITY_BY_CLASSNAME ( NULL, "info_tank_model" );

	if ( pent == NULL )
		return;

	CTank *pTank = (CTank*) CBaseEntity::Instance(pent);

	pTank->UseTank( pOther, pOther, USE_TOGGLE, 0 );

}









//==================================
// CTank
// 




void CTank :: Spawn( void )
{
	Precache( );

//	pev->movetype = MOVETYPE_NOCLIP;
	pev->movetype = MOVETYPE_FLY;
	pev->solid = SOLID_BBOX;
	pev->classname = MAKE_STRING("info_tank_model");	//necessaire pour le passage a la sauvegarde : getclassptr ne cree pas de pev->classname et donc l entite n est pas prise en compte

	SET_MODEL(ENT(pev), "models/tank.mdl");

	Vector zeroVector(0,0,0);
	Vector zeroVector1(0,0,0);
	UTIL_SetSize(pev, zeroVector, zeroVector1 );
	UTIL_SetOrigin( pev, pev->origin );




	pev->flags |= FL_MONSTER;
	pev->takedamage		= DAMAGE_NO;
	pev->sequence		= 0;
	pev->health			= 100;

	m_flTempHealth		= m_pTankBSP->pev->health;

	ResetSequenceInfo( );
	pev->frame = RANDOM_LONG(0,0xFF);
	InitBoneControllers();

	bTankOn = bSetView = bTankDead =0;
	m_flLastAttack1 = m_soundPlaying = 0;

	SetThink( &CTank::IdleThink );
	pev->nextthink = gpGlobals->time + 1;


}


void CTank::Precache( void )
{
	UTIL_PrecacheOther( "info_tank_camera" );

	PRECACHE_MODEL("models/tank.mdl");

	PRECACHE_MODEL( (char *)SPRITE_SMOKE );
	PRECACHE_MODEL( (char *)SPRITE_MUZ );
	PRECACHE_MODEL( (char *)SPRITE_SMOKEBALL );
	PRECACHE_MODEL( (char *)SPRITE_FEU );

	PRECACHE_MODEL( "models/mechgibs.mdl" );

	PRECACHE_SOUND(MITRAILLEUSE_SOUND);
	PRECACHE_SOUND(TIR_SOUND);
	PRECACHE_SOUND(TANK_SOUND);
	PRECACHE_SOUND(CHOC_SOUND);
	PRECACHE_SOUND(CHENILLES_SOUND);
	PRECACHE_SOUND(ACCELERE_SOUND1);
	PRECACHE_SOUND(ACCELERE_SOUND2);
	PRECACHE_SOUND(ACCELERE_SOUND3);
	PRECACHE_SOUND(DECCELERE_SOUND);
	PRECACHE_SOUND(TANK_EXPLO_SOUND1);
	PRECACHE_SOUND(TANK_EXPLO_SOUND2);


	m_usAdjustPitch = PRECACHE_EVENT( 1, "events/train.sc" );

}


void CTank :: TraceAttack ( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType)
{
}

int	CTank :: TakeDamage ( entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType )
{
	return 0;
}


void CTank :: UseTank ( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	edict_t *pentFind;
	CBaseEntity *pFind;

	pentFind = FIND_ENTITY_BY_CLASSNAME( NULL, "info_teleport_destination" );

	if ( pentFind == NULL )
	{
		ALERT ( at_console , "info_tank_model : pas de teleport destination !!!\n" );
		return;
	}

	else
	{
		pFind = CBaseEntity :: Instance ( pentFind );

		Vector vecTeleport = pFind->pev->origin;
		UTIL_SetOrigin( pActivator->pev, vecTeleport );

		m_pPlayer = (CBasePlayer*) pActivator;

		m_pPlayer->m_iDrivingTank	= TRUE;

		m_pPlayer->m_iHideHUD |= HIDEHUD_ALL;


		m_pCam = GetClassPtr( (CTankCam*)NULL );
		UTIL_SetOrigin( m_pCam->pev, vecCamOrigin() );
		m_pCam->pev->angles = TourelleAngle();
		m_pCam->Spawn();
		m_pCam->pev->velocity = ( UpdateCam () - vecCamOrigin() ) /2;
		m_pCam->m_pTankModel = this;


		UpdateCamAngle ( UpdateCam (), 2 );

		m_pCam->SetPlayerTankView ( TRUE );

		SetThink( &CTank::DriveThink );
		pev->nextthink = gpGlobals->time + 2;
	}
}



//===============================================================
//===============================================================
// Fonctions Think


void CTank :: IdleThink ( void )
{
	UpdateSound ();
	pev->nextthink = gpGlobals->time + 0.1;
}


//============================================
//	Le bone controller 0 correspond au d
//	il 
//	le bone 1 est le d
//	et l axe y du joueur



void CTank :: DriveThink ( void )
{
	pev->nextthink = gpGlobals->time + NEXTTHINK_TIME;
	StudioFrameAdvance ( );

	if ( pev->sequence == 1 )
		pev->sequence = 0;

//	ALERT ( at_console, "playerdrivetank : %s\n", m_pPlayer->m_iDrivingTank == TRUE ? "TRUE" : "FALSE" ); 

	// apres le changement de niveau, reinitialisation de la vue

	if ( bSetView == 1 )
	{
		// actualisation de la vie du bsp

		m_pTankBSP->pev->health = m_flTempHealth;

		// r

		m_pCam->SetPlayerTankView ( TRUE );
		bSetView = 0;
	}

	//quitte le tank

	if (m_pPlayer->pev->button & IN_USE)
	{
		pev->velocity = pev->avelocity = m_pTankBSP->pev->velocity = m_pTankBSP->pev->avelocity =Vector (0,0,0);
		m_pTankBSP->pev->origin = pev->origin;
		m_pTankBSP->pev->angles = pev->angles;

		m_pCam->pev->velocity = ( vecCamOrigin() - m_pCam->pev->origin ) /2;
		UpdateCamAngle ( m_pCam->pev->origin, 2 );

		UpdateSound ();

		SetThink( &CTank::StopThink );
		pev->nextthink = gpGlobals->time + 2;
		return;
	}


	float flNextVAngleY = pev->v_angle.y;
	float flNextVAngleX = pev->v_angle.x;
	float flNewAVelocity;
	Vector vecNewVelocity;

	//---------------------------------------------_-_-_ _  _
	//modifications de la direction de la tourelle
			
	if ( bTankOn == 0 )
	{
		bTankOn = 1;
		m_PlayerAngles.x = m_pPlayer->pev->angles.x ;
		m_PlayerAngles.y = m_pPlayer->pev->angles.y ;
	}

	if ( m_pPlayer->pev->angles.y != m_PlayerAngles.y )
	{
		int iSens;
		int iDist = ModifAngles ( m_pPlayer->pev->angles.y ) - ModifAngles ( m_PlayerAngles.y );

		if ( fabs(iDist) > 180 )
		{
			if ( iDist > 0 )
				iDist = iDist - 360;
			else
				iDist = iDist + 360;
		}

		iSens = iDist == fabs(iDist) ? 1 : -1 ;
		iDist = fabs(iDist);


		if ( iDist < TANK_TOURELLE_ROT_SPEED )
			flNextVAngleY += iDist * iSens;

		else
			flNextVAngleY += TANK_TOURELLE_ROT_SPEED * iSens;

		if ( flNextVAngleY > TOURELLE_MAX_ROT_Y )
			flNextVAngleY = TOURELLE_MAX_ROT_Y;

		if ( flNextVAngleY < -TOURELLE_MAX_ROT_Y )
			flNextVAngleY = -TOURELLE_MAX_ROT_Y;

	}

	if ( m_pPlayer->pev->angles.x != m_PlayerAngles.x )
	{
		int iSens;
		int iDist = ModifAngles ( m_pPlayer->pev->angles.x ) - ModifAngles ( m_PlayerAngles.x );

		if ( fabs(iDist) > 180 )
		{
			if ( iDist > 0 )
				iDist = iDist - 360;
			else
				iDist = iDist + 360;
		}

		iSens = iDist == fabs(iDist) ? 1 : -1 ;
		iDist = fabs(iDist);

		if ( iDist < TANK_TOURELLE_ROT_SPEED )
			flNextVAngleX += iDist * iSens;

		else
			flNextVAngleX += TANK_TOURELLE_ROT_SPEED * iSens;

		if ( flNextVAngleX > TOURELLE_MAX_ROT_X )
			flNextVAngleX = TOURELLE_MAX_ROT_X;

		if ( flNextVAngleX < TOURELLE_MAX_ROT_X2 )
			flNextVAngleX = TOURELLE_MAX_ROT_X2;

	}

	m_PlayerAngles.y = m_pPlayer->pev->angles.y;
	m_PlayerAngles.x = m_pPlayer->pev->angles.x;


	//---------------------------------
	// sons d'acceleration du tank

	float flSpeed = pev->velocity.Length();

	if ( m_flNextSound < gpGlobals->time)
	{
		if  ( (m_pPlayer->pev->button & IN_FORWARD) && ((flSpeed==0) || (m_iTankmove & MOVE_BACKWARD)) )
		{
			EMIT_SOUND_DYN(ENT(pev), CHAN_ITEM, ACCELERE_SOUND1, 1 , ATTN_NONE, 0, 100 );
			m_flNextSound = gpGlobals->time + 2.5;
		}
		else if  ( (m_pPlayer->pev->button & IN_BACK) && (m_iTankmove & MOVE_FORWARD) )
		{
			EMIT_SOUND_DYN(ENT(pev), CHAN_ITEM, DECCELERE_SOUND, 1 , ATTN_NONE, 0, 100 );
			m_flNextSound = gpGlobals->time + 2.5;
		}
		else if  ( (m_pPlayer->pev->button & IN_FORWARD) && (m_iTankmove & MOVE_FORWARD) && !(m_iTankmove & PUSH_FORWARD))
		{
			if ( RANDOM_LONG ( 0,1 ) )
				EMIT_SOUND_DYN(ENT(pev), CHAN_ITEM, ACCELERE_SOUND2, 1 , ATTN_NONE, 0, 100 );
			else
				EMIT_SOUND_DYN(ENT(pev), CHAN_ITEM, ACCELERE_SOUND3, 1 , ATTN_NONE, 0, 100 );
			m_flNextSound = gpGlobals->time + 2.5;
		}
	}


	//-------------------------------
	//modification de la vitesse du tank


	UTIL_MakeVectors( pev->angles );

	int iSens = UTIL_AngleDiff( UTIL_VecToAngles ( pev->velocity ).y,  UTIL_VecToAngles	( gpGlobals->v_forward ).y );

	if ( flSpeed == 0 )
		iSens = 0;
	else if ( iSens < -45 || iSens > 45 )
		iSens = -1;
	else 
		iSens = 1;

	if ( m_pPlayer->pev->button & IN_FORWARD)
	{
		m_iTankmove |= PUSH_FORWARD;
		m_iTankmove &= ~PUSH_BACKWARD;		

		if ( iSens == -1 )
		{
			if ( flSpeed > TANK_DECCELERATION * 2 )
				vecNewVelocity = gpGlobals->v_forward * - ( flSpeed - TANK_DECCELERATION );
			else
				vecNewVelocity = Vector ( 0,0,0 );
		}

		else if ( flSpeed < 250 )
			vecNewVelocity = gpGlobals->v_forward * ( flSpeed + TANK_ACCELERATION );

		else
			vecNewVelocity = gpGlobals->v_forward * 250;
	}


	else if ( m_pPlayer->pev->button & IN_BACK)
	{

		m_iTankmove |= PUSH_BACKWARD;
		m_iTankmove &= ~PUSH_FORWARD;		

		if ( iSens == 1 )
		{
			if ( flSpeed > TANK_DECCELERATION * 2 )
				vecNewVelocity = gpGlobals->v_forward * ( flSpeed - TANK_DECCELERATION );
			else
				vecNewVelocity = Vector ( 0,0,0 );
		}

		else if ( flSpeed < 150 )
			vecNewVelocity = gpGlobals->v_forward * - ( flSpeed + TANK_ACCELERATION );
		else
			vecNewVelocity = gpGlobals->v_forward * -150;
	}

	
	else
	{
		if ( flSpeed > 5 )
			vecNewVelocity = gpGlobals->v_forward * ( flSpeed - 1 ) * iSens;
		else
			vecNewVelocity = gpGlobals->v_forward * flSpeed * iSens;


		m_iTankmove &= ~PUSH_BACKWARD;
		m_iTankmove &= ~PUSH_FORWARD;
	}


	if ( iSens == 1)
	{
		m_iTankmove |= MOVE_FORWARD;
		m_iTankmove &= ~MOVE_BACKWARD;
	}
	else
	{
		m_iTankmove |= MOVE_BACKWARD;
		m_iTankmove &= ~MOVE_FORWARD;
	}



	//modification de la direction du tank

	if ( m_pPlayer->pev->button & IN_MOVELEFT )
		flNewAVelocity = TANK_ROT_SPEED;

	else if ( m_pPlayer->pev->button & IN_MOVERIGHT )
		flNewAVelocity = -TANK_ROT_SPEED;

	else
		flNewAVelocity = 0;


	// test de la position envisag

	UTIL_MakeVectors ( pev->angles + Vector ( 0, flNewAVelocity / 10 , 0) );

	TraceResult	tr [4]/*1,tr2,tr3,tr4*/;
	Vector vecFrontLeft, vecFrontRight, vecBackLeft, vecBackRight;

	vecFrontLeft =	NEW_ORIGIN + gpGlobals->v_forward * DIST_FRONT_UP + gpGlobals->v_right * -DIST_SIDE + gpGlobals->v_up * DIST_TOP;
	vecFrontRight = NEW_ORIGIN + gpGlobals->v_forward * DIST_FRONT_UP + gpGlobals->v_right * DIST_SIDE + gpGlobals->v_up * DIST_TOP;
	vecBackLeft =	NEW_ORIGIN + gpGlobals->v_forward * DIST_BACK_UP + gpGlobals->v_right * -DIST_SIDE + gpGlobals->v_up * DIST_TOP;
	vecBackRight =	NEW_ORIGIN + gpGlobals->v_forward * DIST_BACK_UP + gpGlobals->v_right * DIST_SIDE + gpGlobals->v_up * DIST_TOP;
					
	UTIL_TraceLine (vecFrontLeft, vecFrontRight,ignore_monsters, ENT(m_pTankBSP->pev), &tr[0]);
	UTIL_TraceLine (vecFrontRight, vecBackRight,ignore_monsters, ENT(m_pTankBSP->pev), &tr[1]);
	UTIL_TraceLine (vecBackRight, vecBackLeft,	ignore_monsters, ENT(m_pTankBSP->pev), &tr[2]);
	UTIL_TraceLine (vecBackLeft, vecFrontLeft,	ignore_monsters, ENT(m_pTankBSP->pev), &tr[3]);


	//pas de collision - application de la nouvelle position

	if ( tr[0].vecEndPos == vecFrontRight && tr[1].vecEndPos == vecBackRight && tr[2].vecEndPos == vecBackLeft && tr[3].vecEndPos == vecFrontLeft )
	{
		StudioFrameAdvance ( 0.1 );

		pev->velocity = vecNewVelocity;
		pev->avelocity = Vector ( 0, flNewAVelocity, 0 );

		m_pCam->m_vecTourelleAngle = pev->v_angle;
		m_pCam->m_flNextFrameTime = pev->nextthink;

		pev->v_angle.y = flNextVAngleY;
		pev->v_angle.x = flNextVAngleX;


		m_pTankBSP->pev->velocity = (( pev->origin + vecNewVelocity * 10 ) - m_pTankBSP->pev-> origin ) / 10 ;
		m_pTankBSP->pev->avelocity = (( pev->angles + Vector ( 0, flNewAVelocity * 10, 0 ) - m_pTankBSP->pev->angles )) / 10;
		// pour combler la diff

	}

	//collision - arret du tank

	else
	{
		pev->velocity = pev->avelocity = Vector (0,0,0);
		m_pTankBSP->pev->velocity = ( pev->origin - m_pTankBSP->pev-> origin ) / 10 ;
		m_pTankBSP->pev->avelocity = ( pev->angles - m_pTankBSP->pev->angles ) / 10;

		if ( flSpeed > 50 )	// choc violent
		{
			EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, CHOC_SOUND, 0.9, ATTN_NORM, 0, 60 );

		}
	}


	// application des dommages

	vecFrontLeft = vecFrontLeft + Vector ( 0, 0, 10 - DIST_TOP );
	vecFrontRight = vecFrontRight + Vector ( 0, 0, 10 - DIST_TOP );
	vecBackRight = vecBackRight + Vector ( 0, 0, 10 - DIST_TOP );
	vecBackLeft = vecBackLeft + Vector ( 0, 0, 10 - DIST_TOP );

	UTIL_TraceLine (vecFrontLeft, vecFrontRight,dont_ignore_monsters, ENT(m_pTankBSP->pev), &tr[0]);
	UTIL_TraceLine (vecFrontRight, vecBackRight,dont_ignore_monsters, ENT(m_pTankBSP->pev), &tr[1]);
	UTIL_TraceLine (vecBackRight, vecBackLeft,	dont_ignore_monsters, ENT(m_pTankBSP->pev), &tr[2]);
	UTIL_TraceLine (vecBackLeft, vecFrontLeft,	dont_ignore_monsters, ENT(m_pTankBSP->pev), &tr[3]);

	CBaseEntity *pEntity = NULL;

	for ( int i = 0 ; i < 4 ; i ++ )
	{
		if ( tr [ i ].pHit != NULL )
		{
			pEntity =  CBaseEntity :: Instance ( tr [ i ].pHit );

			if ( pEntity != NULL && pEntity->pev->takedamage  )
			{
				float fDamage;

				if ( FClassnameIs ( tr[i].pHit, "func_breakable" ) )
				{
					fDamage =  pEntity->pev->health;
				}
				else
				{
					fDamage = pev->velocity.Length() * 1.5 + 20;
				}

				pEntity->TakeDamage ( pev, pev , fDamage , DMG_CRUSH );
			}
		}
	}

	//rectification de la position de la camera

	vecCamAim = UpdateCam();

	if ( m_pCam->pev->origin != vecCamAim )
		m_pCam->pev->velocity = ( vecCamAim - m_pCam->pev->origin ) * 10;

	UpdateCamAngle ( vecCamAim, NEXTTHINK_TIME );



	//tir de la tourelle

	if ( ( m_pPlayer->pev->button & IN_ATTACK ) && ( gpGlobals->time > m_flLastAttack1 + TANK_REFIRE_DELAY ) )
	{
		Fire ( bCanon );
		bCanon = bCanon == TRUE ? FALSE : TRUE;

		EMIT_SOUND(ENT(pev), CHAN_AUTO, TIR_SOUND, 1, ATTN_NORM);
		m_flLastAttack1 = gpGlobals->time;

		m_pCam->pev->avelocity.x -= 45;

	}

	//tir de la mitrailleuse

	if ( m_pPlayer->pev->button & IN_ATTACK2 )
	{
		Vector posGun, dirGun;
		Vector zeroVector(0,0,0);
		GetAttachment( 3, posGun, zeroVector );
		UTIL_MakeVectorsPrivate( TourelleAngle(), dirGun, NULL, NULL );
		FireBullets( 1, posGun, dirGun, VECTOR_CONE_5DEGREES, 8192, BULLET_MONSTER_12MM );

		EMIT_SOUND(ENT(pev), CHAN_WEAPON, MITRAILLEUSE_SOUND, 1, ATTN_NORM);

		if ( !FStrEq(STRING(gpGlobals->mapname), "l3m10") && !FStrEq(STRING(gpGlobals->mapname), "l3m12")  && !FStrEq(STRING(gpGlobals->mapname), "l3m14")  )
		{			
			CSprite *pSprite = CSprite::SpriteCreate( SPRITE_MUZ, posGun, TRUE );
			pSprite->AnimateAndDie( 15 );
			pSprite->SetTransparency( kRenderTransAdd, 255, 255, 255, 255, kRenderFxNoDissipation );
			pSprite->SetAttachment( edict(), 4 );
			pSprite->SetScale( SPRITE_MUZ_SCALE );
		}


	}



	//sond du tank

	UpdateSound ();

	CSoundEnt::InsertSound ( bits_SOUND_DANGER, pev->origin + pev->velocity * 2.5, 150, NEXTTHINK_TIME );
	CSoundEnt::InsertSound ( bits_SOUND_PLAYER, pev->origin, 2000, 0.5 );



}


void CTank :: StopThink ( void )
{

	m_pCam->SetPlayerTankView ( FALSE );

	m_pCam->SetThink ( &CBaseEntity::SUB_Remove );
	m_pCam->pev->nextthink = gpGlobals->time + 0.1;

	m_pCam = NULL;

//	if ( m_pPlayer->m_pActiveItem )
//		m_pPlayer->m_pActiveItem->Deploy();

	m_pPlayer->m_iDrivingTank	= FALSE;


	m_pPlayer->m_iHideHUD &= ~HIDEHUD_ALL;

	UTIL_SetOrigin( m_pPlayer->pev, Vector ( vecCamOrigin().x, vecCamOrigin().y, vecCamOrigin().z + 30 ) );

	bTankOn = 0;
	m_pPlayer = NULL;

	pev->nextthink = gpGlobals->time + 0.1;
	SetThink( &CTank::IdleThink );

}


void CTank :: DeadThink ( void )
{
	pev->nextthink = gpGlobals->time + 0.1;

	pev->sequence = 0;


	// camera tournante

	pev->v_angle.y += 3;

	//rectification de la position de la camera

	vecCamAim = UpdateCam();

	if ( m_pCam->pev->origin != vecCamAim )
		m_pCam->pev->velocity = ( vecCamAim - m_pCam->pev->origin ) * 10;

	UpdateCamAngle ( vecCamAim, NEXTTHINK_TIME );


	// sprites de feu

	for ( int i=0; i<4; i++ )
	{
		CSprite *pSpr = CSprite::SpriteCreate ( SPRITE_FEU, Vector(pev->origin.x,pev->origin.y,pev->origin.z + 100), TRUE );
		pSpr->SetScale ( SPRITE_FEU_SCALE );
		pSpr->AnimateAndDie ( RANDOM_FLOAT(20,25) );
		pSpr->SetTransparency ( kRenderTransAdd, 255, 255, 255, 120, kRenderFxNone );

		pSpr->pev->velocity = Vector ( RANDOM_FLOAT(-50,50),RANDOM_FLOAT(-50,50),140/*RANDOM_FLOAT(130,150)*/ );
	}

	for ( int i=0; i<1; i++ )
	{
		CSprite *pSpr = CSprite::SpriteCreate ( SPRITE_SMOKEBALL, Vector(pev->origin.x,pev->origin.y,pev->origin.z + 100), TRUE );
		pSpr->SetScale ( SPRITE_SMOKEBALL_SCALE );
		pSpr->AnimateAndDie ( RANDOM_FLOAT(3,4) );
		pSpr->SetTransparency ( kRenderTransAlpha, 255, 255, 255, 200, kRenderFxNone );
		pSpr->pev->velocity = Vector ( RANDOM_FLOAT(-50,50),RANDOM_FLOAT(-50,50),RANDOM_FLOAT(130,150) );
	}




}


void CTank :: TankDeath ( void )
{
	bTankDead = 1;
	pev->sequence = 2;/*LookupSequence( "die" );*/
	ResetSequenceInfo ();

	m_pPlayer->TakeDamage ( pev,pev,(float)999, DMG_CRUSH );	// mouru

	pev->velocity = pev->avelocity = m_pTankBSP->pev->velocity = m_pTankBSP->pev->avelocity =Vector (0,0,0);
	m_pTankBSP->pev->origin = pev->origin;
	m_pTankBSP->pev->angles = pev->angles;

	m_pCam->pev->velocity = m_pCam->pev->avelocity = Vector (0,0,0);

	UpdateSound ();

	SetThink ( &CTank::DeadThink );
	pev->nextthink = gpGlobals->time + 29 / 21.0;

	// maman, c'est quoi qu'a fait boum ?

	EMIT_SOUND(ENT(pev), CHAN_AUTO, TANK_EXPLO_SOUND1, 1, ATTN_NORM);	
	EMIT_SOUND(ENT(pev), CHAN_WEAPON, TANK_EXPLO_SOUND2, 1, ATTN_NORM);	


	// sprites de feu - explosion

	for ( int i=0; i<20; i++ )
	{
		CSprite *pSpr = CSprite::SpriteCreate ( SPRITE_FEU, Vector(pev->origin.x,pev->origin.y,pev->origin.z + 50), TRUE );
		pSpr->SetScale ( SPRITE_FEU_SCALE*2 );
		pSpr->AnimateAndDie ( RANDOM_FLOAT(20,22) );
		pSpr->SetTransparency ( kRenderTransAdd, 255, 255, 255, 120, kRenderFxNone );

		pSpr->pev->velocity = Vector ( RANDOM_FLOAT(-150,150),RANDOM_FLOAT(-150,150),100/*RANDOM_FLOAT(130,150)*/ );
	}
	// sprites de feu en colonne

	for ( int i=0; i<6; i++ )
	{
		CSprite *pSpr = CSprite::SpriteCreate ( SPRITE_FEU, Vector(pev->origin.x,pev->origin.y,pev->origin.z + 100), TRUE );
		pSpr->SetScale ( SPRITE_FEU_SCALE );
		pSpr->AnimateAndDie ( RANDOM_FLOAT(20,25) );
		pSpr->SetTransparency ( kRenderTransAdd, 255, 255, 255, 120, kRenderFxNone );

		pSpr->pev->velocity = Vector ( RANDOM_FLOAT(-50,50),RANDOM_FLOAT(-50,50),140/*RANDOM_FLOAT(130,150)*/ );
	}

	for ( int i=0; i<10; i++ )
	{
		CSprite *pSpr = CSprite::SpriteCreate ( SPRITE_SMOKEBALL, Vector(pev->origin.x,pev->origin.y,pev->origin.z + 100), TRUE );
		pSpr->SetScale ( SPRITE_SMOKEBALL_SCALE );
		pSpr->AnimateAndDie ( RANDOM_FLOAT(2,3) );
		pSpr->SetTransparency ( kRenderTransAlpha, 255, 255, 255, 255, kRenderFxNone );
		pSpr->pev->velocity = Vector ( RANDOM_FLOAT(-50,50),RANDOM_FLOAT(-50,50),RANDOM_FLOAT(50,50) );
	}


	// gibs

	for ( int i = 0; i<20; i++ )
	{
		CGib *pGib = GetClassPtr( (CGib *)NULL );
		pGib->Spawn( "models/mechgibs.mdl" );
		pGib->m_bloodColor = DONT_BLEED;
		pGib->pev->body = RANDOM_LONG (1,5);

		pGib->pev->origin = pev->origin + Vector ( 0, 0, 250 );
		pGib->pev->velocity = Vector ( RANDOM_FLOAT(-200,200),RANDOM_FLOAT(-200,200),RANDOM_FLOAT(0,400));
		pGib->pev->avelocity = Vector ( RANDOM_FLOAT(-1000,1000), RANDOM_FLOAT(-1000,1000), RANDOM_FLOAT(-1000,1000) );

		pGib->pev->solid = SOLID_NOT;
		pGib->SetThink(&CBaseEntity::SUB_Remove);
		pGib->pev->nextthink = gpGlobals->time + 1;
	}

	// 

	for ( int i = 0; i < 10; i++ )
	{
		Create( "spark_shower", pev->origin, Vector (0,0,1), NULL );
	}



}







int CTank :: ModifAngles ( int angle )
{
	if ( angle < 0 )
		return 360 - fabs( angle );
	else
		return angle;
}


void CTank :: UpdateCamAngle ( Vector vecNewPosition, float flTime )
{
	Vector vecNewAngle;
	Vector zeroVector(0,0,0);
	GetAttachment( 2, vecCamTarget, zeroVector );

	vecNewAngle = UTIL_VecToAngles( vecCamTarget - vecNewPosition );
	vecNewAngle.x = -vecNewAngle.x;

	float distX = UTIL_AngleDistance( m_pCam->pev->angles.x, vecNewAngle.x );
	m_pCam->pev->avelocity.x = -distX / flTime;
	
	float distY = UTIL_AngleDistance( m_pCam->pev->angles.y, vecNewAngle.y );
	m_pCam->pev->avelocity.y = -distY / flTime;
}


Vector CTank :: UpdateCam ( void )
{
	TraceResult tr;
	int up = CAM_DIST_UP;
	int back = CAM_DIST_BACK;
	Vector Aim;
	UTIL_MakeVectors( TourelleAngle() );

	do
	{
		Aim = vecCamOrigin() +  gpGlobals->v_up * up  -  gpGlobals->v_forward * back;
		up -= CAM_DIST_UP / 20;
		back -= CAM_DIST_BACK / 20;

		UTIL_TraceLine( vecCamOrigin(), Aim, ignore_monsters, edict(), &tr );

	}
	while ( tr.vecEndPos != Aim /*|| CAM_DIST_UP == 0*/ );

	return Aim;
}


void CTank :: Fire ( int canon )
{
	Vector vecGun;
	Vector zeroVector(0,0,0);
	GetAttachment( canon, vecGun, zeroVector );

	if ( !FStrEq(STRING(gpGlobals->mapname), "l3m10") && !FStrEq(STRING(gpGlobals->mapname), "l3m12")  && !FStrEq(STRING(gpGlobals->mapname), "l3m14")  )
	{			
		CSprite *pSprite = CSprite::SpriteCreate( SPRITE_SMOKE, vecGun, TRUE );
		pSprite->AnimateAndDie( 15 );
		pSprite->SetTransparency( kRenderTransAdd, 255, 255, 255, 255, kRenderFxNoDissipation );
		pSprite->SetAttachment( edict(), canon+1 );
		pSprite->SetScale( SPRITE_SMOKE_SCALE );
	}


	TraceResult tr;

	UTIL_MakeVectors ( TourelleAngle() );
	UTIL_TraceLine( vecGun, vecGun + gpGlobals->v_forward * 8192, dont_ignore_monsters, edict(), &tr );

	// pas de dommages - la fonction standart donne un rayon 2.5 fois les dommages
	// 250 * 2.5 = 625	- bcp trop grand

	ExplosionCreate( tr.vecEndPos, pev->angles, NULL/*edict()*/, 250, FALSE );

	// on applique nous-m
	::RadiusDamage( tr.vecEndPos, pev, pev, 300, 300, CLASS_NONE, DMG_BLAST );
	
	//effet de fum
	EnvSmokeCreate( tr.vecEndPos, 4, 10, 2, 0 );

/*	// sprites de feu

	for ( int i=0; i<4; i++ )
	{
		for ( int j=0; j<3; j++ )
		{
			CSprite *pSpr = CSprite::SpriteCreate ( SPRITE_FEU, tr.vecEndPos + Vector(0,0,50), TRUE );
			pSpr->SetTransparency ( kRenderTransAdd, 255, 255, 255, 180, kRenderFxNone );

			pSpr->pev->scale		= (float)((float)SPRITE_FEU_SCALE*2*(1/(i+1)));
			pSpr->pev->framerate	= RANDOM_FLOAT(18,24);
			pSpr->pev->velocity		= Vector ( RANDOM_FLOAT(-50,50)*(3-i)/3,RANDOM_FLOAT(-50,50)*(3-i)/3, 50*(i));
			pSpr->pev->spawnflags  |= SF_SPRITE_ONCE;
			pSpr->TurnOn();
		}
	}
*/
/*	for ( int i=0; i<1; i++ )
	{
		CSprite *pSpr = CSprite::SpriteCreate ( SPRITE_SMOKEBALL, Vector(pev->origin.x,pev->origin.y,pev->origin.z + 100), TRUE );
		pSpr->SetScale ( SPRITE_SMOKEBALL_SCALE );
		pSpr->AnimateAndDie ( RANDOM_FLOAT(3,4) );
		pSpr->SetTransparency ( kRenderTransAlpha, 255, 255, 255, 200, kRenderFxNone );
		pSpr->pev->velocity = Vector ( RANDOM_FLOAT(-50,50),RANDOM_FLOAT(-50,50),RANDOM_FLOAT(130,150) );
	}
*/
	//breakable sp

	if ( FClassnameIs (tr.pHit, "func_breakable") && VARS(tr.pHit)->spawnflags & SF_BREAK_TANKTOUCH )
	{
		CBreakable *pBreak = (CBreakable*) CBaseEntity::Instance(tr.pHit);

		if ( pBreak->CheckTankPrev() )
		{
			pBreak->pev->health = 0;
			pBreak->Killed( pev, GIB_NORMAL );
			pBreak->Die();
		}
	}
}


Vector CTank :: TourelleAngle ( void )
{
	UTIL_MakeVectors(pev->angles );

	Vector angle = 	UTIL_VecToAngles( gpGlobals->v_forward );

	angle.x += pev->v_angle.x;
	angle.y += pev->v_angle.y;
	
	angle.y = UTIL_AngleMod( angle.y );
	angle.x = -angle.x;

	return angle;
}

void CTank :: UpdateSound ( void )
{

	if ( m_soundPlaying == 0 )
	{
		EMIT_SOUND_DYN(ENT(pev), CHAN_STATIC, TANK_SOUND, 1.0, ATTN_NORM, 0, 100 );
		EMIT_SOUND_DYN(ENT(pev), CHAN_STREAM, CHENILLES_SOUND, 0.9, ATTN_NORM, 0, 100 );
		m_soundPlaying = 1;

	}
	else
	{
		//moteur
		int pitch = (int)( pev->velocity.Length() * 170 / 255 ) + 80;
		pitch = pitch > 255 ? 255 : pitch ;
		EMIT_SOUND_DYN(ENT(pev), CHAN_BODY, TANK_SOUND, 1.0, ATTN_NORM, SND_CHANGE_PITCH | SND_CHANGE_VOL, pitch );

		//chenilles
		int volume = ((int)(pev->velocity.Length()) - 30 ) / 80;
		volume = volume < 0 ? 0 : volume;
		volume = volume > 1 ? 1 : volume;
		EMIT_SOUND_DYN(ENT(pev), CHAN_STREAM, CHENILLES_SOUND, volume, ATTN_NORM, SND_CHANGE_PITCH | SND_CHANGE_VOL, 100 );

	}
}




int CTank::Save( CSave &save )
{
	if ( !CBaseMonster::Save(save) )
		return 0;

	return save.WriteFields( "CTank", this, m_SaveData, ARRAYSIZE(m_SaveData) );
}

int CTank::Restore( CRestore &restore )		// s execute lors du chargement rapide
{
	if ( !CBaseMonster::Restore(restore) )
		return 0;

	int status = restore.ReadFields( "CTank", this, m_SaveData, ARRAYSIZE(m_SaveData) );

	//-----------------------

	ALERT ( at_console,"TANK RESTORE -----------------\n" );

	// restoration de la camera
	bSetView = 1;


	// restoration du tank

	CBaseEntity *pFind = UTIL_FindEntityByClassname( NULL, "info_tank_model" );

	while ( pFind != NULL && pFind == this )
		pFind = UTIL_FindEntityByClassname( pFind, "info_tank_model" );


	// chargement
	if ( pFind == NULL )
	{
		ALERT ( at_console, "TANK RESTORE : il n'y a qu'un tankmodel : simple chargement\n" );
		return status;
	}

	// changement de niveau
	ALERT ( at_console, "TANK RESTORE : autre tankmodel : changement de niveau\n" );

	CTank *pModelFound = (CTank*)pFind;

	m_pTankBSP = pModelFound->m_pTankBSP;					// changement de tankbsp
	m_pTankBSP->m_pTankModel = this;

	pModelFound->SetThink ( &CBaseEntity::SUB_Remove );					// destruction du tankmodel inutile
	pModelFound->pev->nextthink = gpGlobals->time + 0.1;
	pModelFound->pev->rendermode = kRenderTransTexture;
	pModelFound->pev->renderamt = 0;

	m_pTankBSP->pev->angles = pev->angles;
	m_pTankBSP->pev->origin = pev->origin;


	// tite boite

	edict_t *pentTrouve;
	CBaseEntity *pTrouve;

	CBasePlayer *pPlayer = (CBasePlayer*) UTIL_FindEntityByClassname ( NULL, "player" );

	pentTrouve = FIND_ENTITY_BY_CLASSNAME( NULL, "info_teleport_destination" );

	if ( pentTrouve == NULL )
	{
		ALERT ( at_console , "info_tank_model : pas de teleport destination !!!\n" );
		return status;
	}

	else
	{
		pTrouve = CBaseEntity :: Instance ( pentTrouve );

		Vector vecTeleport = pTrouve->pev->origin;
		UTIL_SetOrigin( pPlayer->pev, vecTeleport );
	}


	
	return status;
}



//----------------------------------------------------------
// mine anti char

class CMineAC : public CBaseEntity
{
public:
	void Spawn( void );
	void Precache ( void );
	void EXPORT MineThink ( void );
};

LINK_ENTITY_TO_CLASS( monster_mine_ac , CMineAC );

void CMineAC::Precache( void )
{
	PRECACHE_MODEL("models/dxmine.mdl");
}

void CMineAC :: Spawn( void )
{
	Precache();

	SET_MODEL(ENT(pev), "models/dxmine.mdl");
	UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);
	UTIL_SetOrigin( pev, pev->origin );

	pev->movetype = MOVETYPE_NOCLIP;
	pev->solid = SOLID_NOT;

	SetThink ( &CMineAC::MineThink );
	pev->nextthink = gpGlobals->time + 0.1;

}

void CMineAC :: MineThink ( void )
{
	pev->nextthink = gpGlobals->time + 0.1;

	TraceResult tr;
	UTIL_TraceHull ( pev->origin, pev->origin + Vector (0,0,20), dont_ignore_monsters, head_hull, edict(), &tr );

	if ( tr.pHit == NULL )
		return;

	if ( (tr.pHit->v.flags & FL_MONSTER) || (tr.pHit->v.flags & FL_CLIENT) )
	{
		Vector zeroVector(0,0,0);
		ExplosionCreate ( pev->origin + Vector ( 0,0,30 ), zeroVector, edict(), 200, TRUE );
		SetThink ( &CBaseEntity::SUB_Remove );
	}
}




//-------------------------------------------------------
// Tank Charger


class CTankCharger : public CBaseEntity
{
public:
	void Spawn( void );
	void EXPORT ChargerThink ( void );
};

LINK_ENTITY_TO_CLASS( func_tank_charger , CTankCharger );


void CTankCharger :: Spawn( void )
{
	pev->movetype = MOVETYPE_NONE;
	pev->solid = SOLID_TRIGGER;
	SET_MODEL(ENT(pev), STRING(pev->model));    // set size and link into world

	pev->effects |= EF_NODRAW;

	SetThink ( &CTankCharger::ChargerThink );
	pev->nextthink = gpGlobals->time + 0.1;

}

void CTankCharger :: ChargerThink ( void )
{
	pev->nextthink = gpGlobals->time + 0.1;

	TraceResult tr;
	UTIL_TraceHull ( (pev->mins+pev->maxs)*0.5, (pev->mins+pev->maxs)*0.5 + Vector(0,0,100), dont_ignore_monsters, head_hull, edict(), &tr );

	if ( tr.pHit == NULL )
		return;

	if ( (tr.pHit->v.flags & FL_MONSTER) && FClassnameIs ( tr.pHit, "vehicle_tank") )
	{
		CTankBSP *pBsp = (CTankBSP*)CBaseEntity::Instance(tr.pHit);

		pBsp->pev->health = Q_min ( pBsp->pev->health + TANK_RECHARGE, TANK_LIFE );

		// rafraichissement de l'affichage
		if ( pBsp->m_pTankModel->bTankOn == TRUE )
			pBsp->m_pTankModel->m_pCam->SetPlayerTankView ( TRUE );
	}
}




