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
#include "MainFrm.h"
#include "OpenDirHelper.h"
#include "../../DSUtil/WinAPIUtils.h"

WNDPROC COpenDirHelper::CBProc;
bool COpenDirHelper::m_incl_subdir;
CString COpenDirHelper::strLastOpenDir;

void COpenDirHelper::SetFont(HWND hwnd, LPTSTR FontName, int FontSize)
{
	HFONT hf, hfOld;
	LOGFONT lf = {0};
	HDC hdc = GetDC(hwnd);

	GetObject(GetWindowFont(hwnd), sizeof(lf), &lf);
	lf.lfWeight = FW_REGULAR;
	lf.lfHeight = (LONG)FontSize;
	wcscpy_s(lf.lfFaceName, LF_FACESIZE, FontName);
	hf = CreateFontIndirect(&lf);
	SetBkMode(hdc,OPAQUE);

	hfOld = (HFONT)SendMessage(hwnd, WM_GETFONT, NULL, NULL);  // get old font
	SendMessage(hwnd, WM_SETFONT, (WPARAM)hf, TRUE);           // set new font

	if (!hfOld && (hfOld != hf)) {
		DeleteObject(hfOld);
	}

	ReleaseDC(hwnd, hdc);
}

LRESULT APIENTRY COpenDirHelper::CheckBoxSubclassProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	if (uMsg == WM_LBUTTONUP) {
		if ((SendMessage(hwnd, BM_GETCHECK, 0, 0)) == 1) {
			m_incl_subdir = FALSE;
		} else {
			m_incl_subdir = TRUE;
		}
	}

	return CallWindowProc(CBProc, hwnd, uMsg, wParam, lParam);
}

int CALLBACK COpenDirHelper::BrowseCallbackProcDIR(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData)
{
	HWND checkbox;

	if (uMsg == BFFM_INITIALIZED) {
		SendMessage(hwnd, BFFM_SETSELECTION, TRUE, (LPARAM)(LPCTSTR)strLastOpenDir);

		RECT ListViewRect;
		RECT Dialog;
		RECT ClientArea;
		RECT ButtonRect;

		checkbox = CreateWindowEx(0, L"BUTTON", ResStr(IDS_MAINFRM_DIR_CHECK),
								  WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | BS_AUTOCHECKBOX | BS_MULTILINE,
								  0, 100, 100, 50, hwnd, 0, AfxGetApp()->m_hInstance, NULL);

		HWND ListView = FindWindowEx(hwnd, NULL, L"SysTreeView32", NULL);

		HWND id_ok = GetDlgItem(hwnd, IDOK);
		HWND id_cancel = GetDlgItem(hwnd, IDCANCEL);

		GetWindowRect(hwnd, &Dialog);
		MoveWindow(hwnd, Dialog.left, Dialog.top, Dialog.right - Dialog.left + 50, Dialog.bottom - Dialog.top + 70, TRUE);
		GetWindowRect(hwnd, &Dialog);

		GetClientRect(hwnd, &ClientArea);

		GetWindowRect(ListView, &ListViewRect);
		MoveWindow(ListView, ListViewRect.left - Dialog.left - 3, ListViewRect.top - Dialog.top - 75, ListViewRect.right - ListViewRect.left + 49, ListViewRect.bottom - ListViewRect.top + 115, TRUE);
		GetWindowRect(ListView, &ListViewRect);

		GetWindowRect(id_ok, &ButtonRect);
		MoveWindow(id_ok, ButtonRect.left - Dialog.left + 49, ButtonRect.top - Dialog.top + 40, ButtonRect.right - ButtonRect.left, ButtonRect.bottom - ButtonRect.top, TRUE);

		GetWindowRect(id_cancel, &ButtonRect);
		MoveWindow(id_cancel, ButtonRect.left - Dialog.left + 49, ButtonRect.top - Dialog.top + 40, ButtonRect.right - ButtonRect.left, ButtonRect.bottom - ButtonRect.top, TRUE);

		SetWindowPos(checkbox, HWND_BOTTOM, ListViewRect.left-Dialog.left - 3, ClientArea.bottom - 35, 180, 27, SWP_SHOWWINDOW);
		SetFont(checkbox, L"Tahoma", 13);

		CBProc = (WNDPROC)SetWindowLongPtr(checkbox, GWLP_WNDPROC, (LONG_PTR)CheckBoxSubclassProc);
		SendMessage(checkbox, BM_SETCHECK, (WPARAM)m_incl_subdir, 0);
	}

	return 0;
}

void COpenDirHelper::RecurseAddDir(CString path, CAtlList<CString>* sl)
{
	WIN32_FIND_DATA fd = {0};

	HANDLE hFind = FindFirstFile(path + L"*.*", &fd);

	if (hFind != INVALID_HANDLE_VALUE) {
		do {
			CString f_name = fd.cFileName;

			if ((fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && (f_name!=L".") && (f_name!=L"..")) {
				CString fullpath = path + f_name;

				if (fullpath[fullpath.GetLength() - 1] != '\\') {
					fullpath += '\\';
				}

				sl->AddTail(fullpath);
				RecurseAddDir(fullpath, sl);
			} else {
				continue;
			}
		} while (FindNextFile(hFind, &fd));

		FindClose(hFind);
	}
}
