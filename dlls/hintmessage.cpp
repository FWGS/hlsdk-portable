#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "hintmessage.h"

extern int gmsgHudText;

void CHintMessageQueue::Reset()
{
	m_iCount = 0;
	m_iCurrentMessage = 0;
	m_iLastMessage = 1;
	m_tmMessageEnd = 0;
	memset( m_aszMessage, 0, sizeof( m_aszMessage ) );
}

void CHintMessageQueue::Update( CBaseEntity& client )
{
	if ( !( gpGlobals->time > m_tmMessageEnd ) || !m_iCount )
		return;

	m_aszMessage[m_iCurrentMessage] = NULL;
	m_iCount--;
	m_iCurrentMessage = ( m_iCurrentMessage + 1 ) % ARRAYSIZE( m_aszMessage );
	m_tmMessageEnd = gpGlobals->time + 6;
	if ( !m_aszMessage[m_iCurrentMessage] )
		return;

	MESSAGE_BEGIN( MSG_ONE, gmsgHudText, NULL, client.pev );
	const char* message = m_aszMessage[m_iCurrentMessage];
	char text[500];
	if ( strlen( message ) > sizeof( text ) - 1 )
	{
		strncpy( text, message, sizeof( text ) - 1 );
		text[sizeof( text ) - 1] = 0;
		message = text;
	}
	WRITE_STRING( message );
	WRITE_BYTE( 1 );
	MESSAGE_END();
}

bool CHintMessageQueue::AddMessage( const char* message )
{
	if ( m_iCount > 7 )
		return false;
	m_aszMessage[m_iLastMessage] = message;
	m_iLastMessage = ( m_iLastMessage + 1 ) % ARRAYSIZE( m_aszMessage );
	m_iCount++;
	return true;
}
