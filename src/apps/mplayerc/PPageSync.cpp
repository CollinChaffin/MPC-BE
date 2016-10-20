/*
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
#include "MainFrm.h"
#include "../../DSUtil/SysVersion.h"
#include "PPageSync.h"


IMPLEMENT_DYNAMIC(CPPageSync, CPPageBase)

CPPageSync::CPPageSync()
	: CPPageBase(CPPageSync::IDD, CPPageSync::IDD)
	, m_iSyncMode(0)
	, m_iLineDelta(0)
	, m_iColumnDelta(0)
{
}

CPPageSync::~CPPageSync()
{
}

void CPPageSync::DoDataExchange(CDataExchange* pDX)
{
	__super::DoDataExchange(pDX);

	DDX_Control(pDX, IDC_CHECK1, m_chkVMR9VSync);
	DDX_Control(pDX, IDC_CHECK2, m_chkVMR9VSyncAccurate);
	DDX_Control(pDX, IDC_DSVMR9ALTERNATIVEVSYNC, m_chkVMR9AlterativeVSync);
	DDX_Control(pDX, IDC_EDIT1, m_edtVMR9VSyncOffset);
	DDX_Control(pDX, IDC_SPIN1, m_spnVMR9VSyncOffset);
	DDX_Control(pDX, IDC_CHECK4, m_chkDisableAero);
	DDX_Control(pDX, IDC_CHECK8, m_chkEnableFrameTimeCorrection);
	DDX_Control(pDX, IDC_CHECK5, m_chkVMRFlushGPUBeforeVSync);
	DDX_Control(pDX, IDC_CHECK6, m_chkVMRFlushGPUAfterPresent);
	DDX_Control(pDX, IDC_CHECK7, m_chkVMRFlushGPUWait);

	DDX_Radio(pDX, IDC_RADIO1, m_iSyncMode);
	DDX_Control(pDX, IDC_CYCLEDELTA, m_edtCycleDelta);
	DDX_Text(pDX, IDC_LINEDELTA, m_iLineDelta);
	DDX_Text(pDX, IDC_COLUMNDELTA, m_iColumnDelta);
	DDX_Control(pDX, IDC_TARGETSYNCOFFSET, m_edtTargetSyncOffset);
	DDX_Control(pDX, IDC_CONTROLLIMIT, m_edtControlLimit);
}

BOOL CPPageSync::OnInitDialog()
{
	__super::OnInitDialog();

	InitDialogPrivate();

	CreateToolTip();

	return TRUE;
}

BOOL CPPageSync::OnSetActive()
{
	InitDialogPrivate();

	return __super::OnSetActive();
}

void CPPageSync::InitDialogPrivate()
{
	CAppSettings& s = AfxGetAppSettings();
	CRenderersSettings& rs = s.m_RenderersSettings;

	CMainFrame * pFrame;
	pFrame = (CMainFrame *)(AfxGetApp()->m_pMainWnd);

	m_chkVMR9VSync.SetCheck(rs.bVSync);
	m_chkVMR9VSyncAccurate.SetCheck(rs.bVSyncAccurate);
	m_chkVMR9AlterativeVSync.SetCheck(rs.bAlterativeVSync);
	m_spnVMR9VSyncOffset.SetRange(-20, 20);
	m_edtVMR9VSyncOffset.SetRange(-20, 20);
	m_edtVMR9VSyncOffset = rs.iVSyncOffset;
	m_chkDisableAero.SetCheck(rs.bDisableDesktopComposition);
	m_chkEnableFrameTimeCorrection.SetCheck(rs.bEVRFrameTimeCorrection);
	m_chkVMRFlushGPUBeforeVSync.SetCheck(rs.bFlushGPUBeforeVSync);
	m_chkVMRFlushGPUAfterPresent.SetCheck(rs.bFlushGPUAfterPresent);
	m_chkVMRFlushGPUWait.SetCheck(rs.bFlushGPUWait);

	if (s.iVideoRenderer == VIDRNDT_EVR_CUSTOM) {
		m_chkVMR9VSync.EnableWindow(TRUE);
		m_chkVMR9VSyncAccurate.EnableWindow(TRUE);
		m_chkVMR9AlterativeVSync.EnableWindow(TRUE);
		m_chkVMRFlushGPUBeforeVSync.EnableWindow(TRUE);
		m_chkVMRFlushGPUAfterPresent.EnableWindow(TRUE);
		m_chkVMRFlushGPUWait.EnableWindow(TRUE);
	} else {
		m_chkVMR9VSync.EnableWindow(FALSE);
		m_chkVMR9AlterativeVSync.EnableWindow(FALSE);
		m_chkVMR9VSyncAccurate.EnableWindow(FALSE);
		m_chkVMRFlushGPUBeforeVSync.EnableWindow(FALSE);
		m_chkVMRFlushGPUAfterPresent.EnableWindow(FALSE);
		m_chkVMRFlushGPUWait.EnableWindow(FALSE);
	}
	OnAlterativeVSyncCheck();

	if ((IsWinVista() || IsWin7()) &&
			(s.iVideoRenderer == VIDRNDT_EVR_CUSTOM ||
			s.iVideoRenderer == VIDRNDT_MADVR ||
			s.iVideoRenderer == VIDRNDT_SYNC)) {
		m_chkDisableAero.EnableWindow(TRUE);
	} else {
		m_chkDisableAero.EnableWindow(FALSE);
	}

	m_chkEnableFrameTimeCorrection.EnableWindow(s.iVideoRenderer == VIDRNDT_EVR_CUSTOM ? TRUE : FALSE);

	m_iSyncMode = rs.iSynchronizeMode == SYNCHRONIZE_VIDEO ? 0
				: rs.iSynchronizeMode == SYNCHRONIZE_DISPLAY ? 1
				: 2;
	m_iLineDelta = rs.iLineDelta;
	m_iColumnDelta = rs.iColumnDelta;
	m_edtCycleDelta = rs.dCycleDelta;
	m_edtTargetSyncOffset = rs.dTargetSyncOffset;
	m_edtControlLimit = rs.dControlLimit;

	if ((s.iVideoRenderer == VIDRNDT_SYNC) && (pFrame->GetPlaybackMode() == PM_NONE)) {
		GetDlgItem(IDC_RADIO1)->EnableWindow(TRUE);
		GetDlgItem(IDC_RADIO2)->EnableWindow(TRUE);
		GetDlgItem(IDC_RADIO3)->EnableWindow(TRUE);
		GetDlgItem(IDC_TARGETSYNCOFFSET)->EnableWindow(TRUE);
		GetDlgItem(IDC_CONTROLLIMIT)->EnableWindow(TRUE);
		GetDlgItem(IDC_STATIC2)->EnableWindow(TRUE);
		GetDlgItem(IDC_STATIC3)->EnableWindow(TRUE);
		GetDlgItem(IDC_STATIC4)->EnableWindow(TRUE);
		GetDlgItem(IDC_STATIC9)->EnableWindow(TRUE);
		GetDlgItem(IDC_STATIC10)->EnableWindow(TRUE);
		GetDlgItem(IDC_STATIC11)->EnableWindow(TRUE);
		GetDlgItem(IDC_STATIC12)->EnableWindow(TRUE);

		OnSyncModeClicked(m_iSyncMode == 0 ? IDC_RADIO1 : m_iSyncMode == 1 ? IDC_RADIO2 : IDC_RADIO3);
	} else {
		GetDlgItem(IDC_RADIO1)->EnableWindow(FALSE);
		GetDlgItem(IDC_RADIO2)->EnableWindow(FALSE);
		GetDlgItem(IDC_RADIO3)->EnableWindow(FALSE);
		GetDlgItem(IDC_TARGETSYNCOFFSET)->EnableWindow(FALSE);
		GetDlgItem(IDC_CYCLEDELTA)->EnableWindow(FALSE);
		GetDlgItem(IDC_LINEDELTA)->EnableWindow(FALSE);
		GetDlgItem(IDC_COLUMNDELTA)->EnableWindow(FALSE);
		GetDlgItem(IDC_CONTROLLIMIT)->EnableWindow(FALSE);
		GetDlgItem(IDC_STATIC2)->EnableWindow(FALSE);
		GetDlgItem(IDC_STATIC3)->EnableWindow(FALSE);
		GetDlgItem(IDC_STATIC4)->EnableWindow(FALSE);
		GetDlgItem(IDC_STATIC5)->EnableWindow(FALSE);
		GetDlgItem(IDC_STATIC6)->EnableWindow(FALSE);
		GetDlgItem(IDC_STATIC7)->EnableWindow(FALSE);
		GetDlgItem(IDC_STATIC8)->EnableWindow(FALSE);
		GetDlgItem(IDC_STATIC9)->EnableWindow(FALSE);
		GetDlgItem(IDC_STATIC10)->EnableWindow(FALSE);
		GetDlgItem(IDC_STATIC11)->EnableWindow(FALSE);
		GetDlgItem(IDC_STATIC12)->EnableWindow(FALSE);
	}

	UpdateData(FALSE);
	SetModified(FALSE);
}

BOOL CPPageSync::OnApply()
{
	UpdateData();
	CRenderersSettings& rs = GetRenderersSettings();

	rs.bVSync						= !!m_chkVMR9VSync.GetCheck();
	rs.bVSyncAccurate				= !!m_chkVMR9VSyncAccurate.GetCheck();
	rs.bAlterativeVSync				= !!m_chkVMR9AlterativeVSync.GetCheck();
	rs.iVSyncOffset					= m_edtVMR9VSyncOffset;
	rs.bDisableDesktopComposition	= !!m_chkDisableAero.GetCheck();
	rs.bEVRFrameTimeCorrection		= !!m_chkEnableFrameTimeCorrection.GetCheck();
	rs.bFlushGPUBeforeVSync			= !!m_chkVMRFlushGPUBeforeVSync.GetCheck();
	rs.bFlushGPUAfterPresent		= !!m_chkVMRFlushGPUAfterPresent.GetCheck();
	rs.bFlushGPUWait				= !!m_chkVMRFlushGPUWait.GetCheck();

	rs.iSynchronizeMode	= m_iSyncMode == 0 ? SYNCHRONIZE_VIDEO
							: m_iSyncMode == 1 ? SYNCHRONIZE_DISPLAY
							: SYNCHRONIZE_NEAREST;
	rs.iLineDelta			= m_iLineDelta;
	rs.iColumnDelta			= m_iColumnDelta;
	rs.dCycleDelta			= m_edtCycleDelta;
	rs.dTargetSyncOffset	= m_edtTargetSyncOffset;
	rs.dControlLimit		= m_edtControlLimit;

	return __super::OnApply();
}

BEGIN_MESSAGE_MAP(CPPageSync, CPPageBase)
	ON_BN_CLICKED(IDC_DSVMR9ALTERNATIVEVSYNC, OnAlterativeVSyncCheck)
	ON_CONTROL_RANGE(BN_CLICKED, IDC_RADIO1, IDC_RADIO3, OnSyncModeClicked)
END_MESSAGE_MAP()

void CPPageSync::OnAlterativeVSyncCheck()
{
	CAppSettings& s = AfxGetAppSettings();
	CRenderersSettings& rs = s.m_RenderersSettings;

	if (m_chkVMR9AlterativeVSync.GetCheck() == BST_CHECKED &&
			s.iVideoRenderer == VIDRNDT_EVR_CUSTOM) {
		GetDlgItem(IDC_STATIC1)->EnableWindow(TRUE);
		GetDlgItem(IDC_EDIT1)->EnableWindow(TRUE);
		m_spnVMR9VSyncOffset.EnableWindow(TRUE);
	} else {
		GetDlgItem(IDC_STATIC1)->EnableWindow(FALSE);
		GetDlgItem(IDC_EDIT1)->EnableWindow(FALSE);
		m_spnVMR9VSyncOffset.EnableWindow(FALSE);
	}
	SetModified();
}

void CPPageSync::OnSyncModeClicked(UINT nID)
{
	if (nID == IDC_RADIO1) {
		GetDlgItem(IDC_STATIC5)->EnableWindow(TRUE);
		GetDlgItem(IDC_CYCLEDELTA)->EnableWindow(TRUE);
	} else {
		GetDlgItem(IDC_STATIC5)->EnableWindow(FALSE);
		GetDlgItem(IDC_CYCLEDELTA)->EnableWindow(FALSE);
	}

	if (nID == IDC_RADIO2) {
		GetDlgItem(IDC_STATIC6)->EnableWindow(TRUE);
		GetDlgItem(IDC_LINEDELTA)->EnableWindow(TRUE);
		GetDlgItem(IDC_STATIC7)->EnableWindow(TRUE);
		GetDlgItem(IDC_COLUMNDELTA)->EnableWindow(TRUE);
		GetDlgItem(IDC_STATIC8)->EnableWindow(TRUE);
	} else {
		GetDlgItem(IDC_STATIC6)->EnableWindow(FALSE);
		GetDlgItem(IDC_LINEDELTA)->EnableWindow(FALSE);
		GetDlgItem(IDC_STATIC7)->EnableWindow(FALSE);
		GetDlgItem(IDC_COLUMNDELTA)->EnableWindow(FALSE);
		GetDlgItem(IDC_STATIC8)->EnableWindow(FALSE);
	}
}
