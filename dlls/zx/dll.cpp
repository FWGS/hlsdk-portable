//
// HPB bot - botman's High Ping Bastard bot
//
// (http://planethalflife.com/botman/)
//
// dll.cpp
//

#include "extdll.h"
#include "enginecallback.h"
#include "util.h"
#include "cbase.h"
#include "entity_state.h"
#include "usercmd.h"

#include "bot.h"
#include "bot_func.h"
#include "waypoint.h"


#define VER_MAJOR 3
#define VER_MINOR 0


#define MENU_NONE  0
#define MENU_1     1
#define MENU_2     2
#define MENU_3     3
#define MENU_4     4


extern GETENTITYAPI other_GetEntityAPI;
extern GETNEWDLLFUNCTIONS other_GetNewDLLFunctions;
extern enginefuncs_t g_engfuncs;
extern int debug_engine;
extern globalvars_t  *gpGlobals;
extern char *g_argv;
extern bool g_waypoint_on;
extern bool g_auto_waypoint;
extern bool g_path_waypoint;
extern bool g_path_waypoint_enable;
extern int num_waypoints;  // number of waypoints currently in use
extern WAYPOINT waypoints[MAX_WAYPOINTS];
extern float wp_display_time[MAX_WAYPOINTS];
extern bot_t bots[32];
extern bool b_observer_mode;
extern bool b_botdontshoot;
extern char welcome_msg[80];

static FILE *fp;

DLL_FUNCTIONS other_gFunctionTable;
DLL_GLOBAL const Vector g_vecZero = Vector(0,0,0);

int mod_id = 0;
int m_spriteTexture = 0;
int default_bot_skill = 2;
int bot_strafe_percent = 20; // percent of time to strafe
int bot_chat_percent = 10;   // percent of time to chat
int bot_taunt_percent = 20;  // percent of time to taunt after kill
int bot_whine_percent = 10;  // percent of time to whine after death
int bot_grenade_time = 15;   // seconds between grenade throws
int bot_logo_percent = 40;   // percent of time to spray logo after kill

int bot_chat_tag_percent = 80;   // percent of the time to drop clan tag
int bot_chat_drop_percent = 10;  // percent of the time to drop characters
int bot_chat_swap_percent = 10;  // percent of the time to swap characters
int bot_chat_lower_percent = 50; // percent of the time to lowercase chat

bool b_random_color = TRUE;
int isFakeClientCommand = 0;
int fake_arg_count;
int IsDedicatedServer;
float bot_check_time = 60.0;
int bot_reaction_time = 2;
int min_bots = -1;
int max_bots = -1;
int num_bots = 0;
int prev_num_bots = 0;
bool g_GameRules = FALSE;
edict_t *clients[32];
edict_t *listenserver_edict = NULL;
float welcome_time = 0.0;
bool welcome_sent = FALSE;
int g_menu_waypoint;
int g_menu_state = 0;
int bot_stop = 0;
int jumppad_off = 0;

bool is_team_play = FALSE;
char team_names[MAX_TEAMS][MAX_TEAMNAME_LENGTH];
int num_teams = 0;
bool checked_teamplay = FALSE;
edict_t *pent_info_tfdetect = NULL;
edict_t *pent_info_ctfdetect = NULL;
edict_t *pent_info_frontline = NULL;
edict_t *pent_item_tfgoal = NULL;
edict_t *pent_info_tfgoal = NULL;
int max_team_players[4];
int team_class_limits[4];
int team_allies[4];  // bit mapped allies BLUE, RED, YELLOW, and GREEN
int max_teams = 0;
int num_flags = 0;
FLAG_S flags[MAX_FLAGS];
int num_backpacks = 0;
BACKPACK_S backpacks[MAX_BACKPACKS];

FILE *bot_cfg_fp = NULL;
bool need_to_open_cfg = TRUE;
float bot_cfg_pause_time = 0.0;
float respawn_time = 0.0;
bool spawn_time_reset = FALSE;


cvar_t sv_bot = {"HPB_bot",""};

char *show_menu_none = {" "};
char *show_menu_1 =
   {"Waypoint Tags\n\n1. Team Specific\n2. Wait for Lift\n3. Door\n4. Sniper Spot\n5. More..."};
char *show_menu_2 =
   {"Waypoint Tags\n\n1. Team 1\n2. Team 2\n3. Team 3\n4. Team 4\n5. CANCEL"};
char *show_menu_2_flf =
   {"Waypoint Tags\n\n1. Attackers\n2. Defenders\n\n5. CANCEL"};
char *show_menu_3 =
   {"Waypoint Tags\n\n1. Flag Location\n2. Flag Goal Location\n3. Sentry gun\n4. Dispenser\n5. More"};
char *show_menu_3_flf =
   {"Waypoint Tags\n\n1. Capture Point\n2. Defend Point\n3. Prone\n\n5. CANCEL"};
char *show_menu_3_hw =
   {"Waypoint Tags\n\n1. Halo Location\n\n\n\n5. More"};
char *show_menu_4 =
   {"Waypoint Tags\n\n1. Health\n2. Armor\n3. Ammo\n4. Jump\n5. CANCEL"};


void BotNameInit(void);
void BotLogoInit(void);
void UpdateClientData(const struct edict_s *ent, int sendweapons, struct clientdata_s *cd);
void ProcessBotCfgFile(void);

extern void welcome_init(void);


void GameDLLInit( void )
{
   int i;

   CVAR_REGISTER (&sv_bot);

   IsDedicatedServer = IS_DEDICATED_SERVER();

   for (i=0; i<32; i++)
      clients[i] = NULL;

   welcome_init();

   // initialize the bots array of structures...
   memset(bots, 0, sizeof(bots));

   BotNameInit();
   BotLogoInit();

   LoadBotChat();
   LoadBotModels();

   (*other_gFunctionTable.pfnGameInit)();
}

int DispatchSpawn( edict_t *pent )
{
   int index;

   if (gpGlobals->deathmatch)
   {
      char *pClassname = (char *)STRING(pent->v.classname);

      if (debug_engine)
      {
         fp=fopen("HPB_bot.txt","a");
         fprintf(fp, "DispatchSpawn: %x %s\n",pent,pClassname);
         if (pent->v.model != 0)
            fprintf(fp, " model=%s\n",STRING(pent->v.model));
         fclose(fp);
      }

      if (strcmp(pClassname, "worldspawn") == 0)
      {
         // do level initialization stuff here...

         WaypointInit();
         WaypointLoad(NULL);

         pent_info_tfdetect = NULL;
         pent_info_ctfdetect = NULL;
         pent_info_frontline = NULL;
         pent_item_tfgoal = NULL;
         pent_info_tfgoal = NULL;

         for (index=0; index < 4; index++)
         {
            max_team_players[index] = 0;  // no player limit
            team_class_limits[index] = 0;  // no class limits
            team_allies[index] = 0;
         }

         max_teams = 0;
         num_flags = 0;

         for (index=0; index < MAX_FLAGS; index++)
         {
            flags[index].edict = NULL;
            flags[index].team_no = 0;  // any team unless specified
         }

         num_backpacks = 0;

         for (index=0; index < MAX_BACKPACKS; index++)
         {
            backpacks[index].edict = NULL;
            backpacks[index].armor = 0;
            backpacks[index].health = 0;
            backpacks[index].ammo = 0;
            backpacks[index].team = 0;  // any team unless specified
         }

         PRECACHE_SOUND("weapons/xbow_hit1.wav");      // waypoint add
         PRECACHE_SOUND("weapons/mine_activate.wav");  // waypoint delete
         PRECACHE_SOUND("common/wpn_hudoff.wav");      // path add/delete start
         PRECACHE_SOUND("common/wpn_hudon.wav");       // path add/delete done
         PRECACHE_SOUND("common/wpn_moveselect.wav");  // path add/delete cancel
         PRECACHE_SOUND("common/wpn_denyselect.wav");  // path add/delete error
         PRECACHE_SOUND("player/sprayer.wav");         // logo spray sound

         m_spriteTexture = PRECACHE_MODEL( "sprites/lgtning.spr");

         g_GameRules = TRUE;

         is_team_play = FALSE;
         memset(team_names, 0, sizeof(team_names));
         num_teams = 0;
         checked_teamplay = FALSE;

         bot_cfg_pause_time = 0.0;
         respawn_time = 0.0;
         spawn_time_reset = FALSE;

         prev_num_bots = num_bots;
         num_bots = 0;

         bot_check_time = gpGlobals->time + 60.0;
      }

      if ((mod_id == HOLYWARS_DLL) && (jumppad_off) &&
          (strcmp(pClassname, "trigger_jumppad") == 0))
      {
         return -1;  // disable jumppads
      }
   }

   return (*other_gFunctionTable.pfnSpawn)(pent);
}

void DispatchThink( edict_t *pent )
{
   (*other_gFunctionTable.pfnThink)(pent);
}

void DispatchUse( edict_t *pentUsed, edict_t *pentOther )
{
   (*other_gFunctionTable.pfnUse)(pentUsed, pentOther);
}

void DispatchTouch( edict_t *pentTouched, edict_t *pentOther )
{
   (*other_gFunctionTable.pfnTouch)(pentTouched, pentOther);
}

void DispatchBlocked( edict_t *pentBlocked, edict_t *pentOther )
{
   (*other_gFunctionTable.pfnBlocked)(pentBlocked, pentOther);
}

