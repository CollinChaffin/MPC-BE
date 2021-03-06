/*
* (C) 2012-2018 see Authors.txt
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
#include <windows.h>
#include <initguid.h>
#include <moreuuids.h>
#include <mmreg.h>

#include "MusePackSplitter.h"
#include "../DSUtil/ApeTag.h"

#ifdef REGISTER_FILTER

const AMOVIESETUP_MEDIATYPE sudPinTypesIn[] = {
	{&MEDIATYPE_Stream, &MEDIASUBTYPE_NULL},
	{&MEDIATYPE_Stream, &MEDIASUBTYPE_MPC7},
	{&MEDIATYPE_Stream, &MEDIASUBTYPE_MPC8},
};

const AMOVIESETUP_PIN sudpPins[] = {
	{L"Input", FALSE, TRUE, FALSE, FALSE, &CLSID_NULL, nullptr, _countof(sudPinTypesIn), sudPinTypesIn},
	{L"Output", FALSE, TRUE, FALSE, FALSE, &CLSID_NULL, nullptr, 0, nullptr}
};

const AMOVIESETUP_FILTER sudFilter[] = {
	{&__uuidof(CMusePackSplitter), MusePackSplitterName, MERIT_NORMAL+1, _countof(sudpPins), sudpPins, CLSID_LegacyAmFilterCategory}
};

CFactoryTemplate g_Templates[] = {
	{sudFilter[0].strName, sudFilter[0].clsID, CreateInstance<CMusePackSplitter>, nullptr, &sudFilter[0]}
};

int g_cTemplates = _countof(g_Templates);

STDAPI DllRegisterServer()
{
	return AMovieDllRegisterServer2(TRUE);
}

STDAPI DllUnregisterServer()
{
	return AMovieDllRegisterServer2(FALSE);
}

#include "../../filters/Filters.h"

CFilterApp theApp;

#endif

//-----------------------------------------------------------------------------
//
//	CMusePackSplitter class
//
//-----------------------------------------------------------------------------

CUnknown *CMusePackSplitter::CreateInstance(LPUNKNOWN pUnk, HRESULT *phr)
{
	return DNew CMusePackSplitter(pUnk, phr);
}

CMusePackSplitter::CMusePackSplitter(LPUNKNOWN pUnk, HRESULT *phr)
	: CBaseFilter(L"Light Alloy/MPC MusePack Splitter", pUnk, &lock_filter, CLSID_MusePackSplitter, phr)
	, CAMThread()
	, reader(nullptr)
	, file(nullptr)
	, wnd_prop(nullptr)
	, rtCurrent(0)
	, rtStop(_I64_MAX)
	, rate(1.0)
	, ev_abort(TRUE)
	, output(nullptr)
{
	input = DNew CMusePackInputPin(NAME("MPC Input Pin"), this, phr, L"In");
	retired.clear();

	ev_abort.Reset();
}

CMusePackSplitter::~CMusePackSplitter()
{
	// just to be sure
	if (reader) {
		delete reader; reader = nullptr;
	}

	if (output) {
		delete output;
		output = nullptr;
	}

	for (size_t i = 0; i < retired.size(); i++) {
		CMusePackOutputPin *pin = retired[i];
		if (pin) {
			delete pin;
		}
	}
	retired.clear();

	if (input) {
		delete input;
		input = nullptr;
	}
}

STDMETHODIMP CMusePackSplitter::NonDelegatingQueryInterface(REFIID riid,void **ppv)
{
	CheckPointer(ppv,E_POINTER);

	if (riid == IID_IMediaSeeking) {
		return GetInterface((IMediaSeeking*)this, ppv);
	}

	return
		QI2(IAMMediaContent)
		QI(IPropertyBag)
		QI(IPropertyBag2)
		QI(IDSMResourceBag)
		QI(IDSMChapterBag)
		QI(IDSMPropertyBag)
		__super::NonDelegatingQueryInterface(riid, ppv);
}

int CMusePackSplitter::GetPinCount()
{
	// return pin count
	CAutoLock Lock(&lock_filter);

	return (input ? 1 : 0) + (output ? 1 : 0);
}

CBasePin *CMusePackSplitter::GetPin(int n)
{
	CAutoLock Lock(&lock_filter);

	if (n == 0) {
		return input;
	}

	if (n == 1) {
		return output;
	}

	return nullptr;
}

HRESULT CMusePackSplitter::CheckConnect(PIN_DIRECTION Dir, IPin *pPin)
{
	return S_OK;
}

HRESULT CMusePackSplitter::CheckInputType(const CMediaType* mtIn)
{
	if (mtIn->majortype == MEDIATYPE_Stream) {
		// we are sure we can accept this type
		if (mtIn->subtype == MEDIASUBTYPE_MUSEPACK_Stream) {
			return S_OK;
		}

		// and we may accept unknown type as well
		if (mtIn->subtype == MEDIASUBTYPE_None ||
			mtIn->subtype == MEDIASUBTYPE_NULL) {
			return S_OK;
		}
	} else if (mtIn->majortype == MEDIASUBTYPE_NULL) {
		return S_OK;
	}

	// sorry.. nothing else
	return VFW_E_TYPE_NOT_ACCEPTED;
}

HRESULT CMusePackSplitter::CompleteConnect(PIN_DIRECTION Dir, CBasePin *pCaller, IPin *pReceivePin)
{
	if (Dir == PINDIR_INPUT) {
		// when our input pin gets connected we have to scan
		// the input file if it is really musepack.
		ASSERT(input && input->Reader());
		ASSERT(!reader);
		ASSERT(!file);

		CAutoLock lck(&lock_filter);

		reader	= DNew CMusePackReader(input->Reader());
		file	= DNew CMPCFile();

		// try to open the file
		int ret = file->Open(reader);
		if (ret < 0) {
			delete file;
			file = nullptr;
			delete reader;
			reader = nullptr;
			return E_FAIL;
		}

		// parse APE Tag Header
		char buf[APE_TAG_FOOTER_BYTES];
		memset(buf, 0, sizeof(buf));
		__int64 cur_pos, file_size;
		reader->GetPosition(&cur_pos, &file_size);

		if (cur_pos + APE_TAG_FOOTER_BYTES <= file_size) {
			reader->Seek(file_size - APE_TAG_FOOTER_BYTES);
			if (reader->Read(buf, APE_TAG_FOOTER_BYTES) >= 0) {
				CAPETag* APETag = DNew CAPETag;
				if (APETag->ReadFooter((BYTE*)buf, APE_TAG_FOOTER_BYTES) && APETag->GetTagSize()) {
					size_t tag_size = APETag->GetTagSize();
					reader->Seek(file_size - tag_size);
					BYTE *p = DNew BYTE[tag_size];
					if (reader->Read(p, tag_size) >= 0 && APETag->ReadTags(p, tag_size)) {
						SetAPETagProperties(this, APETag);
					}

					delete [] p;
				}

				delete APETag;
			}

			reader->Seek(cur_pos);
		}

		HRESULT hr = S_OK;
		CMusePackOutputPin *opin = DNew CMusePackOutputPin(L"Outpin", this, &hr, L"Out", 5);
		ConfigureMediaType(opin);
		SetOutputPin(opin);
	}

	return S_OK;
}

HRESULT CMusePackSplitter::RemoveOutputPin()
{
	CAutoLock Lck(&lock_filter);

	if (m_State != State_Stopped) {
		return VFW_E_NOT_STOPPED;
	}

	if (output) {
		if (output->IsConnected()) {
			output->GetConnected()->Disconnect();
			output->Disconnect();
		}

		retired.push_back(output);
		output = nullptr;
	}

	return S_OK;
}

HRESULT CMusePackSplitter::ConfigureMediaType(CMusePackOutputPin *pin)
{
	ASSERT(file);

	int extrasize = file->extradata_size;

	WAVEFORMATEX* wfe		= (WAVEFORMATEX*)DNew BYTE[sizeof(WAVEFORMATEX) + extrasize];
	memset(wfe, 0, sizeof(WAVEFORMATEX));
	wfe->wFormatTag			= WAVE_FORMAT_PCM;
	wfe->nChannels			= file->channels;
	wfe->nSamplesPerSec		= file->sample_rate;
	wfe->wBitsPerSample		= 16;
	wfe->nBlockAlign		= (wfe->wBitsPerSample * wfe->nChannels) >> 3;
	wfe->nAvgBytesPerSec	= (wfe->nBlockAlign * wfe->nSamplesPerSec);
	wfe->cbSize = extrasize;
	if(extrasize) {
		memcpy((BYTE*)(wfe+1), file->extradata, extrasize);
	}

	CMediaType mt;
	ZeroMemory(&mt, sizeof(CMediaType));

	mt.majortype	= MEDIATYPE_Audio;
	mt.subtype		= (file->stream_version == 8) ? MEDIASUBTYPE_MPC8 : MEDIASUBTYPE_MPC7;
	mt.formattype	= FORMAT_WaveFormatEx;
	mt.SetSampleSize(256*1024);
	mt.SetFormat((BYTE*)wfe, sizeof(WAVEFORMATEX)+wfe->cbSize);

	delete [] wfe;

	// the one and only type
	pin->mt_types.push_back(mt);
	return S_OK;
}

HRESULT CMusePackSplitter::BreakConnect(PIN_DIRECTION Dir, CBasePin *pCaller)
{
	ASSERT(m_State == State_Stopped);

	if (Dir == PINDIR_INPUT) {
		// let's disconnect the output pins
		ev_abort.Set();
		//ev_ready.Set();

		HRESULT hr = RemoveOutputPin();
		if (FAILED(hr)) {
			return hr;
		}

		// destroy input file, reader and update property page
		if (file) {
			delete file;
			file = nullptr;
		}
		if (reader) {
			delete reader;
			reader = nullptr;
		}

		ev_abort.Reset();
	}

	return S_OK;
}

// Output pins
HRESULT CMusePackSplitter::SetOutputPin(CMusePackOutputPin *pPin)
{
	CAutoLock lck(&lock_filter);

	RemoveOutputPin();
	output = pPin;

	return S_OK;
}

// IMediaSeeking

STDMETHODIMP CMusePackSplitter::GetCapabilities(DWORD* pCapabilities)
{
	return pCapabilities ? *pCapabilities =
			AM_SEEKING_CanGetStopPos|AM_SEEKING_CanGetDuration|AM_SEEKING_CanSeekAbsolute|AM_SEEKING_CanSeekForwards|AM_SEEKING_CanSeekBackwards,
			S_OK : E_POINTER;
}

STDMETHODIMP CMusePackSplitter::CheckCapabilities(DWORD* pCapabilities)
{
	CheckPointer(pCapabilities, E_POINTER);
	if (*pCapabilities == 0) {
		return S_OK;
	}
	DWORD caps;
	GetCapabilities(&caps);
	if ((caps&*pCapabilities) == 0) {
		return E_FAIL;
	}
	if (caps == *pCapabilities) {
		return S_OK;
	}
	return S_FALSE;
}

STDMETHODIMP CMusePackSplitter::IsFormatSupported(const GUID* pFormat) {return !pFormat ? E_POINTER : *pFormat == TIME_FORMAT_MEDIA_TIME ? S_OK : S_FALSE;}
STDMETHODIMP CMusePackSplitter::QueryPreferredFormat(GUID* pFormat) {return GetTimeFormat(pFormat);}
STDMETHODIMP CMusePackSplitter::GetTimeFormat(GUID* pFormat) {return pFormat ? *pFormat = TIME_FORMAT_MEDIA_TIME, S_OK : E_POINTER;}
STDMETHODIMP CMusePackSplitter::IsUsingTimeFormat(const GUID* pFormat) {return IsFormatSupported(pFormat);}
STDMETHODIMP CMusePackSplitter::SetTimeFormat(const GUID* pFormat) {return S_OK == IsFormatSupported(pFormat) ? S_OK : E_INVALIDARG;}
STDMETHODIMP CMusePackSplitter::GetStopPosition(LONGLONG* pStop) {return this->rtStop; }
STDMETHODIMP CMusePackSplitter::GetCurrentPosition(LONGLONG* pCurrent) {return E_NOTIMPL;}
STDMETHODIMP CMusePackSplitter::ConvertTimeFormat(LONGLONG* pTarget, const GUID* pTargetFormat, LONGLONG Source, const GUID* pSourceFormat) {return E_NOTIMPL;}

STDMETHODIMP CMusePackSplitter::SetPositions(LONGLONG* pCurrent, DWORD dwCurrentFlags, LONGLONG* pStop, DWORD dwStopFlags)
{
	return SetPositionsInternal(0, pCurrent, dwCurrentFlags, pStop, dwStopFlags);
}

STDMETHODIMP CMusePackSplitter::GetPositions(LONGLONG* pCurrent, LONGLONG* pStop)
{
	if(pCurrent) {
		*pCurrent = rtCurrent;
	}
	if(pStop) {
		*pStop = rtStop;
	}
	return S_OK;
}

STDMETHODIMP CMusePackSplitter::GetAvailable(LONGLONG* pEarliest, LONGLONG* pLatest)
{
	if(pEarliest) {
		*pEarliest = 0;
	}
	return GetDuration(pLatest);
}

STDMETHODIMP CMusePackSplitter::SetRate(double dRate) {return dRate > 0 ? rate = dRate, S_OK : E_INVALIDARG;}
STDMETHODIMP CMusePackSplitter::GetRate(double* pdRate) {return pdRate ? *pdRate = rate, S_OK : E_POINTER;}
STDMETHODIMP CMusePackSplitter::GetPreroll(LONGLONG* pllPreroll) {return pllPreroll ? *pllPreroll = 0, S_OK : E_POINTER;}

STDMETHODIMP CMusePackSplitter::GetDuration(LONGLONG* pDuration)
{
	CheckPointer(pDuration, E_POINTER);

	if (file) {
		*pDuration = file->duration_10mhz;
	} else {
		*pDuration = 0;
	}
	return S_OK;
}

STDMETHODIMP CMusePackSplitter::SetPositionsInternal(int iD, LONGLONG* pCurrent, DWORD dwCurrentFlags, LONGLONG* pStop, DWORD dwStopFlags)
{
	// only our first pin can seek
	if (iD != 0) {
		return S_OK;
	}

	CAutoLock cAutoLock(&lock_filter);

	if (!pCurrent && !pStop || (dwCurrentFlags&AM_SEEKING_PositioningBitsMask) == AM_SEEKING_NoPositioning
		&& (dwStopFlags&AM_SEEKING_PositioningBitsMask) == AM_SEEKING_NoPositioning) {
		return S_OK;
	}

	REFERENCE_TIME rtCurrent = this->rtCurrent, rtStop = this->rtStop;

	if (pCurrent) {
		switch(dwCurrentFlags&AM_SEEKING_PositioningBitsMask) {
			case AM_SEEKING_NoPositioning: break;
			case AM_SEEKING_AbsolutePositioning: rtCurrent = *pCurrent; break;
			case AM_SEEKING_RelativePositioning: rtCurrent = rtCurrent + *pCurrent; break;
			case AM_SEEKING_IncrementalPositioning: rtCurrent = rtCurrent + *pCurrent; break;
		}
	}

	if (pStop) {
		switch(dwStopFlags&AM_SEEKING_PositioningBitsMask) {
			case AM_SEEKING_NoPositioning: break;
			case AM_SEEKING_AbsolutePositioning: rtStop = *pStop; break;
			case AM_SEEKING_RelativePositioning: rtStop += *pStop; break;
			case AM_SEEKING_IncrementalPositioning: rtStop = rtCurrent + *pStop; break;
		}
	}

	if (this->rtCurrent == rtCurrent && this->rtStop == rtStop) {
		//return S_OK;
	}

	this->rtCurrent = rtCurrent;
	this->rtStop = rtStop;

	// now there are new valid Current and Stop positions
	HRESULT hr = DoNewSeek();
	return hr;
}

STDMETHODIMP CMusePackSplitter::Pause()
{
	CAutoLock lck(&lock_filter);

	if (m_State == State_Stopped) {

		ev_abort.Reset();

		// activate pins
		if (output) {
			output->Active();
		}
		if (input) {
			input->Active();
		}

		// seekneme na danu poziciu
		DoNewSeek();

		// pustime parser thread
		if (!ThreadExists()) {
			Create();
			CallWorker(CMD_RUN);
		}
	}

	m_State = State_Paused;
	return S_OK;
}

STDMETHODIMP CMusePackSplitter::Stop()
{
	CAutoLock	lock(&lock_filter);
	HRESULT		hr = S_OK;

	if (m_State != State_Stopped) {

		// set abort
		ev_abort.Set();
		if (reader) {
			reader->BeginFlush();
		}

		if (input) {
			input->Inactive();
		}

		if (output) {
			output->Inactive();
		}

		if (ThreadExists()) {
			CallWorker(CMD_EXIT);
			Close();
		}

		if (reader) {
			reader->EndFlush();
		}
		ev_abort.Reset();
	}

	m_State = State_Stopped;
	return hr;
}

HRESULT CMusePackSplitter::DoNewSeek()
{
	CMusePackOutputPin	*pin = output;
	HRESULT				hr;

	if (!pin->IsConnected()) {
		return S_OK;
	}

	// stop first
	ev_abort.Set();
	if (reader) {
		reader->BeginFlush();
	}

	FILTER_STATE state = m_State;

	// abort
	if (state != State_Stopped) {
		if (pin->ThreadExists()) {
			pin->ev_abort.Set();
			hr = pin->DeliverBeginFlush();
			if (FAILED(hr)) {
				ASSERT(FALSE);
			}
			if (ThreadExists()) {
				CallWorker(CMD_STOP);
			}
			pin->CallWorker(CMD_STOP);

			hr = pin->DeliverEndFlush();
			if (FAILED(hr)) {
				ASSERT(FALSE);
			}
			pin->FlushQueue();
		}
	}

	pin->DoNewSegment(rtCurrent, rtStop, rate);
	if (reader) reader->EndFlush();

	// seek the file
	if (file) {
		int64_t sample_pos = (rtCurrent * file->sample_rate) / 10000000;
		file->Seek(sample_pos);
	}

	ev_abort.Reset();

	if (state != State_Stopped) {
		pin->FlushQueue();
		pin->ev_abort.Reset();
		if (pin->ThreadExists()) {
			pin->CallWorker(CMD_RUN);
		}
		if (ThreadExists()) {
			CallWorker(CMD_RUN);
		}
	}

	return S_OK;
}

DWORD CMusePackSplitter::ThreadProc()
{
	DWORD cmd, cmd2;
	while (true) {
		cmd = GetRequest();
		switch (cmd) {
			case CMD_EXIT:
				Reply(NOERROR);
				return 0;
			case CMD_STOP:
				Reply(NOERROR);
				break;
			case CMD_RUN:
				{
					Reply(NOERROR);
					if (!file) {
						break;
					}

					CMPCPacket	packet;
					int32_t		ret = 0;
					bool		eos = false;
					HRESULT		hr;
					int64_t		current_sample;

					if (!output) {
						break;
					}
					if (output->IsConnected() == FALSE) {
						break;
					}
					int delivered = 0;

					do {
						// are we supposed to abort ?
						if (ev_abort.Check()) {
							break;
						}

						ret = file->ReadAudioPacket(&packet, &current_sample);
						if (ret == -2) {
							// end of stream
							if (!ev_abort.Check()) {
								output->DoEndOfStream();
							}
							break;
						} else if (ret < 0) {
							break;
						} else {
							// compute time stamp
							REFERENCE_TIME	tStart = (current_sample * 10000000) / file->sample_rate;
							REFERENCE_TIME	tStop  = ((current_sample + 1152*file->audio_block_frames) * 10000000) / file->sample_rate;

							packet.tStart = tStart - rtCurrent;
							packet.tStop  = tStop  - rtCurrent;

							// deliver packet
							hr = output->DeliverPacket(packet);
							if (FAILED(hr)) {
								break;
							}

							delivered++;
						}
					} while (!CheckRequest(&cmd2));
				}
				break;
			default:
				Reply(E_UNEXPECTED);
				break;
		}
	}
	return S_OK;
}

// IAMMediaContent

STDMETHODIMP CMusePackSplitter::get_AuthorName(BSTR* pbstrAuthorName)
{
	return GetProperty(L"AUTH", pbstrAuthorName);
}

STDMETHODIMP CMusePackSplitter::get_Title(BSTR* pbstrTitle)
{
	return GetProperty(L"TITL", pbstrTitle);
}

STDMETHODIMP CMusePackSplitter::get_Rating(BSTR* pbstrRating)
{
	return GetProperty(L"RTNG", pbstrRating);
}

STDMETHODIMP CMusePackSplitter::get_Description(BSTR* pbstrDescription)
{
	return GetProperty(L"DESC", pbstrDescription);
}

STDMETHODIMP CMusePackSplitter::get_Copyright(BSTR* pbstrCopyright)
{
	return GetProperty(L"CPYR", pbstrCopyright);
}

//-----------------------------------------------------------------------------
//
//	CMusePackInputPin class
//
//-----------------------------------------------------------------------------

CMusePackInputPin::CMusePackInputPin(TCHAR *pObjectName, CMusePackSplitter *pDemux, HRESULT *phr, LPCWSTR pName) :
	CBaseInputPin(pObjectName, pDemux, &pDemux->lock_filter, phr, pName),
	reader(nullptr)
{
	// assign the splitter filter
	demux = pDemux;
}

CMusePackInputPin::~CMusePackInputPin()
{
	if (reader) {
		reader->Release();
		reader = nullptr;
	}
}

// this is a pull mode pin - these can not happen

STDMETHODIMP CMusePackInputPin::EndOfStream()		{ return E_UNEXPECTED; }
STDMETHODIMP CMusePackInputPin::BeginFlush()			{ return E_UNEXPECTED; }
STDMETHODIMP CMusePackInputPin::EndFlush()			{ return E_UNEXPECTED; }
STDMETHODIMP CMusePackInputPin::NewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double rate)	{ return E_UNEXPECTED; }

// we don't support this kind of transport
STDMETHODIMP CMusePackInputPin::Receive(IMediaSample * pSample) { return E_UNEXPECTED; }

// check for input type
HRESULT CMusePackInputPin::CheckConnect(IPin *pPin)
{
	HRESULT hr = demux->CheckConnect(PINDIR_INPUT, pPin);
	if (FAILED(hr)) {
		return hr;
	}

	return CBaseInputPin::CheckConnect(pPin);
}

HRESULT CMusePackInputPin::BreakConnect()
{
	// Can't disconnect unless stopped
	ASSERT(IsStopped());

	demux->BreakConnect(PINDIR_INPUT, this);

	// release the reader
	if (reader) {
		reader->Release();
		reader = nullptr;
	}

	return CBaseInputPin::BreakConnect();
}

HRESULT CMusePackInputPin::CompleteConnect(IPin *pReceivePin)
{
	// make sure there is a pin
	ASSERT(pReceivePin);

	if (reader) reader->Release(); reader = nullptr;

	// and make sure it supports IAsyncReader
	HRESULT hr = pReceivePin->QueryInterface(IID_PPV_ARGS(&reader));
	if (FAILED(hr)) {
		return hr;
	}

	// we're done here
	hr = demux->CompleteConnect(PINDIR_INPUT, this, pReceivePin);
	if (FAILED(hr)) {
		return hr;
	}

	return CBaseInputPin::CompleteConnect(pReceivePin);
}

HRESULT CMusePackInputPin::CheckMediaType(const CMediaType* pmt)
{
	// make sure we have a type
	ASSERT(pmt);

	// ask the splitter
	return demux->CheckInputType(pmt);
}

HRESULT CMusePackInputPin::SetMediaType(const CMediaType* pmt)
{
	// let the baseclass know
	if (FAILED(CheckMediaType(pmt))) {
		return E_FAIL;
	}

	HRESULT hr = CBasePin::SetMediaType(pmt);
	if (FAILED(hr)) {
		return hr;
	}

	return S_OK;
}

HRESULT CMusePackInputPin::Inactive()
{
	HRESULT hr = CBaseInputPin::Inactive();
	if (hr == VFW_E_NO_ALLOCATOR) {
		hr = S_OK;
	}

	return hr;
}

//-----------------------------------------------------------------------------
//
//	CMusePackOutputPin class
//
//-----------------------------------------------------------------------------

CMusePackOutputPin::CMusePackOutputPin(TCHAR *pObjectName, CMusePackSplitter *pDemux, HRESULT *phr, LPCWSTR pName, int iBuffers)
	: CBaseOutputPin(pObjectName, pDemux, &pDemux->lock_filter, phr, pName)
	, CAMThread()
	, demux(pDemux)
	, buffers(iBuffers)
	, active(false)
	, rtStart(0)
	, rtStop(0xffffffffffff)
	, rate(1.0)
	, ev_can_read(TRUE)
	, ev_can_write(TRUE)
	, ev_abort(TRUE)
{
	discontinuity = true;
	eos_delivered = false;

	ev_can_read.Reset();
	ev_can_write.Set();
	ev_abort.Reset();

	// up to 5 seconds
	buffer_time_ms = 5000;
}

CMusePackOutputPin::~CMusePackOutputPin()
{
}

STDMETHODIMP CMusePackOutputPin::NonDelegatingQueryInterface(REFIID riid, void **ppv)
{
	CheckPointer(ppv, E_POINTER);
	if (riid == IID_IMediaSeeking) {
		return GetInterface((IMediaSeeking*)this, ppv);
	} else {
		return CBaseOutputPin::NonDelegatingQueryInterface(riid, ppv);
	}
}

HRESULT CMusePackOutputPin::CheckMediaType(const CMediaType *mtOut)
{
	for (size_t i = 0; i < mt_types.size(); i++) {
		if (mt_types[i] == *mtOut) {
			return S_OK;
		}
	}

	// reject type if it's not among ours
	return E_FAIL;
}

HRESULT CMusePackOutputPin::SetMediaType(const CMediaType *pmt)
{
	// just set internal media type
	if (FAILED(CheckMediaType(pmt))) {
		return E_FAIL;
	}

	return CBaseOutputPin::SetMediaType(pmt);
}

HRESULT CMusePackOutputPin::GetMediaType(int iPosition, CMediaType *pmt)
{
	// enumerate the list
	if (iPosition < 0) {
		return E_INVALIDARG;
	}
	if (iPosition >= (int)mt_types.size()) {
		return VFW_S_NO_MORE_ITEMS;
	}

	*pmt = mt_types[iPosition];
	return S_OK;
}

HRESULT CMusePackOutputPin::DecideBufferSize(IMemAllocator *pAlloc, ALLOCATOR_PROPERTIES *pProp)
{
	ASSERT(pAlloc);
	ASSERT(pProp);
	HRESULT hr = S_OK;

	pProp->cBuffers = std::max(buffers, 1);
	pProp->cbBuffer = std::max(m_mt.lSampleSize, 1uL);
	ALLOCATOR_PROPERTIES Actual;
	hr = pAlloc->SetProperties(pProp, &Actual);
	if (FAILED(hr)) {
		return hr;
	}

	ASSERT(Actual.cBuffers >= pProp->cBuffers);
	return S_OK;
}

HRESULT CMusePackOutputPin::BreakConnect()
{
	ASSERT(IsStopped());
	demux->BreakConnect(PINDIR_OUTPUT, this);
	discontinuity = true;
	return CBaseOutputPin::BreakConnect();
}

HRESULT CMusePackOutputPin::CompleteConnect(IPin *pReceivePin)
{
	// let the parent know
	HRESULT hr = demux->CompleteConnect(PINDIR_OUTPUT, this, pReceivePin);
	if (FAILED(hr)) return hr;

	discontinuity = true;
	// okey
	return CBaseOutputPin::CompleteConnect(pReceivePin);
}

STDMETHODIMP CMusePackOutputPin::Notify(IBaseFilter *pSender, Quality q)
{
	return S_FALSE;
}

HRESULT CMusePackOutputPin::Inactive()
{
	if (!active) {
		return NOERROR;
	}
	active = FALSE;

	// destroy everything
	ev_abort.Set();
	HRESULT hr = CBaseOutputPin::Inactive();
	ASSERT(SUCCEEDED(hr));

	if (ThreadExists()) {
		CallWorker(CMD_EXIT);
		Close();
	}
	FlushQueue();
	ev_abort.Reset();

	return S_OK;
}

HRESULT CMusePackOutputPin::Active()
{
	if (active) return S_OK;

	FlushQueue();
	if (!IsConnected()) {
		return VFW_E_NOT_CONNECTED;
	}

	ev_abort.Reset();

	HRESULT hr = CBaseOutputPin::Active();
	if (FAILED(hr)) {
		active = FALSE;
		return hr;
	}

	// new segment
	DoNewSegment(rtStart, rtStop, rate);

	if (!ThreadExists()) {
		Create();
		CallWorker(CMD_RUN);
	}

	active = TRUE;
	return hr;
}

HRESULT CMusePackOutputPin::DoNewSegment(REFERENCE_TIME rtStart, REFERENCE_TIME rtStop, double dRate)
{
	// queue the EOS packet
	this->rtStart = rtStart;
	this->rtStop = rtStop;
	this->rate = dRate;
	eos_delivered = false;

	if (1) { // TODO - What is this ??? :)
		return DeliverNewSegment(rtStart, rtStop, rate);
	} else {
		DataPacketMPC	*packet = DNew DataPacketMPC();
		{
			CAutoLock	lck(&lock_queue);
			packet->type = DataPacketMPC::PACKET_TYPE_NEW_SEGMENT;
			packet->rtStart = rtStart;
			packet->rtStop = rtStop;
			packet->has_time = true;
			packet->rate = rate;
			queue.push_back(packet);
			ev_can_read.Set();
		}
	}
	return S_OK;
}

// IMediaSeeking
STDMETHODIMP CMusePackOutputPin::GetCapabilities(DWORD* pCapabilities)					{ return demux->GetCapabilities(pCapabilities); }
STDMETHODIMP CMusePackOutputPin::CheckCapabilities(DWORD* pCapabilities)				{ return demux->CheckCapabilities(pCapabilities); }
STDMETHODIMP CMusePackOutputPin::IsFormatSupported(const GUID* pFormat)					{ return demux->IsFormatSupported(pFormat); }
STDMETHODIMP CMusePackOutputPin::QueryPreferredFormat(GUID* pFormat)					{ return demux->QueryPreferredFormat(pFormat); }
STDMETHODIMP CMusePackOutputPin::GetTimeFormat(GUID* pFormat)							{ return demux->GetTimeFormat(pFormat); }
STDMETHODIMP CMusePackOutputPin::IsUsingTimeFormat(const GUID* pFormat)					{ return demux->IsUsingTimeFormat(pFormat); }
STDMETHODIMP CMusePackOutputPin::SetTimeFormat(const GUID* pFormat)						{ return demux->SetTimeFormat(pFormat); }
STDMETHODIMP CMusePackOutputPin::GetDuration(LONGLONG* pDuration)						{ return demux->GetDuration(pDuration); }
STDMETHODIMP CMusePackOutputPin::GetStopPosition(LONGLONG* pStop)						{ return demux->GetStopPosition(pStop); }
STDMETHODIMP CMusePackOutputPin::GetCurrentPosition(LONGLONG* pCurrent)					{ return demux->GetCurrentPosition(pCurrent); }
STDMETHODIMP CMusePackOutputPin::GetPositions(LONGLONG* pCurrent, LONGLONG* pStop)		{ return demux->GetPositions(pCurrent, pStop); }
STDMETHODIMP CMusePackOutputPin::GetAvailable(LONGLONG* pEarliest, LONGLONG* pLatest)	{ return demux->GetAvailable(pEarliest, pLatest); }
STDMETHODIMP CMusePackOutputPin::SetRate(double dRate)									{ return demux->SetRate(dRate); }
STDMETHODIMP CMusePackOutputPin::GetRate(double* pdRate)								{ return demux->GetRate(pdRate); }
STDMETHODIMP CMusePackOutputPin::GetPreroll(LONGLONG* pllPreroll)						{ return demux->GetPreroll(pllPreroll); }

STDMETHODIMP CMusePackOutputPin::ConvertTimeFormat(LONGLONG* pTarget, const GUID* pTargetFormat, LONGLONG Source, const GUID* pSourceFormat)
{
	return demux->ConvertTimeFormat(pTarget, pTargetFormat, Source, pSourceFormat);
}

STDMETHODIMP CMusePackOutputPin::SetPositions(LONGLONG* pCurrent, DWORD dwCurrentFlags, LONGLONG* pStop, DWORD dwStopFlags)
{
	return demux->SetPositionsInternal(0, pCurrent, dwCurrentFlags, pStop, dwStopFlags);
}

int CMusePackOutputPin::GetDataPacketMPC(DataPacketMPC **packet)
{
	// this method may blokc
	HANDLE	events[] = { ev_can_write, ev_abort };
	DWORD	ret;

	do {
		// if the abort event is set, quit
		if (ev_abort.Check()) {
			return -1;
		}

		ret = WaitForMultipleObjects(2, events, FALSE, 10);
		if (ret == WAIT_OBJECT_0) {
			// return new packet
			*packet = DNew DataPacketMPC();
			return 0;
		}
	} while (1);

	// unexpected
	return -1;
}

HRESULT CMusePackOutputPin::DeliverPacket(CMPCPacket &packet)
{
	// we don't accept packets while aborting...
	if (ev_abort.Check()) {
		return E_FAIL;
	}

	// ziskame novy packet na vystup
	DataPacketMPC	*outp = nullptr;
	int ret = GetDataPacketMPC(&outp);
	if (ret < 0 || !outp) {
		return E_FAIL;
	}

	outp->type	  = DataPacketMPC::PACKET_TYPE_DATA;

	// spocitame casy
	outp->rtStart = packet.tStart;
	outp->rtStop  = packet.tStop;

	outp->size	  = packet.payload_size;
	outp->buf	  = (BYTE*)malloc(outp->size);
	memcpy(outp->buf, packet.payload, packet.payload_size);

	// each packet is sync point
	outp->sync_point = TRUE;

	{
		// insert into queue
		CAutoLock lck(&lock_queue);
		queue.push_back(outp);
		ev_can_read.Set();

		// we allow buffering for a few seconds (might be usefull for network playback)
		if (GetBufferTime_MS() >= buffer_time_ms) {
			ev_can_write.Reset();
		}
	}

	return S_OK;
}

HRESULT CMusePackOutputPin::DoEndOfStream()
{
	DataPacketMPC	*packet = DNew DataPacketMPC();

	// naqueueujeme EOS
	{
		CAutoLock lck(&lock_queue);

		packet->type = DataPacketMPC::PACKET_TYPE_EOS;
		queue.push_back(packet);
		ev_can_read.Set();
	}
	eos_delivered = true;

	return S_OK;
}

void CMusePackOutputPin::FlushQueue()
{
	CAutoLock lck(&lock_queue);

	while (queue.size()) {
		DataPacketMPC *packet = queue.front();
		queue.pop_front();
		if (packet) delete packet;
	}
	ev_can_read.Reset();
	ev_can_write.Set();
}

HRESULT CMusePackOutputPin::DeliverDataPacketMPC(DataPacketMPC &packet)
{
	IMediaSample *sample;
	HRESULT hr = GetDeliveryBuffer(&sample, nullptr, nullptr, 0);
	if (FAILED(hr)) {
		return E_FAIL;
	}

	// we should have enough space in there
	long lsize = sample->GetSize();
	ASSERT(lsize >= packet.size);

	BYTE *buf;
	sample->GetPointer(&buf);

	memcpy(buf, packet.buf, packet.size);
	sample->SetActualDataLength(packet.size);

	// sync point, discontinuity ?
	if (packet.sync_point) {
		sample->SetSyncPoint(TRUE);
	} else {
		sample->SetSyncPoint(FALSE);
	}

	if (discontinuity)	{
		discontinuity = false;
		sample->SetDiscontinuity(TRUE);
	} else {
		sample->SetDiscontinuity(FALSE);
	}

	// do we have a time stamp ?
	if (packet.has_time) {
		sample->SetTime(&packet.rtStart, &packet.rtStop);
	}

	hr = Deliver(sample);
	sample->Release();
	return hr;
}

__int64 CMusePackOutputPin::GetBufferTime_MS()
{
	CAutoLock lck(&lock_queue);
	if (queue.empty()) return 0;

	DataPacketMPC *pfirst;
	DataPacketMPC *plast;
	__int64 tstart = -1;
	__int64 tstop  = -1;

	auto it = queue.begin();
	while (it != queue.end()) {
		pfirst = *it++;
		if (pfirst->type == DataPacketMPC::PACKET_TYPE_DATA && pfirst->rtStart != -1) {
			tstart = pfirst->rtStart;
			break;
		}
	}

	it = queue.end();
	do {
		plast = *(--it);
		if (plast->type == DataPacketMPC::PACKET_TYPE_DATA && plast->rtStart != -1) {
			tstop = plast->rtStart;
			break;
		}
	} while (it != queue.begin());

	if (tstart == -1 || tstop == -1) {
		return 0;
	}

	// calculate time in milliseconds
	return (tstop - tstart) / 10000;
}

// vlakno na output
DWORD CMusePackOutputPin::ThreadProc()
{
	while (true) {
		DWORD cmd = GetRequest();
		switch (cmd) {
			case CMD_EXIT:
				Reply(0);
				return 0;
				break;
			case CMD_STOP:
				Reply(0);
				break;
			case CMD_RUN:
				{
					Reply(0);
					if (!IsConnected()) break;

					// deliver packets
					DWORD		cmd2;
					BOOL		is_first = true;
					DataPacketMPC	*packet;
					HRESULT		hr;

					// first packet is a discontinuity
					discontinuity = true;

					do {
						if (ev_abort.Check()) break;
						hr = NOERROR;

						HANDLE	events[] = { ev_can_read, ev_abort};
						DWORD	ret = WaitForMultipleObjects(2, events, FALSE, 10);

						if (ret == WAIT_OBJECT_0) {
							// look for new packet in queue
							{
								CAutoLock lck(&lock_queue);

								packet = queue.front();
								queue.pop_front();
								if (queue.empty()) ev_can_read.Reset();

								// allow buffering
								if (GetBufferTime_MS() < buffer_time_ms) {
									ev_can_write.Set();
								}
							}

							// bud dorucime End Of Stream, alebo packet
							if (packet->type == DataPacketMPC::PACKET_TYPE_EOS) {
								DeliverEndOfStream();
							} else if (packet->type == DataPacketMPC::PACKET_TYPE_NEW_SEGMENT) {
								hr = DeliverNewSegment(packet->rtStart, packet->rtStop, packet->rate);
							} else if (packet->type == DataPacketMPC::PACKET_TYPE_DATA) {
								hr = DeliverDataPacketMPC(*packet);
							}

							delete packet;

							if (FAILED(hr)) {
								break;
							}
						}
					} while (!CheckRequest(&cmd2));
				}
				break;
			default:
				Reply(E_UNEXPECTED);
				break;
		}
	}
	return S_OK;
}

//-----------------------------------------------------------------------------
//
//	CMusePackReader class
//
//-----------------------------------------------------------------------------
CMusePackReader::CMusePackReader(IAsyncReader *rd)
{
	ASSERT(rd);

	reader = rd;
	reader->AddRef();
	position = 0;
}

CMusePackReader::~CMusePackReader()
{
	if (reader) {
		reader->Release();
		reader = nullptr;
	}
}

int CMusePackReader::GetSize(__int64 *avail, __int64 *total)
{
	HRESULT hr = reader->Length(total, avail);
	if (FAILED(hr)) {
		return -1;
	}
	return 0;
}

int CMusePackReader::GetPosition(__int64 *pos, __int64 *avail)
{
	HRESULT hr;
	__int64	total;
	hr = reader->Length(&total, avail);
	if (FAILED(hr)) {
		return -1;
	}

	// Actual position.
	if (pos) {
		*pos = position;
	}

	return 0;
}

int CMusePackReader::Seek(__int64 pos)
{
	__int64 avail, total;
	GetSize(&avail, &total);
	if (pos < 0 || pos >= total) {
		return -1;
	}

	position = pos;
	return 0;
}

int CMusePackReader::Read(void *buf, int size)
{
	__int64 avail, total;
	GetSize(&avail, &total);
	if (position + size > avail) {
		return -1;
	}

	// TODO: Caching here !!!!
	//DLog(L"    - read, %I64d, %d", position, size);

	HRESULT hr = reader->SyncRead(position, size, (BYTE*)buf);
	if (FAILED(hr)) {
		return -1;
	}

	// update position
	position += size;
	return 0;
}

int CMusePackReader::ReadSwapped(void *buf, int size)
{
	// this method should only be used to read smaller chunks of data - 
	// one maximum MPC SV7 frame
	uint8_t temp[16*1024];

	// 32-bit aligned words
	__int64 cur, av;
	GetPosition(&cur, &av);

	int preroll			= cur & 0x03;
	__int64	aligned_pos	= cur - preroll;
	int read_size		= ((size + preroll) + 0x03) &~ 0x03;		// alignment

	// read the data
	Seek(aligned_pos);
	int ret = Read(temp, read_size);
	if (ret < 0) {
		return ret;
	}

	// swap bytes
	uint8_t *bcur = temp;
	while (read_size > 3) {
		std::swap(bcur[0], bcur[3]);
		std::swap(bcur[1], bcur[2]);
		bcur += 4;
		read_size -= 4;
	}

	// copy data
	bcur = temp + preroll;
	memcpy(buf, bcur, size);

	// advance
	Seek(cur + size);

	return 0;
}

// reading syntax elements
int CMusePackReader::GetMagick(uint32_t &elm)
{
	uint8_t buf[4];
	int ret = Read(buf, sizeof(buf));
	if (ret < 0) {
		return ret;
	}

	// network byte order
	elm = (buf[0] << 24) | (buf[1] << 16) | (buf[2] << 8) | (buf[3]);
	return 0;
}

int CMusePackReader::GetKey(uint16_t &key)
{
	uint8_t buf[2];
	int ret = Read(buf, sizeof(buf));
	if (ret < 0) {
		return ret;
	}

	// network byte order
	key = (buf[0] << 8) | (buf[1]);
	return 0;
}

int CMusePackReader::GetSizeElm(int64_t &size, int32_t &size_len)
{
	int ret;
	size = 0;
	size_len = 1;

	while (true) {
		uint8_t val;
		ret = Read(&val, 1);
		if (ret < 0) {
			return ret;
		}

		// this is the last one
		if (!(val&0x80)) {
			size |= val&0x7f;
			return 0;
		}

		// not the last one
		size |= (val&0x7f);
		size <<= 7;
		size_len ++;
	}
	return 0;
}

bool CMusePackReader::KeyValid(uint16_t key)
{
	// according to the specs keys can only take specific values.
	if (((key>>8)&0xff) < 65 || ((key>>8)&0xff) > 90) {
		return false;
	}
	if ((key&0xff) < 65 || (key&0xff) > 90) {
		return false;
	}
	return true;
}

DataPacketMPC::DataPacketMPC()
	: type(PACKET_TYPE_EOS)
	, rtStart(0)
	, rtStop(0)
	, rate(1.0)
	, has_time(false)
	, sync_point(FALSE)
	, discontinuity(FALSE)
	, buf(nullptr)
	, size(0)
{
}

DataPacketMPC::~DataPacketMPC()
{
	if (buf) {
		free(buf);
		buf = nullptr;
	}
}
