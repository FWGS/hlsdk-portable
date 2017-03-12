//++ BulliT
#if !defined(AFX_AGVARIABLECHECKER_H__73BB9962_9A14_4A89_B856_FEFB40FC1E13__INCLUDED_)
#define AFX_AGVARIABLECHECKER_H__73BB9962_9A14_4A89_B856_FEFB40FC1E13__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifdef AG_USE_CHEATPROTECTION

class AgVariableChecker  
{
  bool m_bActive;
  bool m_bHardware;
  bool m_bInit;
  DWORD m_dwNextCheck;
  DWORD m_dwNextCheckFast;
  DWORD m_dwNextA3D;
  short m_iViolations;
  void Reset();

public:
	AgVariableChecker();
	virtual ~AgVariableChecker();

  bool Init();
  bool Check();
  void Activate();
};

extern AgVariableChecker g_VariableChecker;

#endif //AG_USE_CHEATPROTECTION

extern int g_iPure;

extern cvar_t *cl_pitchspeed;
inline float ag_cl_pitchspeed()
{
  if (0 < g_iPure)
    return 255;
  return cl_pitchspeed->value;
}

extern cvar_t *cl_pitchup;
inline float ag_cl_pitchup()
{
  if (0 < g_iPure)
    return 89;
  return cl_pitchup->value;
}

extern cvar_t *cl_pitchdown;
inline float ag_cl_pitchdown()
{
  if (0 < g_iPure)
    return 89;
  return cl_pitchdown->value;
}

extern cvar_t *cl_yawspeed;
inline float ag_cl_yawspeed()
{
  if (0 < g_iPure)
    return 210;
  return cl_yawspeed->value;
}



#endif // !defined(AFX_AGVARIABLECHECKER_H__73BB9962_9A14_4A89_B856_FEFB40FC1E13__INCLUDED_)
//-- Martin Webrant
