// HPB bot - botman's High Ping Bastard bot
//
// (http://planethalflife.com/botman/)
//
// waypoint.cpp
//

#ifndef __linux__
#include <io.h>
#endif
#include <fcntl.h>
#ifndef __linux__
#include <sys\stat.h>
#else
#include <sys/stat.h>
#endif

#include "extdll.h"
#include "enginecallback.h"
#include "util.h"
#include "cbase.h"

#include "bot.h"
#include "waypoint.h"


extern int mod_id;
extern int m_spriteTexture;
extern int IsDedicatedServer;

extern int num_backpacks;
extern BACKPACK_S backpacks[MAX_BACKPACKS];

// waypoints with information bits (flags)
WAYPOINT waypoints[MAX_WAYPOINTS];

// number of waypoints currently in use
int num_waypoints;

// declare the array of head pointers to the path structures...
PATH *paths[MAX_WAYPOINTS];

// time that this waypoint was displayed (while editing)
float wp_display_time[MAX_WAYPOINTS];

bool g_waypoint_paths = FALSE;  // have any paths been allocated?
bool g_waypoint_on = FALSE;
bool g_auto_waypoint = FALSE;
bool g_path_waypoint = FALSE;
bool g_path_waypoint_enable = TRUE;
Vector last_waypoint;
float f_path_time = 0.0;

unsigned int route_num_waypoints;
unsigned short *shortest_path[4] = {NULL, NULL, NULL, NULL};
unsigned short *from_to[4] = {NULL, NULL, NULL, NULL};

static FILE *fp;


void WaypointDebug(void)
{
   int y = 1, x = 1;

   fp=fopen("HPB_bot.txt","a");
   fprintf(fp,"WaypointDebug: LINKED LIST ERROR!!!\n");
   fclose(fp);

   x = x - 1;  // x is zero
   y = y / x;  // cause an divide by zero exception

   return;
}


// free the linked list of waypoint path nodes...
void WaypointFree(void)
{
   for (int i=0; i < MAX_WAYPOINTS; i++)
   {
      int count = 0;

      if (paths[i])
      {
         PATH *p = paths[i];
         PATH *p_next;

         while (p)  // free the linked list
         {
            p_next = p->next;  // save the link to next
            free(p);
            p = p_next;

#ifdef _DEBUG
            count++;
            if (count > 1000) WaypointDebug();
#endif
         }

         paths[i] = NULL;
      }
   }
}


// initialize the waypoint structures...
void WaypointInit(void)
{
   int i;

   // have any waypoint path nodes been allocated yet?
   if (g_waypoint_paths)
      WaypointFree();  // must free previously allocated path memory

   for (i=0; i < 4; i++)
   {
      if (shortest_path[i] != NULL)
         free(shortest_path[i]);

      if (from_to[i] != NULL)
         free(from_to[i]);
   }

   for (i=0; i < MAX_WAYPOINTS; i++)
   {
      waypoints[i].flags = 0;
      waypoints[i].origin = Vector(0,0,0);

      wp_display_time[i] = 0.0;

      paths[i] = NULL;  // no paths allocated yet
   }

   f_path_time = 0.0;  // reset waypoint path display time

   num_waypoints = 0;

   last_waypoint = Vector(0,0,0);

   for (i=0; i < 4; i++)
   {
      shortest_path[i] = NULL;
      from_to[i] = NULL;
   }
}


void WaypointAddPath(short int add_index, short int path_index)
{
   PATH *p, *prev;
   int i;
   int count = 0;

   p = paths[add_index];
   prev = NULL;

   // find an empty slot for new path_index...
   while (p != NULL)
   {
      i = 0;

      while (i < MAX_PATH_INDEX)
      {
         if (p->index[i] == -1)
         {
            p->index[i] = path_index;

            return;
         }

         i++;
      }

      prev = p;     // save the previous node in linked list
      p = p->next;  // go to next node in linked list

#ifdef _DEBUG
      count++;
      if (count > 100) WaypointDebug();
#endif
   }

   p = (PATH *)malloc(sizeof(PATH));

   if (p == NULL)
   {
      ALERT(at_error, "HPB_bot - Error allocating memory for path!");
   }

   p->index[0] = path_index;
   p->index[1] = -1;
   p->index[2] = -1;
   p->index[3] = -1;
   p->next = NULL;

   if (prev != NULL)
      prev->next = p;  // link new node into existing list

   if (paths[add_index] == NULL)
      paths[add_index] = p;  // save head point if necessary
}


void WaypointDeletePath(short int del_index)
{
   PATH *p;
   int index, i;

   // search all paths for this index...
   for (index=0; index < num_waypoints; index++)
   {
      p = paths[index];

      int count = 0;

      // search linked list for del_index...
      while (p != NULL)
      {
         i = 0;

         while (i < MAX_PATH_INDEX)
         {
            if (p->index[i] == del_index)
            {
               p->index[i] = -1;  // unassign this path
            }

            i++;
         }

         p = p->next;  // go to next node in linked list

#ifdef _DEBUG
         count++;
         if (count > 100) WaypointDebug();
#endif
      }
   }
}


void WaypointDeletePath(short int path_index, short int del_index)
{
   PATH *p;
   int i;
   int count = 0;

   p = paths[path_index];

   // search linked list for del_index...
   while (p != NULL)
   {
      i = 0;

      while (i < MAX_PATH_INDEX)
      {
         if (p->index[i] == del_index)
         {
            p->index[i] = -1;  // unassign this path
         }

         i++;
      }

      p = p->next;  // go to next node in linked list

#ifdef _DEBUG
      count++;
      if (count > 100) WaypointDebug();
#endif
   }
}


// find a path from the current waypoint. (pPath MUST be NULL on the
// initial call. subsequent calls will return other paths if they exist.)
int WaypointFindPath(PATH **pPath, int *path_index, int waypoint_index, int team)
{
   int index;
   int count = 0;

   if (*pPath == NULL)
   {
      *pPath = paths[waypoint_index];
      *path_index = 0;
   }

   if (*path_index == MAX_PATH_INDEX)
   {
      *path_index = 0;

      *pPath = (*pPath)->next;  // go to next node in linked list
   }

   while (*pPath != NULL)
   {
      while (*path_index < MAX_PATH_INDEX)
      {
         if ((*pPath)->index[*path_index] != -1)  // found a path?
         {
            // save the return value
            index = (*pPath)->index[*path_index];

            // skip this path if next waypoint is team specific and NOT this team
            if ((team != -1) && (waypoints[index].flags & W_FL_TEAM_SPECIFIC) &&
                ((waypoints[index].flags & W_FL_TEAM) != team))
            {
               (*path_index)++;
               continue;
            }

            // set up stuff for subsequent calls...
            (*path_index)++;

            return index;
         }

         (*path_index)++;
      }

      *path_index = 0;

      *pPath = (*pPath)->next;  // go to next node in linked list

#ifdef _DEBUG
      count++;
      if (count > 100) WaypointDebug();
#endif
   }

   return -1;
}

