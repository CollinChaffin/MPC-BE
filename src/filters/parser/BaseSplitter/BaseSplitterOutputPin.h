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

#pragma once

#include <atlbase.h>
#include <IKeyFrameInfo.h>
#include <IBufferInfo.h>
#include <IBitRateInfo.h>
#include "../../../DSUtil/Packet.h"
#include "../../../DSUtil/DSMPropertyBag.h"

#define DEFAULT_BUFFER_DURATION 3000

class CBaseSplitterFilter;

class CBaseSplitterOutputPin
	: public CBaseOutputPin
	, public IDSMPropertyBagImpl
	, protected CAMThread
	, public IMediaSeeking
	, public IBitRateInfo
{
private:
	bool			m_fFlushing			= false;
	bool			m_fFlushed			= false;
	CAMEvent		m_eEndFlush			= TRUE;

	REFERENCE_TIME	m_maxQueueDuration	= DEFAULT_BUFFER_DURATION * 10000;
	size_t			m_maxQueueCount		= DEFAULT_BUFFER_DURATION * 12 / 10;

	REFERENCE_TIME	m_rtPrev			= 0;
	REFERENCE_TIME	m_rtOffset			= 0;

	enum {
		CMD_EXIT
	};
	DWORD ThreadProc();

	void MakeISCRHappy();

	// please only use DeliverPacket from the derived class
	HRESULT GetDeliveryBuffer(IMediaSample** ppSample, REFERENCE_TIME* pStartTime, REFERENCE_TIME* pEndTime, DWORD dwFlags);
	HRESULT Deliver(IMediaSample* pSample);

	// bitrate stats

	struct {
		UINT64 nTotalBytesDelivered;
		REFERENCE_TIME rtTotalTimeDelivered;
		UINT64 nBytesSinceLastDeliverTime;
		REFERENCE_TIME rtLastDeliverTime;
		DWORD nCurrentBitRate;
		DWORD nAverageBitRate;
	} m_brs;

protected:
	CBaseSplitterFilter* pSplitter;
	std::vector<CMediaType> m_mts;
	CPacketQueue m_queue;

	HRESULT			m_hrDeliver	= S_OK;
	REFERENCE_TIME	m_rtStart	= 0;

	// override this if you need some second level stream specific demuxing (optional)
	// the default implementation will send the sample as is
	virtual HRESULT DeliverPacket(CAutoPtr<CPacket> p);

	// IMediaSeeking

	STDMETHODIMP GetCapabilities(DWORD* pCapabilities);
	STDMETHODIMP CheckCapabilities(DWORD* pCapabilities);
	STDMETHODIMP IsFormatSupported(const GUID* pFormat);
	STDMETHODIMP QueryPreferredFormat(GUID* pFormat);
	STDMETHODIMP GetTimeFormat(GUID* pFormat);
	STDMETHODIMP IsUsingTimeFormat(const GUID* pFormat);
	STDMETHODIMP SetTimeFormat(const GUID* pFormat);
	STDMETHODIMP GetDuration(LONGLONG* pDuration);
	STDMETHODIMP GetStopPosition(LONGLONG* pStop);
	STDMETHODIMP GetCurrentPosition(LONGLONG* pCurrent);
	STDMETHODIMP ConvertTimeFormat(LONGLONG* pTarget, const GUID* pTargetFormat, LONGLONG Source, const GUID* pSourceFormat);
	STDMETHODIMP SetPositions(LONGLONG* pCurrent, DWORD dwCurrentFlags, LONGLONG* pStop, DWORD dwStopFlags);
	STDMETHODIMP GetPositions(LONGLONG* pCurrent, LONGLONG* pStop);
	STDMETHODIMP GetAvailable(LONGLONG* pEarliest, LONGLONG* pLatest);
	STDMETHODIMP SetRate(double dRate);
	STDMETHODIMP GetRate(double* pdRate);
	STDMETHODIMP GetPreroll(LONGLONG* pllPreroll);

public:
	CBaseSplitterOutputPin(std::vector<CMediaType>& mts, LPCWSTR pName, CBaseFilter* pFilter, CCritSec* pLock, HRESULT* phr);
	CBaseSplitterOutputPin(LPCWSTR pName, CBaseFilter* pFilter, CCritSec* pLock, HRESULT* phr);
	virtual ~CBaseSplitterOutputPin();

	DECLARE_IUNKNOWN;
	STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);

	HRESULT SetName(LPCWSTR pName);

	HRESULT DecideBufferSize(IMemAllocator* pAlloc, ALLOCATOR_PROPERTIES* pProperties);
	HRESULT CheckMediaType(const CMediaType* pmt);
	HRESULT GetMediaType(int iPosition, CMediaType* pmt);
	CMediaType& CurrentMediaType() {return m_mt;}

	STDMETHODIMP Notify(IBaseFilter* pSender, Quality q) { return E_NOTIMPL; }

	// Queueing

	HANDLE GetThreadHandle() {
		ASSERT(m_hThread != nullptr);
		return m_hThread;
	}
	void SetThreadPriority(int nPriority) {
		if (m_hThread) {
			::SetThreadPriority(m_hThread, nPriority);
		}
	}

	HRESULT Active();
	HRESULT Inactive();

	HRESULT DeliverBeginFlush();
	HRESULT DeliverEndFlush();
	virtual HRESULT DeliverNewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate);

	size_t QueueCount();
	size_t QueueSize();
	HRESULT QueueEndOfStream();
	virtual HRESULT QueuePacket(CAutoPtr<CPacket> p);

	// returns true for everything which (the lack of) would not block other streams (subtitle streams, basically)
	virtual bool IsDiscontinuous();

	// returns IStreamsSwitcherInputPin::IsActive(), when it can find one downstream
	bool IsActive();

	// IBitRateInfo

	STDMETHODIMP_(DWORD) GetCurrentBitRate() { return m_brs.nCurrentBitRate; }
	STDMETHODIMP_(DWORD) GetAverageBitRate() { return m_brs.nAverageBitRate; }

	REFERENCE_TIME GetOffset() { return m_rtOffset; }
};

class CSubtitleStatus
{
protected:
	enum subtitletype_t {
		not_hdmv_dvbsub = 0,
		hdmvsub,
		dvbsub,
	};

	subtitletype_t m_SubtitleType = not_hdmv_dvbsub;
	bool m_bNeedNextSubtitle = false;

public:
	bool NeedNextSubtitle() { return m_bNeedNextSubtitle; }
};
