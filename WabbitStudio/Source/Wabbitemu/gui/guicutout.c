#include "stdafx.h"

#include "gui.h"
#include "guilcd.h"
#include "guiskin.h"
#include "guibuttons.h"
#include "guicutout.h"
#include "colorlcd.h"

#define IDC_SMALLCLOSE		45
#define IDC_SMALLMINIMIZE	46

#define DWMAPI_DLL _T("dwmapi.dll")
#define DWM_SET_WINDOW_ATTRIB "DwmSetWindowAttribute"

extern HINSTANCE g_hInst;

LRESULT CALLBACK SmallButtonProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	static BOOL fDown = FALSE;
	switch (uMsg)
	{
		case WM_CREATE: {
			LPMAINWINDOW lpMainWindow = (LPMAINWINDOW)((LPCREATESTRUCT)lParam)->lpCreateParams;
			SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)lpMainWindow);
			return 0;
		}

		case WM_PAINT: {
				LPCALC lpCalc = (LPCALC) GetWindowLongPtr(hwnd, GWLP_USERDATA);

				TCHAR szWindowName[256];
				GetWindowText(hwnd, szWindowName, ARRAYSIZE(szWindowName));

				PAINTSTRUCT ps = {0};
				HDC hdc = BeginPaint(hwnd, &ps);

				RECT rc;
				GetClientRect(hwnd, &rc);
				FillRect(hdc, &rc, GetStockBrush(WHITE_BRUSH));

				if (!lpCalc->bCutout)
					return 0;
			
				HBITMAP hbmButtons = LoadBitmap(g_hInst, _T("close"));
				HDC hdcButtons = CreateCompatibleDC(hdc);
				SelectObject(hdcButtons, hbmButtons);

				UINT col, row;
				col = 0;
				if (_tcsicmp(szWindowName, g_szSmallMinimize) == 0) {
					col = 13;
				}
				row = 0;
				if (fDown == TRUE) {
					row = 13;
				}

				RECT r;
				GetWindowRect(hwnd, &r);
				POINT p;
				p.x = r.left;
				p.y = r.top;

				ScreenToClient(hwnd, &p);
				BitBlt(hdc, 0, 0, 13, 13, lpCalc->hdcButtons, p.x, p.y, SRCCOPY);

				BLENDFUNCTION bf;
				bf.BlendOp = AC_SRC_OVER;
				bf.BlendFlags = 0;
				bf.SourceConstantAlpha = 160;
				bf.AlphaFormat = 0;
				AlphaBlend(hdc, 0, 0, 13, 13, hdcButtons, col, row, 13, 13, bf );

				DeleteDC(hdcButtons);
				DeleteObject(hbmButtons);

				EndPaint(hwnd, &ps);

				return 0;
		}

		case WM_LBUTTONDOWN: {
				SetCapture(hwnd);
				fDown = TRUE;
				InvalidateRect(hwnd, NULL, FALSE);
				UpdateWindow(hwnd);
				return 0;
		}
		case WM_LBUTTONUP: {
			LPMAINWINDOW lpMainWindow = (LPMAINWINDOW)GetWindowLongPtr(hwnd, GWLP_USERDATA);
			if (lpMainWindow == NULL) {
				break;
			}

			POINT pt;
			RECT rc;
			//make sure were still over the button
			GetCursorPos(&pt);
			ScreenToClient(hwnd, &pt);
			GetClientRect(hwnd, &rc);
			if (PtInRect(&rc, pt)) {
				TCHAR szWindowName[256];
				GetWindowText(hwnd, szWindowName, ARRAYSIZE(szWindowName));

				if (_tcsicmp(szWindowName, g_szSmallMinimize) == 0) {
					ShowWindow(lpMainWindow->hwndFrame, SW_MINIMIZE);
				} else if (_tcsicmp(szWindowName, g_szSmallClose) == 0) {
					SendMessage(lpMainWindow->hwndFrame, WM_CLOSE, 0, 0);
				}
			}
			ReleaseCapture();
			fDown = FALSE;
			InvalidateRect(hwnd, NULL, FALSE);
			UpdateWindow(hwnd);
			return 0;
		}
		case WM_KEYDOWN: {
			LPMAINWINDOW lpMainWindow = (LPMAINWINDOW)GetWindowLongPtr(hwnd, GWLP_USERDATA);
			if (lpMainWindow == NULL) {
				break;
			}

			HandleKeyDown(lpMainWindow, lpMainWindow->lpCalc, wParam);
			return 0;
		}
		case WM_KEYUP: {
			LPMAINWINDOW lpMainWindow = (LPMAINWINDOW)GetWindowLongPtr(hwnd, GWLP_USERDATA);
			if (lpMainWindow == NULL) {
				break;
			}

			HandleKeyUp(lpMainWindow, lpMainWindow->lpCalc , wParam);
			return 0;
		}
		case WM_CLOSE: {
			LPMAINWINDOW lpMainWindow = (LPMAINWINDOW)GetWindowLongPtr(hwnd, GWLP_USERDATA);
			if (lpMainWindow == NULL) {
				break;
			}

			SendMessage(lpMainWindow->hwndFrame, uMsg, wParam, lParam);
		}
		case WM_NCCALCSIZE:
			return 0;
		default:
			return DefWindowProc(hwnd, uMsg, wParam, lParam);
	}
}

