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

#include "stdafx.h"
#include <ks.h>
#include <ksmedia.h>
#include <BaseClasses/streams.h>
#include <mpeg2data.h>
#include <tuner.h>
#include <dvbsiparser.h>
#include "../../DSUtil/GolombBuffer.h"
#include "../../filters/switcher/AudioSwitcher/AudioSwitcher.h"
#include <moreuuids.h>
#include <basestruct.h>
#include "FGManagerBDA.h"
#include "DVBChannel.h"
#include "Mpeg2SectionData.h"
#include "MainFrm.h"
#include "../../DSUtil/SysVersion.h"


/// Format, Video MPEG2
static const MPEG2VIDEOINFO sMpv_fmt = {
	{
		// hdr
		{0,0,720,576},				// rcSource
		{0,0,0,0},					// rcTarget
		0,							// dwBitRate
		0,							// dwBitErrorRate
		0,							// AvgTimePerFrame
		0,							// dwInterlaceFlags
		0,							// dwCopyProtectFlags
		0,							// dwPictAspectRatioX
		0,							// dwPictAspectRatioY
		{0},						// dwControlFlag & dwReserved1
		0,							// dwReserved2
		{
			// bmiHeader
			sizeof(BITMAPINFOHEADER),// biSize
			720,					// biWidth
			576						// biHeight
		}
		// implicitly sets the others fields to 0
	}
};

/// Media type, Video MPEG2
static const AM_MEDIA_TYPE mt_Mpv = {
	MEDIATYPE_Video,				// majortype
	MEDIASUBTYPE_MPEG2_VIDEO,		// subtype
	FALSE,							// bFixedSizeSamples
	TRUE,							// bTemporalCompression
	0,								// lSampleSize
	FORMAT_MPEG2Video,				// formattype
	nullptr,						// pUnk
	sizeof(sMpv_fmt),				// cbFormat
	(LPBYTE)&sMpv_fmt				// pbFormat
};

#define FCC_h264 MAKEFOURCC('h', '2', '6', '4')

/// Format, Video H264
static const VIDEOINFOHEADER2 vih2_H264 = {
	{0,0,0,0},						// rcSource
	{0,0,0,0},						// rcTarget
	0,								// dwBitRate,
	0,								// dwBitErrorRate
	0,								// AvgTimePerFrame
	0,								// dwInterlaceFlags
	0,								// dwCopyProtectFlags
	0,								// dwPictAspectRatioX
	0,								// dwPictAspectRatioY
	{0},							// dwControlFlag & dwReserved1
	0,								// dwReserved2
	{
		// bmiHeader
		sizeof(BITMAPINFOHEADER),	// biSize
		//		720,						// biWidth
		//		576,						// biHeight
		1920,						// biWidth
		1080,						// biHeight
		0,							// biPlanes
		0,							// biBitCount
		FCC_h264					// biCompression
	}
	// implicitly sets the others fields to 0
};

/// Media type, Video H264
static const AM_MEDIA_TYPE mt_H264 = {
	MEDIATYPE_Video,				// majortype
	MEDIASUBTYPE_H264,				// subtype
	FALSE,							// bFixedSizeSamples
	TRUE,							// bTemporalCompression
	1,								// lSampleSize
	FORMAT_VideoInfo2,				// formattype
	nullptr,						// pUnk
	sizeof(vih2_H264),				// cbFormat
	(LPBYTE)&vih2_H264				// pbFormat
};

/// Format, Audio (common)
static const WAVEFORMATEX wf_Audio = {
	WAVE_FORMAT_PCM,				// wFormatTag
	2,								// nChannels
	48000,							// nSamplesPerSec
	4*48000,						// nAvgBytesPerSec
	4,								// nBlockAlign
	16,								// wBitsPerSample
	0								// cbSize
};

/// Media type, Audio MPEG2
static const AM_MEDIA_TYPE mt_Mpa = {
	MEDIATYPE_Audio,				// majortype
	MEDIASUBTYPE_MPEG2_AUDIO,		// subtype
	TRUE,							// bFixedSizeSamples
	FALSE,							// bTemporalCompression
	0,								// lSampleSize
	FORMAT_WaveFormatEx,			// formattype
	nullptr,						// pUnk
	sizeof(wf_Audio),				// cbFormat
	(LPBYTE)&wf_Audio				// pbFormat
};

