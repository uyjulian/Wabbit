#include "stdafx.h"

#include "gui.h"
#include "calc.h"
#include "gif.h"

static HKEY hkeyTarget;

static struct {
	LPCTSTR lpValueName;
	DWORD dwType;
	LONG_PTR Value;
} regDefaults[] = {
	{_T("cutout"), 			REG_DWORD, 	FALSE},
	{_T("skin"),			REG_DWORD,	FALSE},
	{_T("alphablend_lcd"),	REG_DWORD,	TRUE},
	{_T("version"), 		REG_SZ, 	(LONG_PTR) _T("1.5")},
	{_T("rom_path"), 		REG_SZ, 	(LONG_PTR) _T("z.rom")},
	{_T("shades"),			REG_DWORD,	6},
	{_T("gif_path"), 		REG_SZ,		(LONG_PTR) _T("wabbitemu.gif")},
	{_T("gif_autosave"),	REG_DWORD,	FALSE},
	{_T("gif_useinc"),		REG_DWORD,	FALSE},
	{_T("lcd_mode"),		REG_DWORD,	0},		// perfect gray
	{_T("lcd_freq"),		REG_DWORD,	FPS},	// steady freq
	{_T("screen_scale"),	REG_DWORD,  2},
	{_T("faceplate_color"), REG_DWORD, 	0x838587},
	{_T("exit_save_state"), REG_DWORD,  FALSE},
	{_T("load_files_first"),REG_DWORD,  FALSE},
	{_T("do_backups"),		REG_DWORD,  FALSE},
	{_T("show_wizard"),		REG_DWORD,  TRUE},
	{_T("sync_cores"),		REG_DWORD,  FALSE},
	{_T("num_keys"),		REG_DWORD,  5},
	{_T("always_on_top"),	REG_DWORD,  FALSE},
	{NULL,					0,			0},
};

HRESULT LoadRegistryDefaults(HKEY hkey) {

	u_int i;
	for (i = 0; regDefaults[i].lpValueName != NULL; i++) {
		DWORD cbData;
		BYTE *lpData;
		switch (regDefaults[i].dwType) {
		case REG_DWORD:
			cbData = sizeof(DWORD);
			lpData = (BYTE*) &regDefaults[i].Value;
			break;
		case REG_SZ:
			cbData = lstrlen((TCHAR *) regDefaults[i].Value) + 1;
			lpData = (LPBYTE) regDefaults[i].Value;
			break;
		default:
			cbData = 0;
			lpData = NULL;
		}
		RegSetValueEx(
				hkey,
				regDefaults[i].lpValueName,
				0,
				regDefaults[i].dwType,
				lpData,
				cbData);
	}
	return S_OK;
}

INT_PTR QueryWabbitKey(LPCTSTR lpszName) {
	HKEY hkeySoftware;
	RegOpenKeyEx(HKEY_CURRENT_USER, _T("software"), 0, KEY_ALL_ACCESS, &hkeySoftware);
	
	HKEY hkeyWabbit;
	DWORD dwDisposition;
	RegCreateKeyEx(hkeySoftware, _T("Wabbitemu"), 0, 
			NULL, REG_OPTION_NON_VOLATILE, 
			KEY_ALL_ACCESS, NULL, &hkeyWabbit, &dwDisposition);
	
	DWORD type;
	DWORD len;
	u_int i;
	for (i = 0; regDefaults[i].lpValueName != NULL; i++) {
		if (_tcsicmp(regDefaults[i].lpValueName, lpszName) == 0) break;
	}
	type = regDefaults[i].dwType;
	
	static union {
		DWORD dwResult;
		TCHAR szResult[256];
	} result;
	len = (type == REG_SZ) ? 256 * sizeof(WCHAR) : sizeof(DWORD);
	
	LONG rqvx_res;
	if (regDefaults[i].lpValueName != NULL) {
		TCHAR szKeyName[256];
		StringCbCopy(szKeyName, sizeof(szKeyName), lpszName);

		//MultiByteToWideChar(CP_ACP, 0, lpszName, -1, szKeyName, ARRAYSIZE(szKeyName));
		rqvx_res = RegQueryValueEx(hkeyWabbit, szKeyName, NULL, NULL, (LPBYTE) &result, &len);
		if (rqvx_res == ERROR_FILE_NOT_FOUND) {
			if (type == REG_DWORD)
			{
				result.dwResult = (DWORD) regDefaults[i].Value;
			}
			else
			{
#ifdef UNICODE
				StringCbCopy(result.szResult, sizeof(result.szResult), (LPWSTR) regDefaults[i].Value);
#else
				WideCharToMultiByte(CP_ACP, 0, (LPWSTR) regDefaults[i].Value, -1, result.szResult, sizeof(result.szResult), NULL, NULL);
#endif
			}
		}
	} else {
		//MessageBox(NULL, "Could not find registry key", lpszName, MB_OK);
		return NULL;
	}
	
	RegCloseKey(hkeyWabbit);
	RegCloseKey(hkeySoftware);
	
	return (type == REG_SZ) ? (LONG_PTR) result.szResult : result.dwResult;
}

