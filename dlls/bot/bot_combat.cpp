// botman's Half-Life bot example
//
// http://planethalflife.com/botman/
//
// bot_combat.cpp
//

#include "extdll.h"
#include "util.h"
#include "client.h"
#include "cbase.h"
#include "player.h"
#include "items.h"
#include "effects.h"
#include "weapons.h"
#include "soundent.h"
#include "gamerules.h"
#include "animation.h"

#include "bot.h"


extern int f_Observer;  // flag to indicate if player is in observer mode

// weapon firing delay based on skill (min and max delay for each weapon)
float primary_fire_delay[WEAPON_SNARK+1][5][2] = {
   // WEAPON_NONE - NOT USED
   {{0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}},
   // WEAPON_CROWBAR
   {{0.0, 0.1}, {0.2, 0.3}, {0.3, 0.5}, {0.4, 0.6}, {0.6, 1.0}},
   // WEAPON_GLOCK (9mm)
   {{0.0, 0.1}, {0.1, 0.2}, {0.2, 0.3}, {0.3, 0.4}, {0.4, 0.5}},
   // WEAPON_PYTHON (357)
   {{0.0, 0.25}, {0.2, 0.5}, {0.4, 0.8}, {1.0, 1.3}, {1.5, 2.0}},
   // WEAPON_MP5 (9mmAR)
   {{0.0, 0.1}, {0.1, 0.3}, {0.3, 0.5}, {0.4, 0.6}, {0.5, 0.8}},
   // WEAPON_CHAINGUN - NOT USED
   {{0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}},
   // WEAPON_CROSSBOW
   {{0.0, 0.25}, {0.2, 0.4}, {0.5, 0.7}, {0.8, 1.0}, {1.0, 1.3}},
   // WEAPON_SHOTGUN
   {{0.0, 0.25}, {0.2, 0.5}, {0.4, 0.8}, {0.6, 1.2}, {0.8, 2.0}},
   // WEAPON_RPG
   {{1.0, 3.0}, {2.0, 4.0}, {3.0, 5.0}, {4.0, 6.0}, {5.0, 7.0}},
   // WEAPON_GAUSS
   {{0.0, 0.1}, {0.2, 0.3}, {0.3, 0.5}, {0.5, 0.8}, {1.0, 1.2}},
   // WEAPON_EGON
   {{0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}},
   // WEAPON_HORNETGUN
   {{0.0, 0.1}, {0.25, 0.4}, {0.4, 0.7}, {0.6, 1.0}, {1.0, 1.5}},
   // WEAPON_HANDGRENADE
   {{1.0, 1.4}, {1.4, 2.0}, {1.8, 2.6}, {2.0, 3.0}, {2.5, 3.8}},
   // WEAPON_TRIPMINE
   {{0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}},
   // WEAPON_SATCHEL
   {{0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}},
   // WEAPON_SNARK
   {{0.0, 0.1}, {0.1, 0.2}, {0.2, 0.5}, {0.5, 0.7}, {0.6, 1.0}},
   };

float secondary_fire_delay[WEAPON_SNARK+1][5][2] = {
   // WEAPON_NONE - NOT USED
   {{0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}},
   // WEAPON_CROWBAR - Not applicable
   {{0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}},
   // WEAPON_GLOCK (9mm)
   {{0.0, 0.1}, {0.0, 0.1}, {0.1, 0.2}, {0.1, 0.2}, {0.2, 0.4}},
   // WEAPON_PYTHON (357)
   {{0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}},
   // WEAPON_MP5 (9mmAR)
   {{0.0, 0.3}, {0.5, 0.8}, {0.7, 1.0}, {1.0, 1.6}, {1.4, 2.0}},
   // WEAPON_CHAINGUN - NOT USED
   {{0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}},
   // WEAPON_CROSSBOW
   {{0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}},
   // WEAPON_SHOTGUN
   {{0.0, 0.25}, {0.2, 0.5}, {0.4, 0.8}, {0.6, 1.2}, {0.8, 2.0}},
   // WEAPON_RPG - Not applicable
   {{0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}},
   // WEAPON_GAUSS
   {{0.2, 0.5}, {0.3, 0.7}, {0.5, 1.0}, {0.8, 1.5}, {1.0, 2.0}},
   // WEAPON_EGON - Not applicable
   {{0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}},
   // WEAPON_HORNETGUN
   {{0.0, 0.1}, {0.2, 0.3}, {0.3, 0.5}, {0.5, 0.8}, {0.7, 1.2}},
   // WEAPON_HANDGRENADE - Not applicable
   {{0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}},
   // WEAPON_TRIPMINE - Not applicable
   {{0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}},
   // WEAPON_SATCHEL
   {{0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}},
   // WEAPON_SNARK - Not applicable
   {{0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}}
   };   

