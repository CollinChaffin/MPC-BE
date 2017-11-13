/*
 * (C) 2008-2017 see Authors.txt
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

#include "../mplayerc/MpcApi.h"
#include "HScrollListBox.h"

// CRegisterCopyDataDlg dialog

class CRegisterCopyDataDlg : public CDialog
{
	// Construction
public:
	HWND m_RemoteWindow;
	CRegisterCopyDataDlg(CWnd* pParent = nullptr);

	// Dialog Data
	//{{AFX_DATA(CRegisterCopyDataDlg)
	enum { IDD = IDD_REGISTERCOPYDATA_DIALOG };
	// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CRegisterCopyDataDlg)
protected:
	virtual void DoDataExchange(CDataExchange* pDX);
	//}}AFX_VIRTUAL

	// Implementation
protected:
	HICON	m_hIcon;
	HWND	m_hWndMPC;

	// Generated message map functions
	//{{AFX_MSG(CRegisterCopyDataDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnButtonFindwindow();
	afx_msg BOOL OnCopyData(CWnd* pWnd, COPYDATASTRUCT* pCopyDataStruct);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
public:
	CString		m_strMPCPath;
	CHScrollListBox m_listBox;
	CString		m_txtCommand;
	CComboBox	m_cbCommand;
	afx_msg		void OnBnClickedButtonSendcommand();
	void		Senddata(MPCAPI_COMMAND nCmd, LPCWSTR strCommand);
};
