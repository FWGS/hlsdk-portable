//++ BulliT

#if !defined(_AG_LOCATION_)
#define _AG_LOCATION_

#include "agglobal.h"

#ifdef AG_NO_CLIENT_DLL

class AgLocation  
{
public:
	AgLocation();
	virtual ~AgLocation();

	AgString  m_sLocation;
	Vector	  m_vPosition;
};

#endif

#endif //_AG_LOCATION_

//-- Martin Webrant
