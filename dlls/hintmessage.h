#ifndef HINTMESSAGE_H
#define HINTMESSAGE_H

class CBaseEntity;


//--------------------------------------------------------------------------------------------------------------
class CHintMessageQueue
{
public:
	void Reset();
	void Update( CBaseEntity& client );
	bool AddMessage( const char* message );
	bool IsEmpty() { return m_iCount == 0; }

private:
	int			m_iCount;
	int			m_iCurrentMessage;
	int			m_iLastMessage;
	float		m_tmMessageEnd;
	const char*	m_aszMessage[8];
};

#endif