HRESULT LoadRegistrySettings(const LPCALC lpCalc) {
	HKEY hkeySoftware;
	RegOpenKeyEx(HKEY_CURRENT_USER, _T("software"), 0, KEY_ALL_ACCESS, &hkeySoftware);
	
	HKEY hkeyWabbit;
	DWORD dwDisposition;
	RegCreateKeyEx(hkeySoftware, _T("Wabbitemu"), 0, 
			NULL, REG_OPTION_NON_VOLATILE, 
			KEY_ALL_ACCESS, NULL, &hkeyWabbit, &dwDisposition);
	
	if (dwDisposition == REG_CREATED_NEW_KEY)
		LoadRegistryDefaults(hkeyWabbit);
	
	StringCbCopy(lpCalc->rom_path, sizeof(lpCalc->rom_path), (TCHAR *) QueryWabbitKey(_T("rom_path")));
	lpCalc->SkinEnabled = (BOOL) QueryWabbitKey(_T("skin"));
	lpCalc->bCutout = (BOOL) QueryWabbitKey(_T("cutout"));
	lpCalc->bAlphaBlendLCD = (BOOL) QueryWabbitKey(_T("alphablend_lcd"));
	lpCalc->Scale = (int) QueryWabbitKey(_T("screen_scale"));
	lpCalc->FaceplateColor = (COLORREF) QueryWabbitKey(_T("faceplate_color"));
	exit_save_state = (BOOL) QueryWabbitKey(_T("exit_save_state"));
	load_files_first = (BOOL) QueryWabbitKey(_T("load_files_first"));
	do_backups = (BOOL) QueryWabbitKey(_T("do_backups"));
	show_wizard = (BOOL) QueryWabbitKey(_T("show_wizard"));
	sync_cores = (BOOL) QueryWabbitKey(_T("sync_cores"));
	lpCalc->bAlwaysOnTop = (BOOL) QueryWabbitKey(_T("always_on_top"));
	lpCalc->bCustomSkin = (BOOL) QueryWabbitKey(_T("custom_skin"));
	int num_entries = (int) QueryWabbitKey(_T("num_keys"));
	//need to load accelerators
	// querywabbitkey doesnt work because its a REG_BINARY
	/*ACCEL buf[256];
	DWORD dwCount = sizeof(buf);
	LONG res = RegQueryValueEx(hkeyWabbit, "accelerators", NULL, NULL, (LPBYTE)buf, &dwCount);
	if (res == ERROR_SUCCESS){
		DestroyAcceleratorTable(haccelmain);
		haccelmain = CreateAcceleratorTable(buf, num_entries);
	}*/
	/*
	 * 		{"gif_path", 	REG_SZ,		"wabbitemu.gif"},
		{"gif_autosave",REG_DWORD,	0},
		{"gif_useinc",	REG_DWORD,	0},
		*/
	
	StringCbCopy(gif_file_name, sizeof(gif_file_name), (TCHAR *) QueryWabbitKey(_T("gif_path")));
	gif_autosave = (BOOL) QueryWabbitKey(_T("gif_autosave"));
	gif_use_increasing = (BOOL) QueryWabbitKey(_T("gif_useinc"));
	
	//RegCloseKey(hkeyWabbit);
	hkeyTarget = hkeyWabbit;
	RegCloseKey(hkeySoftware);
	
	return S_OK;
}

