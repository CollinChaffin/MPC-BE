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
#include <afxwin.h>
#include "SubtitleDlDlg.h"
#include "MainFrm.h"
#include "../../filters/transform/VSFilter/IDirectVobSub.h"
#include "../../DSUtil/DSUtil.h"

#define UWM_PARSE	(WM_USER + 100)
#define UWM_FAILED	(WM_USER + 101)

size_t StrMatchA(LPCSTR a, LPCSTR b)
{
	size_t count = 0;

	for (; *a && *b; a++, b++, count++) {
		if (tolower(*a) != tolower(*b)) {
			break;
		}
	}

	return count;
}

size_t StrMatchW(LPCWSTR a, LPCWSTR b)
{
	size_t count = 0;

	for (; *a && *b; a++, b++, count++) {
		if (towlower(*a) != towlower(*b)) {
			break;
		}
	}

	return count;
}

CSubtitleDlDlg::CSubtitleDlDlg(CWnd* pParent, const CStringA& url, const CString& filename)
	: CResizableDialog(CSubtitleDlDlg::IDD, pParent)
	, m_ps(&m_list, 0, TRUE)
	, m_defps(&m_list, filename)
	, m_pTA(nullptr)
	, m_url(url)
	, m_bReplaceSubs(false)
	, m_status()
{
}

CSubtitleDlDlg::~CSubtitleDlDlg()
{
	delete m_pTA;
}

void CSubtitleDlDlg::DoDataExchange(CDataExchange* pDX)
{
	__super::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST1, m_list);
}

CString CSubtitleDlDlg::LangCodeToName(LPCSTR code)
{
	// accept only three-letter language codes
	size_t codeLen = strlen(code);
	if (codeLen != 3) {
		return L"";
	}

	CString name = ISO6392ToLanguage(code);
	if (!name.IsEmpty()) {
		// workaround for ISO6392ToLanguage function behavior
		// for unknown language code it returns the code parameter back
		if (code != name) {
			return name;
		}
	}

	// support abbreviations loosely based on first letters of language name

	// this list is limited to upload-enabled languages
	// retrieved with:
	// wget -q -O- http://www.opensubtitles.org/addons/export_languages.php | \
	// awk 'NR > 1 { if ($(NF-1) == "1") print ("\"" $(NF-2)  "\",")}'
	static LPCSTR ltable[] = {
		"Albanian",  "Arabic",    "Armenian",  "Basque",     "Bengali",       "Bosnian",    "Breton",    "Bulgarian",
		"Burmese",   "Catalan",   "Chinese",   "Czech",      "Danish",        "Dutch",      "English",   "Esperanto",
		"Estonian",  "Finnish",   "French",    "Georgian",   "German",        "Galician",   "Greek",     "Hebrew",
		"Hindi",     "Croatian",  "Hungarian", "Icelandic",  "Indonesian",    "Italian",    "Japanese",  "Kazakh",
		"Khmer",     "Korean",    "Latvian",   "Lithuanian", "Luxembourgish", "Macedonian", "Malayalam", "Malay",
		"Mongolian", "Norwegian", "Occitan",   "Persian",    "Polish",        "Portuguese", "Russian",   "Serbian",
		"Sinhalese", "Slovak",    "Slovenian", "Spanish",    "Swahili",       "Swedish",    "Syriac",    "Telugu",
		"Tagalog",   "Thai",      "Turkish",   "Ukrainian",  "Urdu",          "Vietnamese", "Romanian",  "Brazilian",
	};

	for (size_t i = 0; i < _countof(ltable); ++i) {
		if (StrMatchA(ltable[i], code) == codeLen) {
			return CString(ltable[i]);
		}
	}
	return L"";
}

