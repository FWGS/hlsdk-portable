#include <memory.h>
#include "hud.h"
#include "cl_util.h"
#include "const.h"
#include "entity_types.h"
#include "cdll_int.h"
#include "pm_defs.h"
#include "event_api.h"
#include "rain.h"

void WaterLandingEffect(cl_drip *drip);
void ParseRainFile( void );

rain_properties     Rain;

cl_drip     FirstChainDrip;
cl_rainfx   FirstChainFX;

double rain_curtime;    // current time
double rain_oldtime;    // last time we have updated drips
double rain_timedelta;  // difference between old time and current time
double rain_nextspawntime;  // when the next drip should be spawned

int dripcounter = 0;
int fxcounter = 0;
/*
=================================
ProcessRain
Перемещает существующие объекты, удаляет их
при надобности, и, если дождь включен, создает новые.

Должна вызываться каждый кадр.
=================================
*/
void ProcessRain( void )
{
	rain_oldtime = rain_curtime; // save old time
	rain_curtime = gEngfuncs.GetClientTime();
	rain_timedelta = rain_curtime - rain_oldtime;

	// first frame
	if (rain_oldtime == 0)
	{
		// fix first frame bug with nextspawntime
		rain_nextspawntime = gEngfuncs.GetClientTime();
		ParseRainFile();
		return;
	}

	if (Rain.dripsPerSecond == 0 && FirstChainDrip.p_Next == NULL)
	{
		// keep nextspawntime correct
		rain_nextspawntime = rain_curtime;
		return;
	}

	if (rain_timedelta == 0)
		return; // not in pause

	double timeBetweenDrips = 1 / (double)Rain.dripsPerSecond;

	cl_drip* curDrip = FirstChainDrip.p_Next;
	cl_drip* nextDrip = NULL;

	cl_entity_t *player = gEngfuncs.GetLocalPlayer();

	// хранение отладочной информации
	float debug_lifetime = 0;
	int debug_howmany = 0;
	int debug_attempted = 0;
	int debug_dropped = 0;

	while (curDrip != NULL) // go through list
	{
		nextDrip = curDrip->p_Next; // save pointer to next drip

		if (Rain.weatherMode == 0)
			curDrip->origin.z -= rain_timedelta * DRIPSPEED; // rain
		else
			curDrip->origin.z -= rain_timedelta * SNOWSPEED; // snow

		curDrip->origin.x += rain_timedelta * curDrip->xDelta;
		curDrip->origin.y += rain_timedelta * curDrip->yDelta;
		
		// remove drip if its origin lower than minHeight
		if (curDrip->origin.z < curDrip->minHeight) 
		{
			if (curDrip->landInWater)
				WaterLandingEffect(curDrip); // create water rings

			if (gHUD.RainInfo->value)
			{
				debug_lifetime += (rain_curtime - curDrip->birthTime);
				debug_howmany++;
			}
			
			curDrip->p_Prev->p_Next = curDrip->p_Next; // link chain
			if (nextDrip != NULL)
				nextDrip->p_Prev = curDrip->p_Prev; 
			delete curDrip;
					
			dripcounter--;
		}

		curDrip = nextDrip; // restore pointer, so we can continue moving through chain
	}

int maxDelta; // maximum height randomize distance
	float falltime;
	if (Rain.weatherMode == 0)
	{
		maxDelta = DRIPSPEED * rain_timedelta; // for rain
		falltime = (Rain.globalHeight + 12000) / DRIPSPEED;
	}
	else
	{
		maxDelta = SNOWSPEED * rain_timedelta; // for snow
		falltime = (Rain.globalHeight + 12000) / SNOWSPEED;
	}

	while (rain_nextspawntime < rain_curtime)
	{
		rain_nextspawntime += timeBetweenDrips;		
		if (gHUD.RainInfo->value)
			debug_attempted++;
				
		if (dripcounter < MAXDRIPS) // check for overflow
		{
			float deathHeight;
			vec3_t vecStart, vecEnd;

			vecStart[0] = gEngfuncs.pfnRandomFloat(player->origin.x - Rain.distFromPlayer, player->origin.x + Rain.distFromPlayer);
			vecStart[1] = gEngfuncs.pfnRandomFloat(player->origin.y - Rain.distFromPlayer, player->origin.y + Rain.distFromPlayer);
			vecStart[2] = Rain.globalHeight;

			float xDelta = Rain.windX + gEngfuncs.pfnRandomFloat(Rain.randX * -1, Rain.randX);
			float yDelta = Rain.windY + gEngfuncs.pfnRandomFloat(Rain.randY * -1, Rain.randY);

			// find a point at bottom of map
			vecEnd[0] = falltime * xDelta;
			vecEnd[1] = falltime * yDelta;
			vecEnd[2] = -4096;

			pmtrace_t pmtrace;
			gEngfuncs.pEventAPI->EV_SetTraceHull( 2 );
			gEngfuncs.pEventAPI->EV_PlayerTrace( vecStart, vecStart + vecEnd, PM_STUDIO_IGNORE, -1, &pmtrace );

			if (pmtrace.startsolid || pmtrace.allsolid)
			{
				if (gHUD.RainInfo->value)
					debug_dropped++;
				continue; // drip cannot be placed
			}
			
			// falling to water?
			int contents = gEngfuncs.PM_PointContents( pmtrace.endpos, NULL );
			if (contents == CONTENTS_WATER)
			{
				int waterEntity = gEngfuncs.PM_WaterEntity( pmtrace.endpos );
				if ( waterEntity > 0 )
				{
					cl_entity_t *pwater = gEngfuncs.GetEntityByIndex( waterEntity );
					if ( pwater && ( pwater->model != NULL ) )
					{
						deathHeight = pwater->curstate.maxs[2];
					}
					else
					{
						gEngfuncs.Con_Printf("Rain error: can't get water entity\n");
						continue;
					}
				}
				else
				{
//					gEngfuncs.Con_Printf("Rain error: water is not func_water entity\n");
					continue;
				}
			}
			else
			{
				deathHeight = pmtrace.endpos[2];
			}

			// just in case..
			if (deathHeight > vecStart[2])
			{
				gEngfuncs.Con_Printf("Rain error: can't create drip in water\n");
				continue;
			}


			cl_drip *newClDrip = new cl_drip;
			if (!newClDrip)
			{
				Rain.dripsPerSecond = 0; // disable rain
				gEngfuncs.Con_Printf( "Rain error: failed to allocate object!\n");
				return;
			}
			
			vecStart[2] -= gEngfuncs.pfnRandomFloat(0, maxDelta); // randomize a bit
			
			newClDrip->alpha = gEngfuncs.pfnRandomFloat(0.12, 0.2);
			VectorCopy(vecStart, newClDrip->origin);
			
			newClDrip->xDelta = xDelta;
			newClDrip->yDelta = yDelta;
	
			newClDrip->birthTime = rain_curtime; // store time when it was spawned
			newClDrip->minHeight = deathHeight;

			if (contents == CONTENTS_WATER)
				newClDrip->landInWater = 1;
			else
				newClDrip->landInWater = 0;

			// add to first place in chain
			newClDrip->p_Next = FirstChainDrip.p_Next;
			newClDrip->p_Prev = &FirstChainDrip;
			if (newClDrip->p_Next != NULL)
				newClDrip->p_Next->p_Prev = newClDrip;
			FirstChainDrip.p_Next = newClDrip;

			dripcounter++;
		}
		else
		{
			gEngfuncs.Con_Printf( "Rain error: Drip limit overflow!\n" );
			return;
		}
	}

if (gHUD.RainInfo->value) // print debug info
	{
		gEngfuncs.Con_Printf( "Rain info: Drips exist: %i\n", dripcounter );
		gEngfuncs.Con_Printf( "Rain info: FX's exist: %i\n", fxcounter );
		gEngfuncs.Con_Printf( "Rain info: Attempted/Dropped: %i, %i\n", debug_attempted, debug_dropped);
		if (debug_howmany)
		{
			float ave = debug_lifetime / (float)debug_howmany;
			gEngfuncs.Con_Printf( "Rain info: Average drip life time: %f\n", ave);
		}
		else
			gEngfuncs.Con_Printf( "Rain info: Average drip life time: --\n");
	}
	return;
}