ammo_check_t ammo_check[] = {
   {"ammo_glockclip", "9mm", _9MM_MAX_CARRY},
   {"ammo_9mmclip", "9mm", _9MM_MAX_CARRY},
   {"ammo_9mmAR", "9mm", _9MM_MAX_CARRY},
   {"ammo_9mmbox", "9mm", _9MM_MAX_CARRY},
   {"ammo_mp5clip", "9mm", _9MM_MAX_CARRY},
   {"ammo_chainboxclip", "9mm", _9MM_MAX_CARRY},
   {"ammo_mp5grenades", "ARgrenades", M203_GRENADE_MAX_CARRY},
   {"ammo_ARgrenades", "ARgrenades", M203_GRENADE_MAX_CARRY},
   {"ammo_buckshot", "buckshot", BUCKSHOT_MAX_CARRY},
   {"ammo_crossbow", "bolts", BOLT_MAX_CARRY},
   {"ammo_357", "357", _357_MAX_CARRY},
   {"ammo_rpgclip", "rockets", ROCKET_MAX_CARRY},
   {"ammo_egonclip", "uranium", URANIUM_MAX_CARRY},
   {"ammo_gaussclip", "uranium", URANIUM_MAX_CARRY},
   {"", 0, 0}};

// sounds for Bot taunting after a kill...
char barney_taunt[][30] = { BA_TNT1, BA_TNT2, BA_TNT3, BA_TNT4, BA_TNT5 };
char scientist_taunt[][30] = { SC_TNT1, SC_TNT2, SC_TNT3, SC_TNT4, SC_TNT5 };


CBaseEntity * CBot::BotFindEnemy( void )
{
   Vector vecEnd;
   static BOOL flag=TRUE;
   char sound[40];  // for taunting sounds

   if (pBotEnemy != NULL)  // does the bot already have an enemy?
   {
      vecEnd = pBotEnemy->EyePosition();

      // if the enemy is dead or has switched to botcam mode...
      if (!pBotEnemy->IsAlive() || (pBotEnemy->pev->effects & EF_NODRAW))
      {
         if (!pBotEnemy->IsAlive())  // is the enemy dead?, assume bot killed it
         {
            // the enemy is dead, jump for joy about 10% of the time
            if (RANDOM_LONG(1, 100) <= 10)
               pev->button |= IN_JUMP;

            // check if this player is not a bot (i.e. not fake client)...
            if (pBotEnemy->IsNetClient() && !IS_DEDICATED_SERVER())
            {
               // speak taunt sounds about 10% of the time
               if (RANDOM_LONG(1, 100) <= 10)
               {
                  if (bot_model == MODEL_BARNEY)
                     strcpy( sound, barney_taunt[RANDOM_LONG(0,4)] );
                  else if (bot_model == MODEL_SCIENTIST)
                     strcpy( sound, scientist_taunt[RANDOM_LONG(0,4)] );

                  EMIT_SOUND(ENT(pBotEnemy->pev), CHAN_VOICE, sound,
                             RANDOM_FLOAT(0.9, 1.0), ATTN_NORM);
               }
            }
         }

         // don't have an enemy anymore so null out the pointer...
         pBotEnemy = NULL;
      }
      else if (FInViewCone( &vecEnd ) && FVisible( vecEnd ))
      {
         // if enemy is still visible and in field of view, keep it

         // face the enemy
         Vector v_enemy = pBotEnemy->pev->origin - pev->origin;
         Vector bot_angles = UTIL_VecToAngles( v_enemy );

         pev->ideal_yaw = bot_angles.y;

         // check for wrap around of angle...
         if (pev->ideal_yaw > 180)
            pev->ideal_yaw -= 360;
         if (pev->ideal_yaw < -180)
            pev->ideal_yaw += 360;

         return (pBotEnemy);
      }
   }

   int i;
   float nearestdistance = 1000;
   CBaseEntity *pNewEnemy = NULL;

   // search the world for players...
   for (i = 1; i <= gpGlobals->maxClients; i++)
   {
      CBaseEntity *pPlayer = UTIL_PlayerByIndex( i );

      // skip invalid players and skip self (i.e. this bot)
      if ((!pPlayer) || (pPlayer == this))
         continue;

      // skip this player if not alive (i.e. dead or dying)
      if (pPlayer->pev->deadflag != DEAD_NO)
         continue;

      // skip players that are not bots in observer mode...
      if (pPlayer->IsNetClient() && f_Observer)
         continue;

      // skip players that are in botcam mode...
      if (pPlayer->pev->effects & EF_NODRAW)
         continue;

// BigGuy - START
      // is team play enabled?
      if (g_pGameRules->IsTeamplay())
      {
         // don't target your teammates if team names match...
         if (UTIL_TeamsMatch(g_pGameRules->GetTeamID(this),
                             g_pGameRules->GetTeamID(pPlayer)))
            continue;
      }
// BigGuy - END

      vecEnd = pPlayer->EyePosition();

      // see if bot can see the player...
      if (FInViewCone( &vecEnd ) && FVisible( vecEnd ))
      {
         float distance = (pPlayer->pev->origin - pev->origin).Length();
         if (distance < nearestdistance)
         {
            nearestdistance = distance;
            pNewEnemy = pPlayer;

            pBotUser = NULL;  // don't follow user when enemy found
         }
      }
   }

   if (pNewEnemy)
   {
      // face the enemy
      Vector v_enemy = pNewEnemy->pev->origin - pev->origin;
      Vector bot_angles = UTIL_VecToAngles( v_enemy );

      pev->ideal_yaw = bot_angles.y;

      // check for wrap around of angle...
      if (pev->ideal_yaw > 180)
         pev->ideal_yaw -= 360;
      if (pev->ideal_yaw < -180)
         pev->ideal_yaw += 360;
   }

   return (pNewEnemy);
}


