//
// HPB bot - botman's High Ping Bastard bot
//
// (http://planethalflife.com/botman/)
//
// bot_client.cpp
//

#include "extdll.h"
#include "util.h"
#include "cbase.h"

#include "bot.h"
#include "bot_func.h"
#include "bot_client.h"
#include "bot_weapons.h"

// types of damage to ignore...
#define IGNORE_DAMAGE (DMG_CRUSH | DMG_BURN | DMG_FREEZE | DMG_FALL | \
                       DMG_SHOCK | DMG_DROWN | DMG_NERVEGAS | DMG_RADIATION | \
                       DMG_DROWNRECOVER | DMG_ACID | DMG_SLOWBURN | \
                       DMG_SLOWFREEZE | 0xFF000000)

extern int mod_id;
extern bot_t bots[32];
extern int num_logos;
extern edict_t *holywars_saint;
extern int halo_status;
extern int holywars_gamemode;

extern int bot_taunt_count;
extern int recent_bot_taunt[];
extern bot_chat_t bot_taunt[MAX_BOT_CHAT];

bot_weapon_t weapon_defs[MAX_WEAPONS]; // array of weapon definitions


// This message is sent when the TFC VGUI menu is displayed.
void BotClient_TFC_VGUI(void *p, int bot_index)
{
   static int state = 0;   // current state machine state

   if (state == 0)
   {
      if ((*(int *)p) == 2)  // is it a team select menu?

         bots[bot_index].start_action = MSG_TFC_TEAM_SELECT;

      else if ((*(int *)p) == 3)  // is is a class selection menu?

         bots[bot_index].start_action = MSG_TFC_CLASS_SELECT;
   }

   state++;

   if (state == 1)
      state = 0;
}


// This message is sent when the Counter-Strike VGUI menu is displayed.
void BotClient_CS_VGUI(void *p, int bot_index)
{
   static int state = 0;   // current state machine state

   if (state == 0)
   {
      if ((*(int *)p) == 2)  // is it a team select menu?

         bots[bot_index].start_action = MSG_CS_TEAM_SELECT;

      else if ((*(int *)p) == 26)  // is is a terrorist model select menu?

         bots[bot_index].start_action = MSG_CS_T_SELECT;

      else if ((*(int *)p) == 27)  // is is a counter-terrorist model select menu?

         bots[bot_index].start_action = MSG_CS_CT_SELECT;
   }

   state++;

   if (state == 5)  // ignore other fields in VGUI message
      state = 0;
}


// This message is sent when a menu is being displayed in Counter-Strike.
void BotClient_CS_ShowMenu(void *p, int bot_index)
{
   static int state = 0;   // current state machine state

   if (state < 3)
   {
      state++;  // ignore first 3 fields of message
      return;
   }

   if (strcmp((char *)p, "#Team_Select") == 0)  // team select menu?
   {
      bots[bot_index].start_action = MSG_CS_TEAM_SELECT;
   }
   else if (strcmp((char *)p, "#Terrorist_Select") == 0)  // T model select?
   {
      bots[bot_index].start_action = MSG_CS_T_SELECT;
   }
   else if (strcmp((char *)p, "#CT_Select") == 0)  // CT model select menu?
   {
      bots[bot_index].start_action = MSG_CS_CT_SELECT;
   }

   state = 0;  // reset state machine
}


// This message is sent when the OpFor VGUI menu is displayed.
void BotClient_Gearbox_VGUI(void *p, int bot_index)
{
   static int state = 0;   // current state machine state

   if (state == 0)
   {
      if ((*(int *)p) == 2)  // is it a team select menu?

         bots[bot_index].start_action = MSG_OPFOR_TEAM_SELECT;

      else if ((*(int *)p) == 3)  // is is a class selection menu?

         bots[bot_index].start_action = MSG_OPFOR_CLASS_SELECT;
   }

   state++;

   if (state == 1)
      state = 0;
}


