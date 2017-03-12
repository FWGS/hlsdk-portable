//++ BulliT

#if !defined(AFX_AGADMIN_H__E1E58F06_B2BD_43F9_99A0_0B3F6D6B7B16__INCLUDED_)
#define AFX_AGADMIN_H__E1E58F06_B2BD_43F9_99A0_0B3F6D6B7B16__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

// The admin user
class AgAdmin  
{
public:
	AgAdmin();
	virtual ~AgAdmin();

  AgString      m_sAdmin;
  AgString      m_sPassword;
  AgString      m_sAuthID;
};

#endif // !defined(AFX_AGADMIN_H__E1E58F06_B2BD_43F9_99A0_0B3F6D6B7B16__INCLUDED_)

//-- Martin Webrant
