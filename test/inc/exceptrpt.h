/*
 *
 */

#ifndef __EXCEPTRPT_H__
#define __EXCEPTRPT_H__

#ifdef _WIN32

#include "dbghelp.h"

#ifndef _IA64
#ifndef DWORD_PTR
typedef unsigned long	ULONG_PTR;				// added by kskim. 2005.11.30
typedef unsigned long	DWORD_PTR, *PDWORD_PTR;
#endif
#endif

enum BasicType  // Stolen from CVCONST.H in the DIA 2.0 SDK
{
    btNoType = 0,
    btVoid = 1,
    btChar = 2,
    btWChar = 3,
    btInt = 6,
    btUInt = 7,
    btFloat = 8,
    btBCD = 9,
    btBool = 10,
    btLong = 13,
    btULong = 14,
    btCurrency = 25,
    btDate = 26,
    btVariant = 27,
    btComplex = 28,
    btBit = 29,
    btBSTR = 30,
    btHresult = 31
};

/////////////////////////////////////////////////////////////////////////////
// dbhelp.dll에 구현된 함수 목록
//

typedef BOOL (CALLBACK* LPFN_SymCleanup)(IN HANDLE hProcess);
typedef BOOL (CALLBACK* LPFN_SymEnumSymbols)(
				IN HANDLE                       hProcess,
				IN ULONG64                      BaseOfDll,
				IN PCSTR                        Mask,
				IN PSYM_ENUMERATESYMBOLS_CALLBACK    EnumSymbolsCallback,
				IN PVOID                        UserContext
				);
typedef BOOL (CALLBACK* LPFN_SymInitialize)(
				IN HANDLE   hProcess,
				IN PCSTR    UserSearchPath,
				IN BOOL     fInvadeProcess
				);
typedef DWORD (CALLBACK* LPFN_SymSetOptions)(IN DWORD SymOptions);

typedef BOOL (CALLBACK* LPFN_SymSetContext)(
				HANDLE hProcess,
				PIMAGEHLP_STACK_FRAME StackFrame,
				PIMAGEHLP_CONTEXT Context
				);

typedef BOOL (CALLBACK* LPFN_SymGetLineFromAddr)(
				IN  HANDLE                hProcess,
				IN  DWORD                 dwAddr,
				OUT PDWORD                pdwDisplacement,
				OUT PIMAGEHLP_LINE        Line
				);

typedef BOOL (CALLBACK* LPFN_SymFromAddr)(
				IN  HANDLE              hProcess,
				IN  DWORD64             Address,
				OUT PDWORD64            Displacement,
				IN OUT PSYMBOL_INFO     Symbol
				);

typedef BOOL (CALLBACK* LPFN_StackWalk)(
				DWORD                             MachineType,
				HANDLE                            hProcess,
				HANDLE                            hThread,
				LPSTACKFRAME                      StackFrame,
				PVOID                             ContextRecord,
				PREAD_PROCESS_MEMORY_ROUTINE      ReadMemoryRoutine,
				PFUNCTION_TABLE_ACCESS_ROUTINE    FunctionTableAccessRoutine,
				PGET_MODULE_BASE_ROUTINE          GetModuleBaseRoutine,
				PTRANSLATE_ADDRESS_ROUTINE        TranslateAddress
				);
typedef PVOID (CALLBACK* LPFN_SymFunctionTableAccess)(
				HANDLE  hProcess,
				DWORD   AddrBase
				);

typedef DWORD (CALLBACK* LPFN_SymGetModuleBase)(
				IN  HANDLE              hProcess,
				IN  DWORD               dwAddr
				);

typedef BOOL (CALLBACK* LPFN_SymGetTypeInfo)(
				IN  HANDLE          hProcess,
				IN  DWORD64         ModBase,
				IN  ULONG           TypeId,
				IN  IMAGEHLP_SYMBOL_TYPE_INFO GetType,
				OUT PVOID           pInfo
				);
/////////////////////////////////////////////////////////////////////////////

class CExceptRpt
{
    public:
    
    CExceptRpt( );
    ~CExceptRpt( );
    
    void SetLogFileName( PTSTR pszLogFileName );


    // entry point where control comes on an unhandled exception
    static LONG WINAPI UnhandledExceptionFilter(
                                PEXCEPTION_POINTERS pExceptionInfo );

    private:

    // where report info is extracted and generated 
    static void GenerateExceptionReport( PEXCEPTION_POINTERS pExceptionInfo );

    // Helper functions
    static LPTSTR GetExceptionString( DWORD dwCode );
    static BOOL GetLogicalAddress(  PVOID addr, PTSTR szModule, DWORD len,
                                    DWORD& section, DWORD& offset );

    static void WriteStackDetails( PCONTEXT pContext, bool bWriteVariables );

    static BOOL CALLBACK EnumerateSymbolsCallback(PSYMBOL_INFO,ULONG, PVOID);

    static bool FormatSymbolValue( PSYMBOL_INFO, STACKFRAME *, char * pszBuffer, unsigned cbBuffer );

    static char * DumpTypeIndex( char *, DWORD64, DWORD, unsigned, DWORD_PTR, bool & );

    static char * FormatOutputValue( char * pszCurrBuffer, BasicType basicType, DWORD64 length, PVOID pAddress );
    
    static BasicType GetBasicType( DWORD typeIndex, DWORD64 modBase );

    static int __cdecl _tprintf(const TCHAR * format, ...);

    // Variables used by the class
    static TCHAR m_szLogFileName[MAX_PATH];
    static LPTOP_LEVEL_EXCEPTION_FILTER m_previousFilter;
    static HANDLE  m_hReportFile;
    static HANDLE  m_hProcess;

	static HMODULE m_hHelp;
	static HMODULE m_hImgHelp;

	// dbhelp.dll에서 로드한 함수들
	static LPFN_SymCleanup				SymCleanup;
	static LPFN_SymEnumSymbols			SymEnumSymbols;
	static LPFN_SymInitialize			SymInitialize;
	static LPFN_SymSetOptions			SymSetOptions;
	static LPFN_SymSetContext			SymSetContext;
	static LPFN_SymFromAddr				SymFromAddr;
	static LPFN_StackWalk				StackWalk;
	static LPFN_SymGetModuleBase		SymGetModuleBase;
	static LPFN_SymGetTypeInfo			SymGetTypeInfo;
	static LPFN_SymGetLineFromAddr		SymGetLineFromAddr;
	static LPFN_SymFunctionTableAccess	SymFunctionTableAccess;
};

extern CExceptRpt g_ExceptRpt; //  global instance of class

#endif		// _WIN32

#endif		// __EXCEPTRPT_H__
