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
#include <atlisapi.h>
#include <zlib/zlib.h>
#include "../../DSUtil/FileHandle.h"
#include "WebServer.h"
#include "WebClient.h"

CWebServerSocket::CWebServerSocket(CWebServer* pWebServer, int port)
	: m_pWebServer(pWebServer)
{
	Create(port);
	Listen();
}

CWebServerSocket::~CWebServerSocket()
{
}

void CWebServerSocket::OnAccept(int nErrorCode)
{
	if (nErrorCode == 0 && m_pWebServer) {
		m_pWebServer->OnAccept(this);
	}

	__super::OnAccept(nErrorCode);
}

CAtlStringMap<CWebServer::RequestHandler> CWebServer::m_internalpages;
CAtlStringMap<UINT> CWebServer::m_downloads;
CAtlStringMap<CStringA, CStringA> CWebServer::m_mimes;

CWebServer::CWebServer(CMainFrame* pMainFrame, int nPort)
	: m_pMainFrame(pMainFrame)
	, m_nPort(nPort)
{
	if (m_internalpages.IsEmpty()) {
		m_internalpages[L"/"] = &CWebClientSocket::OnIndex;
		m_internalpages[L"/index.html"] = &CWebClientSocket::OnIndex;
		m_internalpages[L"/info.html"] = &CWebClientSocket::OnInfo;
		m_internalpages[L"/browser.html"] = &CWebClientSocket::OnBrowser;
		m_internalpages[L"/controls.html"] = &CWebClientSocket::OnControls;
		m_internalpages[L"/command.html"] = &CWebClientSocket::OnCommand;
		m_internalpages[L"/status.html"] = &CWebClientSocket::OnStatus;
		m_internalpages[L"/player.html"] = &CWebClientSocket::OnPlayer;
		m_internalpages[L"/variables.html"] = &CWebClientSocket::OnVariables;
		m_internalpages[L"/snapshot.jpg"] = &CWebClientSocket::OnSnapShotJpeg;
		m_internalpages[L"/404.html"] = &CWebClientSocket::OnError404;
	}

	if (m_downloads.IsEmpty()) {
		m_downloads[L"/default.css"] = IDF_DEFAULT_CSS;
		m_downloads[L"/favicon.png"] = IDF_FAVICON;
		m_downloads[L"/vbg.png"] = IDF_VBR_PNG;
		m_downloads[L"/vbs.png"] = IDF_VBS_PNG;
		m_downloads[L"/sliderbar.gif"] = IDF_SLIDERBAR_GIF;
		m_downloads[L"/slidergrip.gif"] = IDF_SLIDERGRIP_GIF;
		m_downloads[L"/sliderback.png"] = IDF_SLIDERBACK_PNG;
		m_downloads[L"/1pix.gif"] = IDF_1PIX_GIF;
		m_downloads[L"/headericon.png"] = IDF_HEADERICON_PNG;
		m_downloads[L"/headerback.png"] = IDF_HEADERBACK_PNG;
		m_downloads[L"/headerclose.png"] = IDF_HEADERCLOSE_PNG;
		m_downloads[L"/leftside.png"] = IDF_LEFTSIDE_PNG;
		m_downloads[L"/rightside.png"] = IDF_RIGHTSIDE_PNG;
		m_downloads[L"/bottomside.png"] = IDF_BOTTOMSIDE_PNG;
		m_downloads[L"/leftbottomside.png"] = IDF_LEFTBOTTOMSIDE_PNG;
		m_downloads[L"/rightbottomside.png"] = IDF_RIGHTBOTTOMSIDE_PNG;
		m_downloads[L"/seekbarleft.png"] = IDF_SEEKBARLEFT_PNG;
		m_downloads[L"/seekbarmid.png"] = IDF_SEEKBARMID_PNG;
		m_downloads[L"/seekbarright.png"] = IDF_SEEKBARRIGHT_PNG;
		m_downloads[L"/seekbargrip.png"] = IDF_SEEKBARGRIP_PNG;
		m_downloads[L"/logo.png"] = IDF_LOGO1;
		m_downloads[L"/controlback.png"] = IDF_CONTROLBACK_PNG;
		m_downloads[L"/controlbuttonplay.png"] = IDF_CONTROLBUTTONPLAY_PNG;
		m_downloads[L"/controlbuttonpause.png"] = IDF_CONTROLBUTTONPAUSE_PNG;
		m_downloads[L"/controlbuttonstop.png"] = IDF_CONTROLBUTTONSTOP_PNG;
		m_downloads[L"/controlbuttonskipback.png"] = IDF_CONTROLBUTTONSKIPBACK_PNG;
		m_downloads[L"/controlbuttondecrate.png"] = IDF_CONTROLBUTTONDECRATE_PNG;
		m_downloads[L"/controlbuttonincrate.png"] = IDF_CONTROLBUTTONINCRATE_PNG;
		m_downloads[L"/controlbuttonskipforward.png"] = IDF_CONTROLBUTTONSKIPFORWARD_PNG;
		m_downloads[L"/controlbuttonstep.png"] = IDF_CONTROLBUTTONSTEP_PNG;
		m_downloads[L"/controlvolumeon.png"] = IDF_CONTROLVOLUMEON_PNG;
		m_downloads[L"/controlvolumeoff.png"] = IDF_CONTROLVOLUMEOFF_PNG;
		m_downloads[L"/controlvolumebar.png"] = IDF_CONTROLVOLUMEBAR_PNG;
		m_downloads[L"/controlvolumegrip.png"] = IDF_CONTROLVOLUMEGRIP_PNG;
	}

	CRegKey key;
	CString str(L"MIME\\Database\\Content Type");
	if (ERROR_SUCCESS == key.Open(HKEY_CLASSES_ROOT, str, KEY_READ)) {
		WCHAR buff[256];
		DWORD len = _countof(buff);
		for (int i = 0; ERROR_SUCCESS == key.EnumKey(i, buff, &len); i++, len = _countof(buff)) {
			CRegKey mime;
			WCHAR ext[64];
			ULONG extlen = _countof(ext);
			if (ERROR_SUCCESS == mime.Open(HKEY_CLASSES_ROOT, str + L"\\" + buff, KEY_READ)
					&& ERROR_SUCCESS == mime.QueryStringValue(L"Extension", ext, &extlen)) {
				m_mimes[CStringA(ext).MakeLower()] = CStringA(buff).MakeLower();
			}
		}
	}

	m_mimes[".html"] = "text/html";
	m_mimes[".txt"] = "text/plain";
	m_mimes[".css"] = "text/css";
	m_mimes[".gif"] = "image/gif";
	m_mimes[".jpeg"] = "image/jpeg";
	m_mimes[".jpg"] = "image/jpeg";
	m_mimes[".png"] = "image/png";
	m_mimes[".js"] = "text/javascript";

	m_webroot = CPath(GetProgramDir());

	CString WebRoot = AfxGetAppSettings().strWebRoot;
	WebRoot.Replace('/', '\\');
	WebRoot.Trim();
	CPath p(WebRoot);
	if (WebRoot.Find(L":\\") < 0 && WebRoot.Find(L"\\\\") < 0) {
		m_webroot.Append(WebRoot);
	} else {
		m_webroot = p;
	}
	m_webroot.Canonicalize();
	m_webroot.MakePretty();
	if (!m_webroot.IsDirectory()) {
		m_webroot = CPath();
	}

	CAtlList<CString> sl;
	Explode(AfxGetAppSettings().strWebServerCGI, sl, L';');
	POSITION pos = sl.GetHeadPosition();
	while (pos) {
		CAtlList<CString> sl2;
		CString ext = Explode(sl.GetNext(pos), sl2, L'=', 2);
		if (sl2.GetCount() < 2) {
			continue;
		}
		m_cgi[ext] = sl2.GetTail();
	}

	m_ThreadId = 0;
	m_hThread = ::CreateThread(NULL, 0, StaticThreadProc, (LPVOID)this, 0, &m_ThreadId);
}