// find the nearest waypoint to the player and return the index (-1 if not found)
int WaypointFindNearest(edict_t *pEntity, float range, int team)
{
   int i, min_index;
   float distance;
   float min_distance;
   TraceResult tr;

   if (num_waypoints < 1)
      return -1;

   // find the nearest waypoint...

   min_index = -1;
   min_distance = 9999.0;

   for (i=0; i < num_waypoints; i++)
   {
      if (waypoints[i].flags & W_FL_DELETED)
         continue;  // skip any deleted waypoints

      if (waypoints[i].flags & W_FL_AIMING)
         continue;  // skip any aiming waypoints

      // skip this waypoint if it's team specific and teams don't match...
      if ((team != -1) && (waypoints[i].flags & W_FL_TEAM_SPECIFIC) &&
          ((waypoints[i].flags & W_FL_TEAM) != team))
         continue;

      distance = (waypoints[i].origin - pEntity->v.origin).Length();

      if ((distance < min_distance) && (distance < range))
      {
         // if waypoint is visible from current position (even behind head)...
         UTIL_TraceLine( pEntity->v.origin + pEntity->v.view_ofs, waypoints[i].origin,
                         ignore_monsters, pEntity->v.pContainingEntity, &tr );

         if (tr.flFraction >= 1.0)
         {
            min_index = i;
            min_distance = distance;
         }
      }
   }

   return min_index;
}


// find the nearest waypoint to the source postition and return the index
int WaypointFindNearest(Vector v_src, edict_t *pEntity, float range, int team)
{
   int index, min_index;
   float distance;
   float min_distance;
   TraceResult tr;

   if (num_waypoints < 1)
      return -1;

   // find the nearest waypoint...

   min_index = -1;
   min_distance = 9999.0;

   for (index=0; index < num_waypoints; index++)
   {
      if (waypoints[index].flags & W_FL_DELETED)
         continue;  // skip any deleted waypoints

      if (waypoints[index].flags & W_FL_AIMING)
         continue;  // skip any aiming waypoints

      // skip this waypoint if it's team specific and teams don't match...
      if ((team != -1) && (waypoints[index].flags & W_FL_TEAM_SPECIFIC) &&
          ((waypoints[index].flags & W_FL_TEAM) != team))
         continue;

      distance = (waypoints[index].origin - v_src).Length();

      if ((distance < min_distance) && (distance < range))
      {
         // if waypoint is visible from source position...
         UTIL_TraceLine( v_src, waypoints[index].origin, ignore_monsters,
                         pEntity->v.pContainingEntity, &tr );

         if (tr.flFraction >= 1.0)
         {
            min_index = index;
            min_distance = distance;
         }
      }
   }

   return min_index;
}


int WaypointFindNearestGoal(edict_t *pEntity, int src, int team, int flags)
{
   int index, min_index;
   int distance, min_distance;

   if (num_waypoints < 1)
      return -1;

   // find the nearest waypoint with the matching flags...

   min_index = -1;
   min_distance = 99999;

   for (index=0; index < num_waypoints; index++)
   {
      if (index == src)
         continue;  // skip the source waypoint

      if (waypoints[index].flags & W_FL_DELETED)
         continue;  // skip any deleted waypoints

      if (waypoints[index].flags & W_FL_AIMING)
         continue;  // skip any aiming waypoints

      // skip this waypoint if it's team specific and teams don't match...
      if ((team != -1) && (waypoints[index].flags & W_FL_TEAM_SPECIFIC) &&
          ((waypoints[index].flags & W_FL_TEAM) != team))
         continue;

      if ((waypoints[index].flags & flags) != flags)
         continue;  // skip this waypoint if the flags don't match

      distance = WaypointDistanceFromTo(src, index, team);

      if (distance < min_distance)
      {
         min_index = index;
         min_distance = distance;
      }
   }

   return min_index;
}


int WaypointFindNearestGoal(edict_t *pEntity, int src, int team, int flags, int exclude[])
{
   int index, min_index;
   int distance, min_distance;
   int exclude_index;

   if (num_waypoints < 1)
      return -1;

   // find the nearest waypoint with the matching flags...

   min_index = -1;
   min_distance = 99999;

   for (index=0; index < num_waypoints; index++)
   {
      if (index == src)
         continue;  // skip the source waypoint

      if (waypoints[index].flags & W_FL_DELETED)
         continue;  // skip any deleted waypoints

      if (waypoints[index].flags & W_FL_AIMING)
         continue;  // skip any aiming waypoints

      // skip this waypoint if it's team specific and teams don't match...
      if ((team != -1) && (waypoints[index].flags & W_FL_TEAM_SPECIFIC) &&
          ((waypoints[index].flags & W_FL_TEAM) != team))
         continue;

      if ((waypoints[index].flags & flags) != flags)
         continue;  // skip this waypoint if the flags don't match

      exclude_index = 0;
      while (exclude[exclude_index])
      {
         if (index == exclude[exclude_index])
            break;  // found a match, break out of while loop

         exclude_index++;
      }

      if (index == exclude[exclude_index])
         continue;  // skip any index that matches exclude list

      distance = WaypointDistanceFromTo(src, index, team);

      if (distance < min_distance)
      {
         min_index = index;
         min_distance = distance;
      }
   }

   return min_index;
}


int WaypointFindNearestGoal(Vector v_src, edict_t *pEntity, float range, int team, int flags)
{
   int index, min_index;
   int distance, min_distance;

   if (num_waypoints < 1)
      return -1;

   // find the nearest waypoint with the matching flags...

   min_index = -1;
   min_distance = 99999;

   for (index=0; index < num_waypoints; index++)
   {
      if (waypoints[index].flags & W_FL_DELETED)
         continue;  // skip any deleted waypoints

      if (waypoints[index].flags & W_FL_AIMING)
         continue;  // skip any aiming waypoints

      // skip this waypoint if it's team specific and teams don't match...
      if ((team != -1) && (waypoints[index].flags & W_FL_TEAM_SPECIFIC) &&
          ((waypoints[index].flags & W_FL_TEAM) != team))
         continue;

      if ((waypoints[index].flags & flags) != flags)
         continue;  // skip this waypoint if the flags don't match

      distance = (waypoints[index].origin - v_src).Length();

      if ((distance < range) && (distance < min_distance))
      {
         min_index = index;
         min_distance = distance;
      }
   }

   return min_index;
}


int WaypointFindRandomGoal(edict_t *pEntity, int team, int flags)
{
   int index;
   int indexes[200];
   int count = 0;

   if (num_waypoints < 1)
      return -1;

   // find all the waypoints with the matching flags...

   for (index=0; index < num_waypoints; index++)
   {
      if (waypoints[index].flags & W_FL_DELETED)
         continue;  // skip any deleted waypoints

      if (waypoints[index].flags & W_FL_AIMING)
         continue;  // skip any aiming waypoints

      // skip this waypoint if it's team specific and teams don't match...
      if ((team != -1) && (waypoints[index].flags & W_FL_TEAM_SPECIFIC) &&
          ((waypoints[index].flags & W_FL_TEAM) != team))
         continue;

      if ((waypoints[index].flags & flags) != flags)
         continue;  // skip this waypoint if the flags don't match

      if (count < 200)
      {
         indexes[count] = index;

         count++;
      }
   }

   if (count == 0)  // no matching waypoints found
      return -1;

   index = RANDOM_LONG(1, count) - 1;

   return indexes[index];
}


int WaypointFindRandomGoal(edict_t *pEntity, int team, int flags, int exclude[])
{
   int index;
   int indexes[200];
   int count = 0;
   int exclude_index;

   if (num_waypoints < 1)
      return -1;

   // find all the waypoints with the matching flags...

   for (index=0; index < num_waypoints; index++)
   {
      if (waypoints[index].flags & W_FL_DELETED)
         continue;  // skip any deleted waypoints

      if (waypoints[index].flags & W_FL_AIMING)
         continue;  // skip any aiming waypoints

      // skip this waypoint if it's team specific and teams don't match...
      if ((team != -1) && (waypoints[index].flags & W_FL_TEAM_SPECIFIC) &&
          ((waypoints[index].flags & W_FL_TEAM) != team))
         continue;

      if ((waypoints[index].flags & flags) != flags)
         continue;  // skip this waypoint if the flags don't match

      exclude_index = 0;
      while (exclude[exclude_index])
      {
         if (index == exclude[exclude_index])
            break;  // found a match, break out of while loop

         exclude_index++;
      }

      if (index == exclude[exclude_index])
         continue;  // skip any index that matches exclude list

      if (count < 200)
      {
         indexes[count] = index;

         count++;
      }
   }

   if (count == 0)  // no matching waypoints found
      return -1;

   index = RANDOM_LONG(1, count) - 1;

   return indexes[index];
}


