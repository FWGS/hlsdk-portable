//++ BulliT

#ifdef AG_USE_MINIDUMP

#ifdef CLIENT_DLL //Only check problems on client...
#define DUMPNAME "agclient.dmp"
#include "hud.h"
#include "cl_util.h"
#include "agglobal.h"

/*
#else
#define DUMPNAME "agserver.dmp"
#include "agglobal.h"
#endif
*/

#define VC_EXTRALEAN
#define WIN32_LEAN_AND_MEAN
#define NOWINRES
#define NOSERVICE
#define NOMCX
#define NOIME
#include <windows.h>
#include <stdio.h>

#if _MSC_VER < 1300
#define DECLSPEC_DEPRECATED
// VC6: change this path to your Platform SDK headers
#include "dbghelp.h"			// must be XP version of file
#else
// VC7: ships with updated headers
#include "dbghelp.h"
#endif

// based on dbghelp.h
typedef BOOL (WINAPI *MINIDUMPWRITEDUMP)(HANDLE hProcess, DWORD dwPid, HANDLE hFile, MINIDUMP_TYPE DumpType,
									CONST PMINIDUMP_EXCEPTION_INFORMATION ExceptionParam,
									CONST PMINIDUMP_USER_STREAM_INFORMATION UserStreamParam,
									CONST PMINIDUMP_CALLBACK_INFORMATION CallbackParam
									);
	
static LONG WINAPI TopLevelFilter( struct _EXCEPTION_POINTERS *pExceptionInfo )
{
	LONG retval = EXCEPTION_EXECUTE_HANDLER;//EXCEPTION_CONTINUE_SEARCH;
	HWND hParent = NULL;						// find a better value for your app

	// firstly see if dbghelp.dll is around and has the function we need
	// look next to the EXE first, as the one in System32 might be old 
	// (e.g. Windows 2000)
	HMODULE hDll = NULL;
	char szDbgHelpPath[MAX_PATH];
	char szMiniDump[MAX_PATH];

	if (GetModuleFileName( GetModuleHandle("client.dll"), szDbgHelpPath, MAX_PATH ))
	{
		char *pSlash = strrchr( szDbgHelpPath, '\\' );
		if (pSlash)
		{
			strcpy( pSlash+1, "DBGHELP.DLL" );
			hDll = ::LoadLibrary( szDbgHelpPath );
		}
	}

	if (GetModuleFileName( GetModuleHandle("client.dll"), szMiniDump, MAX_PATH ))
	{
		char *pSlash = strrchr( szMiniDump, '\\' );
		if (pSlash)
		{
			strcpy( pSlash+1, DUMPNAME );
		}
	}

	if (hDll==NULL)
	{
		// load any version we can
		hDll = ::LoadLibrary( "DBGHELP.DLL" );
	}

	LPCTSTR szResult = NULL;

	if (hDll)
	{
		MINIDUMPWRITEDUMP pDump = (MINIDUMPWRITEDUMP)::GetProcAddress( hDll, "MiniDumpWriteDump" );
		if (pDump)
		{
			char szScratch [MAX_PATH];

			// create the file
			HANDLE hFile = ::CreateFile( szMiniDump, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS,
										FILE_ATTRIBUTE_NORMAL, NULL );

			if (hFile!=INVALID_HANDLE_VALUE)
			{
				_MINIDUMP_EXCEPTION_INFORMATION ExInfo;

				ExInfo.ThreadId = ::GetCurrentThreadId();
				ExInfo.ExceptionPointers = pExceptionInfo;
				ExInfo.ClientPointers = NULL;

				// write the dump
				BOOL bOK = pDump( GetCurrentProcess(), GetCurrentProcessId(), hFile, MiniDumpNormal, &ExInfo, NULL, NULL );
				if (bOK)
				{
					sprintf( szScratch, "Saved dump file to '%s'", szMiniDump );
					szResult = szScratch;
					//retval = EXCEPTION_EXECUTE_HANDLER;
				}
				else
				{
					sprintf( szScratch, "Failed to save dump file to '%s' (error %d)", szMiniDump, GetLastError() );
					szResult = szScratch;
				}
				::CloseHandle(hFile);
			}
			else
			{
				sprintf( szScratch, "Failed to create dump file '%s' (error %d)", szMiniDump, GetLastError() );
				szResult = szScratch;
			}
		}
		else
		{
			szResult = "DBGHELP.DLL too old";
		}
	}
	else
	{
		szResult = "DBGHELP.DLL not found";
	}

	AgLog(szResult);

	return retval;
}

class AgMiniDump
{
public:
	AgMiniDump()
	{
		::SetUnhandledExceptionFilter( TopLevelFilter );
	};
};

AgMiniDump g_MiniDump; //The dumper.

#endif CLIENT_DLL

#endif AG_USE_MINIDUMP
//-- Martin Webrant