CWebServer::~CWebServer()
{
	if (m_hThread != NULL) {
		PostThreadMessage(m_ThreadId, WM_QUIT, 0, 0);
		if (WaitForSingleObject(m_hThread, 10000) == WAIT_TIMEOUT) {
			TerminateThread (m_hThread, 0xDEAD);
		}
		EXECUTE_ASSERT(CloseHandle(m_hThread));
	}
}

DWORD WINAPI CWebServer::StaticThreadProc(LPVOID lpParam)
{
	return static_cast<CWebServer*>(lpParam)->ThreadProc();
}

DWORD CWebServer::ThreadProc()
{
	if (!AfxSocketInit(NULL)) {
		return DWORD_ERROR;
	}

	CWebServerSocket s(this, m_nPort);

	MSG msg;
	while ((int)GetMessage(&msg, NULL, 0, 0) > 0) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return 0;
}

static void PutFileContents(LPCWSTR fn, const CStringA& data)
{
	FILE* f = NULL;
	if (!_wfopen_s(&f, fn, L"wb")) {
		fwrite((LPCSTR)data, 1, data.GetLength(), f);
		fclose(f);
	}
}

void CWebServer::Deploy(CString dir)
{
	CStringA data;
	if (LoadResource(IDR_HTML_INDEX, data, RT_HTML)) {
		PutFileContents(dir + L"index.html", data);
	}
	if (LoadResource(IDR_HTML_INFO, data, RT_HTML)) {
		PutFileContents(dir + L"info.html", data);
	}
	if (LoadResource(IDR_HTML_BROWSER, data, RT_HTML)) {
		PutFileContents(dir + L"browser.html", data);
	}
	if (LoadResource(IDR_HTML_CONTROLS, data, RT_HTML)) {
		PutFileContents(dir + L"controls.html", data);
	}
	if (LoadResource(IDR_HTML_VARIABLES, data, RT_HTML)) {
		PutFileContents(dir + L"variables.html", data);
	}
	if (LoadResource(IDR_HTML_404, data, RT_HTML)) {
		PutFileContents(dir + L"404.html", data);
	}
	if (LoadResource(IDR_HTML_PLAYER, data, RT_HTML)) {
		PutFileContents(dir + L"player.html", data);
	}

	POSITION pos = m_downloads.GetStartPosition();
	while (pos) {
		CString fn;
		UINT id;
		m_downloads.GetNextAssoc(pos, fn, id);
		if (LoadResource(id, data, L"FILE")) {
			PutFileContents(dir + fn, data);
		}
	}
}

