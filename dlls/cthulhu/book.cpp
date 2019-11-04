/***
*
*	Copyright (c) 1999, 2000 Valve LLC. All rights reserved.
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
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"
#include "items.h"
#include "gamerules.h"

extern int gmsgItemPickup;

#define SF_BOOK_FIREONCE		0x00000001

#define NUM_BOOK_PAGES 3

class CBook : public CBaseToggle
{
public:
	void Spawn( void );
	void Precache( void );
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value ); 
	virtual int	ObjectCaps( void ) { return (CBaseToggle :: ObjectCaps() | FCAP_ONOFF_USE) & ~FCAP_ACROSS_TRANSITION; }
	void EXPORT ReadThink( void );

	virtual int		Save( CSave &save ); 
	virtual int		Restore( CRestore &restore );
	
	static	TYPEDESCRIPTION m_SaveData[];

	void KeyValue( KeyValueData *pkvd );

	int		m_iszImage[NUM_BOOK_PAGES];
	float	m_fImageDuration;

	int		m_iImage;

	BOOL	m_bHasFired;
};


LINK_ENTITY_TO_CLASS( item_book, CBook );

TYPEDESCRIPTION	CBook::m_SaveData[] = 
{
	DEFINE_FIELD( CBook, m_iImage, FIELD_INTEGER ),
	DEFINE_FIELD( CBook, m_iszImage[0], FIELD_STRING ),
	DEFINE_FIELD( CBook, m_iszImage[1], FIELD_STRING ),
	DEFINE_FIELD( CBook, m_iszImage[2], FIELD_STRING ),
	DEFINE_FIELD( CBook, m_fImageDuration, FIELD_FLOAT ),
	DEFINE_FIELD( CBook, m_bHasFired, FIELD_BOOLEAN ),
};


IMPLEMENT_SAVERESTORE( CBook, CBaseToggle);

void CBook :: Spawn( void )
{
	Precache( );
	pev->solid		= SOLID_BBOX;
	pev->movetype	= MOVETYPE_NONE;
	pev->takedamage = DAMAGE_NO;
	pev->sequence	= 0;
	pev->frame		= 0;

	// initialise the book pages
	m_iImage = -1;

	// we haven't fired yet
	m_bHasFired = FALSE;

	UTIL_SetOrigin(this, pev->origin);		// set size and link into world
//	UTIL_SetSize(pev, Vector(-16,-16,0), Vector(16,16,16));
	SET_MODEL(ENT(pev), STRING(pev->model));

	ResetSequenceInfo( );
	pev->frame = 0;
	// NB: FallInit is a weapons function, so make sure the book is on a surface in Worldcraft,
	// since it will not fall (i.e. it is not affected by gravity!).
	if (DROP_TO_FLOOR(ENT(pev)) == 0)
	{
		ALERT(at_error, "Book %s fell out of level at %f,%f,%f", STRING( pev->classname ), pev->origin.x, pev->origin.y, pev->origin.z);
		UTIL_Remove( this );
		return;
	}
}

void CBook::Precache( void )
{
	char* sz = (char*)STRING(pev->model);
	PRECACHE_MODEL(sz);
}

void CBook::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value ) 
{
	// we are already reading this book
	if (m_iImage >= 0) return;

	// write a message to the client to display the picture
	m_iImage = 0;
	UTIL_ReadBook(STRING(m_iszImage[m_iImage]));
	SetThink(ReadThink);
	SetNextThink(m_fImageDuration);

	// if we haven't fired or we are allowed to fire more than once, fire target
	if (!m_bHasFired || !(pev->spawnflags & SF_BOOK_FIREONCE))
	{
		SUB_UseTargets( NULL, USE_TOGGLE, 0 );
		m_bHasFired = TRUE;
	}
}

void CBook::KeyValue( KeyValueData *pkvd )
{
	if (FStrEq(pkvd->szKeyName, "image1"))
	{
		m_iszImage[0] = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "image2"))
	{
		m_iszImage[1] = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "image3"))
	{
		m_iszImage[2] = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "imageduration"))
	{
		m_fImageDuration = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else 
		CBaseToggle::KeyValue( pkvd );
}

void CBook::ReadThink(  )
{
	// turn the page
	m_iImage++;

	if (m_iImage == NUM_BOOK_PAGES || FStringNull(m_iszImage[m_iImage]))
	{
		// we have finished the book
		UTIL_ReadBook("");
		DontThink();
		m_iImage = -1;
	}
	else
	{
		// clear the page
		UTIL_ReadBook("");
		// show the next page
		UTIL_ReadBook(STRING(m_iszImage[m_iImage]));
		SetNextThink(m_fImageDuration);
	}
}