void SaveWabbitKey(TCHAR *name, int type, void *value) {
	size_t len;

	if (type == REG_DWORD) {
		len = sizeof(DWORD);
	} else if (type == REG_SZ) {
		StringCbLength((TCHAR *) value, MAX_PATH, &len);
	}
	
	RegSetValueEx(hkeyTarget, name, 0, type, (LPBYTE) value, len);
	
}


HRESULT SaveRegistrySettings(const LPCALC lpCalc) {
	if (hkeyTarget)
		RegCloseKey(hkeyTarget);
	HKEY hkeyWabbit;
	HRESULT res;
	res = RegOpenKeyEx(HKEY_CURRENT_USER, _T("software\\Wabbitemu"), 0, KEY_ALL_ACCESS, &hkeyWabbit);
	if (SUCCEEDED(res)) {
		hkeyTarget = hkeyWabbit;
		
		SaveWabbitKey(_T("cutout"), REG_DWORD, &lpCalc->bCutout);
		SaveWabbitKey(_T("alphablend_lcd"), REG_DWORD, &lpCalc->bAlphaBlendLCD);
		SaveWabbitKey(_T("skin"), REG_DWORD, &lpCalc->SkinEnabled);
		SaveWabbitKey(_T("rom_path"), REG_SZ, &lpCalc->rom_path);
		SaveWabbitKey(_T("gif_path"), REG_SZ, &gif_file_name);
		SaveWabbitKey(_T("gif_autosave"), REG_DWORD, &gif_autosave);
		SaveWabbitKey(_T("gif_useinc"), REG_DWORD, &gif_use_increasing);
		SaveWabbitKey(_T("exit_save_state"), REG_DWORD, &exit_save_state);
		SaveWabbitKey(_T("load_files_first"), REG_DWORD, &load_files_first);
		SaveWabbitKey(_T("do_backups"), REG_DWORD, &do_backups);
		SaveWabbitKey(_T("show_wizard"), REG_DWORD, &show_wizard);
		SaveWabbitKey(_T("sync_cores"), REG_DWORD, &sync_cores);

		SaveWabbitKey(_T("faceplate_color"), REG_DWORD, &lpCalc->FaceplateColor);
		SaveWabbitKey(_T("custom_skin"), REG_DWORD, &lpCalc->bCustomSkin);
		SaveWabbitKey(_T("skin_path"), REG_SZ, &lpCalc->skin_path);
		SaveWabbitKey(_T("keymap_path"), REG_SZ, &lpCalc->keymap_path);
		/*ACCEL buf[256];
		DWORD dwCount = sizeof(buf);
		DWORD dwType = NULL;
		LONG res = RegQueryValueEx(hkeyWabbit, "accelerators", NULL, &dwType,
			(LPBYTE)buf, &dwCount);
		if (res == ERROR_SUCCESS){
			DestroyAcceleratorTable(haccelmain);
			haccelmain = CreateAcceleratorTable(buf, dwCount);
		}*/
		
		SaveWabbitKey(_T("shades"), REG_DWORD, &lpCalc->cpu.pio.lcd->shades);
		SaveWabbitKey(_T("lcd_mode"), REG_DWORD, &lpCalc->cpu.pio.lcd->mode);
		DWORD steady = (DWORD) ( 1.0 / lpCalc->cpu.pio.lcd->steady_frame);
		SaveWabbitKey(_T("lcd_freq"), REG_DWORD, &steady);
		
		SaveWabbitKey(_T("screen_scale"), REG_DWORD, &lpCalc->Scale);

	}
	RegCloseKey(hkeyWabbit);
	return S_OK;
}
