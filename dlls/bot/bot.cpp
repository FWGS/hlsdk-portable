// botman's Half-Life bot example
//
// http://planethalflife.com/botman/
//
// bot.cpp
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

#include <sys/types.h>
#include <sys/stat.h>

extern DLL_GLOBAL ULONG g_ulModelIndexPlayer;

// Set in combat.cpp.  Used to pass the damage inflictor for death messages.
extern entvars_t *g_pevLastInflictor;

extern DLL_GLOBAL BOOL g_fGameOver;
extern int gmsgHealth;
extern int gmsgCurWeapon;
extern int gmsgSetFOV;

#define PLAYER_SEARCH_RADIUS (float)40

int f_Observer = 0;  // flag to indicate if player is in observer mode
int f_botskill = 3;  // default bot skill level
int f_botdontshoot = 0;

// array of bot respawn information
respawn_t bot_respawn[32] = {
   {FALSE, BOT_IDLE, "", "", "", NULL}, {FALSE, BOT_IDLE, "", "", "", NULL},
   {FALSE, BOT_IDLE, "", "", "", NULL}, {FALSE, BOT_IDLE, "", "", "", NULL},
   {FALSE, BOT_IDLE, "", "", "", NULL}, {FALSE, BOT_IDLE, "", "", "", NULL},
   {FALSE, BOT_IDLE, "", "", "", NULL}, {FALSE, BOT_IDLE, "", "", "", NULL},
   {FALSE, BOT_IDLE, "", "", "", NULL}, {FALSE, BOT_IDLE, "", "", "", NULL},
   {FALSE, BOT_IDLE, "", "", "", NULL}, {FALSE, BOT_IDLE, "", "", "", NULL},
   {FALSE, BOT_IDLE, "", "", "", NULL}, {FALSE, BOT_IDLE, "", "", "", NULL},
   {FALSE, BOT_IDLE, "", "", "", NULL}, {FALSE, BOT_IDLE, "", "", "", NULL},
   {FALSE, BOT_IDLE, "", "", "", NULL}, {FALSE, BOT_IDLE, "", "", "", NULL},
   {FALSE, BOT_IDLE, "", "", "", NULL}, {FALSE, BOT_IDLE, "", "", "", NULL},
   {FALSE, BOT_IDLE, "", "", "", NULL}, {FALSE, BOT_IDLE, "", "", "", NULL},
   {FALSE, BOT_IDLE, "", "", "", NULL}, {FALSE, BOT_IDLE, "", "", "", NULL},
   {FALSE, BOT_IDLE, "", "", "", NULL}, {FALSE, BOT_IDLE, "", "", "", NULL},
   {FALSE, BOT_IDLE, "", "", "", NULL}, {FALSE, BOT_IDLE, "", "", "", NULL},
   {FALSE, BOT_IDLE, "", "", "", NULL}, {FALSE, BOT_IDLE, "", "", "", NULL}};

#define MAX_SKINS 10

// indicate which models are currently used for random model allocation
BOOL skin_used[MAX_SKINS] = {
   FALSE, FALSE, FALSE, FALSE, FALSE,
   FALSE, FALSE, FALSE, FALSE, FALSE};

// store the names of the models...
char *bot_skins[MAX_SKINS] = {
   "barney", "gina", "gman", "gordon", "helmet",
   "hgrunt", "recon", "robo", "scientist", "zombie"};

// store the player names for each of the models...
char *bot_names[MAX_SKINS] = {
   "Barney", "Gina", "G-Man", "Gordon", "Helmet",
   "H-Grunt", "Recon", "Robo", "Scientist", "Zombie"};

// how often (out of 1000 times) the bot will pause, based on bot skill
float pause_frequency[5] = {4, 7, 10, 15, 20};

// how long the bot will delay when paused, based on bot skill
float pause_time[5][2] = {
   {0.2, 0.5}, {0.5, 1.0}, {0.7, 1.3}, {1.0, 1.7}, {1.2, 2.0}};

extern ammo_check_t ammo_check[];

// sounds for TakeDamage speaking effects...
char hgrunt_sounds[][30] = { HG_SND1, HG_SND2, HG_SND3, HG_SND4, HG_SND5 };
char barney_sounds[][30] = { BA_SND1, BA_SND2, BA_SND3, BA_SND4, BA_SND5 };
char scientist_sounds[][30] = { SC_SND1, SC_SND2, SC_SND3, SC_SND4, SC_SND5 };


LINK_ENTITY_TO_CLASS( bot, CBot );


inline edict_t *CREATE_FAKE_CLIENT( const char *netname )
{
   return (*g_engfuncs.pfnCreateFakeClient)( netname );
}

inline char *GET_INFOBUFFER( edict_t *e )
{
   return (*g_engfuncs.pfnGetInfoKeyBuffer)( e );
}

inline char *GET_INFO_KEY_VALUE( char *infobuffer, char *key )
{
   return (g_engfuncs.pfnInfoKeyValue( infobuffer, key ));
}

inline void SET_CLIENT_KEY_VALUE( int clientIndex, char *infobuffer,
                                  char *key, char *value )
{
   (*g_engfuncs.pfnSetClientKeyValue)( clientIndex, infobuffer, key, value );
}


void BotDebug( char *buffer )
{
   // print out debug messages to the HUD of all players
   // this allows you to print messages from bots to your display
   // as you are playing the game.  Use STRING(pev->netname) in
   // buffer to see the name of the bot.

   UTIL_ClientPrintAll( HUD_PRINTNOTIFY, buffer );
}