int WaypointFindRandomGoal(Vector v_src, edict_t *pEntity, float range, int team, int flags)
{
   int index;
   int indexes[200];
   int count = 0;
   float distance;

   if (num_waypoints < 1)
      return -1;

   // find all the waypoints with the matching flags...

   for (index=0; index < num_waypoints; index++)
   {
      if (waypoints[index].flags & W_FL_DELETED)
         continue;  // skip any deleted waypoints

      if (waypoints[index].flags & W_FL_AIMING)
         continue;  // skip any aiming waypoints

      // skip this waypoint if it's team specific and teams don't match...
      if ((team != -1) && (waypoints[index].flags & W_FL_TEAM_SPECIFIC) &&
          ((waypoints[index].flags & W_FL_TEAM) != team))
         continue;

      if ((waypoints[index].flags & flags) != flags)
         continue;  // skip this waypoint if the flags don't match

      distance = (waypoints[index].origin - v_src).Length();

      if ((distance < range) && (count < 200))
      {
         indexes[count] = index;

         count++;
      }
   }

   if (count == 0)  // no matching waypoints found
      return -1;

   index = RANDOM_LONG(1, count) - 1;

   return indexes[index];
}


int WaypointFindNearestAiming(Vector v_origin)
{
   int index;
   int min_index = -1;
   int min_distance = 9999.0;
   float distance;

   if (num_waypoints < 1)
      return -1;

   // search for nearby aiming waypoint...
   for (index=0; index < num_waypoints; index++)
   {
      if (waypoints[index].flags & W_FL_DELETED)
         continue;  // skip any deleted waypoints

      if ((waypoints[index].flags & W_FL_AIMING) == 0)
         continue;  // skip any NON aiming waypoints

      distance = (v_origin - waypoints[index].origin).Length();

      if ((distance < min_distance) && (distance < 40))
      {
         min_index = index;
         min_distance = distance;
      }
   }

   return min_index;
}


void WaypointDrawBeam(edict_t *pEntity, Vector start, Vector end, int width,
        int noise, int red, int green, int blue, int brightness, int speed)
{
   MESSAGE_BEGIN(MSG_ONE, SVC_TEMPENTITY, NULL, pEntity);
   WRITE_BYTE( TE_BEAMPOINTS);
   WRITE_COORD(start.x);
   WRITE_COORD(start.y);
   WRITE_COORD(start.z);
   WRITE_COORD(end.x);
   WRITE_COORD(end.y);
   WRITE_COORD(end.z);
   WRITE_SHORT( m_spriteTexture );
   WRITE_BYTE( 1 ); // framestart
   WRITE_BYTE( 10 ); // framerate
   WRITE_BYTE( 10 ); // life in 0.1's
   WRITE_BYTE( width ); // width
   WRITE_BYTE( noise );  // noise

   WRITE_BYTE( red );   // r, g, b
   WRITE_BYTE( green );   // r, g, b
   WRITE_BYTE( blue );   // r, g, b

   WRITE_BYTE( brightness );   // brightness
   WRITE_BYTE( speed );    // speed
   MESSAGE_END();
}


void WaypointSearchItems(edict_t *pEntity, Vector origin, int wpt_index)
{
   edict_t *pent = NULL;
   float radius = 40;
   TraceResult tr;
   float distance;
   float min_distance;
   char item_name[64];
   char nearest_name[64];
   edict_t *nearest_pent;
   int bck_index;
   int tfc_backpack_index;

   nearest_name[0] = 0;      // null out nearest_name string
   tfc_backpack_index = -1;  // "null" out backpack index
   nearest_pent = NULL;

   min_distance = 9999.0;

   //********************************************************
   // look for the nearest health, armor, ammo, weapon, etc.
   //********************************************************

   while ((pent = UTIL_FindEntityInSphere( pent, origin, radius )) != NULL)
   {
      if (pEntity)
         UTIL_TraceLine( origin, pent->v.origin, ignore_monsters,
                         pEntity->v.pContainingEntity, &tr );
      else
         UTIL_TraceLine( origin, pent->v.origin, ignore_monsters, NULL, &tr );

      // make sure entity is visible...
      if (tr.flFraction >= 1.0)
      {
         strcpy(item_name, STRING(pent->v.classname));

         if ((strncmp("item_health", item_name, 11) == 0) ||
             (strncmp("item_armor", item_name, 10) == 0) ||
             (strncmp("ammo_", item_name, 5) == 0) ||
             (strcmp("item_cells", item_name) == 0) ||
             (strcmp("item_shells", item_name) == 0) ||
             (strcmp("item_spikes", item_name) == 0) ||
             (strcmp("item_rockets", item_name) == 0) ||
             ((strncmp("weapon_", item_name, 7) == 0) &&
              (pent->v.owner == NULL)))
         {
            distance = (pent->v.origin - origin).Length();

            if (distance < min_distance)
            {
               strcpy(nearest_name, item_name);

               tfc_backpack_index = -1;  // "null" out backpack index

               nearest_pent = pent;

               min_distance = distance;
            }
         }

         if (mod_id == TFC_DLL)
         {
            for (bck_index=0; bck_index < num_backpacks; bck_index++)
            {
               distance = (pent->v.origin - origin).Length();

               if ((backpacks[bck_index].edict == pent) &&
                   (distance < min_distance))
               {
                  tfc_backpack_index = bck_index;

                  nearest_pent = pent;

                  nearest_name[0] = 0;  // null out nearest_name string

                  min_distance = distance;

                  break;
               }
            }
         }
      }
   }

   if (nearest_name[0])  // found an entity name
   {
      if (strncmp("item_health", nearest_name, 11) == 0)
      {
         if (pEntity)
            ClientPrint(pEntity, HUD_PRINTCONSOLE, "found a healthkit!\n");
         waypoints[wpt_index].flags |= W_FL_HEALTH;
      }

      if (strncmp("item_armor", nearest_name, 10) == 0)
      {
         if (pEntity)
            ClientPrint(pEntity, HUD_PRINTCONSOLE, "found some armor!\n");
         waypoints[wpt_index].flags |= W_FL_ARMOR;
      }

      if ((strncmp("ammo_", nearest_name, 5) == 0) ||
          (strcmp("item_cells", nearest_name) == 0) ||
          (strcmp("item_shells", nearest_name) == 0) ||
          (strcmp("item_spikes", nearest_name) == 0) ||
          (strcmp("item_rockets", nearest_name) == 0))
      {
         if (pEntity)
            ClientPrint(pEntity, HUD_PRINTCONSOLE, "found some ammo!\n");
         waypoints[wpt_index].flags |= W_FL_AMMO;
      }

      if ((strncmp("weapon_", nearest_name, 7) == 0) &&
          (nearest_pent->v.owner == NULL))
      {
         if (pEntity)
            ClientPrint(pEntity, HUD_PRINTCONSOLE, "found a weapon!\n");
         waypoints[wpt_index].flags |= W_FL_WEAPON;
      }
   }

   if ((mod_id == TFC_DLL) &&
       (tfc_backpack_index != -1))  // found a TFC backpack
   {
      if (backpacks[tfc_backpack_index].health)
      {
         if (pEntity)
            ClientPrint(pEntity, HUD_PRINTCONSOLE, "found health!\n");
         waypoints[wpt_index].flags |= W_FL_HEALTH;
      }

      if (backpacks[tfc_backpack_index].armor)
      {
         if (pEntity)
            ClientPrint(pEntity, HUD_PRINTCONSOLE, "found some armor!\n");
         waypoints[wpt_index].flags |= W_FL_ARMOR;
      }

      if (backpacks[tfc_backpack_index].ammo)
      {
         if (pEntity)
            ClientPrint(pEntity, HUD_PRINTCONSOLE, "found some ammo!\n");
         waypoints[wpt_index].flags |= W_FL_AMMO;
      }
   }
}


