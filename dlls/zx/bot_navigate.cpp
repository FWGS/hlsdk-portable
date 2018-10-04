//
// HPB bot - botman's High Ping Bastard bot
//
// (http://planethalflife.com/botman/)
//
// bot_navigate.cpp
//

#include "extdll.h"
#include "util.h"
#include "cbase.h"

#include "bot.h"
#include "bot_func.h"
#include "bot_weapons.h"
#include "waypoint.h"


extern int mod_id;
extern WAYPOINT waypoints[MAX_WAYPOINTS];
extern int num_waypoints;  // number of waypoints currently in use
extern int team_allies[4];
extern edict_t *pent_info_ctfdetect;
extern bool is_team_play;
extern bool checked_teamplay;
extern FLAG_S flags[MAX_FLAGS];
extern int num_flags;
extern bot_weapon_t weapon_defs[MAX_WEAPONS];
extern edict_t *holywars_saint;
extern int halo_status;
extern int holywars_gamemode;

// SET THIS UP BASED ON MOD!!!
int max_drop_height = 800;

static FILE *fp;
char welcome_msg[80];
int x_welcome_msg_len = 42;
unsigned char x_welcome_msg[] = {
'H'^0x5a, 'P'^0xa5, 'B'^0x5a, ' '^0xa5, 'b'^0x5a, 'o'^0xa5, 't'^0x5a, ' '^0xa5,
'-'^0x5a, ' '^0xa5, 'h'^0x5a, 't'^0xa5, 't'^0x5a, 'p'^0xa5, ':'^0x5a, '/'^0xa5,
'/'^0x5a, 'p'^0xa5, 'l'^0x5a, 'a'^0xa5, 'n'^0x5a, 'e'^0xa5, 't'^0x5a, 'h'^0xa5,
'a'^0x5a, 'l'^0xa5, 'f'^0x5a, 'l'^0xa5, 'i'^0x5a, 'f'^0xa5, 'e'^0x5a, '.'^0xa5,
'c'^0x5a, 'o'^0xa5, 'm'^0x5a, '/'^0xa5, 'b'^0x5a, 'o'^0xa5, 't'^0x5a, 'm'^0xa5,
'a'^0x5a, 'n'^0xa5};


extern void BotCheckTeamplay(void);


void welcome_init(void)
{
   for (int i=0; i < x_welcome_msg_len; i++)
   {
      if ((i % 2) == 0)
         welcome_msg[i] = x_welcome_msg[i]^0x5a;
      else
         welcome_msg[i] = x_welcome_msg[i]^0xa5;
   }
   welcome_msg[x_welcome_msg_len] = 0;
}


void BotFixIdealPitch(edict_t *pEdict)
{
   // check for wrap around of angle...
   if (pEdict->v.idealpitch > 180)
      pEdict->v.idealpitch -= 360;

   if (pEdict->v.idealpitch < -180)
      pEdict->v.idealpitch += 360;
}


// returns the number of degrees left to turn toward ideal pitch...
float BotChangePitch( bot_t *pBot, float speed )
{
   edict_t *pEdict = pBot->pEdict;
   float ideal;
   float current;
   float current_180;  // current +/- 180 degrees
   float diff;

   // turn from the current v_angle pitch to the idealpitch by selecting
   // the quickest way to turn to face that direction
   
   current = pEdict->v.v_angle.x;

   ideal = pEdict->v.idealpitch;

   // find the difference in the current and ideal angle
   diff = fabs(current - ideal);

   speed = speed * pBot->f_frame_time;  // angles per second

   // check if difference is less than the max degrees per turn
   if (diff < speed)
      speed = diff;  // just need to turn a little bit (less than max)

   // here we have four cases, both angle positive, one positive and
   // the other negative, one negative and the other positive, or
   // both negative.  handle each case separately...

   if ((current >= 0) && (ideal >= 0))  // both positive
   {
      if (current > ideal)
         current -= speed;
      else
         current += speed;
   }
   else if ((current >= 0) && (ideal < 0))
   {
      current_180 = current - 180;

      if (current_180 > ideal)
         current += speed;
      else
         current -= speed;
   }
   else if ((current < 0) && (ideal >= 0))
   {
      current_180 = current + 180;
      if (current_180 > ideal)
         current += speed;
      else
         current -= speed;
   }
   else  // (current < 0) && (ideal < 0)  both negative
   {
      if (current > ideal)
         current -= speed;
      else
         current += speed;
   }

   // check for wrap around of angle...
   if (current > 180)
      current -= 360;
   if (current < -180)
      current += 360;

   pEdict->v.v_angle.x = current;

   return diff;  // return number of degrees left to turn
}


void BotFixIdealYaw(edict_t *pEdict)
{
   // check for wrap around of angle...
   if (pEdict->v.ideal_yaw > 180)
      pEdict->v.ideal_yaw -= 360;

   if (pEdict->v.ideal_yaw < -180)
      pEdict->v.ideal_yaw += 360;
}


// returns the number of degrees left to turn toward ideal yaw...
float BotChangeYaw( bot_t *pBot, float speed )
{
   edict_t *pEdict = pBot->pEdict;
   float ideal;
   float current;
   float current_180;  // current +/- 180 degrees
   float diff;

   // turn from the current v_angle yaw to the ideal_yaw by selecting
   // the quickest way to turn to face that direction

   current = pEdict->v.v_angle.y;

   ideal = pEdict->v.ideal_yaw;

   // find the difference in the current and ideal angle
   diff = fabs(current - ideal);

   // speed that we can turn during this frame...
   speed = speed * pBot->f_frame_time;

   // check if difference is less than the max degrees per turn
   if (diff < speed)
      speed = diff;  // just need to turn a little bit (less than max)

   // here we have four cases, both angle positive, one positive and
   // the other negative, one negative and the other positive, or
   // both negative.  handle each case separately...

   if ((current >= 0) && (ideal >= 0))  // both positive
   {
      if (current > ideal)
         current -= speed;
      else
         current += speed;
   }
   else if ((current >= 0) && (ideal < 0))
   {
      current_180 = current - 180;

      if (current_180 > ideal)
         current += speed;
      else
         current -= speed;
   }
   else if ((current < 0) && (ideal >= 0))
   {
      current_180 = current + 180;
      if (current_180 > ideal)
         current += speed;
      else
         current -= speed;
   }
   else  // (current < 0) && (ideal < 0)  both negative
   {
      if (current > ideal)
         current -= speed;
      else
         current += speed;
   }

   // check for wrap around of angle...
   if (current > 180)
      current -= 360;
   if (current < -180)
      current += 360;

   pEdict->v.v_angle.y = current;

   return diff;  // return number of degrees left to turn
}


bool BotFindWaypoint( bot_t *pBot )
{
   int index, select_index;
   int team;
   PATH *pPath = NULL;
   int path_index;
   float distance, min_distance[3];
   int min_index[3];

   edict_t *pEdict = pBot->pEdict;

   if (mod_id == FRONTLINE_DLL)
      team = pBot->defender;
   else
      team = UTIL_GetTeam(pEdict);

   for (index=0; index < 3; index++)
   {
      min_distance[index] = 9999.0;
      min_index[index] = -1;
   }

   index = WaypointFindPath(&pPath, &path_index, pBot->curr_waypoint_index, team);

   while (index != -1)
   {
      // if index is not a current or recent previous waypoint...
      if ((index != pBot->curr_waypoint_index) &&
          (index != pBot->prev_waypoint_index[0]) &&
          (index != pBot->prev_waypoint_index[1]) &&
          (index != pBot->prev_waypoint_index[2]) &&
          (index != pBot->prev_waypoint_index[3]) &&
          (index != pBot->prev_waypoint_index[4]))
      {
         // find the distance from the bot to this waypoint
         distance = (pEdict->v.origin - waypoints[index].origin).Length();

         if (distance < min_distance[0])
         {
            min_distance[2] = min_distance[1];
            min_index[2] = min_index[1];

            min_distance[1] = min_distance[0];
            min_index[1] = min_index[0];

            min_distance[0] = distance;
            min_index[0] = index;
         }
         else if (distance < min_distance [1])
         {
            min_distance[2] = min_distance[1];
            min_index[2] = min_index[1];

            min_distance[1] = distance;
            min_index[1] = index;
         }
         else if (distance < min_distance[2])
         {
            min_distance[2] = distance;
            min_index[2] = index;
         }
      }

      // find the next path to a waypoint
      index = WaypointFindPath(&pPath, &path_index, pBot->curr_waypoint_index, team);
   }

   select_index = -1;

   // about 20% of the time choose a waypoint at random
   // (don't do this any more often than every 10 seconds)

   if ((RANDOM_LONG(1, 100) <= 20) &&
       (pBot->f_random_waypoint_time <= gpGlobals->time))
   {
      pBot->f_random_waypoint_time = gpGlobals->time + 10.0;

      if (min_index[2] != -1)
         index = RANDOM_LONG(0, 2);
      else if (min_index[1] != -1)
         index = RANDOM_LONG(0, 1);
      else if (min_index[0] != -1)
         index = 0;
      else
         return FALSE;  // no waypoints found!

      select_index = min_index[index];
   }
   else
   {
      // use the closest waypoint that has been recently used
      select_index = min_index[0];
   }

   if (select_index != -1)  // was a waypoint found?
   {
      pBot->prev_waypoint_index[4] = pBot->prev_waypoint_index[3];
      pBot->prev_waypoint_index[3] = pBot->prev_waypoint_index[2];
      pBot->prev_waypoint_index[2] = pBot->prev_waypoint_index[1];
      pBot->prev_waypoint_index[1] = pBot->prev_waypoint_index[0];
      pBot->prev_waypoint_index[0] = pBot->curr_waypoint_index;

      pBot->curr_waypoint_index = select_index;
      pBot->waypoint_origin = waypoints[select_index].origin;

      pBot->f_waypoint_time = gpGlobals->time;

      return TRUE;
   }

   return FALSE;  // couldn't find a waypoint
}