/*
=================================
WaterLandingEffect
создает круг на водной поверхности
=================================
*/
void WaterLandingEffect(cl_drip *drip)
{
	if (fxcounter >= MAXFX)
	{
		gEngfuncs.Con_Printf( "Rain error: FX limit overflow!\n" );
		return;
	}	
	
	cl_rainfx *newFX = new cl_rainfx;
	if (!newFX)
	{
		gEngfuncs.Con_Printf( "Rain error: failed to allocate FX object!\n");
		return;
	}
			
	newFX->alpha = gEngfuncs.pfnRandomFloat(0.6, 0.9);
	VectorCopy(drip->origin, newFX->origin);
	newFX->origin[2] = drip->minHeight; // correct position
			
	newFX->birthTime = gEngfuncs.GetClientTime();
	newFX->life = gEngfuncs.pfnRandomFloat(0.7, 1);

	// add to first place in chain
	newFX->p_Next = FirstChainFX.p_Next;
	newFX->p_Prev = &FirstChainFX;
	if (newFX->p_Next != NULL)
		newFX->p_Next->p_Prev = newFX;
	FirstChainFX.p_Next = newFX;
			
	fxcounter++;
}

/*
=================================
ProcessFXObjects
удаляет FX объекты, у которых вышел срок жизни

Каждый кадр вызывается перед ProcessRain
=================================
*/
void ProcessFXObjects( void )
{
	float curtime = gEngfuncs.GetClientTime();
	
	cl_rainfx* curFX = FirstChainFX.p_Next;
	cl_rainfx* nextFX = NULL;	

	while (curFX != NULL) // go through FX objects list
	{
		nextFX = curFX->p_Next; // save pointer to next
		
		// delete current?
		if ((curFX->birthTime + curFX->life) < curtime)
		{
			curFX->p_Prev->p_Next = curFX->p_Next; // link chain
			if (nextFX != NULL)
				nextFX->p_Prev = curFX->p_Prev; 
			delete curFX;					
			fxcounter--;
		}
		curFX = nextFX; // restore pointer
	}
}