bool CWebServer::ToLocalPath(CString& path, CString& redir)
{
	if (!path.IsEmpty() && m_webroot.IsDirectory()) {
		CString str = path;
		str.Replace('/', '\\');
		str.TrimLeft('\\');

		CPath p;
		p.Combine(m_webroot, str);
		p.Canonicalize();

		if (p.IsDirectory()) {
			CAtlList<CString> sl;
			Explode(AfxGetAppSettings().strWebDefIndex, sl, L';');
			POSITION pos = sl.GetHeadPosition();
			while (pos) {
				str = sl.GetNext(pos);
				CPath p2 = p;
				p2.Append(str);
				if (p2.FileExists()) {
					p = p2;
					redir = path;
					if (redir.GetAt(redir.GetLength()-1) != L'/') {
						redir += L'/';
					}
					redir += str;
					break;
				}
			}
		}

		if (wcslen(p) > wcslen(m_webroot) && p.FileExists()) {
			path = (LPCTSTR)p;
			return true;
		}
	}

	return false;
}

bool CWebServer::LoadPage(UINT resid, CStringA& str, CString path)
{
	CString redir;
	if (ToLocalPath(path, redir)) {
		FILE* f = NULL;
		if (!_wfopen_s(&f, path, L"rb")) {
			fseek(f, 0, 2);
			char* buff = str.GetBufferSetLength(ftell(f));
			fseek(f, 0, 0);
			int len = (int)fread(buff, 1, str.GetLength(), f);
			fclose(f);
			return len == str.GetLength();
		}
	}

	return LoadResource(resid, str, RT_HTML);
}