void BotCreate(const char *skin, const char *name, const char *skill)
{
   edict_t *BotEnt;
   CBot *BotClass;
   int skill_level;
   char c_skill[2];
   char c_skin[BOT_SKIN_LEN+1];
   char c_name[BOT_NAME_LEN+1];
   char c_index[3];
   int i, j, length;
   int index;  // index into array of predefined names
   BOOL found = FALSE;

   if ((skin == NULL) || (*skin == 0))
   {
      // pick a random skin
      index = RANDOM_LONG(0, 9);  // there are ten possible skins

      // check if this skin has already been used...
      while (skin_used[index] == TRUE)
      {
         index++;

         if (index == MAX_SKINS)
            index = 0;
      }

      skin_used[index] = TRUE;

      // check if all skins are used...
      for (i = 0; i < MAX_SKINS; i++)
      {
         if (skin_used[i] == FALSE)
            break;
      }

      // if all skins are used, reset used to FALSE for next selection
      if (i == MAX_SKINS)
      {
         for (i = 0; i < MAX_SKINS; i++)
            skin_used[i] = FALSE;
      }

      strcpy( c_skin, bot_skins[index] );
   }
   else
   {
      strncpy( c_skin, skin, BOT_SKIN_LEN);
      c_skin[BOT_SKIN_LEN] = 0;  // make sure c_skin is null terminated
   }

   for (i = 0; c_skin[i] != 0; i++)
      c_skin[i] = tolower( c_skin[i] );  // convert to all lowercase

   index = 0;

   while ((!found) && (index < MAX_SKINS))
   {
      if (strcmp(c_skin, bot_skins[index]) == 0)
         found = TRUE;
      else
         index++;
   }

   if (found == TRUE)
   {
      if ((name != NULL) && (*name != 0))
      {
         strncpy( c_name, name, 31 );
         c_name[31] = 0;  // make sure c_name is null terminated
      }
      else
      {
         strcpy( c_name, bot_names[index] );
      }
   }
   else
   {
      char dir_name[32];
      char filename[128];

      struct stat stat_str;

      GET_GAME_DIR(dir_name);

      sprintf(filename, "%s\\models\\player\\%s", dir_name, c_skin);

      if (stat(filename, &stat_str) != 0)
      {
         sprintf(filename, "valve\\models\\player\\%s", c_skin);
         if (stat(filename, &stat_str) != 0)
         {
            char err_msg[80];

            sprintf( err_msg, "model \"%s\" is unknown.\n", c_skin );
            UTIL_ClientPrintAll( HUD_PRINTNOTIFY, err_msg );
            if (IS_DEDICATED_SERVER())
               printf(err_msg);

            UTIL_ClientPrintAll( HUD_PRINTNOTIFY,
               "use barney, gina, gman, gordon, helmet, hgrunt,\n");
            if (IS_DEDICATED_SERVER())
               printf("use barney, gina, gman, gordon, helmet, hgrunt,\n");
            UTIL_ClientPrintAll( HUD_PRINTNOTIFY,
               "    recon, robo, scientist, or zombie\n");
            if (IS_DEDICATED_SERVER())
               printf("    recon, robo, scientist, or zombie\n");
            return;
         }
      }

      // copy the name of the model to the bot's name...
      strncpy( c_name, skin, BOT_SKIN_LEN);
      c_name[BOT_SKIN_LEN] = 0;  // make sure c_skin is null terminated
   }

   length = strlen(c_name);

   // remove any illegal characters from name...
   for (i = 0; i < length; i++)
   {
      if ((c_name[i] <= ' ') || (c_name[i] > '~') ||
          (c_name[i] == '"'))
      {
         for (j = i; j < length; j++)  // shuffle chars left (and null)
            c_name[j] = c_name[j+1];
         length--;
      }               
   }

   skill_level = 0;

   if ((skill != NULL) && (*skill != 0))
      sscanf( skill, "%d", &skill_level );
   else
      skill_level = f_botskill;

   if ((skill_level < 1) || (skill_level > 5))
      skill_level = f_botskill;

   sprintf( c_skill, "%d", skill_level );

   BotEnt = CREATE_FAKE_CLIENT( c_name );

   if (FNullEnt( BotEnt ))
   {
      UTIL_ClientPrintAll( HUD_PRINTNOTIFY, "Max. Players reached.  Can't create bot!\n");

      if (IS_DEDICATED_SERVER())
         printf("Max. Players reached.  Can't create bot!\n");
   }
   else
   {
      char ptr[128];  // allocate space for message from ClientConnect
      char *infobuffer;
      int clientIndex;

      index = 0;
      while ((bot_respawn[index].is_used) && (index < 32))
         index++;

      if (index >= 32)
      {
         UTIL_ClientPrintAll( HUD_PRINTNOTIFY, "Can't create bot!\n");
         return;
      }

      sprintf(c_index, "%d", index);

      bot_respawn[index].is_used = TRUE;  // this slot is used

      // don't store the name here, it might change if same as another
      strcpy(bot_respawn[index].skin, c_skin);
      strcpy(bot_respawn[index].skill, c_skill);

      sprintf(ptr, "Creating bot \"%s\" using model %s with skill=%d\n", c_name, c_skin, skill_level);
      UTIL_ClientPrintAll( HUD_PRINTNOTIFY, ptr);

      if (IS_DEDICATED_SERVER())
         printf("%s", ptr);

      BotClass = GetClassPtr( (CBot *) VARS(BotEnt) );
      infobuffer = GET_INFOBUFFER( BotClass->edict( ) );
      clientIndex = BotClass->entindex( );

      SET_CLIENT_KEY_VALUE( clientIndex, infobuffer, "model", c_skin );
      SET_CLIENT_KEY_VALUE( clientIndex, infobuffer, "skill", c_skill );
      SET_CLIENT_KEY_VALUE( clientIndex, infobuffer, "index", c_index );

      ClientConnect( BotClass->edict( ), c_name, "127.0.0.1", ptr );
      DispatchSpawn( BotClass->edict( ) );
   }
}


void CBot::Spawn( )
{
   char c_skill[2];
   char c_index[3];

   CBasePlayer::Spawn();

   pev->flags = FL_CLIENT | FL_FAKECLIENT;

   // set the respawn index value based on key from BotCreate
   strcpy(c_index, GET_INFO_KEY_VALUE(GET_INFOBUFFER(edict( )), "index") );
   sscanf(c_index, "%d", &respawn_index);

   bot_respawn[respawn_index].pBot = (CBasePlayer *)this;

   // get the bot's name and save it in respawn array...
   strcpy(bot_respawn[respawn_index].name, STRING(pev->netname));

   bot_respawn[respawn_index].state = BOT_IDLE;

   pev->ideal_yaw = pev->v_angle.y;
   pev->yaw_speed = BOT_YAW_SPEED;

   // bot starts out in "paused" state since it hasn't moved yet...
   bot_was_paused = TRUE;
   v_prev_origin = pev->origin;

   f_shoot_time = 0;

   // get bot's skill level (0=good, 4=bad)
   strcpy(c_skill, GET_INFO_KEY_VALUE(GET_INFOBUFFER(edict( )), "skill") );
   sscanf(c_skill, "%d", &bot_skill);
   bot_skill--;  // make 0 based for array index (now 0-4)

   f_max_speed = CVAR_GET_FLOAT("sv_maxspeed");

   f_speed_check_time = gpGlobals->time + 2.0;

   ladder_dir = 0;

   // pick a wander direction (50% of the time to the left, 50% to the right)
   if (RANDOM_LONG(1, 100) <= 50)
      wander_dir = WANDER_LEFT;
   else
      wander_dir = WANDER_RIGHT;

   f_pause_time = 0;
   f_find_item = 0;

   if (g_pGameRules->IsTeamplay())  // is team play enabled?
   {
      strcpy(model_name, g_pGameRules->GetTeamID(this));
   }
   else
   {
      strcpy(model_name, GET_INFO_KEY_VALUE(GET_INFOBUFFER(edict( )), "model") );
   }

   bot_model = 0;
   if ((strcmp( model_name, "hgrunt" ) == 0) ||
       (strcmp( model_name, "recon" ) == 0))
   {
      bot_model = MODEL_HGRUNT;
   }
   else if (strcmp( model_name, "barney") == 0)
   {
      bot_model = MODEL_BARNEY;
   }
   else if (strcmp( model_name, "scientist") == 0)
   {
      bot_model = MODEL_SCIENTIST;
   }

   f_pain_time = gpGlobals->time + 5.0;

   b_use_health_station = FALSE;
   b_use_HEV_station = FALSE;
   b_use_button = FALSE;
   f_use_button_time = 0;
   b_lift_moving = FALSE;
   f_use_ladder_time = 0;
   f_fire_gauss = -1;  // -1 means not charging gauss gun

   b_see_tripmine = FALSE;
   b_shoot_tripmine = FALSE;

   f_weapon_inventory_time = 0;
   f_dont_avoid_wall_time = 0;
   f_bot_use_time = 0;
   f_wall_on_right = 0;
   f_wall_on_left = 0;

   pBotEnemy = NULL;
   pBotUser = NULL;
   pBotPickupItem = NULL;
}


