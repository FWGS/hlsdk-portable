//++ BulliT

#if !defined(_AG_LOCATION_HUD_)
#define _AG_LOCATION_HUD_

#ifdef AG_NO_CLIENT_DLL

#include "aglocation.h"

class AgLocationCache
{
  typedef list<AgLocation*>  AgLocationList;
  AgLocationList m_lstLocations;
  bool NearestLocation(const Vector& vPosition, AgLocation*& pLocation, float& fNearest);

public:
  AgLocationCache();
  virtual ~AgLocationCache();

  void Load();

  AgString Location(const Vector& vPosition);
};

#endif //AG_NO_CLIENT_DLL

#endif //_AG_LOCATION_HUD_

//-- Martin Webrant
