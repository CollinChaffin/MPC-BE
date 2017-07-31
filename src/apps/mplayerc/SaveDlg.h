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

#include <afxtaskdialog.h>
#include <atomic>
#include "../../DSUtil/HTTPAsync.h"

// CSaveDlg dialog

class CSaveDlg : public CTaskDialog
{
	DECLARE_DYNAMIC(CSaveDlg)

private:
	CString m_in, m_out;
	HICON m_hIcon;

	CComPtr<IGraphBuilder>   pGB;
	CComQIPtr<IMediaControl> pMC;
	CComQIPtr<IMediaSeeking> pMS;

	enum protocol {
		PROTOCOL_NONE,
		PROTOCOL_UDP,
		PROTOCOL_HTTP,
	};
	protocol m_protocol = protocol::PROTOCOL_NONE;

	CHTTPAsync m_HTTPAsync;
	HANDLE  m_hFile     = INVALID_HANDLE_VALUE;
	QWORD   m_len       = 0;
	clock_t m_startTime = 0;

	std::thread        m_SaveThread;
	std::atomic_ullong m_pos    = 0;
	std::atomic_bool   m_bAbort = false;

	void Save();

	SOCKET m_UdpSocket     = INVALID_SOCKET;
	WSAEVENT m_WSAEvent    = NULL;
	sockaddr_in m_addr;

public:
	CSaveDlg(LPCWSTR in, LPCWSTR name, LPCWSTR out, HRESULT& hr);
	virtual ~CSaveDlg();

protected:
	virtual HRESULT OnTimer(_In_ long lTime);
	virtual HRESULT OnCommandControlClick(_In_ int nCommandControlID);

	HRESULT InitFileCopy();
};