void WaypointAdd(edict_t *pEntity)
{
   int index;

   if (num_waypoints >= MAX_WAYPOINTS)
      return;

   index = 0;

   // find the next available slot for the new waypoint...
   while (index < num_waypoints)
   {
      if (waypoints[index].flags & W_FL_DELETED)
         break;

      index++;
   }

   waypoints[index].flags = 0;

   // store the origin (location) of this waypoint (use entity origin)
   waypoints[index].origin = pEntity->v.origin;

   // store the last used waypoint for the auto waypoint code...
   last_waypoint = pEntity->v.origin;

   // set the time that this waypoint was originally displayed...
   wp_display_time[index] = gpGlobals->time;


   Vector start, end;

   start = pEntity->v.origin - Vector(0, 0, 34);
   end = start + Vector(0, 0, 68);

   if ((pEntity->v.flags & FL_DUCKING) == FL_DUCKING)
   {
      waypoints[index].flags |= W_FL_CROUCH;  // crouching waypoint

      start = pEntity->v.origin - Vector(0, 0, 17);
      end = start + Vector(0, 0, 34);
   }

   if (pEntity->v.movetype == MOVETYPE_FLY)
      waypoints[index].flags |= W_FL_LADDER;  // waypoint on a ladder


   // search the area near the waypoint for items (HEALTH, AMMO, WEAPON, etc.)
   WaypointSearchItems(pEntity, waypoints[index].origin, index);


   // draw a blue waypoint
   WaypointDrawBeam(pEntity, start, end, 30, 0, 0, 0, 255, 250, 5);

   EMIT_SOUND_DYN2(pEntity, CHAN_WEAPON, "weapons/xbow_hit1.wav", 1.0,
                   ATTN_NORM, 0, 100);

   // increment total number of waypoints if adding at end of array...
   if (index == num_waypoints)
      num_waypoints++;

   // calculate all the paths to this new waypoint
   for (int i=0; i < num_waypoints; i++)
   {
      if (i == index)
         continue;  // skip the waypoint that was just added

      if (waypoints[i].flags & W_FL_AIMING)
         continue;  // skip any aiming waypoints

      // check if the waypoint is reachable from the new one (one-way)
      if ( WaypointReachable(pEntity->v.origin, waypoints[i].origin, pEntity) &&
           g_path_waypoint_enable)
      {
         WaypointAddPath(index, i);
      }

      // check if the new one is reachable from the waypoint (other way)
      if ( WaypointReachable(waypoints[i].origin, pEntity->v.origin, pEntity) &&
           g_path_waypoint_enable)
      {
         WaypointAddPath(i, index);
      }
   }
}


void WaypointAddAiming(edict_t *pEntity)
{
   int index;
   edict_t *pent = NULL;

   if (num_waypoints >= MAX_WAYPOINTS)
      return;

   index = 0;

   // find the next available slot for the new waypoint...
   while (index < num_waypoints)
   {
      if (waypoints[index].flags & W_FL_DELETED)
         break;

      index++;
   }

   waypoints[index].flags = W_FL_AIMING;  // aiming waypoint

   Vector v_angle = pEntity->v.v_angle;

   v_angle.x = 0;  // reset pitch to horizontal
   v_angle.z = 0;  // reset roll to level

   UTIL_MakeVectors(v_angle);

   // store the origin (location) of this waypoint (use entity origin)
   waypoints[index].origin = pEntity->v.origin + gpGlobals->v_forward * 25;

   // set the time that this waypoint was originally displayed...
   wp_display_time[index] = gpGlobals->time;


   Vector start, end;

   start = pEntity->v.origin - Vector(0, 0, 10);
   end = start + Vector(0, 0, 14);

   // draw a blue waypoint
   WaypointDrawBeam(pEntity, start, end, 30, 0, 0, 0, 255, 250, 5);

   EMIT_SOUND_DYN2(pEntity, CHAN_WEAPON, "weapons/xbow_hit1.wav", 1.0,
                   ATTN_NORM, 0, 100);

   // increment total number of waypoints if adding at end of array...
   if (index == num_waypoints)
      num_waypoints++;
}


void WaypointDelete(edict_t *pEntity)
{
   int index;
   int count = 0;

   if (num_waypoints < 1)
      return;

   index = WaypointFindNearest(pEntity, 50.0, -1);

   if (index == -1)
      return;

   if ((waypoints[index].flags & W_FL_SNIPER) ||
       (waypoints[index].flags & W_FL_SENTRYGUN) ||
       (waypoints[index].flags & W_FL_DISPENSER) ||
       (waypoints[index].flags & W_FL_JUMP) ||
       ((mod_id == FRONTLINE_DLL) && (waypoints[index].flags & W_FL_FLF_DEFEND)))
   {
      int i;
      int min_index = -1;
      int min_distance = 9999.0;
      float distance;

      // search for nearby aiming waypoint and delete it also...
      for (i=0; i < num_waypoints; i++)
      {
         if (waypoints[i].flags & W_FL_DELETED)
            continue;  // skip any deleted waypoints

         if ((waypoints[i].flags & W_FL_AIMING) == 0)
            continue;  // skip any NON aiming waypoints

         distance = (waypoints[i].origin - waypoints[index].origin).Length();

         if ((distance < min_distance) && (distance < 40))
         {
            min_index = i;
            min_distance = distance;
         }
      }

      if (min_index != -1)
      {
         waypoints[min_index].flags = W_FL_DELETED;  // not being used
         waypoints[min_index].origin = Vector(0,0,0);

         wp_display_time[min_index] = 0.0;
      }
   }

   // delete any paths that lead to this index...
   WaypointDeletePath(index);

   // free the path for this index...

   if (paths[index] != NULL)
   {
      PATH *p = paths[index];
      PATH *p_next;

      while (p)  // free the linked list
      {
         p_next = p->next;  // save the link to next
         free(p);
         p = p_next;

#ifdef _DEBUG
         count++;
         if (count > 100) WaypointDebug();
#endif
      }

      paths[index] = NULL;
   }

   waypoints[index].flags = W_FL_DELETED;  // not being used
   waypoints[index].origin = Vector(0,0,0);

   wp_display_time[index] = 0.0;

   EMIT_SOUND_DYN2(pEntity, CHAN_WEAPON, "weapons/mine_activate.wav", 1.0,
                   ATTN_NORM, 0, 100);
}


void WaypointUpdate(edict_t *pEntity)
{
   int index;
   int mask;

   mask = W_FL_HEALTH | W_FL_ARMOR | W_FL_AMMO | W_FL_WEAPON;

   for (index=0; index < num_waypoints; index++)
   {
      waypoints[index].flags &= ~mask;  // clear the mask bits

      WaypointSearchItems(NULL, waypoints[index].origin, index);
   }
}


