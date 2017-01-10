#include "bot_common.h"

CHLGameState::CHLGameState(CHLBot *owner)
{
	m_owner = owner;
	m_isRoundOver = false;
	m_validateInterval.Invalidate();
}

// Reset at round start

void CHLGameState::Reset()
{
	int i;
	CHLBotManager *ctrl = TheHLBots();

	m_isRoundOver = false;

}

// Update game state based on events we have received

void CHLGameState::OnEvent(GameEventType event, CBaseEntity *entity, CBaseEntity *other)
{

}

// True if round has been won or lost (but not yet reset)

bool CHLGameState::IsRoundOver() const
{
	return m_isRoundOver;
}
