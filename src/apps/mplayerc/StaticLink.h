/*
 * (C) 2003-2006 Gabest
 * (C) 2006-2016 see Authors.txt
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

// CHyperlink

class CHyperlink : public CString
{
public:
	CHyperlink(LPCTSTR lpLink = NULL) : CString(lpLink) { }
	~CHyperlink() { }
	const CHyperlink& operator=(LPCTSTR lpsz) {
		CString::operator=(lpsz);
		return *this;
	}
	operator LPCTSTR() {
		return CString::operator LPCTSTR();
	}
	/*virtual*/ HINSTANCE Navigate() {
		return IsEmpty() ? NULL :
			   ShellExecute(0, L"open", *this, 0, 0, SW_SHOWNORMAL);
	}
};

// CStaticLink

class CStaticLink : public CStatic
{
public:
	DECLARE_DYNAMIC(CStaticLink)
	CStaticLink(LPCTSTR lpText = NULL, bool bDeleteOnDestroy = false);
	~CStaticLink() { }

	CHyperlink	m_link;
	COLORREF	m_color;

	static COLORREF g_colorUnvisited;
	static COLORREF g_colorVisited;

protected:
	CFont	m_font;
	bool	m_bDeleteOnDestroy;

	virtual void PostNcDestroy();

	DECLARE_MESSAGE_MAP()
	afx_msg LRESULT	OnNcHitTest(CPoint point);
	afx_msg HBRUSH	CtlColor(CDC* pDC, UINT nCtlColor);
	afx_msg void	OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg BOOL	OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
};