//=================================
//ResetRain
//очищает память, удаляя все объекты.
//=================================
void ResetRain( void )
{
// delete all drips
	cl_drip* delDrip = FirstChainDrip.p_Next;
	FirstChainDrip.p_Next = NULL;
	
	while (delDrip != NULL)
	{
		cl_drip* nextDrip = delDrip->p_Next; // save pointer to next drip in chain
		delete delDrip;
		delDrip = nextDrip; // restore pointer
		dripcounter--;
	}

// delete all FX objects
	cl_rainfx* delFX = FirstChainFX.p_Next;
	FirstChainFX.p_Next = NULL;
	
	while (delFX != NULL)
	{
		cl_rainfx* nextFX = delFX->p_Next;
		delete delFX;
		delFX = nextFX;
		fxcounter--;
	}

	InitRain();
	return;
}

/*
=================================
InitRain
Инициализирует все переменные нулевыми значениями.
=================================
*/
void InitRain( void )
{
	Rain.dripsPerSecond = 0;
	Rain.distFromPlayer = 0;
	Rain.windX = 0;
	Rain.windY = 0;
	Rain.randX = 0;
	Rain.randY = 0;
	Rain.weatherMode = 0;
	Rain.globalHeight = 0;

	FirstChainDrip.birthTime = 0;
	FirstChainDrip.minHeight = 0;
	FirstChainDrip.origin[0]=0;
	FirstChainDrip.origin[1]=0;
	FirstChainDrip.origin[2]=0;
	FirstChainDrip.alpha = 0;
	FirstChainDrip.xDelta = 0;
	FirstChainDrip.yDelta = 0;
	FirstChainDrip.landInWater = 0;
	FirstChainDrip.p_Next = NULL;
	FirstChainDrip.p_Prev = NULL;

	FirstChainFX.alpha = 0;
	FirstChainFX.birthTime = 0;
	FirstChainFX.life = 0;
	FirstChainFX.origin[0] = 0;
	FirstChainFX.origin[1] = 0;
	FirstChainFX.origin[2] = 0;
	FirstChainFX.p_Next = NULL;
	FirstChainFX.p_Prev = NULL;
	
	rain_oldtime = 0;
	rain_curtime = 0;
	rain_nextspawntime = 0;

	return;
}

