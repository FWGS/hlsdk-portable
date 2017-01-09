#include "bot_common.h"

CSGameState::CSGameState(CCSBot *owner)
{
	m_owner = owner;
	m_isRoundOver = false;
	m_validateInterval.Invalidate();
}

// Reset at round start

void CSGameState::Reset()
{
	int i;
	CCSBotManager *ctrl = TheCSBots();

	m_isRoundOver = false;

}

// Update game state based on events we have received

void CSGameState::OnEvent(GameEventType event, CBaseEntity *entity, CBaseEntity *other)
{

}

// True if round has been won or lost (but not yet reset)

bool CSGameState::IsRoundOver() const
{
	return m_isRoundOver;
}
