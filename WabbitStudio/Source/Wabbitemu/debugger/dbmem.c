#include "stdafx.h"

#include "core.h"
#include "dbmem.h"
#include "resource.h"
#include "registry.h"
#include "guidebug.h"

#define COLUMN_X_OFFSET (7 + (mps->type == REGULAR ? 0 : 3))

extern HWND hwndLastFocus;
extern HINSTANCE g_hInst;
extern unsigned int goto_addr;
extern int find_value;
extern HFONT hfontLucida, hfontLucidaBold, hfontSegoe;

static int AddrFromPoint(HWND hwnd, POINT pt, RECT *r) {
	mp_settings *mps = (mp_settings *) GetWindowLongPtr(hwnd, GWLP_USERDATA);
	RECT hr;
	GetWindowRect(mps->hwndHeader, &hr);

	int cyHeader = hr.bottom - hr.top;

	TEXTMETRIC tm;
	HDC hdc = GetDC(hwnd);
	SelectObject(hdc, hfontLucida);
	GetTextMetrics(hdc, &tm);
	ReleaseDC(hwnd, hdc);

	int sidebar_width = (7 + (mps->type == REGULAR ? 0 : 3));
	int x = pt.x - tm.tmAveCharWidth * sidebar_width - COLUMN_X_OFFSET - (int) (mps->diff - mps->cxMem - 2 * tm.tmAveCharWidth) / 2;
	int y = pt.y - cyHeader;

	int row = y/mps->cyRow;
	int col = (int) (x/mps->diff);

	int addr = mps->addr + ((col + (mps->nCols * row)) * mps->mode);

	if (r != NULL) {
		r->left = tm.tmAveCharWidth * sidebar_width + COLUMN_X_OFFSET + (int) (mps->diff * col);
		r->right = r->left + mps->cxMem - 2*tm.tmAveCharWidth;
		r->top = cyHeader + (row*mps->cyRow);
		r->bottom = r->top + tm.tmHeight;
	}

	return addr;
}

static int GetMaxAddr(mempane_settings *mps) {
	switch (mps->type) {
		case REGULAR:
			return 0x10000;
		case FLASH:
			return lpDebuggerCalc->cpu.mem_c->flash_size;
		case RAM:
			return lpDebuggerCalc->cpu.mem_c->ram_size;
	}
	return NULL;
}


static void ScrollUp(HWND hwnd) {
	mp_settings *mps = (mp_settings*) GetWindowLongPtr(hwnd, GWLP_USERDATA);

	if (mps->addr > 0) {
		int temp = mps->addr - mps->nCols * mps->mode;
		if (temp > 0)
			mps->addr = temp;
		else
			mps->addr = 0;
	}

	InvalidateRect(hwnd, NULL, FALSE);
	UpdateWindow(hwnd);
}

static void ScrollDown(HWND hwnd) {
	mp_settings *mps = (mp_settings*) GetWindowLongPtr(hwnd, GWLP_USERDATA);

	int data_length = mps->cyRow * mps->nRows * mps->mode;
	if (mps->addr + data_length - mps->cyRow * mps->mode <= GetMaxAddr(mps))
		mps->addr += mps->nCols * mps->mode;

	InvalidateRect(hwnd, NULL, FALSE);
	UpdateWindow(hwnd);
}

static VALUE_FORMAT GetValueFormat(mp_settings *mps) {
	VALUE_FORMAT format = CHAR1;
	if (mps->bText)
		return format;
	switch (mps->mode) {
	case 1:
		switch (mps->display) {
			case HEX:
				format = HEX2;
				break;
			case DEC:
				format = DEC3;
				break;
			case BIN:
				format = BIN8;
				break;
		}
		break;
	case 2:
		switch (mps->display) {
			case HEX:
				format = HEX4;
				break;
			case DEC:
				format = DEC3;
				break;
			case BIN:
				format = BIN16;
				break;
		}
		break;
	}
	return format;
}

