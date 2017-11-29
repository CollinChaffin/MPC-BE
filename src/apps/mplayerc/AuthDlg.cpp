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
#include "AuthDlg.h"

// CAuthDlg dialog

IMPLEMENT_DYNAMIC(CAuthDlg, CDialog)
CAuthDlg::CAuthDlg(CWnd* pParent)
	: CDialog(CAuthDlg::IDD, pParent)
	, m_remember(FALSE)
{
}

CAuthDlg::~CAuthDlg()
{
}

void CAuthDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);

	DDX_Control(pDX, IDC_COMBO1, m_usernamectrl);
	DDX_Text(pDX, IDC_COMBO1, m_username);
	DDX_Text(pDX, IDC_EDIT3, m_password);
	DDX_Check(pDX, IDC_CHECK1, m_remember);
}

CString CAuthDlg::DEncrypt(CString str)
{
	for (int i = 0; i < str.GetLength(); i++) {
		str.SetAt(i, str[i]^5);
	}

	return str;
}

BEGIN_MESSAGE_MAP(CAuthDlg, CDialog)
	ON_BN_CLICKED(IDOK, OnBnClickedOk)
	ON_CBN_SELCHANGE(IDC_COMBO1, OnCbnSelchangeCombo1)
	ON_EN_SETFOCUS(IDC_EDIT3, OnEnSetfocusEdit3)
END_MESSAGE_MAP()

// CAuthDlg message handlers

BOOL CAuthDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	CWinApp* pApp = AfxGetApp();

	if (pApp->m_pszRegistryKey) {
		CRegKey hSecKey(pApp->GetSectionKey(IDS_R_LOGINS));

		if (hSecKey) {
			int i = 0;
			WCHAR username[256], password[256];

			for (;;) {
				DWORD unlen = _countof(username);
				DWORD pwlen = sizeof(password);
				DWORD type = REG_SZ;

				if (ERROR_SUCCESS == RegEnumValueW(hSecKey, i++, username, &unlen, 0, &type, (BYTE*)password, &pwlen)) {
					m_logins[username] = DEncrypt(password);
					m_usernamectrl.AddString(username);
				} else {
					break;
				}
			}
		}
	} else {
		CAutoVectorPtr<WCHAR> buff;
		buff.Allocate(32767/sizeof(WCHAR));

		DWORD len = GetPrivateProfileSectionW(IDS_R_LOGINS, buff, 32767/sizeof(WCHAR), pApp->m_pszProfileName);

		WCHAR* p = buff;
		while (*p && len > 0) {
			CString str = p;
			p += str.GetLength()+1;
			len -= str.GetLength()+1;
			CAtlList<CString> sl;
			Explode(str, sl, L'=', 2);

			if (sl.GetCount() == 2) {
				m_logins[sl.GetHead()] = DEncrypt(sl.GetTail());
				m_usernamectrl.AddString(sl.GetHead());
			}
		}
	}

	m_usernamectrl.SetFocus();

	return TRUE;
}

void CAuthDlg::OnBnClickedOk()
{
	UpdateData();

	if (!m_username.IsEmpty()) {
		AfxGetMyApp()->WriteProfileString(IDS_R_LOGINS, m_username, m_remember ? DEncrypt(m_password) : L"");
	}

	OnOK();
}

void CAuthDlg::OnCbnSelchangeCombo1()
{
	CString username;
	m_usernamectrl.GetLBText(m_usernamectrl.GetCurSel(), username);

	CString password;

	if (m_logins.Lookup(username, password)) {
		m_password = password;
		m_remember = TRUE;
		UpdateData(FALSE);
	}
}

void CAuthDlg::OnEnSetfocusEdit3()
{
	UpdateData();

	CString password;

	if (m_logins.Lookup(m_username, password)) {
		m_password = password;
		m_remember = TRUE;
		UpdateData(FALSE);
	}
}
