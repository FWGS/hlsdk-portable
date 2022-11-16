
//
// custom_message.cpp
//
// implementation of CHudHalloween class
//
// Coded By: Pongles

#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"
#include <string.h>
#include <stdio.h>

#define MSG_RESPAWN      1
#define MSG_GAME         2
#define MSG_FIRSTBLOOD   3
#define MSG_NAME         4
#define MSG_VICTIM       5
#define MSG_KILLER       6
#define MSG_HALLOWEEN       7

const char *billymc [] = {
	"Victim #1",
	"Victim #2. Although he was still alive when he\nwas found starving in an\nabandoned apartment, he\ndied from blood loss on\nthe way to the hospital.\nThe place has apparently\nno tenant or owner",
	"The Three is The Terrible Treat for Those who Think",
	"YOU ENJOYTHIS DO YOU NOT?",
	"Does anyone suffer BEFORE?",
    "FEED ME A STRAY CAT",
    "EVERYONE IS GIYGAS, DONT BELIEVE THEIR LIES. YOU ARE RIGHT.",
    "Right?!!/",
    "I THouGHT Y/OU, LIKE,D, HALLOWEEN, ?!",
    "The pain reflects onto you. We all project pain onto others. To make them understand.",
    "GET BACK TO WHERE YOU BELONG? YOU HAVE MADE SUFFERING ENOUGH!",
    "THIS IS EVIL, STOP THIS THIS IS EVIL.",
    "HE HE HE HE HE :)",
    "AOBTD, I feel great about this! Is it not so?",
    "T0ggTVkgR09E\\\\STOP",
    "TONIGHT’S THE NIGHT; WHY HERE?",
    "17 Mothers without sons, Are yoU haPpy?",
    "I like scissors! 61!",
    "[Picture of Mario without eyes appears] MA MA  MI YA",
    "The Black Wind Howls!",
    "why iS There yOu ProteCting what yOU Never ThINk is right aGain??",
    "WHY DO YOU NOT LEAVE AND LET LIVE?",
    "Game Over!",
    "614122876039093081593306193923589",
    "Do you realise??",
    ":)",
    ":(",
    "[Playername] was banned by the admin?",
    "RG8geW91IHJlYWxseSBlbmpveSB0aGlzIHNlbnNlbGVzcyBraWxsaW5nPyBUaGVzZSBwbGF5ZXJzIGFyZSByZWFsOyB0aGUgYm90cyBhcmUgYWxzbyByZWFsIHRvZGF5LiBUaGV5IGxpdmUgYW5kIGRpZSB3aXRoaW4gdGhlIGdhbWUgYW5kIGdldCBxdWlja2x5IHJlcGxhY2VkIHdoZW4geW91IGtpbGwgdGhlbS4gT25seSBvbiB0b2RheS4gVGhpcyBpcyBhbGwgdHJ1ZS4gVGhhdCBib3QgaXMgcmVhbC4gSSBob3BlIHlvdSBlbmpveSB5b3Vyc2VsZiwgS2lsbGVyLiAyIDMgMSA0IElTIFRIRSBXQVk=",
	"K!" };

CCustomMessage::CCustomMessage(byte rr, byte gg, byte bb, float yy, float fo, float ht,
      float st, char *szt)
{
   r = rr;
   g = gg;
   b = bb;
   y = yy;
   fadeout = fo;
   holdtime = ht;
   time = st;
   strcpy(szText, szt);
}


DECLARE_MESSAGE(m_Halloween, Halloween);

int CHudHalloween::Init(void)
{
   HOOK_MESSAGE(Halloween);

   gHUD.AddHudElem(this);

   Reset();

   return 1;
}

int CHudHalloween::VidInit(void)
{
   return 1;
}

void CHudHalloween::Reset( void )
{
   for(int i = 0; i < maxCustomMessages; i++)
   {
      if (m_pCustomMsgs[i])
         delete m_pCustomMsgs[i];
      m_pCustomMsgs[i] = NULL;
   }
}

CHudHalloween::~CHudHalloween( )
{
   for(int i = 0; i < maxCustomMessages; i++)
   {
      if(m_pCustomMsgs[i])
      {
         delete m_pCustomMsgs[i];
      }
   }
}

