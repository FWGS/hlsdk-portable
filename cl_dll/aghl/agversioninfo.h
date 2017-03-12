#ifndef __AG_VERSION_INFO_H__
#define __AG_VERSION_INFO_H__

#ifdef AG_USE_CHEATPROTECTION

class AgVersionInfo 
{
public:

  // Constructs the class.
  AgVersionInfo();  
  virtual ~AgVersionInfo();

  // Load versioninfo.
  DWORD LoadVersionInfo(LPCSTR pszFileName);  // Load versioninfo on a file.

  BOOL  HasErrors()    const; // Returns TRUE if versioninfo could not be retrivied.
  DWORD GetLastError() const; // Returns the windows errorcode.

  // language independant data

  // Get the fileversion.
  BOOL FileVersion(int& iMajor,int& iMinor,int& iMicro,int& iState)    const; // Get the fileversion.
  // Get the productversion.
  BOOL ProductVersion(int& iMajor,int& iMinor,int& iMicro,int& iState) const; // Get the productversion.

  BOOL IsDebugBuild()   const;  // Returns TRUE if the file contains debugging information or is compiled with debugging features enabled.
  BOOL IsInfoInferred() const;  // Returns TRUE if the file's version structure was created dynamically; therefore, some of the members in this class may be empty or incorrect.
  BOOL IsPatched()      const;  // Returns TRUE if the file has been modified and is not identical to the original shipping file of the same version number.
  BOOL IsPreRelease()   const;  // Returns TRUE if the file is a development version, not a commercially released product.
  BOOL IsPrivateBuild() const;  // Returns TRUE if the file was not built using standard release procedures.
  BOOL IsSpecialBuild() const;  // Returns TRUE if the file was built by the original company using standard release procedures but is a variation of the normal file of the same version number.

  DWORD FileFlag()    const;    // Returns a bitmask that specifies the Boolean attributes of the file. Use IsXXXX functions instead.
  DWORD OSFlag()      const;    // Returns: VOS_NT The file was designed for Windows NT. VOS__WINDOWS32 The file was designed for the Win32 API. For more flags see dwFileOS variable for VS_FIXEDFILEINFO in MSDN.
  DWORD TypeFlag()    const;    // Returns: VFT_APP The file contains an application. VFT_DLL The file contains a dynamic-link library (DLL). For more flags see dwFileType  variable for VS_FIXEDFILEINFO in MSDN.
  DWORD SubtypeFlag() const;    // Returns: See dwFileSubtype variable for VS_FIXEDFILEINFO in MSDN.
  
  BOOL  IsRequiredVersion(int iReqMajor, int iReqMinor = 0, int iReqMicro = 0) const; // Returns TRUE if the file has the required version or is newer.

  // language dependant data

  const char* Comments(        DWORD dwLanguage = -1L); // Returns additional information that should be displayed for diagnostic purposes.
  const char* CompanyName(     DWORD dwLanguage = -1L); // Returns the company that produced the file. For example, "Microsoft Corporation" or "ADRA Datasystem AB (publ)." 
  const char* FileDescription( DWORD dwLanguage = -1L); // Returns the description in such a way that it can be presented to users. This string may be presented in a list box when the user is choosing files to install. For example, "ViewLine" or "Microsoft Word for Windows". 
  const char* FileVersion(     DWORD dwLanguage = -1L); // Returns the version of this file. For example, "3.00.001R" or "5.00.RC2".
  const char* InternalName(    DWORD dwLanguage = -1L); // Returns the files internal name. For example. "UDIMEAS" or "WINWORD"
  const char* LegalCopyright(  DWORD dwLanguage = -1L); // Returns all copyright notices, trademarks, and registered trademarks that apply to the file. For example "Copyright © ADRA Datasystem AB (publ) 1997- 1998"
  const char* LegalTrademarks( DWORD dwLanguage = -1L); // Returns all trademarks and registered trademarks that apply to the file. This should include the full text of all notices, legal symbols, trademark numbers, and so on. In English, this string should be in the format "UDIBAS is a trademark of ADRA Datasystem AB.".
  const char* OriginalFileName(DWORD dwLanguage = -1L); // Returns identifies the original name of the file, not including a path. This enables an application to determine whether a file has been renamed by a user.
  const char* PrivateBuild(    DWORD dwLanguage = -1L); // Returns whom, where, and why this private version of the file was built.
  const char* ProductName(     DWORD dwLanguage = -1L); // Returns the name of the product with which this file is distributed. For example, this string could be "Microsoft Windows".
  const char* ProductVersion(  DWORD dwLanguage = -1L); // Returns the version of the product with which this file is distributed. For example, "3.00.001R" or "5.00.RC2".
  const char* SpecialBuild(    DWORD dwLanguage = -1L); // Returns a description how this version of the file differs from the normal version.