/// Media type, Audio AC3
static const AM_MEDIA_TYPE mt_Ac3 = {
	MEDIATYPE_Audio,				// majortype
	MEDIASUBTYPE_DOLBY_AC3,			// subtype
	TRUE,							// bFixedSizeSamples
	FALSE,							// bTemporalCompression
	0,								// lSampleSize
	FORMAT_WaveFormatEx,			// formattype
	nullptr,						// pUnk
	sizeof(wf_Audio),				// cbFormat
	(LPBYTE)&wf_Audio,				// pbFormat
};

/// Media type, Audio EAC3
static const AM_MEDIA_TYPE mt_Eac3 = {
	MEDIATYPE_Audio,				// majortype
	MEDIASUBTYPE_DOLBY_DDPLUS,		// subtype
	TRUE,							// bFixedSizeSamples
	FALSE,							// bTemporalCompression
	0,								// lSampleSize
	FORMAT_WaveFormatEx,			// formattype
	nullptr,						// pUnk
	sizeof(wf_Audio),				// cbFormat
	(LPBYTE)&wf_Audio,				// pbFormat
};

/// Media type, PSI
static const AM_MEDIA_TYPE mt_Psi = {
	MEDIATYPE_MPEG2_SECTIONS,		// majortype
	MEDIASUBTYPE_MPEG2DATA,			// subtype
	TRUE,							// bFixedSizeSamples
	FALSE,							// bTemporalCompression
	0,								// lSampleSize
	FORMAT_None,					// formattype
	nullptr,						// pUnk
	0,								// cbFormat
	nullptr							// pbFormat
};

/// Media type, TIF
static const AM_MEDIA_TYPE mt_Tif = {
	MEDIATYPE_MPEG2_SECTIONS,		// majortype
	MEDIASUBTYPE_DVB_SI,			// subtype
	TRUE,							// bFixedSizeSamples
	FALSE,							// bTemporalCompression
	0,								// lSampleSize
	FORMAT_None,					// formattype
	nullptr,						// pUnk
	0,								// cbFormat
	nullptr							// pbFormat
};

/// Media type, EPG
static const AM_MEDIA_TYPE mt_Epg = {
	MEDIATYPE_MPEG2_SECTIONS,		// majortype
	MEDIASUBTYPE_DVB_SI,			// subtype
	TRUE,							// bFixedSizeSamples
	FALSE,							// bTemporalCompression
	0,								// lSampleSize
	FORMAT_None,					// formattype
	nullptr,						// pUnk
	0,								// cbFormat
	nullptr,						// pbFormat
};

/// Media type, PMT
static const AM_MEDIA_TYPE mt_Pmt = {
	MEDIATYPE_MPEG2_SECTIONS,		// majortype
	MEDIASUBTYPE_DVB_SI,			// subtype
	TRUE,							// bFixedSizeSamples
	FALSE,							// bTemporalCompression
	0,								// lSampleSize
	FORMAT_None,					// formattype
	nullptr,						// pUnk
	0,								// cbFormat
	nullptr							// pbFormat
};

static const SUBTITLEINFO SubFormat = { 0, "", L"" };

/// Media type, subtitle
static const AM_MEDIA_TYPE mt_Subtitle = {
	MEDIATYPE_Subtitle,				// majortype
	MEDIASUBTYPE_DVB_SUBTITLES,		// subtype
	FALSE,							// bFixedSizeSamples
	FALSE,							// bTemporalCompression
	0,								// lSampleSize
	FORMAT_None,					// formattype
	nullptr,						// pUnk
	sizeof(SubFormat),				// cbFormat
	(LPBYTE)&SubFormat				// pbFormat
};

/// CLSID for TIF
// FC772AB0-0C7F-11D3-8FF2-00A0C9224CF4
static CLSID CLSID_BDA_MPEG2_TIF =
{0xFC772AB0, 0x0C7F, 0x11D3, {0x8F, 0xF2, 0x00, 0xA0, 0xC9, 0x22, 0x4C, 0xF4}};

