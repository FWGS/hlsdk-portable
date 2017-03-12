//++ BulliT
#if !defined(_AG_PAK_H_)
#define _AG_PAK_H_

class AgPak
{
public:
  AgPak();
  bool GetEntries(const AgString& sPakfile, const AgString& sSearch1, const AgString& sSearch2, AgStringList& lstEntries);
};


#endif // !defined(_AG_PAK_H_)

//-- Martin Webrant
