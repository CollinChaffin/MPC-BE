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

#pragma once

#include "AllocatorCommon.h"
#include "../SubPic/SubPicAllocatorPresenterImpl.h"
#include "../SubPic/ISubRender.h"

namespace DSObjects
{
	class CDXRAllocatorPresenter
		: public CSubPicAllocatorPresenterImpl
	{
		class CSubRenderCallback : public CUnknown, public ISubRenderCallback, public CCritSec
		{
			CDXRAllocatorPresenter* m_pDXRAP;

		public:
			CSubRenderCallback(CDXRAllocatorPresenter* pDXRAP)
				: CUnknown(L"CSubRender", NULL)
				, m_pDXRAP(pDXRAP) {
			}

			DECLARE_IUNKNOWN
			STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv) {
				return
					QI(ISubRenderCallback)
					__super::NonDelegatingQueryInterface(riid, ppv);
			}

			void SetDXRAP(CDXRAllocatorPresenter* pDXRAP) {
				CAutoLock cAutoLock(this);
				m_pDXRAP = pDXRAP;
			}

			// ISubRenderCallback

			STDMETHODIMP SetDevice(IDirect3DDevice9* pD3DDev) {
				CAutoLock cAutoLock(this);
				return m_pDXRAP ? m_pDXRAP->SetDevice(pD3DDev) : E_UNEXPECTED;
			}

			STDMETHODIMP Render(REFERENCE_TIME rtStart, int left, int top, int right, int bottom, int width, int height) {
				CAutoLock cAutoLock(this);
				return m_pDXRAP ? m_pDXRAP->Render(rtStart, 0, 0, left, top, right, bottom, width, height) : E_UNEXPECTED;
			}

			// ISubRendererCallback2

			STDMETHODIMP RenderEx(REFERENCE_TIME rtStart, REFERENCE_TIME rtStop, REFERENCE_TIME AvgTimePerFrame, int left, int top, int right, int bottom, int width, int height) {
				CAutoLock cAutoLock(this);
				return m_pDXRAP ? m_pDXRAP->Render(rtStart, rtStop, AvgTimePerFrame, left, top, right, bottom, width, height) : E_UNEXPECTED;
			}
		};

		CComPtr<IUnknown> m_pDXR;
		CComPtr<ISubRenderCallback> m_pSRCB;
		CSize	m_ScreenSize;

	public:
		CDXRAllocatorPresenter(HWND hWnd, HRESULT& hr, CString &_Error);
		virtual ~CDXRAllocatorPresenter();

		DECLARE_IUNKNOWN
		STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);

		HRESULT SetDevice(IDirect3DDevice9* pD3DDev);
		HRESULT Render(
			REFERENCE_TIME rtStart, REFERENCE_TIME rtStop, REFERENCE_TIME atpf,
			int left, int top, int bottom, int right, int width, int height);

		// ISubPicAllocatorPresenter3
		STDMETHODIMP CreateRenderer(IUnknown** ppRenderer);
		STDMETHODIMP_(SIZE) GetVideoSize();
		STDMETHODIMP_(SIZE) GetVideoSizeAR();
		STDMETHODIMP_(void) SetPosition(RECT w, RECT v);
		STDMETHODIMP GetDIB(BYTE* lpDib, DWORD* size);
		STDMETHODIMP_(bool) IsRendering() {
			return true; // We don't know so we always pretend to be rendering
		}
	};
}
