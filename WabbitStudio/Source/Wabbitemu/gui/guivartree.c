#include "stdafx.h"

#include "calc.h"
#include "link.h"
#include "state.h"
#include "guivartree.h"
#include "guicontext.h"

#include "exportvar.h"
#include "resource.h"

static HWND g_hwndVarTree;
extern HINSTANCE g_hInst;
extern unsigned char type_ext[][4];
TCHAR export_file_name[512] = _T("Zelda.8xk");

static RECT VTrc = {-1, -1, -1, -1};
static VARTREEVIEW_t Tree[MAX_CALCS];
static BOOL Tree_init = FALSE;

void test_function(int num);

BOOL VarTreeOpen(BOOL refresh){
	HWND vardialog = FindWindow(NULL, _T("Calculator Variables"));
	if (vardialog) {
		if (refresh) {
			RefreshTreeView(FALSE);
		}
		return TRUE;
	}
	return FALSE;
}

HWND CreateVarTreeList() {
	INITCOMMONCONTROLSEX icc ;
	if (!VarTreeOpen(TRUE)) {
	    icc.dwSize = sizeof(icc);
	    icc.dwICC = ICC_TREEVIEW_CLASSES;
		if (!InitCommonControlsEx(&icc)) return NULL;
	    return  CreateDialog(g_hInst, 
				            MAKEINTRESOURCE(IDD_VARLIST), 
				            NULL, 
				            DlgVarlist);
	}
	return NULL;
}

#ifdef USE_COM
HRESULT CreateDropSource(WB_IDropSource **ppDropSource);
HRESULT CreateDataObject(FORMATETC *fmtetc, STGMEDIUM *stgmeds, UINT count, WB_IDataObject **ppDataObject);
void SetDropSourceDataObject(WB_IDropSource *pDropSource, WB_IDataObject *pDataObject);
#endif