bool BotHeadTowardWaypoint( bot_t *pBot )
{
   int i;
   Vector v_src, v_dest;
   TraceResult tr;
   int index;
   bool status;
   float waypoint_distance, min_distance;
   int team, skin;
   float distance;
   float pause_time = 0.0;
   edict_t *pent;
   bool bot_has_flag = FALSE;
   bool touching;

   edict_t *pEdict = pBot->pEdict;

   if (!checked_teamplay)  // check for team play...
      BotCheckTeamplay();

   // is team play enabled (or is it Counter-Strike)?
   if (is_team_play)
      team = UTIL_GetTeam(pEdict);
   else
      team = -1;  // not team play (all waypoints are valid for everyone)

   // check if the bot has been trying to get to this waypoint for a while...
   if ((pBot->f_waypoint_time + 5.0) < gpGlobals->time)
   {
      pBot->curr_waypoint_index = -1;  // forget about this waypoint
      pBot->waypoint_goal = -1;  // also forget about a goal
   }

   // check if a goal item exists...
   if (mod_id == TFC_DLL)
   {
      pent = NULL;

      while ((pent = UTIL_FindEntityByClassname( pent, "item_tfgoal" )) != NULL)
      {
         if (pent->v.owner == pEdict)  // is this bot carrying the item?
         {
            // we are carrying the flag/card/ball

            bot_has_flag = TRUE;

            break;  // break out of while loop
         }
         else if (FInViewCone( &pent->v.origin, pEdict ) &&  // can bot see it?
                  FVisible( pent->v.origin, pEdict))
         {
            // check if the flag has an owner...
            if (pent->v.owner != NULL)
            {
               // get the team for the owner of the flag...
               int player_team = UTIL_GetTeam(pent->v.owner);

               // attack if not our team and not allies team...
               if ((player_team != team) &&
                   !(team_allies[team] & (1<<player_team)))
               {
                  // kill the man with the flag!
                  pBot->pBotEnemy = pent->v.owner;

                  pBot->waypoint_goal = -1;  // forget the goal (if there is one)

                  return TRUE;
               }
            }
            else
            {
               // check if it matches one of the flags...
               for (i=0; i < num_flags; i++)
               {
                  // is the flag for this team (or any team)?
                  if ((flags[i].edict == pent) &&
                      ((flags[i].team_no == (team+1)) || (flags[i].team_no == 0)))
                  {
                     // find the nearest waypoint to the ball...
                     index = WaypointFindNearest(pent->v.origin, pEdict, 500, team);

                     if (index == -1)
                     {
                        // no waypoint is close enough, just head straight towards the ball
                        Vector v_flag = pent->v.origin - pEdict->v.origin;

                        Vector bot_angles = UTIL_VecToAngles( v_flag );

                        pEdict->v.ideal_yaw = bot_angles.y;

                        BotFixIdealYaw(pEdict);

                        return TRUE;
                     }
                     else
                     {
                        waypoint_distance = (waypoints[index].origin - pent->v.origin).Length();
                        distance = (pent->v.origin - pEdict->v.origin).Length();

                        // is the bot closer to the ball than the waypoint is?
                        if (distance < waypoint_distance)
                        {
                           // just head towards the ball
                           Vector v_flag = pent->v.origin - pEdict->v.origin;

                           Vector bot_angles = UTIL_VecToAngles( v_flag );

                           pEdict->v.ideal_yaw = bot_angles.y;

                           BotFixIdealYaw(pEdict);

                           return TRUE;
                        }
                        else
                        {
                           // head towards this waypoint
                           pBot->waypoint_goal = index;

                           // remember where the ball is located...
                           pBot->waypoint_near_flag = TRUE;
                           pBot->waypoint_flag_origin = pent->v.origin;
                        }
                     }
                  }
               }
            }
         }
      }
   }
   else if ((mod_id == GEARBOX_DLL) && (pent_info_ctfdetect != NULL))
   {
      pent = NULL;

      while ((pent = UTIL_FindEntityByClassname( pent, "item_ctfflag" )) != NULL)
      {
         if (pent->v.owner == pEdict)  // is this bot carrying the item?
         {
            // we are carrying the flag

            bot_has_flag = TRUE;

            break;  // break out of while loop
         }
         else if (FInViewCone( &pent->v.origin, pEdict ) &&
                  FVisible( pent->v.origin, pEdict))
         {
            // the bot can see it, check what type of model it is...

            skin = pent->v.skin;

            if (skin == 0) // Opposing Force team (these are BACKASSWARDS!)
               skin = 1;
            else if (skin == 1) // Black Mesa team
               skin = 0;

            // see if the flag matches the bot's team...
            if (skin == team)
            {
               // is and enemy carrying our flag/card?
               if (pent->v.owner != NULL)
               {
                  // kill the man with the flag/card!
                  pBot->pBotEnemy = pent->v.owner;

                  pBot->waypoint_goal = -1;  // forget the goal (if there is one)

                  return TRUE;
               }
            }
            else  // flag/card is for another team!
            {
               // check if someone is NOT carrying the flag/card...
               if (pent->v.owner == NULL)
               {
                  // find the nearest waypoint to the flag/card...
                  index = WaypointFindNearest(pent->v.origin, pEdict, 500, team);

                  if (index == -1)
                  {
                     // no waypoint is close enough, just head straight towards the flag/card
                     Vector v_flag = pent->v.origin - pEdict->v.origin;

                     Vector bot_angles = UTIL_VecToAngles( v_flag );

                     pEdict->v.ideal_yaw = bot_angles.y;

                     BotFixIdealYaw(pEdict);

                     return TRUE;
                  }
                  else
                  {
                     waypoint_distance = (waypoints[index].origin - pent->v.origin).Length();
                     distance = (pent->v.origin - pEdict->v.origin).Length();

                     // is the bot closer to the flag/card than the waypoint is?
                     if (distance < waypoint_distance)
                     {
                        // just head towards the flag/card
                        Vector v_flag = pent->v.origin - pEdict->v.origin;

                        Vector bot_angles = UTIL_VecToAngles( v_flag );

                        pEdict->v.ideal_yaw = bot_angles.y;

                        BotFixIdealYaw(pEdict);

                        return TRUE;
                     }
                     else
                     {
                        // head towards this waypoint
                        pBot->waypoint_goal = index;

                        // remember where the flag/card is located...
                        pBot->waypoint_near_flag = TRUE;
                        pBot->waypoint_flag_origin = pent->v.origin;
                     }
                  }
               }
            }
         }
      }
   }
   else if (mod_id == FRONTLINE_DLL)
   {
      if ((pBot->waypoint_goal != -1) &&  // does bot have a goal?
          (pBot->pCaptureEdict))
      {
         int team = UTIL_GetTeam(pEdict);  // skin and team must match

         if (!pBot->defender)  // if attacker...
         {
            if (pBot->pCaptureEdict->v.skin == team)  // was point captured?
            {
               pBot->waypoint_goal = -1;
            }
         }
         else
         {
            if (pBot->pCaptureEdict->v.skin != team)  // was point captured?
            {
               pBot->waypoint_goal = -1;
            }
         }
      }
   }
   else if (mod_id == HOLYWARS_DLL)
   {
      if (pBot->waypoint_goal != -1)
      {
         // is the bot currently heading toward halo AND halo is gone?
         if ((waypoints[pBot->waypoint_goal].flags & W_FL_FLAG) &&
             (holywars_saint != NULL) && (holywars_gamemode == 1))
         {
            pBot->waypoint_goal = -1;
         }
      }
   }

   // check if we need to find a waypoint...
   if (pBot->curr_waypoint_index == -1)
   {
      pBot->waypoint_top_of_ladder = FALSE;

      // did we just come off of a ladder or are we underwater?
      if (((pBot->f_end_use_ladder_time + 2.0) > gpGlobals->time) ||
          (pBot->pEdict->v.waterlevel == 3))
      {
         // find the nearest visible waypoint
         if (mod_id == FRONTLINE_DLL)
            i = WaypointFindNearest(pEdict, REACHABLE_RANGE, pBot->defender);
         else
            i = WaypointFindNearest(pEdict, REACHABLE_RANGE, team);
      }
      else
      {
         // find the nearest reachable waypoint
         if (mod_id == FRONTLINE_DLL)
            i = WaypointFindReachable(pEdict, REACHABLE_RANGE, pBot->defender);
         else
            i = WaypointFindReachable(pEdict, REACHABLE_RANGE, team);
      }

      if (i == -1)
      {
         pBot->curr_waypoint_index = -1;
         return FALSE;
      }

      pBot->curr_waypoint_index = i;
      pBot->waypoint_origin = waypoints[i].origin;

      pBot->f_waypoint_time = gpGlobals->time;
   }
   else
   {
      // skip this part if bot is trying to get out of water...
      if (pBot->f_exit_water_time < gpGlobals->time)
      {
         // check if we can still see our target waypoint...

         v_src = pEdict->v.origin + pEdict->v.view_ofs;
         v_dest = waypoints[pBot->curr_waypoint_index].origin;

         // trace a line from bot's eyes to destination...
         UTIL_TraceLine( v_src, v_dest, ignore_monsters,
                         pEdict->v.pContainingEntity, &tr );

         // check if line of sight to object is blocked (i.e. not visible)
         if (tr.flFraction < 1.0)
         {
            // did we just come off of a ladder or are we under water?
            if (((pBot->f_end_use_ladder_time + 2.0) > gpGlobals->time) ||
                (pBot->pEdict->v.waterlevel == 3))
            {
               // find the nearest visible waypoint
               if (mod_id == FRONTLINE_DLL)
                  i = WaypointFindNearest(pEdict, REACHABLE_RANGE, pBot->defender);
               else
                  i = WaypointFindNearest(pEdict, REACHABLE_RANGE, team);
            }
            else
            {
               // find the nearest reachable waypoint
               if (mod_id == FRONTLINE_DLL)
                  i = WaypointFindReachable(pEdict, REACHABLE_RANGE, pBot->defender);
               else
                  i = WaypointFindReachable(pEdict, REACHABLE_RANGE, team);
            }

            if (i == -1)
            {
               pBot->curr_waypoint_index = -1;
               return FALSE;
            }

            pBot->curr_waypoint_index = i;
            pBot->waypoint_origin = waypoints[i].origin;

            pBot->f_waypoint_time = gpGlobals->time;
         }
      }
   }

   // find the distance to the target waypoint
   waypoint_distance = (pEdict->v.origin - pBot->waypoint_origin).Length();

   // set the minimum distance from waypoint to be considered "touching" it
   min_distance = 50.0;

   // if this is a crouch waypoint, bot must be fairly close...
   if (waypoints[pBot->curr_waypoint_index].flags & W_FL_CROUCH)
      min_distance = 20.0;

   if (waypoints[pBot->curr_waypoint_index].flags & W_FL_JUMP)
      min_distance = 25.0;

   if (waypoints[pBot->curr_waypoint_index].flags & W_FL_SENTRYGUN)
      min_distance = 20.0;

   if (waypoints[pBot->curr_waypoint_index].flags & W_FL_DISPENSER)
      min_distance = 20.0;

   // if this is a ladder waypoint, bot must be fairly close to get on ladder
   if (waypoints[pBot->curr_waypoint_index].flags & W_FL_LADDER)
      min_distance = 20.0;

   // if this is a defenders waypoint, bot must be fairly close...
   if ((mod_id == FRONTLINE_DLL) &&
       (waypoints[pBot->curr_waypoint_index].flags & W_FL_FLF_DEFEND))
      min_distance = 20.0;

   // if trying to get out of water, need to get very close to waypoint...
   if (pBot->f_exit_water_time >= gpGlobals->time)
      min_distance = 20.0;

   touching = FALSE;

   // did the bot run past the waypoint? (prevent the loop-the-loop problem)
   if ((pBot->prev_waypoint_distance > 1.0) &&
       (waypoint_distance > pBot->prev_waypoint_distance))
      touching = TRUE;

   // are we close enough to a target waypoint...
   if (waypoint_distance < min_distance)
      touching = TRUE;

   // save current distance as previous
   pBot->prev_waypoint_distance = waypoint_distance;

   if (touching)
   {
      bool waypoint_found = FALSE;

      pBot->prev_waypoint_distance = 0.0;

      // check if the waypoint is a door waypoint
      if (waypoints[pBot->curr_waypoint_index].flags & W_FL_DOOR)
      {
         pBot->f_dont_avoid_wall_time = gpGlobals->time + 5.0;
      }

      // check if the next waypoint is a jump waypoint...
      if (waypoints[pBot->curr_waypoint_index].flags & W_FL_JUMP)
      {
         pEdict->v.button |= IN_JUMP;  // jump here
      }

      // check if the waypoint is a sniper waypoint...
      if (waypoints[pBot->curr_waypoint_index].flags & W_FL_SNIPER)
      {
         if (((mod_id == TFC_DLL) && (pEdict->v.playerclass == TFC_CLASS_SNIPER)) ||
             (mod_id != TFC_DLL))
         {
            int aim_index;

            aim_index = WaypointFindNearestAiming(waypoints[pBot->curr_waypoint_index].origin);

            if (aim_index != -1)
            {
               Vector v_aim = waypoints[aim_index].origin - waypoints[pBot->curr_waypoint_index].origin;

               Vector aim_angles = UTIL_VecToAngles( v_aim );

               pEdict->v.ideal_yaw = aim_angles.y;

               BotFixIdealYaw(pEdict);
            }

            pBot->f_pause_time = gpGlobals->time + RANDOM_FLOAT(20.0, 30.0);

            // fix f_waypoint_time so bot won't think it is stuck
            pBot->f_waypoint_time = pBot->f_pause_time;

            return TRUE;
         }
      }

      // check if the bot has reached the goal waypoint...
      if (pBot->curr_waypoint_index == pBot->waypoint_goal)
      {
         pBot->waypoint_goal = -1;  // forget this goal waypoint

         if (pBot->waypoint_near_flag)
         {
            pBot->waypoint_near_flag = FALSE;

            // just head towards the flag/card/ball
            Vector v_flag = pBot->waypoint_flag_origin - pEdict->v.origin;

            Vector bot_angles = UTIL_VecToAngles( v_flag );

            pEdict->v.ideal_yaw = bot_angles.y;

            BotFixIdealYaw(pEdict);

            return TRUE;
         }

         // see if this waypoint is a sentry gun waypoint...
         if ((waypoints[pBot->curr_waypoint_index].flags & W_FL_SENTRYGUN) &&
             (mod_id == TFC_DLL) && (pEdict->v.playerclass == TFC_CLASS_ENGINEER))
         {
            if (pBot->m_rgAmmo[weapon_defs[TF_WEAPON_SPANNER].iAmmo1] >= 130)
            {
               int aim_index;

               aim_index = WaypointFindNearestAiming(waypoints[pBot->curr_waypoint_index].origin);

               if (aim_index != -1)
               {
                  Vector v_aim = waypoints[aim_index].origin - waypoints[pBot->curr_waypoint_index].origin;

                  Vector aim_angles = UTIL_VecToAngles( v_aim );

                  pEdict->v.ideal_yaw = aim_angles.y;

                  BotFixIdealYaw(pEdict);

                  pBot->b_build_sentrygun = TRUE;

                  pBot->sentrygun_waypoint = pBot->curr_waypoint_index;

                  pBot->f_look_for_waypoint_time = gpGlobals->time + 5.0;

                  return TRUE;
               }
            }
         }

         // see if this waypoint is a dispenser waypoint...
         if ((waypoints[pBot->curr_waypoint_index].flags & W_FL_DISPENSER) &&
             (mod_id == TFC_DLL) && (pEdict->v.playerclass == TFC_CLASS_ENGINEER))
         {
            // does bot have enough metal to build a dispenser?
            if (pBot->m_rgAmmo[weapon_defs[TF_WEAPON_SPANNER].iAmmo1] >= 100)
            {
               int aim_index;

               aim_index = WaypointFindNearestAiming(waypoints[pBot->curr_waypoint_index].origin);

               if (aim_index != -1)
               {
                  Vector v_aim = waypoints[aim_index].origin - waypoints[pBot->curr_waypoint_index].origin;

                  Vector aim_angles = UTIL_VecToAngles( v_aim );

                  pEdict->v.ideal_yaw = aim_angles.y;

                  BotFixIdealYaw(pEdict);

                  pBot->b_build_dispenser = TRUE;

                  pBot->dispenser_waypoint = pBot->curr_waypoint_index;

                  pBot->f_look_for_waypoint_time = gpGlobals->time + 5.0;

                  return TRUE;
               }
            }
         }

         if ((mod_id == FRONTLINE_DLL) &&
             (waypoints[pBot->curr_waypoint_index].flags & W_FL_FLF_CAP))
         {
            // it's a capture point
            pent = NULL;

            while ((pent = UTIL_FindEntityInSphere( pent, pEdict->v.origin, 100.0 )) != NULL)
            {
               if (strcmp(STRING(pent->v.classname), "capture_point") == 0)
               {
                  if (pent->v.skin != (1 - team))  // already captured?
                     return TRUE;  // can't do anything here, just return

                  // turn to face the capture entity
                  Vector v_aim = pent->v.origin - pEdict->v.origin;

                  Vector aim_angles = UTIL_VecToAngles( v_aim );

                  pEdict->v.ideal_yaw = aim_angles.y;

                  BotFixIdealYaw(pEdict);

                  return TRUE;
               }
            }
         }

         if ((mod_id == FRONTLINE_DLL) &&
             (waypoints[pBot->curr_waypoint_index].flags & W_FL_FLF_DEFEND))
         {
            // it's a defend point

            int aim_index;

            aim_index = WaypointFindNearestAiming(waypoints[pBot->curr_waypoint_index].origin);

            if (aim_index != -1)
            {
               Vector v_aim = waypoints[aim_index].origin - waypoints[pBot->curr_waypoint_index].origin;

               Vector aim_angles = UTIL_VecToAngles( v_aim );

               pEdict->v.ideal_yaw = aim_angles.y;

               BotFixIdealYaw(pEdict);

               pBot->f_pause_time = gpGlobals->time + RANDOM_FLOAT(30.0, 45.0);

               // fix f_waypoint_time so bot won't think it is stuck
               pBot->f_waypoint_time = pBot->f_pause_time;

               return TRUE;
            }
         }
      }

      // check if the bot is carrying the flag/card/ball...
      if (bot_has_flag)
      {
         pBot->bot_has_flag = TRUE;

         // find the nearest flag goal waypoint...

         index = WaypointFindNearestGoal(pEdict, pBot->curr_waypoint_index,
                                         team, W_FL_FLAG_GOAL);

         pBot->waypoint_goal = index;  // goal index or -1

         pBot->waypoint_near_flag = FALSE;
      }
      else
      {
         pBot->bot_has_flag = FALSE;
      }

      // test special case of bot underwater and close to surface...
      if (pBot->pEdict->v.waterlevel == 3)
      {
         Vector v_src, v_dest;
         TraceResult tr;
         int contents;

         // trace a line straight up 100 units...
         v_src = pEdict->v.origin;
         v_dest = v_src;
         v_dest.z = v_dest.z + 100.0;

         // trace a line to destination...
         UTIL_TraceLine( v_src, v_dest, ignore_monsters,
                         pEdict->v.pContainingEntity, &tr );

         if (tr.flFraction >= 1.0)
         {
            // find out what the contents is of the end of the trace...
            contents = UTIL_PointContents( tr.vecEndPos );
   
            // check if the trace endpoint is in open space...
            if (contents == CONTENTS_EMPTY)
            {
               // find the nearest visible waypoint
               if (mod_id == FRONTLINE_DLL)
                  i = WaypointFindNearest(tr.vecEndPos, pEdict, 100, pBot->defender);
               else
                  i = WaypointFindNearest(tr.vecEndPos, pEdict, 100, team);

               if (i != -1)
               {
                  waypoint_found = TRUE;
                  pBot->curr_waypoint_index = i;
                  pBot->waypoint_origin = waypoints[i].origin;

                  pBot->f_waypoint_time = gpGlobals->time;

                  // keep trying to exit water for next 3 seconds
                  pBot->f_exit_water_time = gpGlobals->time + 3.0;
               }
            }
         }
      }

      // if the bot doesn't have a goal waypoint then pick one...
      if ((pBot->waypoint_goal == -1) &&
          (pBot->f_waypoint_goal_time < gpGlobals->time))
      {
         // don't pick a goal more often than every 10 seconds...
         pBot->f_waypoint_goal_time = gpGlobals->time + 10.0;

         pBot->waypoint_near_flag = FALSE;


// IF HEALTH LESS THAN CRITICAL LEVEL, GO FIND HEALTH!!!

// IF AMMO LESS THAN CRITICAL LEVEL, GO FIND AMMO!!!

// GO FIND WEAPONS HERE!!!


         if ((mod_id == VALVE_DLL) || (mod_id == DMC_DLL))
         {
            if (RANDOM_LONG(1, 100) <= 50)
            {
               index = WaypointFindNearestGoal(pEdict, pBot->curr_waypoint_index,
                                                  team, W_FL_WEAPON, pBot->weapon_points);
            }
            else
            {
               int count = 0;

               index = -1;

               while ((index == -1) && (count < 3))
               {
                  index = WaypointFindRandomGoal(pEdict, team, W_FL_WEAPON, pBot->weapon_points);
                  count++;
               }
            }

            if (index != -1)
            {
               pBot->waypoint_goal = index;

               pBot->weapon_points[4] = pBot->weapon_points[3];
               pBot->weapon_points[3] = pBot->weapon_points[2];
               pBot->weapon_points[2] = pBot->weapon_points[1];
               pBot->weapon_points[1] = pBot->weapon_points[0];
               pBot->weapon_points[0] = pBot->waypoint_goal;
            }
         }
         else if (mod_id == TFC_DLL)
         {
            if (pEdict->v.playerclass == TFC_CLASS_SNIPER)
            {
               if (RANDOM_LONG(1, 100) <= 10)
               {
                  // find the nearest flag waypoint...

                  index = WaypointFindNearestGoal(pEdict, pBot->curr_waypoint_index,
                                                  team, W_FL_FLAG);
               }
               else
               {
                  // find a random sniper waypoint...

                  index = WaypointFindRandomGoal(pEdict, team, W_FL_SNIPER);
               }

               if (index != -1)
                  pBot->waypoint_goal = index;
            }
            else if (pEdict->v.playerclass == TFC_CLASS_ENGINEER)
            {
               // is it time to build sentry gun or dispenser yet?
               if (pBot->f_engineer_build_time <= gpGlobals->time)
               {
                  int value = RANDOM_LONG(1, 100);

                  if (((value <= 70) && (pBot->sentrygun_level < 3)) ||
                      (value <= 40))
                  {
                     // build or upgrade a sentry gun...

                     index = -1;

                     // do we need more metal to build a sentry gun?
                     if (pBot->m_rgAmmo[weapon_defs[TF_WEAPON_SPANNER].iAmmo1] < 130)
                     {
                        // go find some metal...
                        index = WaypointFindNearestGoal(pEdict, pBot->curr_waypoint_index,
                                                     team, W_FL_ARMOR);
                     }
                     else  // otherwise we have enough metal...
                     {
                        if (pBot->sentrygun_waypoint == -1)
                        {
                           // find a random sentry gun waypoint...

                           index = WaypointFindRandomGoal(pEdict, team, W_FL_SENTRYGUN);
                        }
                        else
                           index = pBot->sentrygun_waypoint;
                     }

                     if (index != -1)
                        pBot->waypoint_goal = index;
                  }
                  else
                  {
                     // build a dispenser...
                     index = -1;

                     // do we need more metal to build a dispenser?
                     if (pBot->m_rgAmmo[weapon_defs[TF_WEAPON_SPANNER].iAmmo1] < 100)
                     {
                        // go find some metal...
                        index = WaypointFindNearestGoal(pEdict, pBot->curr_waypoint_index,
                                                        team, W_FL_ARMOR);
                     }
                     else  // otherwise we have enough metal...
                     {
                        if (pBot->dispenser_waypoint == -1)
                        {
                           index = WaypointFindRandomGoal(pEdict, team, W_FL_DISPENSER);
                        }
                        else
                           index = pBot->dispenser_waypoint;
                     }

                     if (index != -1)
                        pBot->waypoint_goal = index;
                  }
               }
               else
               {
                  if (RANDOM_LONG(1, 100) <= 20)
                  {
                     // find the nearest flag waypoint...

                     index = WaypointFindNearestGoal(pEdict, pBot->curr_waypoint_index,
                                                     team, W_FL_FLAG);

                     if (index != -1)
                        pBot->waypoint_goal = index;
                  }
               }
            }
            else
            {
               if (RANDOM_LONG(1, 100) <= 40)
               {
                  // find the nearest flag waypoint...

                  index = WaypointFindNearestGoal(pEdict, pBot->curr_waypoint_index,
                                                  team, W_FL_FLAG);

                  if (index != -1)
                     pBot->waypoint_goal = index;
               }
            }
         }
         else if (mod_id == FRONTLINE_DLL)
         {
            edict_t *caps[20];
            int select, index = 0;
            int capture_color;

            // find a capture point that hasn't been captured yet...

            if (pBot->defender)
               capture_color = team;  // defenders
            else
               capture_color = 1 - team;  // attackers

            pent = NULL;

            while ((pent = UTIL_FindEntityByClassname( pent, "capture_point" )) != NULL)
            {
               if (pent->v.skin != capture_color)  // skip it if already captured
                  continue;

               caps[index++] = pent;
            }

            if (index > 0)  // are any capture points left?...
            {
               if (!pBot->defender)  // if attacker...
               {
                  select = RANDOM_LONG(0, index-1);

                  index = WaypointFindNearestGoal(caps[select]->v.origin, pEdict,
                                                  100, pBot->defender, W_FL_FLF_CAP);

                  if (index != -1)
                  {
                     pBot->waypoint_goal = index;
                     pBot->pCaptureEdict = caps[select];
                  }
               }
               else  // defender...
               {
                  select = RANDOM_LONG(0, index-1);

                  index = WaypointFindRandomGoal(caps[select]->v.origin, pEdict,
                                                 800, pBot->defender, W_FL_FLF_DEFEND);

                  if (index != -1)
                  {
                     pBot->waypoint_goal = index;
                     pBot->pCaptureEdict = caps[select];
                  }
               }
            }
         }
         else if (mod_id == HOLYWARS_DLL)
         {
            if (holywars_gamemode == 0)  // deathmatch?
            {
               if (RANDOM_LONG(1, 100) <= 50)
               {
                  index = WaypointFindNearestGoal(pEdict, pBot->curr_waypoint_index,
                                                  team, W_FL_WEAPON, pBot->weapon_points);
               }
               else
               {
                  index = WaypointFindRandomGoal(pEdict, team, W_FL_WEAPON, pBot->weapon_points);
               }

               if (index != -1)
               {
                  pBot->waypoint_goal = index;

                  pBot->weapon_points[4] = pBot->weapon_points[3];
                  pBot->weapon_points[3] = pBot->weapon_points[2];
                  pBot->weapon_points[2] = pBot->weapon_points[1];
                  pBot->weapon_points[1] = pBot->weapon_points[0];
                  pBot->weapon_points[0] = pBot->waypoint_goal;
               }
            }
            else if (holywars_gamemode == 1)  // halo mode?
            {
               // if waiting for halo to spawn then head for halo spawn point...
               if (halo_status == HW_WAIT_SPAWN)
               {
                  // find the nearest flag waypoint...

                  index = WaypointFindNearestGoal(pEdict, pBot->curr_waypoint_index,
                                                  team, W_FL_FLAG);

                  if (index != -1)
                     pBot->waypoint_goal = index;
                  else
                  {
                     // can't get to the halo from here, search for a weapon

                     index = WaypointFindNearestGoal(pEdict, pBot->curr_waypoint_index,
                                                     team, W_FL_WEAPON);

                     if (index != -1)
                        pBot->waypoint_goal = index;
                  }
               }
               else  // there's already a saint, search for a weapon...
               {
                  if (RANDOM_LONG(1, 100) <= 50)
                  {
                     index = WaypointFindNearestGoal(pEdict, pBot->curr_waypoint_index,
                                                     team, W_FL_WEAPON, pBot->weapon_points);
                  }
                  else
                  {
                     index = WaypointFindRandomGoal(pEdict, team, W_FL_WEAPON, pBot->weapon_points);
                  }

                  if (index != -1)
                  {
                     pBot->waypoint_goal = index;

                     pBot->weapon_points[4] = pBot->weapon_points[3];
                     pBot->weapon_points[3] = pBot->weapon_points[2];
                     pBot->weapon_points[2] = pBot->weapon_points[1];
                     pBot->weapon_points[1] = pBot->weapon_points[0];
                     pBot->weapon_points[0] = pBot->waypoint_goal;
                  }
               }
            }
            else  // assume instagib...
            {
               // don't do anything, just randomly wander around
            }
         }
      }

      // check if the bot has a goal waypoint...
      if (pBot->waypoint_goal != -1)
      {
         // get the next waypoint to reach goal...
         if (mod_id == FRONTLINE_DLL)
         {
            i = WaypointRouteFromTo(pBot->curr_waypoint_index,
                                    pBot->waypoint_goal, pBot->defender);
         }
         else
         {
            i = WaypointRouteFromTo(pBot->curr_waypoint_index,
                                    pBot->waypoint_goal, team);
         }

         if (i != WAYPOINT_UNREACHABLE)  // can we get to the goal from here?
         {
            waypoint_found = TRUE;
            pBot->curr_waypoint_index = i;
            pBot->waypoint_origin = waypoints[i].origin;

            pBot->f_waypoint_time = gpGlobals->time;
         }
      }

      if (waypoint_found == FALSE)
      {
         index = 4;

         // try to find the next waypoint
         while (((status = BotFindWaypoint( pBot )) == FALSE) &&
                (index > 0))
         {
            // if waypoint not found, clear oldest prevous index and try again

            pBot->prev_waypoint_index[index] = -1;
            index--;
         }

         if (status == FALSE)
         {
            pBot->curr_waypoint_index = -1;  // indicate no waypoint found

            // clear all previous waypoints...
            for (index=0; index < 5; index++)
               pBot->prev_waypoint_index[index] = -1;

            return FALSE;
         }
      }
   }

   // check if the next waypoint is on a ladder AND
   // the bot it not currently on a ladder...
   if ((waypoints[pBot->curr_waypoint_index].flags & W_FL_LADDER) &&
       (pEdict->v.movetype != MOVETYPE_FLY))
   {
      // if it's origin is lower than the bot...
      if (waypoints[pBot->curr_waypoint_index].origin.z < pEdict->v.origin.z)
      {
         pBot->waypoint_top_of_ladder = TRUE;
      }
   }
   else
   {
      pBot->waypoint_top_of_ladder = FALSE;
   }

   // keep turning towards the waypoint...

   Vector v_direction = pBot->waypoint_origin - pEdict->v.origin;

   Vector v_angles = UTIL_VecToAngles(v_direction);

   // if the bot is NOT on a ladder, change the yaw...
   if (pEdict->v.movetype != MOVETYPE_FLY)
   {
      pEdict->v.idealpitch = -v_angles.x;
      BotFixIdealPitch(pEdict);

      pEdict->v.ideal_yaw = v_angles.y;
      BotFixIdealYaw(pEdict);
   }

   return TRUE;
}


