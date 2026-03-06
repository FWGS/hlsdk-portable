#pragma once
#ifndef STEAM_INTEGRATION_H
#define STEAM_INTEGRATION_H

void InitSteam();
void ShutdownSteam();
void SteamRunCallbacks();

void SetAchievement(const char* ID);

#endif