int CBot::TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker,
                      float flDamage, int bitsDamageType )
{
   CBaseEntity *pAttacker = CBaseEntity::Instance(pevAttacker);
   char sound[40];
   int ret_damage;

   // do the damage first...
   ret_damage = CBasePlayer::TakeDamage( pevInflictor, pevAttacker, flDamage,
                                         bitsDamageType );

   // if the bot doesn't have an enemy and someone is shooting at it then
   // turn in the attacker's direction...
   if (pBotEnemy == NULL)
   {
      // face the attacker...
      Vector v_enemy = pAttacker->pev->origin - pev->origin;
      Vector bot_angles = UTIL_VecToAngles( v_enemy );

      pev->ideal_yaw = bot_angles.y;

      // check for wrap around of angle...
      if (pev->ideal_yaw > 180)
         pev->ideal_yaw -= 360;
      if (pev->ideal_yaw < -180)
         pev->ideal_yaw += 360;

      // stop using health or HEV stations...
      b_use_health_station = FALSE;
      b_use_HEV_station = FALSE;
   }

   // check if bot model is known, attacker is not a bot,
   // time for pain sound, and bot has some of health left...

   if ((bot_model != 0) && (pAttacker->IsNetClient()) &&
       (f_pain_time <= gpGlobals->time) && (pev->health > 0) &&
       ( !IS_DEDICATED_SERVER() ))
   {
      float distance = (pAttacker->pev->origin - pev->origin).Length( );

      // check if the distance to attacker is close enough (otherwise
      // the attacker is too far away to hear the pain sounds)

      if (distance <= 400)
      {
         // speak pain sounds about 50% of the time
         if (RANDOM_LONG(1, 100) <= 50)
         {
            f_pain_time = gpGlobals->time + 5.0;

            if (bot_model == MODEL_HGRUNT)
               strcpy( sound, hgrunt_sounds[RANDOM_LONG(0,4)] );
            else if (bot_model == MODEL_BARNEY)
               strcpy( sound, barney_sounds[RANDOM_LONG(0,4)] );
            else if (bot_model == MODEL_SCIENTIST)
               strcpy( sound, scientist_sounds[RANDOM_LONG(0,4)] );

            EMIT_SOUND(ENT(pevAttacker), CHAN_VOICE, sound,
                       RANDOM_FLOAT(0.9, 1.0), ATTN_NORM);
         }
      }
   }

   return ret_damage;
}


void CBot::Use( CBaseEntity *pActivator, CBaseEntity *pCaller,
                USE_TYPE useType, float value )
{
   if (g_pGameRules->IsTeamplay())  // is team play enabled?
   {
      // check the bot and player are on the same team...

      if (UTIL_TeamsMatch(g_pGameRules->GetTeamID(this),
                          g_pGameRules->GetTeamID(pActivator)))
      {
         if (pBotEnemy == NULL)  // is bot NOT currently engaged in combat?
         {
            if (pBotUser == NULL)  // does bot NOT have a "user"
            {
               // tell teammate that bot will cover them...

               EMIT_SOUND(ENT(pActivator->pev), CHAN_VOICE, USE_TEAMPLAY_SND,
                          1.0, ATTN_NORM);

               pBotUser = pActivator;
               f_bot_use_time = gpGlobals->time;
            }
            else
            {
               // tell teammate that you'll see them later..

               EMIT_SOUND(ENT(pActivator->pev), CHAN_VOICE, USE_TEAMPLAY_LATER_SND,
                          1.0, ATTN_NORM);

               pBotUser = NULL;
               f_bot_use_time = 0;
            }
         }
         else
         {
            EMIT_SOUND(ENT(pActivator->pev), CHAN_VOICE, USE_TEAMPLAY_ENEMY_SND, 1.0,
                       ATTN_NORM);
         }
      }
   }
}


int CBot::BotInFieldOfView(Vector dest)
{
   // find angles from source to destination...
   Vector entity_angles = UTIL_VecToAngles( dest );

   // make yaw angle 0 to 360 degrees if negative...
   if (entity_angles.y < 0)
      entity_angles.y += 360;

   // get bot's current view angle...
   float view_angle = pev->v_angle.y;

   // make view angle 0 to 360 degrees if negative...
   if (view_angle < 0)
      view_angle += 360;

   // return the absolute value of angle to destination entity
   // zero degrees means straight ahead,  45 degrees to the left or
   // 45 degrees to the right is the limit of the normal view angle

   return abs((int)view_angle - (int)entity_angles.y);
}


BOOL CBot::BotEntityIsVisible( Vector dest )
{
   TraceResult tr;

   // trace a line from bot's eyes to destination...
   UTIL_TraceLine( pev->origin + pev->view_ofs, dest, ignore_monsters,
                   ENT(pev), &tr );

   // check if line of sight to object is not blocked (i.e. visible)
   if (tr.flFraction >= 1.0)
      return TRUE;
   else
      return FALSE;
}


float CBot::BotChangeYaw( float speed )
{
   float ideal;
   float current;
   float current_180;  // current +/- 180 degrees
   float diff;

   // turn from the current v_angle yaw to the ideal_yaw by selecting
   // the quickest way to turn to face that direction
   
   current = pev->v_angle.y;
   ideal = pev->ideal_yaw;

   // find the difference in the current and ideal angle
   diff = abs(current - ideal);

   // check if the bot is already facing the ideal_yaw direction...
   if (diff <= 1)
      return diff;  // return number of degrees turned

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

   pev->v_angle.y = current;

   pev->angles.x = 0;
   pev->angles.y = pev->v_angle.y;
   pev->angles.z = 0;

   return speed;  // return number of degrees turned
}


void CBot::BotOnLadder( float moved_distance )
{
   // moves the bot up or down a ladder.  if the bot can't move
   // (i.e. get's stuck with someone else on ladder), the bot will
   // change directions and go the other way on the ladder.

   if (ladder_dir == LADDER_UP)  // is the bot currently going up?
   {
      pev->v_angle.x = -60;  // look upwards

      // check if the bot hasn't moved much since the last location...
      if (moved_distance <= 1)
      {
         // the bot must be stuck, change directions...

         pev->v_angle.x = 60;  // look downwards
         ladder_dir = LADDER_DOWN;
      }
   }
   else if (ladder_dir == LADDER_DOWN)  // is the bot currently going down?
   {
      pev->v_angle.x = 60;  // look downwards

      // check if the bot hasn't moved much since the last location...
      if (moved_distance <= 1)
      {
         // the bot must be stuck, change directions...

         pev->v_angle.x = -60;  // look upwards
         ladder_dir = LADDER_UP;
      }
   }
   else  // the bot hasn't picked a direction yet, try going up...
   {
      pev->v_angle.x = -60;  // look upwards
      ladder_dir = LADDER_UP;
   }

   // move forward (i.e. in the direction the bot is looking, up or down)
   pev->button |= IN_FORWARD;
}


void CBot::BotUnderWater( void )
{
   // handle movements under water.  right now, just try to keep from
   // drowning by swimming up towards the surface and look to see if
   // there is a surface the bot can jump up onto to get out of the
   // water.  bots DON'T like water!

   Vector v_src, v_forward;
   TraceResult tr;
   int contents;

   // swim up towards the surface
   pev->v_angle.x = -60;  // look upwards

   // move forward (i.e. in the direction the bot is looking, up or down)
   pev->button |= IN_FORWARD;

   // set gpGlobals angles based on current view angle (for TraceLine)
   UTIL_MakeVectors( pev->v_angle );

   // look from eye position straight forward (remember: the bot is looking
   // upwards at a 60 degree angle so TraceLine will go out and up...

   v_src = pev->origin + pev->view_ofs;  // EyePosition()
   v_forward = v_src + gpGlobals->v_forward * 90;

   // trace from the bot's eyes straight forward...
   UTIL_TraceLine( v_src, v_forward, dont_ignore_monsters, ENT(pev), &tr);

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
                         ENT(pev), &tr);

         // check if the trace hit something...
         if (tr.flFraction < 1.0)
         {
            contents = UTIL_PointContents( tr.vecEndPos );

            // if contents isn't water then assume it's land, jump!
            if (contents != CONTENTS_WATER)
            {
               pev->button |= IN_JUMP;
            }
         }
      }
   }
}