CFGManagerBDA::CFGManagerBDA(LPCTSTR pName, LPUNKNOWN pUnk, HWND hWnd)
	: CFGManagerPlayer (pName, pUnk, hWnd)
{
	LOG (L"\nStarting session ------------------------------------------------->");
	m_DVBStreams[DVB_MPV]	= CDVBStream(L"mpv",	&mt_Mpv);
	m_DVBStreams[DVB_H264]	= CDVBStream(L"h264",	&mt_H264);
	m_DVBStreams[DVB_MPA]	= CDVBStream(L"mpa",	&mt_Mpa);
	m_DVBStreams[DVB_AC3]	= CDVBStream(L"ac3",	&mt_Ac3);
	m_DVBStreams[DVB_EAC3]	= CDVBStream(L"eac3",	&mt_Eac3);
	m_DVBStreams[DVB_PSI]	= CDVBStream(L"psi",	&mt_Psi, true, MEDIA_MPEG2_PSI);
	m_DVBStreams[DVB_TIF]	= CDVBStream(L"tif",	&mt_Tif, true);
	m_DVBStreams[DVB_EPG]	= CDVBStream(L"epg",	&mt_Epg);

	m_DVBStreams[DVB_SUB]	= CDVBStream(L"sub",	&mt_Subtitle/*, false, MEDIA_ELEMENTARY_STREAM*/);

	m_nCurVideoType			= DVB_MPV;
	m_nCurAudioType			= DVB_MPA;
	m_fHideWindow = false;

	// Hack : remove audio switcher !
	POSITION pos = m_transform.GetHeadPosition();
	while (pos) {
		CFGFilter* pFGF = m_transform.GetAt(pos);
		if (pFGF->GetCLSID() == __uuidof(CAudioSwitcherFilter)) {
			m_transform.RemoveAt (pos);
			delete pFGF;
			break;
		}
		m_transform.GetNext(pos);
	}
	LOG (L"CFGManagerBDA object created.");
}

CFGManagerBDA::~CFGManagerBDA()
{
	m_DVBStreams.RemoveAll();
	LOG (L"\nCFGManagerBDA object destroyed.");
}

HRESULT CFGManagerBDA::CreateKSFilter(IBaseFilter** ppBF, CLSID KSCategory, CStringW& DisplayName)
{
	HRESULT		hr = VFW_E_NOT_FOUND;
	BeginEnumSysDev (KSCategory, pMoniker) {
		CComPtr<IPropertyBag>	pPB;
		CComVariant				var;
		LPOLESTR				strName = nullptr;
		if (SUCCEEDED (pMoniker->BindToStorage(0, 0, IID_IPropertyBag, (void**)&pPB)) &&
				SUCCEEDED (pMoniker->GetDisplayName(nullptr, nullptr, &strName)) &&
				SUCCEEDED (pPB->Read(CComBSTR(L"FriendlyName"), &var, nullptr)) ) {
			CStringW	Name = CStringW(strName);
			if (Name != DisplayName) {
				continue;
			}

			hr = pMoniker->BindToObject(nullptr, nullptr, IID_IBaseFilter, (void**)ppBF);
			if (SUCCEEDED (hr)) {
				hr = AddFilter (*ppBF, CStringW(var.bstrVal));
			}
			break;
		}

		if (strName) {
			CoTaskMemFree(strName);
		}
	}
	EndEnumSysDev

	return hr;
}

HRESULT CFGManagerBDA::SearchIBDATopology(const CComPtr<IBaseFilter>& pTuner, REFIID iid, CComPtr<IUnknown>& pUnk)
{
	CComQIPtr<IBDA_Topology> pTop(pTuner);
	CheckPointer (pTop, E_NOINTERFACE);

	ULONG	NodeTypes;
	ULONG	NodeType[32];

	HRESULT	hr = pTop->GetNodeTypes(&NodeTypes, _countof(NodeType), NodeType);

	if (FAILED(hr)) {
		return hr;
	}

	for (ULONG i = 0; i < NodeTypes; i++) {
		ULONG	nInterfaces;
		GUID	aInterface[32];

		hr = pTop->GetNodeInterfaces(NodeType[i], &nInterfaces, _countof(aInterface), aInterface);

		if (FAILED(hr)) {
			continue;
		}

		for (ULONG j = 0; j < nInterfaces; j++) {
			if (aInterface[j] == iid) {
				return pTop->GetControlNode(0, 1, NodeType[i], &pUnk);
			}
		}
	}

	return hr;
}

HRESULT CFGManagerBDA::ConnectFilters(IBaseFilter* pOutFilter, IBaseFilter* pInFilter)
{
	HRESULT		hr = VFW_E_CANNOT_CONNECT;
	BeginEnumPins(pOutFilter, pEP, pOutPin) {
		if (S_OK == IsPinDirection(pOutPin, PINDIR_OUTPUT)
				&& S_OK != IsPinConnected(pOutPin)) {
			BeginEnumPins(pInFilter, pEP, pInPin) {
				if (S_OK == IsPinDirection(pInPin, PINDIR_INPUT)
						&& S_OK != IsPinConnected(pInPin)) {
					hr = this->ConnectDirect(pOutPin, pInPin, nullptr);

					/*#ifdef _DEBUG
										PIN_INFO		InfoPinIn, InfoPinOut;
										FILTER_INFO		InfoFilterIn, InfoFilterOut;
										pInPin->QueryPinInfo (&InfoPinIn);
										pOutPin->QueryPinInfo (&InfoPinOut);
										InfoPinIn.pFilter->QueryFilterInfo(&InfoFilterIn);
										InfoPinOut.pFilter->QueryFilterInfo(&InfoFilterOut);

										DLog(L"%s - %s => %s - %s (hr=0x%08x)", InfoFilterOut.achName, InfoPinOut.achName, InfoFilterIn.achName, InfoPinIn.achName, hr);

										InfoPinIn.pFilter->Release();
										InfoPinOut.pFilter->Release();
					#endif*/
					if (SUCCEEDED (hr)) {
						return hr;
					}
				}
			}
			EndEnumPins;
		}
	}
	EndEnumPins;

	return hr;
}

