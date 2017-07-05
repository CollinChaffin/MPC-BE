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
#include "../../../AudioTools/AudioHelper.h"
#include "../../../DSUtil/DSUtil.h"
#include "StreamSwitcher.h"

#ifdef REGISTER_FILTER
#include <InitGuid.h>
#endif
#include <moreuuids.h>

#define PauseGraph \
	CComQIPtr<IMediaControl> _pMC(m_pGraph); \
	OAFilterState _fs = -1; \
	if(_pMC) _pMC->GetState(1000, &_fs); \
	if(_fs == State_Running) \
		_pMC->Pause(); \
 \
	HRESULT _hr = E_FAIL; \
	CComQIPtr<IMediaSeeking> _pMS((IUnknown*)(INonDelegatingUnknown*)m_pGraph); \
	LONGLONG _rtNow = 0; \
	if(_pMS) _hr = _pMS->GetCurrentPosition(&_rtNow); \
 
#define ResumeGraph \
	if(SUCCEEDED(_hr) && _pMS && _fs != State_Stopped) \
		_hr = _pMS->SetPositions(&_rtNow, AM_SEEKING_AbsolutePositioning, NULL, AM_SEEKING_NoPositioning); \
 \
	if(_fs == State_Running && _pMS) \
		_pMC->Run(); \
 

//
// CStreamSwitcherPassThru
//

CStreamSwitcherPassThru::CStreamSwitcherPassThru(LPUNKNOWN pUnk, HRESULT* phr, CStreamSwitcherFilter* pFilter)
	: CMediaPosition(NAME("CStreamSwitcherPassThru"), pUnk)
	, m_pFilter(pFilter)
{
}

STDMETHODIMP CStreamSwitcherPassThru::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
	CheckPointer(ppv, E_POINTER);
	*ppv = NULL;

	return
		QI(IMediaSeeking)
		CMediaPosition::NonDelegatingQueryInterface(riid, ppv);
}

template<class T>
HRESULT GetPeer(CStreamSwitcherFilter* pFilter, T** ppT)
{
	*ppT = NULL;

	CBasePin* pPin = pFilter->GetInputPin();
	if (!pPin) {
		return E_NOTIMPL;
	}

	CComPtr<IPin> pConnected;
	if (FAILED(pPin->ConnectedTo(&pConnected))) {
		return E_NOTIMPL;
	}

	if (CComQIPtr<T> pT = pConnected) {
		*ppT = pT.Detach();
		return S_OK;
	}

	return E_NOTIMPL;
}

#define CallPeerSeeking(call) \
	CComPtr<IMediaSeeking> pMS; \
	if(FAILED(GetPeer(m_pFilter, &pMS))) return E_NOTIMPL; \
	return pMS->##call; \
 
#define CallPeer(call) \
	CComPtr<IMediaPosition> pMP; \
	if(FAILED(GetPeer(m_pFilter, &pMP))) return E_NOTIMPL; \
	return pMP->##call; \
 
#define CallPeerSeekingAll(call) \
	HRESULT hr = E_NOTIMPL; \
	POSITION pos = m_pFilter->m_pInputs.GetHeadPosition(); \
	while(pos) \
	{ \
		CBasePin* pPin = m_pFilter->m_pInputs.GetNext(pos); \
		CComPtr<IPin> pConnected; \
		if(FAILED(pPin->ConnectedTo(&pConnected))) \
			continue; \
		if(CComQIPtr<IMediaSeeking> pMS = pConnected) \
		{ \
			HRESULT hr2 = pMS->call; \
			if(pPin == m_pFilter->GetInputPin()) \
				hr = hr2; \
		} \
	} \
	return hr; \
 
#define CallPeerAll(call) \
	HRESULT hr = E_NOTIMPL; \
	POSITION pos = m_pFilter->m_pInputs.GetHeadPosition(); \
	while(pos) \
	{ \
		CBasePin* pPin = m_pFilter->m_pInputs.GetNext(pos); \
		CComPtr<IPin> pConnected; \
		if(FAILED(pPin->ConnectedTo(&pConnected))) \
			continue; \
		if(CComQIPtr<IMediaPosition> pMP = pConnected) \
		{ \
			HRESULT hr2 = pMP->call; \
			if(pPin == m_pFilter->GetInputPin()) \
				hr = hr2; \
		} \
	} \
	return hr; \
 

// IMediaSeeking

STDMETHODIMP CStreamSwitcherPassThru::GetCapabilities(DWORD* pCaps)
{
	CallPeerSeeking(GetCapabilities(pCaps));
}

STDMETHODIMP CStreamSwitcherPassThru::CheckCapabilities(DWORD* pCaps)
{
	CallPeerSeeking(CheckCapabilities(pCaps));
}

STDMETHODIMP CStreamSwitcherPassThru::IsFormatSupported(const GUID* pFormat)
{
	CallPeerSeeking(IsFormatSupported(pFormat));
}

STDMETHODIMP CStreamSwitcherPassThru::QueryPreferredFormat(GUID* pFormat)
{
	CallPeerSeeking(QueryPreferredFormat(pFormat));
}

STDMETHODIMP CStreamSwitcherPassThru::SetTimeFormat(const GUID* pFormat)
{
	CallPeerSeeking(SetTimeFormat(pFormat));
}

STDMETHODIMP CStreamSwitcherPassThru::GetTimeFormat(GUID* pFormat)
{
	CallPeerSeeking(GetTimeFormat(pFormat));
}

STDMETHODIMP CStreamSwitcherPassThru::IsUsingTimeFormat(const GUID* pFormat)
{
	CallPeerSeeking(IsUsingTimeFormat(pFormat));
}

STDMETHODIMP CStreamSwitcherPassThru::ConvertTimeFormat(LONGLONG* pTarget, const GUID* pTargetFormat, LONGLONG Source, const GUID* pSourceFormat)
{
	CallPeerSeeking(ConvertTimeFormat(pTarget, pTargetFormat, Source, pSourceFormat));
}

STDMETHODIMP CStreamSwitcherPassThru::SetPositions(LONGLONG* pCurrent, DWORD CurrentFlags, LONGLONG* pStop, DWORD StopFlags)
{
	CallPeerSeekingAll(SetPositions(pCurrent, CurrentFlags, pStop, StopFlags));
}

STDMETHODIMP CStreamSwitcherPassThru::GetPositions(LONGLONG* pCurrent, LONGLONG* pStop)
{
	CallPeerSeeking(GetPositions(pCurrent, pStop));
}

STDMETHODIMP CStreamSwitcherPassThru::GetCurrentPosition(LONGLONG* pCurrent)
{
	CallPeerSeeking(GetCurrentPosition(pCurrent));
}

STDMETHODIMP CStreamSwitcherPassThru::GetStopPosition(LONGLONG* pStop)
{
	CallPeerSeeking(GetStopPosition(pStop));
}

STDMETHODIMP CStreamSwitcherPassThru::GetDuration(LONGLONG* pDuration)
{
	CallPeerSeeking(GetDuration(pDuration));
}

