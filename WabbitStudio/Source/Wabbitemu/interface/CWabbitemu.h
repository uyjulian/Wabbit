#pragma once

#include "calc.h"
#include "CZ80.h"
#include "CLCD.h"
#include "CKeypad.h"
#include "CLabelServer.h"

#include "savestate.h"

struct MainWindow;
typedef Calc_Model CALC_MODEL;

#include "CBreakpointCollection.h"

class ATL_NO_VTABLE CWabbitemu :
	public CComObjectRootEx<CComObjectThreadModel>,
	public CComCoClass<CWabbitemu, &CLSID_Wabbitemu>,
	public IProvideClassInfo2Impl<&CLSID_Wabbitemu, &DIID_DWabbitemuEvents, &LIBID_WabbitemuLib, 1, 0>,
	public IConnectionPointContainerImpl<CWabbitemu>,
	public IConnectionPointImpl<CWabbitemu, &DIID_DWabbitemuEvents>,
	public IPersistStreamInit,
	public IDispatchImpl<IWabbitemu, &IID_IWabbitemu, &LIBID_WabbitemuLib, 1, 0>
{
public:
	DECLARE_REGISTRY_RESOURCEID(IDR_WABBITEMU)

	BEGIN_COM_MAP(CWabbitemu)
		COM_INTERFACE_ENTRY(IDispatch)
		COM_INTERFACE_ENTRY(IWabbitemu)
		COM_INTERFACE_ENTRY(IProvideClassInfo2)
		COM_INTERFACE_ENTRY(IProvideClassInfo)
		COM_INTERFACE_ENTRY(IPersist)
		COM_INTERFACE_ENTRY(IPersistStreamInit)
		COM_INTERFACE_ENTRY(IConnectionPointContainer)
	END_COM_MAP()

	BEGIN_CONNECTION_POINT_MAP(CWabbitemu)
		CONNECTION_POINT_ENTRY(DIID_DWabbitemuEvents)
	END_CONNECTION_POINT_MAP()

	HRESULT FinalConstruct();
	void FinalRelease();

	// IWabbitemu methods
	STDMETHODIMP put_Visible(VARIANT_BOOL fVisible);
	STDMETHODIMP get_Visible(VARIANT_BOOL *lpVisible);

	STDMETHODIMP get_CPU(IZ80 **ppZ80);
	STDMETHODIMP get_LCD(ILCD **ppLCD);

	STDMETHOD(get_Memory)(IMemoryContext **ppMemC);

	STDMETHOD(get_Running)(VARIANT_BOOL *lpfRunning);
	STDMETHOD(get_Model)(CALC_MODEL *lpCalcModel);

	STDMETHODIMP Step();
	STDMETHODIMP StepOver();
	STDMETHOD(Run)();
	STDMETHOD(Break)();

	STDMETHOD(get_Breakpoints)(IBreakpointCollection **pBC);

	STDMETHODIMP LoadFile(BSTR bstrFileName);

	STDMETHODIMP get_Apps(SAFEARRAY **ppAppList);
	STDMETHOD(get_Keypad)(IKeypad **ppKeypad);
	STDMETHODIMP get_Labels(ILabelServer **ppLabelServer);

	//IPersistStreamInit
	STDMETHOD(GetClassID)(CLSID *pclsid)
	{
		*pclsid = CLSID_Wabbitemu;
		return S_OK;
	}

	STDMETHOD(IsDirty)()
	{
		return S_FALSE;
	}

	STDMETHOD(Load)(LPSTREAM pStm)
	{
		SAVESTATE_t *sav;
		//load_save
		return E_NOTIMPL;
	}

	STDMETHOD(Save)(LPSTREAM, BOOL)
	{
		return E_NOTIMPL;
	}

	STDMETHOD(GetSizeMax)(ULARGE_INTEGER *)
	{
		return E_NOTIMPL;
	}

	STDMETHOD(InitNew)()
	{
		return E_NOTIMPL;
	}

	void Fire_OnBreakpoint(waddr *pwaddr);
	void Fire_OnClose();

private:
	DWORD m_dwThreadId;
	static DWORD CALLBACK WabbitemuThread(LPVOID lpParam);

	int m_iSlot;
	VARIANT_BOOL m_fVisible;
	struct MainWindow *m_lpMainWindow;
	LPCALC m_lpCalc;
	CComPtr<IZ80> m_pZ80;
	CComPtr<IMemoryContext> m_pMem;
	CComPtr<ILCD> m_pLCD;
	CComPtr<IKeypad> m_pKeypad;
	HWND m_hwnd;
	UINT_PTR m_idTimer;

	CComObject<CBreakpointCollection> *m_pBreakpointCollObj;

	//CComObject<CLabelServer> m_LabelServer;
};

OBJECT_ENTRY_AUTO(CLSID_Wabbitemu, CWabbitemu)