void PositionLittleButtons(HWND hwnd)
{
	LPCALC lpCalc = (LPCALC) GetWindowLongPtr(hwnd, GWLP_USERDATA);
	HDWP hdwp = BeginDeferWindowPos(3);
	RECT wr;
	GetWindowRect(hwnd, &wr);
	DeferWindowPos(hdwp, lpCalc->hwndSmallMinimize, NULL, wr.right - 81, wr.top + 34, 13, 13, 0);
	DeferWindowPos(hdwp, lpCalc->hwndSmallClose, NULL, wr.right - 66, wr.top + 34, 13, 13, 0);
	EndDeferWindowPos(hdwp);
}

typedef HRESULT (WINAPI *SetThemeFunc)(HWND, LPCWSTR, LPCWSTR);
typedef HRESULT (WINAPI *DwmSetAttrib)(HWND, DWORD, LPCVOID, DWORD);

static LRESULT CALLBACK TestButtonProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch(uMsg) {
		case WM_CREATE:

			return FALSE;
		case WM_PAINT:

			return FALSE;
	}
	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

/* Using a layered window, make the frame window transparent.
 * Also create buttons to allow minimize and close while in skin mode
 */
int EnableCutout(LPMAINWINDOW lpMainWindow, LPCALC lpCalc) {
	if (!lpCalc || !lpCalc->bSkinEnabled) {
		return 1;
	}

	DwmSetAttrib SetAttrib = NULL;
	BOOL disableTransition = TRUE;

	BLENDFUNCTION bf;
	bf.BlendOp = AC_SRC_OVER;
	bf.BlendFlags = 0;
	bf.SourceConstantAlpha = 255;
	bf.AlphaFormat = AC_SRC_ALPHA;

	u_int width = lpCalc->rectSkin.right;
	u_int height = lpCalc->rectSkin.bottom;

	int scale = DEFAULT_SKIN_SCALE;

	if (!lpCalc->hwndLCD || GetParent(lpCalc->hwndLCD)) {
		RECT rc;
		GetClientRect(lpMainWindow->hwndFrame, &rc);
		DestroyWindow(lpCalc->hwndLCD);
		// don't initially show the window so we can disable DWM transitions
		lpCalc->hwndLCD = CreateWindowEx(
			0,
			g_szLCDName,
			_T("Wabbitemu"),
			0,
			0, 0, lpCalc->cpu.pio.lcd->width * scale, 64 * scale,
			lpMainWindow->hwndFrame, NULL, g_hInst, (LPVOID)lpMainWindow);
	}

	// still support XP so have to load this manually
	HMODULE hDwmModule = LoadLibrary(DWMAPI_DLL);
	if (hDwmModule) {
		SetAttrib = (DwmSetAttrib) GetProcAddress(hDwmModule, DWM_SET_WINDOW_ATTRIB);
		if (SetAttrib != NULL) {
			SetAttrib(lpCalc->hwndLCD, DWMWA_TRANSITIONS_FORCEDISABLED, &disableTransition, sizeof(BOOL));
		}
	}
	// show window with no transitions
	ShowWindow(lpCalc->hwndLCD, TRUE);
	SetWindowTheme(lpCalc->hwndLCD, L" ", L" ");

	RECT rc;
	GetClientRect(lpMainWindow->hwndFrame, &rc);
	POINT rectTopLeft;
	rectTopLeft.x = rc.left;
	rectTopLeft.y = rc.top;

	POINT ptSrc = {0 , 0};
	SIZE size;
	size.cx = (LONG)(width * lpCalc->skin_scale);
	size.cy = (LONG)(height * lpCalc->skin_scale);

	SetWindowLongPtr(lpMainWindow->hwndFrame, GWL_EXSTYLE, WS_EX_LAYERED);
	SetWindowLongPtr(lpMainWindow->hwndFrame, GWL_STYLE, WS_VISIBLE);

	HDC hScreen = GetDC(NULL);
	// resize the skin to be correct size
	HDC hdc = CreateCompatibleDC(hScreen);
	HBITMAP hBmp = CreateCompatibleBitmap(hScreen, size.cx, size.cy);
	HBITMAP hBmpOld = (HBITMAP)SelectObject(hdc, hBmp);
	AlphaBlend(hdc, 0, 0, size.cx, size.cy, lpCalc->hdcButtons, 0, 0, width, height, bf);
	BOOL done = UpdateLayeredWindow(lpMainWindow->hwndFrame, hScreen, NULL, &size, hdc, &ptSrc, RGB(255, 255, 255), &bf, ULW_ALPHA);
	DWORD error;
	if (!done) {
		error = GetLastError();
	}

	DeleteDC(hdc);
	ReleaseDC(NULL, hScreen);
	UpdateWindow(lpCalc->hwndLCD);

	BitBlt(lpCalc->hdcButtons, 0, 0, lpCalc->rectSkin.right, lpCalc->rectSkin.bottom, lpCalc->hdcSkin, 0, 0, SRCCOPY);

	// Create the two buttons that appear when the skin is cutout
	if (!lpCalc->hwndSmallClose) {
		lpCalc->hwndSmallClose = CreateWindow(
			g_szSmallButtonsName,
			g_szSmallClose,
			WS_POPUP,
			270, 19,
			13, 13,
			lpMainWindow->hwndFrame,
			(HMENU) NULL,
			g_hInst,
			(LPVOID)lpMainWindow);
		if (lpCalc->hwndSmallClose == NULL) {
			return 1;
		}
		if (SetAttrib) {
			SetAttrib(lpCalc->hwndSmallClose, DWMWA_TRANSITIONS_FORCEDISABLED, &disableTransition, sizeof(BOOL));
		}

		SetWindowLongPtr(lpCalc->hwndSmallClose, GWL_STYLE, WS_VISIBLE);
	}

	if (!lpCalc->hwndSmallMinimize) {
		lpCalc->hwndSmallMinimize = CreateWindowEx(
			0,
			g_szSmallButtonsName,
			g_szSmallMinimize,
			WS_POPUP,
			254, 19,
			13, 13,
			lpMainWindow->hwndFrame,
			(HMENU) NULL,
			g_hInst,
			(LPVOID)lpMainWindow);
		if (lpCalc->hwndSmallMinimize == NULL) {
			return 1;
		}
		if (SetAttrib) {
			SetAttrib(lpCalc->hwndSmallMinimize, DWMWA_TRANSITIONS_FORCEDISABLED, &disableTransition, sizeof(BOOL));
		}
		SetWindowLongPtr(lpCalc->hwndSmallMinimize, GWL_STYLE, WS_VISIBLE);
	}
	InvalidateRect(lpCalc->hwndSmallClose, NULL, FALSE);
	UpdateWindow(lpCalc->hwndSmallClose);
	InvalidateRect(lpCalc->hwndSmallMinimize, NULL, FALSE);
	UpdateWindow(lpCalc->hwndSmallMinimize);

	if (hDwmModule) {
		FreeLibrary(hDwmModule);
	}

	InvalidateRect(lpMainWindow->hwndFrame, NULL, TRUE);
	UpdateWindow(lpMainWindow->hwndFrame);
	
	return 0;
}