INT_PTR CALLBACK DlgVarlist(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam) {
    switch (Message) {
        case WM_INITDIALOG:
        {
            g_hwndVarTree = GetDlgItem(hwnd, IDC_TRV1);
            HIMAGELIST hIL = ImageList_LoadImage(g_hInst, _T("TIvarIcons"), 
                                                    16, 0, RGB(0,255,0),
													IMAGE_BITMAP, LR_CREATEDIBSECTION);
            if (!hIL) _tprintf_s(_T("Imagelist not loaded"));
            else TreeView_SetImageList(g_hwndVarTree, hIL, TVSIL_NORMAL);
            
            if (VTrc.bottom == -1) {
	            GetWindowRect(hwnd, &VTrc);	
			} else {
				MoveWindow(hwnd,VTrc.left,VTrc.top,VTrc.right-VTrc.left,VTrc.bottom-VTrc.top,TRUE);
			}
            RefreshTreeView(TRUE);
            return TRUE;
        }
        case WM_SIZE:
			{
				GetWindowRect(hwnd, &VTrc);
				int width = (VTrc.right-VTrc.left)-14-6;
				int height = (VTrc.bottom-VTrc.top)-38-30-73;
				MoveWindow(g_hwndVarTree,6,30,width,height,TRUE);

				break;
			}
        case WM_COMMAND:
        {
            switch (LOWORD(wParam)) {
                case IDC_REFRESH_VAR_LIST:
					RefreshTreeView(FALSE);
                    break;
				case IDC_EXPORT_VAR: {
					char *buf;
					FILE *file;
					HTREEITEM item = TreeView_GetSelection(g_hwndVarTree);
					//HACK: yes i know FILEDESCRIPTOR is not meant for this.
					//but i dont want to rework the routines to return the attributes differently
					FILEDESCRIPTOR *fd;
					fd = (FILEDESCRIPTOR *) malloc(sizeof(FILEDESCRIPTOR));
					if (fd == NULL)
						MessageBox(NULL, _T("BAD"), _T("FUCK"), MB_OK);
					if (!FillDesc(item, fd)) {
						free (fd);
						break;
					}
					buf =  (char *) malloc(fd->nFileSizeLow);
					FillFileBuffer(item, buf);
					if (SetVarName(fd)) {
						free(buf);
						free (fd);
						break;
					}
#ifdef WINVER
					_tfopen_s(&file, export_file_name, _T("wb"));
#else
					file = fopen(export_file_name, "wb");
#endif
					fwrite(buf, 1, fd->nFileSizeLow, file);
					fclose(file);
					free(buf);
					free(fd);
					break;
				}
				default:
					break;
            }
            return TRUE;
        }
        case WM_NOTIFY:
		{
			NMTREEVIEW *nmtv = (LPNMTREEVIEW) lParam;
			switch (((NMHDR*) lParam)->code) {
				case NM_DBLCLK:	{
//					int slot;
//					for(slot=0;slot<MAX_CALCS;slot++) {
//						DispSymbols(&Tree[slot].sym);
//					}
					break;
				}
				case NM_RCLICK:
				{
					// Working on this ...
					HMENU hmenu;
					hmenu = LoadMenu(g_hInst, MAKEINTRESOURCE(IDR_VARTREE_MENU));
					
					hmenu = GetSubMenu(hmenu, 0); 

					POINT p;
					GetCursorPos(&p);
					
					OnContextMenu(hwnd, p.x, p.y, hmenu);

					DestroyMenu(hmenu); 
					return TRUE;
				}
#ifdef USE_COM
				case TVN_BEGINDRAG:	{
					TreeView_SelectItem(g_hwndVarTree, nmtv->itemNew.hItem);

					WB_IDataObject *pDataObject;
					WB_IDropSource *pDropSource;
					DWORD		 dwEffect;
		
					FORMATETC fmtetc[2] = {
						{RegisterClipboardFormat(CFSTR_FILEDESCRIPTOR), 0, DVASPECT_CONTENT, -1, TYMED_HGLOBAL },
						{RegisterClipboardFormat(CFSTR_FILECONTENTS), 0, DVASPECT_CONTENT, 0, TYMED_HGLOBAL }};
					STGMEDIUM stgmed[2] = {
						{ TYMED_HGLOBAL, { 0 }, 0 },
						{ TYMED_HGLOBAL, { 0 }, 0 }};
		
					// transfer the current selection into the IDataObject
					stgmed[0].hGlobal = GlobalAlloc(GHND, 
										sizeof(FILEGROUPDESCRIPTOR) + sizeof(FILEDESCRIPTOR));
										
					FILEGROUPDESCRIPTOR *fgd = (FILEGROUPDESCRIPTOR*) GlobalLock(stgmed[0].hGlobal);
					
					fgd->cItems = 1;
					FILEDESCRIPTOR *fd = fgd->fgd;
					
					ZeroMemory(fd, sizeof(FILEDESCRIPTOR));
					
					if (FillDesc(nmtv->itemNew.hItem, fd) == NULL || fd->nFileSizeLow == 0) {
						GlobalFree(stgmed[0].hGlobal);
						return FALSE;
					}
					GlobalUnlock(stgmed[0].hGlobal);
		
					stgmed[1].hGlobal = GlobalAlloc(GHND, fd->nFileSizeLow);
					char *buf = GlobalLock(stgmed[1].hGlobal);
					
					FillFileBuffer(nmtv->itemNew.hItem, buf);
		
					GlobalUnlock(stgmed[1].hGlobal);
					
					// Create IDataObject and IDropSource COM objects
					CreateDropSource(&pDropSource);
					CreateDataObject(fmtetc, stgmed, 2, &pDataObject);
		
					//
					//	** ** ** The drag-drop operation starts here! ** ** **
					//
					SetDropSourceDataObject(pDropSource, pDataObject);
					DoDragDrop((IDataObject*)pDataObject, (struct IDropSource*)pDropSource, DROPEFFECT_COPY, &dwEffect);
					return TRUE;
				}
#endif
			}
			
			return TRUE;
		}

		case WM_CLOSE:
		case WM_DESTROY:
			GetWindowRect(hwnd, &VTrc);	
			DestroyWindow(hwnd);
			return TRUE;
    }
   return FALSE;
}

