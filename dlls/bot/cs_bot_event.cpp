#include "bot_common.h"

void CCSBot::OnEvent(GameEventType event, CBaseEntity *entity, CBaseEntity *other)
{

	GetGameState()->OnEvent(event, entity, other);
	GetChatter()->OnEvent(event, entity, other);

	if (!IsAlive())
		return;

	CBasePlayer *player = static_cast<CBasePlayer *>(entity);

	// If we just saw a nearby friend die, and we haven't yet acquired an enemy
	// automatically acquire our dead friend's killer
	if (!IsAttacking() && (GetDisposition() == ENGAGE_AND_INVESTIGATE || GetDisposition() == OPPORTUNITY_FIRE))
	{
		if (event == EVENT_PLAYER_DIED)
		{
#if 0
			if (player->m_iTeam == m_iTeam)
			{
				CBasePlayer *killer = static_cast<CBasePlayer *>(other);

				// check that attacker is an enemy (for friendly fire, etc)
				if (killer && killer->IsPlayer())
				{
					// check if we saw our friend die - dont check FOV - assume we're aware of our surroundings in combat
					// snipers stay put
					if (!IsSniper() && IsVisible(&player->pev->origin))
					{
						// people are dying - we should hurry
						Hurry(RANDOM_FLOAT(10.0f, 15.0f));

						// if we're hiding with only our knife, be a little more cautious
						const float knifeAmbushChance = 50.0f;
						if (!IsHiding() || !IsUsingKnife() || RANDOM_FLOAT(0, 100) < knifeAmbushChance)
						{
							PrintIfWatched("Attacking our friend's killer!\n");
							Attack(killer);
							return;
						}
					}
				}
			}
#endif
		}
	}

	switch (event)
	{
#if 0
		case EVENT_PLAYER_DIED:
		{
			CBasePlayer *victim = player;
			CBasePlayer *killer = (other && other->IsPlayer()) ? static_cast<CBasePlayer *>(other) : NULL;

			// if the human player died in the single player game, tell the team
			if (g_pGameRules->IsCareer() && !victim->IsBot() && victim->m_iTeam == m_iTeam)
			{
				GetChatter()->Say("CommanderDown", 20.0f);
			}

			// keep track of the last player we killed
			if (killer == this)
			{
				m_lastVictimID = victim->entindex();
			}

			// react to teammate death
			if (victim->m_iTeam == m_iTeam)
			{
				// chastise friendly fire from humans
				if (killer != NULL && !killer->IsBot() && killer->m_iTeam == m_iTeam && killer != this)
				{
					GetChatter()->KilledFriend();
				}

				if (IsHunting())
				{
					PrintIfWatched("Rethinking hunt due to teammate death\n");
					Idle();
					return;
				}

				if (IsAttacking())
				{
					if (GetTimeSinceLastSawEnemy() > 0.4f)
					{
						PrintIfWatched("Rethinking my attack due to teammate death\n");

						// allow us to sneak past windows, doors, etc
						IgnoreEnemies(1.0f);

						// move to last known position of enemy - this could cause us to flank if 
						// the danger has changed due to our teammate's recent death
						SetTask(MOVE_TO_LAST_KNOWN_ENEMY_POSITION, GetEnemy());
						MoveTo(&GetLastKnownEnemyPosition());
						return;
					}
				}
			}
			// an enemy was killed
			else
			{
				if (killer != NULL && killer->m_iTeam == m_iTeam)
				{
					// only chatter about enemy kills if we see them occur, and they were the last one we see
					if (GetNearbyEnemyCount() <= 1)
					{
						// report if number of enemies left is few and we killed the last one we saw locally
						GetChatter()->EnemiesRemaining();

						if (IsVisible(&victim->pev->origin, CHECK_FOV))
						{						
							// congratulate teammates on their kills
							if (killer != NULL && killer != this)
							{
								float delay = RANDOM_FLOAT(2.0f, 3.0f);
								if (killer->IsBot())
								{
									if (RANDOM_FLOAT(0.0f, 100.0f) < 40.0f)
										GetChatter()->Say("NiceShot", 3.0f, delay);
								}
								else
								{
									// humans get the honorific
									if (g_pGameRules->IsCareer())
										GetChatter()->Say("NiceShotCommander", 3.0f, delay);
									else
										GetChatter()->Say("NiceShotSir", 3.0f, delay);
								}
							}
						}
					}
				}
			}
			return;
		}
#endif
		case EVENT_WEAPON_FIRED:
		case EVENT_WEAPON_FIRED_ON_EMPTY:
		case EVENT_WEAPON_RELOADED:
		{
			if (m_enemy == entity && IsUsingKnife())
				ForceRun(5.0f);
			break;
		}
		default:
			break;
	}

	// Process radio events from our team
	if (player != NULL && event > EVENT_START_RADIO_1 && event < EVENT_END_RADIO)
	{
		// TODO: Distinguish between radio commands and responses
		if (event != EVENT_RADIO_AFFIRMATIVE && event != EVENT_RADIO_NEGATIVE && event != EVENT_RADIO_REPORTING_IN)
		{
			m_lastRadioCommand = event;
			m_lastRadioRecievedTimestamp = gpGlobals->time;
			m_radioSubject = player;
			m_radioPosition = player->pev->origin;
		}
	}

	// player_follows needs a player
	if (player == NULL)
		return;

	// don't pay attention to noise that friends make
	if (!IsEnemy(player))
		return;

	float range;
	PriorityType priority;
	bool isHostile;

	if (IsGameEventAudible(event, entity, other, &range, &priority, &isHostile) == false)
		return;

	// check if noise is close enough for us to hear
	const Vector *newNoisePosition = &player->pev->origin;
	float newNoiseDist = (pev->origin - *newNoisePosition).Length();
	if (newNoiseDist < range)
	{
		// we heard the sound
		if ((IsLocalPlayerWatchingMe() && cv_bot_debug.value == 3.0f) || cv_bot_debug.value == 4.0f)
		{
			PrintIfWatched("Heard noise (%s from %s, pri %s, time %3.1f)\n",
				(event == EVENT_WEAPON_FIRED) ? "Weapon fire " : "",
				STRING(player->pev->netname),
				(priority == PRIORITY_HIGH) ? "HIGH" : ((priority == PRIORITY_MEDIUM) ? "MEDIUM" : "LOW"),
				gpGlobals->time);
		}

		if (event == EVENT_PLAYER_FOOTSTEP && IsUsingSniperRifle() && newNoiseDist < 300.0)
			EquipPistol();

		// should we pay attention to it
		// if noise timestamp is zero, there is no prior noise
		if (m_noiseTimestamp > 0.0f)
		{
			// only overwrite recent sound if we are louder (closer), or more important - if old noise was long ago, its faded
			const float shortTermMemoryTime = 3.0f;
			if (gpGlobals->time - m_noiseTimestamp < shortTermMemoryTime)
			{
				// prior noise is more important - ignore new one
				if (priority < m_noisePriority)
					return;

				float oldNoiseDist = (pev->origin - m_noisePosition).Length();
				if (newNoiseDist >= oldNoiseDist)
					return;
			}
		}


		// find the area in which the noise occured
		// TODO: Better handle when noise occurs off the nav mesh
		// TODO: Make sure noise area is not through a wall or ceiling from source of noise
		// TODO: Change GetNavTravelTime to better deal with NULL destination areas
		CNavArea *noiseArea = TheNavAreaGrid.GetNavArea(newNoisePosition);
		if (noiseArea == NULL)
			noiseArea = TheNavAreaGrid.GetNearestNavArea(newNoisePosition);

		if (noiseArea == NULL)
		{
			PrintIfWatched("  *** Noise occurred off the nav mesh - ignoring!\n");
			return;
		}

		m_noiseArea = noiseArea;

		// remember noise priority
		m_noisePriority = priority;

		// randomize noise position in the area a bit - hearing isn't very accurate
		// the closer the noise is, the more accurate our placement
		// TODO: Make sure not to pick a position on the opposite side of ourselves.
		const float maxErrorRadius = 400.0f;
		const float maxHearingRange = 2000.0f;
		float errorRadius = maxErrorRadius * newNoiseDist / maxHearingRange;

		m_noisePosition.x = newNoisePosition->x + RANDOM_FLOAT(-errorRadius, errorRadius);
		m_noisePosition.y = newNoisePosition->y + RANDOM_FLOAT(-errorRadius, errorRadius);

		// make sure noise position remains in the same area
		m_noiseArea->GetClosestPointOnArea(&m_noisePosition, &m_noisePosition);

		m_isNoiseTravelRangeChecked = false;
		// note when we heard the noise
		m_noiseTimestamp = gpGlobals->time;
		
	}
}
