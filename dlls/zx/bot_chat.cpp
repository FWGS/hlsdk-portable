//
// HPB bot - botman's High Ping Bastard bot
//
// (http://planethalflife.com/botman/)
//
// bot_combat.cpp
//

#include "extdll.h"
#include "util.h"
#include "cbase.h"

#include "bot.h"


#define NUM_TAGS 22

char *tag1[NUM_TAGS]={
"-=","-[","-]","-}","-{","<[","<]","[-","]-","{-","}-","[[","[","{","]","}","<",">","-","|","=","+"};
char *tag2[NUM_TAGS]={
"=-","]-","[-","{-","}-","]>","[>","-]","-[","-}","-{","]]","]","}","[","{",">","<","-","|","=","+"};

int bot_chat_count;
int bot_taunt_count;
int bot_whine_count;

bot_chat_t bot_chat[MAX_BOT_CHAT];
bot_chat_t bot_taunt[MAX_BOT_CHAT];
bot_chat_t bot_whine[MAX_BOT_CHAT];

int recent_bot_chat[5];
int recent_bot_taunt[5];
int recent_bot_whine[5];

int player_count;
char player_names[32][33];  // 32 players max, 32 chars + null

extern int bot_chat_tag_percent;
extern int bot_chat_drop_percent;
extern int bot_chat_swap_percent;
extern int bot_chat_lower_percent;


void LoadBotChat(void)
{
   FILE *bfp;
   char filename[256];
   char buffer[256];
   char *stat;
   int section = -1;
   int i, length;

   bot_chat_count = 0;
   bot_taunt_count = 0;
   bot_whine_count = 0;

   for (i=0; i < 5; i++)
   {
      recent_bot_chat[i] = -1;
      recent_bot_taunt[i] = -1;
      recent_bot_whine[i] = -1;
   }

   UTIL_BuildFileName(filename, "HPB_bot_chat.txt", NULL);

   bfp = fopen(filename, "r");

   while (bfp != NULL)
   {
      stat = fgets(buffer, 80, bfp);

      if (stat == NULL)
      {
         fclose(bfp);
         bfp = NULL;
         continue;
      }

      buffer[80] = 0;  // truncate lines longer than 80 characters

      length = strlen(buffer);

      if (buffer[length-1] == '\n')
      {
         buffer[length-1] = 0;  // remove '\n'
         length--;
      }

      if (strcmp(buffer, "[bot_chat]") == 0)
      {
         section = 0;
         continue;
      }

      if (strcmp(buffer, "[bot_taunt]") == 0)
      {
         section = 1;
         continue;
      }

      if (strcmp(buffer, "[bot_whine]") == 0)
      {
         section = 2;
         continue;
      }

      if ((length > 0) && (section == 0) &&  // bot chat
          (bot_chat_count < MAX_BOT_CHAT))
      {
         if (buffer[0] == '!')
         {
            strcpy(bot_chat[bot_chat_count].text, &buffer[1]);
            bot_chat[bot_chat_count].can_modify = FALSE;
         }
         else
         {
            strcpy(bot_chat[bot_chat_count].text, buffer);
            bot_chat[bot_chat_count].can_modify = TRUE;
         }

         bot_chat_count++;
      }

      if ((length > 0) && (section == 1) &&  // bot taunt
          (bot_taunt_count < MAX_BOT_CHAT))
      {
         if (buffer[0] == '!')
         {
            strcpy(bot_taunt[bot_taunt_count].text, &buffer[1]);
            bot_taunt[bot_taunt_count].can_modify = FALSE;
         }
         else
         {
            strcpy(bot_taunt[bot_taunt_count].text, buffer);
            bot_taunt[bot_taunt_count].can_modify = TRUE;
         }

         bot_taunt_count++;
      }

      if ((length > 0) && (section == 2) &&  // bot whine
          (bot_whine_count < MAX_BOT_CHAT))
      {
         if (buffer[0] == '!')
         {
            strcpy(bot_whine[bot_whine_count].text, &buffer[1]);
            bot_whine[bot_whine_count].can_modify = FALSE;
         }
         else
         {
            strcpy(bot_whine[bot_whine_count].text, buffer);
            bot_whine[bot_whine_count].can_modify = TRUE;
         }

         bot_whine_count++;
      }
   }
}


