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

#pragma once

#include "FloatEdit.h"
#include <ResizableLib/ResizableDialog.h>


// CPnSPresetsDlg dialog

class CPnSPresetsDlg : public CCmdUIDialog
{
	DECLARE_DYNAMIC(CPnSPresetsDlg)

private:
	void StringToParams(CString str, CString& label, double& PosX, double& PosY, double& ZoomX, double& ZoomY);
	CString ParamsToString(CString label, double PosX, double PosY, double ZoomX, double ZoomY);

public:
	CPnSPresetsDlg(CWnd* pParent = nullptr);
	virtual ~CPnSPresetsDlg();

	CStringArray m_pnspresets;

	enum { IDD = IDD_PNSPRESET_DLG };
	CFloatEdit m_PosX;
	CFloatEdit m_PosY;
	CFloatEdit m_ZoomX;
	CFloatEdit m_ZoomY;
	CString m_label;
	CListBox m_list;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL OnInitDialog();
	virtual void OnOK();

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnLbnSelchangeList1();
	afx_msg void OnBnClickedButton2();
	afx_msg void OnUpdateButton2(CCmdUI* pCmdUI);
	afx_msg void OnBnClickedButton6();
	afx_msg void OnUpdateButton6(CCmdUI* pCmdUI);
	afx_msg void OnBnClickedButton9();
	afx_msg void OnUpdateButton9(CCmdUI* pCmdUI);
	afx_msg void OnBnClickedButton10();
	afx_msg void OnUpdateButton10(CCmdUI* pCmdUI);
	afx_msg void OnBnClickedButton1();
	afx_msg void OnUpdateButton1(CCmdUI* pCmdUI);
};