STDMETHODIMP CStreamSwitcherPassThru::GetPreroll(LONGLONG* pllPreroll)
{
	CallPeerSeeking(GetPreroll(pllPreroll));
}

STDMETHODIMP CStreamSwitcherPassThru::GetAvailable(LONGLONG* pEarliest, LONGLONG* pLatest)
{
	CallPeerSeeking(GetAvailable(pEarliest, pLatest));
}

STDMETHODIMP CStreamSwitcherPassThru::GetRate(double* pdRate)
{
	CallPeerSeeking(GetRate(pdRate));
}

STDMETHODIMP CStreamSwitcherPassThru::SetRate(double dRate)
{
	if (0.0 == dRate) {
		return E_INVALIDARG;
	}
	CallPeerSeekingAll(SetRate(dRate));
}

// IMediaPosition

STDMETHODIMP CStreamSwitcherPassThru::get_Duration(REFTIME* plength)
{
	CallPeer(get_Duration(plength));
}

STDMETHODIMP CStreamSwitcherPassThru::get_CurrentPosition(REFTIME* pllTime)
{
	CallPeer(get_CurrentPosition(pllTime));
}

STDMETHODIMP CStreamSwitcherPassThru::put_CurrentPosition(REFTIME llTime)
{
	CallPeerAll(put_CurrentPosition(llTime));
}

STDMETHODIMP CStreamSwitcherPassThru::get_StopTime(REFTIME* pllTime)
{
	CallPeer(get_StopTime(pllTime));
}

STDMETHODIMP CStreamSwitcherPassThru::put_StopTime(REFTIME llTime)
{
	CallPeerAll(put_StopTime(llTime));
}

STDMETHODIMP CStreamSwitcherPassThru::get_PrerollTime(REFTIME * pllTime)
{
	CallPeer(get_PrerollTime(pllTime));
}

STDMETHODIMP CStreamSwitcherPassThru::put_PrerollTime(REFTIME llTime)
{
	CallPeerAll(put_PrerollTime(llTime));
}

STDMETHODIMP CStreamSwitcherPassThru::get_Rate(double* pdRate)
{
	CallPeer(get_Rate(pdRate));
}

STDMETHODIMP CStreamSwitcherPassThru::put_Rate(double dRate)
{
	if (0.0 == dRate) {
		return E_INVALIDARG;
	}
	CallPeerAll(put_Rate(dRate));
}

STDMETHODIMP CStreamSwitcherPassThru::CanSeekForward(LONG* pCanSeekForward)
{
	CallPeer(CanSeekForward(pCanSeekForward));
}

STDMETHODIMP CStreamSwitcherPassThru::CanSeekBackward(LONG* pCanSeekBackward)
{
	CallPeer(CanSeekBackward(pCanSeekBackward));
}

//
// CStreamSwitcherAllocator
//

CStreamSwitcherAllocator::CStreamSwitcherAllocator(CStreamSwitcherInputPin* pPin, HRESULT* phr)
	: CMemAllocator(NAME("CStreamSwitcherAllocator"), NULL, phr)
	, m_pPin(pPin)
	, m_fMediaTypeChanged(false)
{
	ASSERT(phr);
	ASSERT(pPin);
}

#ifdef _DEBUG
CStreamSwitcherAllocator::~CStreamSwitcherAllocator()
{
	ASSERT(m_bCommitted == FALSE);
}
#endif

STDMETHODIMP_(ULONG) CStreamSwitcherAllocator::NonDelegatingAddRef()
{
	return m_pPin->m_pFilter->AddRef();
}

STDMETHODIMP_(ULONG) CStreamSwitcherAllocator::NonDelegatingRelease()
{
	return m_pPin->m_pFilter->Release();
}

STDMETHODIMP CStreamSwitcherAllocator::GetBuffer(
	IMediaSample** ppBuffer,
	REFERENCE_TIME* pStartTime, REFERENCE_TIME* pEndTime,
	DWORD dwFlags)
{
	HRESULT hr = VFW_E_NOT_COMMITTED;

	if (!m_bCommitted) {
		return hr;
	}

	if (m_fMediaTypeChanged) {
		if (!m_pPin || !m_pPin->m_pFilter) {
			return hr;
		}

		CStreamSwitcherOutputPin* pOut = (static_cast<CStreamSwitcherFilter*>(m_pPin->m_pFilter))->GetOutputPin();
		if (!pOut || !pOut->CurrentAllocator()) {
			return hr;
		}

		ALLOCATOR_PROPERTIES Properties, Actual;
		if (FAILED(pOut->CurrentAllocator()->GetProperties(&Actual))) {
			return hr;
		}
		if (FAILED(GetProperties(&Properties))) {
			return hr;
		}

		if (!m_bCommitted || Properties.cbBuffer < Actual.cbBuffer) {
			Properties.cbBuffer = Actual.cbBuffer;
			if (FAILED(Decommit())) {
				return hr;
			}
			if (FAILED(SetProperties(&Properties, &Actual))) {
				return hr;
			}
			if (FAILED(Commit())) {
				return hr;
			}
			ASSERT(Actual.cbBuffer >= Properties.cbBuffer);
			if (Actual.cbBuffer < Properties.cbBuffer) {
				return hr;
			}
		}
	}

	hr = CMemAllocator::GetBuffer(ppBuffer, pStartTime, pEndTime, dwFlags);

	if (m_fMediaTypeChanged && SUCCEEDED(hr)) {
		(*ppBuffer)->SetMediaType(&m_mt);
		m_fMediaTypeChanged = false;
	}

	return hr;
}

void CStreamSwitcherAllocator::NotifyMediaType(const CMediaType& mt)
{
	CopyMediaType(&m_mt, &mt);
	m_fMediaTypeChanged = true;
}

//
// CStreamSwitcherInputPin
//

CStreamSwitcherInputPin::CStreamSwitcherInputPin(CStreamSwitcherFilter* pFilter, HRESULT* phr, LPCWSTR pName)
	: CBaseInputPin(NAME("CStreamSwitcherInputPin"), pFilter, &pFilter->m_csState, phr, pName)
	, m_Allocator(this, phr)
	, m_bSampleSkipped(FALSE)
	, m_bQualityChanged(FALSE)
	, m_bUsingOwnAllocator(FALSE)
	, m_hNotifyEvent(NULL)
	, m_bFlushing(FALSE)
	, m_pSSF(static_cast<CStreamSwitcherFilter*>(m_pFilter))
{
	m_bCanReconnectWhenActive = true;
}

class __declspec(uuid("138130AF-A79B-45D5-B4AA-87697457BA87"))
		NeroAudioDecoder {};

STDMETHODIMP CStreamSwitcherInputPin::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
	return
		QI(IStreamSwitcherInputPin)
		IsConnected() && GetCLSID(GetFilterFromPin(GetConnected())) == __uuidof(NeroAudioDecoder) && QI(IPinConnection)
		__super::NonDelegatingQueryInterface(riid, ppv);
}

