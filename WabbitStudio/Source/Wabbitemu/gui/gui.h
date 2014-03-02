#ifndef GUI_H
#define GUI_H

#include "calc.h"
#include "guicommandline.h"
#include "DropTarget.h"

class CWabbitemu;

#define MIN_SKIN_WIDTH	290
#define MIN_SKIN_HEIGHT	600

#define MAX_SKIN_WIDTH	520
#define MAX_SKIN_HEIGHT	1077

#define SKIN_WIDTH	350
#define SKIN_HEIGHT	725

#define WABBITVER		_T("1.8")

#define g_szWebPage		_T("http://wabbit.codeplex.com")
#ifdef _WIN64
#define g_szDownload	_T("http://buckeyedude.zapto.org/Revsoft/Wabbitemu/x64/Wabbitemu.exe")
#else
#define g_szDownload	_T("http://buckeyedude.zapto.org/Revsoft/Wabbitemu/Wabbitemu.exe")
#endif
#define g_szVersionFile "http://buckeyedude.zapto.org/Revsoft/Wabbitemu/Version.txt"
#define g_szWhatsNewFile "http://buckeyedude.zapto.org/Revsoft/Wabbitemu/WhatsNew.txt"

#define g_szAppName 	_T("z80")
#define g_szDebugName 	_T("debug")
#define g_szDisasmName 	_T("disasm")
#define g_szRegName 	_T("reg")
#define g_szMemName		_T("mem")
#define g_szWatchName	_T("watchpoints")
#define g_szPortMonitor	_T("portmonitor")
#define g_szLCDMonitor	_T("lcdmonitor")
#define g_szLinkName	_T("link")
#define g_szLCDName 	_T("wabbitlcd")
#define g_szWabbitName 	_T("wabbit")
#define g_szToolbar		_T("wabtool")
#define g_szSubbar		_T("wabsub")
#define g_szDetachedName 	_T("wabbitdetached")
#define g_szDetachedLCDName _T("wabbitdetachedlcd")
#define g_szSmallButtonsName _T("wabbitsmallbutton")

#define ID_STATUSBAR	50


#define REG_SWAPID		80
#define REG_CHK_Z		88
#define REG_CHK_C		89
#define REG_CHK_S		90
#define REG_CHK_PV		91
#define REG_CHK_HC		92
#define REG_CHK_N		93
#define REG_CHK_HALT	94
#define REG_CHK_IFF1	95
#define REG_CHK_IFF2	96

#define DB_CREATE		0
#define DB_UPDATE		1
#define DB_RESUME       2
#define DB_GOTO_RESULT	3
#define DB_SPRITE_CLOSE 3

#define REG_UPDATE		1


struct key_string {
	TCHAR *text;
	int group;
	int bit;
	struct key_string *next;
};

typedef struct MainWindow {
	LPCALC lpCalc;

	HWND hwndFrame;
	HWND hwndLCD;
	HWND hwndDetachedFrame;
	HWND hwndDetachedLCD;
	HWND hwndStatusBar;
	HWND hwndSmallClose;
	HWND hwndSmallMinimize;
	HWND hwndKeyListDialog;
	HWND hwndDebug;

	HDC hdcSkin;
	HDC hdcButtons;
	HDC hdcKeymap;

	RECT rectSkin;
	RECT rectLCD;
	COLORREF FaceplateColor;
	// custom skin path
	TCHAR skin_path[256];
	// custom keymap path
	TCHAR keymap_path[256];

	key_string *last_keypress_head;
	int num_keypresses;

	DWORD scale;
	double default_skin_scale;
	double skin_scale;
	BOOL bCutout;
	BOOL bSkinEnabled;
	BOOL running;
	BOOL bDoDrag;
	BOOL bCustomSkin;
	BOOL bAlwaysOnTop;
	BOOL bAlphaBlendLCD;

	gif_disp_state gif_disp_state;
	int GIFGradientWidth = 1;
	int GIFAdd = 1;

	// used for fps counter
	clock_t sb_refresh;
	// frame context menu location
	POINT ctxtPt;

	CWabbitemu *pWabbitemu;
	CDropTarget *pDropTarget;
	BOOL is_archive_only;
	BOOL is_calc_file;

	BOOL bTIOSDebug;
} MainWindow_t, *LPMAINWINDOW;

HWND gui_debug_hwnd(LPMAINWINDOW lpMainWindow);
LPMAINWINDOW gui_frame(LPCALC lpCalc);
void GetFileCurrentVersionString(TCHAR *buf, size_t len);
LPMAINWINDOW create_calc_frame_register_events();
extern HACCEL haccelmain;

void RegisterWindowClasses(void);
VOID CALLBACK TimerProc(HWND hwnd, UINT Message, UINT_PTR idEvent, DWORD dwTimer);

#ifdef _WINDLL
class CWabbitemuModule : public CAtlDllModuleT < CWabbitemuModule >
#else
class CWabbitemuModule : public CAtlExeModuleT< CWabbitemuModule >
#endif
{
public :
	DECLARE_LIBID(LIBID_WabbitemuLib)

	LPMAINWINDOW CreateNewFrame(LPCALC lpCalc);
	void DestroyFrame(LPMAINWINDOW lpMainWindow);
	HWND CheckValidFrameHandle(HWND hwndToCheck);
	HWND CheckValidOtherHandle(HWND hwndToCheck);
	HWND CheckValidDebugHandle(HWND hwndToCheck);

	void CWabbitemuModule::SetGIFState(gif_disp_state state);

	bool ParseCommandLine(LPCTSTR lpCmdLine, HRESULT* pnRetCode);

	HRESULT PreMessageLoop(int nShowCmd);
	void RunMessageLoop();
	HRESULT PostMessageLoop();

	ParsedCmdArgs *GetParsedCmdArgs() {
		return &m_parsedArgs;
	}

private:
	vector<LPMAINWINDOW> m_lpMainWindows;
	ULONG_PTR m_gdiplusToken;
	ParsedCmdArgs m_parsedArgs;
};

extern CWabbitemuModule _Module;

#endif
