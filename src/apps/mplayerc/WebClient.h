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

class CWebServer;

class CWebClientSocket : public CAsyncSocket
{
	CWebServer* m_pWebServer;
	CMainFrame* m_pMainFrame;

	CString m_hdr;

	struct cookie_attribs {
		CString path, expire, domain;
	};
	CAtlStringMap<cookie_attribs> m_cookieattribs;

	void Clear();
	void Header();

protected:
	void OnReceive(int nErrorCode);
	void OnClose(int nErrorCode);

public:
	CWebClientSocket(CWebServer* pWebServer, CMainFrame* pMainFrame);
	virtual ~CWebClientSocket();

	bool SetCookie(CString name, CString value = L"", __time64_t expire = -1, CString path = L"/", CString domain = L"");

	CString m_sessid;
	CString m_cmd, m_path, m_query, m_ver;
	CStringA m_data;
	CAtlStringMap<> m_hdrlines;
	CAtlStringMap<> m_get, m_post, m_cookie;
	CAtlStringMap<> m_request;

	bool OnCommand(CStringA& hdr, CStringA& body, CStringA& mime);
	bool OnIndex(CStringA& hdr, CStringA& body, CStringA& mime);
	bool OnInfo(CStringA& hdr, CStringA& body, CStringA& mime);
	bool OnBrowser(CStringA& hdr, CStringA& body, CStringA& mime);
	bool OnControls(CStringA& hdr, CStringA& body, CStringA& mime);
	bool OnVariables(CStringA& hdr, CStringA& body, CStringA& mime);
	bool OnStatus(CStringA& hdr, CStringA& body, CStringA& mime);
	bool OnError404(CStringA& hdr, CStringA& body, CStringA& mime);
	bool OnPlayer(CStringA& hdr, CStringA& body, CStringA& mime);
	bool OnSnapShotJpeg(CStringA& hdr, CStringA& body, CStringA& mime);
};
