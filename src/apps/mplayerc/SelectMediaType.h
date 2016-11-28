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

#include <afxwin.h>
#include <ResizableLib/ResizableDialog.h>

CString GetMediaTypeName(const GUID& guid);

// CSelectMediaType dialog

class CSelectMediaType : public CCmdUIDialog
{
	DECLARE_DYNAMIC(CSelectMediaType)

private:
	CAtlArray<GUID>& m_guids;

public:
	CSelectMediaType(CAtlArray<GUID>& guids, GUID guid, CWnd* pParent = NULL);
	virtual ~CSelectMediaType();

	GUID m_guid;

	enum { IDD = IDD_SELECTMEDIATYPE };
	CString m_guidstr;
	CComboBox m_guidsctrl;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL OnInitDialog();
	virtual void OnOK();

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnCbnEditchangeCombo1();
	afx_msg void OnUpdateOK(CCmdUI* pCmdUI);
};