void CWebServer::OnAccept(CWebServerSocket* pServer)
{
	CAutoPtr<CWebClientSocket> p(DNew CWebClientSocket(this, m_pMainFrame));
	if (pServer->Accept(*p)) {
		CString name;
		UINT port;
		if (AfxGetAppSettings().fWebServerLocalhostOnly && p->GetPeerName(name, port) && name != L"127.0.0.1") {
			p->Close();
			return;
		}

		m_clients.AddTail(p);
	}
}

void CWebServer::OnClose(CWebClientSocket* pClient)
{
	POSITION pos = m_clients.GetHeadPosition();
	while (pos) {
		POSITION cur = pos;
		if (m_clients.GetNext(pos) == pClient) {
			m_clients.RemoveAt(cur);
			break;
		}
	}
}

void CWebServer::OnRequest(CWebClientSocket* pClient, CStringA& hdr, CStringA& body)
{
	CPath p(pClient->m_path);
	CStringA ext = p.GetExtension().MakeLower();
	CStringA mime;
	if (ext.IsEmpty()) {
		mime = "text/html";
	} else {
		m_mimes.Lookup(ext, mime);
	}

	hdr = "HTTP/1.1 200 OK\r\n";

	bool fHandled = false, fCGI = false;

	if (m_webroot.IsDirectory()) {
		CStringA tmphdr;
		fHandled = fCGI = CallCGI(pClient, tmphdr, body, mime);

		if (fHandled) {
			tmphdr.Replace("\r\n", "\n");
			CAtlList<CStringA> hdrlines;
			ExplodeMin(tmphdr, hdrlines, '\n');
			POSITION pos = hdrlines.GetHeadPosition();
			while (pos) {
				POSITION cur = pos;
				CAtlList<CStringA> sl;
				CStringA key = Explode(hdrlines.GetNext(pos), sl, ':', 2);
				if (sl.GetCount() < 2) {
					continue;
				}
				key.Trim().MakeLower();
				if (key == "content-type") {
					mime = sl.GetTail().Trim();
					hdrlines.RemoveAt(cur);
				} else if (key == "content-length") {
					hdrlines.RemoveAt(cur);
				}
			}
			tmphdr = Implode(hdrlines, "\r\n");
			hdr += tmphdr + "\r\n";
		}
	}

	RequestHandler rh = NULL;
	if (!fHandled && m_internalpages.Lookup(pClient->m_path, rh) && (pClient->*rh)(hdr, body, mime)) {
		if (mime.IsEmpty()) {
			mime = "text/html";
		}

		CString redir;
		if (pClient->m_get.Lookup(L"redir", redir)
				|| pClient->m_post.Lookup(L"redir", redir)) {
			if (redir.IsEmpty()) {
				redir = '/';
			}

			hdr =
				"HTTP/1.1 302 Found\r\n"
				"Location: " + CStringA(redir) + "\r\n";
			return;
		}

		fHandled = true;
	}

	if (!fHandled && m_webroot.IsDirectory()) {
		fHandled = LoadPage(0, body, pClient->m_path);
	}

	UINT resid;
	if (!fHandled && m_downloads.Lookup(pClient->m_path, resid) && LoadResource(resid, body, L"FILE")) {
		if (mime.IsEmpty()) {
			mime = "application/octet-stream";
		}
		fHandled = true;
	}

	if (!fHandled) {
		hdr = mime == "text/html"
			  ? "HTTP/1.1 301 Moved Permanently\r\n" "Location: /404.html\r\n"
			  : "HTTP/1.1 404 Not Found\r\n";
		return;
	}

	if (mime == "text/html" && !fCGI) {
		hdr +=
			"Expires: Thu, 19 Nov 1981 08:52:00 GMT\r\n"
			"Cache-Control: no-store, no-cache, must-revalidate, post-check=0, pre-check=0\r\n"
			"Pragma: no-cache\r\n";

		CStringA debug;
		if (AfxGetAppSettings().fWebServerPrintDebugInfo) {
			debug += "<hr>\r\n";
			CString key, value;
			POSITION pos;
			pos = pClient->m_hdrlines.GetStartPosition();
			while (pos) {
				pClient->m_hdrlines.GetNextAssoc(pos, key, value);
				debug += "HEADER[" + key + "] = " + value + "<br>\r\n";
			}
			debug += "cmd: " + pClient->m_cmd + "<br>\r\n";
			debug += "path: " + pClient->m_path + "<br>\r\n";
			debug += "ver: " + pClient->m_ver + "<br>\r\n";
			pos = pClient->m_get.GetStartPosition();
			while (pos) {
				pClient->m_get.GetNextAssoc(pos, key, value);
				debug += "GET[" + key + "] = " + value + "<br>\r\n";
			}
			pos = pClient->m_post.GetStartPosition();
			while (pos) {
				pClient->m_post.GetNextAssoc(pos, key, value);
				debug += "POST[" + key + "] = " + value + "<br>\r\n";
			}
			pos = pClient->m_cookie.GetStartPosition();
			while (pos) {
				pClient->m_cookie.GetNextAssoc(pos, key, value);
				debug += "COOKIE[" + key + "] = " + value + "<br>\r\n";
			}
			pos = pClient->m_request.GetStartPosition();
			while (pos) {
				pClient->m_request.GetNextAssoc(pos, key, value);
				debug += "REQUEST[" + key + "] = " + value + "<br>\r\n";
			}
		}
		body.Replace("[debug]", debug);

		body.Replace("[browserpath]", "/browser.html");
		body.Replace("[commandpath]", "/command.html");
		body.Replace("[controlspath]", "/controls.html");
		body.Replace("[indexpath]", "/index.html");
		body.Replace("[path]", CStringA(pClient->m_path));
		body.Replace("[setposcommand]", CMD_SETPOS);
		body.Replace("[setvolumecommand]", CMD_SETVOLUME);
		body.Replace("[wmcname]", "wm_command");
	}

	// gzip
	if (AfxGetAppSettings().fWebServerUseCompression && !body.IsEmpty()
		&& hdr.Find("Content-Encoding:") < 0 && ext != ".png" && ext != ".jpg" && ext != ".gif")
		do {
			CString accept_encoding;
			pClient->m_hdrlines.Lookup(L"accept-encoding", accept_encoding);
			accept_encoding.MakeLower();
			CAtlList<CString> sl;
			ExplodeMin(accept_encoding, sl, ',');
			if (!sl.Find(L"gzip")) {
				break;
			}

			// Allocate deflate state
			z_stream strm;

			strm.zalloc = Z_NULL;
			strm.zfree = Z_NULL;
			strm.opaque = Z_NULL;
			int ret = deflateInit2(&strm, Z_DEFAULT_COMPRESSION, Z_DEFLATED, 15 + 16, 8, Z_DEFAULT_STRATEGY);
			if (ret != Z_OK) {
				ASSERT(0);
				break;
			}

			int gzippedBuffLen = body.GetLength();
			BYTE* gzippedBuff = DNew BYTE[gzippedBuffLen];

			// Compress
			strm.avail_in = body.GetLength();
			strm.next_in = (Bytef*)(LPCSTR)body;

			strm.avail_out = gzippedBuffLen;
			strm.next_out = gzippedBuff;

			ret = deflate(&strm, Z_FINISH);
			if (ret != Z_STREAM_END || strm.avail_in != 0) {
				ASSERT(0);
				deflateEnd(&strm);
				delete [] gzippedBuff;
				break;
			}
			gzippedBuffLen -= strm.avail_out;
			memcpy(body.GetBufferSetLength(gzippedBuffLen), gzippedBuff, gzippedBuffLen);

			// Clean up
			deflateEnd(&strm);
			delete [] gzippedBuff;

			hdr += "Content-Encoding: gzip\r\n";
		} while (0);

	CStringA content;
	content.Format(
		"Content-Type: %s\r\n"
		"Content-Length: %d\r\n",
		mime, body.GetLength());
	hdr += content;
}

