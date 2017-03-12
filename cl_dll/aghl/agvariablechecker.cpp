//++ BulliT

#include "hud.h"
#include "cl_util.h"
#include "const.h"
#include "com_model.h"
#include "studio.h"
#include "entity_state.h"
#include "cl_entity.h"
#include "dlight.h"
#include "triangleapi.h"
#include "studio_util.h"
#include "r_studioint.h"
#include "AgVariableChecker.h"
#include "AgModelCheck.h"


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

#ifdef AG_USE_CHEATPROTECTION

extern engine_studio_api_t IEngineStudio;

#define MAX_VIOLATIONS 5

struct VARIABLES
{
  char    szName[20];
  double  dMin;
  double  dMax;
  double  dDefault;
  bool    bKeepSame;
  bool    bAllowChangeAtInit;
};

static VARIABLES s_Variables[]=
{
  //Evironment
  "r_lightmap",-1,0,-1,false,true,
  "direct",0.9,0.9,0.9,false,true,
  "brightness",0,30,1,false,true,
  "lambert",1.5,1.5,1.5,true,true,
  "cl_solid_players",1,1,1,false,true,
  "cl_himodels",0,1,0,true,false,
  //Sounds
  "s_distance",10,60,60,false,true,
  "s_min_distance",1,20,8,false,true,
  "s_max_distance",20,1000,1000,false,true,
  "s_occlude",0,1,1,false,true,
  "s_occfactor",0.20,0.25,0.25,false,true,
  "s_refgain",0.4,0.45,0.41,false,true,
  //Cheats is somehow client also?
  "sv_cheats",0,0,0,true,true,
};


static VARIABLES s_VariablesFast[]=
{
  //Movement
  "m_pitch",-0.044,0.044,0.022,false,true,
  "cl_backspeed",300,500,400,false,true,
  "cl_sidespeed",300,500,400,false,true,
  "cl_forwardspeed",300,500,400,false,true,
  "cl_upspeed",300,500,320,false,true,
  "default_fov",30,130,90,false,true,
  //Net settings
  "cl_updaterate",15,100,30,false,true,
  "cl_cmdrate",15,100,30,false,true,
  "ex_extrapmax",1.2,1.2,1.2,false,true,
#ifdef CLIENT_WEAPONS
  "ex_interp",0.05,0.1,0.1,false,true,
#else
  "ex_interp",0.1,0.1,0.1,false,true,
#endif
//  Fixed in next patch.
  "ex_minvelocity",0,0,0,false,true,
  "ex_maxspeed",750,750,750,false,true,
  "ex_maxaccel",2000,2000,2000,false,true,
  "ex_maxerrordistance",64,64,64,false,true,
  "cl_nopred",0,0,0,false,true,
  "ex_correct",0,0,0,false,true,
  "ex_diminishextrap",0,0,0,false,true,
};

/*
ex_correct 
ex_extrapmax 
ex_maxerror distance 
cl_nopred 
*/

static VARIABLES s_VariablesHardware[]=
{
  //Open GL settings.
//  "lightgamma",0,3,2.5,true,false,
//  "texgamma",1,3,2,true,false,
  "gl_max_size",128,512,512,false,false,
  "gl_zmax",2048,9999,4096,false,true,
  "gl_alphamin",0.25,0.25,0.25,false,true,
  "gl_nobind",0,0,0,false,true,
  "gl_picmip",0,2,0,false,true,
  "gl_playermip",0,5,0,false,true,
  //"gl_spriteblend",1,1,1,true,true,
  "gl_monolights",0,0,0,true,true,
};

static VARIABLES s_VariablesPure2[]=
{
  "s_distance",10,10,10,false,true,
  "s_rolloff",10,10,10,false,true,
  "s_min_distance",8,8,8,false,true,
  "s_max_distance",1000,1000,1000,false,true,
};

static VARIABLES s_VariablesPure3[]=
{
  "s_a3d",0,0,0,false,true,
};


AgVariableChecker g_VariableChecker;
static char szDisconnect[] = "disconnect\n";

AgVariableChecker::AgVariableChecker()
{
  m_bHardware = false;
  m_bInit = false;
  m_dwNextCheck = m_dwNextCheckFast = m_dwNextA3D = GetTickCount();
  m_iViolations = 0;
  m_bActive = false;
}

AgVariableChecker::~AgVariableChecker()
{

}

void AgVariableChecker::Activate()
{
  m_bActive = true;
  m_bInit = false;
} 


bool InitVariables(VARIABLES* pvarArray,int iElements)
{
  for (int i = 0; i < iElements; i++)
  {
    if (!gEngfuncs.pfnGetCvarPointer(pvarArray[i].szName ))
      continue;
    
    double dValue = gEngfuncs.pfnGetCvarFloat(pvarArray[i].szName);

    if (dValue < (pvarArray[i].dMin - 0.0001) 
      ||dValue > (pvarArray[i].dMax + 0.0001))
    {
      if (pvarArray[i].bAllowChangeAtInit)
      {
        //Correct it.
   		  gEngfuncs.Cvar_SetValue(pvarArray[i].szName, pvarArray[i].dDefault);

        //This dudes standard aint good enough - corrected it for him.
        char szMessage[256];
        sprintf(szMessage,"Server enforces variables and \"%s\" was automatically reset to the HL default %f.\n",pvarArray[i].szName,pvarArray[i].dDefault);        
        ConsolePrint(szMessage);
        AgLog(szMessage);
      }
      else
      {
        //This dudes standard aint good enough.
        char szMessage[256];
        sprintf(szMessage,"Server enforces variables and \"%s\" needs to be between %f and %f. Your variable is set to %f.\n",pvarArray[i].szName,pvarArray[i].dMin,pvarArray[i].dMax,dValue);        
        ConsolePrint(szMessage);
        AgLog(szMessage);
        return false;
      }
    }

    //Save this value as default.
    pvarArray[i].dDefault = gEngfuncs.pfnGetCvarFloat(pvarArray[i].szName);
    if (pvarArray[i].bKeepSame)
      pvarArray[i].dMin = pvarArray[i].dMax = pvarArray[i].dDefault;
  }
  return true;
}