void CBot::BotFindItem( void )
{
   CBaseEntity *pEntity = NULL;
   CBaseEntity *pPickupEntity = NULL;
   Vector pickup_origin;
   Vector entity_origin;
   float radius = 500;
   BOOL can_pickup;
   float min_distance;
   char item_name[40];
   TraceResult tr;
   Vector vecStart;
   Vector vecEnd;
   int angle_to_entity;

   pBotPickupItem = NULL;

   min_distance = radius + 1.0;

   while ((pEntity = UTIL_FindEntityInSphere( pEntity, pev->origin, radius )) != NULL)
   {
      can_pickup = FALSE;  // assume can't use it until known otherwise

      strcpy(item_name, STRING(pEntity->pev->classname));

      // see if this is a "func_" type of entity (func_button, etc.)...
      if (strncmp("func_", item_name, 5) == 0)
      {
         // BModels have 0,0,0 for origin so must use VecBModelOrigin...
         entity_origin = VecBModelOrigin(pEntity->pev);

         vecStart = pev->origin + pev->view_ofs;
         vecEnd = entity_origin;

         angle_to_entity = BotInFieldOfView( vecEnd - vecStart );

         // check if entity is outside field of view (+/- 45 degrees)
         if (angle_to_entity > 45)
            continue;  // skip this item if bot can't "see" it

         // check if entity is a ladder (ladders are a special case)...
         if (strcmp("func_ladder", item_name) == 0)
         {
            // force ladder origin to same z coordinate as bot since
            // the VecBModelOrigin is the center of the ladder.  For
            // LONG ladders, the center MAY be hundreds of units above
            // the bot.  Fake an origin at the same level as the bot...

            entity_origin.z = pev->origin.z;
            vecEnd = entity_origin;

            // trace a line from bot's eyes to func_ladder entity...
            UTIL_TraceLine( vecStart, vecEnd, dont_ignore_monsters, ENT(pev), &tr);

            // check if traced all the way up to the entity (didn't hit wall)
            if (tr.flFraction >= 1.0)
            {
               // find distance to item for later use...
               float distance = (vecEnd - vecStart).Length( );

               // use the ladder about 100% of the time, if haven't
               // used a ladder in at least 5 seconds...
               if ((RANDOM_LONG(1, 100) <= 100) &&
                   ((f_use_ladder_time + 5) < gpGlobals->time))
               {
                  // if close to ladder...
                  if (distance < 100)
                  {
                     // don't avoid walls for a while
                     f_dont_avoid_wall_time = gpGlobals->time + 5.0;
                  }

                  can_pickup = TRUE;
               }
            }
         }
         else
         {
            // trace a line from bot's eyes to func_ entity...
            UTIL_TraceLine( vecStart, vecEnd, dont_ignore_monsters, ENT(pev), &tr);

            // check if traced all the way up to the entity (didn't hit wall)
            if (strcmp(item_name, STRING(tr.pHit->v.classname)) == 0)
            {
               // find distance to item for later use...
               float distance = (vecEnd - vecStart).Length( );

               // check if entity is wall mounted health charger...
               if (strcmp("func_healthcharger", item_name) == 0)
               {
                  // check if the bot can use this item and
                  // check if the recharger is ready to use (has power left)...
                  if ((pev->health < 100) && (pEntity->pev->frame == 0))
                  {
                     // check if flag not set...
                     if (!b_use_health_station)
                     {
                        // check if close enough and facing it directly...
                        if ((distance < PLAYER_SEARCH_RADIUS) &&
                            (angle_to_entity <= 10))
                        {
                           b_use_health_station = TRUE;
                           f_use_health_time = gpGlobals->time;
                        }

                        // if close to health station...
                        if (distance < 100)
                        {
                           // don't avoid walls for a while
                           f_dont_avoid_wall_time = gpGlobals->time + 5.0;
                        }

                        can_pickup = TRUE;
                     }
                  }
                  else
                  {
                     // don't need or can't use this item...
                     b_use_health_station = FALSE;
                  }
               }

               // check if entity is wall mounted HEV charger...
               else if (strcmp("func_recharge", item_name) == 0)
               {
                  // check if the bot can use this item and
                  // check if the recharger is ready to use (has power left)...
                  if ((pev->armorvalue < MAX_NORMAL_BATTERY) &&
                      (pev->weapons & (1<<WEAPON_SUIT)) &&
                      (pEntity->pev->frame == 0))
                  {
                     // check if flag not set and facing it...
                     if (!b_use_HEV_station)
                     {
                        // check if close enough and facing it directly...
                        if ((distance < PLAYER_SEARCH_RADIUS) &&
                            (angle_to_entity <= 10))
                        {
                           b_use_HEV_station = TRUE;
                           f_use_HEV_time = gpGlobals->time;
                        }

                        // if close to HEV recharger...
                        if (distance < 100)
                        {
                           // don't avoid walls for a while
                           f_dont_avoid_wall_time = gpGlobals->time + 5.0;
                        }

                        can_pickup = TRUE;
                     }
                  }
                  else
                  {
                     // don't need or can't use this item...
                     b_use_HEV_station = FALSE;
                  }
               }

               // check if entity is a button...
               else if (strcmp("func_button", item_name) == 0)
               {
                  // use the button about 100% of the time, if haven't
                  // used a button in at least 5 seconds...
                  if ((RANDOM_LONG(1, 100) <= 100) &&
                      ((f_use_button_time + 5) < gpGlobals->time))
                  {
                     // check if flag not set and facing it...
                     if (!b_use_button)
                     {
                        // check if close enough and facing it directly...
                        if ((distance < PLAYER_SEARCH_RADIUS) &&
                            (angle_to_entity <= 10))
                        {
                           b_use_button = TRUE;
                           b_lift_moving = FALSE;
                           f_use_button_time = gpGlobals->time;
                        }

                        // if close to button...
                        if (distance < 100)
                        {
                           // don't avoid walls for a while
                           f_dont_avoid_wall_time = gpGlobals->time + 5.0;
                        }

                        can_pickup = TRUE;
                     }
                  }
                  else
                  {
                     // don't need or can't use this item...
                     b_use_button = FALSE;
                  }
               }
            }
         }
      }

      // check if entity is an armed tripmine beam
      else if (strcmp("beam", item_name) == 0)
      {
         CBeam *pBeam = (CBeam *)pEntity;

//         Vector v_beam_start = pBeam->GetStartPos();
//         Vector v_beam_end = pBeam->GetEndPos();
//
//         if (FInViewCone( &v_beam_start ) && FVisible( v_beam_start ))
//         {
//            BotDebug("I see a beam start!\n");
//         }
//
//         if (FInViewCone( &v_beam_end ) && FVisible( v_beam_end ))
//         {
//            BotDebug("I see a beam end!\n");
//         }
      }

      else  // everything else...
      {
         entity_origin = pEntity->pev->origin;

         vecStart = pev->origin + pev->view_ofs;
         vecEnd = entity_origin;

         // find angles from bot origin to entity...
         angle_to_entity = BotInFieldOfView( vecEnd - vecStart );

         // check if entity is outside field of view (+/- 45 degrees)
         if (angle_to_entity > 45)
            continue;  // skip this item if bot can't "see" it

         // check if line of sight to object is not blocked (i.e. visible)
         if (BotEntityIsVisible( vecEnd ))
         {

            // check if entity is a weapon...
            if (strncmp("weapon_", item_name, 7) == 0)
            {
               CBasePlayerWeapon *pWeapon = (CBasePlayerWeapon *)pEntity;

               if ((pWeapon->m_pPlayer) || (pWeapon->pev->effects & EF_NODRAW))
               {
                  // someone owns this weapon or it hasn't respawned yet
                  continue;
               }

               if (g_pGameRules->CanHavePlayerItem( this, pWeapon ))
               {
                  can_pickup = TRUE;
               }
            }

            // check if entity is ammo...
            else if (strncmp("ammo_", item_name, 5) == 0)
            {
               CBasePlayerAmmo *pAmmo = (CBasePlayerAmmo *)pEntity;
               BOOL ammo_found = FALSE;
               int i;

               // check if the item is not visible (i.e. has not respawned)
               if (pAmmo->pev->effects & EF_NODRAW)
                  continue;

               i = 0;
               while (ammo_check[i].ammo_name[0])
               {
                  if (strcmp(ammo_check[i].ammo_name, item_name) == 0)
                  {
                     ammo_found = TRUE;

                     // see if the bot can pick up this item...
                     if (g_pGameRules->CanHaveAmmo( this,
                         ammo_check[i].weapon_name, ammo_check[i].max_carry))
                     {
                        can_pickup = TRUE;
                        break;
                     }
                  }

                  i++;
               }
               if (ammo_found == FALSE)
               {
                  sprintf(message, "unknown ammo: %s\n", item_name);
                  BotDebug(message);
               }
            }

            // check if entity is a battery...
            else if (strcmp("item_battery", item_name) == 0)
            {
               CItem *pBattery = (CItem *)pEntity;

               // check if the item is not visible (i.e. has not respawned)
               if (pBattery->pev->effects & EF_NODRAW)
                  continue;

               // check if the bot can use this item...
               if ((pev->armorvalue < MAX_NORMAL_BATTERY) &&
                   (pev->weapons & (1<<WEAPON_SUIT)))
               {
                  can_pickup = TRUE;
               }
            }

            // check if entity is a healthkit...
            else if (strcmp("item_healthkit", item_name) == 0)
            {
               CItem *pHealthKit = (CItem *)pEntity;

               // check if the item is not visible (i.e. has not respawned)
               if (pHealthKit->pev->effects & EF_NODRAW)
                  continue;

               // check if the bot can use this item...
               if (pev->health < 100)
               {
                  can_pickup = TRUE;
               }
            }

            // check if entity is a packed up weapons box...
            else if (strcmp("weaponbox", item_name) == 0)
            {
               can_pickup = TRUE;
            }

            // check if entity is the spot from RPG laser
            else if (strcmp("laser_spot", item_name) == 0)
            {
            }

            // check if entity is an armed tripmine
            else if (strcmp("monster_tripmine", item_name) == 0)
            {
               float distance = (pEntity->pev->origin - pev->origin).Length( );

               if (b_see_tripmine)
               {
                  // see if this tripmine is closer to bot...
                  if (distance < (v_tripmine_origin - pev->origin).Length())
                  {
                     v_tripmine_origin = pEntity->pev->origin;
                     b_shoot_tripmine = FALSE;

                     // see if bot is far enough to shoot the tripmine...
                     if (distance >= 375)
                     {
                        b_shoot_tripmine = TRUE;
                     }
                  }
               }
               else
               {
                  b_see_tripmine = TRUE;
                  v_tripmine_origin = pEntity->pev->origin;
                  b_shoot_tripmine = FALSE;

                  // see if bot is far enough to shoot the tripmine...
                  if (distance >= 375)  // 375 is damage radius
                  {
                     b_shoot_tripmine = TRUE;
                  }
               }
            }

            // check if entity is an armed satchel charge
            else if (strcmp("monster_satchel", item_name) == 0)
            {
            }

            // check if entity is a snark (squeak grenade)
            else if (strcmp("monster_snark", item_name) == 0)
            {
            }

         }  // end if object is visible
      }  // end else not "func_" entity

      if (can_pickup) // if the bot found something it can pickup...
      {
         float distance = (entity_origin - pev->origin).Length( );

         // see if it's the closest item so far...
         if (distance < min_distance)
         {
            min_distance = distance;        // update the minimum distance
            pPickupEntity = pEntity;        // remember this entity
            pickup_origin = entity_origin;  // remember location of entity
         }
      }
   }  // end while loop

   if (pPickupEntity != NULL)
   {
      // let's head off toward that item...
      Vector v_item = pickup_origin - pev->origin;

      Vector bot_angles = UTIL_VecToAngles( v_item );

      pev->ideal_yaw = bot_angles.y;

      // check for wrap around of angle...
      if (pev->ideal_yaw > 180)
         pev->ideal_yaw -= 360;
      if (pev->ideal_yaw < -180)
         pev->ideal_yaw += 360;

      pBotPickupItem = pPickupEntity;  // save the item bot is trying to get
   }
}