void BotOnLadder( bot_t *pBot, float moved_distance )
{
   Vector v_src, v_dest, view_angles;
   TraceResult tr;
   float angle = 0.0;
   bool done = FALSE;

   edict_t *pEdict = pBot->pEdict;

   // check if the bot has JUST touched this ladder...
   if (pBot->ladder_dir == LADDER_UNKNOWN)
   {
      // try to square up the bot on the ladder...
      while ((!done) && (angle < 180.0))
      {
         // try looking in one direction (forward + angle)
         view_angles = pEdict->v.v_angle;
         view_angles.y = pEdict->v.v_angle.y + angle;

         if (view_angles.y < 0.0)
            view_angles.y += 360.0;
         if (view_angles.y > 360.0)
            view_angles.y -= 360.0;

         UTIL_MakeVectors( view_angles );

         v_src = pEdict->v.origin + pEdict->v.view_ofs;
         v_dest = v_src + gpGlobals->v_forward * 30;

         UTIL_TraceLine( v_src, v_dest, dont_ignore_monsters,
                         pEdict->v.pContainingEntity, &tr);

         if (tr.flFraction < 1.0)  // hit something?
         {
            if (strcmp("func_wall", STRING(tr.pHit->v.classname)) == 0)
            {
               // square up to the wall...
               view_angles = UTIL_VecToAngles(tr.vecPlaneNormal);

               // Normal comes OUT from wall, so flip it around...
               view_angles.y += 180;

               if (view_angles.y > 180)
                  view_angles.y -= 360;

               pEdict->v.ideal_yaw = view_angles.y;

               BotFixIdealYaw(pEdict);

               done = TRUE;
            }
         }
         else
         {
            // try looking in the other direction (forward - angle)
            view_angles = pEdict->v.v_angle;
            view_angles.y = pEdict->v.v_angle.y - angle;

            if (view_angles.y < 0.0)
               view_angles.y += 360.0;
            if (view_angles.y > 360.0)
               view_angles.y -= 360.0;

            UTIL_MakeVectors( view_angles );

            v_src = pEdict->v.origin + pEdict->v.view_ofs;
            v_dest = v_src + gpGlobals->v_forward * 30;

            UTIL_TraceLine( v_src, v_dest, dont_ignore_monsters,
                            pEdict->v.pContainingEntity, &tr);

            if (tr.flFraction < 1.0)  // hit something?
            {
               if (strcmp("func_wall", STRING(tr.pHit->v.classname)) == 0)
               {
                  // square up to the wall...
                  view_angles = UTIL_VecToAngles(tr.vecPlaneNormal);

                  // Normal comes OUT from wall, so flip it around...
                  view_angles.y += 180;

                  if (view_angles.y > 180)
                     view_angles.y -= 360;

                  pEdict->v.ideal_yaw = view_angles.y;

                  BotFixIdealYaw(pEdict);

                  done = TRUE;
               }
            }
         }

         angle += 10;
      }

      if (!done)  // if didn't find a wall, just reset ideal_yaw...
      {
         // set ideal_yaw to current yaw (so bot won't keep turning)
         pEdict->v.ideal_yaw = pEdict->v.v_angle.y;

         BotFixIdealYaw(pEdict);
      }
   }

   // moves the bot up or down a ladder.  if the bot can't move
   // (i.e. get's stuck with someone else on ladder), the bot will
   // change directions and go the other way on the ladder.

   if (pBot->ladder_dir == LADDER_UP)  // is the bot currently going up?
   {
      pEdict->v.v_angle.x = -60;  // look upwards

      // check if the bot hasn't moved much since the last location...
      if ((moved_distance <= 1) && (pBot->f_prev_speed >= 1.0))
      {
         // the bot must be stuck, change directions...

         pEdict->v.v_angle.x = 60;  // look downwards
         pBot->ladder_dir = LADDER_DOWN;
      }
   }
   else if (pBot->ladder_dir == LADDER_DOWN)  // is the bot currently going down?
   {
      pEdict->v.v_angle.x = 60;  // look downwards

      // check if the bot hasn't moved much since the last location...
      if ((moved_distance <= 1) && (pBot->f_prev_speed >= 1.0))
      {
         // the bot must be stuck, change directions...

         pEdict->v.v_angle.x = -60;  // look upwards
         pBot->ladder_dir = LADDER_UP;
      }
   }
   else  // the bot hasn't picked a direction yet, try going up...
   {
      pEdict->v.v_angle.x = -60;  // look upwards
      pBot->ladder_dir = LADDER_UP;
   }

   // move forward (i.e. in the direction the bot is looking, up or down)
   pEdict->v.button |= IN_FORWARD;
}


