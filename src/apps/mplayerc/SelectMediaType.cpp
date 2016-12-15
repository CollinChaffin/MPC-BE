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

#include "stdafx.h"
#include "SelectMediaType.h"

CString GetMediaTypeName(const GUID& guid)
{
	CString ret = (guid == GUID_NULL) ? L"Any type" : GetGUIDString(guid);

	return ret;
}

// CSelectMediaType dialog

IMPLEMENT_DYNAMIC(CSelectMediaType, CCmdUIDialog)
CSelectMediaType::CSelectMediaType(CAtlArray<GUID>& guids, GUID guid, CWnd* pParent)
	: CCmdUIDialog(CSelectMediaType::IDD, pParent)
	, m_guids(guids), m_guid(guid)
{
	m_guidstr = CStringFromGUID(guid);
}

CSelectMediaType::~CSelectMediaType()
{
}

void CSelectMediaType::DoDataExchange(CDataExchange* pDX)
{
	__super::DoDataExchange(pDX);

	DDX_CBString(pDX, IDC_COMBO1, m_guidstr);
	DDX_Control(pDX, IDC_COMBO1, m_guidsctrl);
}

BEGIN_MESSAGE_MAP(CSelectMediaType, CCmdUIDialog)
	ON_CBN_EDITCHANGE(IDC_COMBO1, OnCbnEditchangeCombo1)
	ON_UPDATE_COMMAND_UI(IDOK, OnUpdateOK)
END_MESSAGE_MAP()

// CSelectMediaType message handlers

BOOL CSelectMediaType::OnInitDialog()
{
	CCmdUIDialog::OnInitDialog();

	for (size_t i = 0; i < m_guids.GetCount(); i++) {
		m_guidsctrl.AddString(GetMediaTypeName(m_guids[i]));
	}

	return TRUE;
}

void CSelectMediaType::OnCbnEditchangeCombo1()
{
	UpdateData();

	int i = m_guidsctrl.FindStringExact(0, m_guidstr);

	if (i >= 0) {
		DWORD sel = m_guidsctrl.GetEditSel();
		m_guidsctrl.SetCurSel(i);
		m_guidsctrl.SetEditSel(sel,sel);
	}
}

void CSelectMediaType::OnUpdateOK(CCmdUI* pCmdUI)
{
	UpdateData();

	pCmdUI->Enable(!m_guidstr.IsEmpty() && (m_guidsctrl.GetCurSel() >= 0 || GUIDFromCString(m_guidstr) != GUID_NULL));
}

void CSelectMediaType::OnOK()
{
	UpdateData();

	int i = m_guidsctrl.GetCurSel();

	m_guid = i >= 0 ? m_guids[i] : GUIDFromCString(m_guidstr);

	CCmdUIDialog::OnOK();
}