static waddr_t GetWaddr(mempane_settings *mps, int addr) {
	waddr_t waddr;
	switch (mps->type) {
		case REGULAR:
			return addr_to_waddr(lpDebuggerCalc->cpu.mem_c, addr);
		case FLASH:
			waddr.addr = addr % PAGE_SIZE;
			waddr.is_ram = FALSE;
			waddr.page = addr / PAGE_SIZE;
			break;
		case RAM:
			waddr.addr = addr % PAGE_SIZE;
			waddr.is_ram = TRUE;
			waddr.page = addr / PAGE_SIZE;
			break;
	}
	return waddr;
}


LRESULT CALLBACK MemProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam) {
	static TEXTMETRIC tm;
	static HWND hwndVal;
	int kMemWidth;
	static int cyHeader;
	switch (Message) {
		case WM_SETFOCUS:
			hwndLastFocus = hwnd;
			return 0;
		case WM_CREATE:
		{
			mp_settings *mps;
			RECT rc;
			GetClientRect(hwnd, &rc);

			HDC hdc = GetDC(hwnd);
			SelectObject(hdc, hfontLucida);
			GetTextMetrics(hdc, &tm);

			mps = (mp_settings *) ((CREATESTRUCT*)lParam)->lpCreateParams;
			SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR) mps);

			SelectObject(hdc, hfontSegoe);

			InitCommonControls();

			mps->hwndHeader = CreateWindowEx(0, WC_HEADER, (LPCTSTR) NULL,
				WS_CHILD | WS_VISIBLE | HDS_HORZ | WS_CLIPSIBLINGS,
				0, 0, 1, 1, hwnd, (HMENU) ID_SIZE, g_hInst,
				(LPVOID) NULL);

			SetWindowFont(mps->hwndHeader, hfontSegoe, TRUE);

			WINDOWPOS wp;
			HDLAYOUT hdl;

			hdl.prc = &rc;
			hdl.pwpos = &wp;
			SendMessage(mps->hwndHeader, HDM_LAYOUT, 0, (LPARAM) &hdl);
			SetWindowPos(mps->hwndHeader, wp.hwndInsertAfter, wp.x, wp.y,
				wp.cx, wp.cy, wp.flags);

			HDITEM hdi;

			hdi.mask = HDI_TEXT | HDI_FORMAT | HDI_WIDTH;
			TCHAR pszText[32];
			GetWindowText(hwnd, pszText, sizeof(pszText));
			hdi.pszText = pszText;
			hdi.cxy = 1;
			hdi.cchTextMax = sizeof(hdi.pszText)/sizeof(hdi.pszText[0]);
			hdi.fmt = HDF_LEFT | HDF_STRING;
			mps->iData = 1;

			Header_InsertItem(mps->hwndHeader, 0, &hdi);


			hdi.pszText = _T("Addr");
			hdi.cxy = tm.tmAveCharWidth*7;
			hdi.cchTextMax = sizeof(hdi.pszText)/sizeof(hdi.pszText[0]);
			mps->iAddr = 0;

			Header_InsertItem(mps->hwndHeader, 0, &hdi);
			mps->hwndTip = CreateWindow(
					TOOLTIPS_CLASS,
					NULL, WS_POPUP | TTS_ALWAYSTIP | TTS_NOPREFIX,
					CW_USEDEFAULT, CW_USEDEFAULT,
					CW_USEDEFAULT, CW_USEDEFAULT,
					hwnd, NULL, g_hInst, NULL);
			SendMessage(mps->hwndTip, TTM_ACTIVATE, TRUE, 0);

			TOOLINFO toolInfo = {0};
			toolInfo.cbSize = sizeof(toolInfo);
			toolInfo.hwnd = hwnd;
			toolInfo.uFlags = TTF_IDISHWND | TTF_SUBCLASS;
			toolInfo.uId = (UINT_PTR) hwnd;
			toolInfo.lpszText = LPSTR_TEXTCALLBACK;
			SendMessage(mps->hwndTip, TTM_ADDTOOL, 0, (LPARAM)&toolInfo);


			RECT r;
			GetWindowRect(mps->hwndHeader, &r);
			cyHeader = r.bottom - r.top;

			mps->hfontData = hfontLucida;
			mps->hfontAddr = hfontLucidaBold;

			mps->cyRow = 4 * tm.tmHeight / 3;
			SendMessage(hwnd, WM_SIZE, 0, 0);

			//we need to get the registry-stored address the user was last on
			TCHAR buffer[64];
			StringCbPrintf(buffer, sizeof(buffer), _T("Mem%i"), mps->memNum);
			int value = (int) QueryDebugKey(buffer);
			mps->addr = value;
			return 0;
		}
		case WM_SIZE:
		{
			RECT rc;
			mp_settings *mps = (mp_settings*) GetWindowLongPtr(hwnd, GWLP_USERDATA);
			GetClientRect(hwnd, &rc);

			WINDOWPOS wp;
			HDLAYOUT hdl;

			hdl.prc = &rc;
			hdl.pwpos = &wp;
			SendMessage(mps->hwndHeader, HDM_LAYOUT, 0, (LPARAM) &hdl);
			SetWindowPos(mps->hwndHeader, wp.hwndInsertAfter, wp.x, wp.y,
				wp.cx, wp.cy, wp.flags);

			TCHAR szHeader[64];
			StringCbPrintf(szHeader, sizeof(szHeader), _T("Memory (%d Columns)"), mps->nCols);
			HDITEM hdi;
			hdi.mask = HDI_WIDTH;
			//| HDI_TEXT;
			hdi.pszText = szHeader;
			hdi.cxy = rc.right - tm.tmAveCharWidth*6;
			Header_SetItem(mps->hwndHeader, mps->iData, &hdi);

			return 0;
		}
		case WM_MOUSEACTIVATE:
			SetFocus(hwnd);
			return 0;
		case WM_PAINT:
		{
			HDC hdc, hdcDest;
			PAINTSTRUCT ps;
			RECT r, dr;
			GetClientRect(hwnd, &r);

			hdcDest = BeginPaint(hwnd, &ps);

			mp_settings *mps = (mp_settings *) GetWindowLongPtr(hwnd, GWLP_USERDATA);

			hdc = CreateCompatibleDC(hdcDest);
			HBITMAP hbm = CreateCompatibleBitmap(hdcDest, r.right, r.bottom);
			SelectObject(hdc, hbm);
			SetBkMode(hdc, TRANSPARENT);

			r.top = cyHeader;
			FillRect(hdc, &r, GetStockBrush(WHITE_BRUSH));

			SelectObject(hdc, GetStockObject(DC_PEN));
			SetDCPenColor(hdc, GetSysColor(COLOR_BTNFACE));

			BOOL isBinary = FALSE;
			TCHAR memfmt[8];
			//calculate how our data should be presented to the user
			VALUE_FORMAT format = GetValueFormat(mps);
			switch (format) {
				case DEC3:
				case DEC5:
					StringCbPrintf(memfmt, sizeof(memfmt), _T("%%d"), mps->mode * 2);
					kMemWidth = tm.tmAveCharWidth * mps->mode * 3;
					break;
				case HEX2:
				case HEX4:
					StringCbPrintf(memfmt, sizeof(memfmt), _T("%%0%dx"), mps->mode * 2);
					kMemWidth = tm.tmAveCharWidth * mps->mode * 2;
					break;
				case BIN8:
				case BIN16:
					isBinary = TRUE;
					kMemWidth = tm.tmAveCharWidth * mps->mode * 8;
					break;
				case CHAR1:
					StringCbCopy(memfmt, sizeof(memfmt), _T("%c"));
					kMemWidth = tm.tmAveCharWidth;
					break;
			}

			int sidebar_width = (7 + (mps->type == REGULAR ? 0 : 3));
			MoveToEx(hdc, tm.tmAveCharWidth * sidebar_width - 1, cyHeader, NULL);
			LineTo(hdc, tm.tmAveCharWidth * sidebar_width - 1, r.bottom);

			int i, j, 	rows = (r.bottom - r.top + mps->cyRow - 1)/mps->cyRow,
						cols =
						(r.right - r.left - tm.tmAveCharWidth * sidebar_width) /
						(kMemWidth + 2 * tm.tmAveCharWidth);

			mps->nRows = rows;
			mps->nCols = cols;
			mps->cxMem = kMemWidth + 2 * tm.tmAveCharWidth;

			double diff = r.right - r.left - tm.tmAveCharWidth * sidebar_width - (cols * mps->cxMem);
			if (cols != 1) {
				diff /= cols - 1;
			}

			diff += mps->cxMem;
			mps->diff = diff;
			int addr = mps->addr;

			unsigned int value, max_addr;
			r.left = COLUMN_X_OFFSET;
			char page;
			for (	i = 0, r.bottom = r.top + mps->cyRow;
					i < rows;
					i++, OffsetRect(&r, 0, mps->cyRow)) {
				TCHAR szVal[32];
				if (mps->type == REGULAR) {
					if (addr < 0)
						StringCbCopy(szVal, sizeof(szVal), _T("0000"));
					else
						StringCbPrintf(szVal, sizeof(szVal), _T("%04X"), addr);
				} else {
					if (addr < 0)
						StringCbPrintf(szVal, sizeof(szVal), _T("%02X 0000"), addr / PAGE_SIZE);
					else
						StringCbPrintf(szVal, sizeof(szVal), _T("%02X %04X"), addr / PAGE_SIZE + 
										(mps->type == RAM ? (lpDebuggerCalc->cpu.pio.model <= TI_83P ? 0x40 : 0x80) : 0), addr % PAGE_SIZE);
				}

				max_addr = GetMaxAddr(mps);

				//draw the address bolded on the right
				if (addr < max_addr) {
					SelectObject(hdc, mps->hfontAddr);
					DrawText(hdc, szVal, -1, &r, DT_LEFT | DT_SINGLELINE | DT_VCENTER);
				}

				if (addr < 0) {
					RECT tr;
					CopyRect(&tr, &r);
					DrawText(hdc, szVal, -1, &tr, DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_CALCRECT);
					OffsetRect(&tr, tm.tmAveCharWidth / 3, 0);
					InflateRect(&tr, 0, -tm.tmAveCharWidth / 2);
					if ((tr.bottom - tr.top) % 2 != 0)
						tr.bottom++;

					POINT pts[] = {
							{tr.right, tr.top},
							{tr.right + (tr.bottom - 1 - tr.top)/2+1, tr.top + (tr.bottom - 1 - tr.top)/2+1},
							{tr.right, tr.bottom}};

					SelectObject(hdc, GetStockObject(DC_PEN));
					SetDCPenColor(hdc, RGB(128, 128, 128));
					SelectObject(hdc, GetStockObject(BLACK_BRUSH));
					Polygon(hdc, pts, 3);
				}

				for (j = 0; j < cols; j++) {
					//rect defining the size of the memory were viewing
					CopyRect(&dr, &r);
					dr.left = tm.tmAveCharWidth * sidebar_width + COLUMN_X_OFFSET + (int) (diff * j);
					dr.right = dr.left + kMemWidth;

					int b;
					BOOL isSel = FALSE;
					if (addr == mps->sel) isSel = TRUE;
					if (isSel == TRUE) {
						mps->xSel = dr.left;
						mps->ySel = dr.top;
						InflateRect(&dr, 2, 0);
						DrawFocusRect(hdc, &dr);
						//DrawItemSelection(hdc, &dr, hwnd == GetFocus());
						InflateRect(&dr, -2, 0);
					}

					value = 0;

					if (addr >= max_addr) break;
					if (addr >= 0) {
						int shift;
						waddr_t waddr = GetWaddr(mps, addr);
						for (b = 0, shift = 0; b < mps->mode; b++, shift += 8) {
							waddr.addr = (addr + b) % PAGE_SIZE;
							waddr.page = !waddr.is_ram ? (addr + b) / PAGE_SIZE : (waddr.addr && b ? waddr.page : GetWaddr(mps, addr + b).page);
							value += wmem_read(lpDebuggerCalc->cpu.mem_c, waddr) << shift;
						}
						waddr = GetWaddr(mps, addr);
						if (isBinary)
							StringCbCopy(szVal, sizeof(szVal), byte_to_binary(value, mps->mode - 1));
						else
							StringCbPrintf(szVal, sizeof(szVal), memfmt, value);

#define COLOR_BREAKPOINT		(RGB(230, 160, 180))
#define COLOR_MEMPOINT_WRITE	(RGB(255, 177, 100))
#define COLOR_MEMPOINT_READ		(RGB(255, 250, 145))
						if (check_break(lpDebuggerCalc->cpu.mem_c, waddr)) {
							InflateRect(&dr, 2, 0);
							DrawItemSelection(hdc, &dr, hwnd == GetFocus(), COLOR_BREAKPOINT, 255);
							if (isSel)
								DrawFocusRect(hdc, &dr);
							InflateRect(&dr, -2, 0);
						}
						if (check_mem_write_break(lpDebuggerCalc->cpu.mem_c, waddr)) {
							InflateRect(&dr, 2, 0);
							DrawItemSelection(hdc, &dr, hwnd == GetFocus(), COLOR_MEMPOINT_WRITE, 255);
							if (isSel)
								DrawFocusRect(hdc, &dr);
							InflateRect(&dr, -2, 0);
						}
						if (check_mem_read_break(lpDebuggerCalc->cpu.mem_c, waddr)) {
							InflateRect(&dr, 2, 0);
							DrawItemSelection(hdc, &dr, hwnd == GetFocus(), COLOR_MEMPOINT_READ , 255);
							if (isSel)
								DrawFocusRect(hdc, &dr);
							InflateRect(&dr, -2, 0);
						}
						addr += mps->mode;

						HGDIOBJ obj = SelectObject(hdc, mps->hfontData);
						DrawText(hdc, szVal, -1, &dr, DT_LEFT | DT_SINGLELINE | DT_VCENTER);
						SelectObject(hdc, obj);
					} else {
						addr += mps->mode;
					}
				}
			}


			GetClientRect(hwnd, &r);
			BitBlt(hdcDest, 0, cyHeader, r.right, r.bottom, hdc, 0, cyHeader, SRCCOPY);

			EndPaint(hwnd, &ps);

			DeleteDC(hdc);
			DeleteObject(hbm);
			return 0;
		}

		case WM_NOTIFY: {
			switch (((NMHDR*) lParam)->code) {
				case HDN_BEGINTRACK:
				case HDN_ENDTRACK:
					return TRUE;
				case TTN_GETDISPINFO:
				{
					NMTTDISPINFO *nmtdi = (NMTTDISPINFO *) lParam;
					mp_settings *mps = (mp_settings*) GetWindowLongPtr(hwnd, GWLP_USERDATA);

					if (mps->type == REGULAR) {
						StringCbPrintf(nmtdi->szText, sizeof(nmtdi->szText), _T("%04X"), mps->addrTrack);
					} else {
						StringCbPrintf(nmtdi->szText, sizeof(nmtdi->szText), _T("%02X %04X"),
							mps->addrTrack / PAGE_SIZE, mps->addrTrack % PAGE_SIZE);
					}
					return TRUE;
				}
			}
			return FALSE;
		}
		case WM_MOUSEWHEEL:
		{
			int zDelta = GET_WHEEL_DELTA_WPARAM(wParam);
			int i;

			WPARAM sbtype;
			if (zDelta > 0) sbtype = SB_LINEUP;
			else sbtype = SB_LINEDOWN;


			for (i = 0; i < abs(zDelta); i += WHEEL_DELTA)
				SendMessage(hwnd, WM_VSCROLL, sbtype, 0);

			return 0;
		}
		case WM_KEYDOWN:
		{
			RECT r;
			GetClientRect(hwnd, &r);
			mp_settings *mps = (mp_settings*) GetWindowLongPtr(hwnd, GWLP_USERDATA);
			int data_length = mps->nCols * mps->nRows * mps->mode;
			switch (wParam) {
				case VK_NEXT:
					SendMessage(hwnd, WM_VSCROLL, SB_PAGEDOWN, 0);
					break;
				case VK_PRIOR:
					SendMessage(hwnd, WM_VSCROLL, SB_PAGEUP, 0);
					break;
				case VK_RIGHT:

					if ((mps->sel - mps->addr)/mps->mode % mps->nCols == mps->nCols-1) {
						mps->sel -= (mps->nCols - 1) * mps->mode;
						break;
					}
				case VK_TAB:
					if (mps->sel <= 0x10000 - mps->mode)
						mps->sel+=mps->mode;
					break;
				case VK_LEFT:
					if ((mps->sel - mps->addr)/mps->mode % mps->nCols == 0)
						mps->sel += (mps->nCols - 1) * mps->mode;
					else if (mps->sel >= mps->mode)
						mps->sel-=mps->mode;
					break;
				case VK_UP:
					if (mps->sel - mps->nCols * mps->mode >= 0)
						mps->sel -= mps->nCols * mps->mode;
					if (mps->sel < mps->addr)
						SendMessage(hwnd, WM_VSCROLL, SB_LINEUP, 0);
					break;
				case VK_DOWN:
					if (mps->sel + mps->nCols * mps->mode < 0x10000)
						mps->sel += mps->nCols * mps->mode;
					if (mps->sel > mps->addr + data_length)
						SendMessage(hwnd, WM_VSCROLL, SB_LINEDOWN, 0);
					break;
				case VK_RETURN:
					SendMessage(hwnd, WM_LBUTTONDBLCLK, 0, MAKELPARAM(mps->xSel, mps->ySel));
					break;
				case VK_F3:
					SendMessage(hwnd, WM_COMMAND, DB_MEMPOINT_WRITE, 0);
					break;
				case 'G':
					SendMessage(hwnd, WM_COMMAND, DB_GOTO, 0);
					break;
				case 'F':
					SendMessage(hwnd, WM_COMMAND, DB_OPEN_FIND, 0);
					break;
				default:
					return 0;
			}
			Debug_UpdateWindow(hwnd);
			return 0;
		}
		case WM_VSCROLL: {
			mp_settings *mps = (mp_settings*) GetWindowLongPtr(hwnd, GWLP_USERDATA);
			int data_length = mps->nCols * mps->nRows * mps->mode;

			SCROLLINFO si;
			si.cbSize = sizeof(SCROLLINFO);
			si.fMask = SIF_PAGE | SIF_RANGE;
			si.nPage = data_length;
			si.nMax = GetMaxAddr(mps) + 1;
			si.nMin = mps->memNum == -1 ? lpDebuggerCalc->cpu.sp : 0x0000;
			SetScrollInfo(hwnd, SB_VERT, &si, TRUE);

			si.fMask = SIF_TRACKPOS | SIF_RANGE;
			GetScrollInfo(hwnd, SB_VERT, &si);

			switch (LOWORD(wParam)) {
				case SB_TOP:			//Home key
					mps->addr = si.nMin;
					break;
				case SB_BOTTOM:
					mps->addr = GetMaxAddr(mps) - data_length;
					break;
				case SB_LINEUP:
					mps->addr -= mps->nCols * mps->mode;
					if (mps->addr < si.nMin)
						mps->addr = si.nMin;
					break;
				case SB_LINEDOWN:
					if (mps->addr + data_length < GetMaxAddr(mps))
						mps->addr += mps->nCols * mps->mode;
					break;
				case SB_THUMBTRACK: {
					int val = si.nTrackPos > mps->addr ? mps->nCols * mps->mode : 0;
					mps->addr = si.nTrackPos - ((si.nTrackPos - si.nMin) % (mps->nCols * mps->mode)) + val;
					break;
				}
				case SB_PAGEDOWN:
					if (mps->addr + data_length < GetMaxAddr(mps))
						mps->addr += mps->nCols * mps->mode * 4;
					break;
				case SB_PAGEUP:
					if (mps->addr - mps->nCols * mps->mode * 4 >= 0)
						mps->addr -= mps->nCols * mps->mode * 4;
					break;
			}

			si.cbSize = sizeof(SCROLLINFO);
			si.fMask = SIF_POS;
			si.nPos = mps->addr;
			SetScrollInfo(hwnd, SB_VERT, &si, TRUE);

			SendMessage(hwnd, WM_USER, DB_UPDATE, WM_VSCROLL);

			return mps->cyRow;
		}
		case WM_COMMAND:
		{
			mp_settings *mps = (mp_settings*) GetWindowLongPtr(hwnd, GWLP_USERDATA);
			switch (HIWORD(wParam)) {
				case EN_KILLFOCUS:
					if (GetFocus() == hwnd) break;
				case EN_SUBMIT:
				{
					_TUCHAR data[8];
					ValueSubmit(hwndVal, (TCHAR *) data, mps->mode + (2 * mps->bText));
					int i;
					for (i = 0; i < mps->mode; i++) {
						mem_write(&lpDebuggerCalc->mem_c, mps->sel + i, data[i]);
					}
					Debug_UpdateWindow(GetParent(hwnd));
					hwndVal = NULL;
					if (HIWORD(wParam) != EN_KILLFOCUS) {
						SendMessage(hwnd, WM_KEYDOWN, VK_TAB, 0);
						SetFocus(hwnd);
					}
					break;
				}
			}
			HWND hwndDialog;
			switch(LOWORD(wParam))
			{
				case DB_OPEN_FIND:
					hwndDialog = CreateDialog(g_hInst, MAKEINTRESOURCE(IDD_DLGFIND), hwnd, (DLGPROC) FindDialogProc);
					ShowWindow(hwndDialog, SW_SHOW);
					break;
				case DB_FIND_NEXT: {
					int addr = mps->addr;
					while (addr < 0xFFFF && mem_read(&lpDebuggerCalc->mem_c, addr) != find_value)
						addr++;
					if (addr > 0xFFFF) {
						MessageBox(NULL, _T("Value not found"), _T("Find results"), MB_OK);
						break;
					}
					mps->addr = addr;
					SetFocus(hwnd);
					Debug_UpdateWindow(hwnd);
					break;
				}
				case DB_GOTO: {
					int result;
					result = (int) DialogBox(g_hInst, MAKEINTRESOURCE(IDD_DLGGOTO), hwnd, (DLGPROC) GotoDialogProc);
					if (result == IDOK) {
						switch (mps->type) {
							case REGULAR:
								goto_addr = goto_addr & 0xFFFF;
								break;
							case FLASH:
								goto_addr = ((goto_addr & 0xFFFF) % PAGE_SIZE) + ((goto_addr >> 16) * PAGE_SIZE);
								if (goto_addr > GetMaxAddr(mps))
									goto_addr = GetMaxAddr(mps) - 1;
								break;
							case RAM: {						
								goto_addr = ((goto_addr & 0xFFFF) % PAGE_SIZE) + (((goto_addr >> 16) % 0x80) * PAGE_SIZE);
								if (goto_addr > GetMaxAddr(mps))
									goto_addr = GetMaxAddr(mps) - 1;
								break;
							}
						}
						mps->addr = goto_addr;
					}
					SCROLLINFO si;
					si.cbSize = sizeof(SCROLLINFO);
					si.fMask = SIF_POS;
					si.nPos = mps->addr;
					SetScrollInfo(hwnd, SB_VERT, &si, TRUE);

					SetFocus(hwnd);
					Debug_UpdateWindow(hwnd);
					break;
				}
				case DB_BREAKPOINT: {
					waddr_t waddr = GetWaddr(mps, mps->sel);
					if (check_break(&lpDebuggerCalc->mem_c, waddr))
						clear_break(&lpDebuggerCalc->mem_c, waddr);
					else
						set_break(&lpDebuggerCalc->mem_c, waddr);

					RECT rc;
					GetClientRect(hwnd, &rc);
					InvalidateRect(hwnd, &rc, FALSE);
					break;
				}
				case DB_MEMPOINT_WRITE: {
					waddr_t waddr = GetWaddr(mps, mps->sel);
					if (check_mem_write_break(&lpDebuggerCalc->mem_c, waddr))
						clear_mem_write_break(&lpDebuggerCalc->mem_c, waddr);
					else
						set_mem_write_break(&lpDebuggerCalc->mem_c, waddr);

					RECT rc;
					GetClientRect(hwnd, &rc);
					InvalidateRect(hwnd, &rc, FALSE);
					break;
				}
				case DB_MEMPOINT_READ: {
					waddr_t waddr = GetWaddr(mps, mps->sel);
					if (check_mem_read_break(&lpDebuggerCalc->mem_c, waddr))
						clear_mem_read_break(&lpDebuggerCalc->mem_c, waddr);
					else
						set_mem_read_break(&lpDebuggerCalc->mem_c, waddr);

					RECT rc;
					GetClientRect(hwnd, &rc);
					InvalidateRect(hwnd, &rc, FALSE);
					break;
				}
				case DB_STEP:
				case DB_STEPOVER:
					extern HWND hdisasm;
					SendMessage(hdisasm, WM_COMMAND, wParam, 0);
					break;
			}
			return 0;
		}
		case WM_LBUTTONDBLCLK:
		case WM_LBUTTONDOWN:
		{
			mp_settings *mps = (mp_settings*) GetWindowLongPtr(hwnd, GWLP_USERDATA);
			RECT rc;
			GetClientRect(hwnd, &rc);

			int x = GET_X_LPARAM(lParam);
			int y = GET_Y_LPARAM(lParam);

			HWND oldVal = FindWindowEx(hwnd, NULL, _T("EDIT"), NULL);
			if (oldVal) {
				SendMessage(hwnd, WM_COMMAND, MAKEWPARAM(0, EN_SUBMIT), (LPARAM) oldVal);
			}

			POINT pt = {x, y};
			RECT r;
			int addr = AddrFromPoint(hwnd, pt, &r);

			if (addr != -1) {
				mps->sel = addr;
				if (Message == WM_LBUTTONDBLCLK) {
					TCHAR szInitial[8];

					int value = 0;
					int shift, b;
					waddr_t waddr = GetWaddr(mps, addr);
					for (b = 0, shift = 0; b < mps->mode; b++, shift += 8) {
						waddr.addr = (addr + b) % PAGE_SIZE;
							waddr.page = !waddr.is_ram ? (addr + b) / PAGE_SIZE: (waddr.addr ? waddr.page : waddr.page + 1);
							value += wmem_read(lpDebuggerCalc->cpu.mem_c, waddr) << shift;
					}

					TCHAR szFmt[8];
					if (mps->bText)
						StringCbCopy(szFmt, sizeof(szFmt), _T("%c"));
					else
						StringCbPrintf(szFmt, sizeof(szFmt), _T("%%0%dx"), mps->mode*2);
					StringCbPrintf(szInitial, sizeof(szFmt), szFmt, value);

					hwndVal =
					CreateWindow(_T("EDIT"), szInitial,
						WS_CHILD | WS_VISIBLE | WS_BORDER | ES_LEFT | ES_MULTILINE,
						r.left-2,
						r.top,
						r.right-r.left+4,
						r.bottom - r.top +4,
						hwnd, 0, g_hInst, NULL);

					VALUE_FORMAT format = GetValueFormat(mps);
					SubclassEdit(hwndVal, (mps->mode == 3) ? 1 : mps->mode * 2, format);
				}
			}
			Debug_UpdateWindow(hwnd);
			SetFocus(hwnd);
			return 0;
		}
		case WM_MOUSEMOVE:
		{
			mp_settings *mps = (mp_settings*) GetWindowLongPtr(hwnd, GWLP_USERDATA);
			static int addrTrackPrev = -1;

			POINT pt = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
			int thisAddr = AddrFromPoint(hwnd, pt, NULL);
			if (thisAddr != addrTrackPrev) {
				mps->addrTrack = thisAddr;
				//Close the tooltip
				if (IsWindowVisible(mps->hwndTip))
					SendMessage(mps->hwndTip, TTM_POP, 0, 0);
			}
			addrTrackPrev = thisAddr;
			return 0;
		}
		case WM_USER:
			switch (wParam) {
				case DB_UPDATE:
				{
					mp_settings *mps = (mp_settings*) GetWindowLongPtr(hwnd, GWLP_USERDATA);
					if (mps->track != -1 && lParam == 0) {
						mps->addr = ((unsigned short*) &lpDebuggerCalc->cpu)[mps->track/2];
					}
					
					//setup the scroll bar 8 isn't used
					if (lParam != WM_VSCROLL)
						SendMessage(hwnd, WM_VSCROLL, MAKEWPARAM(8, 0), 0);
					RECT rc;
					GetClientRect(hwnd, &rc);
					InvalidateRect(hwnd, &rc, FALSE);
					UpdateWindow(hwnd);
					break;
				}
			}
			return 0;
		case WM_DESTROY: {
			mp_settings *mps = (mp_settings *) GetWindowLongPtr(hwnd, GWLP_USERDATA);
			if (mps->memNum != -1) {
				TCHAR buffer[64];
				StringCbPrintf(buffer, sizeof(buffer), _T("Mem%i"), mps->memNum);
				DWORD value = mps->addr;
				SaveDebugKey(buffer, REG_DWORD, &value);
			}
			return 0;
		}
		default:
			return DefWindowProc(hwnd, Message, wParam, lParam);
	}
}