STDMETHODIMP CFGManagerBDA::RenderFile(LPCWSTR lpcwstrFile, LPCWSTR lpcwstrPlayList)
{
	HRESULT						hr;
	CAppSettings&				s = AfxGetAppSettings();
	CComPtr<IBaseFilter>		pNetwork;
	CComPtr<IBaseFilter>		pTuner;
	CComPtr<IBaseFilter>		pReceiver;

	LOG (L"\nCreating BDA filters...");
	CheckAndLog (CreateKSFilter (&pNetwork,		KSCATEGORY_BDA_NETWORK_PROVIDER,	s.strBDANetworkProvider),	"BDA : Network provider creation");
	if (FAILED(hr = CreateKSFilter (&pTuner,	KSCATEGORY_BDA_NETWORK_TUNER,		s.strBDATuner))) {
		MessageBox(AfxGetApp()->GetMainWnd()->m_hWnd, ResStr(IDS_BDA_ERROR_CREATE_TUNER), ResStr(IDS_BDA_ERROR), MB_ICONERROR | MB_OK);
		DLog(L"BDA : Network tuner creation"" :0x%08x",hr);
		return hr;
	}
	if (s.strBDATuner.Right(40) != s.strBDAReceiver.Right(40)) {	// check if filters are the same
		if (FAILED(hr = CreateKSFilter (&pReceiver, KSCATEGORY_BDA_RECEIVER_COMPONENT,	s.strBDAReceiver))) {
			MessageBox(AfxGetApp()->GetMainWnd()->m_hWnd, ResStr(IDS_BDA_ERROR_CREATE_RECEIVER), ResStr(IDS_BDA_ERROR), MB_ICONERROR | MB_OK);
			DLog(L"BDA : Receiver creation"" :0x%08x",hr);
			return hr;
		}
		if (FAILED(hr = ConnectFilters (pNetwork, pTuner))) {
			MessageBox(AfxGetApp()->GetMainWnd()->m_hWnd, ResStr(IDS_BDA_ERROR_CONNECT_NW_TUNER), ResStr(IDS_BDA_ERROR), MB_ICONERROR | MB_OK);
			DLog(L"BDA : Network <-> Tuner"" :0x%08x",hr);
			return hr;
		}
		if (FAILED(hr = ConnectFilters (pTuner, pReceiver))) {
			MessageBox(AfxGetApp()->GetMainWnd()->m_hWnd, ResStr(IDS_BDA_ERROR_CONNECT_TUNER_REC), ResStr(IDS_BDA_ERROR), MB_ICONERROR | MB_OK);
			DLog(L"BDA : Tuner <-> Receiver"" :0x%08x",hr);
			return hr;
		}
		LOG (L"Network -> Tuner -> Receiver connected.");

		CComPtr<IBaseFilter>		pMpeg2Demux;
		m_pBDAControl	= pTuner;
		if (FAILED(hr = SearchIBDATopology (pTuner, m_pBDAFreq))) {
			AfxMessageBox(L"BDA Error: IBDA_FrequencyFilter topology.", MB_OK);
			DLog(L"BDA : IBDA_FrequencyFilter topology"" :0x%08x",hr);
			return hr;
		}
		if (FAILED(hr = SearchIBDATopology (pTuner, m_pBDAStats))) {
			AfxMessageBox(L"BDA Error: IBDA_SignalStatistics topology.", MB_OK);
			DLog(L"BDA : IBDA_SignalStatistics topology"" :0x%08x",hr);
			return hr;
		}

		// Create Mpeg2 demux
		if (FAILED(hr = CreateMicrosoftDemux (pReceiver, pMpeg2Demux))) {
			MessageBox(AfxGetApp()->GetMainWnd()->m_hWnd, ResStr(IDS_BDA_ERROR_DEMULTIPLEXER), ResStr(IDS_BDA_ERROR), MB_ICONERROR | MB_OK);
			DLog(L"BDA : Microsoft demux creation"" :0x%08x",hr);
			return hr;
		}
	} else {	// if same filters, connect pNetwork to pTuner directly
		if (FAILED(hr = ConnectFilters (pNetwork, pTuner))) {
			MessageBox(AfxGetApp()->GetMainWnd()->m_hWnd, ResStr(IDS_BDA_ERROR_CONNECT_TUNER), ResStr(IDS_BDA_ERROR), MB_ICONERROR | MB_OK);
			DLog(L"BDA : Network <-> Tuner/Receiver"" :0x%08x",hr);
			return hr;
		}

		CComPtr<IBaseFilter>		pMpeg2Demux;
		m_pBDAControl	= pTuner;
		if (FAILED(hr = SearchIBDATopology (pTuner, m_pBDAFreq))) {
			AfxMessageBox(L"BDA Error: IBDA_FrequencyFilter topology.", MB_OK);
			DLog(L"BDA : IBDA_FrequencyFilter topology"" :0x%08x",hr);
			return hr;
		}
		if (FAILED(hr = SearchIBDATopology (pTuner, m_pBDAStats))) {
			AfxMessageBox(L"BDA Error: IBDA_SignalStatistics topology.", MB_OK);
			DLog(L"BDA : IBDA_SignalStatistics topology"" :0x%08x",hr);
			return hr;
		}
		LOG (L"Network -> Receiver connected.");

		// Create Mpeg2 demux
		if (FAILED(hr = CreateMicrosoftDemux (pTuner, pMpeg2Demux))) {
			MessageBox(AfxGetApp()->GetMainWnd()->m_hWnd, ResStr(IDS_BDA_ERROR_DEMULTIPLEXER), ResStr(IDS_BDA_ERROR), MB_ICONERROR | MB_OK);
			DLog(L"BDA : Microsoft demux creation"" :0x%08x",hr);
			return hr;
		}
	}

#ifdef _DEBUG
	LOG (L"\nFilter list:");
	BeginEnumFilters(this, pEF, pBF) {
		LOG(GetFilterName(pBF));
	}
	EndEnumFilters;
#endif

	return S_OK;
}

