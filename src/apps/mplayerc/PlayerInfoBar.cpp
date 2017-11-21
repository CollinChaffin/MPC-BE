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
#include "PlayerInfoBar.h"

// CPlayerInfoBar

IMPLEMENT_DYNAMIC(CPlayerInfoBar, CDialogBar)

CPlayerInfoBar::CPlayerInfoBar(CMainFrame* pMainFrame, int nFirstColWidth)
	: m_pMainFrame(pMainFrame)
	, m_nFirstColWidth(nFirstColWidth)
{
}

CPlayerInfoBar::~CPlayerInfoBar()
{
}

void CPlayerInfoBar::SetLine(CString label, CString info)
{
	if (info.IsEmpty()) {
		RemoveLine(label);
		return;
	}

	for (size_t idx = 0; idx < m_labels.GetCount(); idx++) {
		CString tmp;
		m_labels[idx]->GetWindowTextW(tmp);

		if (label == tmp) {
			m_infos[idx]->GetWindowTextW(tmp);

			if (info != tmp) {
				m_infos[idx]->SetWindowText(info);
			}

			return;
		}
	}

	CAutoPtr<CStatusLabel> l(DNew CStatusLabel(true, false));
	l->Create(label, WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS|SS_OWNERDRAW, CRect(0,0,0,0), this);
	m_labels.Add(l);

	CAutoPtr<CStatusLabel> i(DNew CStatusLabel(false, true));
	i->Create(info, WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS|SS_OWNERDRAW, CRect(0,0,0,0), this);
	m_infos.Add(i);

	Relayout();
}

void CPlayerInfoBar::GetLine(CString label, CString& info)
{
	info.Empty();

	for (size_t idx = 0; idx < m_labels.GetCount(); idx++) {
		CString tmp;
		m_labels[idx]->GetWindowTextW(tmp);

		if (label == tmp) {
			m_infos[idx]->GetWindowTextW(tmp);
			info = tmp;

			return;
		}
	}
}

void CPlayerInfoBar::RemoveLine(CString label)
{
	for (size_t i = 0; i < m_labels.GetCount(); i++) {
		CString tmp;
		m_labels[i]->GetWindowTextW(tmp);

		if (label == tmp) {
			m_labels.RemoveAt(i);
			m_infos.RemoveAt(i);

			break;
		}
	}

	Relayout();
}

void CPlayerInfoBar::RemoveAllLines()
{
	m_labels.RemoveAll();
	m_infos.RemoveAll();

	Relayout();
}

void CPlayerInfoBar::ScaleFont()
{
	for (size_t i = 0; i < m_labels.GetCount(); i++) {
		m_labels[i]->ScaleFont();
	}
	for (size_t i = 0; i < m_infos.GetCount(); i++) {
		m_infos[i]->ScaleFont();
	}
}

BOOL CPlayerInfoBar::Create(CWnd* pParentWnd)
{
	return CDialogBar::Create(pParentWnd, IDD_PLAYERINFOBAR, WS_CHILD | WS_VISIBLE | CBRS_ALIGN_BOTTOM, IDD_PLAYERINFOBAR);
}

BOOL CPlayerInfoBar::PreCreateWindow(CREATESTRUCT& cs)
{
	if (!CDialogBar::PreCreateWindow(cs)) {
		return FALSE;
	}

	m_dwStyle &= ~CBRS_BORDER_TOP;
	m_dwStyle &= ~CBRS_BORDER_BOTTOM;

	return TRUE;
}

CSize CPlayerInfoBar::CalcFixedLayout(BOOL bStretch, BOOL bHorz)
{
	CRect r;
	GetParent()->GetClientRect(&r);

	r.bottom = r.top + m_labels.GetCount() * m_pMainFrame->ScaleY(17) + (m_labels.GetCount() ? m_pMainFrame->ScaleY(2) * 2 : 0);

	return r.Size();
}

void CPlayerInfoBar::Relayout()
{
	CRect r;
	GetParent()->GetClientRect(&r);

	int w = m_pMainFrame->ScaleY(m_nFirstColWidth);
	int h = m_pMainFrame->ScaleY(17);
	int y = m_pMainFrame->ScaleY(2);

	for (size_t i = 0; i < m_labels.GetCount(); i++) {
		CDC* pDC = m_labels[i]->GetDC();
		CString str;
		m_labels[i]->GetWindowTextW(str);
		w = std::max(w, (int)pDC->GetTextExtent(str).cx);
		m_labels[i]->ReleaseDC(pDC);
	}

	const int sep = m_pMainFrame->ScaleX(10);
	for (size_t i = 0; i < m_labels.GetCount(); i++, y += h) {
		m_labels[i]->MoveWindow(1, y, w - sep, h);
		m_infos[i]->MoveWindow(w + sep, y, r.Width() - w - sep - 1, h);
	}
}

BEGIN_MESSAGE_MAP(CPlayerInfoBar, CDialogBar)
	ON_WM_ERASEBKGND()
	ON_WM_SIZE()
	ON_WM_LBUTTONDOWN()
END_MESSAGE_MAP()

// CPlayerInfoBar message handlers

BOOL CPlayerInfoBar::OnEraseBkgnd(CDC* pDC)
{
	for (CWnd* pChild = GetWindow(GW_CHILD); pChild; pChild = pChild->GetNextWindow()) {
		CRect r;
		pChild->GetClientRect(&r);
		pChild->MapWindowPoints(this, &r);
		pDC->ExcludeClipRect(&r);
	}

	CRect r;
	GetClientRect(&r);

	if (m_pMainFrame->m_pLastBar != this || m_pMainFrame->m_bFullScreen) {
		r.InflateRect(0, 0, 0, 1);
	}

	if (m_pMainFrame->m_bFullScreen) {
		r.InflateRect(1, 0, 1, 0);
	}

	if (AfxGetAppSettings().bUseDarkTheme) {
		CPen penBlend(PS_SOLID,0,RGB(0,0,0));
		CPen *penSaved = pDC->SelectObject(&penBlend);
		pDC->MoveTo(r.left,r.top);
		pDC->LineTo(r.right,r.top);
		pDC->SelectObject(&penSaved);
		r.DeflateRect(0,1,0,0);

		int R, G, B;
		ThemeRGB(5, 10, 15, R, G, B);
		pDC->FillSolidRect(&r, RGB(R,G,B));
	} else {
		pDC->Draw3dRect(&r, GetSysColor(COLOR_3DSHADOW), GetSysColor(COLOR_3DHILIGHT));

		r.DeflateRect(1, 1);

		pDC->FillSolidRect(&r, 0);
	}

	return TRUE;
}

void CPlayerInfoBar::OnSize(UINT nType, int cx, int cy)
{
	CDialogBar::OnSize(nType, cx, cy);

	Relayout();

	Invalidate();
}

void CPlayerInfoBar::OnLButtonDown(UINT nFlags, CPoint point)
{
	if (!m_pMainFrame->m_bFullScreen) {
		MapWindowPoints(m_pMainFrame, &point, 1);
		m_pMainFrame->PostMessageW(WM_NCLBUTTONDOWN, HTCAPTION, MAKELPARAM(point.x, point.y));
	}
}