Vector CBot::BotBodyTarget( CBaseEntity *pBotEnemy )
{
   Vector target;
   float f_distance;
   float f_scale;
   int d_x, d_y, d_z;

   f_distance = (pBotEnemy->pev->origin - pev->origin).Length();

   if (f_distance > 1000)
      f_scale = 1.0;
   else if (f_distance > 100)
      f_scale = f_distance / 1000.0;
   else
      f_scale = 0.1;

   switch (bot_skill)
   {
      case 0:
         // VERY GOOD, same as from CBasePlayer::BodyTarget (in player.h)
         target = pBotEnemy->Center() + pBotEnemy->pev->view_ofs * RANDOM_FLOAT( 0.5, 1.1 );
         d_x = 0;  // no offset
         d_y = 0;
         d_z = 0;
         break;
      case 1:
         // GOOD, offset a little for x, y, and z
         target = pBotEnemy->Center() + pBotEnemy->pev->view_ofs;
         d_x = RANDOM_FLOAT(-5, 5) * f_scale;
         d_y = RANDOM_FLOAT(-5, 5) * f_scale;
         d_z = RANDOM_FLOAT(-9, 9) * f_scale;
         break;
      case 2:
         // FAIR, offset somewhat for x, y, and z
         target = pBotEnemy->Center() + pBotEnemy->pev->view_ofs;
         d_x = RANDOM_FLOAT(-9, 9) * f_scale;
         d_y = RANDOM_FLOAT(-9, 9) * f_scale;
         d_z = RANDOM_FLOAT(-15, 15) * f_scale;
         break;
      case 3:
         // POOR, offset for x, y, and z
         target = pBotEnemy->Center() + pBotEnemy->pev->view_ofs;
         d_x = RANDOM_FLOAT(-16, 16) * f_scale;
         d_y = RANDOM_FLOAT(-16, 16) * f_scale;
         d_z = RANDOM_FLOAT(-20, 20) * f_scale;
         break;
      case 4:
         // BAD, offset lots for x, y, and z
         target = pBotEnemy->Center() + pBotEnemy->pev->view_ofs;
         d_x = RANDOM_FLOAT(-20, 20) * f_scale;
         d_y = RANDOM_FLOAT(-20, 20) * f_scale;
         d_z = RANDOM_FLOAT(-27, 27) * f_scale;
         break;
   }

   target = target + Vector(d_x, d_y, d_z);

   return target;
}