void DispatchKeyValue( edict_t *pentKeyvalue, KeyValueData *pkvd )
{
   static edict_t *temp_pent;
   static int flag_index;
   static int backpack_index;

   if (debug_engine)
   {
      fp=fopen("HPB_bot.txt","a"); fprintf(fp, "DispatchKeyValue: %x %s=%s\n",pentKeyvalue,pkvd->szKeyName,pkvd->szValue); fclose(fp);
   }

   if (mod_id == TFC_DLL)
   {
      if (pentKeyvalue == pent_info_tfdetect)
      {
         if (strcmp(pkvd->szKeyName, "ammo_medikit") == 0)  // max BLUE players
            max_team_players[0] = atoi(pkvd->szValue);
         else if (strcmp(pkvd->szKeyName, "ammo_detpack") == 0)  // max RED players
            max_team_players[1] = atoi(pkvd->szValue);
         else if (strcmp(pkvd->szKeyName, "maxammo_medikit") == 0)  // max YELLOW players
            max_team_players[2] = atoi(pkvd->szValue);
         else if (strcmp(pkvd->szKeyName, "maxammo_detpack") == 0)  // max GREEN players
            max_team_players[3] = atoi(pkvd->szValue);

         else if (strcmp(pkvd->szKeyName, "maxammo_shells") == 0)  // BLUE class limits
            team_class_limits[0] = atoi(pkvd->szValue);
         else if (strcmp(pkvd->szKeyName, "maxammo_nails") == 0)  // RED class limits
            team_class_limits[1] = atoi(pkvd->szValue);
         else if (strcmp(pkvd->szKeyName, "maxammo_rockets") == 0)  // YELLOW class limits
            team_class_limits[2] = atoi(pkvd->szValue);
         else if (strcmp(pkvd->szKeyName, "maxammo_cells") == 0)  // GREEN class limits
            team_class_limits[3] = atoi(pkvd->szValue);

         else if (strcmp(pkvd->szKeyName, "team1_allies") == 0)  // BLUE allies
            team_allies[0] = atoi(pkvd->szValue);
         else if (strcmp(pkvd->szKeyName, "team2_allies") == 0)  // RED allies
            team_allies[1] = atoi(pkvd->szValue);
         else if (strcmp(pkvd->szKeyName, "team3_allies") == 0)  // YELLOW allies
            team_allies[2] = atoi(pkvd->szValue);
         else if (strcmp(pkvd->szKeyName, "team4_allies") == 0)  // GREEN allies
            team_allies[3] = atoi(pkvd->szValue);
      }
      else if (pent_info_tfdetect == NULL)
      {
         if ((strcmp(pkvd->szKeyName, "classname") == 0) &&
             (strcmp(pkvd->szValue, "info_tfdetect") == 0))
         {
            pent_info_tfdetect = pentKeyvalue;
         }
      }

      if (pentKeyvalue == pent_item_tfgoal)
      {
         if (strcmp(pkvd->szKeyName, "team_no") == 0)
            flags[flag_index].team_no = atoi(pkvd->szValue);

         if ((strcmp(pkvd->szKeyName, "mdl") == 0) &&
             ((strcmp(pkvd->szValue, "models/flag.mdl") == 0) ||
              (strcmp(pkvd->szValue, "models/keycard.mdl") == 0) ||
              (strcmp(pkvd->szValue, "models/ball.mdl") == 0)))
         {
            num_flags++;
         }
      }
      else if (pent_item_tfgoal == NULL)
      {
         if ((strcmp(pkvd->szKeyName, "classname") == 0) &&
             (strcmp(pkvd->szValue, "item_tfgoal") == 0))
         {
            if (num_flags < MAX_FLAGS)
            {
               pent_item_tfgoal = pentKeyvalue;

               flags[num_flags].edict = pentKeyvalue;

               flag_index = num_flags;  // in case the mdl comes before team_no
            }
         }
      }
      else
      {
         pent_item_tfgoal = NULL;  // reset for non-flag item_tfgoal's
      }


      if (pentKeyvalue != pent_info_tfgoal)  // different edict?
      {
         pent_info_tfgoal = NULL;  // reset
      }

      if (pentKeyvalue == pent_info_tfgoal)
      {
         if (strcmp(pkvd->szKeyName, "team_no") == 0)
            backpacks[backpack_index].team = atoi(pkvd->szValue);

         if (strcmp(pkvd->szKeyName, "armorvalue") == 0)
            backpacks[backpack_index].armor = atoi(pkvd->szValue);

         if (strcmp(pkvd->szKeyName, "health") == 0)
            backpacks[backpack_index].health = atoi(pkvd->szValue);

         if ((strcmp(pkvd->szKeyName, "ammo_nails") == 0) ||
             (strcmp(pkvd->szKeyName, "ammo_rockets") == 0) ||
             (strcmp(pkvd->szKeyName, "ammo_cells") == 0) ||
             (strcmp(pkvd->szKeyName, "ammo_shells") == 0))
            backpacks[backpack_index].ammo = atoi(pkvd->szValue);

         if ((strcmp(pkvd->szKeyName, "mdl") == 0) &&
             (strcmp(pkvd->szValue, "models/backpack.mdl") == 0))
         {
            num_backpacks++;
         }
      }
      else if (pent_info_tfgoal == NULL)
      {
         if (((strcmp(pkvd->szKeyName, "classname") == 0) &&
              (strcmp(pkvd->szValue, "info_tfgoal") == 0)) ||
             ((strcmp(pkvd->szKeyName, "classname") == 0) &&
              (strcmp(pkvd->szValue, "i_t_g") == 0)))
         {
            if (num_backpacks < MAX_BACKPACKS)
            {
               pent_info_tfgoal = pentKeyvalue;

               backpacks[num_backpacks].edict = pentKeyvalue;

               // in case the mdl comes before the other fields
               backpack_index = num_backpacks;
            }
         }
      }

      if ((strcmp(pkvd->szKeyName, "classname") == 0) &&
          ((strcmp(pkvd->szValue, "info_player_teamspawn") == 0) ||
           (strcmp(pkvd->szValue, "i_p_t") == 0)))
      {
         temp_pent = pentKeyvalue;
      }
      else if (pentKeyvalue == temp_pent)
      {
         if (strcmp(pkvd->szKeyName, "team_no") == 0)
         {
            int value = atoi(pkvd->szValue);

            if (value > max_teams)
               max_teams = value;
         }
      }
   }
   else if (mod_id == GEARBOX_DLL)
   {
      if (pent_info_ctfdetect == NULL)
      {
         if ((strcmp(pkvd->szKeyName, "classname") == 0) &&
             (strcmp(pkvd->szValue, "info_ctfdetect") == 0))
         {
            pent_info_ctfdetect = pentKeyvalue;
         }
      }
   }

   (*other_gFunctionTable.pfnKeyValue)(pentKeyvalue, pkvd);
}

void DispatchSave( edict_t *pent, SAVERESTOREDATA *pSaveData )
{
   (*other_gFunctionTable.pfnSave)(pent, pSaveData);
}

int DispatchRestore( edict_t *pent, SAVERESTOREDATA *pSaveData, int globalEntity )
{
   return (*other_gFunctionTable.pfnRestore)(pent, pSaveData, globalEntity);
}

void DispatchObjectCollsionBox( edict_t *pent )
{
   (*other_gFunctionTable.pfnSetAbsBox)(pent);
}

void SaveWriteFields( SAVERESTOREDATA *pSaveData, const char *pname, void *pBaseData, TYPEDESCRIPTION *pFields, int fieldCount )
{
   (*other_gFunctionTable.pfnSaveWriteFields)(pSaveData, pname, pBaseData, pFields, fieldCount);
}

void SaveReadFields( SAVERESTOREDATA *pSaveData, const char *pname, void *pBaseData, TYPEDESCRIPTION *pFields, int fieldCount )
{
   (*other_gFunctionTable.pfnSaveReadFields)(pSaveData, pname, pBaseData, pFields, fieldCount);
}

void SaveGlobalState( SAVERESTOREDATA *pSaveData )
{
   (*other_gFunctionTable.pfnSaveGlobalState)(pSaveData);
}

void RestoreGlobalState( SAVERESTOREDATA *pSaveData )
{
   (*other_gFunctionTable.pfnRestoreGlobalState)(pSaveData);
}

void ResetGlobalState( void )
{
   (*other_gFunctionTable.pfnResetGlobalState)();
}

BOOL ClientConnect( edict_t *pEntity, const char *pszName, const char *pszAddress, char szRejectReason[ 128 ]  )
{ 
   if (gpGlobals->deathmatch)
   {
      int i;
      int count = 0;

      if (debug_engine) { fp=fopen("HPB_bot.txt","a"); fprintf(fp, "ClientConnect: pent=%x name=%s\n",pEntity,pszName); fclose(fp); }

      // check if this client is the listen server client
      if (strcmp(pszAddress, "loopback") == 0)
      {
         // save the edict of the listen server client...
         listenserver_edict = pEntity;
      }

      // check if this is NOT a bot joining the server...
      if (strcmp(pszAddress, "127.0.0.1") != 0)
      {
         // don't try to add bots for 60 seconds, give client time to get added
         bot_check_time = gpGlobals->time + 60.0;

         for (i=0; i < 32; i++)
         {
            if (bots[i].is_used)  // count the number of bots in use
               count++;
         }

         // if there are currently more than the minimum number of bots running
         // then kick one of the bots off the server...
         if ((count > min_bots) && (min_bots != -1))
         {
            for (i=0; i < 32; i++)
            {
               if (bots[i].is_used)  // is this slot used?
               {
                  char cmd[80];

                  sprintf(cmd, "kick \"%s\"\n", bots[i].name);

                  SERVER_COMMAND(cmd);  // kick the bot using (kick "name")

                  break;
               }
            }
         }
      }
   }

   return (*other_gFunctionTable.pfnClientConnect)(pEntity, pszName, pszAddress, szRejectReason);
}

void ClientDisconnect( edict_t *pEntity )
{
   if (gpGlobals->deathmatch)
   {
      int i;

      if (debug_engine) { fp=fopen("HPB_bot.txt","a"); fprintf(fp, "ClientDisconnect: %x\n",pEntity); fclose(fp); }

      i = 0;
      while ((i < 32) && (clients[i] != pEntity))
         i++;

      if (i < 32)
         clients[i] = NULL;


      for (i=0; i < 32; i++)
      {
         if (bots[i].pEdict == pEntity)
         {
            // someone kicked this bot off of the server...

            bots[i].is_used = FALSE;  // this slot is now free to use

            bots[i].f_kick_time = gpGlobals->time;  // save the kicked time

            break;
         }
      }
   }

   (*other_gFunctionTable.pfnClientDisconnect)(pEntity);
}

