//++ BulliT

#if !defined(_AG_HUD_SCOREBOARD_)
#define _AG_HUD_SCOREBOARD_


class AgHudScoreboard: public CHudBase
{
public:
	int Init( void );
	int VidInit( void );
	int Draw(float flTime);
	void Reset(void);

	bool	IsVisible();
  void ShowScoreboard(bool bShow = true);
private:
	typedef struct
	{
    
		HSPRITE spr;
		wrect_t rc;
	} icon_flagstatus_t;
	icon_flagstatus_t m_IconFlagScore;

	bool m_bShowScoreboard;
	int DrawPlayers( int xoffset, float listslot, int nameoffset = 0, char *team = NULL ); // returns the ypos where it finishes drawing
};

#endif //_AG_HUD_SCOREBOARD_

//-- Martin Webrant
