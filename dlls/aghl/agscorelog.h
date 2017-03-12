//++ BulliT


#if !defined(AFX_AGSCORELOG_H__A764B26F_F098_4237_B5B9_A62BD9FD9BF6__INCLUDED_)
#define AFX_AGSCORELOG_H__A764B26F_F098_4237_B5B9_A62BD9FD9BF6__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

typedef map<AgString,int,less<AgString> > AgScoreLogMap;

class AgScoreLog  
{
  float m_fNextLogUpdate;
  void EndScore();

public:
	AgScoreLog();
	virtual ~AgScoreLog();

  void Think();

  void Start();
  void End();
  void Score();

  void GetScores(AgScoreLogMap& mapScores);
};

#endif // !defined(AFX_AGSCORELOG_H__A764B26F_F098_4237_B5B9_A62BD9FD9BF6__INCLUDED_)

//-- Martin Webrant