STDMETHODIMP CFGManagerBDA::ConnectDirect(IPin* pPinOut, IPin* pPinIn, const AM_MEDIA_TYPE* pmt)
{
	// Bypass CFGManagerPlayer limitation (IMediaSeeking for Mpeg2 demux)
	return CFGManagerCustom::ConnectDirect (pPinOut, pPinIn, pmt);
}

STDMETHODIMP CFGManagerBDA::SetChannel (int nChannelPrefNumber)
{
	HRESULT			hr		 = E_INVALIDARG;
	CAppSettings&	s		 = AfxGetAppSettings();
	CDVBChannel*	pChannel = s.FindChannelByPref(nChannelPrefNumber);

	if (pChannel != nullptr) {
		hr = SetChannelInternal (pChannel);

		if (SUCCEEDED (hr)) {
			s.nDVBLastChannel = nChannelPrefNumber;
		}
	}

	return hr;
}

STDMETHODIMP CFGManagerBDA::SetAudio (int nAudioIndex)
{
	return E_NOTIMPL;
}

STDMETHODIMP CFGManagerBDA::SetFrequency(ULONG freq)
{
	HRESULT			hr;
	CAppSettings&	s = AfxGetAppSettings();
	CheckPointer (m_pBDAControl, E_FAIL);
	CheckPointer (m_pBDAFreq,	 E_FAIL);

	CheckAndLog (m_pBDAControl->StartChanges(),					"BDA : Setfrequency StartChanges");
	CheckAndLog (m_pBDAFreq->put_Bandwidth(s.iBDABandwidth),		"BDA : Setfrequency put_Bandwidth");
	CheckAndLog (m_pBDAFreq->put_Frequency(freq),				"BDA : Setfrequency put_Frequency");
	CheckAndLog (m_pBDAControl->CheckChanges(),					"BDA : Setfrequency CheckChanges");
	CheckAndLog (m_pBDAControl->CommitChanges(),				"BDA : Setfrequency CommitChanges");

	return hr;
}

