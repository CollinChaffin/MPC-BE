/*
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

#include <d3d9.h>
#include "MediaSampleSideData.h"

class CMPCVideoDecFilter;
class CVideoDecDXVAAllocator;

interface __declspec(uuid("AE7EC2A2-1913-4a80-8DD6-DF1497ABA494"))
IMPCDXVA2Sample :
public IUnknown {
	STDMETHOD_(int, GetDXSurfaceId()) PURE;
};

class CDXVA2Sample : public CMediaSampleSideData, public IMFGetService, public IMPCDXVA2Sample
{
	friend class CVideoDecDXVAAllocator;

public:
	CDXVA2Sample(CVideoDecDXVAAllocator *pAlloc, HRESULT *phr);

	//Note: CMediaSample does not derive from CUnknown, so we cannot use the
	//		DECLARE_IUNKNOWN macro that is used by most of the filter classes.

	STDMETHODIMP         QueryInterface(REFIID riid, __deref_out void **ppv);
	STDMETHODIMP_(ULONG) AddRef();
	STDMETHODIMP_(ULONG) Release();

	// IMFGetService::GetService
	STDMETHODIMP GetService(REFGUID guidService, REFIID riid, LPVOID *ppv);

	// IMPCDXVA2Sample
	STDMETHODIMP_(int) GetDXSurfaceId();

	// Override GetPointer because this class does not manage a system memory buffer.
	// The EVR uses the MR_BUFFER_SERVICE service to get the Direct3D surface.
	STDMETHODIMP GetPointer(BYTE ** ppBuffer);

private:
	// Sets the pointer to the Direct3D surface.
	void SetSurface(DWORD surfaceId, IDirect3DSurface9 *pSurf);

	CComPtr<IDirect3DSurface9> m_pSurface;
	DWORD m_dwSurfaceId;
};

class CVideoDecDXVAAllocator : public CBaseAllocator
{
public:
	CVideoDecDXVAAllocator(CMPCVideoDecFilter* pVideoDecFilter, HRESULT* phr);
	virtual ~CVideoDecDXVAAllocator();

	// *** from LAV
	STDMETHODIMP_(BOOL) DecommitInProgress() { CAutoLock lock(this); return m_bDecommitInProgress; }

protected:
	HRESULT Alloc(void);
	void    Free(void);

private :
	CMPCVideoDecFilter* m_pVideoDecFilter;

	IDirect3DSurface9** m_ppRTSurfaceArray;
	UINT                m_nSurfaceArrayCount;
};
