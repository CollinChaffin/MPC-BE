/*
 * (C) 2003-2006 Gabest
 * (C) 2006-2017 see Authors.txt
 *
 * This file is part of MPC-BE.
 *
 * MPC-BE is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * MPC-BE is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "stdafx.h"
#include "MainFrm.h"
#include "PlayerChildView.h"
#include "OpenImage.h"

// CChildView

CChildView::CChildView()
{
	LoadLogo();
}

CChildView::~CChildView()
{
}

int CChildView::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CWnd::OnCreate(lpCreateStruct) == -1) {
		return -1;
	}

	int digitizerStatus = GetSystemMetrics(SM_DIGITIZER);
	if ((digitizerStatus & (NID_READY + NID_MULTI_INPUT))) {
		DLog(L"CChildView::OnCreate() : touch is ready for input + support multiple inputs");
		if (!RegisterTouchWindow()) {
			DLog(L"CChildView::OnCreate() : RegisterTouchWindow() failed");
		}
	}

	return 0;
}

BOOL CChildView::PreCreateWindow(CREATESTRUCT& cs)
{
	if (!CWnd::PreCreateWindow(cs)) {
		return FALSE;
	}

	cs.style &= ~WS_BORDER;
	cs.lpszClass = AfxRegisterWndClass(CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS, ::LoadCursor(nullptr, IDC_ARROW), HBRUSH(COLOR_WINDOW + 1), nullptr);

	return TRUE;
}

BOOL CChildView::OnTouchInput(CPoint pt, int nInputNumber, int nInputsCount, PTOUCHINPUT pInput)
{
	return AfxGetMainFrame()->OnTouchInput(pt, nInputNumber, nInputsCount, pInput);
}

BOOL CChildView::PreTranslateMessage(MSG* pMsg)
{
	if (pMsg->message >= WM_MOUSEFIRST && pMsg->message <= WM_MYMOUSELAST) {
		CWnd* pParent = GetParent();
		CPoint point(pMsg->lParam);
		::MapWindowPoints(pMsg->hwnd, pParent->m_hWnd, &point, 1);

		const bool fInteractiveVideo = AfxGetMainFrame()->IsInteractiveVideo();
		if (fInteractiveVideo &&
				(pMsg->message == WM_LBUTTONDOWN || pMsg->message == WM_LBUTTONUP || pMsg->message == WM_MOUSEMOVE)) {
			if (pMsg->message == WM_MOUSEMOVE) {
				VERIFY(pParent->PostMessageW(pMsg->message, pMsg->wParam, MAKELPARAM(point.x, point.y)));
			}
		} else {
			VERIFY(pParent->PostMessageW(pMsg->message, pMsg->wParam, MAKELPARAM(point.x, point.y)));
			return TRUE;
		}
	}

	return CWnd::PreTranslateMessage(pMsg);
}

void CChildView::SetVideoRect(CRect r)
{
	m_vrect = r;
	Invalidate();
}

void CChildView::LoadLogo()
{
	CAppSettings& s = AfxGetAppSettings();
	bool bHaveLogo = false;

	CAutoLock cAutoLock(&m_csLogo);

	m_logo.Destroy();

	CString logoFName = L"logo";

	if (m_logo.FileExists(logoFName, true)) {
		m_logo.Attach(OpenImage(logoFName));
	} else {
		if (s.bLogoExternal) {
			m_logo.Attach(OpenImage(s.strLogoFileName));
			if (m_logo) {
				bHaveLogo = true;
			}
		}

		if (!bHaveLogo) {
			s.bLogoExternal = false;
			s.strLogoFileName.Empty();

			if (!m_logo.LoadFromResource(s.nLogoId)) {
				m_logo.LoadFromResource(s.nLogoId = DEF_LOGO);
			}
		}
	}

	if (m_hWnd) {
		Invalidate();
	}
}

CSize CChildView::GetLogoSize() const
{
	return m_logo.GetSize();
}

IMPLEMENT_DYNAMIC(CChildView, CWnd)

BEGIN_MESSAGE_MAP(CChildView, CWnd)
	ON_WM_PAINT()
	ON_WM_ERASEBKGND()
	ON_WM_SIZE()
	ON_WM_SETCURSOR()
	ON_WM_NCHITTEST()
	ON_WM_NCLBUTTONDOWN()
	ON_WM_CREATE()
END_MESSAGE_MAP()

// CChildView message handlers

void CChildView::OnPaint()
{
	CPaintDC dc(this);

	AfxGetMainFrame()->RepaintVideo();
}

BOOL CChildView::OnEraseBkgnd(CDC* pDC)
{
	CAutoLock cAutoLock(&m_csLogo);

	CRect r;
	GetClientRect(r);

	auto pFrame = AfxGetMainFrame();

	COLORREF bkcolor = 0;
	CImage img;
	if (pFrame->IsD3DFullScreenMode() ||
			((pFrame->m_eMediaLoadState != MLS_LOADED || pFrame->m_bAudioOnly) && !pFrame->m_bNextIsOpened)) {
		if (!pFrame->m_InternalImage.IsNull()) {
			img.Attach(pFrame->m_InternalImage);
		} else if (!m_logo.IsNull()) {
			img.Attach(m_logo);
			bkcolor = m_logo.GetPixel(0,0);
		}
	} else {
		pDC->ExcludeClipRect(m_vrect);
	}

	if (!img.IsNull()) {
		BITMAP bm = { 0 };
		if (GetObject(img, sizeof(bm), &bm)) {
			BLENDFUNCTION bf;
			bf.AlphaFormat = AC_SRC_ALPHA;
			bf.BlendFlags = 0;
			bf.BlendOp = AC_SRC_OVER;
			bf.SourceConstantAlpha = 0xFF;

			int h = std::min(abs(bm.bmHeight), (LONG)r.Height());
			int w = min(r.Width(), MulDiv(h, bm.bmWidth, abs(bm.bmHeight)));
			h = MulDiv(w, abs(bm.bmHeight), bm.bmWidth);
			int x = (r.Width() - w) / 2;
			int y = (r.Height() - h) / 2;
			r = CRect(CPoint(x, y), CSize(w, h));

			int oldmode = pDC->SetStretchBltMode(STRETCH_HALFTONE);
			img.StretchBlt(*pDC, r, CRect(0, 0, bm.bmWidth, abs(bm.bmHeight)));
			pDC->SetStretchBltMode(oldmode);
			pDC->AlphaBlend(x, y, w, h, pDC, x, y, w, h, bf);

			pDC->ExcludeClipRect(r);
		}
		img.Detach();
	}

	GetClientRect(r);
	pDC->FillSolidRect(r, bkcolor);

	return TRUE;
}

void CChildView::OnSize(UINT nType, int cx, int cy)
{
	__super::OnSize(nType, cx, cy);

	auto pFrame = AfxGetMainFrame();
	pFrame->MoveVideoWindow();
	pFrame->UpdateThumbnailClip();
}

BOOL CChildView::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
	auto pFrame = AfxGetMainFrame();

	if (pFrame->m_bHideCursor) {
		SetCursor(nullptr);
		return TRUE;
	}

	if (pFrame->IsSomethingLoaded() && (nHitTest == HTCLIENT)) {
		if (pFrame->GetPlaybackMode() == PM_DVD) {
			return FALSE;
		}

		::SetCursor(AfxGetApp()->LoadStandardCursor(IDC_ARROW));

		return TRUE;
	}

	return CWnd::OnSetCursor(pWnd, nHitTest, message);
}

LRESULT CChildView::OnNcHitTest(CPoint point)
{
	UINT nHitTest = CWnd::OnNcHitTest(point);

	auto pFrame = AfxGetMainFrame();

	WINDOWPLACEMENT wp;
	pFrame->GetWindowPlacement(&wp);

	if (!pFrame->m_bFullScreen && wp.showCmd != SW_SHOWMAXIMIZED && AfxGetAppSettings().iCaptionMenuMode == MODE_BORDERLESS) {
		CRect rcClient, rcFrame;
		GetWindowRect(&rcFrame);
		rcClient = rcFrame;

		CSize sizeBorder(GetSystemMetrics(SM_CXBORDER), GetSystemMetrics(SM_CYBORDER));

		rcClient.InflateRect(-(5 * sizeBorder.cx), -(5 * sizeBorder.cy));
		rcFrame.InflateRect(sizeBorder.cx, sizeBorder.cy);

		if (rcFrame.PtInRect(point)) {
			if (point.x > rcClient.right) {
				if (point.y < rcClient.top) {
					nHitTest = HTTOPRIGHT;
				} else if (point.y > rcClient.bottom) {
					nHitTest = HTBOTTOMRIGHT;
				} else {
					nHitTest = HTRIGHT;
				}
			} else if (point.x < rcClient.left) {
				if (point.y < rcClient.top) {
					nHitTest = HTTOPLEFT;
				} else if (point.y > rcClient.bottom) {
					nHitTest = HTBOTTOMLEFT;
				} else {
					nHitTest = HTLEFT;
				}
			} else if (point.y < rcClient.top) {
				nHitTest = HTTOP;
			} else if (point.y > rcClient.bottom) {
				nHitTest = HTBOTTOM;
			}
		}
	}

	return nHitTest;
}

void CChildView::OnNcLButtonDown(UINT nHitTest, CPoint point)
{
	auto pFrame = AfxGetMainFrame();
	bool fLeftMouseBtnUnassigned = !AssignedToCmd(wmcmd::LDOWN);

	if (!pFrame->m_bFullScreen && (pFrame->IsCaptionHidden() || fLeftMouseBtnUnassigned)) {
		BYTE bFlag = 0;
		switch (nHitTest) {
			case HTTOP:
				bFlag = WMSZ_TOP;
				break;
			case HTTOPLEFT:
				bFlag = WMSZ_TOPLEFT;
				break;
			case HTTOPRIGHT:
				bFlag = WMSZ_TOPRIGHT;
				break;
			case HTLEFT:
				bFlag = WMSZ_LEFT;
				break;
			case HTRIGHT:
				bFlag = WMSZ_RIGHT;
				break;
			case HTBOTTOM:
				bFlag = WMSZ_BOTTOM;
				break;
			case HTBOTTOMLEFT:
				bFlag = WMSZ_BOTTOMLEFT;
				break;
			case HTBOTTOMRIGHT:
				bFlag = WMSZ_BOTTOMRIGHT;
				break;
		}

		if (bFlag) {
			pFrame->PostMessageW(WM_SYSCOMMAND, (SC_SIZE | bFlag), (LPARAM)POINTTOPOINTS(point));
		} else {
			CWnd::OnNcLButtonDown(nHitTest, point);
		}
	}
}
