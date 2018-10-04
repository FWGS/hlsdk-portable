#include "extdll.h"
#include "util.h"
#include "cbase.h"

#include "bumplight.h"

extern int gmsgBumpLight;

LINK_ENTITY_TO_CLASS( light_bump, CBumpLight );

TYPEDESCRIPTION	CBumpLight::m_SaveData[] = 
{
	DEFINE_FIELD( CBumpLight, m_fStrength, FIELD_FLOAT ),
	DEFINE_FIELD( CBumpLight, m_fRadius, FIELD_FLOAT ),
	DEFINE_FIELD( CBumpLight, m_vColour, FIELD_VECTOR ),
	DEFINE_FIELD( CBumpLight, m_bEnabled, FIELD_BOOLEAN ),
	DEFINE_FIELD( CBumpLight, m_szMovewithEnt, FIELD_STRING ),
	DEFINE_FIELD( CBumpLight, m_bMovewith, FIELD_BOOLEAN ),
	DEFINE_FIELD( CBumpLight, m_iStyle, FIELD_INTEGER ),
};

IMPLEMENT_SAVERESTORE(CBumpLight, CPointEntity);

CBumpLight::CBumpLight(void) : CPointEntity()
{
	m_bMovewith = false;
	memset(m_szMovewithEnt, 0, 64);
	m_bEnabled = true;
	m_iStyle = 0;
}

//
// Cache user-entity-field values until spawn is called.
//
void CBumpLight :: KeyValue( KeyValueData* pkvd)
{
	if (FStrEq(pkvd->szKeyName, "radius"))
	{
		m_fRadius = (float)atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "strength"))
	{
		m_fStrength = (float)atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "movewith"))
	{
		memcpy(m_szMovewithEnt, pkvd->szValue, strlen(pkvd->szValue));
		m_bMovewith = true;
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "appearance"))
	{
		m_iStyle = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
	{
		CPointEntity::KeyValue( pkvd );
	}
}

void CBumpLight :: Spawn( void )
{	
	if (pev->spawnflags & BUMPLIGHT_SPAWNFLAG_START_OFF)
		m_bEnabled = false;
}

void CBumpLight :: Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	m_bEnabled = !m_bEnabled;

	MESSAGE_BEGIN(MSG_ALL, gmsgBumpLight);

	WRITE_BYTE(1);
	WRITE_STRING(STRING(pev->targetname));
	WRITE_BYTE(m_bEnabled ? 1 : 0);

	MESSAGE_END();
}

void CBumpLight::CreateOnClient(void)
{
	MESSAGE_BEGIN(MSG_ALL, gmsgBumpLight);

	WRITE_BYTE(0);

	WRITE_STRING(STRING(pev->targetname));

	WRITE_COORD(pev->origin.x);
	WRITE_COORD(pev->origin.y);
	WRITE_COORD(pev->origin.z);

	WRITE_COORD(m_fRadius);
	WRITE_COORD(m_fStrength);

	WRITE_BYTE((unsigned char)pev->rendercolor.x);
	WRITE_BYTE((unsigned char)pev->rendercolor.y);
	WRITE_BYTE((unsigned char)pev->rendercolor.z);

	WRITE_BYTE(m_iStyle);

	WRITE_BYTE(m_bEnabled ? 1 : 0);

	if (m_bMovewith)
	{
		edict_t *pFind; 
		pFind = FIND_ENTITY_BY_TARGETNAME( NULL, m_szMovewithEnt );

		if (!FNullEnt(pFind))
		{
			WRITE_SHORT(ENTINDEX(pFind));

			WRITE_BYTE(1);

			WRITE_COORD(VARS(pFind)->origin.x);
			WRITE_COORD(VARS(pFind)->origin.y);
			WRITE_COORD(VARS(pFind)->origin.z);

			WRITE_ANGLE(VARS(pFind)->angles.x);
			WRITE_ANGLE(VARS(pFind)->angles.y);
			WRITE_ANGLE(VARS(pFind)->angles.z);
		}
		else
		{
			WRITE_SHORT(-1);
		}
	}
	else
	{
		WRITE_SHORT(-1);
	}

	MESSAGE_END();

	SetThink(NULL);
}