// allow player to manually create a path from one waypoint to another
void WaypointCreatePath(edict_t *pEntity, int cmd)
{
   static int waypoint1 = -1;  // initialized to unassigned
   static int waypoint2 = -1;  // initialized to unassigned

   if (cmd == 1)  // assign source of path
   {
      waypoint1 = WaypointFindNearest(pEntity, 50.0, -1);

      if (waypoint1 == -1)
      {
         // play "cancelled" sound...
         EMIT_SOUND_DYN2(pEntity, CHAN_WEAPON, "common/wpn_moveselect.wav", 1.0,
                         ATTN_NORM, 0, 100);

         return;
      }

      // play "start" sound...
      EMIT_SOUND_DYN2(pEntity, CHAN_WEAPON, "common/wpn_hudoff.wav", 1.0,
                      ATTN_NORM, 0, 100);

      return;
   }

   if (cmd == 2)  // assign dest of path and make path
   {
      waypoint2 = WaypointFindNearest(pEntity, 50.0, -1);

      if ((waypoint1 == -1) || (waypoint2 == -1))
      {
         // play "error" sound...
         EMIT_SOUND_DYN2(pEntity, CHAN_WEAPON, "common/wpn_denyselect.wav", 1.0,
                         ATTN_NORM, 0, 100);

         return;
      }

      WaypointAddPath(waypoint1, waypoint2);

      // play "done" sound...
      EMIT_SOUND_DYN2(pEntity, CHAN_WEAPON, "common/wpn_hudon.wav", 1.0,
                      ATTN_NORM, 0, 100);
   }
}


// allow player to manually remove a path from one waypoint to another
void WaypointRemovePath(edict_t *pEntity, int cmd)
{
   static int waypoint1 = -1;  // initialized to unassigned
   static int waypoint2 = -1;  // initialized to unassigned

   if (cmd == 1)  // assign source of path
   {
      waypoint1 = WaypointFindNearest(pEntity, 50.0, -1);

      if (waypoint1 == -1)
      {
         // play "cancelled" sound...
         EMIT_SOUND_DYN2(pEntity, CHAN_WEAPON, "common/wpn_moveselect.wav", 1.0,
                         ATTN_NORM, 0, 100);

         return;
      }

      // play "start" sound...
      EMIT_SOUND_DYN2(pEntity, CHAN_WEAPON, "common/wpn_hudoff.wav", 1.0,
                      ATTN_NORM, 0, 100);

      return;
   }

   if (cmd == 2)  // assign dest of path and make path
   {
      waypoint2 = WaypointFindNearest(pEntity, 50.0, -1);

      if ((waypoint1 == -1) || (waypoint2 == -1))
      {
         // play "error" sound...
         EMIT_SOUND_DYN2(pEntity, CHAN_WEAPON, "common/wpn_denyselect.wav", 1.0,
                         ATTN_NORM, 0, 100);

         return;
      }

      WaypointDeletePath(waypoint1, waypoint2);

      // play "done" sound...
      EMIT_SOUND_DYN2(pEntity, CHAN_WEAPON, "common/wpn_hudon.wav", 1.0,
                      ATTN_NORM, 0, 100);
   }
}


bool WaypointLoad(edict_t *pEntity)
{
   FILE *bfp;
   char mapname[64];
   char filename[256];
   char new_filename[256];
#ifdef __linux__
   char cmd[512];
#endif
   WAYPOINT_HDR header;
   char msg[80];
   int index, i;
   short int num;
   short int path_index;
   bool need_rename;

   strcpy(mapname, STRING(gpGlobals->mapname));
   strcat(mapname, ".HPB_wpt");

   UTIL_BuildFileName(filename, "maps", mapname);

   if (IsDedicatedServer)
      printf("loading waypoint file: %s\n", filename);

   bfp = fopen(filename, "rb");

   need_rename = FALSE;

   // if .HBP_wpt files doesn't exist, check .wpt file...
   if (bfp == NULL)
   {
      need_rename = TRUE;

      strcpy(mapname, STRING(gpGlobals->mapname));
      strcat(mapname, ".wpt");

      UTIL_BuildFileName(filename, "maps", mapname);

      if (IsDedicatedServer)
         printf("loading waypoint file: %s\n", filename);

      bfp = fopen(filename, "rb");
   }

   // if file exists, read the waypoint structure from it
   if (bfp != NULL)
   {
      fread(&header, sizeof(header), 1, bfp);

      header.filetype[7] = 0;
      if (strcmp(header.filetype, "HPB_bot") == 0)
      {
         if (header.waypoint_file_version != WAYPOINT_VERSION)
         {
            if (pEntity)
               ClientPrint(pEntity, HUD_PRINTNOTIFY, "Incompatible HPB bot waypoint file version!\nWaypoints not loaded!\n");

            fclose(bfp);
            return FALSE;
         }

         header.mapname[31] = 0;

         if (strcmp(header.mapname, STRING(gpGlobals->mapname)) == 0)
         {
            WaypointInit();  // remove any existing waypoints

            for (i=0; i < header.number_of_waypoints; i++)
            {
               fread(&waypoints[i], sizeof(waypoints[0]), 1, bfp);
               num_waypoints++;
            }

            // read and add waypoint paths...
            for (index=0; index < num_waypoints; index++)
            {
               // read the number of paths from this node...
               fread(&num, sizeof(num), 1, bfp);

               for (i=0; i < num; i++)
               {
                  fread(&path_index, sizeof(path_index), 1, bfp);

                  WaypointAddPath(index, path_index);
               }
            }

            g_waypoint_paths = TRUE;  // keep track so path can be freed
         }
         else
         {
            if (pEntity)
            {
               sprintf(msg, "%s HPB bot waypoints are not for this map!\n", filename);
               ClientPrint(pEntity, HUD_PRINTNOTIFY, msg);
            }

            fclose(bfp);
            return FALSE;
         }
      }
      else
      {
         if (pEntity)
         {
            sprintf(msg, "%s is not a HPB bot waypoint file!\n", filename);
            ClientPrint(pEntity, HUD_PRINTNOTIFY, msg);
         }

         fclose(bfp);
         return FALSE;
      }

      fclose(bfp);

      if (need_rename)
      {
         strcpy(mapname, STRING(gpGlobals->mapname));
         strcat(mapname, ".HPB_wpt");

         UTIL_BuildFileName(new_filename, "maps", mapname);

#ifndef __linux__
         rename(filename, new_filename);
#else
         sprintf(cmd, "/bin/mv -f %s %s", filename, new_filename);
         system(cmd);
#endif
      }

      WaypointRouteInit();
   }
   else
   {
      if (pEntity)
      {
         sprintf(msg, "Waypoint file %s does not exist!\n", filename);
         ClientPrint(pEntity, HUD_PRINTNOTIFY, msg);
      }

      if (IsDedicatedServer)
         printf("waypoint file %s not found!\n", filename);

      return FALSE;
   }

   return TRUE;
}


