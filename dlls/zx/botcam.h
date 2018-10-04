// botman's Half-Life bot example
//
// http://planethalflife.com/botman/
//
// botcam.h
//

#ifndef BOTCAM_H
#define BOTCAM_H

class CBotCam : public CBaseEntity
{
   public:

      CBasePlayer *m_pPlayer;
      CBasePlayer *m_pBot;

      static CBotCam *Create( CBasePlayer *pPlayer, CBasePlayer *pBot );
      void Spawn( void );
      void Disconnect( void );
	
      void EXPORT IdleThink( void );
};

#endif