int CALLBACK CSubtitleDlDlg::DefSortCompare(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
	PDEFPARAMSORT defps = reinterpret_cast<PDEFPARAMSORT>(lParamSort);
	int nLeft = (int)lParam1, nRight = (int)lParam2;

	// sort by language first
	isdb_subtitle* lSub = (isdb_subtitle*)defps->m_list->GetItemData(nLeft);
	isdb_subtitle* rSub = (isdb_subtitle*)defps->m_list->GetItemData(nRight);
	CString left  = defps->m_list->GetItemText(nLeft, COL_LANGUAGE);
	CString right = defps->m_list->GetItemText(nRight, COL_LANGUAGE);
	// user-provided sort order
	int lpos, rpos;
	if (!defps->m_langPos.Lookup(CString(lSub->iso639_2), lpos) && !defps->m_langPos.Lookup(left, lpos)) {
		lpos = INT_MAX;
	}
	if (!defps->m_langPos.Lookup(CString(rSub->iso639_2), rpos) && !defps->m_langPos.Lookup(right, rpos)) {
		rpos = INT_MAX;
	}
	if (lpos < rpos) {
		return -1;
	} else if (lpos > rpos) {
		return 1;
	} else if (lpos == INT_MAX && rpos == INT_MAX) {
		// lexicographical order
		int res = wcscmp(left, right);
		if (res != 0) {
			return res;
		}
	}

	// sort by filename
	left  = defps->m_list->GetItemText(nLeft, COL_FILENAME);
	right = defps->m_list->GetItemText(nRight, COL_FILENAME);
	size_t lmatch = StrMatchW(defps->m_filename, left);
	size_t rmatch = StrMatchW(defps->m_filename, right);

	// sort by matching character number
	if (lmatch > rmatch) {
		return -1;
	} else if (lmatch < rmatch) {
		return 1;
	}

	// prefer shorter names
	int llen = left.GetLength();
	int rlen = right.GetLength();
	if (llen < rlen) {
		return -1;
	} else if (llen > rlen) {
		return 1;
	}
	return 0;
}

void CSubtitleDlDlg::LoadList()
{
	m_list.SetRedraw(FALSE);

	for (int i = 0; i < m_parsed_movies.GetCount(); ++i) {
		isdb_movie_parsed& m = m_parsed_movies[i];

		int iItem = m_list.InsertItem(i, L"");
		m_list.SetItemData(iItem, m.ptr);
		m_list.SetItemText(iItem, COL_FILENAME, m.name);
		m_list.SetItemText(iItem, COL_LANGUAGE, m.language);
		m_list.SetItemText(iItem, COL_FORMAT, m.format);
		m_list.SetItemText(iItem, COL_DISC, m.disc);
		m_list.SetItemText(iItem, COL_TITLES, m.titles);
		m_list.SetCheck(iItem, m.checked);
	}

	// sort by language and filename
	m_list.SortItemsEx(DefSortCompare, (DWORD_PTR)&m_defps);

	m_list.SetRedraw(TRUE);
	m_list.Invalidate();
	m_list.UpdateWindow();
}

bool CSubtitleDlDlg::Parse()
{
	isdb_movie m;
	isdb_subtitle sub;

	CAtlList<CStringA> sl;
	Explode(m_pTA->raw_list, sl, '\n');
	CString str;

	POSITION pos = sl.GetHeadPosition();

	while (pos) {
		str = sl.GetNext(pos);

		CStringA param = str.Left(max(0, str.Find('=')));
		CStringA value = str.Mid(str.Find('=')+1);

		if (param == "ticket") {
			m_pTA->ticket = value;
		} else if (param == "movie") {
			m.reset();
			Explode(value.Trim(" |"), m.titles, '|');
		} else if (param == "subtitle") {
			sub.reset();
			sub.id = atoi(value);
		} else if (param == "name") {
			sub.name = value;
		} else if (param == "discs") {
			sub.discs = atoi(value);
		} else if (param == "disc_no") {
			sub.disc_no = atoi(value);
		} else if (param == "format") {
			sub.format = value;
		} else if (param == "iso639_2") {
			sub.iso639_2 = value;
		} else if (param == "language") {
			sub.language = value;
		} else if (param == "nick") {
			sub.nick = value;
		} else if (param == "email") {
			sub.email = value;
		} else if (param.IsEmpty() && value == "endsubtitle") {
			m.subs.AddTail(sub);
		} else if (param.IsEmpty() && value == "endmovie") {
			m_pTA->raw_movies.AddTail(m);
		} else if (param.IsEmpty() && value == "end") {
			break;
		}
	}

	pos = m_pTA->raw_movies.GetHeadPosition();
	while (pos) {
		isdb_movie& raw_movie = m_pTA->raw_movies.GetNext(pos);
		isdb_movie_parsed p;

		CStringA titlesA = Implode(raw_movie.titles, '|');
		titlesA.Replace("|", ", ");
		p.titles = UTF8To16(titlesA);
		p.checked = false;

		POSITION pos2 = raw_movie.subs.GetHeadPosition();
		while (pos2) {
			const isdb_subtitle& s = raw_movie.subs.GetNext(pos2);
			p.name = UTF8To16(s.name);
			p.language = s.language;
			p.format = s.format;
			p.disc.Format(L"%d/%d", s.disc_no, s.discs);
			p.ptr = reinterpret_cast<DWORD_PTR>(&s);

			m_parsed_movies.Add(p);
		}
	}

	bool ret = true;
	if (m_parsed_movies.IsEmpty()) {
		ret = false;
	}

	return ret;
}