// IPinConnection

STDMETHODIMP CStreamSwitcherInputPin::DynamicQueryAccept(const AM_MEDIA_TYPE* pmt)
{
	return QueryAccept(pmt);
}

STDMETHODIMP CStreamSwitcherInputPin::NotifyEndOfStream(HANDLE hNotifyEvent)
{
	if (m_hNotifyEvent) {
		SetEvent(m_hNotifyEvent);
	}
	m_hNotifyEvent = hNotifyEvent;
	return S_OK;
}

STDMETHODIMP CStreamSwitcherInputPin::IsEndPin()
{
	return S_OK;
}

STDMETHODIMP CStreamSwitcherInputPin::DynamicDisconnect()
{
	CAutoLock cAutoLock(&m_csReceive);
	Disconnect();
	return S_OK;
}

// IStreamSwitcherInputPin

STDMETHODIMP_(bool) CStreamSwitcherInputPin::IsActive()
{
	// TODO: lock onto something here
	return(this == m_pSSF->GetInputPin());
}

//

HRESULT CStreamSwitcherInputPin::QueryAcceptDownstream(const AM_MEDIA_TYPE* pmt)
{
	HRESULT hr = S_OK;

	CStreamSwitcherOutputPin* pOut = m_pSSF->GetOutputPin();

	if (pOut && pOut->IsConnected()) {
		if (CComPtr<IPinConnection> pPC = pOut->CurrentPinConnection()) {
			hr = pPC->DynamicQueryAccept(pmt);
			if (hr == S_OK) {
				return S_OK;
			}
		}

		hr = pOut->GetConnected()->QueryAccept(pmt);
	}

	return hr;
}

HRESULT CStreamSwitcherInputPin::InitializeOutputSample(IMediaSample* pInSample, IMediaSample** ppOutSample)
{
	CheckPointer(ppOutSample, E_POINTER);

	CStreamSwitcherOutputPin* pOut = m_pSSF->GetOutputPin();
	ASSERT(pOut->GetConnected());

	CComPtr<IMediaSample> pOutSample;

	DWORD dwFlags = m_bSampleSkipped ? AM_GBF_PREVFRAMESKIPPED : 0;

	if (!(m_SampleProps.dwSampleFlags & AM_SAMPLE_SPLICEPOINT)) {
		dwFlags |= AM_GBF_NOTASYNCPOINT;
	}

	HRESULT hr = pOut->GetDeliveryBuffer(&pOutSample
										 , m_SampleProps.dwSampleFlags & AM_SAMPLE_TIMEVALID ? &m_SampleProps.tStart : NULL
										 , m_SampleProps.dwSampleFlags & AM_SAMPLE_STOPVALID ? &m_SampleProps.tStop : NULL
										 , dwFlags);

	if (FAILED(hr)) {
		return hr;
	}

	if (!pOutSample) {
		return E_FAIL;
	}

	if (CComQIPtr<IMediaSample2> pOutSample2 = pOutSample) {
		AM_SAMPLE2_PROPERTIES OutProps;
		EXECUTE_ASSERT(SUCCEEDED(pOutSample2->GetProperties(FIELD_OFFSET(AM_SAMPLE2_PROPERTIES, tStart), (PBYTE)&OutProps)));
		OutProps.dwTypeSpecificFlags = m_SampleProps.dwTypeSpecificFlags;
		OutProps.dwSampleFlags =
			(OutProps.dwSampleFlags & AM_SAMPLE_TYPECHANGED) |
			(m_SampleProps.dwSampleFlags & ~AM_SAMPLE_TYPECHANGED);

		OutProps.tStart = m_SampleProps.tStart;
		OutProps.tStop  = m_SampleProps.tStop;
		OutProps.cbData = FIELD_OFFSET(AM_SAMPLE2_PROPERTIES, dwStreamId);

		hr = pOutSample2->SetProperties(FIELD_OFFSET(AM_SAMPLE2_PROPERTIES, dwStreamId), (PBYTE)&OutProps);
		if (m_SampleProps.dwSampleFlags & AM_SAMPLE_DATADISCONTINUITY) {
			m_bSampleSkipped = FALSE;
		}
	} else {
		if (m_SampleProps.dwSampleFlags & AM_SAMPLE_TIMEVALID) {
			pOutSample->SetTime(&m_SampleProps.tStart, &m_SampleProps.tStop);
		}

		if (m_SampleProps.dwSampleFlags & AM_SAMPLE_SPLICEPOINT) {
			pOutSample->SetSyncPoint(TRUE);
		}

		if (m_SampleProps.dwSampleFlags & AM_SAMPLE_DATADISCONTINUITY) {
			pOutSample->SetDiscontinuity(TRUE);
			m_bSampleSkipped = FALSE;
		}

		if (pInSample) {
			LONGLONG MediaStart, MediaEnd;
			if (pInSample->GetMediaTime(&MediaStart, &MediaEnd) == NOERROR) {
				pOutSample->SetMediaTime(&MediaStart, &MediaEnd);
			}
		}
	}

	*ppOutSample = pOutSample.Detach();

	return S_OK;
}

// pure virtual

HRESULT CStreamSwitcherInputPin::CheckMediaType(const CMediaType* pmt)
{
	return m_pSSF->CheckMediaType(pmt);
}

// virtual

HRESULT CStreamSwitcherInputPin::CheckConnect(IPin* pPin)
{
	return (IPin*)m_pSSF->GetOutputPin() == pPin
		   ? E_FAIL
		   : __super::CheckConnect(pPin);
}

