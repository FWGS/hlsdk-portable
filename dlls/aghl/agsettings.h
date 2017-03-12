//++ BulliT

#if !defined(AFX_AGSETTINGS_H__84946760_20B0_4629_84A4_115B6422C74D__INCLUDED_)
#define AFX_AGSETTINGS_H__84946760_20B0_4629_84A4_115B6422C74D__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class AgSettings  
{
  bool      m_bChangeNextLevel;
  bool      m_bCheckNextMap;
  bool      m_bCalcNextMap;
  float     m_fNextCheck;
  void      CalcNextMap();

public:
	AgSettings();
	virtual ~AgSettings();

  bool Think();

  void      SetNextLevel(const AgString& sMap);
  AgString  GetNextLevel();
  void      ChangeNextLevel();

  void      Changelevel(const AgString& sMap);
  bool      AdminSetting(const AgString& sSetting, const AgString& sValue);  //Not all settings is allowed.

};


#endif // !defined(AFX_AGSETTINGS_H__84946760_20B0_4629_84A4_115B6422C74D__INCLUDED_)

//-- Martin Webrant