void WaypointSave(void)
{
   char filename[256];
   char mapname[64];
   WAYPOINT_HDR header;
   int index, i;
   short int num;
   PATH *p;

   strcpy(header.filetype, "HPB_bot");

   header.waypoint_file_version = WAYPOINT_VERSION;

   header.waypoint_file_flags = 0;  // not currently used

   header.number_of_waypoints = num_waypoints;

   memset(header.mapname, 0, sizeof(header.mapname));
   strncpy(header.mapname, STRING(gpGlobals->mapname), 31);
   header.mapname[31] = 0;

   strcpy(mapname, STRING(gpGlobals->mapname));
   strcat(mapname, ".HPB_wpt");

   UTIL_BuildFileName(filename, "maps", mapname);

   FILE *bfp = fopen(filename, "wb");

   // write the waypoint header to the file...
   fwrite(&header, sizeof(header), 1, bfp);

   // write the waypoint data to the file...
   for (index=0; index < num_waypoints; index++)
   {
      fwrite(&waypoints[index], sizeof(waypoints[0]), 1, bfp);
   }

   // save the waypoint paths...
   for (index=0; index < num_waypoints; index++)
   {
      // count the number of paths from this node...

      p = paths[index];
      num = 0;

      while (p != NULL)
      {
         i = 0;

         while (i < MAX_PATH_INDEX)
         {
            if (p->index[i] != -1)
               num++;  // count path node if it's used

            i++;
         }

         p = p->next;  // go to next node in linked list
      }

      fwrite(&num, sizeof(num), 1, bfp);  // write the count

      // now write out each path index...

      p = paths[index];

      while (p != NULL)
      {
         i = 0;

         while (i < MAX_PATH_INDEX)
         {
            if (p->index[i] != -1)  // save path node if it's used
               fwrite(&p->index[i], sizeof(p->index[0]), 1, bfp);

            i++;
         }

         p = p->next;  // go to next node in linked list
      }
   }

   fclose(bfp);
}


bool WaypointReachable(Vector v_src, Vector v_dest, edict_t *pEntity)
{
   TraceResult tr;
   float curr_height, last_height;

   float distance = (v_dest - v_src).Length();

   // is the destination close enough?
   if (distance < REACHABLE_RANGE)
   {
      // check if this waypoint is "visible"...

      UTIL_TraceLine( v_src, v_dest, ignore_monsters,
                      pEntity->v.pContainingEntity, &tr );

      // if waypoint is visible from current position (even behind head)...
      if (tr.flFraction >= 1.0)
      {
         // check for special case of both waypoints being underwater...
         if ((POINT_CONTENTS( v_src ) == CONTENTS_WATER) &&
             (POINT_CONTENTS( v_dest ) == CONTENTS_WATER))
         {
            return TRUE;
         }

         // check for special case of waypoint being suspended in mid-air...

         // is dest waypoint higher than src? (45 is max jump height)
         if (v_dest.z > (v_src.z + 45.0))
         {
            Vector v_new_src = v_dest;
            Vector v_new_dest = v_dest;

            v_new_dest.z = v_new_dest.z - 50;  // straight down 50 units

            UTIL_TraceLine(v_new_src, v_new_dest, dont_ignore_monsters,
                           pEntity->v.pContainingEntity, &tr);

            // check if we didn't hit anything, if not then it's in mid-air
            if (tr.flFraction >= 1.0)
            {
               return FALSE;  // can't reach this one
            }
         }

         // check if distance to ground increases more than jump height
         // at points between source and destination...

         Vector v_direction = (v_dest - v_src).Normalize();  // 1 unit long
         Vector v_check = v_src;
         Vector v_down = v_src;

         v_down.z = v_down.z - 1000.0;  // straight down 1000 units

         UTIL_TraceLine(v_check, v_down, ignore_monsters,
                        pEntity->v.pContainingEntity, &tr);

         last_height = tr.flFraction * 1000.0;  // height from ground

         distance = (v_dest - v_check).Length();  // distance from goal

         while (distance > 10.0)
         {
            // move 10 units closer to the goal...
            v_check = v_check + (v_direction * 10.0);

            v_down = v_check;
            v_down.z = v_down.z - 1000.0;  // straight down 1000 units

            UTIL_TraceLine(v_check, v_down, ignore_monsters,
                           pEntity->v.pContainingEntity, &tr);

            curr_height = tr.flFraction * 1000.0;  // height from ground

            // is the difference in the last height and the current height
            // higher that the jump height?
            if ((last_height - curr_height) > 45.0)
            {
               // can't get there from here...
               return FALSE;
            }

            last_height = curr_height;

            distance = (v_dest - v_check).Length();  // distance from goal
         }

         return TRUE;
      }
   }

   return FALSE;
}


// find the nearest reachable waypoint
int WaypointFindReachable(edict_t *pEntity, float range, int team)
{
   int i, min_index;
   float distance;
   float min_distance;
   TraceResult tr;

   // find the nearest waypoint...

   min_distance = 9999.0;

   for (i=0; i < num_waypoints; i++)
   {
      if (waypoints[i].flags & W_FL_DELETED)
         continue;  // skip any deleted waypoints

      if (waypoints[i].flags & W_FL_AIMING)
         continue;  // skip any aiming waypoints

      // skip this waypoint if it's team specific and teams don't match...
      if ((team != -1) && (waypoints[i].flags & W_FL_TEAM_SPECIFIC) &&
          ((waypoints[i].flags & W_FL_TEAM) != team))
         continue;

      distance = (waypoints[i].origin - pEntity->v.origin).Length();

      if (distance < min_distance)
      {
         // if waypoint is visible from current position (even behind head)...
         UTIL_TraceLine( pEntity->v.origin + pEntity->v.view_ofs, waypoints[i].origin,
                         ignore_monsters, pEntity->v.pContainingEntity, &tr );

         if (tr.flFraction >= 1.0)
         {
            if (WaypointReachable(pEntity->v.origin, waypoints[i].origin, pEntity))
            {
               min_index = i;
               min_distance = distance;
            }
         }
      }
   }

   // if not close enough to a waypoint then just return
   if (min_distance > range)
      return -1;

   return min_index;

}


void WaypointPrintInfo(edict_t *pEntity)
{
   char msg[80];
   int index;
   int flags;

   // find the nearest waypoint...
   index = WaypointFindNearest(pEntity, 50.0, -1);

   if (index == -1)
      return;

   sprintf(msg,"Waypoint %d of %d total\n", index, num_waypoints);
   ClientPrint(pEntity, HUD_PRINTNOTIFY, msg);

   flags = waypoints[index].flags;

   if (flags & W_FL_TEAM_SPECIFIC)
   {
      if (mod_id == FRONTLINE_DLL)
      {
         if ((flags & W_FL_TEAM) == 0)
            strcpy(msg, "Waypoint is for Attackers\n");
         else if ((flags & W_FL_TEAM) == 1)
            strcpy(msg, "Waypoint is for Defenders\n");
      }
      else
      {
         if ((flags & W_FL_TEAM) == 0)
            strcpy(msg, "Waypoint is for TEAM 1\n");
         else if ((flags & W_FL_TEAM) == 1)
            strcpy(msg, "Waypoint is for TEAM 2\n");
         else if ((flags & W_FL_TEAM) == 2)
            strcpy(msg, "Waypoint is for TEAM 3\n");
         else if ((flags & W_FL_TEAM) == 3)
            strcpy(msg, "Waypoint is for TEAM 4\n");
      }

      ClientPrint(pEntity, HUD_PRINTNOTIFY, msg);
   }

   if (flags & W_FL_LIFT)
      ClientPrint(pEntity, HUD_PRINTNOTIFY, "Bot will wait for lift before approaching\n");

   if (flags & W_FL_LADDER)
      ClientPrint(pEntity, HUD_PRINTNOTIFY, "This waypoint is on a ladder\n");

   if (flags & W_FL_DOOR)
      ClientPrint(pEntity, HUD_PRINTNOTIFY, "This is a door waypoint\n");

   if (flags & W_FL_HEALTH)
      ClientPrint(pEntity, HUD_PRINTNOTIFY, "There is health near this waypoint\n");

   if (flags & W_FL_ARMOR)
      ClientPrint(pEntity, HUD_PRINTNOTIFY, "There is armor near this waypoint\n");

   if (flags & W_FL_AMMO)
      ClientPrint(pEntity, HUD_PRINTNOTIFY, "There is ammo near this waypoint\n");

   if (flags & W_FL_WEAPON)
      ClientPrint(pEntity, HUD_PRINTNOTIFY, "There is a weapon near this waypoint\n");

   if (flags & W_FL_JUMP)
      ClientPrint(pEntity, HUD_PRINTNOTIFY, "Bot will jump here\n");

   if (flags & W_FL_SNIPER)
      ClientPrint(pEntity, HUD_PRINTNOTIFY, "This is a sniper waypoint\n");

   if (flags & W_FL_FLAG)
   {
      if (mod_id == FRONTLINE_DLL)
         ClientPrint(pEntity, HUD_PRINTNOTIFY, "There is a capture point near this waypoint\n");
      else if (mod_id == HOLYWARS_DLL)
         ClientPrint(pEntity, HUD_PRINTNOTIFY, "There is a halo spawn point near this waypoint\n");
      else
         ClientPrint(pEntity, HUD_PRINTNOTIFY, "There is a flag near this waypoint\n");
   }

   if (flags & W_FL_FLAG_GOAL)
   {
      if (mod_id == FRONTLINE_DLL)
         ClientPrint(pEntity, HUD_PRINTNOTIFY, "This is a defender location\n");
      else
         ClientPrint(pEntity, HUD_PRINTNOTIFY, "There is a flag goal near this waypoint\n");
   }

   if (flags & W_FL_PRONE)
      ClientPrint(pEntity, HUD_PRINTNOTIFY, "Bot will go prone here\n");

   if (flags & W_FL_SENTRYGUN)
      ClientPrint(pEntity, HUD_PRINTNOTIFY, "Engineers will build a sentry gun here\n");

   if (flags & W_FL_DISPENSER)
      ClientPrint(pEntity, HUD_PRINTNOTIFY, "Engineers will build a dispenser here\n");
}