void CBot::BotUseLift( float moved_distance )
{
   // just need to press the button once, when the flag gets set...
   if (f_use_button_time == gpGlobals->time)
   {
      pev->button = IN_USE;

      // face opposite from the button
      pev->ideal_yaw = -pev->ideal_yaw;  // rotate 180 degrees

      // check for wrap around of angle...
      if (pev->ideal_yaw > 180)
         pev->ideal_yaw -= 360;
      if (pev->ideal_yaw < -180)
         pev->ideal_yaw += 360;
   }

   // check if the bot has waited too long for the lift to move...
   if (((f_use_button_time + 2.0) < gpGlobals->time) &&
       (!b_lift_moving))
   {
      // clear use button flag
      b_use_button = FALSE;

      // bot doesn't have to set f_find_item since the bot
      // should already be facing away from the button

      f_move_speed = f_max_speed;
   }

   // check if lift has started moving...
   if ((moved_distance > 1) && (!b_lift_moving))
   {
      b_lift_moving = TRUE;
   }

   // check if lift has stopped moving...
   if ((moved_distance <= 1) && (b_lift_moving))
   {
      TraceResult tr1, tr2;
      Vector v_src, v_forward, v_right, v_left;
      Vector v_down, v_forward_down, v_right_down, v_left_down;

      b_use_button = FALSE;

      // TraceLines in 4 directions to find which way to go...

      UTIL_MakeVectors( pev->v_angle );

      v_src = pev->origin + pev->view_ofs;
      v_forward = v_src + gpGlobals->v_forward * 90;
      v_right = v_src + gpGlobals->v_right * 90;
      v_left = v_src + gpGlobals->v_right * -90;

      v_down = pev->v_angle;
      v_down.x = v_down.x + 45;  // look down at 45 degree angle

      UTIL_MakeVectors( v_down );

      v_forward_down = v_src + gpGlobals->v_forward * 100;
      v_right_down = v_src + gpGlobals->v_right * 100;
      v_left_down = v_src + gpGlobals->v_right * -100;

      // try tracing forward first...
      UTIL_TraceLine( v_src, v_forward, dont_ignore_monsters, ENT(pev), &tr1);
      UTIL_TraceLine( v_src, v_forward_down, dont_ignore_monsters, ENT(pev), &tr2);

      // check if we hit a wall or didn't find a floor...
      if ((tr1.flFraction < 1.0) || (tr2.flFraction >= 1.0))
      {
         // try tracing to the RIGHT side next...
         UTIL_TraceLine( v_src, v_right, dont_ignore_monsters, ENT(pev), &tr1);
         UTIL_TraceLine( v_src, v_right_down, dont_ignore_monsters, ENT(pev), &tr2);

         // check if we hit a wall or didn't find a floor...
         if ((tr1.flFraction < 1.0) || (tr2.flFraction >= 1.0))
         {
            // try tracing to the LEFT side next...
            UTIL_TraceLine( v_src, v_left, dont_ignore_monsters, ENT(pev), &tr1);
            UTIL_TraceLine( v_src, v_left_down, dont_ignore_monsters, ENT(pev), &tr2);

            // check if we hit a wall or didn't find a floor...
            if ((tr1.flFraction < 1.0) || (tr2.flFraction >= 1.0))
            {
               // only thing to do is turn around...
               pev->ideal_yaw += 180;  // turn all the way around
            }
            else
            {
               pev->ideal_yaw += 90;  // turn to the LEFT
            }
         }
         else
         {
            pev->ideal_yaw -= 90;  // turn to the RIGHT
         }

         // check for wrap around of angle...
         if (pev->ideal_yaw > 180)
            pev->ideal_yaw -= 360;
         if (pev->ideal_yaw < -180)
            pev->ideal_yaw += 360;
      }

      BotChangeYaw( pev->yaw_speed );

      f_move_speed = f_max_speed;
   }
}