/* Remove the cutout region from the window and delete
 * the small minimize and close buttons
 */
int DisableCutout(LPMAINWINDOW lpMainWindow, LPCALC lpCalc) {
	// still support XP so have to load this manually
	HMODULE hDwmModule = LoadLibrary(DWMAPI_DLL);
	if (hDwmModule) {
		BOOL disableTransition = TRUE;
		DwmSetAttrib SetAttrib = (DwmSetAttrib) GetProcAddress(hDwmModule, DWM_SET_WINDOW_ATTRIB);
		if (SetAttrib != NULL) {
			SetAttrib(lpCalc->hwndLCD, DWMWA_TRANSITIONS_FORCEDISABLED, &disableTransition, sizeof(BOOL));
		}
		FreeLibrary(hDwmModule);
	}

	int scale = lpCalc->bSkinEnabled ? DEFAULT_SKIN_SCALE : lpCalc->scale;
	if (!lpCalc->hwndLCD || !GetParent(lpCalc->hwndLCD )) {
		int lcd_width = lpCalc->cpu.pio.lcd->display_width;
		int lcd_height = lpCalc->cpu.pio.lcd->height;
		DestroyWindow(lpCalc->hwndLCD);
		lpCalc->hwndLCD = CreateWindowEx(
			0,
			g_szLCDName,
			_T("LCD"),
			WS_VISIBLE | WS_CHILD,
			0, 0, lcd_width * scale, lcd_height *scale,
			lpMainWindow->hwndFrame, (HMENU)IDC_LCD, g_hInst, (LPVOID)lpMainWindow);
	}


	SetWindowLongPtr(lpMainWindow->hwndFrame, GWL_EXSTYLE, 0);
	SetWindowLongPtr(lpMainWindow->hwndFrame, GWL_STYLE, (WS_TILEDWINDOW | WS_VISIBLE | WS_CLIPCHILDREN) & ~(WS_MAXIMIZEBOX /* | WS_SIZEBOX */));

	if (lpCalc->hwndSmallClose) {
		DestroyWindow(lpCalc->hwndSmallClose);
		lpCalc->hwndSmallClose = NULL;
	}

	if (lpCalc->hwndSmallMinimize) {
		DestroyWindow(lpCalc->hwndSmallMinimize);
		lpCalc->hwndSmallMinimize = NULL;
	}

	InvalidateRect(lpMainWindow->hwndFrame, NULL, TRUE);
	return 0;
}