// This message is sent when the FrontLineForce VGUI menu is displayed.
void BotClient_FLF_VGUI(void *p, int bot_index)
{
   static int state = 0;   // current state machine state

   if (p == NULL)  // handle pfnMessageEnd case
   {
      state = 0;
      return;
   }

   if (state == 0)
   {
      if ((*(int *)p) == 2)  // is it a team select menu?
         bots[bot_index].start_action = MSG_FLF_TEAM_SELECT;
      else if ((*(int *)p) == 3)  // is it a class selection menu?
         bots[bot_index].start_action = MSG_FLF_CLASS_SELECT;
      else if ((*(int *)p) == 70)  // is it a weapon selection menu?
         bots[bot_index].start_action = MSG_FLF_WEAPON_SELECT;
      else if ((*(int *)p) == 72)  // is it a submachine gun selection menu?
         bots[bot_index].start_action = MSG_FLF_SUBMACHINE_SELECT;
      else if ((*(int *)p) == 73)  // is it a shotgun selection menu?
         bots[bot_index].start_action = MSG_FLF_SHOTGUN_SELECT;
      else if ((*(int *)p) == 75)  // is it a rifle selection menu?
         bots[bot_index].start_action = MSG_FLF_RIFLE_SELECT;
      else if ((*(int *)p) == 76)  // is it a pistol selection menu?
         bots[bot_index].start_action = MSG_FLF_PISTOL_SELECT;
      else if ((*(int *)p) == 78)  // is it a heavyweapons selection menu?
         bots[bot_index].start_action = MSG_FLF_HEAVYWEAPONS_SELECT;
   }

   state++;
}


// This message is sent when a client joins the game.  All of the weapons
// are sent with the weapon ID and information about what ammo is used.
void BotClient_Valve_WeaponList(void *p, int bot_index)
{
   static int state = 0;   // current state machine state
   static bot_weapon_t bot_weapon;

   if (state == 0)
   {
      state++;
      strcpy(bot_weapon.szClassname, (char *)p);
   }
   else if (state == 1)
   {
      state++;
      bot_weapon.iAmmo1 = *(int *)p;  // ammo index 1
   }
   else if (state == 2)
   {
      state++;
      bot_weapon.iAmmo1Max = *(int *)p;  // max ammo1
   }
   else if (state == 3)
   {
      state++;
      bot_weapon.iAmmo2 = *(int *)p;  // ammo index 2
   }
   else if (state == 4)
   {
      state++;
      bot_weapon.iAmmo2Max = *(int *)p;  // max ammo2
   }
   else if (state == 5)
   {
      state++;
      bot_weapon.iSlot = *(int *)p;  // slot for this weapon
   }
   else if (state == 6)
   {
      state++;
      bot_weapon.iPosition = *(int *)p;  // position in slot
   }
   else if (state == 7)
   {
      state++;
      bot_weapon.iId = *(int *)p;  // weapon ID
   }
   else if (state == 8)
   {
      state = 0;

      bot_weapon.iFlags = *(int *)p;  // flags for weapon (WTF???)

      // store away this weapon with it's ammo information...
      weapon_defs[bot_weapon.iId] = bot_weapon;

      if (mod_id == DMC_DLL)
      {
         bots[bot_index].bot_weapons |= bot_weapon.iId;
      }
   }
}

void BotClient_TFC_WeaponList(void *p, int bot_index)
{
   // this is just like the Valve Weapon List message
   BotClient_Valve_WeaponList(p, bot_index);
}

void BotClient_CS_WeaponList(void *p, int bot_index)
{
   // this is just like the Valve Weapon List message
   BotClient_Valve_WeaponList(p, bot_index);
}

void BotClient_Gearbox_WeaponList(void *p, int bot_index)
{
   // this is just like the Valve Weapon List message
   BotClient_Valve_WeaponList(p, bot_index);
}

void BotClient_FLF_WeaponList(void *p, int bot_index)
{
   // this is just like the Valve Weapon List message
   BotClient_Valve_WeaponList(p, bot_index);
}

void BotClient_DMC_WeaponList(void *p, int bot_index)
{
   // this is just like the Valve Weapon List message
   BotClient_Valve_WeaponList(p, bot_index);
}

