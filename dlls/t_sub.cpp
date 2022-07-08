/********************************************************************
*																	*
*		t_sub.cpp - code du trigger_submodel						*
*																	*
*		par Julien													*
*																	*
********************************************************************/


//===========================
//===========================
// - include

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"




//===========================
//===========================
// - definition de la classe

class CTriggerSubModel : public CBaseEntity
{
public:
	void Spawn( void );
	void EXPORT ChangeSub ( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );

	void KeyValue( KeyValueData *pkvd );

	int	m_iBodygroup;
	int m_iSubmodel;

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

};


LINK_ENTITY_TO_CLASS( trigger_submodel, CTriggerSubModel );

//=============================
//=============================
// - savestore



TYPEDESCRIPTION	CTriggerSubModel::m_SaveData[] = 
{
	DEFINE_FIELD( CTriggerSubModel, m_iBodygroup, FIELD_INTEGER ),
	DEFINE_FIELD( CTriggerSubModel, m_iSubmodel, FIELD_INTEGER ),
};

IMPLEMENT_SAVERESTORE( CTriggerSubModel, CBaseEntity );



//=============================
//=============================
// - fonctions


void CTriggerSubModel :: Spawn ( void )
{
	pev->solid			= SOLID_NOT;
	pev->movetype		= MOVETYPE_NONE;	
	pev->takedamage		= DAMAGE_NO;

	SetUse ( &CTriggerSubModel::ChangeSub );
}

void CTriggerSubModel :: KeyValue( KeyValueData *pkvd )
{
	if (FStrEq(pkvd->szKeyName, "bodygroup"))
	{
		m_iBodygroup = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "submodel") )
	{
		m_iSubmodel = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
	{
		CBaseEntity :: KeyValue( pkvd );
	}
}

void CTriggerSubModel :: ChangeSub ( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if ( FStringNull ( pev->target ) )
		return;

	edict_t *pentTarget = FIND_ENTITY_BY_TARGETNAME ( NULL, STRING(pev->target) );

	if ( FNullEnt(pentTarget) )
	{
		ALERT ( at_console, "CTriggerSubModel : pas d'entite s appelant %s\n", STRING(pev->target) );
		return;
	}

	CBaseEntity *pTarget = Instance( pentTarget );

	if ( pTarget->MyMonsterPointer() == NULL )
	{
		ALERT ( at_console, "CTriggerSubModel : %s n est pas un monstre\n", STRING(pev->target) );
		return;
	}

	CBaseMonster *pMonster = (CBaseMonster*)pTarget;
	
	pMonster->SetBodygroup( m_iBodygroup, m_iSubmodel);
}