void ClientKill( edict_t *pEntity )
{
   if (debug_engine) { fp=fopen("HPB_bot.txt","a"); fprintf(fp, "ClientKill: %x\n",pEntity); fclose(fp); }
   (*other_gFunctionTable.pfnClientKill)(pEntity);
}

void ClientPutInServer( edict_t *pEntity )
{
   if (debug_engine) { fp=fopen("HPB_bot.txt","a"); fprintf(fp, "ClientPutInServer: %x\n",pEntity); fclose(fp); }

   int i = 0;

   while ((i < 32) && (clients[i] != NULL))
      i++;

   if (i < 32)
      clients[i] = pEntity;  // store this clients edict in the clients array

   (*other_gFunctionTable.pfnClientPutInServer)(pEntity);
}


void ClientCommand( edict_t *pEntity )
{
   const char *pcmd = Cmd_Argv(0);
   const char *arg1 = Cmd_Argv(1);
   const char *arg2 = Cmd_Argv(2);
   const char *arg3 = Cmd_Argv(3);
   const char *arg4 = Cmd_Argv(4);
   const char *arg5 = Cmd_Argv(5);

   if (debug_engine)
   {
      fp=fopen("HPB_bot.txt","a"); fprintf(fp,"ClientCommand: %s",pcmd);
      if ((arg1 != NULL) && (*arg1 != 0))
         fprintf(fp," %s", arg1);
      if ((arg2 != NULL) && (*arg2 != 0))
         fprintf(fp," %s", arg2);
      if ((arg3 != NULL) && (*arg3 != 0))
         fprintf(fp," %s", arg3);
      if ((arg4 != NULL) && (*arg4 != 0))
         fprintf(fp," %s", arg4);
      if ((arg5 != NULL) && (*arg5 != 0))
         fprintf(fp," %s", arg5);
      fprintf(fp, "\n");
      fclose(fp);
   }

   // only allow custom commands if deathmatch mode and NOT dedicated server and
   // client sending command is the listen server client...

   if ((gpGlobals->deathmatch) && (!IsDedicatedServer) &&
       (pEntity == listenserver_edict))
   {
      char msg[80];

      if (FStrEq(pcmd, "addbot"))
      {
         BotCreate( pEntity, arg1, arg2, arg3, arg4, arg5 );

         bot_check_time = gpGlobals->time + 5.0;

         return;
      }
      else if (FStrEq(pcmd, "observer"))
      {
         if ((arg1 != NULL) && (*arg1 != 0))
         {
            int temp = atoi(arg1);
            if (temp)
               b_observer_mode = TRUE;
            else
               b_observer_mode = FALSE;
         }

         if (b_observer_mode)
            ClientPrint(pEntity, HUD_PRINTNOTIFY, "observer mode ENABLED\n");
         else
            ClientPrint(pEntity, HUD_PRINTNOTIFY, "observer mode DISABLED\n");

         return;
      }
      else if (FStrEq(pcmd, "botskill"))
      {
         if ((arg1 != NULL) && (*arg1 != 0))
         {
            int temp = atoi(arg1);

            if ((temp < 1) || (temp > 5))
               ClientPrint(pEntity, HUD_PRINTNOTIFY, "invalid botskill value!\n");
            else
               default_bot_skill = temp;
         }

         sprintf(msg, "botskill is %d\n", default_bot_skill);
         ClientPrint(pEntity, HUD_PRINTNOTIFY, msg);

         return;
      }
      else if (FStrEq(pcmd, "bot_strafe_percent"))
      {
         if ((arg1 != NULL) && (*arg1 != 0))
         {
            int temp = atoi(arg1);

            if ((temp < 0) || (temp > 100))
               ClientPrint(pEntity, HUD_PRINTNOTIFY, "invalid bot_strafe_percent value!\n");
            else
               bot_strafe_percent = temp;
         }

         sprintf(msg, "bot_strafe_percent is %d\n", bot_strafe_percent);
         ClientPrint(pEntity, HUD_PRINTNOTIFY, msg);

         return;
      }
      else if (FStrEq(pcmd, "bot_chat_percent"))
      {
         if ((arg1 != NULL) && (*arg1 != 0))
         {
            int temp = atoi(arg1);

            if ((temp < 0) || (temp > 100))
               ClientPrint(pEntity, HUD_PRINTNOTIFY, "invalid bot_chat_percent value!\n");
            else
               bot_chat_percent = temp;
         }

         sprintf(msg, "bot_chat_percent is %d\n", bot_chat_percent);
         ClientPrint(pEntity, HUD_PRINTNOTIFY, msg);

         return;
      }
      else if (FStrEq(pcmd, "bot_taunt_percent"))
      {
         if ((arg1 != NULL) && (*arg1 != 0))
         {
            int temp = atoi(arg1);

            if ((temp < 0) || (temp > 100))
               ClientPrint(pEntity, HUD_PRINTNOTIFY, "invalid bot_taunt_percent value!\n");
            else
               bot_taunt_percent = temp;
         }

         sprintf(msg, "bot_taunt_percent is %d\n", bot_taunt_percent);
         ClientPrint(pEntity, HUD_PRINTNOTIFY, msg);

         return;
      }
      else if (FStrEq(pcmd, "bot_whine_percent"))
      {
         if ((arg1 != NULL) && (*arg1 != 0))
         {
            int temp = atoi(arg1);

            if ((temp < 0) || (temp > 100))
               ClientPrint(pEntity, HUD_PRINTNOTIFY, "invalid bot_whine_percent value!\n");
            else
               bot_whine_percent = temp;
         }

         sprintf(msg, "bot_whine_percent is %d\n", bot_whine_percent);
         ClientPrint(pEntity, HUD_PRINTNOTIFY, msg);

         return;
      }
      else if (FStrEq(pcmd, "bot_chat_tag_percent"))
      {
         if ((arg1 != NULL) && (*arg1 != 0))
         {
            int temp = atoi(arg1);

            if ((temp < 0) || (temp > 100))
               ClientPrint(pEntity, HUD_PRINTNOTIFY, "invalid bot_chat_tag_percent value!\n");
            else
               bot_chat_tag_percent = temp;
         }

         sprintf(msg, "bot_chat_tag_percent is %d\n", bot_chat_tag_percent);
         ClientPrint(pEntity, HUD_PRINTNOTIFY, msg);

         return;
      }
      else if (FStrEq(pcmd, "bot_chat_drop_percent"))
      {
         if ((arg1 != NULL) && (*arg1 != 0))
         {
            int temp = atoi(arg1);

            if ((temp < 0) || (temp > 100))
               ClientPrint(pEntity, HUD_PRINTNOTIFY, "invalid bot_chat_drop_percent value!\n");
            else
               bot_chat_drop_percent = temp;
         }

         sprintf(msg, "bot_chat_drop_percent is %d\n", bot_chat_drop_percent);
         ClientPrint(pEntity, HUD_PRINTNOTIFY, msg);

         return;
      }
      else if (FStrEq(pcmd, "bot_chat_swap_percent"))
      {
         if ((arg1 != NULL) && (*arg1 != 0))
         {
            int temp = atoi(arg1);

            if ((temp < 0) || (temp > 100))
               ClientPrint(pEntity, HUD_PRINTNOTIFY, "invalid bot_chat_swap_percent value!\n");
            else
               bot_chat_swap_percent = temp;
         }

         sprintf(msg, "bot_chat_swap_percent is %d\n", bot_chat_swap_percent);
         ClientPrint(pEntity, HUD_PRINTNOTIFY, msg);

         return;
      }
      else if (FStrEq(pcmd, "bot_chat_lower_percent"))
      {
         if ((arg1 != NULL) && (*arg1 != 0))
         {
            int temp = atoi(arg1);

            if ((temp < 0) || (temp > 100))
               ClientPrint(pEntity, HUD_PRINTNOTIFY, "invalid bot_chat_lower_percent value!\n");
            else
               bot_chat_lower_percent = temp;
         }

         sprintf(msg, "bot_chat_lower_percent is %d\n", bot_chat_lower_percent);
         ClientPrint(pEntity, HUD_PRINTNOTIFY, msg);

         return;
      }
      else if (FStrEq(pcmd, "bot_grenade_time"))
      {
         if ((arg1 != NULL) && (*arg1 != 0))
         {
            int temp = atoi(arg1);

            if ((temp < 0) || (temp > 60))
               ClientPrint(pEntity, HUD_PRINTNOTIFY, "invalid bot_grenade_time value!\n");
            else
               bot_grenade_time = temp;
         }

         sprintf(msg, "bot_grenade_time is %d\n", bot_grenade_time);
         ClientPrint(pEntity, HUD_PRINTNOTIFY, msg);

         return;
      }
      else if (FStrEq(pcmd, "bot_logo_percent"))
      {
         if ((arg1 != NULL) && (*arg1 != 0))
         {
            int temp = atoi(arg1);

            if ((temp < 0) || (temp > 100))
               ClientPrint(pEntity, HUD_PRINTNOTIFY, "invalid bot_logo_percent value!\n");
            else
               bot_logo_percent = temp;
         }

         sprintf(msg, "bot_logo_percent is %d\n", bot_logo_percent);
         ClientPrint(pEntity, HUD_PRINTNOTIFY, msg);

         return;
      }
      else if (FStrEq(pcmd, "bot_reaction_time"))
      {
         if ((arg1 != NULL) && (*arg1 != 0))
         {
            int temp = atoi(arg1);

            if ((temp < 0) || (temp > 3))
               ClientPrint(pEntity, HUD_PRINTNOTIFY, "invalid bot_reaction_time value!\n");
            else
               bot_reaction_time = temp;
         }

         if (bot_reaction_time)
            sprintf(msg, "bot_reaction_time is %d\n", bot_reaction_time);
         else
            sprintf(msg, "bot_reaction_time is DISABLED\n");

         ClientPrint(pEntity, HUD_PRINTNOTIFY, msg);

         return;
      }
      else if (FStrEq(pcmd, "random_color"))
      {
         if ((arg1 != NULL) && (*arg1 != 0))
         {
            int temp = atoi(arg1);

            if (temp)
               b_random_color = TRUE;
            else
               b_random_color = FALSE;
         }

         if (b_random_color)
            ClientPrint(pEntity, HUD_PRINTNOTIFY, "random_color ENABLED\n");
         else
            ClientPrint(pEntity, HUD_PRINTNOTIFY, "random_color DISABLED\n");

         return;
      }
      else if (FStrEq(pcmd, "botdontshoot"))
      {
         if ((arg1 != NULL) && (*arg1 != 0))
         {
            int temp = atoi(arg1);
            if (temp)
               b_botdontshoot = TRUE;
            else
               b_botdontshoot = FALSE;
         }

         if (b_botdontshoot)
            ClientPrint(pEntity, HUD_PRINTNOTIFY, "botdontshoot ENABLED\n");
         else
            ClientPrint(pEntity, HUD_PRINTNOTIFY, "botdontshoot DISABLED\n");

         return;
      }
      else if (FStrEq(pcmd, "debug_engine"))
      {
         debug_engine = 1;

         ClientPrint(pEntity, HUD_PRINTNOTIFY, "debug_engine enabled!\n");

         return;
      }
      else if (FStrEq(pcmd, "waypoint"))
      {
         if (FStrEq(arg1, "on"))
         {
            g_waypoint_on = TRUE;

            ClientPrint(pEntity, HUD_PRINTNOTIFY, "waypoints are ON\n");
         }
         else if (FStrEq(arg1, "off"))
         {
            g_waypoint_on = FALSE;

            ClientPrint(pEntity, HUD_PRINTNOTIFY, "waypoints are OFF\n");
         }
         else if (FStrEq(arg1, "add"))
         {
            if (!g_waypoint_on)
               g_waypoint_on = TRUE;  // turn waypoints on if off

            WaypointAdd(pEntity);
         }
         else if (FStrEq(arg1, "delete"))
         {
            if (!g_waypoint_on)
               g_waypoint_on = TRUE;  // turn waypoints on if off

            WaypointDelete(pEntity);
         }
         else if (FStrEq(arg1, "save"))
         {
            WaypointSave();

            ClientPrint(pEntity, HUD_PRINTNOTIFY, "waypoints saved\n");
         }
         else if (FStrEq(arg1, "load"))
         {
            if (WaypointLoad(pEntity))
               ClientPrint(pEntity, HUD_PRINTNOTIFY, "waypoints loaded\n");
         }
         else if (FStrEq(arg1, "menu"))
         {
            int index;

            if (num_waypoints < 1)
               return;

            index = WaypointFindNearest(pEntity, 50.0, -1);

            if (index == -1)
               return;

            g_menu_waypoint = index;
            g_menu_state = MENU_1;

            UTIL_ShowMenu(pEntity, 0x1F, -1, FALSE, show_menu_1);
         }
         else if (FStrEq(arg1, "info"))
         {
            WaypointPrintInfo(pEntity);
         }
         else if (FStrEq(arg1, "update"))
         {
            ClientPrint(pEntity, HUD_PRINTNOTIFY, "updating waypoint tags...\n");

            WaypointUpdate(pEntity);

            ClientPrint(pEntity, HUD_PRINTNOTIFY, "...update done!  (don't forget to save!)\n");
         }
         else
         {
            if (g_waypoint_on)
               ClientPrint(pEntity, HUD_PRINTNOTIFY, "waypoints are ON\n");
            else
               ClientPrint(pEntity, HUD_PRINTNOTIFY, "waypoints are OFF\n");
         }

         return;
      }
      else if (FStrEq(pcmd, "autowaypoint"))
      {
         if (FStrEq(arg1, "on"))
         {
            g_auto_waypoint = TRUE;
            g_waypoint_on = TRUE;  // turn this on just in case
         }
         else if (FStrEq(arg1, "off"))
         {
            g_auto_waypoint = FALSE;
         }

         if (g_auto_waypoint)
            sprintf(msg, "autowaypoint is ON\n");
         else
            sprintf(msg, "autowaypoint is OFF\n");

         ClientPrint(pEntity, HUD_PRINTNOTIFY, msg);

         return;
      }
      else if (FStrEq(pcmd, "pathwaypoint"))
      {
         if (FStrEq(arg1, "on"))
         {
            g_path_waypoint = TRUE;
            g_waypoint_on = TRUE;  // turn this on just in case

            ClientPrint(pEntity, HUD_PRINTNOTIFY, "pathwaypoint is ON\n");
         }
         else if (FStrEq(arg1, "off"))
         {
            g_path_waypoint = FALSE;

            ClientPrint(pEntity, HUD_PRINTNOTIFY, "pathwaypoint is OFF\n");
         }
         else if (FStrEq(arg1, "enable"))
         {
            g_path_waypoint_enable = TRUE;

            ClientPrint(pEntity, HUD_PRINTNOTIFY, "pathwaypoint is ENABLED\n");
         }
         else if (FStrEq(arg1, "disable"))
         {
            g_path_waypoint_enable = FALSE;

            ClientPrint(pEntity, HUD_PRINTNOTIFY, "pathwaypoint is DISABLED\n");
         }
         else if (FStrEq(arg1, "create1"))
         {
            WaypointCreatePath(pEntity, 1);
         }
         else if (FStrEq(arg1, "create2"))
         {
            WaypointCreatePath(pEntity, 2);
         }
         else if (FStrEq(arg1, "remove1"))
         {
            WaypointRemovePath(pEntity, 1);
         }
         else if (FStrEq(arg1, "remove2"))
         {
            WaypointRemovePath(pEntity, 2);
         }

         return;
      }
      else if (FStrEq(pcmd, "menuselect") && (g_menu_state != MENU_NONE))
      {
         if (g_menu_state == MENU_1)  // main menu...
         {
            if (FStrEq(arg1, "1"))  // team specific...
            {
               g_menu_state = MENU_2;  // display team specific menu...

               if (mod_id == FRONTLINE_DLL)
                  UTIL_ShowMenu(pEntity, 0x13, -1, FALSE, show_menu_2_flf);
               else
                  UTIL_ShowMenu(pEntity, 0x1F, -1, FALSE, show_menu_2);

               return;
            }
            else if (FStrEq(arg1, "2"))  // wait for lift...
            {
               if (waypoints[g_menu_waypoint].flags & W_FL_LIFT)
                  waypoints[g_menu_waypoint].flags &= ~W_FL_LIFT;  // off
               else
                  waypoints[g_menu_waypoint].flags |= W_FL_LIFT;  // on
            }
            else if (FStrEq(arg1, "3"))  // door waypoint
            {
               if (waypoints[g_menu_waypoint].flags & W_FL_DOOR)
                  waypoints[g_menu_waypoint].flags &= ~W_FL_DOOR;  // off
               else
                  waypoints[g_menu_waypoint].flags |= W_FL_DOOR;  // on
            }
            else if (FStrEq(arg1, "4"))  // sniper spot
            {
               if (waypoints[g_menu_waypoint].flags & W_FL_SNIPER)
                  waypoints[g_menu_waypoint].flags &= ~W_FL_SNIPER;  // off
               else
               {
                  waypoints[g_menu_waypoint].flags |= W_FL_SNIPER;  // on

                  // set the aiming waypoint...

                  WaypointAddAiming(pEntity);
               }
            }
            else if (FStrEq(arg1, "5"))  // more...
            {
               g_menu_state = MENU_3;

               if (mod_id == FRONTLINE_DLL)
                  UTIL_ShowMenu(pEntity, 0x17, -1, FALSE, show_menu_3_flf);
               else if (mod_id == HOLYWARS_DLL)
                  UTIL_ShowMenu(pEntity, 0x11, -1, FALSE, show_menu_3_hw);
               else
                  UTIL_ShowMenu(pEntity, 0x1F, -1, FALSE, show_menu_3);

               return;
            }
         }
         else if (g_menu_state == MENU_2)  // team specific menu
         {
            if (waypoints[g_menu_waypoint].flags & W_FL_TEAM_SPECIFIC)
            {
               waypoints[g_menu_waypoint].flags &= ~W_FL_TEAM;
               waypoints[g_menu_waypoint].flags &= ~W_FL_TEAM_SPECIFIC; // off
            }
            else
            {
               int team = atoi(arg1);

               team--;  // make 0 to 3

               // this is kind of a kludge (team bits MUST be LSB!!!)
               waypoints[g_menu_waypoint].flags |= team;
               waypoints[g_menu_waypoint].flags |= W_FL_TEAM_SPECIFIC; // on
            }
         }
         else if (g_menu_state == MENU_3)  // third menu...
         {
            if (mod_id == FRONTLINE_DLL)
            {
               if (FStrEq(arg1, "1"))  // capture point
               {
                  if (waypoints[g_menu_waypoint].flags & W_FL_FLF_CAP)
                     waypoints[g_menu_waypoint].flags &= ~W_FL_FLF_CAP;  // off
                  else
                     waypoints[g_menu_waypoint].flags |= W_FL_FLF_CAP;  // on
               }
               else if (FStrEq(arg1, "2"))  // defend point
               {
                  if (waypoints[g_menu_waypoint].flags & W_FL_FLF_DEFEND)
                     waypoints[g_menu_waypoint].flags &= ~W_FL_FLF_DEFEND;  // off
                  else
                  {
                     waypoints[g_menu_waypoint].flags |= W_FL_FLF_DEFEND;  // on

                     // set the aiming waypoint...

                     WaypointAddAiming(pEntity);
                  }
               }
               else if (FStrEq(arg1, "3"))  // go prone
               {
                  if (waypoints[g_menu_waypoint].flags & W_FL_PRONE)
                     waypoints[g_menu_waypoint].flags &= ~W_FL_PRONE;  // off
                  else
                     waypoints[g_menu_waypoint].flags |= W_FL_PRONE;  // on
               }
            }
            else if (mod_id == HOLYWARS_DLL)
            {
               if (FStrEq(arg1, "1"))  // flag location
               {
                  if (waypoints[g_menu_waypoint].flags & W_FL_FLAG)
                     waypoints[g_menu_waypoint].flags &= ~W_FL_FLAG;  // off
                  else
                     waypoints[g_menu_waypoint].flags |= W_FL_FLAG;  // on
               }
               else if (FStrEq(arg1, "5"))
               {
                  g_menu_state = MENU_4;

                  UTIL_ShowMenu(pEntity, 0x1F, -1, FALSE, show_menu_4);

                  return;
               }
            }
            else
            {
               if (FStrEq(arg1, "1"))  // flag location
               {
                  if (waypoints[g_menu_waypoint].flags & W_FL_FLAG)
                     waypoints[g_menu_waypoint].flags &= ~W_FL_FLAG;  // off
                  else
                     waypoints[g_menu_waypoint].flags |= W_FL_FLAG;  // on
               }
               else if (FStrEq(arg1, "2"))  // flag goal
               {
                  if (waypoints[g_menu_waypoint].flags & W_FL_FLAG_GOAL)
                     waypoints[g_menu_waypoint].flags &= ~W_FL_FLAG_GOAL;  // off
                  else
                     waypoints[g_menu_waypoint].flags |= W_FL_FLAG_GOAL;  // on
               }
               else if (FStrEq(arg1, "3"))  // sentry gun
               {
                  if (waypoints[g_menu_waypoint].flags & W_FL_SENTRYGUN)
                     waypoints[g_menu_waypoint].flags &= ~W_FL_SENTRYGUN;  // off
                  else
                  {
                     waypoints[g_menu_waypoint].flags |= W_FL_SENTRYGUN;  // on

                     // set the aiming waypoint...

                     WaypointAddAiming(pEntity);
                  }
               }
               else if (FStrEq(arg1, "4"))  // dispenser
               {
                  if (waypoints[g_menu_waypoint].flags & W_FL_DISPENSER)
                     waypoints[g_menu_waypoint].flags &= ~W_FL_DISPENSER;  // off
                  else
                  {
                     waypoints[g_menu_waypoint].flags |= W_FL_DISPENSER;  // on

                     // set the aiming waypoint...

                     WaypointAddAiming(pEntity);
                  }
               }
               else if (FStrEq(arg1, "5"))
               {
                  g_menu_state = MENU_4;

                  UTIL_ShowMenu(pEntity, 0x1F, -1, FALSE, show_menu_4);

                  return;
               }
            }
         }
         else if (g_menu_state == MENU_4)  // fourth menu...
         {
            if (FStrEq(arg1, "1"))  // health
            {
               if (waypoints[g_menu_waypoint].flags & W_FL_HEALTH)
                  waypoints[g_menu_waypoint].flags &= ~W_FL_HEALTH;  // off
               else
                  waypoints[g_menu_waypoint].flags |= W_FL_HEALTH;  // on
            }
            else if (FStrEq(arg1, "2"))  // armor
            {
               if (waypoints[g_menu_waypoint].flags & W_FL_ARMOR)
                  waypoints[g_menu_waypoint].flags &= ~W_FL_ARMOR;  // off
               else
                  waypoints[g_menu_waypoint].flags |= W_FL_ARMOR;  // on
            }
            else if (FStrEq(arg1, "3"))  // ammo
            {
               if (waypoints[g_menu_waypoint].flags & W_FL_AMMO)
                  waypoints[g_menu_waypoint].flags &= ~W_FL_AMMO;  // off
               else
                  waypoints[g_menu_waypoint].flags |= W_FL_AMMO;  // on
            }
            else if (FStrEq(arg1, "4"))  // jump
            {
               if (waypoints[g_menu_waypoint].flags & W_FL_JUMP)
                  waypoints[g_menu_waypoint].flags &= ~W_FL_JUMP;  // off
               else
               {
                  waypoints[g_menu_waypoint].flags |= W_FL_JUMP;  // on

                  // set the aiming waypoint...

                  WaypointAddAiming(pEntity);
               }
            }
         }

         g_menu_state = MENU_NONE;

         // turn off the text menu if the mod doesn't do this automatically
         if ((mod_id == HOLYWARS_DLL) || (mod_id == DMC_DLL))
            UTIL_ShowMenu(pEntity, 0x0, -1, FALSE, show_menu_none);

         return;
      }
      else if (FStrEq(pcmd, "search"))
      {
         edict_t *pent = NULL;
         float radius = 100;
         char str[80];

         ClientPrint(pEntity, HUD_PRINTNOTIFY, "searching...\n");

         while ((pent = UTIL_FindEntityInSphere( pent, pEntity->v.origin, radius )) != NULL)
         {
            sprintf(str, "Found %s at %5.2f %5.2f %5.2f\n",
                       STRING(pent->v.classname),
                       pent->v.origin.x, pent->v.origin.y,
                       pent->v.origin.z);
            ClientPrint(pEntity, HUD_PRINTNOTIFY, str);

            FILE *fp=fopen("HPB_bot.txt", "a");
            fprintf(fp, "ClientCommmand: search %s", str);
            fclose(fp);
         }

         return;
      }
      else if (FStrEq(pcmd, "jumppad"))
      {
         char str[80];

         if (FStrEq(arg1, "off"))
            jumppad_off = 1;
         if (FStrEq(arg1, "on"))
            jumppad_off = 0;

         if (jumppad_off)
            sprintf(str,"jumppad is OFF\n");
         else
            sprintf(str,"jumppad is ON\n");

         ClientPrint(pEntity, HUD_PRINTNOTIFY, str);

         return;
      }
#if _DEBUG
      else if (FStrEq(pcmd, "botstop"))
      {
         bot_stop = 1;

         return;
      }
      else if (FStrEq(pcmd, "botstart"))
      {
         bot_stop = 0;

         return;
      }
#endif
   }

   (*other_gFunctionTable.pfnClientCommand)(pEntity);
}

