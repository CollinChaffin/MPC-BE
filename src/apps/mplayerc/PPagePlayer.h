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

#include "PPageBase.h"


// CPPagePlayer dialog

class CPPagePlayer : public CPPageBase
{
	DECLARE_DYNAMIC(CPPagePlayer)

public:
	CPPagePlayer();
	virtual ~CPPagePlayer();

	int  m_iMultipleInst;
	int  m_iTitleBarTextStyle;
	BOOL m_bTitleBarTextTitle;
	BOOL m_bKeepHistory;
	int  m_nRecentFiles;
	BOOL m_bRememberDVDPos;
	BOOL m_bRememberFilePos;
	BOOL m_bRememberWindowPos;
	BOOL m_bRememberWindowSize;
	BOOL m_bSavePnSZoom;
	BOOL m_bRememberPlaylistItems;
	BOOL m_bTrayIcon;
	BOOL m_bShowOSD;
	BOOL m_bOSDFileName;
	BOOL m_bOSDSeekTime;
	BOOL m_bLimitWindowProportions;
	BOOL m_bSnapToDesktopEdges;
	BOOL m_bUseIni;
	BOOL m_bHideCDROMsSubMenu;
	BOOL m_bPriority;

	CSpinButtonCtrl m_RecentFilesCtrl;

	enum { IDD = IDD_PPAGEPLAYER };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL OnInitDialog();
	virtual BOOL OnApply();

	DECLARE_MESSAGE_MAP()

public:
	afx_msg void OnUpdateTimeout(CCmdUI* pCmdUI);
	afx_msg void OnUpdateCheck13(CCmdUI* pCmdUI);
	afx_msg void OnUpdatePos(CCmdUI* pCmdUI);
	afx_msg void OnUpdateOSD(CCmdUI* pCmdUI);
	afx_msg void OnKillFocusEdit1();
	afx_msg void OnChangeEdit1();
};