int SetVarName(FILEDESCRIPTOR *fd) {
	OPENFILENAME ofn;
	TCHAR *defExt;
	int filterIndex;
	TCHAR lpstrFilter[] 	= _T("\
Programs  (*.8xp)\0*.8xp\0\
Applications (*.8xk)\0*.8xk\0\
App Vars (*.8xv)\0*.8xv\0\
Lists  (*.8xl)\0*.8xl\0\
Real/Complex Variables  (*.8xn)\0*.8xn\0\
Pictures  (*.8xi)\0*.8xi\0\
GDBs  (*.8xd)\0*.8xd\0\
Matrices  (*.8xm)\0*.8xm\0\
Strings  (*.8xs)\0*.8xs\0\
Groups  (*.8xg)\0*.8xg\0\
All Files (*.*)\0*.*\0\0");
	TCHAR lpstrFile[MAX_PATH];
	unsigned int Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;
#ifdef WINVER
	StringCbCopy(lpstrFile, sizeof(lpstrFile), fd->cFileName);
#else
	strcpy(lpstrFile, fd->cFileName);
#endif
	int i = (int) _tcslen(lpstrFile);
	lpstrFile[i] = '\0';
	defExt = &lpstrFile[i];
	while (*defExt != '.')
		defExt--;
	switch (defExt[3]) {
		case 'p':
			filterIndex = 1;
			break;
		case 'k':
			filterIndex = 2;
			break;
		case 'v':
			filterIndex = 3;
			break;
		case 'l':
			filterIndex = 4;
			break;
		case 'n':
			filterIndex = 5;
			break;
		case 'i':
			filterIndex = 6;
			break;
		case 'd':
			filterIndex = 7;
			break;
		case 'm':
			filterIndex = 8;
			break;
		case 's':
			filterIndex = 9;
			break;
		case 'g':
			filterIndex = 10;
			break;
		default:
			filterIndex = 11;
			break;
	}

	ofn.lStructSize			= sizeof(OPENFILENAME);
	ofn.hwndOwner			= GetForegroundWindow();
	ofn.hInstance			= NULL;
	ofn.lpstrFilter			= (LPCTSTR) lpstrFilter;
	ofn.lpstrCustomFilter	= NULL;
	ofn.nMaxCustFilter		= 0;
	ofn.nFilterIndex		= filterIndex;
	ofn.lpstrFile			= lpstrFile;
	ofn.nMaxFile			= sizeof(lpstrFile);
	ofn.lpstrFileTitle		= NULL;
	ofn.nMaxFileTitle		= 0;
	ofn.lpstrInitialDir		= NULL;
	ofn.lpstrTitle			= _T("Wabbitemu Export");
	ofn.Flags				= Flags | OFN_HIDEREADONLY | OFN_EXPLORER | OFN_LONGNAMES;
	ofn.lpstrDefExt			= defExt;
	ofn.lCustData			= 0;
	ofn.lpfnHook			= NULL;
	ofn.lpTemplateName		= NULL;
	ofn.pvReserved			= NULL;
	ofn.dwReserved			= 0;
	ofn.FlagsEx				= 0;

	if (!GetSaveFileName(&ofn)) {
		return 1;
	}
#ifdef WINVER
	StringCbCopy(export_file_name, sizeof(export_file_name), lpstrFile);
#else
	strcpy(export_file_name, lpstrFile);
#endif
	return 0;
}


/* deletes parent's children, not parent*/
void DeleteChildren(HWND hwnd, HTREEITEM parent) {
	HTREEITEM Child;
	while(Child = TreeView_GetChild(hwnd,parent)) {
		TreeView_DeleteItem(hwnd,Child);
	}
}


HTREEITEM InsertVar(HTREEITEM parent, TCHAR *Name, int icon) {
	TVINSERTSTRUCT tvs;
	tvs.hParent				= parent;
	tvs.hInsertAfter		= TVI_SORT;
	tvs.item.mask			= TVIF_IMAGE | TVIF_SELECTEDIMAGE |TVIF_TEXT;
	tvs.item.pszText		= (LPTSTR)(Name);
	tvs.item.cchTextMax		= (int) _tcslen(Name) + 1;
	tvs.item.iImage			= icon;
	tvs.item.iSelectedImage	= tvs.item.iImage;
	return TreeView_InsertItem(g_hwndVarTree, &tvs);
}