static DWORD WINAPI KillCGI(LPVOID lParam)
{
	HANDLE hProcess = (HANDLE)lParam;
	if (WaitForSingleObject(hProcess, 30000) == WAIT_TIMEOUT) {
		TerminateProcess(hProcess, 0);
	}
	return 0;
}

bool CWebServer::CallCGI(CWebClientSocket* pClient, CStringA& hdr, CStringA& body, CStringA& mime)
{
	CString path = pClient->m_path, redir = path;
	if (!ToLocalPath(path, redir)) {
		return false;
	}
	CString ext = CPath(path).GetExtension().MakeLower();
	CPath dir(path);
	dir.RemoveFileSpec();

	CString cgi;
	if (!m_cgi.Lookup(ext, cgi) || !CPath(cgi).FileExists()) {
		return false;
	}

	HANDLE hProcess = GetCurrentProcess();
	HANDLE hChildStdinRd, hChildStdinWr, hChildStdinWrDup = NULL;
	HANDLE hChildStdoutRd, hChildStdoutWr, hChildStdoutRdDup = NULL;

	SECURITY_ATTRIBUTES saAttr;
	ZeroMemory(&saAttr, sizeof(saAttr));
	saAttr.nLength = sizeof(saAttr);
	saAttr.bInheritHandle = TRUE;

	if (CreatePipe(&hChildStdoutRd, &hChildStdoutWr, &saAttr, 0)) {
		BOOL fSuccess = DuplicateHandle(hProcess, hChildStdoutRd, hProcess, &hChildStdoutRdDup, 0, FALSE, DUPLICATE_SAME_ACCESS);
		UNREFERENCED_PARAMETER(fSuccess);
		CloseHandle(hChildStdoutRd);
	}

	if (CreatePipe(&hChildStdinRd, &hChildStdinWr, &saAttr, 0)) {
		BOOL fSuccess = DuplicateHandle(hProcess, hChildStdinWr, hProcess, &hChildStdinWrDup, 0, FALSE, DUPLICATE_SAME_ACCESS);
		UNREFERENCED_PARAMETER(fSuccess);
		CloseHandle(hChildStdinWr);
	}

	STARTUPINFO siStartInfo;
	ZeroMemory(&siStartInfo, sizeof(siStartInfo));
	siStartInfo.cb = sizeof(siStartInfo);
	siStartInfo.hStdError = hChildStdoutWr;
	siStartInfo.hStdOutput = hChildStdoutWr;
	siStartInfo.hStdInput = hChildStdinRd;
	siStartInfo.dwFlags |= STARTF_USESTDHANDLES|STARTF_USESHOWWINDOW;
	siStartInfo.wShowWindow = SW_HIDE;

	PROCESS_INFORMATION piProcInfo;
	ZeroMemory(&piProcInfo, sizeof(piProcInfo));

	CStringA envstr;

	LPVOID lpvEnv = GetEnvironmentStrings();
	if (lpvEnv) {
		CString str;

		CAtlList<CString> env;
		for (LPTSTR lpszVariable = (LPTSTR)lpvEnv; *lpszVariable; lpszVariable += wcslen(lpszVariable)+1)
			if (lpszVariable != (LPTSTR)lpvEnv) {
				env.AddTail(lpszVariable);
			}

		env.AddTail(L"GATEWAY_INTERFACE=CGI/1.1");
		env.AddTail(L"SERVER_SOFTWARE=MPC-BE/6.4.x.y");
		env.AddTail(L"SERVER_PROTOCOL=" + pClient->m_ver);
		env.AddTail(L"REQUEST_METHOD=" + pClient->m_cmd);
		env.AddTail(L"PATH_INFO=" + redir);
		env.AddTail(L"PATH_TRANSLATED=" + path);
		env.AddTail(L"SCRIPT_NAME=" + redir);
		env.AddTail(L"QUERY_STRING=" + pClient->m_query);

		if (pClient->m_hdrlines.Lookup(L"content-type", str)) {
			env.AddTail(L"CONTENT_TYPE=" + str);
		}
		if (pClient->m_hdrlines.Lookup(L"content-length", str)) {
			env.AddTail(L"CONTENT_LENGTH=" + str);
		}

		POSITION pos = pClient->m_hdrlines.GetStartPosition();
		while (pos) {
			CString key = pClient->m_hdrlines.GetKeyAt(pos);
			CString value = pClient->m_hdrlines.GetNextValue(pos);
			key.Replace(L"-", L"_");
			key.MakeUpper();
			env.AddTail(L"HTTP_" + key + L"=" + value);
		}

		CString name;
		UINT port;

		if (pClient->GetPeerName(name, port)) {
			str.Format(L"%u", port);
			env.AddTail(L"REMOTE_ADDR=" + name);
			env.AddTail(L"REMOTE_HOST=" + name);
			env.AddTail(L"REMOTE_PORT=" + str);
		}

		if (pClient->GetSockName(name, port)) {
			str.Format(L"%u", port);
			env.AddTail(L"SERVER_NAME=" + name);
			env.AddTail(L"SERVER_PORT=" + str);
		}

		env.AddTail(L"\0");

		str = Implode(env, '\0');
		envstr = CStringA(str, str.GetLength());

		FreeEnvironmentStrings((LPTSTR)lpvEnv);
	}

	WCHAR* cmdln = DNew WCHAR[32768];
	_snwprintf_s(cmdln, 32768, 32768, L"\"%s\" \"%s\"", cgi, path);

	if (hChildStdinRd && hChildStdoutWr)
		if (CreateProcess(
					NULL, cmdln, NULL, NULL, TRUE, 0,
					envstr.GetLength() ? (LPVOID)(LPCSTR)envstr : NULL,
					dir, &siStartInfo, &piProcInfo)) {
			DWORD ThreadId;
			VERIFY(CreateThread(NULL, 0, KillCGI, (LPVOID)piProcInfo.hProcess, 0, &ThreadId));

			static const int BUFFSIZE = 1024;
			DWORD dwRead, dwWritten = 0;

			int i = 0, len = pClient->m_data.GetLength();
			for (; i < len; i += dwWritten)
				if (!WriteFile(hChildStdinWrDup, (LPCSTR)pClient->m_data + i, min(len - i, BUFFSIZE), &dwWritten, NULL)) {
					break;
				}

			CloseHandle(hChildStdinWrDup);
			CloseHandle(hChildStdoutWr);

			body.Empty();

			CStringA buff;
			while (i == len && ReadFile(hChildStdoutRdDup, buff.GetBuffer(BUFFSIZE), BUFFSIZE, &dwRead, NULL) && dwRead) {
				buff.ReleaseBufferSetLength(dwRead);
				body += buff;
			}

			int hdrend = body.Find("\r\n\r\n");
			if (hdrend >= 0) {
				hdr = body.Left(hdrend + 2);
				body = body.Mid(hdrend + 4);
			}

			CloseHandle(hChildStdinRd);
			CloseHandle(hChildStdoutRdDup);

			CloseHandle(piProcInfo.hProcess);
			CloseHandle(piProcInfo.hThread);
		} else {
			body = L"CGI Error";
		}

	delete [] cmdln;

	return true;
}