void BotUnderWater( bot_t *pBot )
{
   bool found_waypoint = FALSE;

   edict_t *pEdict = pBot->pEdict;

   // are there waypoints in this level (and not trying to exit water)?
   if ((num_waypoints > 0) &&
       (pBot->f_exit_water_time < gpGlobals->time))
   {
      // head towards a waypoint
      found_waypoint = BotHeadTowardWaypoint(pBot);
   }

   if (found_waypoint == FALSE)
   {
      // handle movements under water.  right now, just try to keep from
      // drowning by swimming up towards the surface and look to see if
      // there is a surface the bot can jump up onto to get out of the
      // water.  bots DON'T like water!

      Vector v_src, v_forward;
      TraceResult tr;
      int contents;
   
      // swim up towards the surface
      pEdict->v.v_angle.x = -60;  // look upwards
   
      // move forward (i.e. in the direction the bot is looking, up or down)
      pEdict->v.button |= IN_FORWARD;
   
      // set gpGlobals angles based on current view angle (for TraceLine)
      UTIL_MakeVectors( pEdict->v.v_angle );
   
      // look from eye position straight forward (remember: the bot is looking
      // upwards at a 60 degree angle so TraceLine will go out and up...
   
      v_src = pEdict->v.origin + pEdict->v.view_ofs;  // EyePosition()
      v_forward = v_src + gpGlobals->v_forward * 90;
   
      // trace from the bot's eyes straight forward...
      UTIL_TraceLine( v_src, v_forward, dont_ignore_monsters,
                      pEdict->v.pContainingEntity, &tr);
   
      // check if the trace didn't hit anything (i.e. nothing in the way)...
      if (tr.flFraction >= 1.0)
      {
         // find out what the contents is of the end of the trace...
         contents = UTIL_PointContents( tr.vecEndPos );
   
         // check if the trace endpoint is in open space...
         if (contents == CONTENTS_EMPTY)
         {
            // ok so far, we are at the surface of the water, continue...
   
            v_src = tr.vecEndPos;
            v_forward = v_src;
            v_forward.z -= 90;
   
            // trace from the previous end point straight down...
            UTIL_TraceLine( v_src, v_forward, dont_ignore_monsters,
                            pEdict->v.pContainingEntity, &tr);
   
            // check if the trace hit something...
            if (tr.flFraction < 1.0)
            {
               contents = UTIL_PointContents( tr.vecEndPos );
   
               // if contents isn't water then assume it's land, jump!
               if (contents != CONTENTS_WATER)
               {
                  pEdict->v.button |= IN_JUMP;
               }
            }
         }
      }
   }
}