/* updates the tree view */
void RefreshTreeView(BOOL New) {
	u_int i;
	int slot;
	float ver;
	TVINSERTSTRUCT tvs;

	if (Tree_init == FALSE || New != FALSE) {
		memset(Tree, 0, sizeof(Tree));
		Tree_init = TRUE;
	}

	/* run through all active calcs */
	for(slot = 0;slot < MAX_CALCS;slot++) {


		if (!calcs[slot].active && Tree[slot].model!=0) {
			TreeView_DeleteItem(g_hwndVarTree,Tree[slot].hRoot);
			Tree[slot].model = 0;
		}

		if (calcs[slot].active && Tree[slot].model!=calcs[slot].model && Tree[slot].model!=0) {
			TreeView_DeleteItem(g_hwndVarTree,Tree[slot].hRoot);
			Tree[slot].model = 0;
		}
		
		
		/*It's an 83+ compatible with a known rom(hopefully)*/
		if (calcs[slot].active && calcs[slot].model >=TI_83P &&
#ifdef WINVER
			sscanf_s(calcs[slot].rom_version, "%f", &ver) == 1) {
#else
			sscanf(calcs[slot].rom_version,"%f",&ver) == 1) {
#endif
				

			/* This slot has not yet been initlised. */
			/* so set up the Root */
			if (Tree[slot].model==0) {
				tvs.hParent				= TVI_ROOT;
				tvs.hInsertAfter		= TVI_ROOT;
				tvs.item.mask			= TVIF_IMAGE | TVIF_SELECTEDIMAGE |TVIF_TEXT;
				tvs.item.pszText		= (LPTSTR)CalcModelTxt[calcs[slot].model];
				tvs.item.cchTextMax		= (int) _tcslen(tvs.item.pszText) + 1;
				tvs.item.iImage			= TI_ICON_84PSE;
				tvs.item.iSelectedImage	= tvs.item.iImage;
				Tree[slot].hRoot		= TreeView_InsertItem(g_hwndVarTree, &tvs);
			}

			/* If nodes haven't been init or the model is reset, create nodes */
			/* otherwise delete children so the vars can be appended */
			if (!Tree[slot].hApplication || Tree[slot].model==0) {
				Tree[slot].hApplication = InsertVar(Tree[slot].hRoot, _T("Application"), TI_ICON_APP);
			} else DeleteChildren(g_hwndVarTree,Tree[slot].hApplication);

			if (!Tree[slot].hProgram || Tree[slot].model==0) {
				Tree[slot].hProgram = InsertVar(Tree[slot].hRoot, _T("Program"), TI_ICON_PROGRAM);
			} else DeleteChildren(g_hwndVarTree,Tree[slot].hProgram);

			if (!Tree[slot].hAppVar || Tree[slot].model==0) {
				Tree[slot].hAppVar = InsertVar(Tree[slot].hRoot, _T("Application Variable"), TI_ICON_APPVAR);
			} else DeleteChildren(g_hwndVarTree,Tree[slot].hAppVar);

			if (!Tree[slot].hPic || Tree[slot].model==0) {
				Tree[slot].hPic = InsertVar(Tree[slot].hRoot, _T("Picture"), TI_ICON_PIC);
			} else DeleteChildren(g_hwndVarTree,Tree[slot].hPic);

			if (!Tree[slot].hGDB || Tree[slot].model==0) {
				Tree[slot].hGDB = InsertVar(Tree[slot].hRoot, _T("Graph Database"), TI_ICON_GDB);
			} else DeleteChildren(g_hwndVarTree,Tree[slot].hGDB);

			if (!Tree[slot].hString || Tree[slot].model==0) {
				Tree[slot].hString = InsertVar(Tree[slot].hRoot, _T("String"), TI_ICON_STRING);
			} else DeleteChildren(g_hwndVarTree,Tree[slot].hString);

			if (!Tree[slot].hNumber || Tree[slot].model==0) {
				Tree[slot].hNumber = InsertVar(Tree[slot].hRoot, _T("Number"), TI_ICON_NUMBER);
			} else DeleteChildren(g_hwndVarTree,Tree[slot].hNumber);

			if (!Tree[slot].hList || Tree[slot].model==0) {
				Tree[slot].hList = InsertVar(Tree[slot].hRoot, _T("List"), TI_ICON_LIST);
			} else DeleteChildren(g_hwndVarTree,Tree[slot].hList);

			if (!Tree[slot].hMatrix || Tree[slot].model==0) {
				Tree[slot].hMatrix = InsertVar(Tree[slot].hRoot, _T("Matrix"), TI_ICON_MATRIX);
			} else DeleteChildren(g_hwndVarTree,Tree[slot].hMatrix);

			if (!Tree[slot].hGroup || Tree[slot].model==0) {
				Tree[slot].hGroup = InsertVar(Tree[slot].hRoot, _T("Group"), TI_ICON_GROUP);
			} else DeleteChildren(g_hwndVarTree,Tree[slot].hGroup);

			if (!Tree[slot].hEquation || Tree[slot].model==0) {
				Tree[slot].hEquation = InsertVar(Tree[slot].hRoot, _T("Equation"), TI_ICON_EQUATIONS);
			} else DeleteChildren(g_hwndVarTree,Tree[slot].hEquation);

			Tree[slot].model		= calcs[slot].model;
			
			/* Apps are handled outside of the symbol table*/
			state_build_applist(&calcs[slot].cpu,&Tree[slot].applist);
			for(i = 0; i < Tree[slot].applist.count; i++) {
				Tree[slot].hApps[i] = InsertVar(Tree[slot].hApplication,Tree[slot].applist.apps[i].name,TI_ICON_FILE_ARC);
			}
			symlist_t* sym = state_build_symlist_83P(&calcs[slot].cpu, &Tree[slot].sym);
			if (sym) {
				// FIXME
				for(i = 0; (&sym->symbols[i]) <= sym->last; i++) {
					TCHAR tmpstring[64];
					int icon;
					
					/* whether its archived or not */
					/* depends on the page its stored */
					if (sym->symbols[i].page) {
						icon = TI_ICON_FILE_ARC;  //red
					} else {
						icon = TI_ICON_FILE_RAM;  //green
					}
					
					if (Symbol_Name_to_String(&sym->symbols[i], tmpstring)) {
						switch(sym->symbols[i].type_ID) {
							case ProgObj:
							case ProtProgObj:
								Tree[slot].hVars[i] = InsertVar(Tree[slot].hProgram,tmpstring,icon);
								break;
							case AppVarObj:
								Tree[slot].hVars[i] = InsertVar(Tree[slot].hAppVar,tmpstring,icon);
								break;
							case GroupObj:
								Tree[slot].hVars[i] = InsertVar(Tree[slot].hGroup,tmpstring,icon);
								break;
							case PictObj:
								Tree[slot].hVars[i] = InsertVar(Tree[slot].hPic,tmpstring,icon);
								break;
							case GDBObj:
								Tree[slot].hVars[i] = InsertVar(Tree[slot].hGDB,tmpstring,icon);
								break;
							case StrngObj:
								Tree[slot].hVars[i] = InsertVar(Tree[slot].hString,tmpstring,icon);
								break;
							case RealObj:
							case CplxObj:
								Tree[slot].hVars[i] = InsertVar(Tree[slot].hNumber,tmpstring,icon);
								break;
							case ListObj:
							case CListObj:
								Tree[slot].hVars[i] = InsertVar(Tree[slot].hList,tmpstring,icon);
								break;
							case MatObj:
								Tree[slot].hVars[i] = InsertVar(Tree[slot].hMatrix,tmpstring,icon);
								break;
//							case EquObj:
							case EquObj_2:
								Tree[slot].hVars[i] = InsertVar(Tree[slot].hEquation,tmpstring,icon);
								break;
						}
					}
				}
				//free(sym);
			}
			// If no children are found kill parent.
			for(i=0;i<11;i++) {
				if (Tree[slot].hTypes[i]) {
					if (TreeView_GetChild(g_hwndVarTree,Tree[slot].hTypes[i]) == NULL) {
						TreeView_DeleteItem(g_hwndVarTree,Tree[slot].hTypes[i]);
						Tree[slot].hTypes[i] = NULL;
					}
				}
			}
		}
	}
}
	
	
FILEDESCRIPTOR *FillDesc(HTREEITEM hSelect,  FILEDESCRIPTOR *fd) {
	int slot;
	u_int i;
	TCHAR string[MAX_PATH];
	for(slot = 0; slot < MAX_CALCS; slot++) {
		if (Tree[slot].model) {
			
			for(i = 0; i < Tree[slot].applist.count; i++) {
				if (Tree[slot].hApps[i] == hSelect) {
					if (App_Name_to_String(&Tree[slot].applist.apps[i], string)) {
#ifdef WINVER
						StringCbCat(string, sizeof(string), _T(".8xk"));
#else
						strcat(string, ".8xk");
#endif
						MFILE *outfile = ExportApp(slot, NULL, &Tree[slot].applist.apps[i]);
						fd->dwFlags = FD_ATTRIBUTES | FD_FILESIZE;
						fd->dwFileAttributes = FILE_ATTRIBUTE_NORMAL;
						fd->nFileSizeLow = msize(outfile);
#ifdef WINVER
						StringCbCopy(fd->cFileName, sizeof(fd->cFileName), string);
#else
						strcpy(fd->cFileName, string);
#endif
						mclose(outfile);
						return fd;
					}
					_tprintf_s(_T("%s\n"), Tree[slot].applist.apps[i].name);
					return NULL;
				}
			}
			for(i = 0; i < (u_int) (Tree[slot].sym.last - Tree[slot].sym.symbols + 1); i++) {
				if (Tree[slot].hVars[i] == hSelect) {
					if (Symbol_Name_to_String(&Tree[slot].sym.symbols[i], string)) {
#ifdef WINVER
						StringCbCat(string, sizeof(string), _T("."));
						StringCbCat(string, sizeof(string), (const TCHAR *) type_ext[Tree[slot].sym.symbols[i].type_ID]);
#else
						strcat(string, ".");
						strcat(string, (const char *) type_ext[Tree[slot].sym.symbols[i].type_ID]);
#endif
						MFILE *outfile = ExportVar(slot, NULL, &Tree[slot].sym.symbols[i]);
						fd->dwFlags = FD_ATTRIBUTES | FD_FILESIZE;
						fd->dwFileAttributes = FILE_ATTRIBUTE_NORMAL;
						fd->nFileSizeLow = msize(outfile);
#ifdef WINVER
						StringCbCopy(fd->cFileName, sizeof(fd->cFileName), string);
#else
						strcpy(fd->cFileName, string);
#endif
						mclose(outfile);
						return fd;
					}
				}
			}
		}
	}
	return NULL;
}
	