STDMETHODIMP CFGManagerBDA::Scan(ULONG ulFrequency, HWND hWnd)
{
	CMpeg2DataParser		Parser (m_DVBStreams[DVB_PSI].GetFilter());

	Parser.ParseSDT(ulFrequency);
	Parser.ParsePAT();
	Parser.ParseNIT();

	POSITION pos = Parser.Channels.GetStartPosition();
	while (pos) {
		CDVBChannel&		Channel = Parser.Channels.GetNextValue(pos);
		if (Channel.HasName()) {
			::SendMessage (hWnd, WM_TUNER_NEW_CHANNEL, 0, (LPARAM)(LPCTSTR)Channel.ToString());
		}
	}

	return S_OK;
}

STDMETHODIMP CFGManagerBDA::GetStats (BOOLEAN& bPresent, BOOLEAN& bLocked, LONG& lStrength, LONG& lQuality)
{
	HRESULT		hr;
	CheckPointer (m_pBDAStats, E_UNEXPECTED);

	CheckNoLog (m_pBDAStats->get_SignalPresent	(&bPresent));
	CheckNoLog (m_pBDAStats->get_SignalLocked	(&bLocked));
	CheckNoLog (m_pBDAStats->get_SignalStrength (&lStrength));
	CheckNoLog (m_pBDAStats->get_SignalQuality  (&lQuality));

	return S_OK;
}

// IAMStreamSelect
STDMETHODIMP CFGManagerBDA::Count(DWORD* pcStreams)
{
	CheckPointer(pcStreams, E_POINTER);
	CAppSettings&	s		 = AfxGetAppSettings();
	CDVBChannel*	pChannel = s.FindChannelByPref(s.nDVBLastChannel);

	*pcStreams = 0;

	if (pChannel != 0) {
		*pcStreams = pChannel->GetAudioCount() + pChannel->GetSubtitleCount();
	}

	return S_OK;
}

STDMETHODIMP CFGManagerBDA::Enable(long lIndex, DWORD dwFlags)
{
	HRESULT			hr				= E_INVALIDARG;
	CAppSettings&	s				= AfxGetAppSettings();
	CDVBChannel*	pChannel		= s.FindChannelByPref(s.nDVBLastChannel);
	DVBStreamInfo*	pStreamInfo		= nullptr;
	CDVBStream*		pStream			= nullptr;
	FILTER_STATE	nState;

	if (pChannel) {
		if (lIndex>=0 && lIndex < pChannel->GetAudioCount()) {
			pStreamInfo	= pChannel->GetAudio(lIndex);
			pStream		= &m_DVBStreams[pStreamInfo->Type];
			if (pStream && pStreamInfo) {
				nState = GetState();
				if (m_nCurAudioType != pStreamInfo->Type) {
					SwitchStream (m_nCurAudioType, pStreamInfo->Type);
				}
				pStream->Map (pStreamInfo->PID);
				ChangeState ((FILTER_STATE)nState);

				hr = S_OK;
			}
		} else if (lIndex > 0 && lIndex < pChannel->GetAudioCount()+pChannel->GetSubtitleCount()) {
			pStreamInfo	= pChannel->GetSubtitle(lIndex-pChannel->GetAudioCount());

			if (pStreamInfo) {
				m_DVBStreams[DVB_SUB].Map(pStreamInfo->PID);
				hr = S_OK;
			}
		}
	}

	return hr;
}

STDMETHODIMP CFGManagerBDA::Info(long lIndex, AM_MEDIA_TYPE** ppmt, DWORD* pdwFlags, LCID* plcid, DWORD* pdwGroup, WCHAR** ppszName, IUnknown** ppObject, IUnknown** ppUnk)
{
	HRESULT				hr				= E_INVALIDARG;
	CAppSettings&		s				= AfxGetAppSettings();
	CDVBChannel*		pChannel		= s.FindChannelByPref(s.nDVBLastChannel);
	DVBStreamInfo*		pStreamInfo		= nullptr;
	CDVBStream*			pStream			= nullptr;
	CDVBStream*			pCurrentStream	= nullptr;

	if (pChannel) {
		if (lIndex>=0 && lIndex < pChannel->GetAudioCount()) {
			pCurrentStream	= &m_DVBStreams[m_nCurAudioType];
			pStreamInfo		= pChannel->GetAudio(lIndex);
			if (pStreamInfo) {
				pStream		= &m_DVBStreams[pStreamInfo->Type];
			}
			if (pdwGroup) {
				*pdwGroup	= 1;    // Audio group
			}
		} else if (lIndex > 0 && lIndex < pChannel->GetAudioCount()+pChannel->GetSubtitleCount()) {
			pCurrentStream	= &m_DVBStreams[DVB_SUB];
			pStreamInfo		= pChannel->GetSubtitle(lIndex-pChannel->GetAudioCount());
			if (pStreamInfo) {
				pStream		= &m_DVBStreams[pStreamInfo->Type];
			}
			if (pdwGroup) {
				*pdwGroup	= 2;    // Subtitle group
			}
		}

		if (pStreamInfo && pStream && pCurrentStream) {
			if (ppmt) {
				*ppmt		= CreateMediaType(pStream->GetMediaType());
			}
			if (pdwFlags) {
				*pdwFlags	= (pCurrentStream->GetMappedPID() == pStreamInfo->PID) ? AMSTREAMSELECTINFO_ENABLED|AMSTREAMSELECTINFO_EXCLUSIVE : 0;
			}
			if (plcid) {
				*plcid		= pStreamInfo->GetLCID();
			}
			if (ppObject) {
				*ppObject	= nullptr;
			}
			if (ppUnk) {
				*ppUnk		= nullptr;
			}
			if (ppszName) {
				CStringW str;

				str = StreamTypeToName(pStreamInfo->PesType);

				*ppszName = (WCHAR*)CoTaskMemAlloc((str.GetLength()+1)*sizeof(WCHAR));
				if (*ppszName == nullptr) {
					return E_OUTOFMEMORY;
				}
				wcscpy_s(*ppszName, str.GetLength()+1, str);
			}

			hr = S_OK;
		}
	}

	return hr;
}

