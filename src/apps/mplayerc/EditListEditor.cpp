/*
 * (C) 2006-2018 see Authors.txt
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
#include "EditListEditor.h"

CClip::CClip()
{
	m_rtIn	= INVALID_TIME;
	m_rtOut	= INVALID_TIME;
}

void CClip::SetIn(LPCTSTR strVal)
{
	m_rtIn = StringToReftime (strVal);
}

void CClip::SetOut(LPCTSTR strVal)
{
	m_rtOut = StringToReftime (strVal);
}

void CClip::SetIn  (REFERENCE_TIME rtVal)
{
	m_rtIn  = rtVal;

	if (m_rtIn > m_rtOut) {
		m_rtOut = INVALID_TIME;
	}
};

void CClip::SetOut (REFERENCE_TIME rtVal)
{
	m_rtOut = rtVal;

	if (m_rtIn > m_rtOut) {
		m_rtIn = INVALID_TIME;
	}
};

CString CClip::GetIn()
{
	if (m_rtIn == INVALID_TIME) {
		return L"";
	} else {
		return ReftimeToString(m_rtIn);
	}
}

CString CClip::GetOut()
{
	if (m_rtOut == INVALID_TIME) {
		return L"";
	} else {
		return ReftimeToString(m_rtOut);
	}
}

IMPLEMENT_DYNAMIC(CEditListEditor, CPlayerBar)
CEditListEditor::CEditListEditor(void)
{
	m_CurPos		= nullptr;
	m_bDragging		= FALSE;
	m_nDragIndex	= -1;
	m_nDropIndex	= -1;
	m_pDragImage	= nullptr;
	m_bFileOpen		= false;
}

CEditListEditor::~CEditListEditor(void)
{
	SaveEditListToFile();
}

BEGIN_MESSAGE_MAP(CEditListEditor, CPlayerBar)
	ON_WM_SIZE()
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_EDITLIST, OnLvnItemchanged)
	ON_NOTIFY(LVN_KEYDOWN, IDC_EDITLIST, OnLvnKeyDown)
	ON_WM_DRAWITEM()
	ON_NOTIFY(LVN_BEGINDRAG, IDC_EDITLIST, OnBeginDrag)
	ON_WM_MOUSEMOVE()
	ON_WM_LBUTTONUP()
	ON_WM_TIMER()
	ON_NOTIFY(LVN_BEGINLABELEDIT, IDC_EDITLIST, OnBeginlabeleditList)
	ON_NOTIFY(LVN_DOLABELEDIT, IDC_EDITLIST, OnDolabeleditList)
	ON_NOTIFY(LVN_ENDLABELEDIT, IDC_EDITLIST, OnEndlabeleditList)
END_MESSAGE_MAP()

BOOL CEditListEditor::Create(CWnd* pParentWnd, UINT defDockBarID)
{
	if (!__super::Create(ResStr(IDS_EDIT_LIST_EDITOR), pParentWnd, ID_VIEW_EDITLISTEDITOR, defDockBarID, L"Edit List Editor")) {
		return FALSE;
	}

	m_stUsers.Create(L"User :", WS_VISIBLE | WS_CHILD, CRect (5,5,100,21), this, 0);
	m_cbUsers.Create(WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST, CRect (90,0, 260, 21), this, 0);
	FillCombo(L"Users.txt", m_cbUsers, false);

	m_stHotFolders.Create(L"Hot folder :", WS_VISIBLE | WS_CHILD, CRect (5,35,100,51), this, 0);
	m_cbHotFolders.Create(WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST, CRect (90,30, 260, 21), this, 0);
	FillCombo(L"HotFolders.txt", m_cbHotFolders, true);

	m_list.CreateEx(
		WS_EX_DLGMODALFRAME | WS_EX_CLIENTEDGE,
		WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_TABSTOP
		| LVS_OWNERDRAWFIXED
		| LVS_REPORT | LVS_SINGLESEL | LVS_AUTOARRANGE | LVS_NOSORTHEADER,
		CRect(0,0,100,100), this, IDC_EDITLIST);

	m_list.SetExtendedStyle(m_list.GetExtendedStyle() | LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);

	m_list.InsertColumn(COL_IN,	 L"Nb.",  LVCFMT_LEFT, 35);
	m_list.InsertColumn(COL_IN,	 L"In",  LVCFMT_LEFT, 100);
	m_list.InsertColumn(COL_OUT, L"Out", LVCFMT_LEFT, 100);
	m_list.InsertColumn(COL_NAME, L"Name", LVCFMT_LEFT, 150);

	m_fakeImageList.Create(1, 16, ILC_COLOR4, 10, 10);
	m_list.SetImageList(&m_fakeImageList, LVSIL_SMALL);

	return TRUE;
}

void CEditListEditor::OnSize(UINT nType, int cx, int cy)
{
	CSizingControlBarG::OnSize(nType, cx, cy);

	ResizeListColumn();
}

void CEditListEditor::ResizeListColumn()
{
	if (::IsWindow(m_list.m_hWnd)) {
		CRect r;
		GetClientRect(r);
		r.DeflateRect(2, 2);

		r.top += 60;
		m_list.SetRedraw(FALSE);
		m_list.MoveWindow(r);
		m_list.GetClientRect(r);
		m_list.SetRedraw(TRUE);
	}
}

void CEditListEditor::SaveEditListToFile()
{
	if ((m_bFileOpen || m_EditList.GetCount() >0) && !m_strFileName.IsEmpty()) {
		CStdioFile		EditListFile;

		if (EditListFile.Open (m_strFileName, CFile::modeCreate|CFile::modeWrite)) {
			CString			strLine;
			int				nIndex;
			CString			strUser;
			CString			strHotFolders;

			nIndex = m_cbUsers.GetCurSel();
			if (nIndex >= 0) {
				m_cbUsers.GetLBText(nIndex, strUser);
			}

			nIndex = m_cbHotFolders.GetCurSel();
			if (nIndex >= 0) {
				m_cbHotFolders.GetLBText(nIndex, strHotFolders);
			}

			POSITION pos = m_EditList.GetHeadPosition();

			for (int i = 0; pos; i++, m_EditList.GetNext(pos)) {
				CClip&	CurClip = m_EditList.GetAt(pos);

				if (CurClip.HaveIn() && CurClip.HaveOut()) {
					strLine.Format(L"%s\t%s\t%s\t%s\t%s\n", CurClip.GetIn(), CurClip.GetOut(), CurClip.GetName(), strUser, strHotFolders);
					EditListFile.WriteString (strLine);
				}
			}

			EditListFile.Close();
		}
	}
}

void CEditListEditor::CloseFile()
{
	SaveEditListToFile();
	m_EditList.RemoveAll();
	m_list.DeleteAllItems();
	m_CurPos		= nullptr;
	m_strFileName	= "";
	m_bFileOpen		= false;
	m_cbHotFolders.SetCurSel(0);
}

void CEditListEditor::OpenFile(LPCTSTR lpFileName)
{
	CString			strLine;
	CStdioFile		EditListFile;
	CString			strUser;
	CString			strHotFolders;

	CloseFile();
	m_strFileName.Format(L"%s.edl", lpFileName);

	if (EditListFile.Open (m_strFileName, CFile::modeRead)) {
		m_bFileOpen = true;

		while (EditListFile.ReadString(strLine)) {
			//int		nPos = 0;
			CString		strIn;		//	= strLine.Tokenize(L" \t", nPos);
			CString		strOut;		//	= strLine.Tokenize(L" \t", nPos);
			CString		strName;	//	= strLine.Tokenize(L" \t", nPos);

			AfxExtractSubString (strIn,			strLine, 0, L'\t');
			AfxExtractSubString (strOut,		strLine, 1, L'\t');
			AfxExtractSubString (strName,		strLine, 2, L'\t');
			if (strUser.IsEmpty()) {
				AfxExtractSubString (strUser,		 strLine, 3, L'\t');
				SelectCombo(strUser, m_cbUsers);
			}

			if (strHotFolders.IsEmpty()) {
				AfxExtractSubString (strHotFolders, strLine, 4, L'\t');
				SelectCombo(strHotFolders, m_cbHotFolders);
			}

			if (!strIn.IsEmpty() && !strOut.IsEmpty()) {
				CClip	NewClip;
				NewClip.SetIn  (strIn);
				NewClip.SetOut (strOut);
				NewClip.SetName(strName);

				InsertClip (nullptr, NewClip);
			}
		}

		EditListFile.Close();
	} else {
		m_bFileOpen = false;
	}

	if (m_NameList.empty()) {
		CStdioFile NameFile;
		CString str;

		if (NameFile.Open (L"EditListNames.txt", CFile::modeRead)) {
			while (NameFile.ReadString(str)) {
				m_NameList.push_back(str);
			}

			NameFile.Close();
		}
	}
}

void CEditListEditor::SetIn (REFERENCE_TIME rtIn)
{
	if (m_CurPos != nullptr) {
		CClip&		CurClip = m_EditList.GetAt (m_CurPos);

		CurClip.SetIn (rtIn);
		m_list.Invalidate();
	}
}

void CEditListEditor::SetOut(REFERENCE_TIME rtOut)
{
	if (m_CurPos != nullptr) {
		CClip&		CurClip = m_EditList.GetAt (m_CurPos);

		CurClip.SetOut (rtOut);
		m_list.Invalidate();
	}
}

void CEditListEditor::NewClip(REFERENCE_TIME rtVal)
{
	CClip		NewClip;

	if (m_CurPos != nullptr) {
		CClip&		CurClip = m_EditList.GetAt (m_CurPos);

		if (CurClip.HaveIn()) {
			if (!CurClip.HaveOut()) {
				CurClip.SetOut (rtVal);
			}
		}
	}

	m_CurPos = InsertClip (m_CurPos, NewClip);
	m_list.Invalidate();
}

void CEditListEditor::Save()
{
	SaveEditListToFile();
}

int CEditListEditor::FindIndex(POSITION pos)
{
	int			iItem	= 0;
	POSITION	CurPos	= m_EditList.GetHeadPosition();

	while (CurPos && CurPos != pos) {
		m_EditList.GetNext (CurPos);
		iItem++;
	}

	return iItem;
}

POSITION CEditListEditor::InsertClip(POSITION pos, CClip& NewClip)
{
	LVITEM		lv;
	POSITION	NewClipPos;

	if (pos == nullptr) {
		NewClipPos = m_EditList.AddTail (NewClip);
	} else {
		NewClipPos = m_EditList.InsertAfter (pos, NewClip);
	}

	lv.mask		= LVIF_STATE | LVIF_TEXT;
	lv.iItem	= FindIndex (pos);
	lv.iSubItem	= 0;
	lv.pszText	= L"";
	lv.state	= m_list.GetItemCount()==0 ? LVIS_SELECTED : 0;
	m_list.InsertItem(&lv);

	return NewClipPos;
}

void CEditListEditor::OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct)
{
	if (nIDCtl != IDC_EDITLIST) {
		return;
	}

	int			nItem		= lpDrawItemStruct->itemID;
	CRect		rcItem		= lpDrawItemStruct->rcItem;
	POSITION	pos			= m_EditList.FindIndex(nItem);

	if (pos != nullptr) {
		bool		fSelected	= (pos == m_CurPos);
		UNREFERENCED_PARAMETER(fSelected);
		CClip&		CurClip		= m_EditList.GetAt(pos);
		CString		strTemp;

		CDC* pDC = CDC::FromHandle(lpDrawItemStruct->hDC);

		if (!!m_list.GetItemState(nItem, LVIS_SELECTED)) {
			FillRect(pDC->m_hDC, rcItem, CBrush(0xf1dacc));
			FrameRect(pDC->m_hDC, rcItem, CBrush(0xc56a31));
		} else {
			FillRect(pDC->m_hDC, rcItem, CBrush(GetSysColor(COLOR_WINDOW)));
		}

		COLORREF	textcolor = RGB(0,0,0);

		if (!CurClip.HaveIn() || !CurClip.HaveOut()) {
			textcolor = RGB(255,0,0);
		}

		for (int i=0; i<COL_MAX; i++) {
			m_list.GetSubItemRect(nItem, i, LVIR_LABEL, rcItem);
			pDC->SetTextColor(textcolor);
			switch (i) {
				case COL_INDEX :
					strTemp.Format (L"%d", nItem+1);
					pDC->DrawText (strTemp, rcItem, DT_CENTER | DT_VCENTER);
					break;
				case COL_IN :
					pDC->DrawText (CurClip.GetIn(), rcItem, DT_CENTER | DT_VCENTER);
					break;
				case COL_OUT :
					pDC->DrawText (CurClip.GetOut(), rcItem, DT_CENTER | DT_VCENTER);
					break;
				case COL_NAME :
					pDC->DrawText (CurClip.GetName(), rcItem, DT_LEFT | DT_VCENTER);
					break;
			}
		}
	}
}

void CEditListEditor::OnLvnItemchanged(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);

	if (pNMLV->iItem >= 0) {
		m_CurPos = m_EditList.FindIndex (pNMLV->iItem);
	}
}

void CEditListEditor::OnLvnKeyDown(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMLVKEYDOWN pLVKeyDown = reinterpret_cast<LPNMLVKEYDOWN>(pNMHDR);

	*pResult = FALSE;

	if (pLVKeyDown->wVKey == VK_DELETE) {
		POSITION	pos = m_list.GetFirstSelectedItemPosition();
		POSITION	ClipPos;
		int			nItem = -1;

		while (pos) {
			nItem	= m_list.GetNextSelectedItem(pos);
			ClipPos = m_EditList.FindIndex (nItem);

			if (ClipPos) {
				m_EditList.RemoveAt (ClipPos);
			}

			m_list.DeleteItem (nItem);
		}

		if (nItem != -1) {
			m_list.SetItemState(std::min(nItem, m_list.GetItemCount()-1), LVIS_SELECTED, LVIS_SELECTED);
		}

		m_list.Invalidate();
	}
}

void CEditListEditor::OnBeginDrag(NMHDR* pNMHDR, LRESULT* pResult)
{
	ModifyStyle(WS_EX_ACCEPTFILES, 0);

	m_nDragIndex = ((LPNMLISTVIEW)pNMHDR)->iItem;

	CPoint p(0, 0);
	m_pDragImage = m_list.CreateDragImageEx(&p);

	CPoint p2 = ((LPNMLISTVIEW)pNMHDR)->ptAction;

	m_pDragImage->BeginDrag(0, p2 - p);
	m_pDragImage->DragEnter(GetDesktopWindow(), ((LPNMLISTVIEW)pNMHDR)->ptAction);

	m_bDragging = TRUE;
	m_nDropIndex = -1;

	SetCapture();
}

void CEditListEditor::OnMouseMove(UINT nFlags, CPoint point)
{
	if (m_bDragging) {
		m_ptDropPoint = point;
		ClientToScreen(&m_ptDropPoint);

		m_pDragImage->DragMove(m_ptDropPoint);
		m_pDragImage->DragShowNolock(FALSE);

		WindowFromPoint(m_ptDropPoint)->ScreenToClient(&m_ptDropPoint);

		m_pDragImage->DragShowNolock(TRUE);

		{
			int iOverItem = m_list.HitTest(m_ptDropPoint);
			int iTopItem = m_list.GetTopIndex();
			int iBottomItem = m_list.GetBottomIndex();

			if (iOverItem == iTopItem && iTopItem != 0) { // top of list
				SetTimer(1, 100, nullptr);
			} else {
				KillTimer(1);
			}

			if (iOverItem >= iBottomItem && iBottomItem != (m_list.GetItemCount() - 1)) { // bottom of list
				SetTimer(2, 100, nullptr);
			} else {
				KillTimer(2);
			}
		}
	}

	__super::OnMouseMove(nFlags, point);
}

void CEditListEditor::OnTimer(UINT_PTR nIDEvent)
{
	int iTopItem = m_list.GetTopIndex();
	int iBottomItem = iTopItem + m_list.GetCountPerPage() - 1;

	if (m_bDragging) {
		m_pDragImage->DragShowNolock(FALSE);

		if (nIDEvent == 1) {
			m_list.EnsureVisible(iTopItem - 1, false);
			m_list.UpdateWindow();

			if (m_list.GetTopIndex() == 0) {
				KillTimer(1);
			}
		} else if (nIDEvent == 2) {
			m_list.EnsureVisible(iBottomItem + 1, false);
			m_list.UpdateWindow();

			if (m_list.GetBottomIndex() == (m_list.GetItemCount() - 1)) {
				KillTimer(2);
			}
		}

		m_pDragImage->DragShowNolock(TRUE);
	}

	__super::OnTimer(nIDEvent);
}

void CEditListEditor::OnLButtonUp(UINT nFlags, CPoint point)
{
	if (m_bDragging) {
		::ReleaseCapture();

		m_bDragging = FALSE;
		m_pDragImage->DragLeave(GetDesktopWindow());
		m_pDragImage->EndDrag();

		delete m_pDragImage;
		m_pDragImage = nullptr;

		KillTimer(1);
		KillTimer(2);

		CPoint pt(point);
		ClientToScreen(&pt);

		if (WindowFromPoint(pt) == &m_list) {
			DropItemOnList();
		}
	}

	ModifyStyle(0, WS_EX_ACCEPTFILES);

	__super::OnLButtonUp(nFlags, point);
}

void CEditListEditor::DropItemOnList()
{
	m_ptDropPoint.y -= 10;
	m_nDropIndex = m_list.HitTest(CPoint(10, m_ptDropPoint.y));

	POSITION	DragPos		= m_EditList.FindIndex (m_nDragIndex);
	POSITION	DropPos		= m_EditList.FindIndex (m_nDropIndex);

	if ((DragPos!=nullptr) && (DropPos!=nullptr)) {
		CClip& DragClip	= m_EditList.GetAt(DragPos);
		m_EditList.InsertAfter (DropPos, DragClip);
		m_EditList.RemoveAt (DragPos);
		m_list.Invalidate();
	}
}

void CEditListEditor::OnBeginlabeleditList(NMHDR* pNMHDR, LRESULT* pResult)
{
	LV_DISPINFO* pDispInfo = (LV_DISPINFO*)pNMHDR;
	LV_ITEM* pItem = &pDispInfo->item;

	*pResult = FALSE;

	if (pItem->iItem < 0) {
		return;
	}

	if (pItem->iSubItem == COL_NAME) {
		*pResult = TRUE;
	}
}

void CEditListEditor::OnDolabeleditList(NMHDR* pNMHDR, LRESULT* pResult)
{
	LV_DISPINFO* pDispInfo = (LV_DISPINFO*)pNMHDR;
	LV_ITEM* pItem = &pDispInfo->item;

	*pResult = FALSE;

	if (pItem->iItem < 0) {
		return;
	}

	if (m_CurPos != nullptr && pItem->iSubItem == COL_NAME) {
		CClip& CurClip = m_EditList.GetAt (m_CurPos);
		int nSel = FindNameIndex (CurClip.GetName());

		std::list<CString> sl;
		for (unsigned i = 0; i < m_NameList.size(); i++) {
			sl.push_back(m_NameList[i]);
		}

		m_list.ShowInPlaceComboBox(pItem->iItem, pItem->iSubItem, sl, nSel, true);

		*pResult = TRUE;
	}
}

void CEditListEditor::OnEndlabeleditList(NMHDR* pNMHDR, LRESULT* pResult)
{
	LV_DISPINFO* pDispInfo = (LV_DISPINFO*)pNMHDR;
	LV_ITEM* pItem = &pDispInfo->item;

	*pResult = FALSE;

	if (!m_list.m_fInPlaceDirty) {
		return;
	}

	if (pItem->iItem < 0) {
		return;
	}

	size_t i = pItem->lParam;

	if (i < m_NameList.size() && m_CurPos != nullptr && pItem->iSubItem == COL_NAME) {
		CClip& CurClip = m_EditList.GetAt (m_CurPos);
		CurClip.SetName(m_NameList[i]);

		*pResult = TRUE;
	}
}

int CEditListEditor::FindNameIndex(LPCTSTR strName)
{
	int nResult = -1;

	for (unsigned i = 0; i < m_NameList.size(); i++) {
		CString& CurName = m_NameList[i];

		if (CurName == strName) {
			nResult = i;

			break;
		}
	}

	return nResult;
}

void CEditListEditor::FillCombo(LPCTSTR strFileName, CComboBox& Combo, bool bAllowNull)
{
	CStdioFile NameFile;
	CString	str;
	if (NameFile.Open (strFileName, CFile::modeRead)) {
		if (bAllowNull) {
			Combo.AddString(L"");
		}

		while (NameFile.ReadString(str)) {
			Combo.AddString(str);
		}

		NameFile.Close();
	}
}

void CEditListEditor::SelectCombo(LPCTSTR strValue, CComboBox& Combo)
{
	for (int i=0; i<Combo.GetCount(); i++) {
		CString		strTemp;
		Combo.GetLBText(i, strTemp);

		if (strTemp == strValue) {
			Combo.SetCurSel(i);

			break;
		}
	}
}