// This message is sent when a weapon is selected (either by the bot chosing
// a weapon or by the server auto assigning the bot a weapon).
void BotClient_Valve_CurrentWeapon(void *p, int bot_index)
{
   static int state = 0;   // current state machine state
   static int iState;
   static int iId;
   static int iClip;

   if (state == 0)
   {
      state++;
      iState = *(int *)p;  // state of the current weapon
   }
   else if (state == 1)
   {
      state++;
      iId = *(int *)p;  // weapon ID of current weapon
   }
   else if (state == 2)
   {
      state = 0;

      iClip = *(int *)p;  // ammo currently in the clip for this weapon

      if (mod_id == DMC_DLL)
      {
         if ((iState == 1) && (iId <= 128))
            bots[bot_index].bot_weapons |= iId;
      }
      else
      {
         if (iId <= 31)
         {
            bots[bot_index].bot_weapons |= (1<<iId);  // set this weapon bit

            if (iState == 1)
            {
               bots[bot_index].current_weapon.iId = iId;
               bots[bot_index].current_weapon.iClip = iClip;

               // update the ammo counts for this weapon...
               bots[bot_index].current_weapon.iAmmo1 =
                  bots[bot_index].m_rgAmmo[weapon_defs[iId].iAmmo1];
               bots[bot_index].current_weapon.iAmmo2 =
                  bots[bot_index].m_rgAmmo[weapon_defs[iId].iAmmo2];
            }
         }
      }
   }
}

void BotClient_TFC_CurrentWeapon(void *p, int bot_index)
{
   // this is just like the Valve Current Weapon message
   BotClient_Valve_CurrentWeapon(p, bot_index);
}

void BotClient_CS_CurrentWeapon(void *p, int bot_index)
{
   // this is just like the Valve Current Weapon message
   BotClient_Valve_CurrentWeapon(p, bot_index);
}

void BotClient_Gearbox_CurrentWeapon(void *p, int bot_index)
{
   // this is just like the Valve Current Weapon message
   BotClient_Valve_CurrentWeapon(p, bot_index);
}

void BotClient_FLF_CurrentWeapon(void *p, int bot_index)
{
   // this is just like the Valve Current Weapon message
   BotClient_Valve_CurrentWeapon(p, bot_index);
}

void BotClient_DMC_CurrentWeapon(void *p, int bot_index)
{
   // this is just like the Valve Current Weapon message
   BotClient_Valve_CurrentWeapon(p, bot_index);
}

// This message is sent whenever ammo ammounts are adjusted (up or down).
void BotClient_Valve_AmmoX(void *p, int bot_index)
{
   static int state = 0;   // current state machine state
   static int index;
   static int ammount;
   int ammo_index;

   if (state == 0)
   {
      state++;
      index = *(int *)p;  // ammo index (for type of ammo)
   }
   else if (state == 1)
   {
      state = 0;

      ammount = *(int *)p;  // the ammount of ammo currently available

      bots[bot_index].m_rgAmmo[index] = ammount;  // store it away

      ammo_index = bots[bot_index].current_weapon.iId;

      // update the ammo counts for this weapon...
      bots[bot_index].current_weapon.iAmmo1 =
         bots[bot_index].m_rgAmmo[weapon_defs[ammo_index].iAmmo1];
      bots[bot_index].current_weapon.iAmmo2 =
         bots[bot_index].m_rgAmmo[weapon_defs[ammo_index].iAmmo2];
   }
}

void BotClient_TFC_AmmoX(void *p, int bot_index)
{
   // this is just like the Valve AmmoX message
   BotClient_Valve_AmmoX(p, bot_index);
}

void BotClient_CS_AmmoX(void *p, int bot_index)
{
   // this is just like the Valve AmmoX message
   BotClient_Valve_AmmoX(p, bot_index);
}

void BotClient_Gearbox_AmmoX(void *p, int bot_index)
{
   // this is just like the Valve AmmoX message
   BotClient_Valve_AmmoX(p, bot_index);
}

void BotClient_FLF_AmmoX(void *p, int bot_index)
{
   // this is just like the Valve AmmoX message
   BotClient_Valve_AmmoX(p, bot_index);
}

void BotClient_DMC_AmmoX(void *p, int bot_index)
{
   // this is just like the Valve AmmoX message
   BotClient_Valve_AmmoX(p, bot_index);
}