int CHudHalloween::Draw(float flTime)
{
   int Index;
   
   bool BeingDrawn = false;

   float factor, endTime, holdTime;

   CCustomMessage *pMessage;

   // loop though 0 - 16
   for ( Index = 0; Index < maxCustomMessages; Index++ )
   {
      // is there one here?
      if ( m_pCustomMsgs[Index] )
      {
         pMessage = m_pCustomMsgs[Index];

         endTime = pMessage->time + pMessage->fadeout
            + pMessage->holdtime;
         holdTime = pMessage->time + pMessage->holdtime;

         BeingDrawn = true;
         
         if ( flTime <= holdTime )
         {
            // hold
            factor = 1;
         }
         else
         {
            // fade out
            factor = 1 - ((flTime - holdTime) / pMessage->fadeout);
         }
           
         gHUD.DrawHudString( (ScreenWidth - CenterPos(pMessage->szText)) / 2,
            pMessage->y, ScreenWidth, pMessage->szText, factor * (pMessage->r),
            factor * (pMessage->g), factor * (pMessage->b) );
           
         // finished ?
         if(flTime >= endTime)
         {
            m_pCustomMsgs[Index] = NULL;
            delete pMessage;
         }
      }
   }

   if ( !BeingDrawn )
      m_iFlags &= ~HUD_ACTIVE;

   return 1;
}

int CHudHalloween::MsgFunc_Halloween(const char*pszName, int iSize, void *pbuf)
{
   BEGIN_READ(pbuf,iSize);

   int x = READ_BYTE();

   // reads string sent from server
   char *szText = READ_STRING();

   MessageAdd( x, gHUD.m_flTime, szText );

   m_iFlags |= HUD_ACTIVE;

   return 1;
}

int CHudHalloween::CenterPos( char *szMessage )
{
   int width = 0;

   for (; *szMessage != 0 && *szMessage != '\n'; szMessage++ )
   {
      width += gHUD.m_scrinfo.charWidths[ *szMessage ];
   }

   return width;
}

void CHudHalloween::MessageAdd( int type, float time, char *text )
{
   // check if there is an instance already

   char tempBuffer[512];
   
   if(m_pCustomMsgs[type] != NULL)
   {
      delete m_pCustomMsgs[type];
   }

   // add new instance

   switch ( type )
   {
   case MSG_RESPAWN:
      m_pCustomMsgs[type] = new CCustomMessage(0, 255, 0, (ScreenHeight / 2)
            + (ScreenHeight / 4),1.5, 1, time, "Press [Fire] To Respawn");
      break;
   case MSG_GAME:
      m_pCustomMsgs[type] = new CCustomMessage(192, 192, 192, ScreenHeight / 2,
         1.5, 5, time, text);
      break;
   case MSG_FIRSTBLOOD:
      sprintf(tempBuffer, "%s Drew First Blood", text);
      m_pCustomMsgs[type] = new CCustomMessage(255, 0, 0, ScreenHeight / 4.5,
         1.5, 5, time, tempBuffer);
      break;
   case MSG_NAME:
      sprintf(tempBuffer, "Name: %s", text);
      m_pCustomMsgs[type] = new CCustomMessage(0, 255, 0, ScreenHeight / 2
            + (ScreenHeight / 4),1, 1.5, time, tempBuffer);
      break;
   case MSG_VICTIM:
      sprintf(tempBuffer, "You were killed by %s", text);
      m_pCustomMsgs[type] = new CCustomMessage(0, 0, 255, ScreenHeight / 4,
            1.5, 1, time, tempBuffer);
      break;
   case MSG_KILLER:
      sprintf(tempBuffer, "You Killed %s", text);
      m_pCustomMsgs[type] = new CCustomMessage(255, 0, 0, ScreenHeight / 4.5,
            1.5, 1, time, tempBuffer);
      break;
   case MSG_HALLOWEEN:
	  int youbet = atoi(text);
      m_pCustomMsgs[type] = new CCustomMessage(200, 200, 200, ScreenHeight / 2
            + (ScreenHeight / 4.5), 1.5, 5, time, billymc[youbet]);
      break;
   }
}
