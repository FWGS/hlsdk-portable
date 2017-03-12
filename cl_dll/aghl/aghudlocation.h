//++ BulliT

#if !defined(_AG_LOCATION_HUD_)
#define _AG_LOCATION_HUD_
#include "aglocation.h"

class AgHudLocation: public CHudBase
{
public:
  virtual ~AgHudLocation();
  
	int Init( void );
	int VidInit( void );
	int Draw(float flTime);
  void Reset(void);

private:
  typedef list<AgLocation*>  AgLocationList;
  AgLocationList m_lstLocations;
  float m_fAt;
  float m_fNear;

  bool NearestLocation(const Vector& vPosition, AgLocation*& pLocation, float& fNearest);
  

  void InitDistances();
  void Load();
  void Save();

  AgString Location(const Vector& vPosition);
  
public:
  char m_szMap[32];

  void ParseAndEditSayString(char* pszSay, int iPlayer);

  void UserCmd_OpenLocation();
  void UserCmd_CloseLocation();

  void UserCmd_AddLocation();
  void UserCmd_DeleteLocation();
  void UserCmd_ShowLocations();

  int MsgFunc_InitLoc(const char *pszName, int iSize, void *pbuf);
  int MsgFunc_Location(const char *pszName, int iSize, void *pbuf);

};

#endif //_AG_LOCATION_HUD_

//-- Martin Webrant