// This message is sent when the bot picks up some ammo (AmmoX messages are
// also sent so this message is probably not really necessary except it
// allows the HUD to draw pictures of ammo that have been picked up.  The
// bots don't really need pictures since they don't have any eyes anyway.
void BotClient_Valve_AmmoPickup(void *p, int bot_index)
{
   static int state = 0;   // current state machine state
   static int index;
   static int ammount;
   int ammo_index;

   if (state == 0)
   {
      state++;
      index = *(int *)p;
   }
   else if (state == 1)
   {
      state = 0;

      ammount = *(int *)p;

      bots[bot_index].m_rgAmmo[index] = ammount;

      ammo_index = bots[bot_index].current_weapon.iId;

      // update the ammo counts for this weapon...
      bots[bot_index].current_weapon.iAmmo1 =
         bots[bot_index].m_rgAmmo[weapon_defs[ammo_index].iAmmo1];
      bots[bot_index].current_weapon.iAmmo2 =
         bots[bot_index].m_rgAmmo[weapon_defs[ammo_index].iAmmo2];
   }
}

void BotClient_TFC_AmmoPickup(void *p, int bot_index)
{
   // this is just like the Valve Ammo Pickup message
   BotClient_Valve_AmmoPickup(p, bot_index);
}

void BotClient_CS_AmmoPickup(void *p, int bot_index)
{
   // this is just like the Valve Ammo Pickup message
   BotClient_Valve_AmmoPickup(p, bot_index);
}

void BotClient_Gearbox_AmmoPickup(void *p, int bot_index)
{
   // this is just like the Valve Ammo Pickup message
   BotClient_Valve_AmmoPickup(p, bot_index);
}

void BotClient_FLF_AmmoPickup(void *p, int bot_index)
{
   // this is just like the Valve Ammo Pickup message
   BotClient_Valve_AmmoPickup(p, bot_index);
}

void BotClient_DMC_AmmoPickup(void *p, int bot_index)
{
   // this is just like the Valve Ammo Pickup message
   BotClient_Valve_AmmoPickup(p, bot_index);
}

// This message is sent whenever grenade ammounts are adjusted.
void BotClient_TFC_SecAmmoVal(void *p, int bot_index)
{
   static int state = 0;   // current state machine state
   static int type;
   static int ammount;

   if (state == 0)
   {
      state++;
      type = *(int *)p;   // 0 = primary, 1 = secondary
   }
   else if (state == 1)
   {
      state = 0;

      ammount = *(int *)p;  // the ammount of ammo currently available

      if (type == 0)
         bots[bot_index].gren1 = ammount;  // store it away
      else if (type == 1)
         bots[bot_index].gren2 = ammount;  // store it away
   }
}


// This message gets sent when the bot picks up a weapon.
void BotClient_Valve_WeaponPickup(void *p, int bot_index)
{
   int index;

   index = *(int *)p;

   // set this weapon bit to indicate that we are carrying this weapon
   bots[bot_index].bot_weapons |= (1<<index);
}

void BotClient_TFC_WeaponPickup(void *p, int bot_index)
{
   // this is just like the Valve Weapon Pickup message
   BotClient_Valve_WeaponPickup(p, bot_index);
}

void BotClient_CS_WeaponPickup(void *p, int bot_index)
{
   // this is just like the Valve Weapon Pickup message
   BotClient_Valve_WeaponPickup(p, bot_index);
}

void BotClient_Gearbox_WeaponPickup(void *p, int bot_index)
{
   // this is just like the Valve Weapon Pickup message
   BotClient_Valve_WeaponPickup(p, bot_index);
}

void BotClient_FLF_WeaponPickup(void *p, int bot_index)
{
   // this is just like the Valve Weapon Pickup message
   BotClient_Valve_WeaponPickup(p, bot_index);
}

void BotClient_DMC_WeaponPickup(void *p, int bot_index)
{
   // this is just like the Valve Weapon Pickup message
   BotClient_Valve_WeaponPickup(p, bot_index);
}

