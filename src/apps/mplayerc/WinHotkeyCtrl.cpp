/*
 * (C) 2011-2017 see Authors.txt
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
#include "WinHotkeyCtrl.h"
#include "vkCodes.h"

#define WM_KEY	(WM_USER + 444)

// CWinHotkeyCtrl

HHOOK CWinHotkeyCtrl::sm_hhookKb = nullptr;
CWinHotkeyCtrl* CWinHotkeyCtrl::sm_pwhcFocus = nullptr;

IMPLEMENT_DYNAMIC(CWinHotkeyCtrl, CEdit)
CWinHotkeyCtrl::CWinHotkeyCtrl():
	m_vkCode(0),
	m_fModSet(0),
	m_fModRel(0),
	m_vkCode_def(0),
	m_fModSet_def(0),
	m_fIsPressed(FALSE)
{
}

CWinHotkeyCtrl::~CWinHotkeyCtrl()
{
}

BEGIN_MESSAGE_MAP(CWinHotkeyCtrl, CEditWithButton)
	ON_MESSAGE(WM_KEY, OnKey)
	ON_WM_CHAR()
	ON_WM_SETCURSOR()
	ON_WM_SETFOCUS()
	ON_WM_KILLFOCUS()
	ON_WM_CONTEXTMENU()
	ON_WM_DESTROY()
	ON_MESSAGE(EDIT_BUTTON_LEFTCLICKED, OnLeftClick)
END_MESSAGE_MAP()

// CWinHotkeyCtrl

void CWinHotkeyCtrl::PreSubclassWindow()
{
	CEditWithButton::PreSubclassWindow();

	UpdateText();
}

LRESULT CALLBACK CWinHotkeyCtrl::LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	LRESULT lResult = 1;

	if (nCode == HC_ACTION && (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN ||
							   wParam == WM_KEYUP || wParam == WM_SYSKEYUP) && sm_pwhcFocus) {
		sm_pwhcFocus->PostMessageW(WM_KEY, ((PKBDLLHOOKSTRUCT)lParam)->vkCode, (wParam & 1));
	}

	return(lResult);
}

BOOL CWinHotkeyCtrl::InstallKbHook()
{
	if (sm_pwhcFocus && sm_hhookKb) {
		sm_pwhcFocus->UninstallKbHook();
	}

	sm_pwhcFocus = this;

	sm_hhookKb = ::SetWindowsHookExW(WH_KEYBOARD_LL, (HOOKPROC)LowLevelKeyboardProc, GetModuleHandle(nullptr), 0);

	return(sm_hhookKb != nullptr);
}

BOOL CWinHotkeyCtrl::UninstallKbHook()
{
	BOOL fOk = FALSE;

	if (sm_hhookKb) {
		fOk = ::UnhookWindowsHookEx(sm_hhookKb);
		sm_hhookKb = nullptr;
	}

	sm_pwhcFocus = nullptr;

	return(fOk);
}

void CWinHotkeyCtrl::UpdateText()
{
	CString sText;
	HotkeyToString(m_vkCode, m_fModSet, sText);
	SetWindowText((LPCTSTR)sText);
	SetSel(0x8fffffff, 0x8fffffff, FALSE);
}

DWORD CWinHotkeyCtrl::GetWinHotkey()
{
	return(MAKEWORD(m_vkCode, m_fModSet));
}

BOOL CWinHotkeyCtrl::GetWinHotkey(UINT* pvkCode, UINT* pfModifiers)
{
	*pvkCode = m_vkCode;
	*pfModifiers = m_fModSet;
	return(m_vkCode != 0);
}

void CWinHotkeyCtrl::SetWinHotkey(DWORD dwHk)
{
	SetWinHotkey(LOBYTE(LOWORD(dwHk)), HIBYTE(LOWORD(dwHk)));
}

void CWinHotkeyCtrl::SetWinHotkey(UINT vkCode, UINT fModifiers)
{
	m_vkCode = m_vkCode_def = vkCode;
	m_fModSet = m_fModSet_def = m_fModRel = fModifiers;
	m_fIsPressed = FALSE;

	UpdateText();
}

LRESULT CWinHotkeyCtrl::OnKey(WPARAM wParam, LPARAM lParam)
{
	DWORD fMod = 0;
	BOOL fRedraw = TRUE;

	switch (wParam) {
		case VK_CONTROL:
		case VK_LCONTROL:
		case VK_RCONTROL:
			fMod = MOD_CONTROL;
			break;
		case VK_MENU:
		case VK_LMENU:
		case VK_RMENU:
			fMod = MOD_ALT;
			break;
		case VK_SHIFT:
		case VK_LSHIFT:
		case VK_RSHIFT:
			fMod = MOD_SHIFT;
			break;
	}

	if (fMod) { // modifier
		if (!lParam) { // press
			if (!m_fIsPressed && m_vkCode) {
				m_fModSet = m_fModRel = 0;
				m_vkCode = 0;
			}
			m_fModRel &= ~fMod;
		} else if (m_fModSet & fMod) { // release
			m_fModRel |= fMod;
		}

		if (m_fIsPressed || !m_vkCode) {
			if (!lParam) { // press
				if (!(m_fModSet & fMod)) { // new modifier
					m_fModSet |= fMod;
				} else {
					fRedraw = FALSE;
				}
			} else {
				m_fModSet &= ~fMod;
			}
		}
	} else { // another key
		if (wParam == VK_DELETE && m_fModSet == (MOD_CONTROL | MOD_ALT)                      // skip "Ctrl+Alt+Del"
				|| wParam == VK_LWIN || wParam == VK_RWIN                                    // skip "Win"
				|| wParam == VK_SNAPSHOT                                                     // skip "PrintScreen"
				//|| wParam == VK_APPS                                                         // skip "Menu key"
				|| (wParam == VK_ESCAPE && (m_fModSet == MOD_CONTROL || m_fModSet == MOD_ALT)) // skip "Ctrl+Esc", "Alt+Esc"
				|| (wParam == VK_TAB && m_fModSet == MOD_ALT)) {                               // skip "Alt+Tab"
			m_fModSet = m_fModRel = 0;
			m_vkCode = 0;
			m_fIsPressed = FALSE;
		} else if (wParam == m_vkCode && lParam) {
			m_fIsPressed = FALSE;
			fRedraw = FALSE;
		} else {
			if (!m_fIsPressed && !lParam) { // pressed a another key
				if (m_fModRel & m_fModSet) {
					m_fModSet = m_fModRel = 0;
				}
				m_vkCode = (UINT)wParam;
				m_fIsPressed = TRUE;
			}
		}
	}

	if (fRedraw) {
		UpdateText();
	}

	return(0L);
}

LRESULT CWinHotkeyCtrl::OnLeftClick(WPARAM wParam, LPARAM lParam)
{
	CRect r;
	CPoint pt;
	CEditWithButton::GetWindowRect(r);
	CRect rectButton = GetButtonRect(r);
	pt = rectButton.BottomRight();
	pt.x = pt.x-(rectButton.Width());
	OnContextMenu(this, pt);

	return 0;
}

// CWinHotkeyCtrl message handlers

void CWinHotkeyCtrl::OnChar(UINT nChar, UINT nRepCnt, UINT nFlags)
{
}

BOOL CWinHotkeyCtrl::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
	return FALSE;
}

void CWinHotkeyCtrl::OnSetFocus(CWnd* pOldWnd)
{
	InstallKbHook();

	CEditWithButton::OnSetFocus(pOldWnd);
}

void CWinHotkeyCtrl::OnKillFocus(CWnd* pNewWnd)
{
	UninstallKbHook();

	CEditWithButton::OnKillFocus(pNewWnd);
}

void CWinHotkeyCtrl::OnContextMenu(CWnd*, CPoint pt)
{
	HMENU hmenu = CreatePopupMenu();
	UINT cod = 0, mod = 0;
	AppendMenu(hmenu, MF_STRING, 1, ResStr(IDS_APPLY));
	AppendMenu(hmenu, MF_STRING, 2, ResStr(IDS_CLEAR));
	AppendMenu(hmenu, MF_STRING, 3, ResStr(IDS_CANCEL));

	UINT uMenuID = TrackPopupMenu(hmenu,
								  TPM_LEFTALIGN | TPM_RIGHTBUTTON | TPM_VERPOSANIMATION | TPM_NONOTIFY | TPM_RETURNCMD,
								  pt.x, pt.y, 0, GetSafeHwnd(), nullptr);

	if (uMenuID) {
		switch (uMenuID) {
			case 1:
				GetWinHotkey(&cod, &mod);
				if (cod == 0 || m_vkCode == 0) {
					mod = m_fModSet = m_fModRel = 0;
				}
				SetWinHotkey(cod, mod);
				m_fIsPressed = FALSE;
				break;
			case 2:
				m_fModSet = m_fModRel = 0;
				m_vkCode = 0;
				m_fIsPressed = FALSE;
				break;
			case 3:
				m_fModSet = m_fModRel = m_fModSet_def;
				m_vkCode = m_vkCode_def;
				m_fIsPressed = FALSE;
				break;
		}
		UpdateText();
		GetParent() ->SetFocus();
	}

	DestroyMenu(hmenu);
}

void CWinHotkeyCtrl::OnDestroy()
{
	if (sm_pwhcFocus == this) {
		sm_pwhcFocus->UninstallKbHook();
	}

	CEditWithButton::OnDestroy();
}
