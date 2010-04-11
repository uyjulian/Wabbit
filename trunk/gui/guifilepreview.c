#include <windows.h>
#include "guifilepreview.h"
#include "rsrc.h"
#include "lcd.h"
#include "savestate.h"

extern HINSTANCE g_hInst;
extern BITMAPINFO *bi;

static HWND grpSettings;
static OFNHookOptions *HookOptions;


static LRESULT CALLBACK TargetMemProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam) {
	static HWND rbnRAM, rbnArchive, rbnFileSettings;
	
	static int nIDDefault = IDC_RBNFILESETTINGS;
	
	switch (Message) {
		case WM_INITDIALOG: 
		{
			rbnRAM = GetDlgItem(hwnd, IDC_RBNFILERAM);
			rbnArchive = GetDlgItem(hwnd, IDC_RBNFILEARCHIVE);
			rbnFileSettings = GetDlgItem(hwnd, IDC_RBNFILESETTINGS);
			
			CheckRadioButton(hwnd, IDC_RBNFILESETTINGS, IDC_RBNFILERAM, nIDDefault);
			
			SendMessage(hwnd, WM_SIZE, 0 ,0);
			return TRUE;
		}
		case WM_SIZE: 
		{
			RECT r;
			GetClientRect(hwnd, &r);
			
			HWND hwndParent = GetParent(hwnd);
			RECT rp;
			GetClientRect(hwndParent, &rp);
			
			int x = 0;
			int y = (rp.bottom - r.bottom);
			
			SetWindowPos(hwnd, NULL, x, y, 0, 0, SWP_NOZORDER | SWP_NOSIZE);
			return TRUE;
		}
		case WM_COMMAND:
		{
			/* Set the options once a file is selected */
			if (IsDlgButtonChecked(hwnd, IDC_RBNFILERAM) == BST_CHECKED) {
				HookOptions->bArchive = FALSE;
				HookOptions->bFileSettings = FALSE;
				nIDDefault = IDC_RBNFILERAM;
			} else
			if (IsDlgButtonChecked(hwnd, IDC_RBNFILEARCHIVE) == BST_CHECKED) {
				HookOptions->bArchive = TRUE;
				HookOptions->bFileSettings = FALSE;
				nIDDefault = IDC_RBNFILEARCHIVE;
			} else {
				HookOptions->bFileSettings = TRUE;
				nIDDefault = IDC_RBNFILESETTINGS;
			}
			break;
		}
		
	}
	return FALSE;
}
static LRESULT CALLBACK FilePreviewPaneProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam) {
	static HWND imgPreview, edtAuthor, edtComment;
	static HBITMAP hbmPreview;
	
	switch (Message) {
		case WM_INITDIALOG:
	
			imgPreview = GetDlgItem(hwnd, IDC_IMGPREVIEW);
			edtAuthor = GetDlgItem(hwnd, IDC_EDTAUTHOR);
			edtComment = GetDlgItem(hwnd, IDC_EDTCOMMENT);
			
			RECT r;
			GetClientRect(hwnd, &r);
			// Center the preview image
			SetWindowPos(imgPreview, NULL, (r.right-192)/2, 14, 192, 128, SWP_NOZORDER);
			
			hbmPreview = CreateBitmap(192, 128, 1, 32, NULL);
	
			return TRUE;
		case WM_SIZE:
			SetWindowPos(hwnd, NULL, 0, 0, LOWORD(lParam), HIWORD(lParam), SWP_NOZORDER | SWP_NOMOVE);
			SendMessage(grpSettings, WM_SIZE, wParam, lParam);
			return TRUE;
		case WM_DESTROY:
			DeleteObject(hbmPreview);
			return FALSE;
		case WM_USER:
		{
			if (lParam == 0) goto NoFilePreview;
			
			FILE *prgFile = fopen((char*) lParam, "rb");
			if (!prgFile) goto NoFilePreview;
	
			SAVESTATE_t *save = ReadSave(prgFile);
			if (!save) goto NoFilePreviewNotSave;
			
			LCD_t lcd;
			LoadLCD(save, &lcd);
			
			SendMessage(edtAuthor, WM_SETTEXT, 0, (LPARAM) save->author);
			SendMessage(edtComment, WM_SETTEXT, 0, (LPARAM) save->comment);
			
			HDC hdc = CreateCompatibleDC(NULL);
			HBITMAP hbmOld = SelectObject(hdc, hbmPreview);
			
			StretchDIBits(hdc, 0, 0, 192, 128,
				0, 0, 96, 64,
				LCD_image(&lcd),
				bi,
				DIB_RGB_COLORS,
				SRCCOPY);
	
			SelectObject(hdc, hbmOld);
			DeleteDC(hdc);
			SendMessage(imgPreview, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM) hbmPreview);
			fclose(prgFile);
			return S_OK;
		NoFilePreviewNotSave:
			fclose(prgFile);
		NoFilePreview:
			SendMessage(edtAuthor, WM_SETTEXT, 0, (LPARAM) "");
			SendMessage(edtComment, WM_SETTEXT, 0, (LPARAM) "");
			
			hdc = CreateCompatibleDC(NULL);
			hbmOld = SelectObject(hdc, hbmPreview);
			RECT r = {0, 0, 192, 168};
			FillRect(hdc, &r, GetStockObject(LTGRAY_BRUSH));
			SelectObject(hdc, hbmOld);
			DeleteDC(hdc);
			SendMessage(imgPreview, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM) hbmPreview);
			return S_OK;
		}
	}
	return FALSE;
}

HWND CreateFilePreviewPane(HWND hwndParent, int x, int y, int cx, int cy, OFNHookOptions *lParam) {
	HWND hwnd = CreateDialog(g_hInst, MAKEINTRESOURCE(IDD_OFN), hwndParent, (DLGPROC) FilePreviewPaneProc);
	SetWindowPos(hwnd, NULL, x, y, cx, cy, SWP_NOZORDER);

	HookOptions = lParam;
	
	grpSettings = CreateDialog(g_hInst, MAKEINTRESOURCE(IDD_DLGTARGETMEM), hwnd, (DLGPROC) TargetMemProc);
	return hwnd;
}
	

LRESULT ShowFilePreview(HWND hwndPreview, LPSTR szPath) {
	return SendMessage(hwndPreview, WM_USER, 0, (LPARAM) szPath);
}
