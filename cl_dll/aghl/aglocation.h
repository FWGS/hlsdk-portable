//++ BulliT

#if !defined(_AG_LOCATION_)
#define _AG_LOCATION_

#include "agglobal.h"

class AgLocation  
{
public:
	AgLocation();
	virtual ~AgLocation();

  AgString  m_sLocation;
	Vector	  m_vPosition;

  void Show();
};

#endif //_AG_LOCATION_

//-- Martin Webrant