void WaypointThink(edict_t *pEntity)
{
   float distance, min_distance;
   Vector start, end;
   int i, index;

   if (g_auto_waypoint)  // is auto waypoint on?
   {
      // find the distance from the last used waypoint
      distance = (last_waypoint - pEntity->v.origin).Length();

      if (distance > 200)
      {
         min_distance = 9999.0;

         // check that no other reachable waypoints are nearby...
         for (i=0; i < num_waypoints; i++)
         {
            if (waypoints[i].flags & W_FL_DELETED)
               continue;

            if (waypoints[i].flags & W_FL_AIMING)
               continue;

            if (WaypointReachable(pEntity->v.origin, waypoints[i].origin, pEntity))
            {
               distance = (waypoints[i].origin - pEntity->v.origin).Length();

               if (distance < min_distance)
                  min_distance = distance;
            }
         }

         // make sure nearest waypoint is far enough away...
         if (min_distance >= 200)
            WaypointAdd(pEntity);  // place a waypoint here
      }
   }

   min_distance = 9999.0;

   if (g_waypoint_on)  // display the waypoints if turned on...
   {
      for (i=0; i < num_waypoints; i++)
      {
         if ((waypoints[i].flags & W_FL_DELETED) == W_FL_DELETED)
            continue;

         distance = (waypoints[i].origin - pEntity->v.origin).Length();

         if (distance < 500)
         {
            if (distance < min_distance)
            {
               index = i; // store index of nearest waypoint
               min_distance = distance;
            }

            if ((wp_display_time[i] + 1.0) < gpGlobals->time)
            {
               if (waypoints[i].flags & W_FL_CROUCH)
               {
                  start = waypoints[i].origin - Vector(0, 0, 17);
                  end = start + Vector(0, 0, 34);
               }
               else if (waypoints[i].flags & W_FL_AIMING)
               {
                  start = waypoints[i].origin + Vector(0, 0, 10);
                  end = start + Vector(0, 0, 14);
               }
               else
               {
                  start = waypoints[i].origin - Vector(0, 0, 34);
                  end = start + Vector(0, 0, 68);
               }

               // draw a blue waypoint
               WaypointDrawBeam(pEntity, start, end, 30, 0, 0, 0, 255, 250, 5);

               wp_display_time[i] = gpGlobals->time;
            }
         }
      }

      // check if path waypointing is on...
      if (g_path_waypoint)
      {
         // check if player is close enough to a waypoint and time to draw path...
         if ((min_distance <= 50) && (f_path_time <= gpGlobals->time))
         {
            PATH *p;

            f_path_time = gpGlobals->time + 1.0;

            p = paths[index];

            while (p != NULL)
            {
               i = 0;

               while (i < MAX_PATH_INDEX)
               {
                  if (p->index[i] != -1)
                  {
                     Vector v_src = waypoints[index].origin;
                     Vector v_dest = waypoints[p->index[i]].origin;

                     // draw a white line to this index's waypoint
                     WaypointDrawBeam(pEntity, v_src, v_dest, 10, 2, 250, 250, 250, 200, 10);
                  }

                  i++;
               }

               p = p->next;  // go to next node in linked list
            }
         }
      }
   }
}


void WaypointFloyds(unsigned short *shortest_path, unsigned short *from_to)
{
   unsigned int x, y, z;
   int changed = 1;
   int distance;

   for (y=0; y < route_num_waypoints; y++)
   {
      for (z=0; z < route_num_waypoints; z++)
      {
         from_to[y * route_num_waypoints + z] = z;
      }
   }

   while (changed)
   {
      changed = 0;

      for (x=0; x < route_num_waypoints; x++)
      {
         for (y=0; y < route_num_waypoints; y++)
         {
            for (z=0; z < route_num_waypoints; z++)
            {
               if ((shortest_path[y * route_num_waypoints + x] == WAYPOINT_UNREACHABLE) ||
                   (shortest_path[x * route_num_waypoints + z] == WAYPOINT_UNREACHABLE))
                  continue;

               distance = shortest_path[y * route_num_waypoints + x] +
                          shortest_path[x * route_num_waypoints + z];

               if (distance > WAYPOINT_MAX_DISTANCE)
                  distance = WAYPOINT_MAX_DISTANCE;

               if ((distance < shortest_path[y * route_num_waypoints + z]) ||
                   (shortest_path[y * route_num_waypoints + z] == WAYPOINT_UNREACHABLE))
               {
                  shortest_path[y * route_num_waypoints + z] = distance;
                  from_to[y * route_num_waypoints + z] = from_to[y * route_num_waypoints + x];
                  changed = 1;
               }
            }
         }
      }
   }
}


