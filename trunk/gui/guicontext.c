#include <windows.h>

extern HINSTANCE g_hInst;

BOOL WINAPI OnContextMenu(HWND hwnd, int x, int y, HMENU hmenu) {
	RECT rc;
	POINT pt = {x, y};
	
	GetClientRect(hwnd, &rc);
	ScreenToClient(hwnd, &pt);
	
	if (PtInRect(&rc, pt)) {
		ClientToScreen(hwnd, &pt);
		
		TrackPopupMenu(hmenu, 
            TPM_LEFTALIGN | TPM_RIGHTBUTTON, 
            pt.x, pt.y, 0, hwnd, NULL); 
 
	    // Destroy the menu. 
		return TRUE;
	}

	return FALSE;
}