void BotTrimBlanks(char *in_string, char *out_string)
{
   int i, pos;
   char *dest;

   pos=0;
   while ((pos < 80) && (in_string[pos] == ' '))  // skip leading blanks
      pos++;

   dest=&out_string[0];

   while ((pos < 80) && (in_string[pos]))
   {
      *dest++ = in_string[pos];
      pos++;
   }
   *dest = 0;  // store the null

   i = strlen(out_string) - 1;
   while ((i > 0) && (out_string[i] == ' '))  // remove trailing blanks
   {
      out_string[i] = 0;
      i--;
   }
}


int BotChatTrimTag(char *original_name, char *out_name)
{
   int i;
   char *pos1, *pos2, *src, *dest;
   char in_name[80];
   int result = 0;

   strncpy(in_name, original_name, 31);
   in_name[32] = 0;

   for (i=0; i < NUM_TAGS; i++)
   {
      pos1=strstr(in_name, tag1[i]);
      if (pos1)
         pos2=strstr(pos1+strlen(tag1[i]), tag2[i]);
      else
         pos2 = NULL;

      if (pos1 && pos2 && pos1 < pos2)
      {
         src = pos2+strlen(tag2[i]);
         dest = pos1;
         while (*src)
            *dest++ = *src++;
         *dest = *src;  // copy the null;

         result = 1;
      }
   }

   strcpy(out_name, in_name);

   BotTrimBlanks(out_name, in_name);

   if (strlen(in_name) == 0)  // is name just a tag?
   {
      strncpy(in_name, original_name, 31);
      in_name[32] = 0;

      // strip just the tag part...
      for (i=0; i < NUM_TAGS; i++)
      {
         pos1=strstr(in_name, tag1[i]);
         if (pos1)
            pos2=strstr(pos1+strlen(tag1[i]), tag2[i]);
         else
            pos2 = NULL;

         if (pos1 && pos2 && pos1 < pos2)
         {
            src = pos1 + strlen(tag1[i]);
            dest = pos1;
            while (*src)
               *dest++ = *src++;
            *dest = *src;  // copy the null;

            src = pos2 - strlen(tag2[i]);
            *src = 0; // null out the rest of the string
         }
      }
   }

   BotTrimBlanks(in_name, out_name);

   out_name[31] = 0;

   return (result);
}


void BotDropCharacter(char *in_string, char *out_string)
{
   int len, pos;
   int count = 0;
   char *src, *dest;
   bool is_bad;

   strcpy(out_string, in_string);

   len = strlen(out_string);
   pos = RANDOM_LONG(1, len-1);  // don't drop position zero

   is_bad = !isalpha(out_string[pos]) || (out_string[pos-1] == '%');

   while ((is_bad) && (count < 20))
   {
      pos = RANDOM_LONG(1, len-1);
      is_bad = !isalpha(out_string[pos]) || (out_string[pos-1] == '%');
      count++;
   }

   if (count < 20)
   {
      src = &out_string[pos+1];
      dest = &out_string[pos];
      while (*src)
         *dest++ = *src++;
      *dest = *src;  // copy the null;
   }
}


void BotSwapCharacter(char *in_string, char *out_string)
{
   int len, pos;
   int count = 0;
   char temp;
   bool is_bad;

   strcpy(out_string, in_string);

   len = strlen(out_string);
   pos = RANDOM_LONG(1, len-2);  // don't swap position zero

   is_bad = !isalpha(out_string[pos]) || !isalpha(out_string[pos+1]) ||
            (out_string[pos-1] == '%');

   while ((is_bad) && (count < 20))
   {
      pos = RANDOM_LONG(1, len-2);
      is_bad = !isalpha(out_string[pos]) || !isalpha(out_string[pos+1]) ||
               (out_string[pos-1] == '%');
      count++;
   }

   if (count < 20)
   {
      temp = out_string[pos];
      out_string[pos] = out_string[pos+1];
      out_string[pos+1] = temp;
   }
}


void BotChatName(char *original_name, char *out_name)
{
   int pos;

   if (RANDOM_LONG(1, 100) <= bot_chat_tag_percent)
   {
      char temp_name[80];

      strncpy(temp_name, original_name, 31);
      temp_name[31] = 0;

      while (BotChatTrimTag(temp_name, out_name))
      {
         strcpy(temp_name, out_name);
      }
   }
   else
   {
      strncpy(out_name, original_name, 31);
      out_name[31] = 0;
   }

   if (RANDOM_LONG(1, 100) <= bot_chat_lower_percent)
   {
      pos=0;
      while ((pos < 80) && (out_name[pos]))
      {
         out_name[pos] = tolower(out_name[pos]);
         pos++;
      }
   }
}