/*
===========================
ParseRainFile

пытается загрузить настройки дождя из файла.
имеет низкий приоритет перед настройками с карты

Список настроек:
	drips
	distance
	windx
	windy
	randx
	randy
	mode
	height
===========================
*/
void ParseRainFile( void )
{
	if (Rain.distFromPlayer != 0 || Rain.dripsPerSecond != 0 || Rain.globalHeight != 0)
		return;

	char mapname[64];
	char token[64];
	char *pfile;

	strcpy( mapname, gEngfuncs.pfnGetLevelName() );
	if (strlen(mapname) == 0)
	{
		gEngfuncs.Con_Printf( "Rain error: unable to read map name\n");
		return;
	}

	mapname[strlen(mapname)-4] = 0;
	sprintf(mapname, "%s.pcs", mapname); // pcs - precipiations - осадки

	pfile = (char *)gEngfuncs.COM_LoadFile( mapname, 5, NULL);
	if (!pfile)
	{
		if (gHUD.RainInfo->value)
			gEngfuncs.Con_Printf("Rain: couldn't open rain settings file %s\n", mapname);
		return;
	}

	while (true)
	{
		pfile = gEngfuncs.COM_ParseFile(pfile, token);
		if (!pfile)
			break;

		if (!stricmp(token, "drips")) // dripsPerSecond
		{
			pfile = gEngfuncs.COM_ParseFile(pfile, token);
			Rain.dripsPerSecond = atoi(token);
		}
		else if (!stricmp(token, "distance")) // distFromPlayer
		{
			pfile = gEngfuncs.COM_ParseFile(pfile, token);
			Rain.distFromPlayer = atof(token);
		}
		else if (!stricmp(token, "windx")) // windX
		{
			pfile = gEngfuncs.COM_ParseFile(pfile, token);
			Rain.windX = atof(token);
		}
		else if (!stricmp(token, "windy")) // windY
		{
			pfile = gEngfuncs.COM_ParseFile(pfile, token);
			Rain.windY = atof(token);		
		}
		else if (!stricmp(token, "randx")) // randX
		{
			pfile = gEngfuncs.COM_ParseFile(pfile, token);
			Rain.randX = atof(token);
		}
		else if (!stricmp(token, "randy")) // randY
		{
			pfile = gEngfuncs.COM_ParseFile(pfile, token);
			Rain.randY = atof(token);
		}
		else if (!stricmp(token, "mode")) // weatherMode
		{
			pfile = gEngfuncs.COM_ParseFile(pfile, token);
			Rain.weatherMode = atoi(token);
		}
		else if (!stricmp(token, "height")) // globalHeight
		{
			pfile = gEngfuncs.COM_ParseFile(pfile, token);
			Rain.globalHeight = atof(token);
		}
		else
			gEngfuncs.Con_Printf("Rain error: unknown token %s in file %s\n", token, mapname);
	}
	
	gEngfuncs.COM_FreeFile( pfile );
}


