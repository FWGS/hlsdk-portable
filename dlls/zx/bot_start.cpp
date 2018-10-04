//
// HPB bot - botman's High Ping Bastard bot
//
// (http://planethalflife.com/botman/)
//
// bot_start.cpp
//

#include "extdll.h"
#include "util.h"
#include "cbase.h"

#include "bot.h"
#include "bot_func.h"
#include "bot_weapons.h"

extern int mod_id;
extern edict_t *pent_info_ctfdetect;

extern int max_team_players[4];
extern int team_class_limits[4];
extern int max_teams;


void BotStartGame( bot_t *pBot )
{
   char c_team[32];
   char c_class[32];
   char c_item[32];
   int index, count, retry_count;
   edict_t *pPlayer;
   int team;
   int class_not_allowed;

   edict_t *pEdict = pBot->pEdict;

   if (mod_id == TFC_DLL)
   {
      if ((pBot->start_action == MSG_TFC_IDLE) &&
          (pBot->f_create_time + 3.0 <= gpGlobals->time))
      {
         pBot->start_action = MSG_TFC_TEAM_SELECT;  // force team selection
      }

      // handle Team Fortress Classic stuff here...

      if (pBot->start_action == MSG_TFC_TEAM_SELECT)
      {
         pBot->start_action = MSG_TFC_IDLE;  // switch back to idle
         pBot->f_create_time = gpGlobals->time;  // reset

         if ((pBot->bot_team != 1) && (pBot->bot_team != 2) &&
             (pBot->bot_team != 3) && (pBot->bot_team != 4) &&
             (pBot->bot_team != 5))
            pBot->bot_team = -1;

         if (pBot->bot_team == -1)
            pBot->bot_team = RANDOM_LONG(1, max_teams);

         retry_count = 0;

         while ((retry_count < 4) &&
                (max_team_players[pBot->bot_team-1] > 0))  // not unlimited?
         {
            count = 0;

            // count number of players on this team...
            for (index = 1; index <= gpGlobals->maxClients; index++)
            {
               pPlayer = INDEXENT(index);

               if (pPlayer && !pPlayer->free)
               {
                  if (UTIL_GetTeam(pPlayer) == (pBot->bot_team - 1))
                     count++;
               }
            }

            if (count < max_team_players[pBot->bot_team-1])
               break;  // haven't reached limit yet, continue
            else
            {
               pBot->bot_team++;

               if (pBot->bot_team > max_teams)
                  pBot->bot_team = 1;

               retry_count++;
            }
         }

         // select the team the bot wishes to join...
         if (pBot->bot_team == 1)
            strcpy(c_team, "1");
         else if (pBot->bot_team == 2)
            strcpy(c_team, "2");
         else if (pBot->bot_team == 3)
            strcpy(c_team, "3");
         else if (pBot->bot_team == 4)
            strcpy(c_team, "4");
         else
            strcpy(c_team, "5");

         FakeClientCommand(pEdict, "jointeam", c_team, NULL);

         return;
      }

      if (pBot->start_action == MSG_TFC_CLASS_SELECT)
      {
         pBot->start_action = MSG_TFC_IDLE;  // switch back to idle
         pBot->f_create_time = gpGlobals->time;  // reset

         if ((pBot->bot_class < 0) || (pBot->bot_class > 10))
            pBot->bot_class = -1;

         if (pBot->bot_class == -1)
            pBot->bot_class = RANDOM_LONG(1, 10);

         team = UTIL_GetTeam(pEdict);

         if (team_class_limits[team] == -1)  // civilian only?
         {
            pBot->bot_class = 0;  // civilian
         }
         else
         {
            if (pBot->bot_class == 10)
               class_not_allowed = team_class_limits[team] & (1<<7);
            else if (pBot->bot_class <= 7)
               class_not_allowed = team_class_limits[team] & (1<<(pBot->bot_class-1));
            else
               class_not_allowed = team_class_limits[team] & (1<<(pBot->bot_class));

            while (class_not_allowed)
            {
               pBot->bot_class = RANDOM_LONG(1, 10);

               if (pBot->bot_class == 10)
                  class_not_allowed = team_class_limits[team] & (1<<7);
               else if (pBot->bot_class <= 7)
                  class_not_allowed = team_class_limits[team] & (1<<(pBot->bot_class-1));
               else
                  class_not_allowed = team_class_limits[team] & (1<<(pBot->bot_class));
            }
         }

         // select the class the bot wishes to use...
         if (pBot->bot_class == 0)
            strcpy(c_class, "civilian");
         else if (pBot->bot_class == 1)
            strcpy(c_class, "scout");
         else if (pBot->bot_class == 2)
            strcpy(c_class, "sniper");
         else if (pBot->bot_class == 3)
            strcpy(c_class, "soldier");
         else if (pBot->bot_class == 4)
            strcpy(c_class, "demoman");
         else if (pBot->bot_class == 5)
            strcpy(c_class, "medic");
         else if (pBot->bot_class == 6)
            strcpy(c_class, "hwguy");
         else if (pBot->bot_class == 7)
            strcpy(c_class, "pyro");
         else if (pBot->bot_class == 8)
            strcpy(c_class, "spy");
         else if (pBot->bot_class == 9)
            strcpy(c_class, "engineer");
         else
            strcpy(c_class, "randompc");

         FakeClientCommand(pEdict, c_class, NULL, NULL);

         // bot has now joined the game (doesn't need to be started)
         pBot->not_started = 0;

         return;
      }
   }
   else if (mod_id == CSTRIKE_DLL)
   {
      // handle Counter-Strike stuff here...

      if (pBot->start_action == MSG_CS_TEAM_SELECT)
      {
         pBot->start_action = MSG_CS_IDLE;  // switch back to idle

         if ((pBot->bot_team != 1) && (pBot->bot_team != 2) &&
             (pBot->bot_team != 5))
            pBot->bot_team = -1;

         if (pBot->bot_team == -1)
            pBot->bot_team = RANDOM_LONG(1, 2);

         // select the team the bot wishes to join...
         if (pBot->bot_team == 1)
            strcpy(c_team, "1");
         else if (pBot->bot_team == 2)
            strcpy(c_team, "2");
         else
            strcpy(c_team, "5");

         FakeClientCommand(pEdict, "menuselect", c_team, NULL);

         return;
      }

      if (pBot->start_action == MSG_CS_CT_SELECT)  // counter terrorist
      {
         pBot->start_action = MSG_CS_IDLE;  // switch back to idle

         if ((pBot->bot_class < 1) || (pBot->bot_class > 4))
            pBot->bot_class = -1;  // use random if invalid

         if (pBot->bot_class == -1)
            pBot->bot_class = RANDOM_LONG(1, 4);

         // select the class the bot wishes to use...
         if (pBot->bot_class == 1)
            strcpy(c_class, "1");
         else if (pBot->bot_class == 2)
            strcpy(c_class, "2");
         else if (pBot->bot_class == 3)
            strcpy(c_class, "3");
         else if (pBot->bot_class == 4)
            strcpy(c_class, "4");
         else
            strcpy(c_class, "5");  // random

         FakeClientCommand(pEdict, "menuselect", c_class, NULL);

         // bot has now joined the game (doesn't need to be started)
         pBot->not_started = 0;

         return;
      }

      if (pBot->start_action == MSG_CS_T_SELECT)  // terrorist select
      {
         pBot->start_action = MSG_CS_IDLE;  // switch back to idle

         if ((pBot->bot_class < 1) || (pBot->bot_class > 4))
            pBot->bot_class = -1;  // use random if invalid

         if (pBot->bot_class == -1)
            pBot->bot_class = RANDOM_LONG(1, 4);

         // select the class the bot wishes to use...
         if (pBot->bot_class == 1)
            strcpy(c_class, "1");
         else if (pBot->bot_class == 2)
            strcpy(c_class, "2");
         else if (pBot->bot_class == 3)
            strcpy(c_class, "3");
         else if (pBot->bot_class == 4)
            strcpy(c_class, "4");
         else
            strcpy(c_class, "5");  // random

         FakeClientCommand(pEdict, "menuselect", c_class, NULL);

         // bot has now joined the game (doesn't need to be started)
         pBot->not_started = 0;

         return;
      }
   }
   else if ((mod_id == GEARBOX_DLL) && (pent_info_ctfdetect != NULL))
   {
      // handle Opposing Force CTF stuff here...

      if (pBot->start_action == MSG_OPFOR_TEAM_SELECT)
      {
         pBot->start_action = MSG_OPFOR_IDLE;  // switch back to idle

         if ((pBot->bot_team != 1) && (pBot->bot_team != 2) &&
             (pBot->bot_team != 3))
            pBot->bot_team = -1;

         if (pBot->bot_team == -1)
            pBot->bot_team = RANDOM_LONG(1, 2);

         // select the team the bot wishes to join...
         if (pBot->bot_team == 1)
            strcpy(c_team, "1");
         else if (pBot->bot_team == 2)
            strcpy(c_team, "2");
         else
            strcpy(c_team, "3");

         FakeClientCommand(pEdict, "jointeam", c_team, NULL);

         return;
      }

      if (pBot->start_action == MSG_OPFOR_CLASS_SELECT)
      {
         pBot->start_action = MSG_OPFOR_IDLE;  // switch back to idle

         if ((pBot->bot_class < 0) || (pBot->bot_class > 10))
            pBot->bot_class = -1;

         if (pBot->bot_class == -1)
            pBot->bot_class = RANDOM_LONG(1, 10);

         // select the class the bot wishes to use...
         if (pBot->bot_class == 1)
            strcpy(c_class, "1");
         else if (pBot->bot_class == 2)
            strcpy(c_class, "2");
         else if (pBot->bot_class == 3)
            strcpy(c_class, "3");
         else if (pBot->bot_class == 4)
            strcpy(c_class, "4");
         else if (pBot->bot_class == 5)
            strcpy(c_class, "5");
         else if (pBot->bot_class == 6)
            strcpy(c_class, "6");
         else
            strcpy(c_class, "7");

         FakeClientCommand(pEdict, "selectchar", c_class, NULL);

         // bot has now joined the game (doesn't need to be started)
         pBot->not_started = 0;

         return;
      }
   }
   else if (mod_id == FRONTLINE_DLL)
   {
      // handle FrontLineForce stuff here...

      if (pBot->start_action == MSG_FLF_TEAM_SELECT)
      {
         pBot->start_action = MSG_FLF_IDLE;  // switch back to idle

         if ((pBot->bot_team != 1) && (pBot->bot_team != 2) &&
             (pBot->bot_team != 5))
            pBot->bot_team = -1;

         if (pBot->bot_team == -1)
            pBot->bot_team = RANDOM_LONG(1, 2);

         // select the team the bot wishes to join...
         if (pBot->bot_team == 1)
            strcpy(c_team, "1");
         else if (pBot->bot_team == 2)
            strcpy(c_team, "2");
         else
            strcpy(c_team, "5");

         FakeClientCommand(pEdict, "jointeam", c_team, NULL);

         return;
      }

      if (pBot->start_action == MSG_FLF_CLASS_SELECT)
      {
         pBot->start_action = MSG_FLF_IDLE;  // switch back to idle

         team = UTIL_GetTeam(pEdict);

         if (team == 0)  // rebels
         {
            if ((pBot->bot_class < 0) || (pBot->bot_class > 3))
               pBot->bot_class = -1;

            if (pBot->bot_class == -1)
               pBot->bot_class = RANDOM_LONG(1, 3);

            // select the class the bot wishes to use...
            if (pBot->bot_class == 1)
               strcpy(c_class, "rebelsrecon");
            else if (pBot->bot_class == 2)
               strcpy(c_class, "rebelsassault");
            else
               strcpy(c_class, "rebelssupport");
         }
         else // commandos
         {
            if ((pBot->bot_class < 0) || (pBot->bot_class > 3))
               pBot->bot_class = -1;

            if (pBot->bot_class == -1)
               pBot->bot_class = RANDOM_LONG(1, 3);

            // select the class the bot wishes to use...
            if (pBot->bot_class == 1)
               strcpy(c_class, "commandosrecon");
            else if (pBot->bot_class == 2)
               strcpy(c_class, "commandosassault");
            else
               strcpy(c_class, "commandossupport");
         }

         FakeClientCommand(pEdict, c_class, NULL, NULL);

         return;
      }

      if (pBot->start_action == MSG_FLF_PISTOL_SELECT)
      {
         int prim_weapon_group, sec_weapon_group;

         pBot->start_action = MSG_FLF_IDLE;  // switch back to idle

         int flf_class = UTIL_GetClass(pEdict);

         if (flf_class == 0)  // recon
         {
            prim_weapon_group = RANDOM_LONG(1, 3);

            if (prim_weapon_group == 1)  // shotguns
            {
               int weapon = RANDOM_LONG(1, 2);
               
               if (weapon == 1)
                  pBot->primary_weapon = FLF_WEAPON_SPAS12;
               else
                  pBot->primary_weapon = FLF_WEAPON_RS202M2;
            }
            else if (prim_weapon_group == 2)  // submachine
            {
               int weapon = RANDOM_LONG(1, 4);

               if (weapon == 1)
                  pBot->primary_weapon = FLF_WEAPON_MP5A2;
               else if (weapon == 2)
                  pBot->primary_weapon = FLF_WEAPON_MP5SD;
               else if (weapon == 3)
                  pBot->primary_weapon = FLF_WEAPON_MAC10;
               else
                  pBot->primary_weapon = FLF_WEAPON_UMP45;
            }
            else  // rifles
            {
               int weapon = RANDOM_LONG(1, 2);

               if (weapon == 1)
                  pBot->primary_weapon = FLF_WEAPON_MSG90;
               else
                  pBot->primary_weapon = FLF_WEAPON_SAKO;
            }

            if (prim_weapon_group == 1)
               sec_weapon_group = RANDOM_LONG(2, 3);
            else if (prim_weapon_group == 3)
               sec_weapon_group = RANDOM_LONG(1, 2);
            else
            {
               if (RANDOM_LONG(1, 100) <= 50)
                  sec_weapon_group = 1;
               else
                  sec_weapon_group = 3;
            }

            if (sec_weapon_group == 1)  // shotguns
            {
               int weapon = RANDOM_LONG(1, 2);
               
               if (weapon == 1)
                  pBot->secondary_weapon = FLF_WEAPON_SPAS12;
               else
                  pBot->secondary_weapon = FLF_WEAPON_RS202M2;
            }
            else if (sec_weapon_group == 2)  // submachine
            {
               int weapon = RANDOM_LONG(1, 4);

               if (weapon == 1)
                  pBot->secondary_weapon = FLF_WEAPON_MP5A2;
               else if (weapon == 2)
                  pBot->secondary_weapon = FLF_WEAPON_MP5SD;
               else if (weapon == 3)
                  pBot->secondary_weapon = FLF_WEAPON_MAC10;
               else
                  pBot->secondary_weapon = FLF_WEAPON_UMP45;
            }
            else  // rifles
            {
               int weapon = RANDOM_LONG(1, 2);

               if (weapon == 1)
                  pBot->secondary_weapon = FLF_WEAPON_MSG90;
               else
                  pBot->secondary_weapon = FLF_WEAPON_SAKO;
            }
         }
         else if (flf_class == 1)  // assault
         {
            prim_weapon_group = RANDOM_LONG(1, 3);

            if (prim_weapon_group == 1)  // shotguns
            {
               int weapon = RANDOM_LONG(1, 2);
               
               if (weapon == 1)
                  pBot->primary_weapon = FLF_WEAPON_SPAS12;
               else
                  pBot->primary_weapon = FLF_WEAPON_RS202M2;
            }
            else if (prim_weapon_group == 2)  // submachine
            {
               int weapon = RANDOM_LONG(1, 4);

               if (weapon == 1)
                  pBot->primary_weapon = FLF_WEAPON_MP5A2;
               else if (weapon == 2)
                  pBot->primary_weapon = FLF_WEAPON_MP5SD;
               else if (weapon == 3)
                  pBot->primary_weapon = FLF_WEAPON_MAC10;
               else
                  pBot->primary_weapon = FLF_WEAPON_UMP45;
            }
            else  // rifles
            {
               int weapon = RANDOM_LONG(1, 3);

               if (weapon == 1)
                  pBot->primary_weapon = FLF_WEAPON_M4;
               else if (weapon == 2)
                  pBot->primary_weapon = FLF_WEAPON_FAMAS;
               else
                  pBot->primary_weapon = FLF_WEAPON_AK5;
            }

            if (prim_weapon_group == 1)
               sec_weapon_group = RANDOM_LONG(2, 3);
            else if (prim_weapon_group == 2)
            {
               if (RANDOM_LONG(1, 100) <= 50)
                  sec_weapon_group = 1;
               else
                  sec_weapon_group = 3;
            }
            else  // prim == 3
               sec_weapon_group = RANDOM_LONG(1, 2);

            if (sec_weapon_group == 1)  // shotguns
            {
               int weapon = RANDOM_LONG(1, 2);
               
               if (weapon == 1)
                  pBot->secondary_weapon = FLF_WEAPON_SPAS12;
               else
                  pBot->secondary_weapon = FLF_WEAPON_RS202M2;
            }
            else if (sec_weapon_group == 2)  // submachine
            {
               int weapon = RANDOM_LONG(1, 4);

               if (weapon == 1)
                  pBot->secondary_weapon = FLF_WEAPON_MP5A2;
               else if (weapon == 2)
                  pBot->secondary_weapon = FLF_WEAPON_MP5SD;
               else if (weapon == 3)
                  pBot->secondary_weapon = FLF_WEAPON_MAC10;
               else
                  pBot->secondary_weapon = FLF_WEAPON_UMP45;
            }
            else  // rifles
            {
               int weapon = RANDOM_LONG(1, 3);

               if (weapon == 1)
                  pBot->secondary_weapon = FLF_WEAPON_M4;
               else if (weapon == 2)
                  pBot->secondary_weapon = FLF_WEAPON_FAMAS;
               else
                  pBot->secondary_weapon = FLF_WEAPON_AK5;
            }
         }
         else  // support
         {
            prim_weapon_group = RANDOM_LONG(1, 3);

            if (prim_weapon_group == 1)  // shotguns
            {
               int weapon = RANDOM_LONG(1, 2);
               
               if (weapon == 1)
                  pBot->primary_weapon = FLF_WEAPON_SPAS12;
               else
                  pBot->primary_weapon = FLF_WEAPON_RS202M2;
            }
            else if (prim_weapon_group == 2)  // submachine
            {
               int weapon = RANDOM_LONG(1, 4);

               if (weapon == 1)
                  pBot->primary_weapon = FLF_WEAPON_MP5A2;
               else if (weapon == 2)
                  pBot->primary_weapon = FLF_WEAPON_MP5SD;
               else if (weapon == 3)
                  pBot->primary_weapon = FLF_WEAPON_MAC10;
               else
                  pBot->primary_weapon = FLF_WEAPON_UMP45;
            }
            else if (prim_weapon_group == 3)  // rifles & heavyweapons
            {
               if (RANDOM_LONG(1, 100) <= 50)
               {
                  int weapon = RANDOM_LONG(1, 3);  // rifles

                  if (weapon == 1)
                     pBot->primary_weapon = FLF_WEAPON_M4;
                  else if (weapon == 2)
                     pBot->primary_weapon = FLF_WEAPON_FAMAS;
                  else
                     pBot->primary_weapon = FLF_WEAPON_AK5;
               }
               else  // heavy weapons
               {
                  pBot->primary_weapon = FLF_WEAPON_HK21;
               }
            }

            if (prim_weapon_group == 1)
               sec_weapon_group = RANDOM_LONG(2, 3);
            else if (prim_weapon_group == 2)
            {
               if (RANDOM_LONG(1, 100) <= 50)
                  sec_weapon_group = 1;
               else
                  sec_weapon_group = 3;
            }
            else  // prim == 3
               sec_weapon_group = RANDOM_LONG(1, 2);

            if (sec_weapon_group == 1)  // shotguns
            {
               int weapon = RANDOM_LONG(1, 2);
               
               if (weapon == 1)
                  pBot->secondary_weapon = FLF_WEAPON_SPAS12;
               else
                  pBot->secondary_weapon = FLF_WEAPON_RS202M2;
            }
            else if (sec_weapon_group == 2)  // submachine
            {
               int weapon = RANDOM_LONG(1, 4);

               if (weapon == 1)
                  pBot->secondary_weapon = FLF_WEAPON_MP5A2;
               else if (weapon == 2)
                  pBot->secondary_weapon = FLF_WEAPON_MP5SD;
               else if (weapon == 3)
                  pBot->secondary_weapon = FLF_WEAPON_MAC10;
               else
                  pBot->secondary_weapon = FLF_WEAPON_UMP45;
            }
            else if (sec_weapon_group == 3)  // rifles & heavyweapons
            {
               if (RANDOM_LONG(1, 100) <= 50)
               {
                  int weapon = RANDOM_LONG(1, 3);  // rifles

                  if (weapon == 1)
                     pBot->secondary_weapon = FLF_WEAPON_M4;
                  else if (weapon == 2)
                     pBot->secondary_weapon = FLF_WEAPON_FAMAS;
                  else
                     pBot->secondary_weapon = FLF_WEAPON_AK5;
               }
               else  // heavy weapons
               {
                  pBot->secondary_weapon = FLF_WEAPON_HK21;
               }
            }
         }

         int pistol = RANDOM_LONG(1, 2);

         if (pistol == 1)
            strcpy(c_item, "26");  // mk23
         else
            strcpy(c_item, "23");  // beretta

         FakeClientCommand(pEdict, "pistols", c_item, NULL);

         return;
      }

      if (pBot->start_action == MSG_FLF_WEAPON_SELECT)
      {
         int weapon_class;

         pBot->start_action = MSG_FLF_IDLE;  // switch back to idle

         if (pBot->primary_weapon)
            weapon_class = pBot->primary_weapon;
         else
            weapon_class = pBot->secondary_weapon;

         if ((weapon_class == FLF_WEAPON_SPAS12) ||  // shotguns
             (weapon_class == FLF_WEAPON_RS202M2))
            strcpy(c_item, "shotgun");
         else if ((weapon_class == FLF_WEAPON_MP5A2) ||  // submachine
                  (weapon_class == FLF_WEAPON_MP5SD) ||
                  (weapon_class == FLF_WEAPON_MAC10) ||
                  (weapon_class == FLF_WEAPON_UMP45))
            strcpy(c_item, "submachine");
         else if ((weapon_class == FLF_WEAPON_M4) ||
                  (weapon_class == FLF_WEAPON_FAMAS) ||
                  (weapon_class == FLF_WEAPON_AK5) ||
                  (weapon_class == FLF_WEAPON_MSG90) ||
                  (weapon_class == FLF_WEAPON_SAKO))
            strcpy(c_item, "rifles");
         else
            strcpy(c_item, "heavyweapons");

         FakeClientCommand(pEdict, "wpnclass", c_item, NULL);

         return;
      }

      if (pBot->start_action == MSG_FLF_SHOTGUN_SELECT)
      {
         int weapon_class;

         pBot->start_action = MSG_FLF_IDLE;  // switch back to idle

         if (pBot->primary_weapon)
         {
            weapon_class = pBot->primary_weapon;
            pBot->primary_weapon = 0;
         }
         else
         {
            weapon_class = pBot->secondary_weapon;

            // bot has now joined the game (doesn't need to be started)
            pBot->not_started = 0;
         }

         sprintf(c_item, "%d", weapon_class);

         FakeClientCommand(pEdict, "shotgun", c_item, NULL);

         return;
      }

      if (pBot->start_action == MSG_FLF_SUBMACHINE_SELECT)
      {
         int weapon_class;

         pBot->start_action = MSG_FLF_IDLE;  // switch back to idle

         if (pBot->primary_weapon)
         {
            weapon_class = pBot->primary_weapon;
            pBot->primary_weapon = 0;
         }
         else
         {
            weapon_class = pBot->secondary_weapon;

            // bot has now joined the game (doesn't need to be started)
            pBot->not_started = 0;
         }

         sprintf(c_item, "%d", weapon_class);

         FakeClientCommand(pEdict, "submach", c_item, NULL);

         return;
      }

      if (pBot->start_action == MSG_FLF_RIFLE_SELECT)
      {
         int weapon_class;

         pBot->start_action = MSG_FLF_IDLE;  // switch back to idle

         if (pBot->primary_weapon)
         {
            weapon_class = pBot->primary_weapon;
            pBot->primary_weapon = 0;
         }
         else
         {
            weapon_class = pBot->secondary_weapon;

            // bot has now joined the game (doesn't need to be started)
            pBot->not_started = 0;
         }

         sprintf(c_item, "%d", weapon_class);

         FakeClientCommand(pEdict, "rifles", c_item, NULL);

         return;
      }

      if (pBot->start_action == MSG_FLF_HEAVYWEAPONS_SELECT)
      {
         int weapon_class;

         pBot->start_action = MSG_FLF_IDLE;  // switch back to idle

         if (pBot->primary_weapon)
         {
            weapon_class = pBot->primary_weapon;
            pBot->primary_weapon = 0;
         }
         else
         {
            weapon_class = pBot->secondary_weapon;

            // bot has now joined the game (doesn't need to be started)
            pBot->not_started = 0;
         }

         sprintf(c_item, "%d", weapon_class);

         FakeClientCommand(pEdict, "heavyweapons", c_item, NULL);

         return;
      }
   }
   else if (mod_id == DMC_DLL)
   {
      FakeClientCommand(pEdict, "_firstspawn", NULL, NULL);

      pBot->not_started = 0;
   }
   else
   {
      // otherwise, don't need to do anything to start game...
      pBot->not_started = 0;
   }
}