void BotUseLift( bot_t *pBot, float moved_distance )
{
   edict_t *pEdict = pBot->pEdict;

   // just need to press the button once, when the flag gets set...
   if (pBot->f_use_button_time == gpGlobals->time)
   {
      pEdict->v.button = IN_USE;

      // face opposite from the button
      pEdict->v.ideal_yaw += 180;  // rotate 180 degrees

      BotFixIdealYaw(pEdict);
   }

   // check if the bot has waited too long for the lift to move...
   if (((pBot->f_use_button_time + 2.0) < gpGlobals->time) &&
       (!pBot->b_lift_moving))
   {
      // clear use button flag
      pBot->b_use_button = FALSE;

      // bot doesn't have to set f_find_item since the bot
      // should already be facing away from the button

      pBot->f_move_speed = pBot->f_max_speed;
   }

   // check if lift has started moving...
   if ((moved_distance > 1) && (!pBot->b_lift_moving))
   {
      pBot->b_lift_moving = TRUE;
   }

   // check if lift has stopped moving...
   if ((moved_distance <= 1) && (pBot->b_lift_moving))
   {
      TraceResult tr1, tr2;
      Vector v_src, v_forward, v_right, v_left;
      Vector v_down, v_forward_down, v_right_down, v_left_down;

      pBot->b_use_button = FALSE;

      // TraceLines in 4 directions to find which way to go...

      UTIL_MakeVectors( pEdict->v.v_angle );

      v_src = pEdict->v.origin + pEdict->v.view_ofs;
      v_forward = v_src + gpGlobals->v_forward * 90;
      v_right = v_src + gpGlobals->v_right * 90;
      v_left = v_src + gpGlobals->v_right * -90;

      v_down = pEdict->v.v_angle;
      v_down.x = v_down.x + 45;  // look down at 45 degree angle

      UTIL_MakeVectors( v_down );

      v_forward_down = v_src + gpGlobals->v_forward * 100;
      v_right_down = v_src + gpGlobals->v_right * 100;
      v_left_down = v_src + gpGlobals->v_right * -100;

      // try tracing forward first...
      UTIL_TraceLine( v_src, v_forward, dont_ignore_monsters,
                      pEdict->v.pContainingEntity, &tr1);
      UTIL_TraceLine( v_src, v_forward_down, dont_ignore_monsters,
                      pEdict->v.pContainingEntity, &tr2);

      // check if we hit a wall or didn't find a floor...
      if ((tr1.flFraction < 1.0) || (tr2.flFraction >= 1.0))
      {
         // try tracing to the RIGHT side next...
         UTIL_TraceLine( v_src, v_right, dont_ignore_monsters,
                         pEdict->v.pContainingEntity, &tr1);
         UTIL_TraceLine( v_src, v_right_down, dont_ignore_monsters,
                         pEdict->v.pContainingEntity, &tr2);

         // check if we hit a wall or didn't find a floor...
         if ((tr1.flFraction < 1.0) || (tr2.flFraction >= 1.0))
         {
            // try tracing to the LEFT side next...
            UTIL_TraceLine( v_src, v_left, dont_ignore_monsters,
                            pEdict->v.pContainingEntity, &tr1);
            UTIL_TraceLine( v_src, v_left_down, dont_ignore_monsters,
                            pEdict->v.pContainingEntity, &tr2);

            // check if we hit a wall or didn't find a floor...
            if ((tr1.flFraction < 1.0) || (tr2.flFraction >= 1.0))
            {
               // only thing to do is turn around...
               pEdict->v.ideal_yaw += 180;  // turn all the way around
            }
            else
            {
               pEdict->v.ideal_yaw += 90;  // turn to the LEFT
            }
         }
         else
         {
            pEdict->v.ideal_yaw -= 90;  // turn to the RIGHT
         }

         BotFixIdealYaw(pEdict);
      }

      BotChangeYaw( pBot, pEdict->v.yaw_speed );

      pBot->f_move_speed = pBot->f_max_speed;
   }
}