void ClientUserInfoChanged( edict_t *pEntity, char *infobuffer )
{
   if (debug_engine) { fp=fopen("HPB_bot.txt", "a"); fprintf(fp, "ClientUserInfoChanged: pEntity=%x infobuffer=%s\n", pEntity, infobuffer); fclose(fp); }

   (*other_gFunctionTable.pfnClientUserInfoChanged)(pEntity, infobuffer);
}

void ServerActivate( edict_t *pEdictList, int edictCount, int clientMax )
{
   (*other_gFunctionTable.pfnServerActivate)(pEdictList, edictCount, clientMax);
}

void ServerDeactivate( void )
{
   (*other_gFunctionTable.pfnServerDeactivate)();
}

void PlayerPreThink( edict_t *pEntity )
{
   (*other_gFunctionTable.pfnPlayerPreThink)(pEntity);
}

void PlayerPostThink( edict_t *pEntity )
{
   (*other_gFunctionTable.pfnPlayerPostThink)(pEntity);
}

void StartFrame( void )
{
   if (gpGlobals->deathmatch)
   {
      edict_t *pPlayer;
      static float check_server_cmd = 0.0;
      static int i, index, player_index, bot_index;
      static float previous_time = -1.0;
      static float client_update_time = 0.0;
      clientdata_s cd;
      char msg[256];
      int count;

      // if a new map has started then (MUST BE FIRST IN StartFrame)...
      if ((gpGlobals->time + 0.1) < previous_time)
      {
         char filename[256];
         char mapname[64];

         check_server_cmd = 0.0;  // reset at start of map

         // check if mapname_bot.cfg file exists...

         strcpy(mapname, STRING(gpGlobals->mapname));
         strcat(mapname, "_HPB_bot.cfg");

         UTIL_BuildFileName(filename, "maps", mapname);

         if ((bot_cfg_fp = fopen(filename, "r")) != NULL)
         {
            sprintf(msg, "Executing %s\n", filename);
            ALERT( at_console, msg );

            for (index = 0; index < 32; index++)
            {
               bots[index].is_used = FALSE;
               bots[index].respawn_state = 0;
               bots[index].f_kick_time = 0.0;
            }

            if (IsDedicatedServer)
               bot_cfg_pause_time = gpGlobals->time + 5.0;
            else
               bot_cfg_pause_time = gpGlobals->time + 20.0;
         }
         else
         {
            count = 0;

            // mark the bots as needing to be respawned...
            for (index = 0; index < 32; index++)
            {
               if (count >= prev_num_bots)
               {
                  bots[index].is_used = FALSE;
                  bots[index].respawn_state = 0;
                  bots[index].f_kick_time = 0.0;
               }

               if (bots[index].is_used)  // is this slot used?
               {
                  bots[index].respawn_state = RESPAWN_NEED_TO_RESPAWN;
                  count++;
               }

               // check for any bots that were very recently kicked...
               if ((bots[index].f_kick_time + 5.0) > previous_time)
               {
                  bots[index].respawn_state = RESPAWN_NEED_TO_RESPAWN;
                  count++;
               }
               else
                  bots[index].f_kick_time = 0.0;  // reset to prevent false spawns later
            }

            // set the respawn time
            if (IsDedicatedServer)
               respawn_time = gpGlobals->time + 5.0;
            else
               respawn_time = gpGlobals->time + 20.0;
         }

         client_update_time = gpGlobals->time + 10.0;  // start updating client data again

         bot_check_time = gpGlobals->time + 60.0;
      }

      if (!IsDedicatedServer)
      {
         if ((listenserver_edict != NULL) && (welcome_sent == FALSE) &&
             (welcome_time < 1.0))
         {
            // are they out of observer mode yet?
            if (IsAlive(listenserver_edict))
               welcome_time = gpGlobals->time + 5.0;  // welcome in 5 seconds
         }

         if ((welcome_time > 0.0) && (welcome_time < gpGlobals->time) &&
             (welcome_sent == FALSE))
         {
            char version[80];

            sprintf(version,"%s Version %d.%d\n", welcome_msg, VER_MAJOR, VER_MINOR);

            // let's send a welcome message to this client...
            UTIL_SayText(version, listenserver_edict);

            welcome_sent = TRUE;  // clear this so we only do it once
         }
      }

      if (mod_id != DMC_DLL)
      {
         if ((client_update_time <= gpGlobals->time) && (mod_id != DMC_DLL))
         {
            client_update_time = gpGlobals->time + 1.0;

            for (i=0; i < 32; i++)
            {
               if (bots[i].is_used)
               {
                  memset(&cd, 0, sizeof(cd));

                  UpdateClientData( bots[i].pEdict, 1, &cd );

                  // see if a weapon was dropped...
                  if (bots[i].bot_weapons != cd.weapons)
                  {
                     bots[i].bot_weapons = cd.weapons;
                  }
               }
            }
         }
      }

      count = 0;

      if (bot_stop == 0)
      {
         for (bot_index = 0; bot_index < gpGlobals->maxClients; bot_index++)
         {
            if ((bots[bot_index].is_used) &&  // is this slot used AND
                (bots[bot_index].respawn_state == RESPAWN_IDLE))  // not respawning
            {
               BotThink(&bots[bot_index]);

               count++;
            }
         }
      }

      if (count > num_bots)
         num_bots = count;

      for (player_index = 1; player_index <= gpGlobals->maxClients; player_index++)
      {
         pPlayer = INDEXENT(player_index);

         if (pPlayer && !pPlayer->free)
         {
            if ((g_waypoint_on) &&
                FBitSet(pPlayer->v.flags, FL_CLIENT) &&
                !FBitSet(pPlayer->v.flags, FL_FAKECLIENT))
            {
                  WaypointThink(pPlayer);
            }
         }
      }

      // are we currently respawning bots and is it time to spawn one yet?
      if ((respawn_time > 1.0) && (respawn_time <= gpGlobals->time))
      {
         int index = 0;

         // find bot needing to be respawned...
         while ((index < 32) &&
                (bots[index].respawn_state != RESPAWN_NEED_TO_RESPAWN))
            index++;

         if (index < 32)
         {
            int strafe = bot_strafe_percent;  // save global strafe percent
            int chat = bot_chat_percent;    // save global chat percent
            int taunt = bot_taunt_percent;  // save global taunt percent
            int whine = bot_whine_percent;  // save global whine percent
            int grenade = bot_grenade_time; // save global grenade time
            int logo = bot_logo_percent;    // save global logo percent
            int tag = bot_chat_tag_percent;    // save global clan tag percent
            int drop = bot_chat_drop_percent;  // save global chat drop percent
            int swap = bot_chat_swap_percent;  // save global chat swap percent
            int lower = bot_chat_lower_percent; // save global chat lower percent
            int react = bot_reaction_time;

            bots[index].respawn_state = RESPAWN_IS_RESPAWNING;
            bots[index].is_used = FALSE;      // free up this slot

            bot_strafe_percent = bots[index].strafe_percent;
            bot_chat_percent = bots[index].chat_percent;
            bot_taunt_percent = bots[index].taunt_percent;
            bot_whine_percent = bots[index].whine_percent;
            bot_grenade_time = bots[index].grenade_time;
            bot_logo_percent = bots[index].logo_percent;
            bot_chat_tag_percent = bots[index].chat_tag_percent;
            bot_chat_drop_percent = bots[index].chat_drop_percent;
            bot_chat_swap_percent = bots[index].chat_swap_percent;
            bot_chat_lower_percent = bots[index].chat_lower_percent;
            bot_reaction_time = bots[index].reaction_time;

            // respawn 1 bot then wait a while (otherwise engine crashes)
            if ((mod_id == VALVE_DLL) ||
                ((mod_id == GEARBOX_DLL) && (pent_info_ctfdetect == NULL)) ||
                (mod_id == HOLYWARS_DLL) || (mod_id == DMC_DLL))
            {
               char c_skill[2];
               char c_topcolor[4];
               char c_bottomcolor[4];

               sprintf(c_skill, "%d", bots[index].bot_skill);
               sprintf(c_topcolor, "%d", bots[index].top_color);
               sprintf(c_bottomcolor, "%d", bots[index].bottom_color);
               
               BotCreate(NULL, bots[index].skin, bots[index].name, c_skill, c_topcolor, c_bottomcolor);
            }
            else
            {
               char c_skill[2];
               char c_team[2];
               char c_class[3];

               sprintf(c_skill, "%d", bots[index].bot_skill);
               sprintf(c_team, "%d", bots[index].bot_team);
               sprintf(c_class, "%d", bots[index].bot_class);

               if ((mod_id == TFC_DLL) || (mod_id == GEARBOX_DLL))
                  BotCreate(NULL, NULL, NULL, bots[index].name, c_skill, NULL);
               else
                  BotCreate(NULL, c_team, c_class, bots[index].name, c_skill, NULL);
            }

            bot_strafe_percent = strafe;  // restore global strafe percent
            bot_chat_percent = chat;    // restore global chat percent
            bot_taunt_percent = taunt;  // restore global taunt percent
            bot_whine_percent = whine;  // restore global whine percent
            bot_grenade_time = grenade;  // restore global grenade time
            bot_logo_percent = logo;  // restore global logo percent
            bot_chat_tag_percent = tag;    // restore global chat percent
            bot_chat_drop_percent = drop;    // restore global chat percent
            bot_chat_swap_percent = swap;    // restore global chat percent
            bot_chat_lower_percent = lower;    // restore global chat percent
            bot_reaction_time = react;

            respawn_time = gpGlobals->time + 2.0;  // set next respawn time

            bot_check_time = gpGlobals->time + 5.0;
         }
         else
         {
            respawn_time = 0.0;
         }
      }

      if (g_GameRules)
      {
         if (need_to_open_cfg)  // have we open HPB_bot.cfg file yet?
         {
            char filename[256];
            char mapname[64];

            need_to_open_cfg = FALSE;  // only do this once!!!

            // check if mapname_HPB_bot.cfg file exists...

            strcpy(mapname, STRING(gpGlobals->mapname));
            strcat(mapname, "_HPB_bot.cfg");

            UTIL_BuildFileName(filename, "maps", mapname);

            if ((bot_cfg_fp = fopen(filename, "r")) != NULL)
            {
               sprintf(msg, "Executing %s\n", filename);
               ALERT( at_console, msg );
            }
            else
            {
               UTIL_BuildFileName(filename, "HPB_bot.cfg", NULL);

               sprintf(msg, "Executing %s\n", filename);
               ALERT( at_console, msg );

               bot_cfg_fp = fopen(filename, "r");

               if (bot_cfg_fp == NULL)
                  ALERT( at_console, "HPB_bot.cfg file not found\n" );
            }

            if (IsDedicatedServer)
               bot_cfg_pause_time = gpGlobals->time + 5.0;
            else
               bot_cfg_pause_time = gpGlobals->time + 20.0;
         }

         if (!IsDedicatedServer && !spawn_time_reset)
         {
            if (listenserver_edict != NULL)
            {
               if (IsAlive(listenserver_edict))
               {
                  spawn_time_reset = TRUE;

                  if (respawn_time >= 1.0)
                     respawn_time = min(respawn_time, gpGlobals->time + (float)1.0);

                  if (bot_cfg_pause_time >= 1.0)
                     bot_cfg_pause_time = min(bot_cfg_pause_time, gpGlobals->time + (float)1.0);
               }
            }
         }

         if ((bot_cfg_fp) &&
             (bot_cfg_pause_time >= 1.0) && (bot_cfg_pause_time <= gpGlobals->time))
         {
            // process HPB_bot.cfg file options...
            ProcessBotCfgFile();
         }

      }      

      // if time to check for server commands then do so...
      if ((check_server_cmd <= gpGlobals->time) && IsDedicatedServer)
      {
         check_server_cmd = gpGlobals->time + 1.0;

         char *cvar_bot = (char *)CVAR_GET_STRING( "HPB_bot" );

         if ( cvar_bot && cvar_bot[0] )
         {
            char cmd_line[80];
            char *cmd, *arg1, *arg2, *arg3, *arg4, *arg5;

            strcpy(cmd_line, cvar_bot);

            index = 0;
            cmd = cmd_line;
            arg1 = arg2 = arg3 = arg4 = arg5 = NULL;

            // skip to blank or end of string...
            while ((cmd_line[index] != ' ') && (cmd_line[index] != 0))
               index++;

            if (cmd_line[index] == ' ')
            {
               cmd_line[index++] = 0;
               arg1 = &cmd_line[index];

               // skip to blank or end of string...
               while ((cmd_line[index] != ' ') && (cmd_line[index] != 0))
                  index++;

               if (cmd_line[index] == ' ')
               {
                  cmd_line[index++] = 0;
                  arg2 = &cmd_line[index];

                  // skip to blank or end of string...
                  while ((cmd_line[index] != ' ') && (cmd_line[index] != 0))
                     index++;

                  if (cmd_line[index] == ' ')
                  {
                     cmd_line[index++] = 0;
                     arg3 = &cmd_line[index];

                     // skip to blank or end of string...
                     while ((cmd_line[index] != ' ') && (cmd_line[index] != 0))
                        index++;

                     if (cmd_line[index] == ' ')
                     {
                        cmd_line[index++] = 0;
                        arg4 = &cmd_line[index];

                        // skip to blank or end of string...
                        while ((cmd_line[index] != ' ') && (cmd_line[index] != 0))
                           index++;

                        if (cmd_line[index] == ' ')
                        {
                           cmd_line[index++] = 0;
                           arg5 = &cmd_line[index];
                        }
                     }
                  }
               }
            }

            if (strcmp(cmd, "addbot") == 0)
            {
               BotCreate( NULL, arg1, arg2, arg3, arg4, arg5 );

               bot_check_time = gpGlobals->time + 5.0;
            }
            else if (strcmp(cmd, "min_bots") == 0)
            {
               min_bots = atoi( arg1 );

               if ((min_bots < 0) || (min_bots > 31))
                  min_bots = 1;
      
               sprintf(msg, "min_bots set to %d\n", min_bots);
               printf(msg);
            }
            else if (strcmp(cmd, "max_bots") == 0)
            {
               max_bots = atoi( arg1 );
     
               if ((max_bots < 0) || (max_bots > 31)) 
                  max_bots = 1;

               sprintf(msg, "max_bots set to %d\n", max_bots);
               printf(msg);
            }

            CVAR_SET_STRING("HPB_bot", "");
         }
      }

      // check if time to see if a bot needs to be created...
      if (bot_check_time < gpGlobals->time)
      {
         int count = 0;

         bot_check_time = gpGlobals->time + 5.0;

         for (i = 0; i < 32; i++)
         {
            if (clients[i] != NULL)
               count++;
         }

         // if there are currently less than the maximum number of "players"
         // then add another bot using the default skill level...
         if ((count < max_bots) && (max_bots != -1))
         {
            BotCreate( NULL, NULL, NULL, NULL, NULL, NULL );
         }
      }

      previous_time = gpGlobals->time;
   }

   (*other_gFunctionTable.pfnStartFrame)();
}