HRESULT CStreamSwitcherInputPin::CompleteConnect(IPin* pReceivePin)
{
	HRESULT hr = __super::CompleteConnect(pReceivePin);
	if (FAILED(hr)) {
		return hr;
	}

	m_pSSF->CompleteConnect(PINDIR_INPUT, this, pReceivePin);

	CString fileName;
	CString pinName;
	CString trackName;

	IPin* pPin = (IPin*)this;
	IBaseFilter* pBF = (IBaseFilter*)m_pFilter;

	pPin = GetUpStreamPin(pBF, pPin);
	if (pPin) {
		pBF = GetFilterFromPin(pPin);
	}
	while (pPin && pBF) {
		if (IsSplitter(pBF)) {
			pinName = GetPinName(pPin);
		}

		DWORD cStreams = 0;
		if (CComQIPtr<IAMStreamSelect> pSSF = pBF) {
			hr = pSSF->Count(&cStreams);
			if (SUCCEEDED(hr)) {
				for (int i = 0; i < (int)cStreams; i++) {
					WCHAR* pszName = NULL;
					AM_MEDIA_TYPE* pmt = NULL;
					hr = pSSF->Info(i, &pmt, NULL, NULL, NULL, &pszName, NULL, NULL);
					if (SUCCEEDED(hr) && pmt && pmt->majortype == MEDIATYPE_Audio) {
						trackName = pszName;
						m_pISSF = pSSF;
						DeleteMediaType(pmt);
						if (pszName) {
							CoTaskMemFree(pszName);
						}
						break;
					}
					DeleteMediaType(pmt);
					if (pszName) {
						CoTaskMemFree(pszName);
					}
				}
			}
		}

		if (CComQIPtr<IFileSourceFilter> pFSF = pBF) {
			WCHAR* pszName = NULL;
			AM_MEDIA_TYPE mt;
			if (SUCCEEDED(pFSF->GetCurFile(&pszName, &mt)) && pszName) {
				fileName = pszName;
				CoTaskMemFree(pszName);

				if (::PathIsURL(fileName)) {
					pinName = GetPinName(pPin);
					fileName = !trackName.IsEmpty() ? trackName : !pinName.IsEmpty() ? pinName : L"Audio";
				} else {
					if (!pinName.IsEmpty()) {
						fileName = pinName;
					} else {
						fileName.Replace('\\', '/');
						CString fn = fileName.Mid(fileName.ReverseFind('/') + 1);
						if (!fn.IsEmpty()) {
							fileName = fn;
						}
					}
				}

				WCHAR* pName = DNew WCHAR[fileName.GetLength() + 1];
				if (pName) {
					wcscpy_s(pName, fileName.GetLength() + 1, fileName);
					SAFE_DELETE_ARRAY(m_pName);
					m_pName = pName;
					if (cStreams == 1) { // Simple external track, no need to use the info from IAMStreamSelect
						m_pISSF.Release();
					}
				}
			}

			break;
		}

		pPin = GetFirstPin(pBF);

		pPin = GetUpStreamPin(pBF, pPin);
		if (pPin) {
			pBF = GetFilterFromPin(pPin);
		}
	}

	m_hNotifyEvent = NULL;

	return S_OK;
}

// IPin

STDMETHODIMP CStreamSwitcherInputPin::QueryAccept(const AM_MEDIA_TYPE* pmt)
{
	HRESULT hr = __super::QueryAccept(pmt);
	if (S_OK != hr) {
		return hr;
	}

	return QueryAcceptDownstream(pmt);
}

STDMETHODIMP CStreamSwitcherInputPin::ReceiveConnection(IPin* pConnector, const AM_MEDIA_TYPE* pmt)
{
	// FIXME: this locked up once
	// CAutoLock cAutoLock(&((CStreamSwitcherFilter*)m_pFilter)->m_csReceive);

	HRESULT hr;
	if (S_OK != (hr = QueryAcceptDownstream(pmt))) {
		return VFW_E_TYPE_NOT_ACCEPTED;
	}

	if (m_Connected && m_Connected != pConnector) {
		return VFW_E_ALREADY_CONNECTED;
	}

	if (m_Connected) {
		m_Connected->Release(), m_Connected = NULL;
	}

	return SUCCEEDED(__super::ReceiveConnection(pConnector, pmt)) ? S_OK : E_FAIL;
}

STDMETHODIMP CStreamSwitcherInputPin::GetAllocator(IMemAllocator** ppAllocator)
{
	CheckPointer(ppAllocator, E_POINTER);

	if (m_pAllocator == NULL) {
		(m_pAllocator = &m_Allocator)->AddRef();
	}

	(*ppAllocator = m_pAllocator)->AddRef();

	return NOERROR;
}

STDMETHODIMP CStreamSwitcherInputPin::NotifyAllocator(IMemAllocator* pAllocator, BOOL bReadOnly)
{
	HRESULT hr = __super::NotifyAllocator(pAllocator, bReadOnly);
	if (FAILED(hr)) {
		return hr;
	}

	m_bUsingOwnAllocator = (pAllocator == (IMemAllocator*)&m_Allocator);

	return S_OK;
}

STDMETHODIMP CStreamSwitcherInputPin::BeginFlush()
{
	CAutoLock cAutoLock(&m_pSSF->m_csState);

	CStreamSwitcherOutputPin* pOut = m_pSSF->GetOutputPin();
	if (!IsConnected() || !pOut || !pOut->IsConnected()) {
		return VFW_E_NOT_CONNECTED;
	}

	m_bFlushing = TRUE;

	HRESULT hr;
	if (FAILED(hr = __super::BeginFlush())) {
		return hr;
	}

	return IsActive() ? m_pSSF->DeliverBeginFlush() : S_OK;
}

STDMETHODIMP CStreamSwitcherInputPin::EndFlush()
{
	CAutoLock cAutoLock(&m_pSSF->m_csState);

	CStreamSwitcherOutputPin* pOut = m_pSSF->GetOutputPin();
	if (!IsConnected() || !pOut || !pOut->IsConnected()) {
		return VFW_E_NOT_CONNECTED;
	}

	HRESULT hr;
	if (FAILED(hr = __super::EndFlush())) {
		return hr;
	}

	hr = IsActive() ? m_pSSF->DeliverEndFlush() : S_OK;
	m_bFlushing = FALSE;

	return hr;
}

STDMETHODIMP CStreamSwitcherInputPin::EndOfStream()
{
	CAutoLock cAutoLock(&m_csReceive);

	CStreamSwitcherOutputPin* pOut = m_pSSF->GetOutputPin();
	if (!IsConnected() || !pOut || !pOut->IsConnected()) {
		return VFW_E_NOT_CONNECTED;
	}

	if (m_hNotifyEvent) {
		SetEvent(m_hNotifyEvent), m_hNotifyEvent = NULL;
		return S_OK;
	}

	return IsActive() ? m_pSSF->DeliverEndOfStream() : S_OK;
}