bool BotStuckInCorner( bot_t *pBot )
{
   TraceResult tr;
   Vector v_src, v_dest;
   edict_t *pEdict = pBot->pEdict;
   
   UTIL_MakeVectors( pEdict->v.v_angle );

   // trace 45 degrees to the right...
   v_src = pEdict->v.origin;
   v_dest = v_src + gpGlobals->v_forward*20 + gpGlobals->v_right*20;

   UTIL_TraceLine( v_src, v_dest, dont_ignore_monsters,
                   pEdict->v.pContainingEntity, &tr);

   if (tr.flFraction >= 1.0)
      return FALSE;  // no wall, so not in a corner

   // trace 45 degrees to the left...
   v_src = pEdict->v.origin;
   v_dest = v_src + gpGlobals->v_forward*20 - gpGlobals->v_right*20;

   UTIL_TraceLine( v_src, v_dest, dont_ignore_monsters,
                   pEdict->v.pContainingEntity, &tr);

   if (tr.flFraction >= 1.0)
      return FALSE;  // no wall, so not in a corner

   return TRUE;  // bot is in a corner
}


void BotTurnAtWall( bot_t *pBot, TraceResult *tr, bool negative )
{
   edict_t *pEdict = pBot->pEdict;
   Vector Normal;
   float Y, Y1, Y2, D1, D2, Z;

   // Find the normal vector from the trace result.  The normal vector will
   // be a vector that is perpendicular to the surface from the TraceResult.

   Normal = UTIL_VecToAngles(tr->vecPlaneNormal);

   // Since the bot keeps it's view angle in -180 < x < 180 degrees format,
   // and since TraceResults are 0 < x < 360, we convert the bot's view
   // angle (yaw) to the same format at TraceResult.

   Y = pEdict->v.v_angle.y;
   Y = Y + 180;
   if (Y > 359) Y -= 360;

   if (negative)
   {
      // Turn the normal vector around 180 degrees (i.e. make it point towards
      // the wall not away from it.  That makes finding the angles that the
      // bot needs to turn a little easier.

      Normal.y = Normal.y - 180;
   }

   if (Normal.y < 0)
      Normal.y += 360;

   // Here we compare the bots view angle (Y) to the Normal - 90 degrees (Y1)
   // and the Normal + 90 degrees (Y2).  These two angles (Y1 & Y2) represent
   // angles that are parallel to the wall surface, but heading in opposite
   // directions.  We want the bot to choose the one that will require the
   // least amount of turning (saves time) and have the bot head off in that
   // direction.

   Y1 = Normal.y - 90;
   if (RANDOM_LONG(1, 100) <= 50)
   {
      Y1 = Y1 - RANDOM_FLOAT(5.0, 20.0);
   }
   if (Y1 < 0) Y1 += 360;

   Y2 = Normal.y + 90;
   if (RANDOM_LONG(1, 100) <= 50)
   {
      Y2 = Y2 + RANDOM_FLOAT(5.0, 20.0);
   }
   if (Y2 > 359) Y2 -= 360;

   // D1 and D2 are the difference (in degrees) between the bot's current
   // angle and Y1 or Y2 (respectively).

   D1 = fabs(Y - Y1);
   if (D1 > 179) D1 = fabs(D1 - 360);
   D2 = fabs(Y - Y2);
   if (D2 > 179) D2 = fabs(D2 - 360);

   // If difference 1 (D1) is more than difference 2 (D2) then the bot will
   // have to turn LESS if it heads in direction Y1 otherwise, head in
   // direction Y2.  I know this seems backwards, but try some sample angles
   // out on some graph paper and go through these equations using a
   // calculator, you'll see what I mean.

   if (D1 > D2)
      Z = Y1;
   else
      Z = Y2;

   // convert from TraceResult 0 to 360 degree format back to bot's
   // -180 to 180 degree format.

   if (Z > 180)
      Z -= 360;

   // set the direction to head off into...
   pEdict->v.ideal_yaw = Z;

   BotFixIdealYaw(pEdict);
}


bool BotCantMoveForward( bot_t *pBot, TraceResult *tr )
{
   edict_t *pEdict = pBot->pEdict;

   // use some TraceLines to determine if anything is blocking the current
   // path of the bot.

   Vector v_src, v_forward;

   UTIL_MakeVectors( pEdict->v.v_angle );

   // first do a trace from the bot's eyes forward...

   v_src = pEdict->v.origin + pEdict->v.view_ofs;  // EyePosition()
   v_forward = v_src + gpGlobals->v_forward * 40;

   // trace from the bot's eyes straight forward...
   UTIL_TraceLine( v_src, v_forward, dont_ignore_monsters,
                   pEdict->v.pContainingEntity, tr);

   // check if the trace hit something...
   if (tr->flFraction < 1.0)
   {
      return TRUE;  // bot's head will hit something
   }

   // bot's head is clear, check at waist level...

   v_src = pEdict->v.origin;
   v_forward = v_src + gpGlobals->v_forward * 40;

   // trace from the bot's waist straight forward...
   UTIL_TraceLine( v_src, v_forward, dont_ignore_monsters,
                   pEdict->v.pContainingEntity, tr);

   // check if the trace hit something...
   if (tr->flFraction < 1.0)
   {
      return TRUE;  // bot's body will hit something
   }

   return FALSE;  // bot can move forward, return false
}