void ParmsNewLevel( void )
{
   (*other_gFunctionTable.pfnParmsNewLevel)();
}

void ParmsChangeLevel( void )
{
   (*other_gFunctionTable.pfnParmsChangeLevel)();
}

const char *GetGameDescription( void )
{
   return (*other_gFunctionTable.pfnGetGameDescription)();
}

void PlayerCustomization( edict_t *pEntity, customization_t *pCust )
{
   if (debug_engine) { fp=fopen("HPB_bot.txt", "a"); fprintf(fp, "PlayerCustomization: %x\n",pEntity); fclose(fp); }

   (*other_gFunctionTable.pfnPlayerCustomization)(pEntity, pCust);
}

void SpectatorConnect( edict_t *pEntity )
{
   (*other_gFunctionTable.pfnSpectatorConnect)(pEntity);
}

void SpectatorDisconnect( edict_t *pEntity )
{
   (*other_gFunctionTable.pfnSpectatorDisconnect)(pEntity);
}

void SpectatorThink( edict_t *pEntity )
{
   (*other_gFunctionTable.pfnSpectatorThink)(pEntity);
}

void Sys_Error( const char *error_string )
{
   (*other_gFunctionTable.pfnSys_Error)(error_string);
}

void PM_Move ( struct playermove_s *ppmove, int server )
{
   (*other_gFunctionTable.pfnPM_Move)(ppmove, server);
}