void CBot::BotWeaponInventory( void )
{
   int i;

   // initialize the elements of the weapons arrays...
   for (i = 0; i < MAX_WEAPONS; i++)
   {
      weapon_ptr[i] = NULL;
      primary_ammo[i] = 0;
      secondary_ammo[i] = 0;
   }

   // find out which weapons the bot is carrying...
   for (i = 0; i < MAX_ITEM_TYPES; i++)
   {
      CBasePlayerItem *pItem = NULL;

      if (m_rgpPlayerItems[i])
      {
         pItem = m_rgpPlayerItems[i];
         while (pItem)
         {
            weapon_ptr[pItem->m_iId] = pItem;  // store pointer to item

            pItem = pItem->m_pNext;
         }
      }
   }

   // find out how much ammo of each type the bot is carrying...
   for (i = 0; i < MAX_AMMO_SLOTS; i++)
   {
      if (!CBasePlayerItem::AmmoInfoArray[i].pszName)
         continue;

      if (strcmp("9mm", CBasePlayerItem::AmmoInfoArray[i].pszName) == 0)
      {
         primary_ammo[WEAPON_GLOCK] = m_rgAmmo[i];
         primary_ammo[WEAPON_MP5] = m_rgAmmo[i];
      }
      else if (strcmp("357", CBasePlayerItem::AmmoInfoArray[i].pszName) == 0)
      {
         primary_ammo[WEAPON_PYTHON] = m_rgAmmo[i];
      }
      else if (strcmp("ARgrenades", CBasePlayerItem::AmmoInfoArray[i].pszName) == 0)
         secondary_ammo[WEAPON_MP5] = m_rgAmmo[i];
      else if (strcmp("bolts", CBasePlayerItem::AmmoInfoArray[i].pszName) == 0)
      {
         primary_ammo[WEAPON_CROSSBOW] = m_rgAmmo[i];
      }
      else if (stricmp("buckshot", CBasePlayerItem::AmmoInfoArray[i].pszName) == 0)
      {
         primary_ammo[WEAPON_SHOTGUN] = m_rgAmmo[i];
      }
      else if (stricmp("rockets", CBasePlayerItem::AmmoInfoArray[i].pszName) == 0)
         primary_ammo[WEAPON_RPG] = m_rgAmmo[i];
      else if (strcmp("uranium", CBasePlayerItem::AmmoInfoArray[i].pszName) == 0)
      {
         primary_ammo[WEAPON_GAUSS] = m_rgAmmo[i];
         primary_ammo[WEAPON_EGON] = m_rgAmmo[i];
      }
      else if (stricmp("Hornets", CBasePlayerItem::AmmoInfoArray[i].pszName) == 0)
         primary_ammo[WEAPON_HORNETGUN] = m_rgAmmo[i];
      else if (stricmp("Hand Grenade", CBasePlayerItem::AmmoInfoArray[i].pszName) == 0)
         primary_ammo[WEAPON_HANDGRENADE] = m_rgAmmo[i];
      else if (stricmp("Trip Mine", CBasePlayerItem::AmmoInfoArray[i].pszName) == 0)
         primary_ammo[WEAPON_TRIPMINE] = m_rgAmmo[i];
      else if (stricmp("Satchel Charge", CBasePlayerItem::AmmoInfoArray[i].pszName) == 0)
         primary_ammo[WEAPON_SATCHEL] = m_rgAmmo[i];
      else if (stricmp("Snarks", CBasePlayerItem::AmmoInfoArray[i].pszName) == 0)
         primary_ammo[WEAPON_SNARK] = m_rgAmmo[i];
   }

}


// specifing a weapon_choice allows you to choose the weapon the bot will
// use (assuming enough ammo exists for that weapon)
// BotFireWeapon will return TRUE if weapon was fired, FALSE otherwise
// primary is used to indicate whether you want primary or secondary fire
// if you have specified a weapon using weapon_choice