// IMemInputPin
STDMETHODIMP CStreamSwitcherInputPin::Receive(IMediaSample* pSample)
{
	if (!IsActive() || m_bFlushing) {
		return S_FALSE;
	}

	bool bFormatChanged = m_pSSF->m_bInputPinChanged || m_pSSF->m_bOutputFormatChanged;

	AM_MEDIA_TYPE* pmt = NULL;
	if (SUCCEEDED(pSample->GetMediaType(&pmt)) && pmt) {
		const CMediaType mt(*pmt);
		DeleteMediaType(pmt), pmt = NULL;
		bFormatChanged |= !!(mt != m_mt);
		SetMediaType(&mt);
	}

	CAutoLock cAutoLock(&m_csReceive);
	std::unique_lock<std::mutex> lock(m_pSSF->m_inputpin_receive_mutex);

	CStreamSwitcherOutputPin* pOut = m_pSSF->GetOutputPin();
	ASSERT(pOut->GetConnected());

	HRESULT hr = __super::Receive(pSample);
	if (S_OK != hr) {
		return hr;
	}

	if (m_SampleProps.dwStreamId != AM_STREAM_MEDIA) {
		return pOut->Deliver(pSample);
	}

	ALLOCATOR_PROPERTIES props, actual;
	hr = m_pAllocator->GetProperties(&props);
	hr = pOut->CurrentAllocator()->GetProperties(&actual);

	long cbBuffer = pSample->GetActualDataLength();

	if (bFormatChanged) {
		pOut->SetMediaType(&m_mt);

		CMediaType& out_mt = pOut->CurrentMediaType();
		if (*out_mt.FormatType() == FORMAT_WaveFormatEx) {
			WAVEFORMATEX* wfe = (WAVEFORMATEX*)m_mt.Format();
			WAVEFORMATEX* out_wfe = (WAVEFORMATEX*)out_mt.Format();

			cbBuffer *= out_wfe->nChannels * out_wfe->wBitsPerSample;
			cbBuffer /= wfe->nChannels * wfe->wBitsPerSample;
		}

		m_pSSF->m_bInputPinChanged = false;
		m_pSSF->m_bOutputFormatChanged = false;
	}


	if (bFormatChanged || cbBuffer > actual.cbBuffer) {
		DLog(L"CStreamSwitcherInputPin::Receive(): %s", bFormatChanged ? L"input or output media type changed" : L"cbBuffer > actual.cbBuffer");

		m_SampleProps.dwSampleFlags |= AM_SAMPLE_TYPECHANGED/* | AM_SAMPLE_DATADISCONTINUITY | AM_SAMPLE_TIMEDISCONTINUITY*/;

		/*
		if (CComQIPtr<IPinConnection> pPC = pOut->CurrentPinConnection()) {
			HANDLE hEOS = CreateEvent(NULL, FALSE, FALSE, NULL);
			hr = pPC->NotifyEndOfStream(hEOS);
			hr = pOut->DeliverEndOfStream();
			WaitForSingleObject(hEOS, 3000);
			CloseHandle(hEOS);
			hr = pOut->DeliverBeginFlush();
			hr = pOut->DeliverEndFlush();
		}
		*/

		if (props.cBuffers < 8 && m_mt.majortype == MEDIATYPE_Audio) {
			props.cBuffers = 8;
		}

		props.cbBuffer = cbBuffer * 3 / 2;

		if (actual.cbAlign != props.cbAlign
				|| actual.cbPrefix != props.cbPrefix
				|| actual.cBuffers < props.cBuffers
				|| actual.cbBuffer < props.cbBuffer) {
			hr = pOut->DeliverBeginFlush();
			hr = pOut->DeliverEndFlush();
			hr = pOut->CurrentAllocator()->Decommit();
			hr = pOut->CurrentAllocator()->SetProperties(&props, &actual);
			hr = pOut->CurrentAllocator()->Commit();
		}
	}

	CComPtr<IMediaSample> pOutSample;
	if (FAILED(InitializeOutputSample(pSample, &pOutSample))) {
		return E_FAIL;
	}

	pmt = NULL;
	if (bFormatChanged || S_OK == pOutSample->GetMediaType(&pmt)) {
		pOutSample->SetMediaType(&pOut->CurrentMediaType());
		DLog(L"CStreamSwitcherInputPin::Receive(): output media type changed");
	}

	// Transform
	hr = m_pSSF->Transform(pSample, pOutSample);
	//

	if (S_OK == hr) {
		hr = pOut->Deliver(pOutSample);
		m_bSampleSkipped = FALSE;
		/*
		if (FAILED(hr)) {
			ASSERT(0);
		}
		*/
	} else if (S_FALSE == hr) {
		hr = S_OK;
		pOutSample = NULL;
		m_bSampleSkipped = TRUE;

		if (!m_bQualityChanged) {
			m_pFilter->NotifyEvent(EC_QUALITY_CHANGE, 0, 0);
			m_bQualityChanged = TRUE;
		}
	}

	return hr;
}

STDMETHODIMP CStreamSwitcherInputPin::NewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate)
{
	if (!IsConnected()) {
		return S_OK;
	}

	CAutoLock cAutoLock(&m_csReceive);

	CStreamSwitcherOutputPin* pOut = m_pSSF->GetOutputPin();
	if (!pOut || !pOut->IsConnected()) {
		return VFW_E_NOT_CONNECTED;
	}

	HRESULT hr = m_pSSF->DeliverNewSegment(tStart, tStop, dRate);
	/*
	if (hr == S_OK) {
		const CLSID clsid = GetCLSID(pOut->GetConnected());
		if (clsid != CLSID_ReClock) {
			// hack - create and send an "empty" packet to make sure that the playback started is 100%
			const WAVEFORMATEX* out_wfe = (WAVEFORMATEX*)pOut->CurrentMediaType().pbFormat;
			const SampleFormat out_sampleformat = GetSampleFormat(out_wfe);
			if (out_sampleformat != SAMPLE_FMT_NONE) {
				CComPtr<IMediaSample> pOutSample;
				if (SUCCEEDED(InitializeOutputSample(NULL, &pOutSample))
						&& SUCCEEDED(pOutSample->SetActualDataLength(0))) {
					REFERENCE_TIME rtStart = 0, rtStop = 0;
					pOutSample->SetTime(&rtStart, &rtStop);

					pOut->Deliver(pOutSample);
				}
			}
		}
	}
	*/

	return hr;
}

//
// CStreamSwitcherOutputPin
//

CStreamSwitcherOutputPin::CStreamSwitcherOutputPin(CStreamSwitcherFilter* pFilter, HRESULT* phr)
	: CBaseOutputPin(NAME("CStreamSwitcherOutputPin"), pFilter, &pFilter->m_csState, phr, L"Out")
{
	//	m_bCanReconnectWhenActive = true;
}

STDMETHODIMP CStreamSwitcherOutputPin::NonDelegatingQueryInterface(REFIID riid, void **ppv)
{
	CheckPointer(ppv,E_POINTER);
	ValidateReadWritePtr(ppv, sizeof(PVOID));
	*ppv = NULL;

	if (riid == IID_IMediaPosition || riid == IID_IMediaSeeking) {
		if (m_pStreamSwitcherPassThru == NULL) {
			HRESULT hr = S_OK;
			m_pStreamSwitcherPassThru = (IUnknown*)(INonDelegatingUnknown*)
										DNew CStreamSwitcherPassThru(GetOwner(), &hr, static_cast<CStreamSwitcherFilter*>(m_pFilter));

			if (!m_pStreamSwitcherPassThru) {
				return E_OUTOFMEMORY;
			}
			if (FAILED(hr)) {
				return hr;
			}
		}

		return m_pStreamSwitcherPassThru->QueryInterface(riid, ppv);
	}
	/*
		else if(riid == IID_IStreamBuilder)
		{
			return GetInterface((IStreamBuilder*)this, ppv);
		}
	*/
	return CBaseOutputPin::NonDelegatingQueryInterface(riid, ppv);
}

HRESULT CStreamSwitcherOutputPin::QueryAcceptUpstream(const AM_MEDIA_TYPE* pmt)
{
	HRESULT hr = S_FALSE;

	CStreamSwitcherInputPin* pIn = (static_cast<CStreamSwitcherFilter*>(m_pFilter))->GetInputPin();

	if (pIn && pIn->IsConnected() && (pIn->IsUsingOwnAllocator() || pIn->CurrentMediaType() == *pmt)) {
		if (CComQIPtr<IPin> pPinTo = pIn->GetConnected()) {
			if (S_OK != (hr = pPinTo->QueryAccept(pmt))) {
				return VFW_E_TYPE_NOT_ACCEPTED;
			}
		} else {
			return E_FAIL;
		}
	}

	return hr;
}

