/*
 * (C) 2016 see Authors.txt
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
#include "MFCHelper.h"

CString ResStr(UINT nID)
{
	CString id;
	if (!id.LoadString(nID)) {
		id.LoadString(AfxGetApp()->m_hInstance, nID);
	}

	return id;
}

void SetCursor(HWND m_hWnd, LPCTSTR lpCursorName)
{
	SetClassLongPtr(m_hWnd, GCLP_HCURSOR, (LONG_PTR)AfxGetApp()->LoadStandardCursor(lpCursorName));
}

void SetCursor(HWND m_hWnd, UINT nID, LPCTSTR lpCursorName)
{
	SetCursor(::GetDlgItem(m_hWnd, nID), lpCursorName);
}

void CorrectComboListWidth(CComboBox& ComboBox)
{
	if (ComboBox.GetCount() <= 0)
		return;

	CString    str;
	CSize      sz;
	TEXTMETRIC tm;
	int        dx		= 0;
	CDC*       pDC		= ComboBox.GetDC();
	CFont*     pFont	= ComboBox.GetFont();

	// Select the listbox font, save the old font
	CFont* pOldFont = pDC->SelectObject(pFont);
	// Get the text metrics for avg char width
	pDC->GetTextMetrics(&tm);

	// Find the longest string in the combo box.
	for (int i = 0; i < ComboBox.GetCount(); i++) {
		ComboBox.GetLBText(i, str);
		sz = pDC->GetTextExtent(str);

		// Add the avg width to prevent clipping
		sz.cx += tm.tmAveCharWidth;

		if (sz.cx > dx) {
			dx = sz.cx;
		}
	}
	// Select the old font back into the DC
	pDC->SelectObject(pOldFont);
	ComboBox.ReleaseDC(pDC);

	// Get the scrollbar width if it exists
	int min_visible = ComboBox.GetMinVisible();
	int scroll_width = (ComboBox.GetCount() > min_visible) ?
					   ::GetSystemMetrics(SM_CXVSCROLL) : 0;

	// Adjust the width for the vertical scroll bar and the left and right border.
	dx += scroll_width + 2*::GetSystemMetrics(SM_CXEDGE);

	// Set the width of the list box so that every item is completely visible.
	ComboBox.SetDroppedWidth(dx);
}

void CorrectCWndWidth(CWnd* pWnd)
{
	if (!pWnd) {
		return;
	}

	CDC*   pDC = pWnd->GetDC();
	CFont* pFont = pWnd->GetFont();
	CFont* pOldFont = pDC->SelectObject(pFont);

	CString str;
	pWnd->GetWindowText(str);
	CSize szText = pDC->GetTextExtent(str);

	TEXTMETRIC tm;
	pDC->GetTextMetrics(&tm);
	pDC->SelectObject(pOldFont);
	pWnd->ReleaseDC(pDC);

	CRect r;
	pWnd->GetWindowRect(r);
	pWnd->GetOwner()->ScreenToClient(r);

	r.right = r.left + ::GetSystemMetrics(SM_CXMENUCHECK) + szText.cx + tm.tmAveCharWidth;
	pWnd->MoveWindow(r);
}

void SelectByItemData(CComboBox& ComboBox, int data)
{
	for (int i = 0; i < ComboBox.GetCount(); i++) {
		if (ComboBox.GetItemData(i) == (DWORD_PTR)data) {
			ComboBox.SetCurSel(i);
			break;
		}
	}
}

void SelectByItemData(CListBox& ListBox, int data)
{
	for (int i = 0; i < ListBox.GetCount(); i++) {
		if (ListBox.GetItemData(i) == (DWORD_PTR)data) {
			ListBox.SetCurSel(i);
			ListBox.SetTopIndex(i);
			break;
		}
	}
}

inline int GetCurItemData(CComboBox& ComboBox)
{
	return (int)ComboBox.GetItemData(ComboBox.GetCurSel());
}

inline int GetCurItemData(CListBox& ListBox)
{
	return (int)ListBox.GetItemData(ListBox.GetCurSel());
}

inline void AddStringData(CComboBox& ComboBox, LPCTSTR str, int data)
{
	ComboBox.SetItemData(ComboBox.AddString(str), (DWORD_PTR)data);
}

inline void AddStringData(CListBox& ListBox, LPCTSTR str, int data)
{
	ListBox.SetItemData(ListBox.AddString(str), (DWORD_PTR)data);
}
