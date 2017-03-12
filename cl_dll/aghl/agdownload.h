// AgDownload.h: interface for the AgDownload class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_AGDOWNLOAD_H__67A39740_D747_4453_AC71_32320604FAAB__INCLUDED_)
#define AFX_AGDOWNLOAD_H__67A39740_D747_4453_AC71_32320604FAAB__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class AgDownload  
{
public:
	AgDownload();
	virtual ~AgDownload();

	void DownloadFile(const char* pszURL, const char* pszSaveAs);

};

#endif // !defined(AFX_AGDOWNLOAD_H__67A39740_D747_4453_AC71_32320604FAAB__INCLUDED_)
