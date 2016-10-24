/*
 * (C) 2012-2016 see Authors.txt
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

#pragma once

class CMainFrame;

// CPreView

class CPreView : public CWnd
{
	DECLARE_DYNAMIC(CPreView)

private:
	CMainFrame* m_pMainFrame;

	const int m_border = 5;

	int m_caption = 20;
	int m_relativeSize = 15;

	CString	m_tooltipstr;
	CWnd	m_view;
	CRect	m_videorect;

public:
	CPreView(CMainFrame* pMainFrame);
	virtual ~CPreView();

	virtual BOOL SetWindowText(LPCWSTR lpString);

	void SetRelativeSize(int relativesize) { m_relativeSize = relativesize; }
	void GetVideoRect(LPRECT lpRect);
	HWND GetVideoHWND();

	void SetWindowSize();

protected:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);

	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnPaint();
	afx_msg void OnShowWindow(BOOL bShow, UINT nStatus);

	DECLARE_MESSAGE_MAP()
};