void *FillFileBuffer(HTREEITEM hSelect, void *buf) {
	u_int slot, i, b;
	_TUCHAR *buffer = (_TUCHAR *) buf;
	TCHAR string[64];
	_tprintf_s(_T("Fill file buffer\n"));
	for(slot = 0; slot<MAX_CALCS; slot++) {
		if (Tree[slot].model) {
			_tprintf_s(_T("model found\n"));
			for(i = 0; i < Tree[slot].applist.count; i++) {
				if (Tree[slot].hApps[i]==hSelect) {
					MFILE *outfile = ExportApp(slot, NULL, &Tree[slot].applist.apps[i]);
					if(!outfile) _putts(_T("MFile not found"));
					_tprintf_s(_T("size: %d\n"), outfile->size);
					for(b = 0; b < outfile->size; b++) {
						_tprintf_s(_T("%02X"), outfile->data[b]);
						buffer[b] = outfile->data[b];
					}
					_tprintf_s(_T("\n"));
					mclose(outfile);
					return buffer;
				}
			}
			for(i = 0; i < (u_int) (Tree[slot].sym.last - Tree[slot].sym.symbols + 1); i++) {
				if (Tree[slot].hVars[i] == hSelect) {
					if (Symbol_Name_to_String(&Tree[slot].sym.symbols[i], string)) {
						MFILE *outfile = ExportVar(slot,NULL, &Tree[slot].sym.symbols[i]);
						if(!outfile) _putts(_T("MFile not found"));
						_tprintf_s(_T("size: %d\n"), outfile->size);
						for(b=0;b<outfile->size;b++) {
							_tprintf_s(_T("%02X"),outfile->data[b]);
							buffer[b] = outfile->data[b];
						}
						_tprintf_s(_T("\n"));
						mclose(outfile);
						return buffer;
					}
				}
			}
		}
	}
	return NULL;
}