void CBot::BotTurnAtWall( TraceResult *tr )
{
   Vector Normal;
   float Y, Y1, Y2, D1, D2, Z;

   // Find the normal vector from the trace result.  The normal vector will
   // be a vector that is perpendicular to the surface from the TraceResult.

   Normal = UTIL_VecToAngles(tr->vecPlaneNormal);

   // Since the bot keeps it's view angle in -180 < x < 180 degrees format,
   // and since TraceResults are 0 < x < 360, we convert the bot's view
   // angle (yaw) to the same format at TraceResult.

   Y = pev->v_angle.y;
   Y = Y + 180;
   if (Y > 359) Y -= 360;

   // Turn the normal vector around 180 degrees (i.e. make it point towards
   // the wall not away from it.  That makes finding the angles that the
   // bot needs to turn a little easier.

   Normal.y = Normal.y - 180;
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

   D1 = abs(Y - Y1);
   if (D1 > 179) D1 = abs(D1 - 360);
   D2 = abs(Y - Y2);
   if (D2 > 179) D2 = abs(D2 - 360);

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
   pev->ideal_yaw = Z;

   // check for wrap around of angle...
   if (pev->ideal_yaw > 180)
      pev->ideal_yaw -= 360;
   if (pev->ideal_yaw < -180)
      pev->ideal_yaw += 360;

   f_move_speed = 0;  // don't move while turning
}


BOOL CBot::BotCantMoveForward( TraceResult *tr )
{
   // use some TraceLines to determine if anything is blocking the current
   // path of the bot.

   Vector v_src, v_forward;

   UTIL_MakeVectors( pev->v_angle );

   // first do a trace from the bot's eyes forward...

   v_src = pev->origin + pev->view_ofs;  // EyePosition()
   v_forward = v_src + gpGlobals->v_forward * 40;

   // trace from the bot's eyes straight forward...
   UTIL_TraceLine( v_src, v_forward, dont_ignore_monsters, ENT(pev), tr);

   // check if the trace hit something...
   if (tr->flFraction < 1.0)
   {
      return TRUE;  // bot's head will hit something
   }

   // bot's head is clear, check at waist level...

   v_src = pev->origin;
   v_forward = v_src + gpGlobals->v_forward * 40;

   // trace from the bot's waist straight forward...
   UTIL_TraceLine( v_src, v_forward, dont_ignore_monsters, ENT(pev), tr);

   // check if the trace hit something...
   if (tr->flFraction < 1.0)
   {
      return TRUE;  // bot's body will hit something
   }

   return FALSE;  // bot can move forward, return false
}


BOOL CBot::BotCanJumpUp( void )
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
   Vector v_jump, v_source, v_dest;

   // convert current view angle to vectors for TraceLine math...

   v_jump = pev->v_angle;
   v_jump.x = 0;  // reset pitch to 0 (level horizontally)
   v_jump.z = 0;  // reset roll to 0 (straight up and down)

   UTIL_MakeVectors( v_jump );

   // use center of the body first...

   // maximum jump height is 45, so check one unit above that (46)
   v_source = pev->origin + Vector(0, 0, -36 + 46);
   v_dest = v_source + gpGlobals->v_forward * 24;

   // trace a line forward at maximum jump height...
   UTIL_TraceLine( v_source, v_dest, dont_ignore_monsters, ENT(pev), &tr);

   // if trace hit something, return FALSE
   if (tr.flFraction < 1.0)
      return FALSE;

   // now check same height to one side of the bot...
   v_source = pev->origin + gpGlobals->v_right * 16 + Vector(0, 0, -36 + 46);
   v_dest = v_source + gpGlobals->v_forward * 24;

   // trace a line forward at maximum jump height...
   UTIL_TraceLine( v_source, v_dest, dont_ignore_monsters, ENT(pev), &tr);

   // if trace hit something, return FALSE
   if (tr.flFraction < 1.0)
      return FALSE;

   // now check same height on the other side of the bot...
   v_source = pev->origin + gpGlobals->v_right * -16 + Vector(0, 0, -36 + 46);
   v_dest = v_source + gpGlobals->v_forward * 24;

   // trace a line forward at maximum jump height...
   UTIL_TraceLine( v_source, v_dest, dont_ignore_monsters, ENT(pev), &tr);

   // if trace hit something, return FALSE
   if (tr.flFraction < 1.0)
      return FALSE;

   // now trace from head level downward to check for obstructions...

   // start of trace is 24 units in front of bot, 72 units above head...
   v_source = pev->origin + gpGlobals->v_forward * 24;

   // offset 72 units from top of head (72 + 36)
   v_source.z = v_source.z + 108;

   // end point of trace is 99 units straight down from start...
   // (99 is 108 minus the jump limit height which is 45 - 36 = 9)
   v_dest = v_source + Vector(0, 0, -99);

   // trace a line straight down toward the ground...
   UTIL_TraceLine( v_source, v_dest, dont_ignore_monsters, ENT(pev), &tr);

   // if trace hit something, return FALSE
   if (tr.flFraction < 1.0)
      return FALSE;

   // now check same height to one side of the bot...
   v_source = pev->origin + gpGlobals->v_right * 16 + gpGlobals->v_forward * 24;
   v_source.z = v_source.z + 108;
   v_dest = v_source + Vector(0, 0, -99);

   // trace a line straight down toward the ground...
   UTIL_TraceLine( v_source, v_dest, dont_ignore_monsters, ENT(pev), &tr);

   // if trace hit something, return FALSE
   if (tr.flFraction < 1.0)
      return FALSE;

   // now check same height on the other side of the bot...
   v_source = pev->origin + gpGlobals->v_right * -16 + gpGlobals->v_forward * 24;
   v_source.z = v_source.z + 108;
   v_dest = v_source + Vector(0, 0, -99);

   // trace a line straight down toward the ground...
   UTIL_TraceLine( v_source, v_dest, dont_ignore_monsters, ENT(pev), &tr);

   // if trace hit something, return FALSE
   if (tr.flFraction < 1.0)
      return FALSE;

   return TRUE;
}


BOOL CBot::BotCanDuckUnder( void )
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

   // convert current view angle to vectors for TraceLine math...

   v_duck = pev->v_angle;
   v_duck.x = 0;  // reset pitch to 0 (level horizontally)
   v_duck.z = 0;  // reset roll to 0 (straight up and down)

   UTIL_MakeVectors( v_duck );

   // use center of the body first...

   // duck height is 36, so check one unit above that (37)
   v_source = pev->origin + Vector(0, 0, -36 + 37);
   v_dest = v_source + gpGlobals->v_forward * 24;

   // trace a line forward at duck height...
   UTIL_TraceLine( v_source, v_dest, dont_ignore_monsters, ENT(pev), &tr);

   // if trace hit something, return FALSE
   if (tr.flFraction < 1.0)
      return FALSE;

   // now check same height to one side of the bot...
   v_source = pev->origin + gpGlobals->v_right * 16 + Vector(0, 0, -36 + 37);
   v_dest = v_source + gpGlobals->v_forward * 24;

   // trace a line forward at duck height...
   UTIL_TraceLine( v_source, v_dest, dont_ignore_monsters, ENT(pev), &tr);

   // if trace hit something, return FALSE
   if (tr.flFraction < 1.0)
      return FALSE;

   // now check same height on the other side of the bot...
   v_source = pev->origin + gpGlobals->v_right * -16 + Vector(0, 0, -36 + 37);
   v_dest = v_source + gpGlobals->v_forward * 24;

   // trace a line forward at duck height...
   UTIL_TraceLine( v_source, v_dest, dont_ignore_monsters, ENT(pev), &tr);

   // if trace hit something, return FALSE
   if (tr.flFraction < 1.0)
      return FALSE;

   // now trace from the ground up to check for object to duck under...

   // start of trace is 24 units in front of bot near ground...
   v_source = pev->origin + gpGlobals->v_forward * 24;
   v_source.z = v_source.z - 35;  // offset to feet + 1 unit up

   // end point of trace is 72 units straight up from start...
   v_dest = v_source + Vector(0, 0, 72);

   // trace a line straight up in the air...
   UTIL_TraceLine( v_source, v_dest, dont_ignore_monsters, ENT(pev), &tr);

   // if trace didn't hit something, return FALSE
   if (tr.flFraction >= 1.0)
      return FALSE;

   // now check same height to one side of the bot...
   v_source = pev->origin + gpGlobals->v_right * 16 + gpGlobals->v_forward * 24;
   v_source.z = v_source.z - 35;  // offset to feet + 1 unit up
   v_dest = v_source + Vector(0, 0, 72);

   // trace a line straight up in the air...
   UTIL_TraceLine( v_source, v_dest, dont_ignore_monsters, ENT(pev), &tr);

   // if trace didn't hit something, return FALSE
   if (tr.flFraction >= 1.0)
      return FALSE;

   // now check same height on the other side of the bot...
   v_source = pev->origin + gpGlobals->v_right * -16 + gpGlobals->v_forward * 24;
   v_source.z = v_source.z - 35;  // offset to feet + 1 unit up
   v_dest = v_source + Vector(0, 0, 72);

   // trace a line straight up in the air...
   UTIL_TraceLine( v_source, v_dest, dont_ignore_monsters, ENT(pev), &tr);

   // if trace didn't hit something, return FALSE
   if (tr.flFraction >= 1.0)
      return FALSE;

   return TRUE;
}