// This message gets sent when the bot picks up an item (like a battery
// or a healthkit)
void BotClient_Valve_ItemPickup(void *p, int bot_index)
{
}

void BotClient_TFC_ItemPickup(void *p, int bot_index)
{
   // this is just like the Valve Item Pickup message
   BotClient_Valve_ItemPickup(p, bot_index);
}

void BotClient_CS_ItemPickup(void *p, int bot_index)
{
   // this is just like the Valve Item Pickup message
   BotClient_Valve_ItemPickup(p, bot_index);
}

void BotClient_Gearbox_ItemPickup(void *p, int bot_index)
{
   // this is just like the Valve Item Pickup message
   BotClient_Valve_ItemPickup(p, bot_index);
}

void BotClient_FLF_ItemPickup(void *p, int bot_index)
{
   // this is just like the Valve Item Pickup message
   BotClient_Valve_ItemPickup(p, bot_index);
}

void BotClient_DMC_ItemPickup(void *p, int bot_index)
{
   // this is just like the Valve Item Pickup message
   BotClient_Valve_ItemPickup(p, bot_index);
}

// This message gets sent when the bots health changes.
void BotClient_Valve_Health(void *p, int bot_index)
{
   bots[bot_index].bot_health = *(int *)p;  // health ammount
}

void BotClient_TFC_Health(void *p, int bot_index)
{
   // this is just like the Valve Health message
   BotClient_Valve_Health(p, bot_index);
}

void BotClient_CS_Health(void *p, int bot_index)
{
   // this is just like the Valve Health message
   BotClient_Valve_Health(p, bot_index);
}

void BotClient_Gearbox_Health(void *p, int bot_index)
{
   // this is just like the Valve Health message
   BotClient_Valve_Health(p, bot_index);
}

void BotClient_FLF_Health(void *p, int bot_index)
{
   // this is just like the Valve Health message
   BotClient_Valve_Health(p, bot_index);
}

void BotClient_DMC_Health(void *p, int bot_index)
{
   // this is just like the Valve Health message
   BotClient_Valve_Health(p, bot_index);
}

// This message gets sent when the bots armor changes.
void BotClient_Valve_Battery(void *p, int bot_index)
{
   bots[bot_index].bot_armor = *(int *)p;  // armor ammount
}

void BotClient_TFC_Battery(void *p, int bot_index)
{
   // this is just like the Valve Battery message
   BotClient_Valve_Battery(p, bot_index);
}

void BotClient_CS_Battery(void *p, int bot_index)
{
   // this is just like the Valve Battery message
   BotClient_Valve_Battery(p, bot_index);
}

void BotClient_Gearbox_Battery(void *p, int bot_index)
{
   // this is just like the Valve Battery message
   BotClient_Valve_Battery(p, bot_index);
}

void BotClient_FLF_Battery(void *p, int bot_index)
{
   // this is just like the Valve Battery message
   BotClient_Valve_Battery(p, bot_index);
}

void BotClient_DMC_Battery(void *p, int bot_index)
{
   // this is just like the Valve Battery message
   BotClient_Valve_Battery(p, bot_index);
}

// This message gets sent when the bots are getting damaged.
void BotClient_Valve_Damage(void *p, int bot_index)
{
   static int state = 0;   // current state machine state
   static int damage_armor;
   static int damage_taken;
   static int damage_bits;  // type of damage being done
   static Vector damage_origin;

   if (state == 0)
   {
      state++;
      damage_armor = *(int *)p;
   }
   else if (state == 1)
   {
      state++;
      damage_taken = *(int *)p;
   }
   else if (state == 2)
   {
      state++;
      damage_bits = *(int *)p;
   }
   else if (state == 3)
   {
      state++;
      damage_origin.x = *(float *)p;
   }
   else if (state == 4)
   {
      state++;
      damage_origin.y = *(float *)p;
   }
   else if (state == 5)
   {
      state = 0;

      damage_origin.z = *(float *)p;

      if ((damage_armor > 0) || (damage_taken > 0))
      {
         // ignore certain types of damage...
         if (damage_bits & IGNORE_DAMAGE)
            return;

         // if the bot doesn't have an enemy and someone is shooting at it then
         // turn in the attacker's direction...
         if (bots[bot_index].pBotEnemy == NULL)
         {
            // face the attacker...
            Vector v_enemy = damage_origin - bots[bot_index].pEdict->v.origin;
            Vector bot_angles = UTIL_VecToAngles( v_enemy );

            bots[bot_index].pEdict->v.ideal_yaw = bot_angles.y;

            BotFixIdealYaw(bots[bot_index].pEdict);
         
            // stop using health or HEV stations...
            bots[bot_index].b_use_health_station = FALSE;
            bots[bot_index].b_use_HEV_station = FALSE;
            bots[bot_index].b_use_capture = FALSE;
         }
      }
   }
}

