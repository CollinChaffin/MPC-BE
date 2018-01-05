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

#pragma once

#include "PlayerBar.h"
#include "PlayerListCtrl.h"

class CClip
{
private :
	REFERENCE_TIME		m_rtIn;
	REFERENCE_TIME		m_rtOut;
	CString				m_strName;

public :
	CClip();

	bool		HaveIn() {
		return m_rtIn  != INVALID_TIME;
	};

	bool		HaveOut() {
		return m_rtOut != INVALID_TIME;
	};

	void		SetOut (LPCTSTR strVal);
	void		SetIn  (LPCTSTR strVal);
	void		SetIn  (REFERENCE_TIME rtVal);
	void		SetOut (REFERENCE_TIME rtVal);
	void		SetName(LPCTSTR strName) {
		m_strName = strName;
	};

	CString		GetIn();
	CString		GetOut();
	CString		GetName() const {
		return m_strName;
	};
};

class CEditListEditor :	public CPlayerBar
{
	DECLARE_DYNAMIC(CEditListEditor)

	enum {COL_INDEX, COL_IN, COL_OUT, COL_NAME, COL_MAX};

	CPlayerListCtrl	m_list;
	CStatic			m_stUsers;
	CComboBox		m_cbUsers;
	CStatic			m_stHotFolders;
	CComboBox		m_cbHotFolders;
	CImageList		m_fakeImageList;
	POSITION		m_CurPos;
	BOOL			m_bDragging;
	int				m_nDragIndex;
	int				m_nDropIndex;
	CPoint			m_ptDropPoint;
	CImageList*		m_pDragImage;

	CString			m_strFileName;
	bool			m_bFileOpen;
	CList<CClip>	m_EditList;
	std::vector<CString> m_NameList;

	void			SaveEditListToFile();
	void			ResizeListColumn();
	POSITION		InsertClip(POSITION pos, CClip& NewClip);
	void			DropItemOnList();
	int				FindIndex(POSITION pos);
	int				FindNameIndex(LPCTSTR strName);
	void			FillCombo(LPCTSTR strFileName, CComboBox& Combo, bool bAllowNull);
	void			SelectCombo(LPCTSTR strValue, CComboBox& Combo);

protected :
	DECLARE_MESSAGE_MAP()

public:
	CEditListEditor(void);
	~CEditListEditor(void);

	BOOL Create(CWnd* pParentWnd, UINT defDockBarID);

	void CloseFile();
	void OpenFile(LPCTSTR lpFileName);
	void SetIn(REFERENCE_TIME rtIn);
	void SetOut(REFERENCE_TIME rtOut);
	void NewClip(REFERENCE_TIME rtVal);
	void Save();

	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnLvnKeyDown(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct);
	afx_msg void OnLvnItemchanged(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnBeginDrag(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnBeginlabeleditList(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDolabeleditList(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnEndlabeleditList(NMHDR* pNMHDR, LRESULT* pResult);
};