// pure virtual

HRESULT CStreamSwitcherOutputPin::DecideBufferSize(IMemAllocator* pAllocator, ALLOCATOR_PROPERTIES* pProperties)
{
	CStreamSwitcherInputPin* pIn = (static_cast<CStreamSwitcherFilter*>(m_pFilter))->GetInputPin();
	if (!pIn || !pIn->IsConnected()) {
		return E_UNEXPECTED;
	}

	CComPtr<IMemAllocator> pAllocatorIn;
	pIn->GetAllocator(&pAllocatorIn);
	if (!pAllocatorIn) {
		return E_UNEXPECTED;
	}

	HRESULT hr;
	if (FAILED(hr = pAllocatorIn->GetProperties(pProperties))) {
		return hr;
	}

	if (pProperties->cBuffers < 8 && pIn->CurrentMediaType().majortype == MEDIATYPE_Audio) {
		pProperties->cBuffers = 8;
	}

	if (*m_mt.FormatType() == FORMAT_WaveFormatEx) {
		long cbBuffer = pProperties->cbBuffer;
		WAVEFORMATEX* wfe = (WAVEFORMATEX*)m_mt.Format();
		WAVEFORMATEX* in_wfe = (WAVEFORMATEX*)pIn->CurrentMediaType().Format();

		cbBuffer *= wfe->nChannels * wfe->wBitsPerSample;
		cbBuffer /= in_wfe->nChannels * in_wfe->wBitsPerSample;

		if (cbBuffer > pProperties->cbBuffer) {
			pProperties->cbBuffer = cbBuffer;
		}
	}

	ALLOCATOR_PROPERTIES Actual;
	if (FAILED(hr = pAllocator->SetProperties(pProperties, &Actual))) {
		return hr;
	}

	return(pProperties->cBuffers > Actual.cBuffers || pProperties->cbBuffer > Actual.cbBuffer
		   ? E_FAIL
		   : NOERROR);
}

// virtual

HRESULT CStreamSwitcherOutputPin::CheckConnect(IPin* pPin)
{
	CComPtr<IBaseFilter> pBF = GetFilterFromPin(pPin);
	CLSID clsid = GetCLSID(pBF);

	return
		IsAudioWaveRenderer(pBF)
			|| clsid == CLSID_InfTee
			|| clsid == CLSID_XySubFilter_AutoLoader
			|| clsid == GUIDFromCString(L"{B86F6BEE-E7C0-4D03-8D52-5B4430CF6C88}") // ffdshow Audio Processor
			|| clsid == GUIDFromCString(L"{A753A1EC-973E-4718-AF8E-A3F554D45C44}") // AC3Filter
			|| clsid == GUIDFromCString(L"{B38C58A0-1809-11D6-A458-EDAE78F1DF12}") // DC-DSP Filter
			|| clsid == GUIDFromCString(L"{36F74DF0-12FF-4881-8A55-E7CE4D12688E}") // CyberLink TimeStretch Filter (PDVD10)
			//|| clsid == GUIDFromCString(L"{AEFA5024-215A-4FC7-97A4-1043C86FD0B8}") // MatrixMixer unstable when changing format
		? __super::CheckConnect(pPin)
		: E_FAIL;

	//	return CComQIPtr<IPinConnection>(pPin) ? CBaseOutputPin::CheckConnect(pPin) : E_NOINTERFACE;
	//	return CBaseOutputPin::CheckConnect(pPin);
}

HRESULT CStreamSwitcherOutputPin::BreakConnect()
{
	m_pPinConnection = NULL;
	return __super::BreakConnect();
}

HRESULT CStreamSwitcherOutputPin::CompleteConnect(IPin* pReceivePin)
{
	m_pPinConnection = CComQIPtr<IPinConnection>(pReceivePin);
	HRESULT hr = __super::CompleteConnect(pReceivePin);

	CStreamSwitcherInputPin* pIn = (static_cast<CStreamSwitcherFilter*>(m_pFilter))->GetInputPin();
	CMediaType mt;
	if (SUCCEEDED(hr) && pIn && pIn->IsConnected()
			&& SUCCEEDED(pIn->GetConnected()->ConnectionMediaType(&mt))) {
		(static_cast<CStreamSwitcherFilter*>(m_pFilter))->TransformMediaType(mt);
		if (m_mt != mt) {
			if (pIn->GetConnected()->QueryAccept(&m_mt) == S_OK) {
				hr = m_pFilter->ReconnectPin(pIn->GetConnected(), &m_mt);
			} else {
				hr = VFW_E_TYPE_NOT_ACCEPTED;
			}
		}
	}

	return hr;
}

HRESULT CStreamSwitcherOutputPin::SetMediaType(const CMediaType *pmt)
{
	HRESULT hr = m_mt.Set(*pmt);
	if (FAILED(hr)) {
		return hr;
	}

	(static_cast<CStreamSwitcherFilter*>(m_pFilter))->TransformMediaType(m_mt);

	return NOERROR;
}

HRESULT CStreamSwitcherOutputPin::CheckMediaType(const CMediaType* pmt)
{
	return (static_cast<CStreamSwitcherFilter*>(m_pFilter))->CheckMediaType(pmt);
}

HRESULT CStreamSwitcherOutputPin::GetMediaType(int iPosition, CMediaType* pmt)
{
	CStreamSwitcherInputPin* pIn = (static_cast<CStreamSwitcherFilter*>(m_pFilter))->GetInputPin();
	if (!pIn || !pIn->IsConnected()) {
		return E_UNEXPECTED;
	}

	CComPtr<IEnumMediaTypes> pEM;
	if (FAILED(pIn->GetConnected()->EnumMediaTypes(&pEM))) {
		return VFW_S_NO_MORE_ITEMS;
	}

	if (iPosition > 0 && FAILED(pEM->Skip(iPosition))) {
		return VFW_S_NO_MORE_ITEMS;
	}

	AM_MEDIA_TYPE* tmp = NULL;
	if (S_OK != pEM->Next(1, &tmp, NULL) || !tmp) {
		return VFW_S_NO_MORE_ITEMS;
	}

	CopyMediaType(pmt, tmp);
	DeleteMediaType(tmp);

	(static_cast<CStreamSwitcherFilter*>(m_pFilter))->TransformMediaType(*pmt);

	/*
		if(iPosition < 0) return E_INVALIDARG;
		if(iPosition > 0) return VFW_S_NO_MORE_ITEMS;

		CopyMediaType(pmt, &pIn->CurrentMediaType());
	*/
	return S_OK;
}

// IPin