bool BotCanJumpUp( bot_t *pBot, bool *bDuckJump)
{
   // What I do here is trace 3 lines straight out, one unit higher than
   // the highest normal jumping distance.  I trace once at the center of
   // the body, once at the right side, and once at the left side.  If all
   // three of these TraceLines don't hit an obstruction then I know the
   // area to jump to is clear.  I then need to trace from head level,
   // above where the bot will jump to, downward to see if there is anything
   // blocking the jump.  There could be a narrow opening that the body
   // will not fit into.  These horizontal and vertical TraceLines seem
   // to catch most of the problems with falsely trying to jump on something
   // that the bot can not get onto.

   TraceResult tr;
   bool check_duck = FALSE;
   Vector v_jump, v_source, v_dest;
   edict_t *pEdict = pBot->pEdict;

   *bDuckJump = FALSE;

   // convert current view angle to vectors for TraceLine math...

   v_jump = pEdict->v.v_angle;
   v_jump.x = 0;  // reset pitch to 0 (level horizontally)
   v_jump.z = 0;  // reset roll to 0 (straight up and down)

   UTIL_MakeVectors( v_jump );

   // use center of the body first...

   // maximum normal jump height is 45, so check one unit above that (46)
   v_source = pEdict->v.origin + Vector(0, 0, -36 + 46);
   v_dest = v_source + gpGlobals->v_forward * 24;

   // trace a line forward at maximum jump height...
   UTIL_TraceLine( v_source, v_dest, dont_ignore_monsters,
                   pEdict->v.pContainingEntity, &tr);

   // if trace hit something, check duck jump...
   if (tr.flFraction < 1.0)
      check_duck = TRUE;

   if (!check_duck)
   {
      // now check same height to one side of the bot...
      v_source = pEdict->v.origin + gpGlobals->v_right * 16 + Vector(0, 0, -36 + 46);
      v_dest = v_source + gpGlobals->v_forward * 24;

      // trace a line forward at maximum jump height...
      UTIL_TraceLine( v_source, v_dest, dont_ignore_monsters,
                      pEdict->v.pContainingEntity, &tr);

      // if trace hit something, check duck jump...
      if (tr.flFraction < 1.0)
         check_duck = TRUE;
   }

   if (!check_duck)
   {
      // now check same height on the other side of the bot...
      v_source = pEdict->v.origin + gpGlobals->v_right * -16 + Vector(0, 0, -36 + 46);
      v_dest = v_source + gpGlobals->v_forward * 24;

      // trace a line forward at maximum jump height...
      UTIL_TraceLine( v_source, v_dest, dont_ignore_monsters,
                      pEdict->v.pContainingEntity, &tr);

      // if trace hit something, check duck jump...
      if (tr.flFraction < 1.0)
         check_duck = TRUE;
   }

   if (check_duck)
   {
      // maximum crouch jump height is 63, so check one unit above that (64)
      v_source = pEdict->v.origin + Vector(0, 0, -36 + 64);
      v_dest = v_source + gpGlobals->v_forward * 24;

      // trace a line forward at maximum jump height...
      UTIL_TraceLine( v_source, v_dest, dont_ignore_monsters,
                      pEdict->v.pContainingEntity, &tr);

      // if trace hit something, return FALSE
      if (tr.flFraction < 1.0)
         return FALSE;

      // now check same height on the other side of the bot...
      v_source = pEdict->v.origin + gpGlobals->v_right * -16 + Vector(0, 0, -36 + 64);
      v_dest = v_source + gpGlobals->v_forward * 24;

      // trace a line forward at maximum jump height...
      UTIL_TraceLine( v_source, v_dest, dont_ignore_monsters,
                      pEdict->v.pContainingEntity, &tr);

      // if trace hit something, return FALSE
      if (tr.flFraction < 1.0)
         return FALSE;

      // now check same height on the other side of the bot...
      v_source = pEdict->v.origin + gpGlobals->v_right * -16 + Vector(0, 0, -36 + 64);
      v_dest = v_source + gpGlobals->v_forward * 24;

      // trace a line forward at maximum jump height...
      UTIL_TraceLine( v_source, v_dest, dont_ignore_monsters,
                      pEdict->v.pContainingEntity, &tr);

      // if trace hit something, return FALSE
      if (tr.flFraction < 1.0)
         return FALSE;
   }

   // now trace from head level downward to check for obstructions...

   // start of trace is 24 units in front of bot...
   v_source = pEdict->v.origin + gpGlobals->v_forward * 24;

   if (check_duck)
      // offset 36 units if crouch-jump (36 + 36)
      v_source.z = v_source.z + 72;
   else
      // offset 72 units from top of head (72 + 36)
      v_source.z = v_source.z + 108;


   if (check_duck)
      // end point of trace is 27 units straight down from start...
      // (27 is 72 minus the jump limit height which is 63 - 18 = 45)
      v_dest = v_source + Vector(0, 0, -27);
   else
      // end point of trace is 99 units straight down from start...
      // (99 is 108 minus the jump limit height which is 45 - 36 = 9)
      v_dest = v_source + Vector(0, 0, -99);


   // trace a line straight down toward the ground...
   UTIL_TraceLine( v_source, v_dest, dont_ignore_monsters,
                   pEdict->v.pContainingEntity, &tr);

   // if trace hit something, return FALSE
   if (tr.flFraction < 1.0)
      return FALSE;

   // now check same height to one side of the bot...
   v_source = pEdict->v.origin + gpGlobals->v_right * 16 + gpGlobals->v_forward * 24;

   if (check_duck)
      v_source.z = v_source.z + 72;
   else
      v_source.z = v_source.z + 108;

   if (check_duck)
      v_dest = v_source + Vector(0, 0, -27);
   else
      v_dest = v_source + Vector(0, 0, -99);

   // trace a line straight down toward the ground...
   UTIL_TraceLine( v_source, v_dest, dont_ignore_monsters,
                   pEdict->v.pContainingEntity, &tr);

   // if trace hit something, return FALSE
   if (tr.flFraction < 1.0)
      return FALSE;

   // now check same height on the other side of the bot...
   v_source = pEdict->v.origin + gpGlobals->v_right * -16 + gpGlobals->v_forward * 24;

   if (check_duck)
      v_source.z = v_source.z + 72;
   else
      v_source.z = v_source.z + 108;

   if (check_duck)
      v_dest = v_source + Vector(0, 0, -27);
   else
      v_dest = v_source + Vector(0, 0, -99);

   // trace a line straight down toward the ground...
   UTIL_TraceLine( v_source, v_dest, dont_ignore_monsters,
                   pEdict->v.pContainingEntity, &tr);

   // if trace hit something, return FALSE
   if (tr.flFraction < 1.0)
      return FALSE;

   return TRUE;
}


bool BotCanDuckUnder( bot_t *pBot )
{
   // What I do here is trace 3 lines straight out, one unit higher than
   // the ducking height.  I trace once at the center of the body, once
   // at the right side, and once at the left side.  If all three of these
   // TraceLines don't hit an obstruction then I know the area to duck to
   // is clear.  I then need to trace from the ground up, 72 units, to make
   // sure that there is something blocking the TraceLine.  Then we know
   // we can duck under it.

   TraceResult tr;
   Vector v_duck, v_source, v_dest;
   edict_t *pEdict = pBot->pEdict;

   // convert current view angle to vectors for TraceLine math...

   v_duck = pEdict->v.v_angle;
   v_duck.x = 0;  // reset pitch to 0 (level horizontally)
   v_duck.z = 0;  // reset roll to 0 (straight up and down)

   UTIL_MakeVectors( v_duck );

   // use center of the body first...

   // duck height is 36, so check one unit above that (37)
   v_source = pEdict->v.origin + Vector(0, 0, -36 + 37);
   v_dest = v_source + gpGlobals->v_forward * 24;

   // trace a line forward at duck height...
   UTIL_TraceLine( v_source, v_dest, dont_ignore_monsters,
                   pEdict->v.pContainingEntity, &tr);

   // if trace hit something, return FALSE
   if (tr.flFraction < 1.0)
      return FALSE;

   // now check same height to one side of the bot...
   v_source = pEdict->v.origin + gpGlobals->v_right * 16 + Vector(0, 0, -36 + 37);
   v_dest = v_source + gpGlobals->v_forward * 24;

   // trace a line forward at duck height...
   UTIL_TraceLine( v_source, v_dest, dont_ignore_monsters,
                   pEdict->v.pContainingEntity, &tr);

   // if trace hit something, return FALSE
   if (tr.flFraction < 1.0)
      return FALSE;

   // now check same height on the other side of the bot...
   v_source = pEdict->v.origin + gpGlobals->v_right * -16 + Vector(0, 0, -36 + 37);
   v_dest = v_source + gpGlobals->v_forward * 24;

   // trace a line forward at duck height...
   UTIL_TraceLine( v_source, v_dest, dont_ignore_monsters,
                   pEdict->v.pContainingEntity, &tr);

   // if trace hit something, return FALSE
   if (tr.flFraction < 1.0)
      return FALSE;

   // now trace from the ground up to check for object to duck under...

   // start of trace is 24 units in front of bot near ground...
   v_source = pEdict->v.origin + gpGlobals->v_forward * 24;
   v_source.z = v_source.z - 35;  // offset to feet + 1 unit up

   // end point of trace is 72 units straight up from start...
   v_dest = v_source + Vector(0, 0, 72);

   // trace a line straight up in the air...
   UTIL_TraceLine( v_source, v_dest, dont_ignore_monsters,
                   pEdict->v.pContainingEntity, &tr);

   // if trace didn't hit something, return FALSE
   if (tr.flFraction >= 1.0)
      return FALSE;

   // now check same height to one side of the bot...
   v_source = pEdict->v.origin + gpGlobals->v_right * 16 + gpGlobals->v_forward * 24;
   v_source.z = v_source.z - 35;  // offset to feet + 1 unit up
   v_dest = v_source + Vector(0, 0, 72);

   // trace a line straight up in the air...
   UTIL_TraceLine( v_source, v_dest, dont_ignore_monsters,
                   pEdict->v.pContainingEntity, &tr);

   // if trace didn't hit something, return FALSE
   if (tr.flFraction >= 1.0)
      return FALSE;

   // now check same height on the other side of the bot...
   v_source = pEdict->v.origin + gpGlobals->v_right * -16 + gpGlobals->v_forward * 24;
   v_source.z = v_source.z - 35;  // offset to feet + 1 unit up
   v_dest = v_source + Vector(0, 0, 72);

   // trace a line straight up in the air...
   UTIL_TraceLine( v_source, v_dest, dont_ignore_monsters,
                   pEdict->v.pContainingEntity, &tr);

   // if trace didn't hit something, return FALSE
   if (tr.flFraction >= 1.0)
      return FALSE;

   return TRUE;
}