STDMETHODIMP CFGManagerBDA::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
	CheckPointer(ppv, E_POINTER);

	return
		QI(IBDATuner)
		QI(IAMStreamSelect)
		__super::NonDelegatingQueryInterface(riid, ppv);
}

HRESULT CFGManagerBDA::CreateMicrosoftDemux(IBaseFilter* pReceiver, CComPtr<IBaseFilter>& pMpeg2Demux)
{
	CComPtr<IMpeg2Demultiplexer>	pDemux;
	HRESULT							hr;

	CheckNoLog (pMpeg2Demux.CoCreateInstance (CLSID_MPEG2Demultiplexer, nullptr, CLSCTX_INPROC_SERVER));
	CheckNoLog (AddFilter (pMpeg2Demux, L"MPEG-2 Demultiplexer"));
	CheckNoLog (ConnectFilters (pReceiver, pMpeg2Demux));
	CheckNoLog (pMpeg2Demux->QueryInterface(IID_IMpeg2Demultiplexer, (void**)&pDemux));

	// Cleanup unnecessary pins
	//for (int i=0; i<6; i++)
	//{
	//	CStringW	strPin;
	//	strPin.Format(L"%d", i);
	//	pDemux->DeleteOutputPin((LPWSTR)(LPCWSTR)strPin);
	//}

	LOG (L"Receiver -> Demux connected.");

	POSITION	pos = m_DVBStreams.GetStartPosition();
	while (pos) {
		CComPtr<IPin>		pPin;
		DVB_STREAM_TYPE		nType  = m_DVBStreams.GetNextKey(pos);
		CDVBStream&			Stream = m_DVBStreams[nType];

		if (nType != DVB_EPG) {	// Hack: DVB_EPG not required
			if (!Stream.GetFindExisting() ||
					(pPin = FindPin (pMpeg2Demux, PINDIR_OUTPUT, Stream.GetMediaType())) == nullptr) {
				CheckNoLog (pDemux->CreateOutputPin ((AM_MEDIA_TYPE*)Stream.GetMediaType(), Stream.GetName(), &pPin));
			}

			if (nType == m_nCurVideoType || nType == m_nCurAudioType) {
				CheckNoLog (Connect (pPin, nullptr, true));
				Stream.SetPin (pPin);
				LOG (L"Graph completed for stream type %d.",nType);
			} else {
				CheckNoLog (Connect (pPin, nullptr, false));
				Stream.SetPin (pPin);
				LOG (L"Filter connected to Demux for media type %d.",nType);
			}
			if (nType == DVB_H264) {
				m_pPin_h264 = pPin;  // Demux h264 output pin
			}
		}
	}

	return S_OK;
}