STDMETHODIMP CStreamSwitcherOutputPin::QueryAccept(const AM_MEDIA_TYPE* pmt)
{
	HRESULT hr = __super::QueryAccept(pmt);
	if (S_OK != hr) {
		return hr;
	}

	return QueryAcceptUpstream(pmt);
}

// IQualityControl

STDMETHODIMP CStreamSwitcherOutputPin::Notify(IBaseFilter* pSender, Quality q)
{
	CStreamSwitcherInputPin* pIn = (static_cast<CStreamSwitcherFilter*>(m_pFilter))->GetInputPin();
	if (!pIn || !pIn->IsConnected()) {
		return VFW_E_NOT_CONNECTED;
	}
	return pIn->PassNotify(q);
}

// IStreamBuilder

STDMETHODIMP CStreamSwitcherOutputPin::Render(IPin* ppinOut, IGraphBuilder* pGraph)
{
	CComPtr<IBaseFilter> pBF;
	pBF.CoCreateInstance(CLSID_DSoundRender);
	if (!pBF || FAILED(pGraph->AddFilter(pBF, L"Default DirectSound Device"))) {
		return E_FAIL;
	}

	if (FAILED(pGraph->ConnectDirect(ppinOut, GetFirstDisconnectedPin(pBF, PINDIR_INPUT), NULL))) {
		pGraph->RemoveFilter(pBF);
		return E_FAIL;
	}

	return S_OK;
}

STDMETHODIMP CStreamSwitcherOutputPin::Backout(IPin* ppinOut, IGraphBuilder* pGraph)
{
	return S_OK;
}

//
// CStreamSwitcherFilter
//

CStreamSwitcherFilter::CStreamSwitcherFilter(LPUNKNOWN lpunk, HRESULT* phr, const CLSID& clsid)
	: CBaseFilter(NAME("CStreamSwitcherFilter"), lpunk, &m_csState, clsid)
	, m_bInputPinChanged(false)
	, m_bOutputFormatChanged(false)
{
	if (phr) {
		*phr = S_OK;
	}

	HRESULT hr = S_OK;

	do {
		CAutoPtr<CStreamSwitcherInputPin> pInput;
		CAutoPtr<CStreamSwitcherOutputPin> pOutput;

		hr = S_OK;
		pInput.Attach(DNew CStreamSwitcherInputPin(this, &hr, L"Channel 1"));
		if (!pInput || FAILED(hr)) {
			break;
		}

		hr = S_OK;
		pOutput.Attach(DNew CStreamSwitcherOutputPin(this, &hr));
		if (!pOutput || FAILED(hr)) {
			break;
		}

		CAutoLock cAutoLock(&m_csPins);

		m_pInputs.AddHead(m_pInput = pInput.Detach());
		m_pOutput = pOutput.Detach();

		return;
	} while (false);

	if (phr) {
		*phr = E_FAIL;
	}
}

CStreamSwitcherFilter::~CStreamSwitcherFilter()
{
	CAutoLock cAutoLock(&m_csPins);

	POSITION pos = m_pInputs.GetHeadPosition();
	while (pos) {
		delete m_pInputs.GetNext(pos);
	}
	m_pInputs.RemoveAll();
	m_pInput = NULL;

	delete m_pOutput;
	m_pOutput = NULL;
}

STDMETHODIMP CStreamSwitcherFilter::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
	return
		QI(IAMStreamSelect)
		__super::NonDelegatingQueryInterface(riid, ppv);
}

//

int CStreamSwitcherFilter::GetPinCount()
{
	CAutoLock cAutoLock(&m_csPins);

	return(1 + (int)m_pInputs.GetCount());
}

CBasePin* CStreamSwitcherFilter::GetPin(int n)
{
	CAutoLock cAutoLock(&m_csPins);

	if (n < 0 || n >= GetPinCount()) {
		return NULL;
	} else if (n == 0) {
		return m_pOutput;
	} else {
		return m_pInputs.GetAt(m_pInputs.FindIndex(n-1));
	}
}

int CStreamSwitcherFilter::GetConnectedInputPinCount()
{
	CAutoLock cAutoLock(&m_csPins);

	int nConnected = 0;

	POSITION pos = m_pInputs.GetHeadPosition();
	while (pos) {
		if (m_pInputs.GetNext(pos)->IsConnected()) {
			nConnected++;
		}
	}

	return(nConnected);
}

CStreamSwitcherInputPin* CStreamSwitcherFilter::GetConnectedInputPin(int n)
{
	if (n >= 0) {
		POSITION pos = m_pInputs.GetHeadPosition();
		while (pos) {
			CStreamSwitcherInputPin* pPin = m_pInputs.GetNext(pos);
			if (pPin->IsConnected()) {
				if (n == 0) {
					return pPin;
				}
				n--;
			}
		}
	}

	return NULL;
}

CStreamSwitcherInputPin* CStreamSwitcherFilter::GetInputPin()
{
	return m_pInput;
}

CStreamSwitcherOutputPin* CStreamSwitcherFilter::GetOutputPin()
{
	return m_pOutput;
}

//

HRESULT CStreamSwitcherFilter::CompleteConnect(PIN_DIRECTION dir, CBasePin* pPin, IPin* pReceivePin)
{
	if (dir == PINDIR_INPUT) {
		CAutoLock cAutoLock(&m_csPins);

		int nConnected = GetConnectedInputPinCount();

		if (nConnected == 1) {
			m_pInput = static_cast<CStreamSwitcherInputPin*>(pPin);
		}

		if ((size_t)nConnected == m_pInputs.GetCount()) {
			CStringW name;
			name.Format(L"Channel %d", ++m_PinVersion);

			HRESULT hr = S_OK;
			CStreamSwitcherInputPin* pInputPin = DNew CStreamSwitcherInputPin(this, &hr, name);
			if (!pInputPin || FAILED(hr)) {
				delete pInputPin;
				return E_FAIL;
			}
			m_pInputs.AddTail(pInputPin);
		}
	}

	return S_OK;
}

// this should be very thread safe, I hope it is, it must be... :)

void CStreamSwitcherFilter::SelectInput(CStreamSwitcherInputPin* pInput)
{
	// make sure no input thinks it is active
	m_pInput = NULL;

	// this will let waiting GetBuffer() calls go on inside our Receive()
	if (m_pOutput) {
		m_pOutput->DeliverBeginFlush();
		m_pOutput->DeliverEndFlush();
	}

	if (!pInput) {
		return;
	}

	// set new input
	m_pInput = pInput;
	m_bInputPinChanged = true;
}

//

HRESULT CStreamSwitcherFilter::Transform(IMediaSample* pIn, IMediaSample* pOut)
{
	BYTE* pDataIn = NULL;
	BYTE* pDataOut = NULL;

	HRESULT hr;
	if (FAILED(hr = pIn->GetPointer(&pDataIn))) {
		return hr;
	}
	if (FAILED(hr = pOut->GetPointer(&pDataOut))) {
		return hr;
	}

	long len = pIn->GetActualDataLength();
	long size = pOut->GetSize();

	if (!pDataIn || !pDataOut /*|| len > size || len <= 0*/) {
		return S_FALSE; // FIXME
	}

	memcpy(pDataOut, pDataIn, min(len, size));
	pOut->SetActualDataLength(min(len, size));

	return S_OK;
}

