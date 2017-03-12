//++ BulliT

#include "hud.h"
#include "cl_util.h"
#include "const.h"
#include "entity_state.h"
#include "cl_entity.h"
#include "pm_defs.h"
#include "event_api.h"
#include "r_efx.h"
#include "aglocation.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

AgLocation::AgLocation()
{
  m_vPosition = Vector(0,0,0);
}

AgLocation::~AgLocation()
{

} 

void AgLocation::Show()
{
  int iSpot = gEngfuncs.pEventAPI->EV_FindModelIndex( "sprites/laserdot.spr" );
  gEngfuncs.pEfxAPI->R_TempSprite( m_vPosition, vec3_origin, 1, iSpot, kRenderTransAlpha, kRenderFxNoDissipation, 255.0, 10, FTENT_SPRCYCLE );
}

//-- Martin Webrant


