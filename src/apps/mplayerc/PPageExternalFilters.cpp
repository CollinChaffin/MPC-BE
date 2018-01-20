/*
 * (C) 2003-2006 Gabest
 * (C) 2006-2018 see Authors.txt
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
#include <atlpath.h>
#include <InitGuid.h>
#include <dmoreg.h>
#include "PPageExternalFilters.h"
#include "ComPropertyPage.h"
#include "RegFilterChooserDlg.h"
#include "SelectMediaType.h"
#include "FGFilter.h"
#include <moreuuids.h>

// returns an iterator on the found element, or last if nothing is found
template <class T>
auto FindInListByPointer(std::list<T>& list, const T* p)
{
	auto it = list.begin();
	for (; it != list.end(); ++it) {
		if (&*it == p) {
			break;
		}
	}

	return it;
}

// CPPageExternalFilters dialog

IMPLEMENT_DYNAMIC(CPPageExternalFilters, CPPageBase)
CPPageExternalFilters::CPPageExternalFilters()
	: CPPageBase(CPPageExternalFilters::IDD, CPPageExternalFilters::IDD)
	, m_iLoadType(FilterOverride::PREFERRED)
	, m_pLastSelFilter(nullptr)
{
}

CPPageExternalFilters::~CPPageExternalFilters()
{
}

void CPPageExternalFilters::DoDataExchange(CDataExchange* pDX)
{
	__super::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST1, m_filters);
	DDX_Radio(pDX, IDC_RADIO1, m_iLoadType);
	DDX_Control(pDX, IDC_EDIT1, m_dwMerit);
	DDX_Control(pDX, IDC_TREE2, m_tree);
}

void CPPageExternalFilters::StepUp(CCheckListBox& list)
{
	int i = list.GetCurSel();
	if (i < 1) {
		return;
	}

	CString str;
	list.GetText(i, str);
	DWORD_PTR dwItemData = list.GetItemData(i);
	int nCheck = list.GetCheck(i);
	list.DeleteString(i);
	i--;
	list.InsertString(i, str);
	list.SetItemData(i, dwItemData);
	list.SetCheck(i, nCheck);
	list.SetCurSel(i);

	SetModified();
}

void CPPageExternalFilters::StepDown(CCheckListBox& list)
{
	int i = list.GetCurSel();
	if (i < 0 || i >= list.GetCount()-1) {
		return;
	}

	CString str;
	list.GetText(i, str);
	DWORD_PTR dwItemData = list.GetItemData(i);
	int nCheck = list.GetCheck(i);
	list.DeleteString(i);
	i++;
	list.InsertString(i, str);
	list.SetItemData(i, dwItemData);
	list.SetCheck(i, nCheck);
	list.SetCurSel(i);

	SetModified();
}

FilterOverride* CPPageExternalFilters::GetCurFilter()
{
	int i = m_filters.GetCurSel();
	return i >= 0 ? (FilterOverride*)m_pFilters.GetAt((POSITION)m_filters.GetItemDataPtr(i)) : (FilterOverride*)nullptr;
}

void CPPageExternalFilters::SetupMajorTypes(CAtlArray<GUID>& guids)
{
	guids.RemoveAll();
	guids.Add(MEDIATYPE_NULL);
	guids.Add(MEDIATYPE_Video);
	guids.Add(MEDIATYPE_Audio);
	guids.Add(MEDIATYPE_Text);
	guids.Add(MEDIATYPE_Midi);
	guids.Add(MEDIATYPE_Stream);
	guids.Add(MEDIATYPE_Interleaved);
	guids.Add(MEDIATYPE_File);
	guids.Add(MEDIATYPE_ScriptCommand);
	guids.Add(MEDIATYPE_AUXLine21Data);
	guids.Add(MEDIATYPE_VBI);
	guids.Add(MEDIATYPE_Timecode);
	guids.Add(MEDIATYPE_LMRT);
	guids.Add(MEDIATYPE_URL_STREAM);
	guids.Add(MEDIATYPE_MPEG1SystemStream);
	guids.Add(MEDIATYPE_AnalogVideo);
	guids.Add(MEDIATYPE_AnalogAudio);
	guids.Add(MEDIATYPE_MPEG2_PACK);
	guids.Add(MEDIATYPE_MPEG2_PES);
	guids.Add(MEDIATYPE_MPEG2_SECTIONS);
	guids.Add(MEDIATYPE_DVD_ENCRYPTED_PACK);
	guids.Add(MEDIATYPE_DVD_NAVIGATION);
}

void CPPageExternalFilters::SetupSubTypes(CAtlArray<GUID>& guids)
{
	guids.RemoveAll();
	guids.Add(MEDIASUBTYPE_None);
	guids.Add(MEDIASUBTYPE_YUY2);
	guids.Add(MEDIASUBTYPE_MJPG);
	guids.Add(MEDIASUBTYPE_MJPA);
	guids.Add(MEDIASUBTYPE_MJPB);
	guids.Add(MEDIASUBTYPE_TVMJ);
	guids.Add(MEDIASUBTYPE_WAKE);
	guids.Add(MEDIASUBTYPE_CFCC);
	guids.Add(MEDIASUBTYPE_IJPG);
	guids.Add(MEDIASUBTYPE_Plum);
	guids.Add(MEDIASUBTYPE_DVCS);
	guids.Add(MEDIASUBTYPE_DVSD);
	guids.Add(MEDIASUBTYPE_MDVF);
	guids.Add(MEDIASUBTYPE_RGB1);
	guids.Add(MEDIASUBTYPE_RGB4);
	guids.Add(MEDIASUBTYPE_RGB8);
	guids.Add(MEDIASUBTYPE_RGB565);
	guids.Add(MEDIASUBTYPE_RGB555);
	guids.Add(MEDIASUBTYPE_RGB24);
	guids.Add(MEDIASUBTYPE_RGB32);
	guids.Add(MEDIASUBTYPE_ARGB1555);
	guids.Add(MEDIASUBTYPE_ARGB4444);
	guids.Add(MEDIASUBTYPE_ARGB32);
	guids.Add(MEDIASUBTYPE_A2R10G10B10);
	guids.Add(MEDIASUBTYPE_A2B10G10R10);
	guids.Add(MEDIASUBTYPE_AYUV);
	guids.Add(MEDIASUBTYPE_AI44);
	guids.Add(MEDIASUBTYPE_IA44);
	guids.Add(MEDIASUBTYPE_YV12);
	guids.Add(MEDIASUBTYPE_NV12);
	guids.Add(MEDIASUBTYPE_IMC1);
	guids.Add(MEDIASUBTYPE_IMC2);
	guids.Add(MEDIASUBTYPE_IMC3);
	guids.Add(MEDIASUBTYPE_IMC4);
	guids.Add(MEDIASUBTYPE_Overlay);
	guids.Add(MEDIASUBTYPE_MPEG1Packet);
	guids.Add(MEDIASUBTYPE_MPEG1Payload);
	guids.Add(MEDIASUBTYPE_MPEG1AudioPayload);
	guids.Add(MEDIASUBTYPE_MPEG1System);
	guids.Add(MEDIASUBTYPE_MPEG1VideoCD);
	guids.Add(MEDIASUBTYPE_MPEG1Video);
	guids.Add(MEDIASUBTYPE_MPEG1Audio);
	guids.Add(MEDIASUBTYPE_Avi);
	guids.Add(MEDIASUBTYPE_Asf);
	guids.Add(MEDIASUBTYPE_QTMovie);
	guids.Add(MEDIASUBTYPE_QTRpza);
	guids.Add(MEDIASUBTYPE_QTSmc);
	guids.Add(MEDIASUBTYPE_QTRle);
	guids.Add(MEDIASUBTYPE_QTJpeg);
	guids.Add(MEDIASUBTYPE_PCMAudio_Obsolete);
	guids.Add(MEDIASUBTYPE_PCM);
	guids.Add(MEDIASUBTYPE_WAVE);
	guids.Add(MEDIASUBTYPE_AU);
	guids.Add(MEDIASUBTYPE_AIFF);
	guids.Add(MEDIASUBTYPE_dvsd);
	guids.Add(MEDIASUBTYPE_dvhd);
	guids.Add(MEDIASUBTYPE_dvsl);
	guids.Add(MEDIASUBTYPE_dv25);
	guids.Add(MEDIASUBTYPE_dv50);
	guids.Add(MEDIASUBTYPE_dvh1);
	guids.Add(MEDIASUBTYPE_Line21_BytePair);
	guids.Add(MEDIASUBTYPE_Line21_GOPPacket);
	guids.Add(MEDIASUBTYPE_Line21_VBIRawData);
	guids.Add(MEDIASUBTYPE_TELETEXT);
	guids.Add(MEDIASUBTYPE_DRM_Audio);
	guids.Add(MEDIASUBTYPE_IEEE_FLOAT);
	guids.Add(MEDIASUBTYPE_DOLBY_AC3_SPDIF);
	guids.Add(MEDIASUBTYPE_RAW_SPORT);
	guids.Add(MEDIASUBTYPE_SPDIF_TAG_241h);
	guids.Add(MEDIASUBTYPE_DssVideo);
	guids.Add(MEDIASUBTYPE_DssAudio);
	guids.Add(MEDIASUBTYPE_VPVideo);
	guids.Add(MEDIASUBTYPE_VPVBI);
	guids.Add(MEDIASUBTYPE_ATSC_SI);
	guids.Add(MEDIASUBTYPE_DVB_SI);
	guids.Add(MEDIASUBTYPE_MPEG2DATA);
	guids.Add(MEDIASUBTYPE_MPEG2_VIDEO);
	guids.Add(MEDIASUBTYPE_MPEG2_PROGRAM);
	guids.Add(MEDIASUBTYPE_MPEG2_TRANSPORT);
	guids.Add(MEDIASUBTYPE_MPEG2_TRANSPORT_STRIDE);
	guids.Add(MEDIASUBTYPE_MPEG2_AUDIO);
	guids.Add(MEDIASUBTYPE_DOLBY_AC3);
	guids.Add(MEDIASUBTYPE_DVD_SUBPICTURE);
	guids.Add(MEDIASUBTYPE_DVD_LPCM_AUDIO);
	guids.Add(MEDIASUBTYPE_DTS);
	guids.Add(MEDIASUBTYPE_SDDS);
	guids.Add(MEDIASUBTYPE_DVD_NAVIGATION_PCI);
	guids.Add(MEDIASUBTYPE_DVD_NAVIGATION_DSI);
	guids.Add(MEDIASUBTYPE_DVD_NAVIGATION_PROVIDER);
	guids.Add(MEDIASUBTYPE_I420);
	guids.Add(MEDIASUBTYPE_WAVE_DOLBY_AC3);
	guids.Add(MEDIASUBTYPE_DTS2);
}

BEGIN_MESSAGE_MAP(CPPageExternalFilters, CPPageBase)
	ON_UPDATE_COMMAND_UI(IDC_BUTTON2, OnUpdateFilter)
	ON_UPDATE_COMMAND_UI(IDC_RADIO1, OnUpdateFilter)
	ON_UPDATE_COMMAND_UI(IDC_RADIO2, OnUpdateFilter)
	ON_UPDATE_COMMAND_UI(IDC_RADIO3, OnUpdateFilter)
	ON_UPDATE_COMMAND_UI(IDC_BUTTON3, OnUpdateFilterUp)
	ON_UPDATE_COMMAND_UI(IDC_BUTTON4, OnUpdateFilterDown)
	ON_UPDATE_COMMAND_UI(IDC_EDIT1, OnUpdateFilterMerit)
	ON_UPDATE_COMMAND_UI(IDC_BUTTON5, OnUpdateFilter)
	ON_UPDATE_COMMAND_UI(IDC_BUTTON6, OnUpdateSubType)
	ON_UPDATE_COMMAND_UI(IDC_BUTTON7, OnUpdateDeleteType)
	ON_UPDATE_COMMAND_UI(IDC_BUTTON8, OnUpdateFilter)
	ON_BN_CLICKED(IDC_BUTTON1, OnAddRegistered)
	ON_BN_CLICKED(IDC_BUTTON2, OnRemoveFilter)
	ON_BN_CLICKED(IDC_BUTTON3, OnMoveFilterUp)
	ON_BN_CLICKED(IDC_BUTTON4, OnMoveFilterDown)
	ON_LBN_DBLCLK(IDC_LIST1, OnLbnDblclkFilter)
	ON_WM_VKEYTOITEM()
	ON_BN_CLICKED(IDC_BUTTON5, OnAddMajorType)
	ON_BN_CLICKED(IDC_BUTTON6, OnAddSubType)
	ON_BN_CLICKED(IDC_BUTTON7, OnDeleteType)
	ON_BN_CLICKED(IDC_BUTTON8, OnResetTypes)
	ON_LBN_SELCHANGE(IDC_LIST1, OnLbnSelchangeList1)
	ON_CLBN_CHKCHANGE(IDC_LIST1, OnCheckChangeList1)
	ON_BN_CLICKED(IDC_RADIO1, OnBnClickedRadio)
	ON_BN_CLICKED(IDC_RADIO2, OnBnClickedRadio)
	ON_BN_CLICKED(IDC_RADIO3, OnBnClickedRadio)
	ON_EN_CHANGE(IDC_EDIT1, OnEnChangeEdit1)
	ON_NOTIFY(NM_DBLCLK, IDC_TREE2, OnNMDblclkTree2)
	ON_NOTIFY(TVN_KEYDOWN, IDC_TREE2, OnTVNKeyDownTree2)
	ON_WM_DROPFILES()
END_MESSAGE_MAP()


// CPPageExternalFilters message handlers

BOOL CPPageExternalFilters::OnInitDialog()
{
	__super::OnInitDialog();

	DragAcceptFiles(TRUE);

	CAppSettings& s = AfxGetAppSettings();

	m_pFilters.RemoveAll();

	m_filters.SetItemHeight(0, ScaleY(13)+3); // 16 without scale
	POSITION pos = s.m_filters.GetHeadPosition();
	while (pos) {
		CAutoPtr<FilterOverride> f(DNew FilterOverride(s.m_filters.GetNext(pos)));

		CString name(L"<unknown>");

		if (f->type == FilterOverride::REGISTERED) {
			name = CFGFilterRegistry(f->dispname).GetName();
			if (name.IsEmpty()) {
				name = f->name + L" <not registered>";
			}
		} else if (f->type == FilterOverride::EXTERNAL) {
			name = f->name;
			if (f->fTemporary) {
				name += L" <temporary>";
			}
			if (!CPath(MakeFullPath(f->path)).FileExists()) {
				name += L" <not found!>";
			}
		}

		int i = m_filters.AddString(name);
		m_filters.SetCheck(i, f->fDisabled ? 0 : 1);
		m_filters.SetItemDataPtr(i, m_pFilters.AddTail(f));
	}

	UpdateData(FALSE);

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CPPageExternalFilters::OnApply()
{
	UpdateData();

	CAppSettings& s = AfxGetAppSettings();

	s.m_filters.RemoveAll();

	for (int i = 0; i < m_filters.GetCount(); i++) {
		if (POSITION pos = (POSITION)m_filters.GetItemData(i)) {
			CAutoPtr<FilterOverride> f(DNew FilterOverride(m_pFilters.GetAt(pos)));
			f->fDisabled = !m_filters.GetCheck(i);
			s.m_filters.AddTail(f);
		}
	}

	s.SaveExternalFilters();

	return __super::OnApply();
}

void CPPageExternalFilters::OnUpdateFilter(CCmdUI* pCmdUI)
{
	if (FilterOverride* f = GetCurFilter()) {
		pCmdUI->Enable(!(pCmdUI->m_nID == IDC_RADIO2 && f->type == FilterOverride::EXTERNAL));
	} else {
		pCmdUI->Enable(FALSE);
	}
}

void CPPageExternalFilters::OnUpdateFilterUp(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(m_filters.GetCurSel() > 0);
}

void CPPageExternalFilters::OnUpdateFilterDown(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(m_filters.GetCurSel() >= 0 && m_filters.GetCurSel() < m_filters.GetCount()-1);
}

void CPPageExternalFilters::OnUpdateFilterMerit(CCmdUI* pCmdUI)
{
	UpdateData();
	pCmdUI->Enable(m_iLoadType == FilterOverride::MERIT);
}

void CPPageExternalFilters::OnUpdateSubType(CCmdUI* pCmdUI)
{
	HTREEITEM node = m_tree.GetSelectedItem();
	pCmdUI->Enable(node != nullptr && m_tree.GetItemData(node) == NULL);
}

void CPPageExternalFilters::OnUpdateDeleteType(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(!!m_tree.GetSelectedItem());
}

void CPPageExternalFilters::OnAddRegistered()
{
	CRegFilterChooserDlg dlg(this);
	if (dlg.DoModal() == IDOK) {
		while (!dlg.m_filters.empty()) {
			FilterOverride* f = dlg.m_filters.front();
			dlg.m_filters.pop_front();

			if (f) {
				CAutoPtr<FilterOverride> p(f);

				if (f->name.IsEmpty() && !f->guids.size() && !f->dwMerit) {
					// skip something strange
					continue;
				}

				CString name = f->name;

				if (f->type == FilterOverride::EXTERNAL) {
					if (!CPath(MakeFullPath(f->path)).FileExists()) {
						name += L" <not found!>";
					}
				}

				int i = m_filters.AddString(name);
				m_filters.SetItemDataPtr(i, m_pFilters.AddTail(p));
				m_filters.SetCheck(i, 1);

				if (dlg.m_filters.empty()) {
					m_filters.SetCurSel(i);
					OnLbnSelchangeList1();
				}

				SetModified();
			}
		}
	}
}

void CPPageExternalFilters::OnRemoveFilter()
{
	int i = m_filters.GetCurSel();
	m_pFilters.RemoveAt((POSITION)m_filters.GetItemDataPtr(i));
	m_filters.DeleteString(i);

	if (i >= m_filters.GetCount()) {
		i--;
	}
	m_filters.SetCurSel(i);
	OnLbnSelchangeList1();

	SetModified();
}

void CPPageExternalFilters::OnMoveFilterUp()
{
	StepUp(m_filters);
}

void CPPageExternalFilters::OnMoveFilterDown()
{
	StepDown(m_filters);
}

void CPPageExternalFilters::OnLbnDblclkFilter()
{
	if (FilterOverride* f = GetCurFilter()) {
		CComPtr<IBaseFilter> pBF;
		CString name;

		if (f->type == FilterOverride::REGISTERED) {
			CStringW namew;
			if (CreateFilter(f->dispname, &pBF, namew)) {
				name = namew;
			}
		} else if (f->type == FilterOverride::EXTERNAL) {
			if (SUCCEEDED(LoadExternalFilter(f->path, f->clsid, &pBF))) {
				name = f->name;
			}
		}

		if (CComQIPtr<ISpecifyPropertyPages> pSPP = pBF) {
			CComPropertySheet ps(name, this);
			if (ps.AddPages(pSPP) > 0) {
				CComPtr<IFilterGraph> pFG;
				if (SUCCEEDED(pFG.CoCreateInstance(CLSID_FilterGraph))) {
					pFG->AddFilter(pBF, L"");
				}

				ps.DoModal();
			}
		}
	}
}

int CPPageExternalFilters::OnVKeyToItem(UINT nKey, CListBox* pListBox, UINT nIndex)
{
	if (nKey == VK_DELETE) {
		OnRemoveFilter();
		return -2;
	}

	return __super::OnVKeyToItem(nKey, pListBox, nIndex);
}

void CPPageExternalFilters::OnAddMajorType()
{
	FilterOverride* f = GetCurFilter();
	if (!f) {
		return;
	}

	CAtlArray<GUID> guids;
	SetupMajorTypes(guids);

	CSelectMediaType dlg(guids, MEDIATYPE_NULL, this);
	if (dlg.DoModal() == IDOK) {
		auto it = f->guids.begin();
		while (it != f->guids.end() && std::next(it) != f->guids.end()) {
			if (*it++ == dlg.m_guid) {
				AfxMessageBox(ResStr(IDS_EXTERNAL_FILTERS_ERROR_MT), MB_ICONEXCLAMATION | MB_OK);
				return;
			}
			it++;
		}

		f->guids.push_back(dlg.m_guid);
		it = --f->guids.end();
		f->guids.push_back(GUID_NULL);

		CString major = GetMediaTypeName(dlg.m_guid);
		CString sub = GetMediaTypeName(GUID_NULL);

		HTREEITEM node = m_tree.InsertItem(major);
		m_tree.SetItemData(node, NULL);

		node = m_tree.InsertItem(sub, node);
		m_tree.SetItemData(node, (DWORD_PTR)&*it);

		SetModified();
	}
}

void CPPageExternalFilters::OnAddSubType()
{
	FilterOverride* f = GetCurFilter();
	if (!f) {
		return;
	}

	HTREEITEM node = m_tree.GetSelectedItem();
	if (!node) {
		return;
	}

	HTREEITEM child = m_tree.GetChildItem(node);
	if (!child) {
		return;
	}

	auto it = FindInListByPointer(f->guids, (GUID*)m_tree.GetItemData(child));
	if (it == f->guids.end()) {
		ASSERT(0);
		return;
	}

	GUID major = *it;

	CAtlArray<GUID> guids;
	SetupSubTypes(guids);

	CSelectMediaType dlg(guids, MEDIASUBTYPE_NULL, this);
	if (dlg.DoModal() == IDOK) {
		for (child = m_tree.GetChildItem(node); child; child = m_tree.GetNextSiblingItem(child)) {
			it = FindInListByPointer(f->guids, (GUID*)m_tree.GetItemData(child));
			it++;
			if (*it == dlg.m_guid) {
				AfxMessageBox(ResStr(IDS_EXTERNAL_FILTERS_ERROR_MT), MB_ICONEXCLAMATION | MB_OK);
				return;
			}
		}

		f->guids.push_back(major);
		it = --f->guids.end(); // iterator for major
		f->guids.push_back(dlg.m_guid);

		CString sub = GetMediaTypeName(dlg.m_guid);

		node = m_tree.InsertItem(sub, node);
		m_tree.SetItemData(node, (DWORD_PTR)&*it);

		SetModified();
	}
}

void CPPageExternalFilters::OnDeleteType()
{
	if (FilterOverride* f = GetCurFilter()) {
		HTREEITEM node = m_tree.GetSelectedItem();
		if (!node) {
			return;
		}

		auto it = FindInListByPointer(f->guids, (GUID*)m_tree.GetItemData(node));

		if (it == f->guids.end()) {
			for (HTREEITEM child = m_tree.GetChildItem(node); child; child = m_tree.GetNextSiblingItem(child)) {
				it = FindInListByPointer(f->guids, (GUID*)m_tree.GetItemData(child));

				f->guids.erase(it++);
				f->guids.erase(it++);
			}

			m_tree.DeleteItem(node);
		} else {
			HTREEITEM parent = m_tree.GetParentItem(node);

			m_tree.DeleteItem(node);

			if (!m_tree.ItemHasChildren(parent)) {
				auto it1 = it++;
				*it = GUID_NULL;
				node = m_tree.InsertItem(GetMediaTypeName(GUID_NULL), parent);
				m_tree.SetItemData(node, (DWORD_PTR)&*it1);
			} else {
				f->guids.erase(it++);
				f->guids.erase(it++);
			}
		}

		SetModified();
	}
}

void CPPageExternalFilters::OnResetTypes()
{
	if (FilterOverride* f = GetCurFilter()) {
		if (f->type == FilterOverride::REGISTERED) {
			CFGFilterRegistry fgf(f->dispname);
			if (!fgf.GetName().IsEmpty()) {
				//f->guids.clear();
				//f->backup.clear();
				f->guids = fgf.GetTypes();
				f->backup = fgf.GetTypes();
			} else {
				//f->guids.clear();
				f->guids = f->backup;
			}
		} else {
			//f->guids.clear();
			f->guids = f->backup;
		}

		m_pLastSelFilter = nullptr;
		OnLbnSelchangeList1();

		SetModified();
	}
}

void CPPageExternalFilters::OnLbnSelchangeList1()
{
	if (FilterOverride* f = GetCurFilter()) {
		if (m_pLastSelFilter == f) {
			return;
		}
		m_pLastSelFilter = f;

		m_iLoadType = f->iLoadType;
		UpdateData(FALSE);
		m_dwMerit = f->dwMerit;

		HTREEITEM dummy_item = m_tree.InsertItem(L"", 0,0, nullptr, TVI_FIRST);
		if (dummy_item)
			for (HTREEITEM item = m_tree.GetNextVisibleItem(dummy_item); item; item = m_tree.GetNextVisibleItem(dummy_item)) {
				m_tree.DeleteItem(item);
			}

		CMapStringToPtr map;

		auto it = f->guids.begin();
		while (it != f->guids.end() && std::next(it) != f->guids.end()) {
			GUID* tmp = &*it;
			CString major = GetMediaTypeName(*it++);
			CString sub = GetMediaTypeName(*it++);

			HTREEITEM node = nullptr;

			void* val = nullptr;
			if (map.Lookup(major, val)) {
				node = (HTREEITEM)val;
			} else {
				map[major] = node = m_tree.InsertItem(major);
			}
			m_tree.SetItemData(node, NULL);

			node = m_tree.InsertItem(sub, node);
			m_tree.SetItemData(node, (DWORD_PTR)tmp);
		}

		m_tree.DeleteItem(dummy_item);

		for (HTREEITEM item = m_tree.GetFirstVisibleItem(); item; item = m_tree.GetNextVisibleItem(item)) {
			m_tree.Expand(item, TVE_EXPAND);
		}

		m_tree.EnsureVisible(m_tree.GetRootItem());
	} else {
		m_pLastSelFilter = nullptr;

		m_iLoadType = FilterOverride::PREFERRED;
		UpdateData(FALSE);
		m_dwMerit = 0;

		m_tree.DeleteAllItems();
	}
}

void CPPageExternalFilters::OnCheckChangeList1()
{
	SetModified();
}

void CPPageExternalFilters::OnBnClickedRadio()
{
	UpdateData();

	FilterOverride* f = GetCurFilter();
	if (f && f->iLoadType != m_iLoadType) {
		f->iLoadType = m_iLoadType;

		SetModified();
	}
}

void CPPageExternalFilters::OnEnChangeEdit1()
{
	UpdateData();
	if (FilterOverride* f = GetCurFilter()) {
		DWORD dw;
		if (m_dwMerit.GetDWORD(dw) && f->dwMerit != dw) {
			f->dwMerit = dw;

			SetModified();
		}
	}
}

void CPPageExternalFilters::OnNMDblclkTree2(NMHDR *pNMHDR, LRESULT *pResult)
{
	*pResult = 0;

	if (FilterOverride* f = GetCurFilter()) {
		HTREEITEM node = m_tree.GetSelectedItem();
		if (!node) {
			return;
		}

		auto it = FindInListByPointer(f->guids, (GUID*)m_tree.GetItemData(node));
		if (it == f->guids.end()) {
			ASSERT(0);
			return;
		}

		it++;
		if (it == f->guids.end()) {
			return;
		}

		CAtlArray<GUID> guids;
		SetupSubTypes(guids);

		CSelectMediaType dlg(guids, *it, this);
		if (dlg.DoModal() == IDOK) {
			*it = dlg.m_guid;
			m_tree.SetItemText(node, GetMediaTypeName(dlg.m_guid));

			SetModified();
		}
	}
}

void CPPageExternalFilters::OnTVNKeyDownTree2(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMTVKEYDOWN pTVKeyDown = reinterpret_cast<LPNMTVKEYDOWN>(pNMHDR);

	if (pTVKeyDown->wVKey == VK_DELETE) {
		OnDeleteType();
	}

	*pResult = 0;
}

void CPPageExternalFilters::OnDropFiles(HDROP hDropInfo)
{
	SetActiveWindow();

	UINT nFiles = ::DragQueryFileW(hDropInfo, (UINT)-1, nullptr, 0);
	for (UINT iFile = 0; iFile < nFiles; iFile++) {
		WCHAR szFileName[MAX_PATH];
		::DragQueryFileW(hDropInfo, iFile, szFileName, MAX_PATH);

		CFilterMapper2 fm2(false);
		fm2.Register(szFileName);

		while (!fm2.m_filters.empty()) {
			FilterOverride* f = fm2.m_filters.front();
			fm2.m_filters.pop_front();

			if (f) {
				CAutoPtr<FilterOverride> p(f);
				int i = m_filters.AddString(f->name);
				m_filters.SetItemDataPtr(i, m_pFilters.AddTail(p));
				m_filters.SetCheck(i, 1);

				if (fm2.m_filters.empty()) {
					m_filters.SetCurSel(i);
					OnLbnSelchangeList1();
				}

				SetModified();
			}
		}
	}
	::DragFinish(hDropInfo);
}