void BotClient_TFC_Damage(void *p, int bot_index)
{
   // this is just like the Valve Damage message
   BotClient_Valve_Damage(p, bot_index);
}

void BotClient_CS_Damage(void *p, int bot_index)
{
   // this is just like the Valve Damage message
   BotClient_Valve_Damage(p, bot_index);
}

void BotClient_Gearbox_Damage(void *p, int bot_index)
{
   // this is just like the Valve Damage message
   BotClient_Valve_Damage(p, bot_index);
}

void BotClient_FLF_Damage(void *p, int bot_index)
{
   // this is just like the Valve Damage message
   BotClient_Valve_Damage(p, bot_index);
}

void BotClient_DMC_Damage(void *p, int bot_index)
{
   // this is just like the Valve Damage message
   BotClient_Valve_Damage(p, bot_index);
}

// This message gets sent when the bots money ammount changes (for CS)
void BotClient_CS_Money(void *p, int bot_index)
{
   static int state = 0;   // current state machine state

   if (state == 0)
   {
      state++;

      bots[bot_index].bot_money = *(int *)p;  // amount of money
   }
   else
   {
      state = 0;  // ingore this field
   }
}


// This message gets sent when the bots get killed
void BotClient_Valve_DeathMsg(void *p, int bot_index)
{
   static int state = 0;   // current state machine state
   static int killer_index;
   static int victim_index;
   static edict_t *killer_edict;
   static edict_t *victim_edict;
   static int index;

   char chat_text[81];
   char chat_name[64];
   char temp_name[64];
   const char *bot_name;

   if (state == 0)
   {
      state++;
      killer_index = *(int *)p;  // ENTINDEX() of killer
   }
   else if (state == 1)
   {
      state++;
      victim_index = *(int *)p;  // ENTINDEX() of victim
   }
   else if (state == 2)
   {
      state = 0;

      killer_edict = INDEXENT(killer_index);
      victim_edict = INDEXENT(victim_index);

      // get the bot index of the killer...
      index = UTIL_GetBotIndex(killer_edict);

      // is this message about a bot killing someone?
      if (index != -1)
      {
         if (killer_index != victim_index)  // didn't kill self...
         {
            if ((RANDOM_LONG(1, 100) <= bots[index].logo_percent) && (num_logos))
            {
               bots[index].b_spray_logo = TRUE;  // this bot should spray logo now
               bots[index].f_spray_logo_time = gpGlobals->time;
            }
         }

         if (victim_edict != NULL)
         {
            // are there any taunt messages and should the bot taunt?
            if ((bot_taunt_count > 0) &&
                (RANDOM_LONG(1,100) <= bots[index].taunt_percent))
            {
               int taunt_index;
               bool used;
               int i, recent_count;

               // set chat flag and time to chat...
               bots[index].b_bot_say = TRUE;
               bots[index].f_bot_say = gpGlobals->time + 5.0 + RANDOM_FLOAT(0.0, 5.0);

               recent_count = 0;

               while (recent_count < 5)
               {
                  taunt_index = RANDOM_LONG(0, bot_taunt_count-1);

                  used = FALSE;

                  for (i=0; i < 5; i++)
                  {
                     if (recent_bot_taunt[i] == taunt_index)
                        used = TRUE;
                  }

                  if (used)
                     recent_count++;
                  else
                     break;
               }

               for (i=4; i > 0; i--)
                  recent_bot_taunt[i] = recent_bot_taunt[i-1];

               recent_bot_taunt[0] = taunt_index;

               if (bot_taunt[taunt_index].can_modify)
                  BotChatText(bot_taunt[taunt_index].text, chat_text);
               else
                  strcpy(chat_text, bot_taunt[taunt_index].text);

               if (victim_edict->v.netname)
               {
                  strncpy(temp_name, STRING(victim_edict->v.netname), 31);
                  temp_name[31] = 0;

                  BotChatName(temp_name, chat_name);
               }
               else
                  strcpy(chat_name, "NULL");

               bot_name = STRING(bots[index].pEdict->v.netname);

               BotChatFillInName(bots[index].bot_say_msg, chat_text, chat_name, bot_name);
            }
         }
      }

      // get the bot index of the victim...
      index = UTIL_GetBotIndex(victim_edict);

      // is this message about a bot being killed?
      if (index != -1)
      {
         if ((killer_index == 0) || (killer_index == victim_index))
         {
            // bot killed by world (worldspawn) or bot killed self...
            bots[index].killer_edict = NULL;
         }
         else
         {
            // store edict of player that killed this bot...
            bots[index].killer_edict = INDEXENT(killer_index);
         }
      }
   }
}

