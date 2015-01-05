#ifndef WIN7TASKBARPROGRESS_H
#define WIN7TASKBARPROGRESS_H

#ifndef __ITaskbarList3_INTERFACE_DEFINED__

typedef enum TBPFLAG
{
	TBPF_NOPROGRESS	= 0,
	TBPF_INDETERMINATE	= 0x1,
	TBPF_NORMAL	= 0x2,
	TBPF_ERROR	= 0x4,
	TBPF_PAUSED	= 0x8
} TBPFLAG;

MIDL_INTERFACE("ea1afb91-9e28-4b86-90e9-9e9f8a5eefaf")
ITaskbarList3 : public ITaskbarList2
{
public:
	virtual HRESULT STDMETHODCALLTYPE SetProgressValue(
			HWND hwnd,
			ULONGLONG ullCompleted,
			ULONGLONG ullTotal) = 0;

	virtual HRESULT STDMETHODCALLTYPE SetProgressState(
			HWND hwnd,
			TBPFLAG tbpFlags) = 0;

	virtual HRESULT STDMETHODCALLTYPE RegisterTab(
			HWND hwndTab,
			HWND hwndMDI) = 0;

	virtual HRESULT STDMETHODCALLTYPE UnregisterTab(
			HWND hwndTab) = 0;

	virtual HRESULT STDMETHODCALLTYPE SetTabOrder(
			HWND hwndTab,
			HWND hwndInsertBefore) = 0;

	virtual HRESULT STDMETHODCALLTYPE SetTabActive(
			HWND hwndTab,
			HWND hwndMDI,
			DWORD dwReserved) = 0;

	virtual HRESULT STDMETHODCALLTYPE ThumbBarAddButtons(
			HWND hwnd,
			UINT cButtons,
			/*LPTHUMBBUTTON*/ LPVOID pButton) = 0;

	virtual HRESULT STDMETHODCALLTYPE ThumbBarUpdateButtons(
			HWND hwnd,
			UINT cButtons,
			/*LPTHUMBBUTTON*/ LPVOID pButton) = 0;

	virtual HRESULT STDMETHODCALLTYPE ThumbBarSetImageList(
			HWND hwnd,
			HIMAGELIST himl) = 0;

	virtual HRESULT STDMETHODCALLTYPE SetOverlayIcon(
			HWND hwnd,
			HICON hIcon,
			LPCWSTR pszDescription) = 0;

	virtual HRESULT STDMETHODCALLTYPE SetThumbnailTooltip(
			HWND hwnd,
			LPCWSTR pszTip) = 0;

	virtual HRESULT STDMETHODCALLTYPE SetThumbnailClip(
			HWND hwnd,
			RECT *prcClip) = 0;
};
#endif


class Win7TaskbarProgress
{
	//copy from http://stackoverflow.com/questions/15001638/windows-7-taskbar-state-with-minimal-code
public:
	Win7TaskbarProgress();
	virtual ~Win7TaskbarProgress();

	void SetProgressState(HWND hwnd, TBPFLAG flag);
	void SetProgressValue(HWND hwnd, ULONGLONG ullCompleted, ULONGLONG ullTotal);

private:
	bool Init();
	ITaskbarList3* m_pITaskBarList3;
	bool m_bFailed;
};

extern Win7TaskbarProgress win7TaskbarProgress;

#endif