BOOL CBot::BotShootTripmine( void )
{
   if (b_shoot_tripmine != TRUE)
      return FALSE;

   // aim at the tripmine and fire the glock...

   Vector v_enemy = v_tripmine_origin - GetGunPosition( );

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

   return (BotFireWeapon( v_tripmine_origin, WEAPON_GLOCK, TRUE ));
}


BOOL CBot::BotFollowUser( void )
{
   BOOL user_visible;
   float f_distance;

   Vector vecEnd = pBotUser->pev->origin + pBotUser->pev->view_ofs;

   pev->v_angle.x = 0;  // reset pitch to 0 (level horizontally)
   pev->v_angle.z = 0;  // reset roll to 0 (straight up and down)

   pev->angles.x = 0;
   pev->angles.y = pev->v_angle.y;
   pev->angles.z = 0;

   if (!pBotUser->IsAlive( ))
   {
      // the bot's user is dead!
      pBotUser = NULL;
      return FALSE;
   }

   user_visible = FInViewCone( &vecEnd ) && FVisible( vecEnd );

   // check if the "user" is still visible or if the user has been visible
   // in the last 5 seconds (or the player just starting "using" the bot)

   if (user_visible || (f_bot_use_time + 5 > gpGlobals->time))
   {
      if (user_visible)
         f_bot_use_time = gpGlobals->time;  // reset "last visible time"

      // face the user
      Vector v_user = pBotUser->pev->origin - pev->origin;
      Vector bot_angles = UTIL_VecToAngles( v_user );

      pev->ideal_yaw = bot_angles.y;

      // check for wrap around of angle...
      if (pev->ideal_yaw > 180)
         pev->ideal_yaw -= 360;
      if (pev->ideal_yaw < -180)
         pev->ideal_yaw += 360;

      f_distance = v_user.Length( );  // how far away is the "user"?

      if (f_distance > 200)      // run if distance to enemy is far
         f_move_speed = f_max_speed;
      else if (f_distance > 50)  // walk if distance is closer
         f_move_speed = f_max_speed / 2;
      else                     // don't move if close enough
         f_move_speed = 0.0;

      return TRUE;
   }
   else
   {
      // person to follow has gone out of sight...
      pBotUser = NULL;

      return FALSE;
   }
}


BOOL CBot::BotCheckWallOnLeft( void )
{
   Vector v_src, v_left;
   TraceResult tr;

   UTIL_MakeVectors( pev->v_angle );

   // do a trace to the left...

   v_src = pev->origin;
   v_left = v_src + gpGlobals->v_right * -40;  // 40 units to the left

   UTIL_TraceLine( v_src, v_left, dont_ignore_monsters, ENT(pev), &tr);

   // check if the trace hit something...
   if (tr.flFraction < 1.0)
   {
      if (f_wall_on_left == 0)
         f_wall_on_left = gpGlobals->time;

      return TRUE;
   }

   return FALSE;
}


BOOL CBot::BotCheckWallOnRight( void )
{
   Vector v_src, v_right;
   TraceResult tr;

   UTIL_MakeVectors( pev->v_angle );

   // do a trace to the right...

   v_src = pev->origin;
   v_right = v_src + gpGlobals->v_right * 40;  // 40 units to the right

   UTIL_TraceLine( v_src, v_right, dont_ignore_monsters, ENT(pev), &tr);

   // check if the trace hit something...
   if (tr.flFraction < 1.0)
   {
      if (f_wall_on_right == 0)
         f_wall_on_right = gpGlobals->time;

      return TRUE;
   }

   return FALSE;
}