void PM_Init ( struct playermove_s *ppmove )
{
   (*other_gFunctionTable.pfnPM_Init)(ppmove);
}

char PM_FindTextureType( char *name )
{
   return (*other_gFunctionTable.pfnPM_FindTextureType)(name);
}

void SetupVisibility( edict_t *pViewEntity, edict_t *pClient, unsigned char **pvs, unsigned char **pas )
{
   (*other_gFunctionTable.pfnSetupVisibility)(pViewEntity, pClient, pvs, pas);
}

void UpdateClientData ( const struct edict_s *ent, int sendweapons, struct clientdata_s *cd )
{
   (*other_gFunctionTable.pfnUpdateClientData)(ent, sendweapons, cd);
}

int AddToFullPack( struct entity_state_s *state, int e, edict_t *ent, edict_t *host, int hostflags, int player, unsigned char *pSet )
{
   return (*other_gFunctionTable.pfnAddToFullPack)(state, e, ent, host, hostflags, player, pSet);
}

void CreateBaseline( int player, int eindex, struct entity_state_s *baseline, struct edict_s *entity, int playermodelindex, vec3_t player_mins, vec3_t player_maxs )
{
   (*other_gFunctionTable.pfnCreateBaseline)(player, eindex, baseline, entity, playermodelindex, player_mins, player_maxs);
}