BOOL CBot::BotFireWeapon( Vector v_enemy_origin, int weapon_choice, BOOL primary )
{
   CBasePlayerItem *new_weapon;
   BOOL enemy_below;

   // is it time to check weapons inventory yet?
   if (f_weapon_inventory_time <= gpGlobals->time)
   {
      // check weapon and ammo inventory then update check time...
      BotWeaponInventory();
      f_weapon_inventory_time = gpGlobals->time + 1.0;
   }

   Vector v_enemy = v_enemy_origin - GetGunPosition( );

   float distance = v_enemy.Length();  // how far away is the enemy?

   // is enemy at least 45 units below bot? (for handgrenades and snarks)
   if (v_enemy_origin.z < (pev->origin.z - 45))
      enemy_below = TRUE;
   else
      enemy_below = FALSE;

   // if bot is carrying the crowbar...
   if (pev->weapons & (1<<WEAPON_CROWBAR))
   {
      // if close to enemy, and skill level is 1, 2 or 3, use the crowbar
      if (((distance <= 40) && (bot_skill <= 2) && (weapon_choice == 0)) ||
          (weapon_choice == WEAPON_CROWBAR))
      {
         new_weapon = weapon_ptr[WEAPON_CROWBAR];

         // check if the bot isn't already using this item...
         if (m_pActiveItem != new_weapon)
            SelectItem("weapon_crowbar");  // select the crowbar

         pev->button |= IN_ATTACK;  // use primary attack (whack! whack!)

         // set next time to "shoot"
         f_shoot_time = gpGlobals->time + 0.3 + 
            RANDOM_FLOAT(primary_fire_delay[WEAPON_CROWBAR][bot_skill][0],
                         primary_fire_delay[WEAPON_CROWBAR][bot_skill][1]);
         return TRUE;
      }
   }

   // if bot is carrying any hand grenades and enemy is below bot...
   if ((pev->weapons & (1<<WEAPON_HANDGRENADE)) && (enemy_below))
   {
      long use_grenade = RANDOM_LONG(1,100);

      // use hand grenades about 30% of the time...
      if (((distance > 250) && (distance < 750) &&
           (weapon_choice == 0) && (use_grenade <= 30)) ||
          (weapon_choice == WEAPON_HANDGRENADE))
      {
// BigGuy - START
         new_weapon = weapon_ptr[WEAPON_HANDGRENADE];

         // check if the bot isn't already using this item...
         if (m_pActiveItem != new_weapon)
            SelectItem("weapon_handgrenade");  // select the hand grenades

         pev->button |= IN_ATTACK;  // use primary attack (boom!)

         // set next time to "shoot"
         f_shoot_time = gpGlobals->time + 0.1 + 
            RANDOM_FLOAT(primary_fire_delay[WEAPON_HANDGRENADE][bot_skill][0],
                         primary_fire_delay[WEAPON_HANDGRENADE][bot_skill][1]);
         return TRUE;
// BigGuy - END
      }
   }

   // if bot is carrying any snarks (can't use underwater) and enemy is below bot...
   if ((pev->weapons & (1<<WEAPON_SNARK)) && (pev->waterlevel != 3) &&
       (enemy_below))
   {
      long use_snark = RANDOM_LONG(1,100);

      // use snarks about 50% of the time...
      if (((distance > 150) && (distance < 500) &&
           (weapon_choice == 0) && (use_snark <= 50)) ||
          (weapon_choice == WEAPON_SNARK))
      {
// BigGuy - START
         new_weapon = weapon_ptr[WEAPON_SNARK];

         // check if the bot isn't already using this item...
         if (m_pActiveItem != new_weapon)
            SelectItem("weapon_snark");  // select the "squeak grenades"

         pev->button |= IN_ATTACK;  // use primary attack (eek! eek!)

         // set next time to "shoot"
         f_shoot_time = gpGlobals->time + 0.1 + 
            RANDOM_FLOAT(primary_fire_delay[WEAPON_SNARK][bot_skill][0],
                         primary_fire_delay[WEAPON_SNARK][bot_skill][1]);
         return TRUE;
// BigGuy - END
      }
   }

   // if the bot is carrying the egon gun (can't use underwater)...
   if ((pev->weapons & (1<<WEAPON_EGON)) && (pev->waterlevel != 3))
   {
      if ((weapon_choice == 0) || (weapon_choice == WEAPON_EGON))
      {
         new_weapon = weapon_ptr[WEAPON_EGON];

         // check if the bot has any ammo left for this weapon...
         if (primary_ammo[WEAPON_EGON] > 0)
         {
            // check if the bot isn't already using this item...
            if (m_pActiveItem != new_weapon)
               SelectItem("weapon_egon");  // select the egon gun

            pev->button |= IN_ATTACK;  // use primary attack (bang! bang!)

            // set next time to shoot
            f_shoot_time = gpGlobals->time;

            return TRUE;
         }
      }
   }

   // if the bot is carrying the gauss gun (can't use underwater)...
   if ((pev->weapons & (1<<WEAPON_GAUSS)) && (pev->waterlevel != 3))
   {
      if ((weapon_choice == 0) || (weapon_choice == WEAPON_GAUSS))
      {
         new_weapon = weapon_ptr[WEAPON_GAUSS];

         // check if the bot has any ammo left for this weapon...
         if (primary_ammo[WEAPON_GAUSS] > 1)
         {
            // check if the bot isn't already using this item...
            if (m_pActiveItem != new_weapon)
               SelectItem("weapon_gauss");  // select the gauss gun

            long use_secondary = RANDOM_LONG(1,100);

            // are we charging the gauss gun?
            if (f_fire_gauss > 0)
            {
               // is it time to fire the charged gauss gun?
               if (f_fire_gauss >= gpGlobals->time)
               {
                  // we DON'T set pev->button here to release the secondary
                  // fire button which will fire the charged gauss gun

                  f_fire_gauss = -1;  // -1 means not charging gauss gun

                  // set next time to shoot
                  f_shoot_time = gpGlobals->time + 1.0 +
                     RANDOM_FLOAT(secondary_fire_delay[WEAPON_GAUSS][bot_skill][0],
                                  secondary_fire_delay[WEAPON_GAUSS][bot_skill][1]);
               }
               else
               {
                  pev->button |= IN_ATTACK2;  // charge the gauss gun
                  f_shoot_time = gpGlobals->time;  // keep charging
               }
            }
            else if ((use_secondary <= 20) &&
                     (primary_ammo[WEAPON_GAUSS] >= 10))
            {
               // release secondary fire in 0.5 seconds...
               f_fire_gauss = gpGlobals->time + 0.5;

               pev->button |= IN_ATTACK2;  // charge the gauss gun
               f_shoot_time = gpGlobals->time; // keep charging
            }
            else
            {
               pev->button |= IN_ATTACK;  // use primary attack (bang! bang!)

               // set next time to shoot
               f_shoot_time = gpGlobals->time + 0.2 +
                  RANDOM_FLOAT(primary_fire_delay[WEAPON_GAUSS][bot_skill][0],
                               primary_fire_delay[WEAPON_GAUSS][bot_skill][1]);
            }

            return TRUE;
         }
      }
   }

   // if the bot is carrying the shotgun (can't use underwater)...
   if ((pev->weapons & (1<<WEAPON_SHOTGUN)) && (pev->waterlevel != 3))
   {
      // if close enough for good shotgun blasts...
      if (((distance > 30) && (distance < 150) && (weapon_choice == 0)) ||
          (weapon_choice == WEAPON_SHOTGUN))
      {
         new_weapon = weapon_ptr[WEAPON_SHOTGUN];

         // check if the bot has any ammo left for this weapon...
         if (primary_ammo[WEAPON_SHOTGUN] > 0)
         {
            // check if the bot isn't already using this item...
            if (m_pActiveItem != new_weapon)
               SelectItem("weapon_shotgun");  // select the shotgun

            long use_secondary = RANDOM_LONG(1,100);

            // use secondary attack about 30% of the time
            if ((use_secondary <= 30) && (primary_ammo[WEAPON_SHOTGUN] >= 2))
            {
// BigGuy - START
               pev->button |= IN_ATTACK2;  // use secondary attack (bang! bang!)

               // set next time to shoot
               f_shoot_time = gpGlobals->time + 1.5 +
                  RANDOM_FLOAT(secondary_fire_delay[WEAPON_SHOTGUN][bot_skill][0],
                               secondary_fire_delay[WEAPON_SHOTGUN][bot_skill][1]);
            }
// BigGuy - END
            else
            {
               pev->button |= IN_ATTACK;  // use primary attack (bang! bang!)

               // set next time to shoot
               f_shoot_time = gpGlobals->time + 0.75 +
                  RANDOM_FLOAT(primary_fire_delay[WEAPON_SHOTGUN][bot_skill][0],
                               primary_fire_delay[WEAPON_SHOTGUN][bot_skill][1]);
            }

            return TRUE;
         }
      }
   }

   // if the bot is carrying the 357/PYTHON, (can't use underwater)...
   if ((pev->weapons & (1<<WEAPON_PYTHON)) && (pev->waterlevel != 3))
   {
      // if close enough for 357 shot...
      if (((distance > 30) && (distance < 700) && (weapon_choice == 0)) ||
          (weapon_choice == WEAPON_PYTHON))
      {
         new_weapon = weapon_ptr[WEAPON_PYTHON];

         // check if the bot has any ammo left for this weapon...
         if (primary_ammo[WEAPON_PYTHON] > 0)
         {
            // check if the bot isn't already using this item...
            if (m_pActiveItem != new_weapon)
               SelectItem("weapon_357");  // select the 357 python

            pev->button |= IN_ATTACK;  // use primary attack (bang! bang!)

            // set next time to shoot
            f_shoot_time = gpGlobals->time + 0.75 +
               RANDOM_FLOAT(primary_fire_delay[WEAPON_PYTHON][bot_skill][0],
                            primary_fire_delay[WEAPON_PYTHON][bot_skill][1]);

            return TRUE;
         }
      }
   }

   // if the bot is carrying the hornet gun...
   if (pev->weapons & (1<<WEAPON_HORNETGUN))
   {
      // if close enough for hornet gun...
      if (((distance > 30) && (distance < 1000) && (weapon_choice == 0)) ||
          (weapon_choice == WEAPON_HORNETGUN))
      {
         new_weapon = weapon_ptr[WEAPON_HORNETGUN];

         // check if the bot has any ammo left for this weapon...
         if (primary_ammo[WEAPON_HORNETGUN] > 0)
         {
            // check if the bot isn't already using this item...
            if (m_pActiveItem != new_weapon)
               SelectItem("weapon_hornetgun");  // select the hornet gun

            long use_secondary = RANDOM_LONG(1,100);

            // use secondary attack about 50% of the time (if fully reloaded)
            if ((use_secondary <= 50) &&
                (primary_ammo[WEAPON_HORNETGUN] >= HORNET_MAX_CARRY))
            {
// BigGuy - START
               pev->button |= IN_ATTACK2;  // use secondary attack (buzz! buzz!)

               // set next time to shoot
               f_shoot_time = gpGlobals->time + 0.1 +
                  RANDOM_FLOAT(secondary_fire_delay[WEAPON_HORNETGUN][bot_skill][0],
                               secondary_fire_delay[WEAPON_HORNETGUN][bot_skill][1]);
// BigGuy - END
            }
            else
            {
               pev->button |= IN_ATTACK;  // use primary attack (buzz! buzz!)

               // set next time to shoot
               f_shoot_time = gpGlobals->time + 0.25 +
                  RANDOM_FLOAT(primary_fire_delay[WEAPON_HORNETGUN][bot_skill][0],
                               primary_fire_delay[WEAPON_HORNETGUN][bot_skill][1]);
            }

            return TRUE;
         }
      }
   }

   // if the bot is carrying the MP5 (can't use underwater)...
   if ((pev->weapons & (1<<WEAPON_MP5)) && (pev->waterlevel != 3))
   {
      long use_secondary = RANDOM_LONG(1,100);

      // use secondary attack about 10% of the time...
      if (((distance > 300) && (distance < 600) &&
           (weapon_choice == 0) && (use_secondary <= 10)) ||
          ((weapon_choice == WEAPON_MP5) && (primary == FALSE)))
      {
         // at some point we need to fire upwards in the air slightly
         // for long distance kills.  for right now, just fire the
         // grenade at the poor sucker.

// BigGuy - START
         new_weapon = weapon_ptr[WEAPON_MP5];

         // check if the bot has any ammo left for this weapon...
         if (secondary_ammo[WEAPON_MP5] > 0)
         {
            // check if the bot isn't already using this item...
            if (m_pActiveItem != new_weapon)
               SelectItem("weapon_9mmAR");  // select the 9mmAR (MP5)

            pev->button |= IN_ATTACK2;  // use secodnary attack (boom!)

            // set next time to shoot
            f_shoot_time = gpGlobals->time + 1.0 +
               RANDOM_FLOAT(secondary_fire_delay[WEAPON_MP5][bot_skill][0],
                            secondary_fire_delay[WEAPON_MP5][bot_skill][1]);

            return TRUE;
         }
// BigGuy - END
      }

      // if close enough for good MP5 shot...
      if (((distance < 250) && (weapon_choice == 0)) ||
          (weapon_choice == WEAPON_MP5))
      {
         new_weapon = weapon_ptr[WEAPON_MP5];

         // check if the bot has any ammo left for this weapon...
         if (primary_ammo[WEAPON_MP5] > 0)
         {
            // check if the bot isn't already using this item...
            if (m_pActiveItem != new_weapon)
               SelectItem("weapon_9mmAR");  // select the 9mmAR (MP5)

            pev->button |= IN_ATTACK;  // use primary attack (bang! bang!)

            // set next time to shoot
            f_shoot_time = gpGlobals->time + 0.1 +
               RANDOM_FLOAT(primary_fire_delay[WEAPON_MP5][bot_skill][0],
                            primary_fire_delay[WEAPON_MP5][bot_skill][1]);

            return TRUE;
         }
      }
   }

   // if the bot is carrying the crossbow...
   if (pev->weapons & (1<<WEAPON_CROSSBOW))
   {
      // if bot is not too close for crossbow and not too far...
      if (((distance > 100) && (distance < 1000) && (weapon_choice == 0)) ||
          (weapon_choice == WEAPON_CROSSBOW))
      {
         new_weapon = weapon_ptr[WEAPON_CROSSBOW];

         // check if the bot has any ammo left for this weapon...
         if (primary_ammo[WEAPON_CROSSBOW] > 0)
         {
            // check if the bot isn't already using this item...
            if (m_pActiveItem != new_weapon)
               SelectItem("weapon_crossbow");  // select the crossbow

            pev->button |= IN_ATTACK;  // use primary attack (bang! bang!)

            // set next time to shoot
            f_shoot_time = gpGlobals->time + 0.75 +
               RANDOM_FLOAT(primary_fire_delay[WEAPON_CROSSBOW][bot_skill][0],
                            primary_fire_delay[WEAPON_CROSSBOW][bot_skill][1]);

            return TRUE;
         }
      }
   }

   // if the bot is carrying the RPG...
   if (pev->weapons & (1<<WEAPON_RPG))
   {
      // don't use the RPG unless the enemy is pretty far away...
      if (((distance > 300) && (weapon_choice == 0)) ||
          (weapon_choice == WEAPON_RPG))
      {
         new_weapon = weapon_ptr[WEAPON_RPG];

         // check if the bot has any ammo left for this weapon...
         if (primary_ammo[WEAPON_RPG] > 0)
         {
            // check if the bot isn't already using this item...
            if (m_pActiveItem != new_weapon)
               SelectItem("weapon_rpg");  // select the RPG rocket launcher

            pev->button |= IN_ATTACK;  // use primary attack (bang! bang!)

            // set next time to shoot
            f_shoot_time = gpGlobals->time + 1.5 +
               RANDOM_FLOAT(primary_fire_delay[WEAPON_RPG][bot_skill][0],
                            primary_fire_delay[WEAPON_RPG][bot_skill][1]);

            return TRUE;
         }
      }
   }

   // if the bot is carrying the 9mm glock...
   if (pev->weapons & (1<<WEAPON_GLOCK))
   {
      // if nothing else was selected, try the good ol' 9mm glock...
      if (((distance < 1200) && (weapon_choice == 0)) ||
          (weapon_choice == WEAPON_GLOCK))
      {
         new_weapon = weapon_ptr[WEAPON_GLOCK];

         // check if the bot has any ammo left for this weapon...
         if (primary_ammo[WEAPON_GLOCK] > 0)
         {
            // check if the bot isn't already using this item...
            if (m_pActiveItem != new_weapon)
               SelectItem("weapon_9mmhandgun");  // select the trusty 9mm glock

            long use_secondary = RANDOM_LONG(1,100);

            // use secondary attack about 30% of the time
            if (use_secondary <= 30)
            {
// BigGuy - START
               pev->button |= IN_ATTACK2;  // use secondary attack (bang! bang!)

               // set next time to shoot
               f_shoot_time = gpGlobals->time + 0.2 +
                  RANDOM_FLOAT(secondary_fire_delay[WEAPON_GLOCK][bot_skill][0],
                               secondary_fire_delay[WEAPON_GLOCK][bot_skill][1]);
// BigGuy - END
            }
            else
            {
               pev->button |= IN_ATTACK;  // use primary attack (bang! bang!)

               // set next time to shoot
               f_shoot_time = gpGlobals->time + 0.3 +
                  RANDOM_FLOAT(primary_fire_delay[WEAPON_GLOCK][bot_skill][0],
                               primary_fire_delay[WEAPON_GLOCK][bot_skill][1]);
            }

            return TRUE;
         }
      }
   }

   // didn't have any available weapons or ammo, return FALSE
   return FALSE;
}