void BotRandomTurn( bot_t *pBot )
{
   pBot->f_move_speed = 0;  // don't move while turning
            
   if (RANDOM_LONG(1, 100) <= 10)
   {
      // 10 percent of the time turn completely around...
      pBot->pEdict->v.ideal_yaw += 180;
   }
   else
   {
      // turn randomly between 30 and 60 degress
      if (pBot->wander_dir == WANDER_LEFT)
         pBot->pEdict->v.ideal_yaw += RANDOM_LONG(30, 60);
      else
         pBot->pEdict->v.ideal_yaw -= RANDOM_LONG(30, 60);
   }
            
   BotFixIdealYaw(pBot->pEdict);
}


bool BotFollowUser( bot_t *pBot )
{
   bool user_visible;
   float f_distance;
   edict_t *pEdict = pBot->pEdict;

   Vector vecEnd = pBot->pBotUser->v.origin + pBot->pBotUser->v.view_ofs;

   if (!IsAlive( pBot->pBotUser ))
   {
      // the bot's user is dead!
      pBot->pBotUser = NULL;
      return FALSE;
   }

   user_visible = FInViewCone( &vecEnd, pEdict ) &&
                  FVisible( vecEnd, pEdict );

   // check if the "user" is still visible or if the user has been visible
   // in the last 5 seconds (or the player just starting "using" the bot)

   if (user_visible || (pBot->f_bot_use_time + 5 > gpGlobals->time))
   {
      if (user_visible)
         pBot->f_bot_use_time = gpGlobals->time;  // reset "last visible time"

      // face the user
      Vector v_user = pBot->pBotUser->v.origin - pEdict->v.origin;
      Vector bot_angles = UTIL_VecToAngles( v_user );

      pEdict->v.ideal_yaw = bot_angles.y;

      BotFixIdealYaw(pEdict);

      f_distance = v_user.Length( );  // how far away is the "user"?

      if (f_distance > 200)      // run if distance to enemy is far
         pBot->f_move_speed = pBot->f_max_speed;
      else if (f_distance > 50)  // walk if distance is closer
         pBot->f_move_speed = pBot->f_max_speed / 2;
      else                     // don't move if close enough
         pBot->f_move_speed = 0.0;

      return TRUE;
   }
   else
   {
      // person to follow has gone out of sight...
      pBot->pBotUser = NULL;

      return FALSE;
   }
}


bool BotCheckWallOnLeft( bot_t *pBot )
{
   edict_t *pEdict = pBot->pEdict;
   Vector v_src, v_left;
   TraceResult tr;

   UTIL_MakeVectors( pEdict->v.v_angle );

   // do a trace to the left...

   v_src = pEdict->v.origin;
   v_left = v_src + gpGlobals->v_right * -40;  // 40 units to the left

   UTIL_TraceLine( v_src, v_left, dont_ignore_monsters,
                   pEdict->v.pContainingEntity, &tr);

   // check if the trace hit something...
   if (tr.flFraction < 1.0)
   {
      if (pBot->f_wall_on_left < 1.0)
         pBot->f_wall_on_left = gpGlobals->time;

      return TRUE;
   }

   return FALSE;
}


bool BotCheckWallOnRight( bot_t *pBot )
{
   edict_t *pEdict = pBot->pEdict;
   Vector v_src, v_right;
   TraceResult tr;

   UTIL_MakeVectors( pEdict->v.v_angle );

   // do a trace to the right...

   v_src = pEdict->v.origin;
   v_right = v_src + gpGlobals->v_right * 40;  // 40 units to the right

   UTIL_TraceLine( v_src, v_right, dont_ignore_monsters,
                   pEdict->v.pContainingEntity, &tr);

   // check if the trace hit something...
   if (tr.flFraction < 1.0)
   {
      if (pBot->f_wall_on_right < 1.0)
         pBot->f_wall_on_right = gpGlobals->time;

      return TRUE;
   }

   return FALSE;
}


void BotLookForDrop( bot_t *pBot )
{
   edict_t *pEdict = pBot->pEdict;

   Vector v_src, v_dest, v_ahead;
   float scale, direction;
   TraceResult tr;
   int contents;
   bool need_to_turn, done;
   int turn_count;

   scale = 80 + (pBot->f_max_speed / 10);

   v_ahead = pEdict->v.v_angle;
   v_ahead.x = 0;  // set pitch to level horizontally
   UTIL_MakeVectors(v_ahead);

   v_src = pEdict->v.origin;
   v_dest = v_src + gpGlobals->v_forward * scale;

   UTIL_TraceLine( v_src, v_dest, ignore_monsters,
                   pEdict->v.pContainingEntity, &tr );

   // check if area in front of bot was clear...
   if (tr.flFraction >= 1.0) 
   {
      v_src = v_dest;  // start downward trace from endpoint of open trace
      v_dest.z = v_dest.z - max_drop_height;

      UTIL_TraceLine( v_src, v_dest, ignore_monsters,
                      pEdict->v.pContainingEntity, &tr );

      need_to_turn = FALSE;

      // if trace did not hit anything then drop is TOO FAR...
      if (tr.flFraction >= 1.0) 
      {
         need_to_turn = TRUE;
      }
      else
      {
         // we've hit something, see if it's water or lava
         contents = UTIL_PointContents( tr.vecEndPos );

         if (contents == CONTENTS_LAVA)
         {
            need_to_turn = TRUE;
         }

         if (contents == CONTENTS_WATER)
         {
            // if you don't like water, set need_to_turn = TRUE here
         }
      }

      if (need_to_turn)
      {

         // if we have an enemy, stop heading towards enemy...
         if (pBot->pBotEnemy)
         {
            pBot->pBotEnemy = NULL;
            pBot->f_bot_find_enemy_time = gpGlobals->time + 1.0;
         }

         // don't look for items for a while...
         pBot->f_find_item = gpGlobals->time + 1.0;

         // change the bot's ideal yaw by finding surface normal
         // slightly below where the bot is standing

         v_dest = pEdict->v.origin;

         if (pEdict->v.flags & FL_DUCKING)
         {
            v_src.z = v_src.z - 22;  // (36/2) + 4 units
            v_dest.z = v_dest.z - 22;
         }
         else
         {
            v_src.z = v_src.z - 40;  // (72/2) + 4 units
            v_dest.z = v_dest.z - 40;
         }

         UTIL_TraceLine( v_src, v_dest, ignore_monsters,
                         pEdict->v.pContainingEntity, &tr );

         if (tr.flFraction < 1.0)
         {
            // hit something the bot is standing on...
            BotTurnAtWall( pBot, &tr, FALSE );
         }
         else
         {
            // pick a random direction to turn...
            if (RANDOM_LONG(1, 100) <= 50)
               direction = 1.0f;
            else
               direction = -1.0f;

            // turn 30 degrees at a time until bot is on solid ground
            v_ahead = pEdict->v.v_angle;
            v_ahead.x = 0;  // set pitch to level horizontally

            done = FALSE;
            turn_count = 0;

            while (!done)
            {
               v_ahead.y += 30.0 * direction;

               if (v_ahead.y > 360.0f)
                  v_ahead.y -= 360.0f;
               if (v_ahead.y < -360.0f)
                  v_ahead.y += 360.0f;

               UTIL_MakeVectors(v_ahead);

               v_src = pEdict->v.origin;
               v_dest = v_src + gpGlobals->v_forward * scale;

               UTIL_TraceLine( v_src, v_dest, ignore_monsters,
                               pEdict->v.pContainingEntity, &tr );

               // check if area in front of bot was clear...
               if (tr.flFraction >= 1.0) 
               {
                  v_src = v_dest;  // start downward trace from endpoint of open trace
                  v_dest.z = v_dest.z - max_drop_height;

                  UTIL_TraceLine( v_src, v_dest, ignore_monsters,
                                  pEdict->v.pContainingEntity, &tr );

                  // if trace hit something then drop is NOT TOO FAR...
                  if (tr.flFraction >= 1.0) 
                     done = TRUE;
               }

               turn_count++;
               if (turn_count == 6)  // 180 degrees? (30 * 6 = 180)
                  done = TRUE;
            }

            pBot->pEdict->v.ideal_yaw = v_ahead.y;
            BotFixIdealYaw(pEdict);
         }
      }
   }
}