void BotClient_TFC_DeathMsg(void *p, int bot_index)
{
   // this is just like the Valve DeathMsg message
   BotClient_Valve_DeathMsg(p, bot_index);
}

void BotClient_CS_DeathMsg(void *p, int bot_index)
{
   // this is just like the Valve DeathMsg message
   BotClient_Valve_DeathMsg(p, bot_index);
}

void BotClient_Gearbox_DeathMsg(void *p, int bot_index)
{
   // this is just like the Valve DeathMsg message
   BotClient_Valve_DeathMsg(p, bot_index);
}

void BotClient_FLF_DeathMsg(void *p, int bot_index)
{
   // this is just like the Valve DeathMsg message
   BotClient_Valve_DeathMsg(p, bot_index);
}

void BotClient_DMC_DeathMsg(void *p, int bot_index)
{
   // this is just like the Valve DeathMsg message
   BotClient_Valve_DeathMsg(p, bot_index);
}

// This message gets sent when a text message is displayed
void BotClient_TFC_TextMsg(void *p, int bot_index)
{
   static int state = 0;   // current state machine state
   static int msg_dest = 0;

   if (p == NULL)  // handle pfnMessageEnd case
   {
      state = 0;
      return;
   }

   if (state == 0)
   {
      state++;
      msg_dest = *(int *)p;  // HUD_PRINTCENTER, etc.
   }
   else if (state == 1)
   {
      if (strcmp((char *)p, "#Sentry_finish") == 0)  // sentry gun built
      {
         bots[bot_index].sentrygun_level = 1;
      }
      else if (strcmp((char *)p, "#Sentry_upgrade") == 0)  // sentry gun upgraded
      {
         bots[bot_index].sentrygun_level += 1;

         bots[bot_index].pBotEnemy = NULL;  // don't attack it anymore
         bots[bot_index].enemy_attack_count = 0;
      }
      else if (strcmp((char *)p, "#Sentry_destroyed") == 0)  // sentry gun destroyed
      {
         bots[bot_index].sentrygun_waypoint = -1;
         bots[bot_index].sentrygun_level = 0;
      }
      else if (strcmp((char *)p, "#Dispenser_finish") == 0)  // dispenser built
      {
         bots[bot_index].dispenser_built = 1;
      }
      else if (strcmp((char *)p, "#Dispenser_destroyed") == 0)  // dispenser destroyed
      {
         bots[bot_index].dispenser_waypoint = -1;
         bots[bot_index].dispenser_built = 0;
      }
   }
}

// This message gets sent when a text message is displayed
void BotClient_FLF_TextMsg(void *p, int bot_index)
{
   static int state = 0;   // current state machine state
   static int msg_dest = 0;

   if (p == NULL)
   {
      state = 0;
      return;
   }

   if (state == 0)
   {
      state++;
      msg_dest = *(int *)p;  // HUD_PRINTCENTER, etc.
   }
   else if (state == 1)
   {
      if (strcmp((char *)p, "You are Attacking\n") == 0)  // attacker msg
      {
         bots[bot_index].defender = 0;  // attacker
      }
      else if (strcmp((char *)p, "You are Defending\n") == 0)  // defender msg
      {
         bots[bot_index].defender = 1;  // defender
      }
   }
}