void CBot::BotShootAtEnemy( void )
{
   float f_distance;

   // aim for the head and/or body
   Vector v_enemy = BotBodyTarget( pBotEnemy ) - GetGunPosition();

   pev->v_angle = UTIL_VecToAngles( v_enemy );

   pev->angles.x = 0;
   pev->angles.y = pev->v_angle.y;
   pev->angles.z = 0;

   pev->ideal_yaw = pev->v_angle.y;

   // check for wrap around of angle...
   if (pev->ideal_yaw > 180)
      pev->ideal_yaw -= 360;
   if (pev->ideal_yaw < -180)
      pev->ideal_yaw += 360;

   pev->v_angle.x = -pev->v_angle.x;  //adjust pitch to point gun

   // is it time to shoot yet?
   if (f_shoot_time <= gpGlobals->time)
   {
      // select the best weapon to use at this distance and fire...
      BotFireWeapon( pBotEnemy->pev->origin );
   }

   v_enemy.z = 0;  // ignore z component (up & down)

   f_distance = v_enemy.Length();  // how far away is the enemy scum?

   if (f_distance > 200)      // run if distance to enemy is far
      f_move_speed = f_max_speed;
   else if (f_distance > 20)  // walk if distance is closer
      f_move_speed = f_max_speed / 2;
   else                     // don't move if close enough
      f_move_speed = 0.0;
}



