#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "animation.h"
#include "effects.h"
#include "actanimating.h"

TYPEDESCRIPTION	CActAnimating::m_SaveData[] = 
{
	DEFINE_FIELD( CActAnimating, m_Activity, FIELD_INTEGER ),
	DEFINE_FIELD( CActAnimating, m_iSequence, FIELD_INTEGER ),
};

IMPLEMENT_SAVERESTORE( CActAnimating, CBaseAnimating );

void CActAnimating :: SetActivity( Activity act ) 
{ 
	int sequence = LookupActivity( act ); 
	if ( sequence != ACTIVITY_NOT_AVAILABLE )
	{
		m_iSequence = sequence;
		pev->sequence = sequence;
		m_Activity = act; 
		pev->frame = 0;
		ResetSequenceInfo( );
	}
}

void CActAnimating :: SetSequence( int sequence )
{
	//m_Activity = ACTIVITY_NOT_AVAILABLE;
	pev->sequence = sequence;
	m_iSequence = sequence; 
	pev->frame = 0;
	ResetSequenceInfo( );
}