  const char* GetTextData(LPCSTR pszParameter,DWORD dwLanguage = -1L); // Do a VerQueryValue for a paramater. For example "ProductVersion".

protected:

  void  Init();
  DWORD SetError();

  DWORD            m_dwHandle;
  char*            m_pszData;
  DWORD            m_dwDefaultLang;
  VS_FIXEDFILEINFO m_ffi;
  DWORD            m_dwLastError;
};

// AgVersionInfo inlines //

inline void AgVersionInfo::Init()
{ m_dwLastError = (DWORD)-1; m_dwDefaultLang = (DWORD)-1; m_dwHandle = (DWORD)0; memset(&m_ffi,'\0',sizeof(m_ffi)); m_pszData = NULL; }

inline AgVersionInfo::AgVersionInfo()
{ Init(); }

inline  AgVersionInfo::~AgVersionInfo()
{
  if (m_pszData)
    free(m_pszData);
}

inline BOOL AgVersionInfo::HasErrors() const
{ return (0 != GetLastError()); }

inline DWORD AgVersionInfo::GetLastError() const
{ return m_dwLastError; }

inline BOOL AgVersionInfo::IsDebugBuild() const
{ //AFC_ASSERT(!HasErrors()); 
return ( 0 != (VS_FF_DEBUG & FileFlag())); }

inline BOOL AgVersionInfo::IsInfoInferred() const
{ //AFC_ASSERT(!HasErrors()); 
return ( 0 != (VS_FF_INFOINFERRED & FileFlag())); }

inline BOOL AgVersionInfo::IsPatched() const
{ //AFC_ASSERT(!HasErrors()); 
return ( 0 != (VS_FF_PATCHED & FileFlag())); }

inline BOOL AgVersionInfo::IsPreRelease() const
{ //AFC_ASSERT(!HasErrors()); 
return ( 0 != (VS_FF_PRERELEASE & FileFlag())); }

inline BOOL AgVersionInfo::IsPrivateBuild() const
{ //AFC_ASSERT(!HasErrors()); 
return ( 0 != (VS_FF_PRIVATEBUILD & FileFlag())); }

inline BOOL AgVersionInfo::IsSpecialBuild() const
{ //AFC_ASSERT(!HasErrors()); 
return ( 0 != (VS_FF_SPECIALBUILD & FileFlag())); }

inline DWORD AgVersionInfo::FileFlag() const
{ //AFC_ASSERT(!HasErrors()); 
return (m_ffi.dwFileFlagsMask & m_ffi.dwFileFlags); }

inline DWORD AgVersionInfo::OSFlag() const
{ //AFC_ASSERT(!HasErrors()); 
return m_ffi.dwFileOS; }

inline DWORD AgVersionInfo::TypeFlag() const
{ //AFC_ASSERT(!HasErrors()); 
return m_ffi.dwFileType; }

inline DWORD AgVersionInfo::SubtypeFlag() const
{ //AFC_ASSERT(!HasErrors()); 
return m_ffi.dwFileSubtype; }

inline const char* AgVersionInfo::Comments(DWORD dwLanguage)
{ return GetTextData("Comments",dwLanguage); }

inline const char* AgVersionInfo::CompanyName(DWORD dwLanguage)
{ return GetTextData("CompanyName",dwLanguage); }

inline const char* AgVersionInfo::FileDescription(DWORD dwLanguage)
{ return GetTextData("FileDescription",dwLanguage); }

inline const char* AgVersionInfo::FileVersion(DWORD dwLanguage)
{ return GetTextData("FileVersion",dwLanguage); }

inline const char* AgVersionInfo::InternalName(DWORD dwLanguage)
{ return GetTextData("InternalName",dwLanguage); }

inline const char* AgVersionInfo::LegalCopyright(DWORD dwLanguage)
{ return GetTextData("LegalCopyright",dwLanguage); }

inline const char* AgVersionInfo::LegalTrademarks(DWORD dwLanguage)
{ return GetTextData("LegalTrademarks",dwLanguage); }

inline const char* AgVersionInfo::OriginalFileName(DWORD dwLanguage)
{ return GetTextData("OriginalFileName",dwLanguage); }

inline const char* AgVersionInfo::PrivateBuild(DWORD dwLanguage)
{ return GetTextData("PrivateBuild",dwLanguage); }

inline const char* AgVersionInfo::ProductName(DWORD dwLanguage)
{ return GetTextData("ProductName",dwLanguage); }

inline const char* AgVersionInfo::ProductVersion(DWORD dwLanguage)
{ return GetTextData("ProductVersion",dwLanguage); }

inline const char* AgVersionInfo::SpecialBuild(DWORD dwLanguage)
{ return GetTextData("SpecialBuild",dwLanguage); }

#endif //AG_USE_CHEATPROTECTION


#endif //__AG_VERSION_INFO_H__