void CSubtitleDlDlg::SetStatus(const CString& status)
{
	m_status.SetText(status, 0, 0);
}

UINT CSubtitleDlDlg::RunThread(LPVOID pParam)
{
	PTHREADSTRUCT pTA = reinterpret_cast<PTHREADSTRUCT>(pParam);

	if (!OpenUrl(pTA->is, CString(pTA->url), pTA->raw_list)) {
		::PostMessage(pTA->hWND, UWM_FAILED, (WPARAM)0, (LPARAM)0);
		AfxEndThread(1, TRUE);
	}

	::PostMessage(pTA->hWND, UWM_PARSE, (WPARAM)0, (LPARAM)0);

	return 0;
};

int CALLBACK CSubtitleDlDlg::SortCompare(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
	PPARAMSORT ps = reinterpret_cast<PPARAMSORT>(lParamSort);

	CString left  = ps->m_list->GetItemText((int)lParam1, ps->m_colIndex);
	CString right = ps->m_list->GetItemText((int)lParam2, ps->m_colIndex);

	return ps->m_ascending ? left.Compare(right) : right.Compare(left);
}

BOOL CSubtitleDlDlg::OnInitDialog()
{
	__super::OnInitDialog();

	m_status.Create(WS_CHILD | WS_VISIBLE | CCS_BOTTOM, CRect(0,0,0,0), this, IDC_STATUSBAR);

	int n, curPos = 0;
	CArray<int> columnWidth;

	CString strColumnWidth = AfxGetMyApp()->GetProfileString(IDS_R_DLG_SUBTITLEDL, IDS_RS_DLG_SUBTITLEDL_COLWIDTH);
	CString token = strColumnWidth.Tokenize(L",", curPos);

	while (!token.IsEmpty()) {
		if (swscanf_s(token, L"%d", &n) == 1) {
			columnWidth.Add(n);
			token = strColumnWidth.Tokenize(L",", curPos);
		} else {
			throw 1;
		}
	}

	m_list.SetExtendedStyle(m_list.GetExtendedStyle()
							| LVS_EX_DOUBLEBUFFER | LVS_EX_FULLROWSELECT
							| LVS_EX_CHECKBOXES   | LVS_EX_LABELTIP);

	if (columnWidth.GetCount() != 5) {
		columnWidth.RemoveAll();
		columnWidth.Add(290);
		columnWidth.Add(70);
		columnWidth.Add(50);
		columnWidth.Add(50);
		columnWidth.Add(270);
	}

	m_list.InsertColumn(COL_FILENAME, ResStr(IDS_SUBDL_DLG_FILENAME_COL), LVCFMT_LEFT, columnWidth[0]);
	m_list.InsertColumn(COL_LANGUAGE, ResStr(IDS_SUBDL_DLG_LANGUAGE_COL), LVCFMT_CENTER, columnWidth[1]);
	m_list.InsertColumn(COL_FORMAT, ResStr(IDS_SUBDL_DLG_FORMAT_COL), LVCFMT_CENTER, columnWidth[2]);
	m_list.InsertColumn(COL_DISC, ResStr(IDS_SUBDL_DLG_DISC_COL), LVCFMT_CENTER, columnWidth[3]);
	m_list.InsertColumn(COL_TITLES, ResStr(IDS_SUBDL_DLG_TITLES_COL), LVCFMT_LEFT, columnWidth[4]);

	AddAnchor(IDC_LIST1, TOP_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDC_CHECK1, BOTTOM_LEFT);
	AddAnchor(IDOK, BOTTOM_RIGHT);
	AddAnchor(IDC_STATUSBAR, BOTTOM_LEFT, BOTTOM_RIGHT);

	const CSize s(420, 200);
	SetMinTrackSize(s);
	EnableSaveRestore(IDS_R_DLG_SUBTITLEDL);

	// set language sorting order
	const CAppSettings& settings = AfxGetAppSettings();
	CString order = settings.strSubtitlesLanguageOrder;
	// fill language->position map
	int listPos = 0;
	int tPos = 0;
	CString langCode = order.Tokenize(L",; ", tPos);
	while (tPos != -1) {
		int pos;
		CString langCodeISO6391 = ISO6392To6391(CStringA(langCode));
		if (!langCodeISO6391.IsEmpty() && !m_defps.m_langPos.Lookup(langCodeISO6391, pos)) {
			m_defps.m_langPos[langCodeISO6391] = listPos;
		}
		CString langName = LangCodeToName(CStringA(langCode));
		if (!langName.IsEmpty() && !m_defps.m_langPos.Lookup(langName, pos)) {
			m_defps.m_langPos[langName] = listPos;
		}
		langCode = order.Tokenize(L",; ", tPos);
		listPos++;
	}

	m_pTA = DNew THREADSTRUCT;
	m_pTA->url = m_url;
	m_pTA->hWND = GetSafeHwnd();

	SetStatus(ResStr(IDS_SUBDL_DLG_DOWNLOADING));
	AfxBeginThread(RunThread, static_cast<LPVOID>(m_pTA));

	return TRUE;
}

