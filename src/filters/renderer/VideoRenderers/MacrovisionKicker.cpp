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
#include "MacrovisionKicker.h"


//
// CMacrovisionKicker
//

CMacrovisionKicker::CMacrovisionKicker(const TCHAR* pName, LPUNKNOWN pUnk)
	: CUnknown(pName, pUnk)
{
}

CMacrovisionKicker::~CMacrovisionKicker()
{
}

void CMacrovisionKicker::SetInner(IUnknown* pUnk)
{
	m_pInner = pUnk;
}

STDMETHODIMP CMacrovisionKicker::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
	if (riid == __uuidof(IUnknown)) {
		return __super::NonDelegatingQueryInterface(riid, ppv);
	}
	if (riid == __uuidof(IKsPropertySet) && CComQIPtr<IKsPropertySet>(m_pInner)) {
		return GetInterface((IKsPropertySet*)this, ppv);
	}

	HRESULT hr = m_pInner ? m_pInner->QueryInterface(riid, ppv) : E_NOINTERFACE;

	return SUCCEEDED(hr) ? hr : __super::NonDelegatingQueryInterface(riid, ppv);
}

// IKsPropertySet

STDMETHODIMP CMacrovisionKicker::Set(REFGUID PropSet, ULONG Id, LPVOID pInstanceData, ULONG InstanceLength, LPVOID pPropertyData, ULONG DataLength)
{
	if (CComQIPtr<IKsPropertySet> pKsPS = m_pInner) {
		if (PropSet == AM_KSPROPSETID_CopyProt && Id == AM_PROPERTY_COPY_MACROVISION
				/*&& DataLength == 4 && *(DWORD*)pPropertyData*/) {
			TRACE(L"Oops, no-no-no, no macrovision please\n");
			return S_OK;
		}

		return pKsPS->Set(PropSet, Id, pInstanceData, InstanceLength, pPropertyData, DataLength);
	}

	return E_UNEXPECTED;
}

STDMETHODIMP CMacrovisionKicker::Get(REFGUID PropSet, ULONG Id, LPVOID pInstanceData, ULONG InstanceLength, LPVOID pPropertyData, ULONG DataLength, ULONG* pBytesReturned)
{
	if (CComQIPtr<IKsPropertySet> pKsPS = m_pInner) {
		return pKsPS->Get(PropSet, Id, pInstanceData, InstanceLength, pPropertyData, DataLength, pBytesReturned);
	}

	return E_UNEXPECTED;
}

STDMETHODIMP CMacrovisionKicker::QuerySupported(REFGUID PropSet, ULONG Id, ULONG* pTypeSupport)
{
	if (CComQIPtr<IKsPropertySet> pKsPS = m_pInner) {
		return pKsPS->QuerySupported(PropSet, Id, pTypeSupport);
	}

	return E_UNEXPECTED;
}