void CBot::BotThink( void )
{
   Vector v_diff;             // vector from previous to current location
   float moved_distance;      // length of v_diff vector (distance bot moved)
   float degrees_turned;

   // check if someone kicked the bot off of the server (DON'T RESPAWN!)...
   if ((pev->takedamage == DAMAGE_NO) && (respawn_index >= 0))
   {
      pev->health = 0;
      pev->deadflag = DEAD_DEAD;  // make the kicked bot be dead

      bot_respawn[respawn_index].is_used = FALSE;  // this slot is now free
      bot_respawn[respawn_index].state = BOT_IDLE;
      respawn_index = -1;  // indicate no slot used

      // fall through to next if statement (respawn_index will be -1)
   }

   // is the round over (time/frag limit) or has the bot been removed?
   if ((g_fGameOver) || (respawn_index == -1))
   {
      CSound *pSound;

      // keep resetting the sound entity until the bot is respawned...
      pSound = CSoundEnt::SoundPointerForIndex( CSoundEnt::ClientSoundIndex( edict() ) );
      if ( pSound )
      {
         pSound->Reset();
      }

      return;
   }

   pev->button = 0;  // make sure no buttons are pressed

   // if the bot is dead, randomly press fire to respawn...
   if ((pev->health < 1) || (pev->deadflag != DEAD_NO))
   {
      if (RANDOM_LONG(1, 100) > 50)
         pev->button = IN_ATTACK;

      g_engfuncs.pfnRunPlayerMove( edict( ), pev->v_angle, f_move_speed,
                                   0, 0, pev->button, 0,
                                   gpGlobals->frametime * 1000 );
      return;
   }

   // see if it's time to check for sv_maxspeed change...
   if (f_speed_check_time <= gpGlobals->time)
   {
      // store current sv_maxspeed setting for quick access
      f_max_speed = CVAR_GET_FLOAT("sv_maxspeed");

      // set next check time to 2 seconds from now
      f_speed_check_time = gpGlobals->time + 2.0;
   }

   // see how far bot has moved since the previous position...
   v_diff = v_prev_origin - pev->origin;
   moved_distance = v_diff.Length();

   v_prev_origin = pev->origin;  // save current position as previous

   f_move_speed = f_max_speed;  // set to max speed unless known otherwise

   // turn towards ideal_yaw by yaw_speed degrees
   degrees_turned = BotChangeYaw( pev->yaw_speed );

   if (degrees_turned >= pev->yaw_speed)
   {
      // if bot is still turning, turn in place by setting speed to 0
      f_move_speed = 0;
   }

   else if (IsOnLadder( ))  // check if the bot is on a ladder...
   {
      f_use_ladder_time = gpGlobals->time;

      BotOnLadder( moved_distance );  // go handle the ladder movement
   }

   else  // else handle movement related actions...
   {
      // bot is not on a ladder so clear ladder direction flag...
      ladder_dir = 0;

      if (f_botdontshoot == 0)  // is bot targeting turned on?
         pBotEnemy = BotFindEnemy( );
      else
         pBotEnemy = NULL;  // clear enemy pointer (no ememy for you!)

      if (pBotEnemy != NULL)  // does an enemy exist?
      {
         BotShootAtEnemy( );  // shoot at the enemy
      }

      else if (f_pause_time > gpGlobals->time)  // is bot "paused"?
      {
         // you could make the bot look left then right, or look up
         // and down, to make it appear that the bot is hunting for
         // something (don't do anything right now)

         f_move_speed = 0;
      }

      // is bot being "used" and can still follow "user"?
      else if ((pBotUser != NULL) && BotFollowUser( ))
      {
         // do nothing here!
         ;
      }

      else
      {
         // no enemy, let's just wander around...

         pev->v_angle.x = 0;  // reset pitch to 0 (level horizontally)
         pev->v_angle.z = 0;  // reset roll to 0 (straight up and down)

         pev->angles.x = 0;
         pev->angles.y = pev->v_angle.y;
         pev->angles.z = 0;

         // check if bot should look for items now or not...
         if (f_find_item < gpGlobals->time)
         {
            BotFindItem( );  // see if there are any visible items
         }

         // check if bot sees a tripmine...
         if (b_see_tripmine)
         {
            // check if bot can shoot the tripmine...
            if ((b_shoot_tripmine) && BotShootTripmine( ))
            {
               // shot at tripmine, may or may not have hit it, clear
               // flags anyway, next BotFindItem will see it again if
               // it is still there...

               b_shoot_tripmine = FALSE;
               b_see_tripmine = FALSE;

               // pause for a while to allow tripmine to explode...
               f_pause_time = gpGlobals->time + 0.5;
            }
            else  // run away!!!
            {
               Vector tripmine_angles;

               tripmine_angles = UTIL_VecToAngles( v_tripmine_origin - pev->origin );

               // face away from the tripmine
               pev->ideal_yaw = -tripmine_angles.y;

               // check for wrap around of angle...
               if (pev->ideal_yaw > 180)
                  pev->ideal_yaw -= 360;
               if (pev->ideal_yaw < -180)
                  pev->ideal_yaw += 360;

               b_see_tripmine = FALSE;

               f_move_speed = 0;  // don't run while turning
            }
         }

         // check if should use wall mounted health station...
         else if (b_use_health_station)
         {
            if ((f_use_health_time + 10.0) > gpGlobals->time)
            {
               f_move_speed = 0;  // don't move while using health station

               pev->button = IN_USE;
            }
            else
            {
               // bot is stuck trying to "use" a health station...

               b_use_health_station = FALSE;

               // don't look for items for a while since the bot
               // could be stuck trying to get to an item
               f_find_item = gpGlobals->time + 0.5;
            }
         }

         // check if should use wall mounted HEV station...
         else if (b_use_HEV_station)
         {
            if ((f_use_HEV_time + 10.0) > gpGlobals->time)
            {
               f_move_speed = 0;  // don't move while using HEV station

               pev->button = IN_USE;
            }
            else
            {
               // bot is stuck trying to "use" a HEV station...

               b_use_HEV_station = FALSE;

               // don't look for items for a while since the bot
               // could be stuck trying to get to an item
               f_find_item = gpGlobals->time + 0.5;
            }
         }

         else if (b_use_button)
         {
            f_move_speed = 0;  // don't move while using elevator

            BotUseLift( moved_distance );
         }

         else
         {
            TraceResult tr;

            if (pev->waterlevel == 3)  // check if the bot is underwater...
            {
               BotUnderWater( );
            }

            // check if there is a wall on the left...
            if (!BotCheckWallOnLeft())
            {
               // if there was a wall on the left over 1/2 a second ago then
               // 20% of the time randomly turn between 45 and 60 degrees
           
               if ((f_wall_on_left != 0) &&
                   (f_wall_on_left <= gpGlobals->time - 0.5) &&
                   (RANDOM_LONG(1, 100) <= 20))
               {
                  pev->ideal_yaw += RANDOM_LONG(45, 60);

                  // check for wrap around of angle...
                  if (pev->ideal_yaw > 180)
                     pev->ideal_yaw -= 360;
                  if (pev->ideal_yaw < -180)
                     pev->ideal_yaw += 360;

                  f_move_speed = 0;  // don't move while turning
                  f_dont_avoid_wall_time = gpGlobals->time + 1.0;
               }

               f_wall_on_left = 0;  // reset wall detect time
            }

            // check if there is a wall on the right...
            if (!BotCheckWallOnRight())
            {
               // if there was a wall on the right over 1/2 a second ago then
               // 20% of the time randomly turn between 45 and 60 degrees

               if ((f_wall_on_right != 0) &&
                   (f_wall_on_right <= gpGlobals->time - 0.5) &&
                   (RANDOM_LONG(1, 100) <= 20))
               {
                  pev->ideal_yaw -= RANDOM_LONG(45, 60);

                  // check for wrap around of angle...
                  if (pev->ideal_yaw > 180)
                     pev->ideal_yaw -= 360;
                  if (pev->ideal_yaw < -180)
                     pev->ideal_yaw += 360;

                  f_move_speed = 0;  // don't move while turning
                  f_dont_avoid_wall_time = gpGlobals->time + 1.0;
               }

               f_wall_on_right = 0;  // reset wall detect time
            }

            // check if bot is about to hit a wall.  TraceResult gets returned
            if ((f_dont_avoid_wall_time <= gpGlobals->time) &&
                BotCantMoveForward( &tr ))
            {
               // ADD LATER
               // need to check if bot can jump up or duck under here...
               // ADD LATER

               BotTurnAtWall( &tr );
            }

            // check if the bot hasn't moved much since the last location
            else if ((moved_distance <= 1) && (!bot_was_paused))
            {
               // the bot must be stuck!
            
               if (BotCanJumpUp( ))  // can the bot jump onto something?
               {
                  pev->button |= IN_JUMP;  // jump up and move forward
               }
               else if (BotCanDuckUnder( ))  // can the bot duck under something?
               {
                  pev->button |= IN_DUCK;  // duck down and move forward
               }
               else
               {
                  f_move_speed = 0;  // don't move while turning

                  // turn randomly between 30 and 60 degress
                  if (wander_dir == WANDER_LEFT)
                     pev->ideal_yaw += RANDOM_LONG(30, 60);
                  else
                     pev->ideal_yaw -= RANDOM_LONG(30, 60);

                  // check for wrap around of angle...
                  if (pev->ideal_yaw > 180)
                     pev->ideal_yaw -= 360;
                  if (pev->ideal_yaw < -180)
                     pev->ideal_yaw += 360;

                  // is the bot trying to get to an item?...
                  if (pBotPickupItem != NULL)
                  {
                     // don't look for items for a while since the bot
                     // could be stuck trying to get to an item
                     f_find_item = gpGlobals->time + 0.5;
                  }
               }
            }

            // should the bot pause for a while here?
            if ((RANDOM_LONG(1, 1000) <= pause_frequency[bot_skill]) &&
                (pBotUser == NULL))  // don't pause if being "used"
            {
               // set the time that the bot will stop "pausing"
               f_pause_time = gpGlobals->time +
                  RANDOM_FLOAT(pause_time[bot_skill][0],
                               pause_time[bot_skill][1]);
               f_move_speed = 0;  // don't move while turning
            }
         }
      }
   }

   if (f_move_speed < 1)
      bot_was_paused = TRUE;
   else
      bot_was_paused = FALSE;

   // TheFatal START - from www.telefragged.com/thefatal/jumblow.shtml

   g_engfuncs.pfnRunPlayerMove( edict( ), pev->v_angle, f_move_speed,
                                0, 0, pev->button, 0,
                                gpGlobals->frametime * 1000 );
   // TheFatal - END
}

