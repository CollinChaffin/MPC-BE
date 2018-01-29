/*
 * (C) 2003-2006 Gabest
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

#include <ResizableLib/ResizableDialog.h>

// CFavoriteAddDlg dialog

class CFavoriteAddDlg : public CCmdUIDialog
{
	DECLARE_DYNAMIC(CFavoriteAddDlg)

private:
	CString m_fullname;
	std::list<CString> m_shortnames;

public:
	CFavoriteAddDlg(std::list<CString>& shortnames, CString fullname, BOOL bShowRelativeDrive = TRUE, CWnd* pParent = nullptr);
	virtual ~CFavoriteAddDlg();

	enum { IDD = IDD_FAVADD };

	CComboBox m_namectrl;
	CString m_name;
	BOOL m_bRememberPos;
	BOOL m_bRelativeDrive;

	BOOL m_bShowRelativeDrive;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL OnInitDialog();

	DECLARE_MESSAGE_MAP()

public:
	afx_msg void OnUpdateOk(CCmdUI* pCmdUI);
protected:
	virtual void OnOK();
};
