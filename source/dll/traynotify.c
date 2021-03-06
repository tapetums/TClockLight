/*-------------------------------------------------------------
  traynotify.c : fill background of tray in taskbar
  (C) 1997-2003 Kazuto Sato
  Please read readme.txt about the license.
  
  Written by Kazubon, Nanashi-san
---------------------------------------------------------------*/

#include "tcdll.h"
//#include "newapi.h"

#if TC_ENABLE_TRAYNOTIFY

/* Statics */

static LRESULT CALLBACK SubclassProcTrayNotify(HWND, UINT, WPARAM, LPARAM, UINT_PTR, DWORD_PTR);
static void OnPaintTray(HWND hwnd, HDC hdc);
static void OnCustomDraw(HWND hwnd, HWND hwndFrom, const LPNMCUSTOMDRAW pnmcd);

static HWND m_hwndTrayNotify = NULL, m_hwndToolbar = NULL;
static HWND m_hwndClock = NULL;
static LONG m_oldClassStyle, m_oldStyle;


/*--------------------------------------------------
  initialize
----------------------------------------------------*/
void InitTrayNotify(HWND hwndClock)
{
	HWND hwnd;
	
	EndTrayNotify();
	
	if(GetMyRegLong(NULL, "NoClock", FALSE)) return;
	
	if(GetMyRegLong(NULL, "FillTray", FALSE) == FALSE) return;
	
	if(GetMyRegLong(NULL, "UseBackColor", TRUE) == FALSE) return;
	
	m_hwndClock = hwndClock;
	m_hwndTrayNotify = GetParent(hwndClock);  // TrayNotifyWnd
	
	if(IsSubclassed(m_hwndTrayNotify)) return;
	
	/* ---------- search toolbar ----------- */
	
	m_hwndToolbar = NULL;
	hwnd = FindWindowEx(m_hwndTrayNotify, NULL, "ToolbarWindow32", NULL);
	if(!hwnd)
	{
		hwnd = FindWindowEx(m_hwndTrayNotify, NULL, "SysPager", NULL);
		if(hwnd)
			hwnd = FindWindowEx(hwnd, NULL, "ToolbarWindow32", NULL);
	}
	m_hwndToolbar = hwnd;
	
	/* ---------- subclassify ------------ */
	
	m_oldClassStyle = GetClassLong(m_hwndTrayNotify, GCL_STYLE);
	SetClassLong(m_hwndTrayNotify, GCL_STYLE,
		m_oldClassStyle|CS_HREDRAW|CS_VREDRAW);
	
	SetWindowSubclass(m_hwndTrayNotify, SubclassProcTrayNotify, 0, 0);
	
	m_oldStyle = GetWindowLong(m_hwndTrayNotify, GWL_STYLE);
	SetWindowLong(m_hwndTrayNotify, GWL_STYLE,
		m_oldStyle & ~(WS_CLIPCHILDREN|WS_CLIPSIBLINGS));
	
	InvalidateRect(m_hwndTrayNotify, NULL, TRUE);
	
	if(m_hwndToolbar)
	{
		SendMessage(GetParent(m_hwndToolbar), WM_SYSCOLORCHANGE, 0, 0);
	}
}

/*--------------------------------------------------
  clear up
----------------------------------------------------*/
void EndTrayNotify(void)
{
	if(m_hwndTrayNotify && IsWindow(m_hwndTrayNotify))
	{
		SetWindowLong(m_hwndTrayNotify, GWL_STYLE, m_oldStyle);
		
		RemoveWindowSubclass(m_hwndTrayNotify, SubclassProcTrayNotify, 0);
		
		SetClassLong(m_hwndTrayNotify, GCL_STYLE, m_oldClassStyle);
		
		InvalidateRect(m_hwndTrayNotify, NULL, TRUE);
		
		if(m_hwndToolbar)
		{
			SendMessage(m_hwndToolbar, WM_SYSCOLORCHANGE, 0, 0);
			InvalidateRect(m_hwndToolbar, NULL, TRUE);
		}
	}
	
	m_hwndTrayNotify = NULL;
	m_hwndToolbar = NULL;
}

/*------------------------------------------------
   subclass procedure of TrayNotifyWnd
--------------------------------------------------*/
LRESULT CALLBACK SubclassProcTrayNotify(HWND hwnd, UINT message,
	WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
	switch(message)
	{
		case WM_ERASEBKGND:
			OnPaintTray(hwnd, (HDC)wParam);
			return 1;
		case WM_PAINT:
		{
			PAINTSTRUCT ps;
			HDC hdc;
			hdc = BeginPaint(hwnd, &ps);
			OnPaintTray(hwnd, hdc);
			EndPaint(hwnd, &ps);
			return 0;
		}
		case WM_PRINTCLIENT:
			OnPaintTray(hwnd, (HDC)wParam);
			return 0;
		case WM_SIZE:
			if(m_hwndClock) SendMessage(m_hwndClock, WM_SIZE, 0, 0);
			break;
		case WM_NOTIFY:
		{
			LPNMHDR pnmh;
			pnmh = (LPNMHDR)lParam;
			if(pnmh->code == NM_CUSTOMDRAW && pnmh->idFrom == 0)
			{
				LPNMCUSTOMDRAW pnmcd = (LPNMCUSTOMDRAW)lParam;
				if(pnmcd->dwDrawStage  == CDDS_ITEMPREPAINT)
					OnCustomDraw(hwnd, pnmh->hwndFrom, pnmcd);
			}
			break;
		}
	}
	return DefSubclassProc(hwnd, message, wParam, lParam);
}

/*------------------------------------------------
   paint background of TrayNotifyWnd
--------------------------------------------------*/
void OnPaintTray(HWND hwnd, HDC hdc)
{
	HDC hdcClockBack;
	RECT rc;
	
	hdcClockBack = GetClockBackDC();
	if(!hdcClockBack) return;
	
	GetClientRect(hwnd, &rc);
	BitBlt(hdc, 0, 0, rc.right, rc.bottom,
		hdcClockBack, 0, 0, SRCCOPY);
}

/*------------------------------------------------
   paint background of Toolbar
--------------------------------------------------*/
void OnCustomDraw(HWND hwnd, HWND hwndFrom, const LPNMCUSTOMDRAW pnmcd)
{
	HDC hdcClockBack;
	POINT ptTray, ptToolbar;
	int x, y;
	
	hdcClockBack = GetClockBackDC();
	if(!hdcClockBack) return;
	
	ptTray.x = ptTray.y = 0;
	ClientToScreen(hwnd, &ptTray);
	
	ptToolbar.x = ptToolbar.y = 0;
	ClientToScreen(hwndFrom, &ptToolbar);
	
	x = ptToolbar.x - ptTray.x;
	y = ptToolbar.y - ptTray.y;
	
	BitBlt(pnmcd->hdc, pnmcd->rc.left, pnmcd->rc.top,
		pnmcd->rc.right - pnmcd->rc.left,
		pnmcd->rc.bottom - pnmcd->rc.top,
		hdcClockBack, x + pnmcd->rc.left, y + pnmcd->rc.top,
		SRCCOPY);
}

#endif	/* TC_ENABLE_TRAYNOTIFY */
