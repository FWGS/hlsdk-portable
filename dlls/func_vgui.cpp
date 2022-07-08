/********************************************************************
*																	*
*		func_vgui.cpp												*
*																	*
*		par Julien													*
*																	*
********************************************************************/

// code du func_vgui
// et du monster_camera


//===========================
//===========================
// - include

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "effects.h"
#include "../cl_dll/crutches.h" //Needed for cameras.

extern int gmsgKeypad;
extern int gmsgConveyor;


//===========================
//===========================
// - definition de la classe
//

class CFuncVgui : public CBaseToggle
{
public:
	void Spawn( void );
	void Use ( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void EXPORT VguiThink ( void );


	void KeyValue( KeyValueData *pkvd );

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	CBasePlayer *m_pPlayer;
	CBaseEntity *m_pCaller;
	int m_iparam1;
	int m_iparam2;
	int m_iparam3;

};


LINK_ENTITY_TO_CLASS( func_vgui, CFuncVgui );


//=============================
// - savestore

TYPEDESCRIPTION	CFuncVgui::m_SaveData[] = 
{
	DEFINE_FIELD( CFuncVgui, m_pPlayer, FIELD_CLASSPTR ),
	DEFINE_FIELD( CFuncVgui, m_pCaller, FIELD_CLASSPTR ),
	DEFINE_FIELD( CFuncVgui, m_iparam1, FIELD_INTEGER ),
	DEFINE_FIELD( CFuncVgui, m_iparam2, FIELD_INTEGER ),
	DEFINE_FIELD( CFuncVgui, m_iparam3, FIELD_INTEGER ),
};

IMPLEMENT_SAVERESTORE( CFuncVgui, CBaseToggle );


//=============================
//=============================
// - fonctions


void CFuncVgui :: Spawn ( void )
{
	pev->angles		= g_vecZero;
	pev->movetype		= MOVETYPE_PUSH;
	pev->solid			= SOLID_BSP;
	
	UTIL_SetOrigin( pev, pev->origin );
	SET_MODEL( ENT(pev), STRING(pev->model) );

	pev->takedamage		= DAMAGE_NO;

	pev->rendermode		= kRenderTransTexture;
	pev->renderamt		= 0;	//50;

	pev->flags |= FL_WORLDBRUSH;

//	SetUse ( &CFuncVgui::UseVgui );
}


void CFuncVgui :: KeyValue( KeyValueData *pkvd )
{
	if (FStrEq(pkvd->szKeyName, "vgui"))
	{
		m_iparam1 = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "iparam2") )
	{
		m_iparam2 = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "iparam3") )
	{
		m_iparam3 = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
	{
		CBaseEntity :: KeyValue( pkvd );
	}
}



void CFuncVgui :: Use ( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	//que le joueur

	if ( !pActivator->IsPlayer() )
		return;

	m_pPlayer = (CBasePlayer*)pActivator;
	m_pCaller = pCaller;

	m_pPlayer->ShowVGUIordiMenu( m_iparam1, m_iparam2, m_iparam3 );
	SetThink ( &CFuncVgui::VguiThink );
	pev->nextthink = pev->nextthink = pev->ltime + 0.5;

}


void CFuncVgui :: VguiThink ( void )
{
	pev->nextthink = pev->ltime + 0.5;


	if ( m_pPlayer == NULL || m_pCaller == NULL || ( m_pPlayer->Center() - m_pCaller->pev->origin ).Length() > 90 )
	{
		SetThink ( NULL );
		m_pPlayer->ShowVGUIordiMenu( m_iparam1, m_iparam2, 0 );
	}
}






//=========================================
//
//	monster_camera
//
//=========================================


//=============================
// - savestore


//===========================
//===========================
// - definition de la classe
//

class CMonsterCamera : public CBaseAnimating
{
public:
	void Precache ( void );
	void Spawn( void );
	void Use ( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void KeyValue( KeyValueData *pkvd );

	void EXPORT CamAngles ( void );


	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];
/*#ifndef DONTSAVECAMERASFIX_INVASION_DLL
#endif*/

	CSprite *pSprite;
	int m_iAlarmState;
};


LINK_ENTITY_TO_CLASS( monster_camera, CMonsterCamera );


TYPEDESCRIPTION	CMonsterCamera::m_SaveData[] = 
{
#ifndef DONTSAVECAMERASFIX_INVASION_DLL
	DEFINE_FIELD( CMonsterCamera, pSprite, FIELD_CLASSPTR ),
#endif
	DEFINE_FIELD( CMonsterCamera, m_iAlarmState, FIELD_INTEGER ),
};

IMPLEMENT_SAVERESTORE( CMonsterCamera, CBaseAnimating );



//=============================
//=============================
// - fonctions

void CMonsterCamera :: Precache()
{
	PRECACHE_MODEL("models/camera.mdl");
	PRECACHE_MODEL("sprites/yelflare1.spr");
	PRECACHE_SOUND("turret/tu_spinup.wav");
}	