void CheckVariables(VARIABLES* pvarArray,int iElements, short& iViolations)
{
  for (int i = 0; i < iElements; i++)
  {
    if (!gEngfuncs.pfnGetCvarPointer(pvarArray[i].szName ))
      continue;

    double dValue = gEngfuncs.pfnGetCvarFloat(pvarArray[i].szName);
    if (dValue < (pvarArray[i].dMin - 0.0001) 
      ||dValue > (pvarArray[i].dMax + 0.0001))
    {
      char szMessage[256];
      if (pvarArray[i].bKeepSame && pvarArray[i].bAllowChangeAtInit)
        sprintf(szMessage,"Server enforces variables and \"%s\" needs to be the same during the game\n",pvarArray[i].szName);
      else
        sprintf(szMessage,"Server enforces variables and \"%s\" needs to be between %f and %f.\n",pvarArray[i].szName,pvarArray[i].dMin,pvarArray[i].dMax);
      ConsolePrint(szMessage);

	  if (pvarArray[i].bAllowChangeAtInit)
	  {
		//Reset to previous value.
   		gEngfuncs.Cvar_SetValue(pvarArray[i].szName, pvarArray[i].dDefault);
		iViolations++;
	  }
	  else
	  {
	  	iViolations = MAX_VIOLATIONS + 1;
	  }


      sprintf(szMessage,"say <AG Mod> Warned for using variable %s with value %lf (%d violation(s)",pvarArray[i].szName,dValue,iViolations);
      ServerCmd(szMessage);
    }
  }
}

bool AgVariableChecker::Init()
{
  if (m_bInit)
    return true;

  bool bSuccess = true;

  m_bHardware = IEngineStudio.IsHardware() ? true : false;
  
  if (!InitVariables(s_Variables,sizeof(s_Variables)/sizeof(s_Variables[0])))
    bSuccess = false;

  if (!InitVariables(s_VariablesFast,sizeof(s_VariablesFast)/sizeof(s_VariablesFast[0])))
    bSuccess = false;

  if (m_bHardware)
  {
    if (!InitVariables(s_VariablesHardware,sizeof(s_VariablesHardware)/sizeof(s_VariablesHardware[0])))
      bSuccess = false;
  }

  if (bSuccess)
  {
    m_bInit = true;
    return true;
  }

  static char szMessage[] = "Variable init failed, exiting.\n";
  ConsolePrint(szMessage);
  AgLog(szMessage);

  ServerCmd( "say <AG Mod> Disconnected for using invalid variable.\n" );
	ClientCmd( szDisconnect );

  Reset();

  return false;
}

void AgVariableChecker::Reset()
{
  m_bHardware = false;
  m_bInit = false;
  m_bActive = false;
  m_iViolations = 0;
}

bool AgVariableChecker::Check()
{
  if (!m_bActive)
    return true;

  if (!m_bInit)
  {
    return Init();
  }

  DWORD dwNow = GetTickCount();
  if (m_dwNextCheck < dwNow)
  {
	  CheckVariables(s_Variables,sizeof(s_Variables)/sizeof(s_Variables[0]),m_iViolations);

    if (m_bHardware)
		  CheckVariables(s_VariablesHardware,sizeof(s_VariablesHardware)/sizeof(s_VariablesHardware[0]),m_iViolations);

    if (g_iPure > 1)
	    CheckVariables(s_VariablesPure2,sizeof(s_VariablesPure2)/sizeof(s_VariablesPure2[0]),m_iViolations);

    m_dwNextCheck = dwNow + 500; //500ms for the less important variables.
  }

  if (m_dwNextCheckFast < dwNow)
  {
	  CheckVariables(s_VariablesFast,sizeof(s_VariablesFast)/sizeof(s_VariablesFast[0]),m_iViolations);
    m_dwNextCheckFast = dwNow + 150; //150ms for the important variables.
  }

  if (g_iPure > 2)
  {
    if (m_dwNextA3D < dwNow)
    {
	  CheckVariables(s_VariablesPure3,sizeof(s_VariablesPure3)/sizeof(s_VariablesPure3[0]),m_iViolations);
      ClientCmd("s_disable_a3d\n");
      m_dwNextA3D = dwNow + 5000; //5 seconds .
    }
  }

  if (m_iViolations > MAX_VIOLATIONS)
  {
    static char szMessageServer[] = "say <AG Mod> Disconnected for repeated cvar violations\n";
    static char szMessage[] = "Cheat check: Disconnected for repeated cvar violations\n";
    ServerCmd(szMessageServer);
    AgLog(szMessage);
    ConsolePrint(szMessage);
    ClientCmd(szDisconnect);
    Reset();
    return false;
  }

  return true;
}

#endif //AG_USE_CHEATPROTECTION

//-- Martin Webrant