void RegisterEncoders( void )
{
   (*other_gFunctionTable.pfnRegisterEncoders)();
}

int GetWeaponData( struct edict_s *player, struct weapon_data_s *info )
{
   return (*other_gFunctionTable.pfnGetWeaponData)(player, info);
}

void CmdStart( const edict_t *player, const struct usercmd_s *cmd, unsigned int random_seed )
{
   (*other_gFunctionTable.pfnCmdStart)(player, cmd, random_seed);
}

void CmdEnd ( const edict_t *player )
{
   (*other_gFunctionTable.pfnCmdEnd)(player);
}

int ConnectionlessPacket( const struct netadr_s *net_from, const char *args, char *response_buffer, int *response_buffer_size )
{
   return (*other_gFunctionTable.pfnConnectionlessPacket)(net_from, args, response_buffer, response_buffer_size);
}

int GetHullBounds( int hullnumber, float *mins, float *maxs )
{
   return (*other_gFunctionTable.pfnGetHullBounds)(hullnumber, mins, maxs);
}

void CreateInstancedBaselines( void )
{
   (*other_gFunctionTable.pfnCreateInstancedBaselines)();
}

int InconsistentFile( const edict_t *player, const char *filename, char *disconnect_message )
{
   if (debug_engine) { fp=fopen("HPB_bot.txt", "a"); fprintf(fp, "InconsistentFile: %x filename=%s\n",player,filename); fclose(fp); }

   return (*other_gFunctionTable.pfnInconsistentFile)(player, filename, disconnect_message);
}

int AllowLagCompensation( void )
{
   return (*other_gFunctionTable.pfnAllowLagCompensation)();
}


DLL_FUNCTIONS gFunctionTable =
{
   GameDLLInit,               //pfnGameInit
   DispatchSpawn,             //pfnSpawn
   DispatchThink,             //pfnThink
   DispatchUse,               //pfnUse
   DispatchTouch,             //pfnTouch
   DispatchBlocked,           //pfnBlocked
   DispatchKeyValue,          //pfnKeyValue
   DispatchSave,              //pfnSave
   DispatchRestore,           //pfnRestore
   DispatchObjectCollsionBox, //pfnAbsBox

   SaveWriteFields,           //pfnSaveWriteFields
   SaveReadFields,            //pfnSaveReadFields

   SaveGlobalState,           //pfnSaveGlobalState
   RestoreGlobalState,        //pfnRestoreGlobalState
   ResetGlobalState,          //pfnResetGlobalState

   ClientConnect,             //pfnClientConnect
   ClientDisconnect,          //pfnClientDisconnect
   ClientKill,                //pfnClientKill
   ClientPutInServer,         //pfnClientPutInServer
   ClientCommand,             //pfnClientCommand
   ClientUserInfoChanged,     //pfnClientUserInfoChanged
   ServerActivate,            //pfnServerActivate
   ServerDeactivate,          //pfnServerDeactivate

   PlayerPreThink,            //pfnPlayerPreThink
   PlayerPostThink,           //pfnPlayerPostThink

   StartFrame,                //pfnStartFrame
   ParmsNewLevel,             //pfnParmsNewLevel
   ParmsChangeLevel,          //pfnParmsChangeLevel

   GetGameDescription,        //pfnGetGameDescription    Returns string describing current .dll game.
   PlayerCustomization,       //pfnPlayerCustomization   Notifies .dll of new customization for player.

   SpectatorConnect,          //pfnSpectatorConnect      Called when spectator joins server
   SpectatorDisconnect,       //pfnSpectatorDisconnect   Called when spectator leaves the server
   SpectatorThink,            //pfnSpectatorThink        Called when spectator sends a command packet (usercmd_t)

   Sys_Error,                 //pfnSys_Error          Called when engine has encountered an error

   PM_Move,                   //pfnPM_Move
   PM_Init,                   //pfnPM_Init            Server version of player movement initialization
   PM_FindTextureType,        //pfnPM_FindTextureType

   SetupVisibility,           //pfnSetupVisibility        Set up PVS and PAS for networking for this client
   UpdateClientData,          //pfnUpdateClientData       Set up data sent only to specific client
   AddToFullPack,             //pfnAddToFullPack
   CreateBaseline,            //pfnCreateBaseline        Tweak entity baseline for network encoding, allows setup of player baselines, too.
   RegisterEncoders,          //pfnRegisterEncoders      Callbacks for network encoding
   GetWeaponData,             //pfnGetWeaponData
   CmdStart,                  //pfnCmdStart
   CmdEnd,                    //pfnCmdEnd
   ConnectionlessPacket,      //pfnConnectionlessPacket
   GetHullBounds,             //pfnGetHullBounds
   CreateInstancedBaselines,  //pfnCreateInstancedBaselines
   InconsistentFile,          //pfnInconsistentFile
   AllowLagCompensation,      //pfnAllowLagCompensation
};

#ifdef __BORLANDC__
int EXPORT GetEntityAPI( DLL_FUNCTIONS *pFunctionTable, int interfaceVersion )
#else
extern "C" EXPORT int GetEntityAPI( DLL_FUNCTIONS *pFunctionTable, int interfaceVersion )
#endif
{
   // check if engine's pointer is valid and version is correct...

   if ( !pFunctionTable || interfaceVersion != INTERFACE_VERSION )
      return FALSE;

   // pass engine callback function table to engine...
   memcpy( pFunctionTable, &gFunctionTable, sizeof( DLL_FUNCTIONS ) );

   // pass other DLLs engine callbacks to function table...
   if (!(*other_GetEntityAPI)(&other_gFunctionTable, INTERFACE_VERSION))
   {
      return FALSE;  // error initializing function table!!!
   }

   return TRUE;
}


#ifdef __BORLANDC__
int EXPORT GetNewDLLFunctions( NEW_DLL_FUNCTIONS *pFunctionTable, int *interfaceVersion )
#else
extern "C" EXPORT int GetNewDLLFunctions( NEW_DLL_FUNCTIONS *pFunctionTable, int *interfaceVersion )
#endif
{
   if (other_GetNewDLLFunctions == NULL)
      return FALSE;

   // pass other DLLs engine callbacks to function table...
   if (!(*other_GetNewDLLFunctions)(pFunctionTable, interfaceVersion))
   {
      return FALSE;  // error initializing function table!!!
   }

   return TRUE;
}