void BotChatText(char *in_text, char *out_text)
{
   int pos;
   char temp_text[81];
   int count;

   strncpy(temp_text, in_text, 79);
   temp_text[80] = 0;

   if (RANDOM_LONG(1, 100) <= bot_chat_drop_percent)
   {
      count = RANDOM_LONG(1, 3);

      while (count)
      {
         BotDropCharacter(temp_text, out_text);
         strcpy(temp_text, out_text);
         count--;
      }
   }

   if (RANDOM_LONG(1, 100) <= bot_chat_swap_percent)
   {
      count = RANDOM_LONG(1, 2);

      while (count)
      {
         BotSwapCharacter(temp_text, out_text);
         strcpy(temp_text, out_text);
         count--;
      }
   }

   if (RANDOM_LONG(1, 100) <= bot_chat_lower_percent)
   {
      pos=0;
      while (temp_text[pos])
      {
         temp_text[pos] = tolower(temp_text[pos]);
         pos++;
      }
   }

   strcpy(out_text, temp_text);
}


void BotChatGetPlayers(void)
{
   int index;
   const char *pName;

   player_count = 0;

   for (index = 1; index <= gpGlobals->maxClients; index++)
   {
      edict_t *pPlayer = INDEXENT(index);

      // skip invalid players
      if ((pPlayer) && (!pPlayer->free))
      {
         if (pPlayer->v.netname)
         {
            pName = STRING(pPlayer->v.netname);

            if (*pName != 0)
            {
               strncpy(player_names[player_count], pName, 32);

               player_count++;
            }
         }
      }
   }
}


void BotChatFillInName(char *bot_say_msg, char *chat_text,
                       char *chat_name, const char *bot_name)
{
   int chat_index, say_index;
   char *name_pos, *rand_pos;
   char random_name[64];
   int index, name_offset, rand_offset;
   bool is_bad;

   chat_index = 0;
   say_index = 0;
   bot_say_msg[0] = 0;

   name_pos = strstr(&chat_text[chat_index], "%n");
   rand_pos = strstr(&chat_text[chat_index], "%r");

   while ((name_pos != NULL) || (rand_pos != NULL))
   {
      if (name_pos != NULL)
         name_offset = name_pos - chat_text;
      if (rand_pos != NULL)
         rand_offset = rand_pos - chat_text;

      if ((rand_pos == NULL) ||
          ((name_offset < rand_offset) && (name_pos != NULL)))
      {
         while (&chat_text[chat_index] < name_pos)
            bot_say_msg[say_index++] = chat_text[chat_index++];

         bot_say_msg[say_index] = 0;  // add null terminator

         chat_index += 2;  // skip the "%n"

         strcat(bot_say_msg, chat_name);
         say_index += strlen(chat_name);

         bot_say_msg[say_index] = 0;
      }
      else  // use random player name...
      {
         int count = 0;

         BotChatGetPlayers();

         // pick a name at random from the list of players...

         index = RANDOM_LONG(0, player_count-1);

         is_bad = (strcmp(player_names[index], chat_name) == 0) ||
                  (strcmp(player_names[index], bot_name) == 0);

         while ((is_bad) && (count < 20))
         {
            index = RANDOM_LONG(0, player_count-1);

            is_bad = (strcmp(player_names[index], chat_name) == 0) ||
                     (strcmp(player_names[index], bot_name) == 0);

            count++;
         }

         BotChatName(player_names[index], random_name);

         while (&chat_text[chat_index] < rand_pos)
            bot_say_msg[say_index++] = chat_text[chat_index++];

         bot_say_msg[say_index] = 0;  // add null terminator

         chat_index += 2;  // skip the "%r"

         strcat(bot_say_msg, random_name);
         say_index += strlen(random_name);

         bot_say_msg[say_index] = 0;
      }

      name_pos = strstr(&chat_text[chat_index], "%n");
      rand_pos = strstr(&chat_text[chat_index], "%r");
   }

   // copy the rest of the chat_text into the bot_say_msg...

   while (chat_text[chat_index])
      bot_say_msg[say_index++] = chat_text[chat_index++];

   bot_say_msg[say_index] = 0;  // add null terminator
}