HRESULT CFGManagerBDA::SetChannelInternal (CDVBChannel* pChannel)
{
	HRESULT		hr = S_OK;
	bool		fRadioToTV = false;

	int nState = GetState();

	if (pChannel->GetVideoPID() != 0) {
		SwitchStream (m_nCurVideoType, pChannel->GetVideoType());
		if (m_fHideWindow) {
			fRadioToTV = true;
		}
	} else {
		m_fHideWindow = true;
		AfxGetMainFrame()->HideVideoWindow(m_fHideWindow);
	}

	SwitchStream (m_nCurAudioType, pChannel->GetDefaultAudioType());

	// Re-connection needed for H264 to allow different formats of H264 channels
	// Makes possible switching between H264 channels of different resolutions
	// and/or between interlaced and progressive.

	if (m_nCurVideoType == DVB_H264) {
		CComPtr<IPin> pInPin;
		m_pPin_h264->ConnectedTo(&pInPin);
		m_pPin_h264->Disconnect();
		pInPin->Disconnect();
		ConnectDirect(m_pPin_h264, pInPin, nullptr);
	}

	CheckNoLog (SetFrequency (pChannel->GetFrequency()));
	if (pChannel->GetVideoPID() != 0) {
		CheckNoLog (m_DVBStreams[m_nCurVideoType].Map (pChannel->GetVideoPID()));
	}

	CheckNoLog (m_DVBStreams[m_nCurAudioType].Map (pChannel->GetDefaultAudioPID()));

	if (GetState() == State_Stopped) {
		hr = ChangeState ((FILTER_STATE)nState);
	}

	if (fRadioToTV) {
		m_fHideWindow = false;
		Sleep(1800);
		AfxGetMainFrame()->HideVideoWindow(m_fHideWindow);
	}

	// TODO : remove sub later!
	//	CheckNoLog (m_DVBStreams[DVB_SUB].Map (pChannel->GetDefaultSubtitlePID()));

	return hr;
}

HRESULT CFGManagerBDA::SwitchStream (DVB_STREAM_TYPE& nOldType, DVB_STREAM_TYPE nNewType)
{
	if ((nNewType != nOldType) || (nNewType == DVB_H264)) {
		CComPtr<IBaseFilter>		pFGOld   = m_DVBStreams[nOldType].GetFilter();
		CComPtr<IBaseFilter>		pFGNew   = m_DVBStreams[nNewType].GetFilter();
		CComPtr<IPin>				pOldOut  = GetFirstPin (pFGOld, PINDIR_OUTPUT);
		CComPtr<IPin>				pInPin;
		CComPtr<IPin>				pNewOut  = GetFirstPin (pFGNew,  PINDIR_OUTPUT);

		if (GetState() != State_Stopped) {
			ChangeState (State_Stopped);
		}
		pOldOut->ConnectedTo(&pInPin);
		Disconnect (pOldOut);
		Disconnect (pInPin);
		ConnectDirect(pNewOut, pInPin, nullptr);
		nOldType = nNewType;
	}
	return S_OK;
}

HRESULT CFGManagerBDA::UpdatePSI(PresentFollowing &NowNext)
{
	HRESULT				hr		 = S_FALSE;
	CAppSettings&		s		 = AfxGetAppSettings();
	CDVBChannel*		pChannel = s.FindChannelByPref(s.nDVBLastChannel);
	CMpeg2DataParser	Parser (m_DVBStreams[DVB_PSI].GetFilter());

	if (pChannel->GetNowNextFlag()) {
		hr = Parser.ParseEIT(pChannel->GetSID(), NowNext);
	}

	return hr;
}

HRESULT CFGManagerBDA::ChangeState(FILTER_STATE nRequested)
{
	HRESULT hr = S_OK;
	OAFilterState	nState	= nRequested+1;

	CComPtr<IMediaControl>					pMC;
	QueryInterface(__uuidof(IMediaControl), (void**) &pMC);
	pMC->GetState (500, &nState);
	if (nState != nRequested) {
		switch (nRequested) {
			case State_Stopped : {
				if (SUCCEEDED(hr = pMC->Stop())) {
					AfxGetMainFrame()->KillTimersStop();
				}
				LOG (L"IMediaControl stop: %d.",hr);
				return hr;
			}
			case State_Paused : {
				LOG (L"IMediaControl pause.");
				return pMC->Pause();
			}
			case State_Running : {
				int iCount = 0;
				hr = S_FALSE;
				while ((hr == S_FALSE) && (iCount++ < 10)) {
					hr = pMC->Run();
					if (hr == S_FALSE) {
						Sleep(50);
					}
				}
				if (SUCCEEDED(hr)) {
					AfxGetMainFrame()->SetTimersPlay();
				}
				LOG (L"IMediaControl play: %d.",hr);
				return hr;
			}
		}
	}
	return hr;
}

FILTER_STATE CFGManagerBDA::GetState()
{
	CComPtr<IMediaControl>		pMC;
	OAFilterState				nState;
	QueryInterface(__uuidof(IMediaControl), (void**) &pMC);
	pMC->GetState (500, &nState);

	return (FILTER_STATE) nState;
}
