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

// CComPropertyPage dialog

class CComPropertyPage : public CPropertyPage
{
	DECLARE_DYNAMIC(CComPropertyPage)

	CComPtr<IPropertyPage> m_pPage;

public:
	CComPropertyPage(IPropertyPage* pPage);
	virtual ~CComPropertyPage();

	enum { IDD = IDD_COMPROPERTYPAGE };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL OnSetActive();
	virtual BOOL OnKillActive();

	DECLARE_MESSAGE_MAP()

public:
	virtual BOOL OnInitDialog();
	afx_msg void OnDestroy();
	virtual void OnOK();
};

// CComPropertySheet

interface IComPropertyPageDirty {
	virtual void OnSetDirty(bool fDirty) PURE;
};

class CComPropertySheet : public CPropertySheet, public IComPropertyPageDirty
{
	DECLARE_DYNAMIC(CComPropertySheet)

	CComPtr<IPropertyPageSite> m_pSite;
	CInterfaceList<ISpecifyPropertyPages> m_spp;
	CAutoPtrList<CComPropertyPage> m_pages;
	CSize m_size;

public:
	CComPropertySheet(UINT nIDCaption, CWnd* pParentWnd = nullptr, UINT iSelectPage = 0);
	CComPropertySheet(LPCTSTR pszCaption, CWnd* pParentWnd = nullptr, UINT iSelectPage = 0);
	virtual ~CComPropertySheet();

	int AddPages(ISpecifyPropertyPages* pSPP);
	bool AddPage(IPropertyPage* pPage, IUnknown* pUnk);

	void OnActivated(CPropertyPage* pPage);

	void OnSetDirty(bool fDirty) {
		if (CPropertyPage* p = GetActivePage()) {
			p->SetModified(fDirty);
		}
	}

	virtual BOOL OnInitDialog();

protected:
	DECLARE_MESSAGE_MAP()
};