BOOL CSubtitleDlDlg::PreTranslateMessage(MSG* pMsg)
{
	// Inhibit default handling for the Enter key when the list has the focus and an item is selected.
	if (pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_RETURN
			&& pMsg->hwnd == m_list.GetSafeHwnd() && m_list.GetSelectedCount() > 0) {
		return FALSE;
	}

	return __super::PreTranslateMessage(pMsg);
}

void CSubtitleDlDlg::OnOK()
{
	SetStatus(ResStr(IDS_SUBDL_DLG_DOWNLOADING));

	for (int i = 0; i < m_list.GetItemCount(); ++i) {
		if (m_list.GetCheck(i)) {
			m_selsubs.AddTail(*reinterpret_cast<isdb_subtitle*>(m_list.GetItemData(i)));
		}
	}

	m_bReplaceSubs = IsDlgButtonChecked(IDC_CHECK1) == BST_CHECKED;

	auto pFrame = AfxGetMainFrame();

	if (m_bReplaceSubs) {
		pFrame->m_pSubStreams.RemoveAll();
	}

	CComPtr<ISubStream> pSubStreamToSet;

	POSITION pos = m_selsubs.GetHeadPosition();

	while (pos) {
		const isdb_subtitle& sub = m_selsubs.GetNext(pos);
		CAppSettings& s = AfxGetAppSettings();
		CInternetSession is;
		CStringA url = "http://" + s.strISDb + "/dl.php?";
		CStringA ticket = UrlEncode(m_pTA->ticket);
		CStringA args, str;
		args.Format("id=%d&ticket=%s", sub.id, ticket);
		url.Append(args);

		if (OpenUrl(is, CString(url), str)) {

			if (pFrame->m_pDVS) {
				WCHAR lpszTempPath[_MAX_PATH] = { 0 };
				if (::GetTempPath(_MAX_PATH, lpszTempPath)) {
					CString subFileName(lpszTempPath);
					subFileName.Append(CString(sub.name));
					if (::PathFileExists(subFileName)) {
						::DeleteFile(subFileName);
					}

					CFile cf;
					if (cf.Open(subFileName, CFile::modeCreate|CFile::modeWrite|CFile::shareDenyNone)) {
						cf.Write(str.GetString(), str.GetLength());
						cf.Close();

						if (SUCCEEDED(pFrame->m_pDVS->put_FileName((LPWSTR)(LPCWSTR)subFileName))) {
							pFrame->m_pDVS->put_SelectedLanguage(0);
							pFrame->m_pDVS->put_HideSubtitles(true);
							pFrame->m_pDVS->put_HideSubtitles(false);
						}

						::DeleteFile(subFileName);
					}
				}

				__super::OnOK();
				return;
			}

			CAutoPtr<CRenderedTextSubtitle> pRTS(DNew CRenderedTextSubtitle(&pFrame->m_csSubLock));
			if (pRTS && pRTS->Open((BYTE*)(LPCSTR)str, str.GetLength(), DEFAULT_CHARSET, CString(sub.name)) && pRTS->GetStreamCount() > 0) {
				CComPtr<ISubStream> pSubStream = pRTS.Detach();
				pFrame->m_pSubStreams.AddTail(pSubStream);
				if (!pSubStreamToSet) {
					pSubStreamToSet = pSubStream;
				}
			}
		}
	}

	if (pSubStreamToSet) {
		pFrame->SetSubtitle(pSubStreamToSet);
		AfxGetAppSettings().fEnableSubtitles = true;
	}

	__super::OnOK();
}

void CSubtitleDlDlg::OnUpdateOk(CCmdUI* pCmdUI)
{
	bool fEnable = false;
	for (int i = 0; !fEnable && i < m_list.GetItemCount(); ++i) {
		fEnable = !!m_list.GetCheck(i);
	}

	pCmdUI->Enable(fEnable);
}

void CSubtitleDlDlg::OnFailedConnection()
{
	SetStatus(ResStr(IDS_SUBDL_DLG_CONNECT_ERROR));
}

