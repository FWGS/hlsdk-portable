// Pongles [
class CCustomMessage
{
public:
   byte   r, g, b;
   float   y;
   float   fadein;
   float   fadeout;
   float   holdtime;
   char   *pText;
   float   time;
   char   szText[64];

   CCustomMessage() {}
   
   CCustomMessage(byte rr, byte gg, byte bb, float yy, float fo, float ht,
      float st, char *szt);
};

const int maxCustomMessages = 16;

class CHudHalloween : public CHudBase
{
   CCustomMessage   *m_pCustomMsgs[maxCustomMessages];

public:
   int   Init(void);
   int   VidInit( void );
   int   Draw(float flTime);
   int   MsgFunc_Halloween(const char *pszName, int iSize, void *pbuf);
   int   CenterPos(char *szMessage);
   void MessageAdd( int type, float time, char *text );
   void Reset( void );

   ~CHudHalloween();
};
// Pongles ]