void FakeClientCommand(edict_t *pBot, char *arg1, char *arg2, char *arg3)
{
   int length;

   memset(g_argv, 0, 1024);

   isFakeClientCommand = 1;

   if ((arg1 == NULL) || (*arg1 == 0))
      return;

   if ((arg2 == NULL) || (*arg2 == 0))
   {
      length = sprintf(&g_argv[0], "%s", arg1);
      fake_arg_count = 1;
   }
   else if ((arg3 == NULL) || (*arg3 == 0))
   {
      length = sprintf(&g_argv[0], "%s %s", arg1, arg2);
      fake_arg_count = 2;
   }
   else
   {
      length = sprintf(&g_argv[0], "%s %s %s", arg1, arg2, arg3);
      fake_arg_count = 3;
   }

   g_argv[length] = 0;  // null terminate just in case

   strcpy(&g_argv[64], arg1);

   if (arg2)
      strcpy(&g_argv[128], arg2);

   if (arg3)
      strcpy(&g_argv[192], arg3);

   if (debug_engine) { fp=fopen("HPB_bot.txt","a"); fprintf(fp,"FakeClientCommand=%s\n",g_argv); fclose(fp); }

   // allow the MOD DLL to execute the ClientCommand...
   ClientCommand(pBot);

   isFakeClientCommand = 0;
}


const char *Cmd_Args( void )
{
   if (isFakeClientCommand)
   {
      return &g_argv[0];
   }
   else
   {
      return (*g_engfuncs.pfnCmd_Args)();
   }
}


const char *Cmd_Argv( int argc )
{
   if (isFakeClientCommand)
   {
      if (argc == 0)
      {
         return &g_argv[64];
      }
      else if (argc == 1)
      {
         return &g_argv[128];
      }
      else if (argc == 2)
      {
         return &g_argv[192];
      }
      else
      {
         return NULL;
      }
   }
   else
   {
      return (*g_engfuncs.pfnCmd_Argv)(argc);
   }
}


int Cmd_Argc( void )
{
   if (isFakeClientCommand)
   {
      return fake_arg_count;
   }
   else
   {
      return (*g_engfuncs.pfnCmd_Argc)();
   }
}


void ProcessBotCfgFile(void)
{
   int ch;
   char cmd_line[256];
   int cmd_index;
   static char server_cmd[80];
   char *cmd, *arg1, *arg2, *arg3, *arg4, *arg5;
   char msg[80];

   if (bot_cfg_pause_time > gpGlobals->time)
      return;

   if (bot_cfg_fp == NULL)
      return;

   cmd_index = 0;
   cmd_line[cmd_index] = 0;

   ch = fgetc(bot_cfg_fp);

   // skip any leading blanks
   while (ch == ' ')
      ch = fgetc(bot_cfg_fp);

   while ((ch != EOF) && (ch != '\r') && (ch != '\n'))
   {
      if (ch == '\t')  // convert tabs to spaces
         ch = ' ';

      cmd_line[cmd_index] = ch;

      ch = fgetc(bot_cfg_fp);

      // skip multiple spaces in input file
      while ((cmd_line[cmd_index] == ' ') &&
             (ch == ' '))      
         ch = fgetc(bot_cfg_fp);

      cmd_index++;
   }

   if (ch == '\r')  // is it a carriage return?
   {
      ch = fgetc(bot_cfg_fp);  // skip the linefeed
   }

   // if reached end of file, then close it
   if (ch == EOF)
   {
      fclose(bot_cfg_fp);

      bot_cfg_fp = NULL;

      bot_cfg_pause_time = 0.0;
   }

   cmd_line[cmd_index] = 0;  // terminate the command line

   // copy the command line to a server command buffer...
   strcpy(server_cmd, cmd_line);
   strcat(server_cmd, "\n");

   cmd_index = 0;
   cmd = cmd_line;
   arg1 = arg2 = arg3 = arg4 = arg5 = NULL;

   // skip to blank or end of string...
   while ((cmd_line[cmd_index] != ' ') && (cmd_line[cmd_index] != 0))
      cmd_index++;

   if (cmd_line[cmd_index] == ' ')
   {
      cmd_line[cmd_index++] = 0;
      arg1 = &cmd_line[cmd_index];

      // skip to blank or end of string...
      while ((cmd_line[cmd_index] != ' ') && (cmd_line[cmd_index] != 0))
         cmd_index++;

      if (cmd_line[cmd_index] == ' ')
      {
         cmd_line[cmd_index++] = 0;
         arg2 = &cmd_line[cmd_index];

         // skip to blank or end of string...
         while ((cmd_line[cmd_index] != ' ') && (cmd_line[cmd_index] != 0))
            cmd_index++;

         if (cmd_line[cmd_index] == ' ')
         {
            cmd_line[cmd_index++] = 0;
            arg3 = &cmd_line[cmd_index];

            // skip to blank or end of string...
            while ((cmd_line[cmd_index] != ' ') && (cmd_line[cmd_index] != 0))
               cmd_index++;

            if (cmd_line[cmd_index] == ' ')
            {
               cmd_line[cmd_index++] = 0;
               arg4 = &cmd_line[cmd_index];

               // skip to blank or end of string...
               while ((cmd_line[cmd_index] != ' ') && (cmd_line[cmd_index] != 0))
                  cmd_index++;

               if (cmd_line[cmd_index] == ' ')
               {
                  cmd_line[cmd_index++] = 0;
                  arg5 = &cmd_line[cmd_index];
               }
            }
         }
      }
   }

   if ((cmd_line[0] == '#') || (cmd_line[0] == 0))
      return;  // return if comment or blank line

   if (strcmp(cmd, "addbot") == 0)
   {
      BotCreate( NULL, arg1, arg2, arg3, arg4, arg5 );

      // have to delay here or engine gives "Tried to write to
      // uninitialized sizebuf_t" error and crashes...

      bot_cfg_pause_time = gpGlobals->time + 2.0;
      bot_check_time = gpGlobals->time + 5.0;

      return;
   }

   if (strcmp(cmd, "botskill") == 0)
   {
      int temp = atoi(arg1);

      if ((temp >= 1) && (temp <= 5))
         default_bot_skill = atoi( arg1 );  // set default bot skill level

      return;
   }

   if (strcmp(cmd, "random_color") == 0)
   {
      int temp = atoi(arg1);

      if (temp)
         b_random_color = TRUE;
      else
         b_random_color = FALSE;

      return;
   }

   if (strcmp(cmd, "bot_strafe_percent") == 0)
   {
      int temp = atoi(arg1);

      if ((temp >= 0) && (temp <= 100))
         bot_strafe_percent = atoi( arg1 );  // set bot strafe percent

      return;
   }

   if (strcmp(cmd, "bot_chat_percent") == 0)
   {
      int temp = atoi(arg1);

      if ((temp >= 0) && (temp <= 100))
         bot_chat_percent = atoi( arg1 );  // set bot chat percent

      return;
   }

   if (strcmp(cmd, "bot_taunt_percent") == 0)
   {
      int temp = atoi(arg1);

      if ((temp >= 0) && (temp <= 100))
         bot_taunt_percent = atoi( arg1 );  // set bot taunt percent

      return;
   }

   if (strcmp(cmd, "bot_whine_percent") == 0)
   {
      int temp = atoi(arg1);

      if ((temp >= 0) && (temp <= 100))
         bot_whine_percent = atoi( arg1 );  // set bot whine percent

      return;
   }

   if (strcmp(cmd, "bot_chat_tag_percent") == 0)
   {
      int temp = atoi(arg1);

      if ((temp >= 0) && (temp <= 100))
         bot_chat_tag_percent = atoi( arg1 );  // set bot chat percent

      return;
   }

   if (strcmp(cmd, "bot_chat_drop_percent") == 0)
   {
      int temp = atoi(arg1);

      if ((temp >= 0) && (temp <= 100))
         bot_chat_drop_percent = atoi( arg1 );  // set bot chat percent

      return;
   }

   if (strcmp(cmd, "bot_chat_swap_percent") == 0)
   {
      int temp = atoi(arg1);

      if ((temp >= 0) && (temp <= 100))
         bot_chat_swap_percent = atoi( arg1 );  // set bot chat percent

      return;
   }

   if (strcmp(cmd, "bot_chat_lower_percent") == 0)
   {
      int temp = atoi(arg1);

      if ((temp >= 0) && (temp <= 100))
         bot_chat_lower_percent = atoi( arg1 );  // set bot chat percent

      return;
   }

   if (strcmp(cmd, "bot_grenade_time") == 0)
   {
      int temp = atoi(arg1);

      if ((temp >= 0) && (temp <= 60))
         bot_grenade_time = atoi( arg1 );  // set bot grenade time

      return;
   }

   if (strcmp(cmd, "bot_logo_percent") == 0)
   {
      int temp = atoi(arg1);

      if ((temp >= 0) && (temp <= 100))
         bot_logo_percent = atoi( arg1 );  // set bot strafe percent

      return;
   }

   if (strcmp(cmd, "bot_reaction_time") == 0)
   {
      int temp = atoi(arg1);

      if ((temp >= 0) && (temp <= 3))
         bot_reaction_time = atoi( arg1 );  // set bot reaction time

      return;
   }

   if (strcmp(cmd, "observer") == 0)
   {
      int temp = atoi(arg1);

      if (temp)
         b_observer_mode = TRUE;
      else
         b_observer_mode = FALSE;

      return;
   }

   if (strcmp(cmd, "botdontshoot") == 0)
   {
      int temp = atoi(arg1);

      if (temp)
         b_botdontshoot = TRUE;
      else
         b_botdontshoot = FALSE;

      return;
   }

   if (strcmp(cmd, "min_bots") == 0)
   {
      min_bots = atoi( arg1 );

      if ((min_bots < 0) || (min_bots > 31))
         min_bots = 1;

      if (IsDedicatedServer)
      {
         sprintf(msg, "min_bots set to %d\n", min_bots);
         printf(msg);
      }

      return;
   }

   if (strcmp(cmd, "max_bots") == 0)
   {
      max_bots = atoi( arg1 );

      if ((max_bots < 0) || (max_bots > 31)) 
         max_bots = 1;

      if (IsDedicatedServer)
      {
         sprintf(msg, "max_bots set to %d\n", max_bots);
         printf(msg);
      }

      return;
   }

   if (strcmp(cmd, "pause") == 0)
   {
      bot_cfg_pause_time = gpGlobals->time + atoi( arg1 );

      return;
   }

   sprintf(msg, "executing server command: %s\n", server_cmd);
   ALERT( at_console, msg );

   if (IsDedicatedServer)
      printf(msg);

   SERVER_COMMAND(server_cmd);
}