void CMonsterCamera :: Spawn ( void )
{
	Precache();

	pev->movetype		= MOVETYPE_NONE;
	pev->solid			= SOLID_NOT;
	
	UTIL_SetOrigin( pev, pev->origin );
	SET_MODEL( ENT(pev), "models/camera.mdl" );

	pev->takedamage		= DAMAGE_NO;

	InitBoneControllers();

	pSprite = CSprite::SpriteCreate( "sprites/yelflare1.spr", pev->origin, FALSE );
	pSprite->SetTransparency( kRenderTransAdd, 0, 0, 255, 220, kRenderFxNoDissipation );
	pSprite->SetAttachment( edict(), 1 );
	pSprite->SetScale( 0.2 );


	SetThink( &CMonsterCamera::CamAngles );
	pev->nextthink = gpGlobals->time + 0.1;

}


void CMonsterCamera :: KeyValue( KeyValueData *pkvd )
{
	if (FStrEq(pkvd->szKeyName, "viewtarget"))
	{
		pev->netname = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
	{
		CBaseAnimating :: KeyValue( pkvd );
	}
}


void CMonsterCamera :: Use ( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if ( value == 1 )
	{
		//d
#ifndef DONTSAVECAMERASFIX_INVASION_DLL
		pSprite->SetThink( &CBaseEntity::SUB_Remove );
		pSprite->pev->nextthink = gpGlobals->time + 0.1;
#endif

		SetBoneController( 0, 90 );
		SetBoneController( 1, 0 );
		m_iAlarmState = 3;

	}

	else if ( m_iAlarmState == 0 )
	{
		//cr

#ifndef DONTSAVECAMERASFIX_INVASION_DLL
		pSprite->SetThink( &CBaseEntity::SUB_Remove );
		pSprite->pev->nextthink = gpGlobals->time + 0.1;
#endif

		pSprite = CSprite::SpriteCreate( "sprites/yelflare1.spr", pev->origin, FALSE );
		pSprite->SetTransparency( kRenderTransAdd, 255, 0, 0, 220, kRenderFxStrobeFast );
		pSprite->SetAttachment( edict(), 1 );
		pSprite->SetScale( 0.2 );

		SUB_UseTargets( this, USE_TOGGLE, 0 );
		m_iAlarmState = 1;

		EMIT_SOUND ( ENT(pev), CHAN_VOICE, "turret/tu_spinup.wav", 1.0, ATTN_NORM );
	}
}


void CMonsterCamera :: CamAngles ( void )
{
	CBaseEntity *pViewTarget;
	edict_t *pentViewTarget = FIND_ENTITY_BY_TARGETNAME( NULL, STRING(pev->netname));

	if (!FNullEnt(pentViewTarget))
	{
		pViewTarget = CBaseEntity::Instance( pentViewTarget );
		
		Vector vecViewAngles = UTIL_VecToAngles( pViewTarget->pev->origin - pev->origin );

		SetBoneController( 0, -vecViewAngles.x );
		SetBoneController( 1, vecViewAngles.y - pev->angles.y );

	}

	SetThink( NULL );
}




//===========================
//===========================
// - definition de la classe
//

class CFuncKeypad : public CBaseToggle
{
public:
	void Spawn( void );
	void Use ( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void EXPORT VguiThink ( void );

	void Precache ( void );


	virtual int ObjectCaps( void ) { return (CBaseEntity ::ObjectCaps() & ~FCAP_ACROSS_TRANSITION) | FCAP_IMPULSE_USE; }


	void KeyValue( KeyValueData *pkvd );

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	int m_iCode;

};


LINK_ENTITY_TO_CLASS( func_keypad, CFuncKeypad );


//=============================
// - savestore

TYPEDESCRIPTION	CFuncKeypad::m_SaveData[] = 
{
	DEFINE_FIELD( CFuncKeypad, m_iCode, FIELD_INTEGER ),
};

IMPLEMENT_SAVERESTORE( CFuncKeypad, CBaseToggle );


//=============================
//=============================
// - fonctions


void CFuncKeypad :: Spawn ( void )
{
	Precache ();

	pev->angles		= g_vecZero;
	pev->movetype		= MOVETYPE_PUSH;
	pev->solid			= SOLID_BSP;
	
	UTIL_SetOrigin( pev, pev->origin );
	SET_MODEL( ENT(pev), STRING(pev->model) );

	pev->takedamage		= DAMAGE_NO;

	pev->flags |= FL_WORLDBRUSH;

}


void CFuncKeypad :: Precache ( void )
{
	PRECACHE_SOUND ( "buttons/button10.wav" );
}

void CFuncKeypad :: KeyValue( KeyValueData *pkvd )
{
	if (FStrEq(pkvd->szKeyName, "code"))
	{
		m_iCode = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
	{
		CBaseEntity :: KeyValue( pkvd );
	}
}



void CFuncKeypad :: Use ( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	//que le joueur

	if ( !pActivator->IsPlayer() )
		return;

	SetThink ( &CFuncKeypad::VguiThink );
	pev->nextthink = pev->nextthink = pev->ltime + 0.5;

	MESSAGE_BEGIN( MSG_ALL, gmsgKeypad, NULL );
		WRITE_BYTE( 1 );		// on-off
		WRITE_LONG( m_iCode );
		WRITE_LONG( ENTINDEX(edict() ) );
	MESSAGE_END();
}


void CFuncKeypad :: VguiThink ( void )
{
	pev->nextthink = pev->ltime + 0.5;

	CBaseEntity *pPlayer = (CBasePlayer *)CBaseEntity::Instance( g_engfuncs.pfnPEntityOfEntIndex( 1 ) );


	if ( pPlayer == NULL || ( pPlayer->Center() - pev->origin ).Length() > 64 )
	{
		SetThink ( NULL );

		MESSAGE_BEGIN( MSG_ALL, gmsgKeypad, NULL );
			WRITE_BYTE( 0 );		// on-off
		MESSAGE_END();
	}
}

//-----------------------------------------------------
// - Conveyor control

//===========================
//===========================
// - definition de la classe
//

class CConveyorControl : public CBaseToggle
{
public:
	void Spawn( void );
	void Use ( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void EXPORT VguiThink ( void );

	void Precache ( void );


	virtual int ObjectCaps( void ) { return (CBaseEntity ::ObjectCaps() & ~FCAP_ACROSS_TRANSITION) | FCAP_IMPULSE_USE; }

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	int		m_ibitConveyor;
};


LINK_ENTITY_TO_CLASS( func_conveyorcontrol, CConveyorControl );


//=============================
// - savestore

TYPEDESCRIPTION	CConveyorControl::m_SaveData[] = 
{
	DEFINE_FIELD ( CConveyorControl, m_ibitConveyor, FIELD_INTEGER ),
};

IMPLEMENT_SAVERESTORE( CConveyorControl, CBaseToggle );


//=============================
//=============================
// - fonctions


void CConveyorControl :: Spawn ( void )
{
	Precache ();

	pev->angles		= g_vecZero;
	pev->movetype		= MOVETYPE_PUSH;
	pev->solid			= SOLID_BSP;
	
	UTIL_SetOrigin( pev, pev->origin );
	SET_MODEL( ENT(pev), STRING(pev->model) );

	pev->takedamage		= DAMAGE_NO;

	pev->flags |= FL_WORLDBRUSH;

	m_ibitConveyor = 0;

	for ( int i=0; i<4; i++ )
		m_ibitConveyor |= (1<<i);

}


void CConveyorControl :: Precache ( void )
{
	PRECACHE_SOUND ( "buttons/button10.wav" );
}


void CConveyorControl :: Use ( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{

	if  ( useType == USE_OFF )
	{
		int iNewBit = value;

		for ( int i=0; i<5; i++ )
		{
			int iOld = m_ibitConveyor & ( 1<<i );
			int iNew = iNewBit & ( 1<<i );

			if ( iOld != iNew )
			{
				char sz [64];
				sprintf ( sz, "conveyor%i", i+1 );

				CBaseEntity *pPlayer = (CBasePlayer *)CBaseEntity::Instance( g_engfuncs.pfnPEntityOfEntIndex( 1 ) );

				FireTargets( sz, pPlayer, pPlayer, USE_ON, 0 );
			}
		}

		m_ibitConveyor = iNewBit;
	}


	else
	{
		//que le joueur

		if ( !pActivator->IsPlayer() )
			return;

		SetThink ( &CConveyorControl::VguiThink );
		pev->nextthink = pev->nextthink = pev->ltime + 0.5;
		//ALERT( at_console, "Sending conveyor message: %d \n", m_ibitConveyor ); //just a bit of debug code modif de Roy

		MESSAGE_BEGIN( MSG_ALL, gmsgConveyor, NULL );
			WRITE_BYTE( 1 );		// on-off
			WRITE_BYTE( m_ibitConveyor );
		MESSAGE_END();
	}
}


void CConveyorControl :: VguiThink ( void )
{
	pev->nextthink = pev->ltime + 0.5;

	CBaseEntity *pPlayer = (CBasePlayer *)CBaseEntity::Instance( g_engfuncs.pfnPEntityOfEntIndex( 1 ) );


	if ( pPlayer == NULL || ( pPlayer->Center() - pev->origin ).Length() > 64 )
	{
		SetThink ( NULL );

		MESSAGE_BEGIN( MSG_ALL, gmsgConveyor, NULL );
			WRITE_BYTE( 0 );		// on-off
		MESSAGE_END();
	}
}