void WaypointRouteInit(void)
{
   unsigned int index;
   bool build_matrix[4];
   int matrix;
   unsigned int array_size;
   unsigned int row;
   int i, offset;
   unsigned int a, b;
   float distance;
   unsigned short *pShortestPath, *pFromTo;
   char msg[80];
   unsigned int num_items;
   FILE *bfp;
   char filename[256];
   char filename2[256];
   char mapname[64];

   if (num_waypoints == 0)
      return;

   // save number of current waypoints in case waypoints get added later
   route_num_waypoints = num_waypoints;

   strcpy(mapname, STRING(gpGlobals->mapname));
   strcat(mapname, ".HPB_wpt");

   UTIL_BuildFileName(filename, "maps", mapname);

   build_matrix[0] = TRUE;  // always build matrix 0 (non-team and team 1)
   build_matrix[1] = FALSE;
   build_matrix[2] = FALSE;
   build_matrix[3] = FALSE;

   // find out how many route matrixes to create...
   for (index=0; index < route_num_waypoints; index++)
   {
      if (waypoints[index].flags & W_FL_TEAM_SPECIFIC)
      {
         if ((waypoints[index].flags & W_FL_TEAM) == 0x01)  // team 2?
            build_matrix[1] = TRUE;

         if ((waypoints[index].flags & W_FL_TEAM) == 0x02)  // team 3?
            build_matrix[2] = TRUE;

         if ((waypoints[index].flags & W_FL_TEAM) == 0x03)  // team 4?
            build_matrix[3] = TRUE;
      }
   }

   array_size = route_num_waypoints * route_num_waypoints;

   for (matrix=0; matrix < 4; matrix++)
   {
      if (build_matrix[matrix])
      {
         char ext_str[16];  // ".HPB_wpX\0"
         int file1, file2;
         struct stat stat1, stat2;

         sprintf(ext_str, ".HPB_wp%d", matrix+1);

         strcpy(mapname, STRING(gpGlobals->mapname));
         strcat(mapname, ext_str);

         UTIL_BuildFileName(filename2, "maps", mapname);

         if (access(filename2, 0) == 0)  // does the .HPB_wpX file exist?
         {
            file1 = open(filename, O_RDONLY);
            file2 = open(filename2, O_RDONLY);

            fstat(file1, &stat1);
            fstat(file2, &stat2);

            close(file1);
            close(file2);

            if (stat1.st_mtime < stat2.st_mtime)  // is .HPB_wpt older than .HPB_wpX file?
            {
               sprintf(msg, "loading HPB bot waypoint paths for team %d\n", matrix+1);
               ALERT(at_console, msg);

               shortest_path[matrix] = (unsigned short *)malloc(sizeof(unsigned short) * array_size);

               if (shortest_path[matrix] == NULL)
                  ALERT(at_error, "HPB_bot - Error allocating memory for shortest path!");

               from_to[matrix] = (unsigned short *)malloc(sizeof(unsigned short) * array_size);

               if (from_to[matrix] == NULL)
                  ALERT(at_error, "HPB_bot - Error allocating memory for from to matrix!");

               bfp = fopen(filename2, "rb");

               if (bfp != NULL)
               {
                  num_items = fread(shortest_path[matrix], sizeof(unsigned short), array_size, bfp);

                  if (num_items != array_size)
                  {
                     // if couldn't read enough data, free memory to recalculate it

                     ALERT(at_console, "error reading enough path items, recalculating...\n");

                     free(shortest_path[matrix]);
                     shortest_path[matrix] = NULL;

                     free(from_to[matrix]);
                     from_to[matrix] = NULL;
                  }
                  else
                  {
                     num_items = fread(from_to[matrix], sizeof(unsigned short), array_size, bfp);

                     if (num_items != array_size)
                     {
                        // if couldn't read enough data, free memory to recalculate it

                        ALERT(at_console, "error reading enough path items, recalculating...\n");

                        free(shortest_path[matrix]);
                        shortest_path[matrix] = NULL;

                        free(from_to[matrix]);
                        from_to[matrix] = NULL;
                     }
                  }
               }
               else
               {
                  ALERT(at_console, "HPB_bot - Error reading waypoint paths!\n");

                  free(shortest_path[matrix]);
                  shortest_path[matrix] = NULL;

                  free(from_to[matrix]);
                  from_to[matrix] = NULL;
               }

               fclose(bfp);
            }
         }

         if (shortest_path[matrix] == NULL)
         {
            sprintf(msg, "calculating HPB bot waypoint paths for team %d...\n", matrix+1);
            ALERT(at_console, msg);

            shortest_path[matrix] = (unsigned short *)malloc(sizeof(unsigned short) * array_size);

            if (shortest_path[matrix] == NULL)
               ALERT(at_error, "HPB_bot - Error allocating memory for shortest path!");

            from_to[matrix] = (unsigned short *)malloc(sizeof(unsigned short) * array_size);

            if (from_to[matrix] == NULL)
               ALERT(at_error, "HPB_bot - Error allocating memory for from to matrix!");

            pShortestPath = shortest_path[matrix];
            pFromTo = from_to[matrix];

            for (index=0; index < array_size; index++)
               pShortestPath[index] = WAYPOINT_UNREACHABLE;

            for (index=0; index < route_num_waypoints; index++)
               pShortestPath[index * route_num_waypoints + index] = 0;  // zero diagonal

            for (row=0; row < route_num_waypoints; row++)
            {
               if (paths[row] != NULL)
               {
                  PATH *p = paths[row];

                  while (p)
                  {
                     i = 0;

                     while (i < MAX_PATH_INDEX)
                     {
                        if (p->index[i] != -1)
                        {
                           index = p->index[i];

                           // check if this is NOT team specific OR matches this team
                           if (!(waypoints[index].flags & W_FL_TEAM_SPECIFIC) ||
                               ((waypoints[index].flags & W_FL_TEAM) == matrix))
                           {
                              distance = (waypoints[row].origin - waypoints[index].origin).Length();

                              if (distance > (float)WAYPOINT_MAX_DISTANCE)
                                 distance = (float)WAYPOINT_MAX_DISTANCE;

                              if (distance > REACHABLE_RANGE)
                              {
                                 sprintf(msg, "Waypoint path distance > %4.1f at from %d to %d\n",
                                              REACHABLE_RANGE, row, index);
                                 ALERT(at_console, msg);
                              }
                              else
                              {
                                 offset = row * route_num_waypoints + index;

                                 pShortestPath[offset] = (unsigned short)distance;
                              }
                           }
                        }

                        i++;
                     }

                     p = p->next;  // go to next node in linked list
                  }
               }
            }

            // run Floyd's Algorithm to generate the from_to matrix...
            WaypointFloyds(pShortestPath, pFromTo);

            for (a=0; a < route_num_waypoints; a++)
            {
               for (b=0; b < route_num_waypoints; b++)
                  if (pShortestPath[a * route_num_waypoints + b] == WAYPOINT_UNREACHABLE)
                    pFromTo[a * route_num_waypoints + b] = WAYPOINT_UNREACHABLE;
            }

            bfp = fopen(filename2, "wb");

            if (bfp != NULL)
            {
               num_items = fwrite(shortest_path[matrix], sizeof(unsigned short), array_size, bfp);

               if (num_items != array_size)
               {
                  // if couldn't write enough data, close file and delete it

                  fclose(bfp);
                  unlink(filename2);
               }
               else
               {
                  num_items = fwrite(from_to[matrix], sizeof(unsigned short), array_size, bfp);

                  fclose(bfp);

                  if (num_items != array_size)
                  {
                     // if couldn't write enough data, delete file
                     unlink(filename2);
                  }
               }
            }
            else
            {
               ALERT(at_console, "HPB_bot - Error writing waypoint paths!\n");
            }

            sprintf(msg, "HPB bot waypoint path calculations for team %d complete!\n",matrix+1);
            ALERT(at_console, msg);
         }
      }
   }

}


unsigned short WaypointRouteFromTo(int src, int dest, int team)
{
   unsigned short *pFromTo;

   if ((team < -1) || (team > 3))
      return -1;

   if (team == -1)  // -1 means non-team play
      team = 0;

   if (from_to[team] == NULL)  // if no team specific waypoints use team 0
      team = 0;

   if (from_to[team] == NULL)  // if no route information just return
      return -1;

   pFromTo = from_to[team];

   return pFromTo[src * route_num_waypoints + dest];
}


int WaypointDistanceFromTo(int src, int dest, int team)
{
   unsigned short *pShortestPath;

   if ((team < -1) || (team > 3))
      return -1;

   if (team == -1)  // -1 means non-team play
      team = 0;

   if (from_to[team] == NULL)  // if no team specific waypoints use team 0
      team = 0;

   if (from_to[team] == NULL)  // if no route information just return
      return -1;

   pShortestPath = shortest_path[team];

   return (int)(pShortestPath[src * route_num_waypoints + dest]);
}

