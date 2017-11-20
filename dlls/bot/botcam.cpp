// botman's Half-Life bot example
//
// http://planethalflife.com/botman/
//
// botcam.cpp
//

#include "extdll.h"
#include "util.h"
#include "client.h"
#include "cbase.h"
#include "player.h"

#include "botcam.h"


LINK_ENTITY_TO_CLASS(entity_botcam, CBotCam);


CBotCam *CBotCam::Create( CBasePlayer *pPlayer, CBasePlayer *pBot )
{
   CBotCam *pBotCam = GetClassPtr( (CBotCam *)NULL );

   PRECACHE_MODEL( "models/mechgibs.mdl" );

   pBotCam->m_pPlayer = pPlayer;
   pBotCam->m_pBot = pBot;

   pPlayer->pev->effects |= EF_NODRAW;
   pPlayer->pev->solid = SOLID_NOT;
   pPlayer->pev->takedamage = DAMAGE_NO;
   pPlayer->m_iHideHUD |= HIDEHUD_ALL;

   pBotCam->Spawn();

   return pBotCam;
}
   

void CBotCam::Spawn( void )
{
   pev->classname = MAKE_STRING("entity_botcam");

   UTIL_MakeVectors(m_pBot->pev->v_angle);

   TraceResult tr;
   UTIL_TraceLine(m_pBot->pev->origin + m_pBot->pev->view_ofs,
                  m_pBot->pev->origin + m_pBot->pev->view_ofs + gpGlobals->v_forward * -16 + gpGlobals->v_up * 10,
                  dont_ignore_monsters, m_pBot->edict(), &tr );

   UTIL_SetOrigin(pev, tr.vecEndPos);

   pev->angles = m_pBot->pev->v_angle;

   pev->fixangle = TRUE;

   SET_VIEW (m_pPlayer->edict(), edict());

   // mechgibs seems to be an "invisible" model.  Other players won't see
   // anything when this model is used as the botcam...

   SET_MODEL(ENT(pev), "models/mechgibs.mdl");

   pev->movetype = MOVETYPE_FLY;
   pev->solid = SOLID_NOT;
   pev->takedamage = DAMAGE_NO;

   pev->renderamt = 0;

   m_pPlayer->EnableControl(FALSE);

   SetTouch( NULL );

   SetThink( &CBotCam::IdleThink );

   pev->nextthink = gpGlobals->time + 0.1;
}


void CBotCam::IdleThink( void )
{
   // make sure bot is still in the game...

   if (m_pBot->pev->takedamage != DAMAGE_NO)  // not "kicked"
   {
      UTIL_MakeVectors(m_pBot->pev->v_angle);

      TraceResult tr;
      UTIL_TraceLine(m_pBot->pev->origin + m_pBot->pev->view_ofs,
                     m_pBot->pev->origin + m_pBot->pev->view_ofs + gpGlobals->v_forward * -16 + gpGlobals->v_up * 10,
                     dont_ignore_monsters, m_pBot->edict(), &tr );

      UTIL_SetOrigin(pev, tr.vecEndPos);

      pev->angles = m_pBot->pev->v_angle;

      pev->fixangle = TRUE;

      SET_VIEW (m_pPlayer->edict(), edict());

      pev->nextthink = gpGlobals->time;
   }
   else
   {
      SET_VIEW (m_pPlayer->edict(), m_pPlayer->edict());

      m_pPlayer->pev->effects &= ~EF_NODRAW;
      m_pPlayer->pev->solid = SOLID_SLIDEBOX;
      m_pPlayer->pev->takedamage = DAMAGE_AIM;
      m_pPlayer->m_iHideHUD &= ~HIDEHUD_ALL;

      m_pPlayer->EnableControl(TRUE);

      m_pPlayer->pBotCam = NULL;  // player's botcam is no longer valid

      m_pBot = NULL;
      m_pPlayer = NULL;

      REMOVE_ENTITY( ENT(pev) );
   }
}


void CBotCam::Disconnect( void )
{
   SET_VIEW (m_pPlayer->edict(), m_pPlayer->edict());

   m_pPlayer->pev->effects &= ~EF_NODRAW;
   m_pPlayer->pev->solid = SOLID_SLIDEBOX;
   m_pPlayer->pev->takedamage = DAMAGE_AIM;
   m_pPlayer->m_iHideHUD &= ~HIDEHUD_ALL;

   m_pPlayer->EnableControl(TRUE);

   m_pPlayer->pBotCam = NULL;  // player's botcam is no longer valid

   m_pBot = NULL;
   m_pPlayer = NULL;

   REMOVE_ENTITY( ENT(pev) );
}

