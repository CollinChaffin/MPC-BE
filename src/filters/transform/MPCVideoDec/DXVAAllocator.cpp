/*
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
#include "DXVAAllocator.h"
#include "MPCVideoDec.h"

CDXVA2Sample::CDXVA2Sample(CVideoDecDXVAAllocator *pAlloc, HRESULT *phr)
	: CMediaSample(NAME("CDXVA2Sample"), (CBaseAllocator*)pAlloc, phr, NULL, 0)
	, m_dwSurfaceId(0)
{
}

//Note: CMediaSample does not derive from CUnknown, so we cannot use the
//		DECLARE_IUNKNOWN macro that is used by most of the filter classes.

STDMETHODIMP CDXVA2Sample::QueryInterface(REFIID riid, __deref_out void **ppv)
{
	CheckPointer(ppv, E_POINTER);
	ValidateReadWritePtr(ppv, sizeof(PVOID));

	if (riid == __uuidof(IMFGetService)) {
		return GetInterface((IMFGetService*)this, ppv);
	}
	if (riid == __uuidof(IMPCDXVA2Sample)) {
		return GetInterface((IMPCDXVA2Sample*)this, ppv);
	} else {
		return CMediaSample::QueryInterface(riid, ppv);
	}
}

STDMETHODIMP_(ULONG) CDXVA2Sample::AddRef()
{
	return __super::AddRef();
}

STDMETHODIMP_(ULONG) CDXVA2Sample::Release()
{
	// Return a temporary variable for thread safety.
	ULONG cRef = __super::Release();
	return cRef;
}

// IMFGetService::GetService
STDMETHODIMP CDXVA2Sample::GetService(REFGUID guidService, REFIID riid, LPVOID *ppv)
{
	if (guidService != MR_BUFFER_SERVICE) {
		return MF_E_UNSUPPORTED_SERVICE;
	} else if (m_pSurface == NULL) {
		return E_NOINTERFACE;
	} else {
		return m_pSurface->QueryInterface(riid, ppv);
	}
}

// Override GetPointer because this class does not manage a system memory buffer.
// The EVR uses the MR_BUFFER_SERVICE service to get the Direct3D surface.
STDMETHODIMP CDXVA2Sample::GetPointer(BYTE ** ppBuffer)
{
	return E_NOTIMPL;
}

// Sets the pointer to the Direct3D surface.
void CDXVA2Sample::SetSurface(DWORD surfaceId, IDirect3DSurface9 *pSurf)
{
	m_pSurface = pSurf;
	m_dwSurfaceId = surfaceId;
}

STDMETHODIMP_(int) CDXVA2Sample::GetDXSurfaceId()
{
	return m_dwSurfaceId;
}

CVideoDecDXVAAllocator::CVideoDecDXVAAllocator(CMPCVideoDecFilter* pVideoDecFilter,  HRESULT* phr)
	: CBaseAllocator(NAME("CVideoDecDXVAAllocator"), NULL, phr)
	, m_pVideoDecFilter(pVideoDecFilter)
	, m_ppRTSurfaceArray(NULL)
	, m_nSurfaceArrayCount(0)
{
}

CVideoDecDXVAAllocator::~CVideoDecDXVAAllocator()
{
	DLog(L"CVideoDecDXVAAllocator::~CVideoDecDXVAAllocator()");

	if (m_pVideoDecFilter) {
		m_pVideoDecFilter->FlushDXVADecoder();
	}
	Free();
	if (m_pVideoDecFilter && m_pVideoDecFilter->m_pDXVA2Allocator == this) {
		m_pVideoDecFilter->m_pDXVA2Allocator = NULL;
	}
}

HRESULT CVideoDecDXVAAllocator::Alloc()
{
	HRESULT hr;
	CComPtr<IDirectXVideoDecoderService> pDXVA2Service;

	CheckPointer(m_pVideoDecFilter->m_pDeviceManager, E_UNEXPECTED);
	hr = m_pVideoDecFilter->m_pDeviceManager->GetVideoService(m_pVideoDecFilter->m_hDevice, IID_PPV_ARGS(&pDXVA2Service));
	CheckPointer(pDXVA2Service, E_UNEXPECTED);
	CAutoLock lock(this);

	hr = __super::Alloc();

	if (SUCCEEDED(hr)) {
		// Free the old resources.
		m_pVideoDecFilter->FlushDXVADecoder();
		Free();

		m_nSurfaceArrayCount = m_lCount;

		// Allocate a new array of pointers.
		m_ppRTSurfaceArray = DNew IDirect3DSurface9*[m_lCount];
		if (m_ppRTSurfaceArray == NULL) {
			hr = E_OUTOFMEMORY;
		} else {
			ZeroMemory(m_ppRTSurfaceArray, sizeof(IDirect3DSurface9*) * m_lCount);
		}
	}

	// Allocate the surfaces.
	D3DFORMAT dwFormat = m_pVideoDecFilter->m_VideoDesc.Format;
	if (SUCCEEDED(hr)) {
		hr = pDXVA2Service->CreateSurface(
				m_pVideoDecFilter->PictWidthAligned(),
				m_pVideoDecFilter->PictHeightAligned(),
				m_lCount - 1,
				dwFormat,
				D3DPOOL_DEFAULT,
				0,
				DXVA2_VideoDecoderRenderTarget,
				m_ppRTSurfaceArray,
				NULL
			 );
	}

	if (FAILED(hr)) {
		DLog(L"CVideoDecDXVAAllocator::Alloc() : IDirectXVideoDecoderService::CreateSurface() - FAILED (0x%08x)", hr);
	}

	if (SUCCEEDED(hr)) {
		// get the device, for ColorFill() to init the surfaces in black
		CComPtr<IDirect3DDevice9> pDev;
		m_ppRTSurfaceArray[0]->GetDevice(&pDev);

		// Important : create samples in reverse order !
		for (m_lAllocated = m_lCount - 1; m_lAllocated >= 0; m_lAllocated--) {
			// fill the surface in black, to avoid the "green screen" in case the first frame fails to decode.
			if (pDev) {
				pDev->ColorFill(m_ppRTSurfaceArray[m_lAllocated], NULL, D3DCOLOR_XYUV(0, 128, 128));
			}

			CDXVA2Sample *pSample = DNew CDXVA2Sample(this, &hr);
			if (pSample == NULL) {
				hr = E_OUTOFMEMORY;
				break;
			}
			if (FAILED(hr)) {
				break;
			}
			// Assign the Direct3D surface pointer and the index.
			pSample->SetSurface(m_lAllocated, m_ppRTSurfaceArray[m_lAllocated]);

			// Add to the sample list.
			m_lFree.Add(pSample);
		}

		hr = m_pVideoDecFilter->CreateDXVA2Decoder(m_lCount, m_ppRTSurfaceArray);
		if (FAILED(hr)) {
			DLog(L"CVideoDecDXVAAllocator::Alloc() : CMPCVideoDecFilter::CreateDXVA2Decoder() - FAILED (0x%08x)", hr);
			Free();
		}
	}

	if (SUCCEEDED(hr)) {
		m_bChanged = FALSE;
	}

	return hr;
}

void CVideoDecDXVAAllocator::Free()
{
	CMediaSample *pSample = NULL;
	CAutoLock lock(this);

	do {
		pSample = m_lFree.RemoveHead();
		if (pSample) {
			delete pSample;
		}
	} while (pSample);

	if (m_ppRTSurfaceArray) {
		for (UINT i = 0; i < m_nSurfaceArrayCount; i++) {
			if (m_ppRTSurfaceArray[i] != NULL) {
				m_ppRTSurfaceArray[i]->Release();
			}
		}

		delete [] m_ppRTSurfaceArray;
		m_ppRTSurfaceArray = NULL;
	}
	m_lAllocated		 = 0;
	m_nSurfaceArrayCount = 0;
}