HRESULT CStreamSwitcherFilter::DeliverEndOfStream()
{
	return m_pOutput ? m_pOutput->DeliverEndOfStream() : E_FAIL;
}

HRESULT CStreamSwitcherFilter::DeliverBeginFlush()
{
	return m_pOutput ? m_pOutput->DeliverBeginFlush() : E_FAIL;
}

HRESULT CStreamSwitcherFilter::DeliverEndFlush()
{
	return m_pOutput ? m_pOutput->DeliverEndFlush() : E_FAIL;
}

HRESULT CStreamSwitcherFilter::DeliverNewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate)
{
	return m_pOutput ? m_pOutput->DeliverNewSegment(tStart, tStop, dRate) : E_FAIL;
}

// IAMStreamSelect

STDMETHODIMP CStreamSwitcherFilter::Count(DWORD* pcStreams)
{
	CheckPointer(pcStreams, E_POINTER);

	CAutoLock cAutoLock(&m_csPins);

	*pcStreams = 0;
	POSITION pos = m_pInputs.GetHeadPosition();
	while (pos) {
		CStreamSwitcherInputPin* pInputPin = m_pInputs.GetNext(pos);

		if (pInputPin->IsConnected()) {
			if (CComPtr<IAMStreamSelect> pSSF = pInputPin->GetStreamSelectionFilter()) {
				DWORD cStreams = 0;
				HRESULT hr = pSSF->Count(&cStreams);
				if (SUCCEEDED(hr)) {
					for (int i = 0; i < (int)cStreams; i++) {
						AM_MEDIA_TYPE* pmt = NULL;
						hr = pSSF->Info(i, &pmt, NULL, NULL, NULL, NULL, NULL, NULL);
						if (SUCCEEDED(hr) && pmt && pmt->majortype == MEDIATYPE_Audio) {
							(*pcStreams)++;
						}
						DeleteMediaType(pmt);
					}
				}
			} else {
				(*pcStreams)++;
			}
		}
	}

	return S_OK;
}

STDMETHODIMP CStreamSwitcherFilter::Info(long lIndex, AM_MEDIA_TYPE** ppmt, DWORD* pdwFlags, LCID* plcid, DWORD* pdwGroup, WCHAR** ppszName, IUnknown** ppObject, IUnknown** ppUnk)
{
	CAutoLock cAutoLock(&m_csPins);

	bool bFound = false;
	POSITION pos = m_pInputs.GetHeadPosition();
	while (pos && !bFound) {
		CStreamSwitcherInputPin* pInputPin = m_pInputs.GetNext(pos);

		if (pInputPin->IsConnected()) {
			if (CComPtr<IAMStreamSelect> pSSF = pInputPin->GetStreamSelectionFilter()) {
				DWORD cStreams = 0;
				HRESULT hr = pSSF->Count(&cStreams);
				if (SUCCEEDED(hr)) {
					for (int i = 0; i < (int)cStreams; i++) {
						AM_MEDIA_TYPE* pmt = NULL;
						DWORD dwFlags;
						LPWSTR pszName = NULL;
						hr = pSSF->Info(i, &pmt, &dwFlags, plcid, NULL, &pszName, NULL, NULL);
						if (SUCCEEDED(hr) && pmt && pmt->majortype == MEDIATYPE_Audio) {
							if (lIndex == 0) {
								bFound = true;

								if (ppmt) {
									*ppmt = pmt;
								} else {
									DeleteMediaType(pmt);
								}

								if (pdwFlags) {
									*pdwFlags = (m_pInput == pInputPin) ? dwFlags : 0;
								}

								if (pdwGroup) {
									*pdwGroup = 0;
								}

								if (ppszName) {
									*ppszName = pszName;
								} else {
									CoTaskMemFree(pszName);
								}

								break;
							} else {
								lIndex--;
							}
						}
						DeleteMediaType(pmt);
						CoTaskMemFree(pszName);
					}
				}
			} else if (lIndex == 0) {
				bFound = true;

				if (ppmt) {
					*ppmt = CreateMediaType(&m_pOutput->CurrentMediaType());
				}

				if (pdwFlags) {
					*pdwFlags = (m_pInput == pInputPin) ? AMSTREAMSELECTINFO_EXCLUSIVE : 0;
				}

				if (plcid) {
					*plcid = 0;
				}

				if (pdwGroup) {
					*pdwGroup = 0;
				}

				if (ppszName) {
					*ppszName = (WCHAR*)CoTaskMemAlloc((wcslen(pInputPin->Name()) + 1) * sizeof(WCHAR));
					if (*ppszName) {
						wcscpy_s(*ppszName, wcslen(pInputPin->Name()) + 1, pInputPin->Name());
					}
				}
			} else {
				lIndex--;
			}
		}
	}

	if (!bFound) {
		return E_INVALIDARG;
	}

	if (ppUnk) {
		*ppUnk = NULL;
	}

	return S_OK;
}

STDMETHODIMP CStreamSwitcherFilter::Enable(long lIndex, DWORD dwFlags)
{
	if (dwFlags != AMSTREAMSELECTENABLE_ENABLE) {
		return E_NOTIMPL;
	}

	PauseGraph;

	bool bFound = false;
	int i = 0;
	CStreamSwitcherInputPin* pNewInputPin = NULL;
	POSITION pos = m_pInputs.GetHeadPosition();
	while (pos && !bFound) {
		pNewInputPin = m_pInputs.GetNext(pos);

		if (pNewInputPin->IsConnected()) {
			if (CComPtr<IAMStreamSelect> pSSF = pNewInputPin->GetStreamSelectionFilter()) {
				DWORD cStreams = 0;
				HRESULT hr = pSSF->Count(&cStreams);
				if (SUCCEEDED(hr)) {
					for (i = 0; i < (int)cStreams; i++) {
						AM_MEDIA_TYPE* pmt = NULL;
						hr = pSSF->Info(i, &pmt, NULL, NULL, NULL, NULL, NULL, NULL);
						if (SUCCEEDED(hr) && pmt && pmt->majortype == MEDIATYPE_Audio) {
							if (lIndex == 0) {
								bFound = true;
								DeleteMediaType(pmt);
								break;
							} else {
								lIndex--;
							}
						}
						DeleteMediaType(pmt);
					}
				}
			} else if (lIndex == 0) {
				bFound = true;
			} else {
				lIndex--;
			}
		}
	}

	if (!bFound) {
		return E_INVALIDARG;
	}

	SelectInput(pNewInputPin);

	if (CComPtr<IAMStreamSelect> pSSF = pNewInputPin->GetStreamSelectionFilter()) {
		pSSF->Enable(i, dwFlags);
	}

	ResumeGraph;

	return S_OK;
}