void CSubtitleDlDlg::OnParse()
{
	SetStatus(ResStr(IDS_SUBDL_DLG_PARSING));

	if (Parse()) {
		LoadList();
		CString msg;
		msg.Format(ResStr(IDS_SUBDL_DLG_SUBS_AVAIL), m_list.GetItemCount());
		SetStatus(msg);
	} else {
		SetStatus(ResStr(IDS_SUBDL_DLG_NOT_FOUND));
	}
}

void CSubtitleDlDlg::OnColumnClick(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMHEADER phdr = reinterpret_cast<LPNMHEADER>(pNMHDR);
	*pResult = 0;

	if (phdr->iItem == m_ps.m_colIndex) {
		m_ps.m_ascending = !m_ps.m_ascending;
	} else {
		m_ps.m_ascending = true;
	}

	m_ps.m_colIndex = phdr->iItem;

	SetRedraw(FALSE);
	m_list.SortItemsEx(SortCompare, (DWORD_PTR)&m_ps);
	SetRedraw(TRUE);
	m_list.Invalidate();
	m_list.UpdateWindow();
}

void CSubtitleDlDlg::OnSize(UINT nType, int cx, int cy)
{
	__super::OnSize(nType, cx, cy);

	ArrangeLayout();
}

void CSubtitleDlDlg::OnDestroy()
{
	RemoveAllAnchors();

	const CHeaderCtrl& pHC = *m_list.GetHeaderCtrl();
	CString strColumnWidth;

	for (int i = 0; i < pHC.GetItemCount(); ++i) {
		int w = m_list.GetColumnWidth(i);
		strColumnWidth.AppendFormat(L"%d,", w);
	}
	AfxGetMyApp()->WriteProfileString(IDS_R_DLG_SUBTITLEDL, IDS_RS_DLG_SUBTITLEDL_COLWIDTH, strColumnWidth);

	__super::OnDestroy();
}

BOOL CSubtitleDlDlg::OnEraseBkgnd(CDC* pDC)
{
	EraseBackground(pDC);

	return TRUE;
}

void CSubtitleDlDlg::DownloadSelectedSubtitles()
{
	POSITION pos = m_list.GetFirstSelectedItemPosition();
	while (pos) {
		int nItem = m_list.GetNextSelectedItem(pos);
		if (nItem >= 0 && nItem < m_list.GetItemCount()) {
			ListView_SetCheckState(m_list.GetSafeHwnd(), nItem, TRUE);
		}
	}
	OnOK();
}

BEGIN_MESSAGE_MAP(CSubtitleDlDlg, CResizableDialog)
	ON_WM_ERASEBKGND()
	ON_WM_SIZE()
	ON_MESSAGE_VOID(UWM_PARSE, OnParse)
	ON_MESSAGE_VOID(UWM_FAILED, OnFailedConnection)
	ON_UPDATE_COMMAND_UI(IDOK, OnUpdateOk)
	ON_NOTIFY(HDN_ITEMCLICK, 0, OnColumnClick)
	ON_WM_DESTROY()
	ON_NOTIFY(NM_DBLCLK, IDC_LIST1, OnDoubleClickSubtitle)
	ON_NOTIFY(LVN_KEYDOWN, IDC_LIST1, OnKeyPressedSubtitle)
END_MESSAGE_MAP()

void CSubtitleDlDlg::OnDoubleClickSubtitle(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMITEMACTIVATE pItemActivate = (LPNMITEMACTIVATE)(pNMHDR);

	if (pItemActivate->iItem >= 0) {
		DownloadSelectedSubtitles();
	}
}

void CSubtitleDlDlg::OnKeyPressedSubtitle(NMHDR* pNMHDR, LRESULT* pResult)
{
	LV_KEYDOWN* pLVKeyDow = (LV_KEYDOWN*)pNMHDR;

	if (pLVKeyDow->wVKey == VK_RETURN) {
		DownloadSelectedSubtitles();
		*pResult = TRUE;
	}
}

bool CSubtitleDlDlg::OpenUrl(CInternetSession& is, CString url, CStringA& str)
{
	str.Empty();

	try {
		CAutoPtr<CStdioFile> f(is.OpenURL(url, 1, INTERNET_FLAG_TRANSFER_BINARY | INTERNET_FLAG_EXISTING_CONNECT));

		char buff[1024];
		for (int len; (len = f->Read(buff, sizeof(buff))) > 0; str += CStringA(buff, len));

		f->Close(); // must close it because the destructor doesn't seem to do it and we will get an exception when "is" is destroying
	} catch (CInternetException* ie) {
		ie->Delete();
		return false;
	}

	return true;
}