// This message gets sent when the WarmUpTime is enabled/disabled
void BotClient_FLF_WarmUp(void *p, int bot_index)
{
   bots[bot_index].warmup = *(int *)p;
}


// This message gets sent to ALL when the WarmUpTime is enabled/disabled
void BotClient_FLF_WarmUpAll(void *p, int bot_index)
{
   for (int i=0; i < 32; i++)
   {
      if (bots[i].is_used)  // count the number of bots in use
         bots[i].warmup = *(int *)p;
   }
}


// This message gets sent when the round is over
void BotClient_FLF_WinMessage(void *p, int bot_index)
{
   for (int i=0; i < 32; i++)
   {
      if (bots[i].is_used)  // count the number of bots in use
         bots[i].round_end = 1;
   }
}


// This message gets sent when a weapon is hidden or restored
void BotClient_FLF_HideWeapon(void *p, int bot_index)
{
   int hide;

   hide = *(int *)p;

   if ((hide == 0) && (bots[bot_index].b_use_capture))
   {
      bots[bot_index].b_use_capture = FALSE;
      bots[bot_index].f_use_capture_time = 0.0;
   }

   if ((hide) && (bots[bot_index].b_use_capture))
      bots[bot_index].f_use_capture_time = gpGlobals->time + 30;
}


void BotClient_Valve_ScreenFade(void *p, int bot_index)
{
   static int state = 0;   // current state machine state
   static int duration;
   static int hold_time;
   static int fade_flags;
   int length;

   if (state == 0)
   {
      state++;
      duration = *(int *)p;
   }
   else if (state == 1)
   {
      state++;
      hold_time = *(int *)p;
   }
   else if (state == 2)
   {
      state++;
      fade_flags = *(int *)p;
   }
   else if (state == 6)
   {
      state = 0;

      length = (duration + hold_time) / 4096;
      bots[bot_index].blinded_time = gpGlobals->time + length - 2.0;
   }
   else
   {
      state++;
   }
}

void BotClient_TFC_ScreenFade(void *p, int bot_index)
{
   // this is just like the Valve ScreenFade message
   BotClient_Valve_ScreenFade(p, bot_index);
}

void BotClient_CS_ScreenFade(void *p, int bot_index)
{
   // this is just like the Valve ScreenFade message
   BotClient_Valve_ScreenFade(p, bot_index);
}

void BotClient_Gearbox_ScreenFade(void *p, int bot_index)
{
   // this is just like the Valve ScreenFade message
   BotClient_Valve_ScreenFade(p, bot_index);
}

void BotClient_FLF_ScreenFade(void *p, int bot_index)
{
   // this is just like the Valve ScreenFade message
   BotClient_Valve_ScreenFade(p, bot_index);
}


void BotClient_HolyWars_Halo(void *p, int edict)
{
   int type = *(int *)p;

   if (type == 0)  // wait for halo to respawn
   {
      holywars_saint = NULL;
      halo_status = HW_WAIT_SPAWN;
   }
   else if (type == 1)  // there's no saint
   {
      holywars_saint = NULL;
   }
   else if (type == 2)  // there's a new saint
   {
      halo_status = HW_NEW_SAINT;
   }
   else if (type == 3)  // you are the new saint
   {
      holywars_saint = (edict_t *)edict;
      halo_status = HW_NEW_SAINT;
   }
}

void BotClient_HolyWars_GameMode(void *p, int bot_index)
{
   int mode = *(int *)p;

   holywars_gamemode = mode;
}

void BotClient_HolyWars_HudText(void *p, int bot_index)
{
   if (strncmp((char *)p, "Voting for", 10) == 0)
   {
      bots[bot_index].vote_in_progress = TRUE;
      bots[bot_index].f_vote_time = gpGlobals->time + RANDOM_LONG(2.0, 5.0);
   }
}
