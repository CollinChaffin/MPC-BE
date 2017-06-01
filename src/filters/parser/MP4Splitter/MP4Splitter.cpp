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

#include "stdafx.h"
#include <MMreg.h>
#include "MP4Splitter.h"
#include "../../../DSUtil/GolombBuffer.h"
#include "../../../DSUtil/AudioParser.h"

#ifdef REGISTER_FILTER
#include <InitGuid.h>
#endif
#include <moreuuids.h>
#include <basestruct.h>

#include <Bento4/Core/Ap4.h>
#include <Bento4/Core/Ap4File.h>
#include <Bento4/Core/Ap4SttsAtom.h>
#include <Bento4/Core/Ap4StssAtom.h>
#include <Bento4/Core/Ap4StsdAtom.h>
#include <Bento4/Core/Ap4IsmaCryp.h>
#include <Bento4/Core/Ap4ChplAtom.h>
#include <Bento4/Core/Ap4FtabAtom.h>
#include <Bento4/Core/Ap4DataAtom.h>
#include <Bento4/Core/Ap4PaspAtom.h>
#include <Bento4/Core/Ap4ChapAtom.h>
#include <Bento4/Core/Ap4Dvc1Atom.h>
#include <Bento4/Core/Ap4DataInfoAtom.h>

#include <libavutil/intreadwrite.h>

#ifdef REGISTER_FILTER

const AMOVIESETUP_MEDIATYPE sudPinTypesIn[] = {
	{&MEDIATYPE_Stream, &MEDIASUBTYPE_MP4},
	{&MEDIATYPE_Stream, &MEDIASUBTYPE_NULL},
};

const AMOVIESETUP_PIN sudpPins[] = {
	{L"Input", FALSE, FALSE, FALSE, FALSE, &CLSID_NULL, NULL, _countof(sudPinTypesIn), sudPinTypesIn},
	{L"Output", FALSE, TRUE, FALSE, FALSE, &CLSID_NULL, NULL, 0, NULL}
};

const AMOVIESETUP_FILTER sudFilter[] = {
	{&__uuidof(CMP4SplitterFilter), MP4SplitterName, MERIT_NORMAL, _countof(sudpPins), sudpPins, CLSID_LegacyAmFilterCategory},
	{&__uuidof(CMP4SourceFilter), MP4SourceName, MERIT_NORMAL, 0, NULL, CLSID_LegacyAmFilterCategory},
};

CFactoryTemplate g_Templates[] = {
	{sudFilter[0].strName, sudFilter[0].clsID, CreateInstance<CMP4SplitterFilter>, NULL, &sudFilter[0]},
	{sudFilter[1].strName, sudFilter[1].clsID, CreateInstance<CMP4SourceFilter>, NULL, &sudFilter[1]},
};

int g_cTemplates = _countof(g_Templates);

STDAPI DllRegisterServer()
{
	DeleteRegKey(L"Media Type\\Extensions\\", L".mp4");
	DeleteRegKey(L"Media Type\\Extensions\\", L".mov");

	CAtlList<CString> chkbytes;

	// mov, mp4
	chkbytes.AddTail(L"4,4,,66747970"); // '....ftyp'
	chkbytes.AddTail(L"4,4,,6d6f6f76"); // '....moov'
	chkbytes.AddTail(L"4,4,,6d646174"); // '....mdat'
	chkbytes.AddTail(L"4,4,,77696465"); // '....wide'
	chkbytes.AddTail(L"4,4,,736b6970"); // '....skip'
	chkbytes.AddTail(L"4,4,,66726565"); // '....free'
	chkbytes.AddTail(L"4,4,,706e6f74"); // '....pnot'

	RegisterSourceFilter(CLSID_AsyncReader, MEDIASUBTYPE_MP4, chkbytes, NULL);

	return AMovieDllRegisterServer2(TRUE);
}

STDAPI DllUnregisterServer()
{
	UnRegisterSourceFilter(MEDIASUBTYPE_MP4);

	return AMovieDllRegisterServer2(FALSE);
}

#include "../../filters/Filters.h"

CFilterApp theApp;

#endif

//
// CMP4SplitterFilter
//

CMP4SplitterFilter::CMP4SplitterFilter(LPUNKNOWN pUnk, HRESULT* phr)
	: CBaseSplitterFilter(NAME("CMP4SplitterFilter"), pUnk, phr, __uuidof(this))
{
	m_nFlag |= SOURCE_SUPPORT_URL;
}

CMP4SplitterFilter::~CMP4SplitterFilter()
{
}

STDMETHODIMP CMP4SplitterFilter::QueryFilterInfo(FILTER_INFO* pInfo)
{
	CheckPointer(pInfo, E_POINTER);
	ValidateReadWritePtr(pInfo, sizeof(FILTER_INFO));

	if (m_pName && m_pName[0]==L'M' && m_pName[1]==L'P' && m_pName[2]==L'C') {
		(void)StringCchCopyW(pInfo->achName, NUMELMS(pInfo->achName), m_pName);
	} else {
		wcscpy_s(pInfo->achName, MP4SourceName);
	}
	pInfo->pGraph = m_pGraph;
	if (m_pGraph) {
		m_pGraph->AddRef();
	}

	return S_OK;
}

static void SetTrackName(CString *TrackName, CString Suffix)
{
	if (TrackName->IsEmpty()) {
		*TrackName = Suffix;
	} else {
		*TrackName += L" - ";
		*TrackName += Suffix;
	}
}

#define FormatTrackName(name, append)			\
	if (!tname.IsEmpty() && tname[0] < 0x20) {	\
		tname.Delete(0);						\
	}											\
	if (tname.IsEmpty()) {						\
		tname = name;							\
	} else if (append) {						\
		tname.AppendFormat(L"(%s)", name);		\
	}											\
	SetTrackName(&TrackName, tname);			\

#define HandlePASP(SampleEntry)																			\
	if (AP4_PaspAtom* pasp = dynamic_cast<AP4_PaspAtom*>(SampleEntry->GetChild(AP4_ATOM_TYPE_PASP))) {	\
		if (!Aspect.cx && pasp->GetNum() > 0 && pasp->GetDen() > 0) {									\
			Aspect.SetSize(pasp->GetNum(), pasp->GetDen());												\
		}																								\
	}																									\

static void SetAspect(CSize& Aspect, LONG width, LONG height, LONG codec_width, LONG codec_height, VIDEOINFOHEADER2* vih2)
{
	if (!Aspect.cx || !Aspect.cy) {
		if (width && height) {
			Aspect.SetSize(width, height);
		} else {
			Aspect.SetSize(codec_width, codec_height);
		}
	} else {
		Aspect.cx *= codec_width;
		Aspect.cy *= codec_height;
	}
	ReduceDim(Aspect);

	if (vih2) {
		vih2->dwPictAspectRatioX = Aspect.cx;
		vih2->dwPictAspectRatioY = Aspect.cy;
	}
}

static const CString ConvertStr(const char* S)
{
	CString str = AltUTF8To16(S);
	if (str.IsEmpty()) {
		str = ConvertToUTF16(S, CP_ACP); //Trying Local...
	}

	return str;
}

static const DWORD GetFourcc(AP4_VisualSampleEntry* vse)
{
	DWORD fourcc = -1;

	AP4_Atom::Type type = vse->GetType();
	CString tname = UTF8To16(vse->GetCompressorName());

	switch (type) {
	// RAW video
	case AP4_ATOM_TYPE_RAW:
		fourcc = BI_RGB;
		break;
	case AP4_ATOM_TYPE_2vuy:
	case AP4_ATOM_TYPE_2Vuy:
		fourcc = FCC('UYVY');
		break;
	case AP4_ATOM_TYPE_DVOO:
	case AP4_ATOM_TYPE_yuvs:
		fourcc = FCC('YUY2');
		break;
	case AP4_ATOM_TYPE_v410:
		fourcc = FCC('V410');
		break;
	// DivX 3
	case AP4_ATOM_TYPE_DIV3:
	case AP4_ATOM_TYPE_3IVD:
		fourcc = FCC('DIV3');
		break;
	// H.263
	case AP4_ATOM_TYPE_H263:
		if (tname == L"Sorenson H263") {
			fourcc = FCC('FLV1');
		} else {
			fourcc = FCC('H263');
		}
	break;
	// Motion-JPEG
	case AP4_ATOM_TYPE_MJPG:
	case AP4_ATOM_TYPE_AVDJ: // uncommon fourcc
	case AP4_ATOM_TYPE_DMB1: // uncommon fourcc
		fourcc = FCC('MJPG');
		break;
	// JPEG2000
	case AP4_ATOM_TYPE_MJP2:
	case AP4_ATOM_TYPE_AVj2: // uncommon fourcc
		fourcc = FCC('MJP2');
		break;
	// VC-1
	case AP4_ATOM_TYPE_OVC1:
	case AP4_ATOM_TYPE_VC1:
		fourcc = FCC('WVC1');
	// DV Video (http://msdn.microsoft.com/en-us/library/windows/desktop/dd388646%28v=vs.85%29.aspx)
	case AP4_ATOM_TYPE_DVC:
	case AP4_ATOM_TYPE_DVCP:
		fourcc = FCC('dvsd'); // MEDIASUBTYPE_dvsd (DV Video Decoder, ffdshow, LAV)
		break;
	case AP4_ATOM_TYPE_DVPP:
		fourcc = FCC('dv25'); // MEDIASUBTYPE_dv25 (ffdshow, LAV)
		break;
	case AP4_ATOM_TYPE_DV5N:
	case AP4_ATOM_TYPE_DV5P:
		fourcc = FCC('dv50'); // MEDIASUBTYPE_dv50 (ffdshow, LAV)
		break;
	case AP4_ATOM_TYPE_DVHQ:
	case AP4_ATOM_TYPE_DVH5:
	case AP4_ATOM_TYPE_DVH6:
		fourcc = FCC('CDVH'); // MEDIASUBTYPE_CDVH (LAV)
		break;
	// MagicYUV
	case AP4_ATOM_TYPE_M8RG:
	case AP4_ATOM_TYPE_M8RA:
	case AP4_ATOM_TYPE_M8G0:
	case AP4_ATOM_TYPE_M8Y0:
	case AP4_ATOM_TYPE_M8Y2:
	case AP4_ATOM_TYPE_M8Y4:
	case AP4_ATOM_TYPE_M8YA:
	case AP4_ATOM_TYPE_M0RG:
	case AP4_ATOM_TYPE_M0RA:
	case AP4_ATOM_TYPE_M0R0:
	case AP4_ATOM_TYPE_M0Y2:
		fourcc = FCC('MAGY');
		break;
	case AP4_ATOM_TYPE_VP80:
		fourcc = FCC('VP80');
		break;
	case AP4_ATOM_TYPE_VP90:
		fourcc = FCC('VP90');
		break;
	default:
		fourcc = _byteswap_ulong(type);
		break;
	}

	return fourcc;
}

HRESULT CMP4SplitterFilter::CreateOutputs(IAsyncReader* pAsyncReader)
{
	CheckPointer(pAsyncReader, E_POINTER);

	HRESULT hr = E_FAIL;

	//bool b_HasVideo = false;

	m_trackpos.RemoveAll();

	m_pFile.Free();
	m_pFile.Attach(DNew CMP4SplitterFile(pAsyncReader, hr));
	if (!m_pFile) {
		return E_OUTOFMEMORY;
	}
	if (FAILED(hr)) {
		m_pFile.Free();
		return hr;
	}
	m_pFile->SetBreakHandle(GetRequestHandle());

	m_rtNewStart = m_rtCurrent = 0;
	m_rtNewStop = m_rtStop = m_rtDuration = 0;
	REFERENCE_TIME rtVideoDuration = 0;

	CSize videoSize;

	int nRotation      = 0;
	int ChapterTrackId = INT_MIN;

	if (AP4_Movie* movie = m_pFile->GetMovie()) {
		// looking for main video track (skip tracks with motionless frames)
		AP4_UI32 mainvideoID = 0;
		for (AP4_List<AP4_Track>::Item* item = movie->GetTracks().FirstItem();
				item;
				item = item->GetNext()) {
			AP4_Track* track = item->GetData();

			if (track->GetType() != AP4_Track::TYPE_VIDEO) {
				continue;
			}
			if (!mainvideoID) {
				mainvideoID = track->GetId();
			}
			if (AP4_StssAtom* stss = dynamic_cast<AP4_StssAtom*>(track->GetTrakAtom()->FindChild("mdia/minf/stbl/stss"))) {
				const AP4_Array<AP4_UI32>& entries = stss->GetEntries();
				if (entries.ItemCount() > 0) {
					mainvideoID = track->GetId();
					break;
				}
			}
		}
		// process the tracks
		for (AP4_List<AP4_Track>::Item* item = movie->GetTracks().FirstItem();
				item;
				item = item->GetNext()) {
			AP4_Track* track = item->GetData();

			if (track->GetType() != AP4_Track::TYPE_VIDEO
					&& track->GetType() != AP4_Track::TYPE_AUDIO
					&& track->GetType() != AP4_Track::TYPE_TEXT
					&& track->GetType() != AP4_Track::TYPE_SUBP) {
				continue;
			}

			//if (b_HasVideo && track->GetType() == AP4_Track::TYPE_VIDEO) {
			if (track->GetType() == AP4_Track::TYPE_VIDEO && track->GetId() != mainvideoID) {
				continue;
			}

			AP4_Sample sample;

			if (!AP4_SUCCEEDED(track->GetSample(0, sample)) || sample.GetDescriptionIndex() == 0xFFFFFFFF) {
				continue;
			}

			if (AP4_ChapAtom* chap = dynamic_cast<AP4_ChapAtom*>(track->GetTrakAtom()->FindChild("tref/chap"))) {
				ChapterTrackId = chap->GetChapterTrackId();
			}

			if (ChapterTrackId == track->GetId()) {
				continue;
			}

			if (track->GetType() == AP4_Track::TYPE_VIDEO && !nRotation) {
				if (AP4_TkhdAtom* tkhd = dynamic_cast<AP4_TkhdAtom*>(track->GetTrakAtom()->GetChild(AP4_ATOM_TYPE_TKHD))) {
					nRotation = tkhd->GetRotation();
					if (nRotation) {
						CString prop;
						prop.Format(L"%d", nRotation);
						SetProperty(L"ROTATION", prop);
					}
				}
			}

			CSize Aspect(0, 0);
			AP4_UI32 width = 0;
			AP4_UI32 height = 0;

			CString TrackName = ConvertStr(track->GetTrackName().c_str());
			if (TrackName.GetLength() && TrackName[0] < 0x20) {
				TrackName.Delete(0);
			}
			TrackName.Trim();

			CStringA TrackLanguage = track->GetTrackLanguage().c_str();

			REFERENCE_TIME AvgTimePerFrame = 0;
			if (track->GetType() == AP4_Track::TYPE_VIDEO) {
				if (AP4_TkhdAtom* tkhd = dynamic_cast<AP4_TkhdAtom*>(track->GetTrakAtom()->GetChild(AP4_ATOM_TYPE_TKHD))) {
					width = tkhd->GetWidth() >> 16;
					height = tkhd->GetHeight() >> 16;
					double num = 0;
					double den = 0;
					tkhd->GetAspect(num, den);
					if (num > 0 && den > 0) {
						Aspect = ReduceDim(num/den);
					}
				}

				AvgTimePerFrame = track->GetSampleCount() ? REFERENCE_TIME(track->GetDurationHighPrecision() * 10000.0 / (track->GetSampleCount())) : 0;
				if (AP4_SttsAtom* stts = dynamic_cast<AP4_SttsAtom*>(track->GetTrakAtom()->FindChild("mdia/minf/stbl/stts"))) {
					AP4_Duration totalDuration	= stts->GetTotalDuration();
					AP4_UI32 totalFrames		= stts->GetTotalFrames();
					if (totalDuration && totalFrames) {
						AvgTimePerFrame = 10000000.0 / track->GetMediaTimeScale() * totalDuration / totalFrames;
					}
				}
			}

			CAtlArray<CMediaType> mts;

			CMediaType mt;
			mt.SetSampleSize(1);

			VIDEOINFOHEADER2* vih2	= NULL;
			WAVEFORMATEX* wfe		= NULL;

			AP4_DataBuffer empty;

			if (AP4_SampleDescription* desc = track->GetSampleDescription(sample.GetDescriptionIndex())) {
				AP4_MpegSampleDescription* mpeg_desc = NULL;

				if (desc->GetType() == AP4_SampleDescription::TYPE_MPEG) {
					mpeg_desc = dynamic_cast<AP4_MpegSampleDescription*>(desc);
				} else if (desc->GetType() == AP4_SampleDescription::TYPE_ISMACRYP) {
					AP4_IsmaCrypSampleDescription* isma_desc = dynamic_cast<AP4_IsmaCrypSampleDescription*>(desc);
					mpeg_desc = isma_desc->GetOriginalSampleDescription();
				}

				if (mpeg_desc) {
					CStringW TypeString = CStringW(mpeg_desc->GetObjectTypeString(mpeg_desc->GetObjectTypeId()));
					if ((TypeString.Find(L"UNKNOWN") == -1) && (TypeString.Find(L"INVALID") == -1)) {
						SetTrackName(&TrackName, TypeString);
					}
				}

				if (AP4_MpegVideoSampleDescription* video_desc =
							dynamic_cast<AP4_MpegVideoSampleDescription*>(mpeg_desc)) {
					const AP4_DataBuffer* di = video_desc->GetDecoderInfo();
					if (!di) {
						di = &empty;
					}

					LONG biWidth = (LONG)video_desc->GetWidth();
					LONG biHeight = (LONG)video_desc->GetHeight();

					if (!biWidth || !biHeight) {
						biWidth = width;
						biHeight = height;
					}

					if (!biWidth || !biHeight) {
						continue;
					}

					mt.majortype	= MEDIATYPE_Video;
					mt.formattype	= FORMAT_VideoInfo2;

					vih2						= (VIDEOINFOHEADER2*)mt.AllocFormatBuffer(sizeof(VIDEOINFOHEADER2) + di->GetDataSize());
					memset(vih2, 0, mt.FormatLength());
					vih2->dwBitRate				= video_desc->GetAvgBitrate()/8;
					vih2->bmiHeader.biSize		= sizeof(vih2->bmiHeader);
					vih2->bmiHeader.biWidth		= biWidth;
					vih2->bmiHeader.biHeight	= biHeight;
					vih2->rcSource				= vih2->rcTarget = CRect(0, 0, biWidth, biHeight);
					vih2->AvgTimePerFrame		= AvgTimePerFrame;

					SetAspect(Aspect, width, height, vih2->bmiHeader.biWidth, vih2->bmiHeader.biHeight, vih2);

					memcpy(vih2 + 1, di->GetData(), di->GetDataSize());

					switch (video_desc->GetObjectTypeId()) {
						case AP4_MPEG4_VISUAL_OTI:
							{
								BYTE* data			= (BYTE*)di->GetData();
								size_t size			= (size_t)di->GetDataSize();

								BITMAPINFOHEADER pbmi;
								memset(&pbmi, 0, sizeof(BITMAPINFOHEADER));
								pbmi.biSize			= sizeof(pbmi);
								pbmi.biWidth		= biWidth;
								pbmi.biHeight		= biHeight;
								pbmi.biCompression	= FCC('mp4v');
								pbmi.biPlanes		= 1;
								pbmi.biBitCount		= 24;
								pbmi.biSizeImage	= DIBSIZE(pbmi);

								CreateMPEG2VISimple(&mt, &pbmi, AvgTimePerFrame, Aspect, data, size);
								mt.SetSampleSize(pbmi.biSizeImage);
								mts.Add(mt);

								MPEG2VIDEOINFO* mvih	= (MPEG2VIDEOINFO*)mt.pbFormat;
								mt.subtype				= FOURCCMap(mvih->hdr.bmiHeader.biCompression = 'V4PM');
								mts.Add(mt);
							}
							break;
						case AP4_JPEG_OTI:
							{
								BYTE* data			= (BYTE*)di->GetData();
								size_t size			= (size_t)di->GetDataSize();

								BITMAPINFOHEADER pbmi;
								memset(&pbmi, 0, sizeof(BITMAPINFOHEADER));
								pbmi.biSize			= sizeof(pbmi);
								pbmi.biWidth		= biWidth;
								pbmi.biHeight		= biHeight;
								pbmi.biCompression	= FCC('jpeg');
								pbmi.biPlanes		= 1;
								pbmi.biBitCount		= 24;
								pbmi.biSizeImage	= DIBSIZE(pbmi);

								CreateMPEG2VISimple(&mt, &pbmi, AvgTimePerFrame, Aspect, data, size);
								mt.SetSampleSize(pbmi.biSizeImage);
								mts.Add(mt);
							}
							break;
						case AP4_MPEG2_VISUAL_SIMPLE_OTI:
						case AP4_MPEG2_VISUAL_MAIN_OTI:
						case AP4_MPEG2_VISUAL_SNR_OTI:
						case AP4_MPEG2_VISUAL_SPATIAL_OTI:
						case AP4_MPEG2_VISUAL_HIGH_OTI:
						case AP4_MPEG2_VISUAL_422_OTI:
							mt.subtype = MEDIASUBTYPE_MPEG2_VIDEO;
							{
								m_pFile->Seek(sample.GetOffset());
								CBaseSplitterFileEx::seqhdr h;
								CMediaType mt2;
								if (m_pFile->Read(h, sample.GetSize(), &mt2)) {
									mt = mt2;
								}
							}
							mts.Add(mt);
							break;
						case AP4_MPEG1_VISUAL_OTI:
							mt.subtype = MEDIASUBTYPE_MPEG1Payload;
							{
								m_pFile->Seek(sample.GetOffset());
								CBaseSplitterFileEx::seqhdr h;
								CMediaType mt2;
								if (m_pFile->Read(h, sample.GetSize(), &mt2)) {
									mt = mt2;
								}
							}
							mts.Add(mt);
							break;
					}

					if (mt.subtype == GUID_NULL) {
						DLog(L"Unknown video OBI: %02x", video_desc->GetObjectTypeId());
					}
				} else if (AP4_MpegAudioSampleDescription* audio_desc =
							   dynamic_cast<AP4_MpegAudioSampleDescription*>(mpeg_desc)) {
					const AP4_DataBuffer* di = audio_desc->GetDecoderInfo();
					if (!di) {
						di = &empty;
					}

					mt.majortype	= MEDIATYPE_Audio;
					mt.formattype	= FORMAT_WaveFormatEx;

					wfe = (WAVEFORMATEX*)mt.AllocFormatBuffer(sizeof(WAVEFORMATEX) + di->GetDataSize());
					memset(wfe, 0, mt.FormatLength());
					wfe->nSamplesPerSec		= audio_desc->GetSampleRate();
					if (!wfe->nSamplesPerSec) {
						wfe->nSamplesPerSec	= track->GetMediaTimeScale();
					}
					wfe->nAvgBytesPerSec	= audio_desc->GetAvgBitrate()/8;
					wfe->nChannels			= audio_desc->GetChannelCount();
					wfe->wBitsPerSample		= audio_desc->GetSampleSize();
					wfe->cbSize				= (WORD)di->GetDataSize();
					wfe->nBlockAlign		= (WORD)((wfe->nChannels * wfe->wBitsPerSample) / 8);

					memcpy(wfe + 1, di->GetData(), di->GetDataSize());

					AP4_UI08 Mpeg4AudioObjectType = 0;

					switch (audio_desc->GetObjectTypeId()) {
						case AP4_MPEG4_AUDIO_OTI:
							if (di->GetDataSize() >= 1) {
								Mpeg4AudioObjectType = di->GetData()[0] >> 3;
								if (Mpeg4AudioObjectType == 31) {
									if (di->GetDataSize() < 2) {
										Mpeg4AudioObjectType = 0;
									} else {
										Mpeg4AudioObjectType = 32 + (((di->GetData()[0] & 0x07) << 3) |
																	 ((di->GetData()[1] & 0xE0) >> 5));
									}
								}
							}
						case AP4_MPEG2_AAC_AUDIO_MAIN_OTI:
						case AP4_MPEG2_AAC_AUDIO_LC_OTI:
						case AP4_MPEG2_AAC_AUDIO_SSRP_OTI:
							if (Mpeg4AudioObjectType == 36) { // ALS Lossless Coding
								wfe->wFormatTag = WAVE_FORMAT_UNKNOWN;
								mt.subtype = MEDIASUBTYPE_ALS;
								mts.Add(mt);
								break;
							}

							mt.subtype = FOURCCMap(wfe->wFormatTag = WAVE_FORMAT_RAW_AAC1);
							if (wfe->cbSize >= 2) {
								WORD Channels = (((BYTE*)(wfe+1))[1]>>3) & 0xf;
								if (Channels) {
									wfe->nChannels   = Channels;
									wfe->nBlockAlign = (WORD)((wfe->nChannels * wfe->wBitsPerSample) / 8);
								}
							}
							mts.Add(mt);
							break;
						case AP4_MPEG2_PART3_AUDIO_OTI:
						case AP4_MPEG1_AUDIO_OTI:
							mt.subtype = FOURCCMap(wfe->wFormatTag = WAVE_FORMAT_MPEGLAYER3);
							{
								m_pFile->Seek(sample.GetOffset());
								CBaseSplitterFileEx::mpahdr h;
								CMediaType mt2;
								if (m_pFile->Read(h, sample.GetSize(), &mt2)) {
									mt = mt2;
								}
							}
							mts.Add(mt);
							break;
						case AP4_DTSC_AUDIO_OTI:
						case AP4_DTSH_AUDIO_OTI:
						case AP4_DTSL_AUDIO_OTI:
							mt.subtype = FOURCCMap(wfe->wFormatTag = WAVE_FORMAT_DTS2);
							{
								m_pFile->Seek(sample.GetOffset());
								CBaseSplitterFileEx::dtshdr h;
								CMediaType mt2;
								if (m_pFile->Read(h, sample.GetSize(), &mt2, false)) {
									mt = mt2;

									AP4_DataBuffer data;
									if (AP4_SUCCEEDED(sample.ReadData(data))) {
										const BYTE* start = data.GetData();
										const BYTE* end = start + data.GetDataSize();
										int size = ParseDTSHeader(start);
										if (size) {
											if (start + size + 40 <= end) {
												audioframe_t aframe;
												int sizehd = ParseDTSHDHeader(start + size, end - start - size, &aframe);
												if (sizehd) {
													WAVEFORMATEX* wfe = (WAVEFORMATEX*)mt.pbFormat;
													wfe->nSamplesPerSec = aframe.samplerate;
													wfe->nChannels = aframe.channels;
													if (aframe.param1) {
														wfe->wBitsPerSample = aframe.param1;
													}
													wfe->nBlockAlign = (wfe->nChannels * wfe->wBitsPerSample) / 8;
													if (aframe.param2 == DCA_PROFILE_HD_HRA) {
														wfe->nAvgBytesPerSec += CalcBitrate(aframe) / 8;
													} else {
														wfe->nAvgBytesPerSec = 0;
													}
												}
											}
										}
									}
								}
							}
							mts.Add(mt);
							break;
						case AP4_VORBIS_OTI:
							CreateVorbisMediaType(mt, mts,
												  (DWORD)audio_desc->GetChannelCount(),
												  (DWORD)(audio_desc->GetSampleRate() ? audio_desc->GetSampleRate() : track->GetMediaTimeScale()),
												  (DWORD)audio_desc->GetSampleSize(), di->GetData(), di->GetDataSize());
							break;
					}

					if (mt.subtype == GUID_NULL) {
						DLog(L"Unknown audio OBI: %02x", audio_desc->GetObjectTypeId());
					}
				} else if (AP4_MpegSystemSampleDescription* system_desc =
							   dynamic_cast<AP4_MpegSystemSampleDescription*>(desc)) {
					const AP4_DataBuffer* di = system_desc->GetDecoderInfo();
					if (!di) {
						di = &empty;
					}

					switch (system_desc->GetObjectTypeId()) {
						case AP4_NERO_VOBSUB:
							if (di->GetDataSize() >= 16 * 4) {
								CSize size(videoSize);
								if (AP4_TkhdAtom* tkhd = dynamic_cast<AP4_TkhdAtom*>(track->GetTrakAtom()->GetChild(AP4_ATOM_TYPE_TKHD))) {
									if (tkhd->GetWidth() && tkhd->GetHeight()) {
										size.cx = tkhd->GetWidth() >> 16;
										size.cy = tkhd->GetHeight() >> 16;
									}
								}

								if (size == CSize(0, 0)) {
									size = CSize(720, 576);
								}

								const AP4_Byte* pal = di->GetData();
								CAtlList<CStringA> sl;
								for (int i = 0; i < 16 * 4; i += 4) {
									BYTE y = (pal[i + 1] - 16) * 255 / 219;
									BYTE u = pal[i + 2];
									BYTE v = pal[i + 3];
									BYTE r = (BYTE)clamp(1.0 * y + 1.4022 * (v - 128), 0.0, 255.0);
									BYTE g = (BYTE)clamp(1.0 * y - 0.3456 * (u - 128) - 0.7145 * (v - 128), 0.0, 255.0);
									BYTE b = (BYTE)clamp(1.0 * y + 1.7710 * (u - 128), 0.0, 255.0);
									CStringA str;
									str.Format("%02x%02x%02x", r, g, b);
									sl.AddTail(str);
								}

								CStringA hdr;
								hdr.Format(
									"# VobSub index file, v7 (do not modify this line!)\n"
									"size: %dx%d\n"
									"palette: %s\n",
									size.cx, size.cy,
									Implode(sl, ','));

								mt.majortype	= MEDIATYPE_Subtitle;
								mt.subtype		= MEDIASUBTYPE_VOBSUB;
								mt.formattype	= FORMAT_SubtitleInfo;

								SUBTITLEINFO* psi	= (SUBTITLEINFO*)mt.AllocFormatBuffer(sizeof(SUBTITLEINFO) + hdr.GetLength());
								memset(psi, 0, mt.FormatLength());
								psi->dwOffset		= sizeof(SUBTITLEINFO);
								strncpy_s(psi->IsoLang, TrackLanguage, _countof(psi->IsoLang) - 1);
								wcsncpy_s(psi->TrackName, TrackName, _countof(psi->TrackName) - 1);
								memcpy(psi + 1, (LPCSTR)hdr, hdr.GetLength());
								mts.Add(mt);
							}
							break;
					}

					if (mt.subtype == GUID_NULL) {
						DLog(L"Unknown audio OBI: %02x", system_desc->GetObjectTypeId());
					}
				} else if (AP4_UnknownSampleDescription* unknown_desc =
							   dynamic_cast<AP4_UnknownSampleDescription*>(desc)) { // TEMP
					AP4_SampleEntry* sample_entry = unknown_desc->GetSampleEntry();

					if (dynamic_cast<AP4_TextSampleEntry*>(sample_entry)
							|| dynamic_cast<AP4_Tx3gSampleEntry*>(sample_entry)) {
						mt.majortype = MEDIATYPE_Subtitle;
						mt.subtype = MEDIASUBTYPE_UTF8;
						mt.formattype = FORMAT_SubtitleInfo;
						SUBTITLEINFO* si = (SUBTITLEINFO*)mt.AllocFormatBuffer(sizeof(SUBTITLEINFO));
						memset(si, 0, mt.FormatLength());
						si->dwOffset = sizeof(SUBTITLEINFO);
						strcpy_s(si->IsoLang, _countof(si->IsoLang), CStringA(TrackLanguage));
						if (AP4_Tx3gSampleEntry* tx3g = dynamic_cast<AP4_Tx3gSampleEntry*>(sample_entry)) {
							const AP4_Tx3gSampleEntry::AP4_Tx3gDescription& description = tx3g->GetDescription();
							if (description.DisplayFlags & 0x80000000) {
								TrackName += L" [Forced]";
							}
						}
						wcscpy_s(si->TrackName, _countof(si->TrackName), TrackName);
						mts.Add(mt);
					}
				}
			} else if (AP4_StsdAtom* stsd = dynamic_cast<AP4_StsdAtom*>(track->GetTrakAtom()->FindChild("mdia/minf/stbl/stsd"))) {
				const AP4_DataBuffer& db = stsd->GetDataBuffer();

				int k = 0;
				for (AP4_List<AP4_Atom>::Item* items = stsd->GetChildren().FirstItem();
						items;
						items = items->GetNext(), ++k) {

					AP4_Atom* atom = items->GetData();
					AP4_Atom::Type type = atom->GetType();

					if (k == 0 && stsd->GetChildren().ItemCount() > 1 && type == AP4_ATOM_TYPE_JPEG) {
						continue; // Multiple fourcc, we skip first JPEG.
					}

					if (AP4_VisualSampleEntry* vse = dynamic_cast<AP4_VisualSampleEntry*>(atom)) {
						const DWORD fourcc = GetFourcc(vse);
						AP4_DataBuffer* pData = (AP4_DataBuffer*)&db;

						char buff[5] = { 0 };
						memcpy(buff, &fourcc, 4);
						CString tname = UTF8To16(vse->GetCompressorName());

						if ((buff[0] == 'x' || buff[0] == 'h') && buff[1] == 'd') {
							// Apple HDV/XDCAM
							FormatTrackName(L"HDV/XDV MPEG2", 0);

							m_pFile->Seek(sample.GetOffset());
							CBaseSplitterFileEx::seqhdr h;
							CMediaType mt2;
							if (m_pFile->Read(h, sample.GetSize(), &mt2)) {
								mt = mt2;
							}
							mts.Add(mt);
							break;
						}

						if (type == AP4_ATOM_TYPE_MJPA || type == AP4_ATOM_TYPE_MJPB || fourcc == FCC('MJPG')) {
							FormatTrackName(L"M-Jpeg", 1);
						} else if (type == AP4_ATOM_TYPE_MJP2) {
							FormatTrackName(L"M-Jpeg 2000", 1);
						} else if (type == AP4_ATOM_TYPE_APCN ||
								   type == AP4_ATOM_TYPE_APCH ||
								   type == AP4_ATOM_TYPE_APCO ||
								   type == AP4_ATOM_TYPE_APCS ||
								   type == AP4_ATOM_TYPE_AP4H ||
								   type == AP4_ATOM_TYPE_AP4X) {
							FormatTrackName(L"Apple ProRes", 0);
						} else if (type == AP4_ATOM_TYPE_SVQ1 ||
								   type == AP4_ATOM_TYPE_SVQ2 ||
								   type == AP4_ATOM_TYPE_SVQ3) {
							FormatTrackName(L"Sorenson", 0);
						} else if (type == AP4_ATOM_TYPE_CVID) {
							FormatTrackName(L"Cinepack", 0);
						} else if (type == AP4_ATOM_TYPE_VP80) {
							FormatTrackName(L"VP8", 0);
						} else if (type == AP4_ATOM_TYPE_VP90) {
							FormatTrackName(L"VP9", 0);
						} else if (fourcc == FCC('WVC1')) {
							FormatTrackName(L"VC-1", 0);
							if (AP4_Dvc1Atom* Dvc1 = dynamic_cast<AP4_Dvc1Atom*>(vse->GetChild(AP4_ATOM_TYPE_DVC1))) {
								pData = (AP4_DataBuffer*)Dvc1->GetDecoderInfo();
							}
						}

						AP4_Size ExtraSize = pData->GetDataSize();
						if (type == AP4_ATOM_TYPE_FFV1
								|| type == AP4_ATOM_TYPE_H263) {
							ExtraSize = 0;
						}

						BOOL bVC1ExtraData = FALSE;
						if (type == AP4_ATOM_TYPE_OVC1 && ExtraSize > 198) {
							ExtraSize -= 198;
							bVC1ExtraData = TRUE;
						}

						mt.majortype	= MEDIATYPE_Video;
						mt.formattype	= FORMAT_VideoInfo2;

						vih2							= (VIDEOINFOHEADER2*)mt.AllocFormatBuffer(sizeof(VIDEOINFOHEADER2) + ExtraSize);
						memset(vih2, 0, mt.FormatLength());
						vih2->bmiHeader.biSize			= sizeof(vih2->bmiHeader);
						vih2->bmiHeader.biWidth			= (LONG)vse->GetWidth();
						vih2->bmiHeader.biHeight		= (LONG)vse->GetHeight();
						vih2->bmiHeader.biCompression	= fourcc;
						vih2->bmiHeader.biBitCount		= (LONG)vse->GetDepth();
						vih2->bmiHeader.biSizeImage		= DIBSIZE(vih2->bmiHeader);
						vih2->rcSource					= vih2->rcTarget = CRect(0, 0, vih2->bmiHeader.biWidth, vih2->bmiHeader.biHeight);
						vih2->AvgTimePerFrame			= AvgTimePerFrame;

						HandlePASP(vse);
						SetAspect(Aspect, width, height, vih2->bmiHeader.biWidth, vih2->bmiHeader.biHeight, vih2);

						mt.SetSampleSize(vih2->bmiHeader.biSizeImage);

						if (ExtraSize) {
							if (bVC1ExtraData) {
								memcpy(vih2 + 1, pData->GetData() + 198, ExtraSize);
							} else {
								memcpy(vih2 + 1, pData->GetData(), ExtraSize);
							}
						}

						if (fourcc == BI_RGB) {
							WORD &bitcount = vih2->bmiHeader.biBitCount;
							if (bitcount == 16) {
								mt.subtype = MEDIASUBTYPE_RGB555;
							} else if (bitcount == 24) {
								mt.subtype = MEDIASUBTYPE_RGB24;
							} else if (bitcount == 32) {
								mt.subtype = MEDIASUBTYPE_ARGB32;
							} else {
								break; // incorect or unsuported
							}
							mts.Add(mt);
							break;
						}

						mt.subtype = FOURCCMap(fourcc);
						mts.Add(mt);

						if (fourcc == FCC('yuv2')
								|| fourcc == FCC('b16g')
								|| fourcc == FCC('b48r')
								|| fourcc == FCC('b64a')
							) {
							mt.subtype = MEDIASUBTYPE_LAV_RAWVIDEO;
							mts.Add(mt);
							break;
						}

						if (mt.subtype == MEDIASUBTYPE_WVC1) {
							mt.subtype = MEDIASUBTYPE_WVC1_CYBERLINK;
							mts.InsertAt(0, mt);
						}

						_strlwr_s(buff);
						DWORD typelwr = GETDWORD(buff);
						if (typelwr != fourcc) {
							mt.subtype = FOURCCMap(vih2->bmiHeader.biCompression = typelwr);
							mts.Add(mt);
							//b_HasVideo = true;
						}

						_strupr_s(buff);
						DWORD typeupr = GETDWORD(buff);
						if (typeupr != fourcc) {
							mt.subtype = FOURCCMap(vih2->bmiHeader.biCompression = typeupr);
							mts.Add(mt);
							//b_HasVideo = true;
						}

						if (vse->m_hasPalette) {
							track->SetPalette(vse->GetPalette());
						}

						switch (type) {
							case AP4_ATOM_TYPE_AVC1:
							case AP4_ATOM_TYPE_AVC2:
							case AP4_ATOM_TYPE_AVC3:
							case AP4_ATOM_TYPE_AVC4:
							case AP4_ATOM_TYPE_DVAV:
							case AP4_ATOM_TYPE_DVA1:
							case AP4_ATOM_TYPE_AVLG:
							case AP4_ATOM_TYPE_XALG:
								{
									SetTrackName(&TrackName, L"MPEG4 Video (H264)");

									const AP4_DataBuffer* di = NULL;
									if (AP4_DataInfoAtom* avcC = dynamic_cast<AP4_DataInfoAtom*>(vse->GetChild(AP4_ATOM_TYPE_AVCC))) {
										di = avcC->GetData();
									}
									if (!di) {
										di = &empty;
									}

									BYTE* data         = (BYTE*)di->GetData();
									size_t size        = (size_t)di->GetDataSize();

									BITMAPINFOHEADER pbmi;
									memset(&pbmi, 0, sizeof(BITMAPINFOHEADER));
									pbmi.biSize        = sizeof(pbmi);
									pbmi.biWidth       = (LONG)vse->GetWidth();
									pbmi.biHeight      = (LONG)vse->GetHeight();
									pbmi.biCompression = FCC('AVC1');
									pbmi.biPlanes      = 1;
									pbmi.biBitCount    = 24;
									pbmi.biSizeImage   = DIBSIZE(pbmi);

									CreateMPEG2VIfromAVC(&mt, &pbmi, AvgTimePerFrame, Aspect, data, size);
									mt.SetSampleSize(pbmi.biSizeImage);

									mts.RemoveAll();
									mts.Add(mt);

									if (!di->GetDataSize()) {
										m_pFile->Seek(sample.GetOffset());
										CBaseSplitterFileEx::avchdr h;
										CMediaType mt2;
										if (m_pFile->Read(h, sample.GetSize(), &mt2)) {
											mts.InsertAt(0, mt2);
										}
									}
								}
								break;
							case AP4_ATOM_TYPE_HVC1:
							case AP4_ATOM_TYPE_HEV1:
							case AP4_ATOM_TYPE_DVHE:
							case AP4_ATOM_TYPE_DVH1:
								{
									SetTrackName(&TrackName, L"HEVC Video (H.265)");

									const AP4_DataBuffer* di = NULL;
									if (AP4_DataInfoAtom* hvcC = dynamic_cast<AP4_DataInfoAtom*>(vse->GetChild(AP4_ATOM_TYPE_HVCC))) {
										di = hvcC->GetData();
									}
									if (!di) {
										di = &empty;
									}

									BYTE* data         = (BYTE*)di->GetData();
									size_t size        = (size_t)di->GetDataSize();

									BITMAPINFOHEADER pbmi;
									memset(&pbmi, 0, sizeof(BITMAPINFOHEADER));
									pbmi.biSize        = sizeof(pbmi);
									pbmi.biWidth       = (LONG)vse->GetWidth();
									pbmi.biHeight      = (LONG)vse->GetHeight();
									pbmi.biCompression = FCC('HVC1');
									pbmi.biPlanes      = 1;
									pbmi.biBitCount    = 24;
									pbmi.biSizeImage   = DIBSIZE(pbmi);

									vc_params_t params = { 0 };
									HEVCParser::ParseHEVCDecoderConfigurationRecord(data, size, params, false);

									CreateMPEG2VISimple(&mt, &pbmi, AvgTimePerFrame, Aspect, data, size, params.profile, params.level, params.nal_length_size);
									mt.SetSampleSize(pbmi.biSizeImage);

									mts.RemoveAll();
									mts.Add(mt);

									if (!di->GetDataSize()) {
										m_pFile->Seek(sample.GetOffset());
										CBaseSplitterFileEx::hevchdr h;
										CMediaType mt2;
										if (m_pFile->Read(h, sample.GetSize(), &mt2)) {
											mts.InsertAt(0, mt2);
										}
									}
								}
								break;
							case AP4_ATOM_TYPE_VP90:
								if (AP4_DataInfoAtom* vpcC = dynamic_cast<AP4_DataInfoAtom*>(vse->GetChild(AP4_ATOM_TYPE_VPCC))) {
									const AP4_DataBuffer* di = vpcC->GetData();
									if (di->GetDataSize() >= 12) {
										vih2 = (VIDEOINFOHEADER2*)mt.ReallocFormatBuffer(sizeof(VIDEOINFOHEADER2) + di->GetDataSize() + 4);
										BYTE *extra = (BYTE*)(vih2 + 1);
										memcpy(extra, "vpcC", 4);
										memcpy(extra + 4, di->GetData(), di->GetDataSize());
										
										mts.RemoveAll();

										mt.subtype = FOURCCMap(vih2->bmiHeader.biCompression = fourcc);
										mts.Add(mt);
									}
								}
								break;
						}
					} else if (AP4_AudioSampleEntry* ase = dynamic_cast<AP4_AudioSampleEntry*>(atom)) {
						DWORD fourcc        = _byteswap_ulong(ase->GetType());
						DWORD samplerate    = ase->GetSampleRate();
						WORD  channels      = ase->GetChannelCount();
						WORD  bitspersample = ase->GetSampleSize();

						// overwrite audio fourc
						if ((type & 0xffff0000) == AP4_ATOM_TYPE('m', 's', 0, 0)) {
							fourcc = type & 0xffff;
						} else if (type == AP4_ATOM_TYPE_ALAW) {
							fourcc = WAVE_FORMAT_ALAW;
							SetTrackName(&TrackName, L"PCM A-law");
						} else if (type == AP4_ATOM_TYPE_ULAW) {
							fourcc = WAVE_FORMAT_MULAW;
							SetTrackName(&TrackName, L"PCM mu-law");
						} else if (type == AP4_ATOM_TYPE__MP3) {
							SetTrackName(&TrackName, L"MPEG Audio (MP3)");
							fourcc = WAVE_FORMAT_MPEGLAYER3;
						} else if ((type == AP4_ATOM_TYPE__AC3) || (type == AP4_ATOM_TYPE_SAC3) || (type == AP4_ATOM_TYPE_EAC3)) {
							if (type == AP4_ATOM_TYPE_EAC3) {
								SetTrackName(&TrackName, L"Enhanced AC-3 audio");
								fourcc = WAVE_FORMAT_UNKNOWN;
							} else {
								fourcc = WAVE_FORMAT_DOLBY_AC3;
								SetTrackName(&TrackName, L"AC-3 Audio");
							}
						} else if (type == AP4_ATOM_TYPE_MP4A) {
							fourcc = WAVE_FORMAT_RAW_AAC1;
							SetTrackName(&TrackName, L"MPEG-2 Audio AAC");
						} else if (type == AP4_ATOM_TYPE_NMOS) {
							fourcc = MAKEFOURCC('N','E','L','L');
							SetTrackName(&TrackName, L"NellyMoser Audio");
						} else if (type == AP4_ATOM_TYPE_ALAC) {
							fourcc = MAKEFOURCC('a','l','a','c');
							SetTrackName(&TrackName, L"ALAC Audio");
						} else if (type == AP4_ATOM_TYPE_QDM2) {
							SetTrackName(&TrackName, L"QDesign Music 2");
						} else if (type == AP4_ATOM_TYPE_SPEX) {
							fourcc = 0xa109;
							SetTrackName(&TrackName, L"Speex");
						} else if ((type == AP4_ATOM_TYPE_NONE || type == AP4_ATOM_TYPE_RAW) && bitspersample == 8 ||
								    type == AP4_ATOM_TYPE_SOWT && bitspersample == 16 ||
								   (type == AP4_ATOM_TYPE_IN24 || type == AP4_ATOM_TYPE_IN32) && ase->GetEndian()==ENDIAN_LITTLE) {
							fourcc = type = WAVE_FORMAT_PCM;
						} else if ((type == AP4_ATOM_TYPE_FL32 || type == AP4_ATOM_TYPE_FL64) && ase->GetEndian()==ENDIAN_LITTLE) {
							fourcc = type = WAVE_FORMAT_IEEE_FLOAT;
						} else if (type == AP4_ATOM_TYPE_LPCM) {
							SetTrackName(&TrackName, L"LPCM");
							DWORD flags = ase->GetFormatSpecificFlags();
							if (flags & 2) { // big endian
								if (flags & 1) { // floating point
									if      (bitspersample == 32) type = AP4_ATOM_TYPE_FL32;
									else if (bitspersample == 64) type = AP4_ATOM_TYPE_FL64;
								} else {
									if      (bitspersample == 16) type = AP4_ATOM_TYPE_TWOS;
									else if (bitspersample == 24) type = AP4_ATOM_TYPE_IN24;
									else if (bitspersample == 32) type = AP4_ATOM_TYPE_IN32;
								}
								fourcc = ((type >> 24) & 0x000000ff) |
										 ((type >>  8) & 0x0000ff00) |
										 ((type <<  8) & 0x00ff0000) |
										 ((type << 24) & 0xff000000);
							} else {         // little endian
								if (flags & 1) { // floating point
									fourcc = type = WAVE_FORMAT_IEEE_FLOAT;
								} else {
									fourcc = type = WAVE_FORMAT_PCM;
								}
							}
						} else if (type == AP4_ATOM_TYPE_FLAC) {
							SetTrackName(&TrackName, L"FLAC");
							fourcc = WAVE_FORMAT_FLAC;
						} else if (type == AP4_ATOM_TYPE_DTSC || type == AP4_ATOM_TYPE_DTSE
								|| type == AP4_ATOM_TYPE_DTSH || type == AP4_ATOM_TYPE_DTSL) {
							fourcc = type = WAVE_FORMAT_DTS2;
						}

						if (type == AP4_ATOM_TYPE_NONE ||
							type == AP4_ATOM_TYPE_TWOS ||
							type == AP4_ATOM_TYPE_SOWT ||
							type == AP4_ATOM_TYPE_IN24 ||
							type == AP4_ATOM_TYPE_IN32 ||
							type == AP4_ATOM_TYPE_FL32 ||
							type == AP4_ATOM_TYPE_FL64 ||
							type == WAVE_FORMAT_PCM    ||
							type == WAVE_FORMAT_IEEE_FLOAT) {
							SetTrackName(&TrackName, L"PCM");
						}

						mt.majortype = MEDIATYPE_Audio;
						mt.formattype = FORMAT_WaveFormatEx;
						wfe = (WAVEFORMATEX*)mt.AllocFormatBuffer(sizeof(WAVEFORMATEX));
						memset(wfe, 0, mt.FormatLength());
						if (!(fourcc & 0xffff0000)) {
							wfe->wFormatTag = (WORD)fourcc;
						}
						wfe->nSamplesPerSec		= samplerate;
						wfe->nChannels			= channels;
						wfe->wBitsPerSample		= bitspersample;
						wfe->nBlockAlign		= ase->GetBytesPerFrame();
						wfe->nAvgBytesPerSec	= ase->GetSamplesPerPacket() ? wfe->nSamplesPerSec * wfe->nBlockAlign / ase->GetSamplesPerPacket() : 0;

						mt.subtype = FOURCCMap(fourcc);
						if (type == AP4_ATOM_TYPE_EAC3) {
							mt.subtype = MEDIASUBTYPE_DOLBY_DDPLUS;
						}

						if (type == AP4_ATOM_TYPE('m', 's', 0x00, 0x02)) {
							const WORD numcoef = 7;
							static ADPCMCOEFSET coef[] = { {256, 0}, {512, -256}, {0,0}, {192,64}, {240,0}, {460, -208}, {392,-232} };
							const ULONG size = sizeof(ADPCMWAVEFORMAT) + (numcoef * sizeof(ADPCMCOEFSET));
							ADPCMWAVEFORMAT* format = (ADPCMWAVEFORMAT*)mt.ReallocFormatBuffer(size);
							if (format != NULL) {
								format->wfx.wFormatTag = WAVE_FORMAT_ADPCM;
								format->wfx.wBitsPerSample = 4;
								format->wfx.cbSize = (WORD)(size - sizeof(WAVEFORMATEX));
								format->wSamplesPerBlock = format->wfx.nBlockAlign * 2 / format->wfx.nChannels - 12;
								format->wNumCoef = numcoef;
								memcpy( format->aCoef, coef, sizeof(coef) );
							}
						} else if (type == AP4_ATOM_TYPE('m', 's', 0x00, 0x11)) {
							IMAADPCMWAVEFORMAT* format = (IMAADPCMWAVEFORMAT*)mt.ReallocFormatBuffer(sizeof(IMAADPCMWAVEFORMAT));
							if (format != NULL) {
								format->wfx.wFormatTag = WAVE_FORMAT_IMA_ADPCM;
								format->wfx.wBitsPerSample = 4;
								format->wfx.cbSize = (WORD)(sizeof(IMAADPCMWAVEFORMAT) - sizeof(WAVEFORMATEX));
								int X = (format->wfx.nBlockAlign - (4 * format->wfx.nChannels)) * 8;
								int Y = format->wfx.wBitsPerSample * format->wfx.nChannels;
								format->wSamplesPerBlock = (X / Y) + 1;
							}
						} else if (type == AP4_ATOM_TYPE_ALAC) {
							const AP4_Byte* data = db.GetData();
							AP4_Size size = db.GetDataSize();

							while (size >= 36) {
								if ((GETDWORD(data) == 0x24000000) && (GETDWORD(data+4) == 0x63616c61)) {
									break;
								}
								size--;
								data++;
							}

							if (size >= 36) {
								wfe = (WAVEFORMATEX*)mt.ReallocFormatBuffer(sizeof(WAVEFORMATEX) + 36);
								wfe->cbSize = 36;
								memcpy(wfe+1, data, 36);
							}
						} else if (type == WAVE_FORMAT_PCM) {
							mt.SetSampleSize(wfe->nBlockAlign);
							if (channels > 2 || bitspersample > 16) {
								WAVEFORMATEXTENSIBLE* wfex = (WAVEFORMATEXTENSIBLE*)mt.ReallocFormatBuffer(sizeof(WAVEFORMATEXTENSIBLE));
								if (wfex != NULL) {
									wfex->Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
									wfex->Format.cbSize = 22;
									wfex->Samples.wValidBitsPerSample = bitspersample;
									wfex->dwChannelMask = GetDefChannelMask(channels); // TODO: get correct channel mask from mov file
									wfex->SubFormat = MEDIASUBTYPE_PCM;
								}
							}
						} else if (type == WAVE_FORMAT_IEEE_FLOAT) {
							mt.SetSampleSize(wfe->nBlockAlign);
							if (channels > 2) {
								WAVEFORMATEXTENSIBLE* wfex = (WAVEFORMATEXTENSIBLE*)mt.ReallocFormatBuffer(sizeof(WAVEFORMATEXTENSIBLE));
								if (wfex != NULL) {
									wfex->Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
									wfex->Format.cbSize = 22;
									wfex->Samples.wValidBitsPerSample = bitspersample;
									wfex->dwChannelMask = GetDefChannelMask(channels); // TODO: get correct channel mask from mov file
									wfex->SubFormat = MEDIASUBTYPE_IEEE_FLOAT;
								}
							}
						} else if (type == AP4_ATOM_TYPE_MP4A ||
								   type == AP4_ATOM_TYPE_ALAW ||
								   type == AP4_ATOM_TYPE_ULAW) {
							// not need any extra data for ALAW, ULAW
							// also extra data is not required for IMA4, MAC3, MAC6
						} else if (type == AP4_ATOM_TYPE_OWMA && db.GetDataSize() > 36) {
							// skip unknown first 36 bytes
							const AP4_Size size = db.GetDataSize() - 36;
							const AP4_Byte* pData = db.GetData() + 36;

							wfe = (WAVEFORMATEX*)mt.ReallocFormatBuffer(size);
							memcpy(wfe, pData, size);

							mt.subtype = FOURCCMap(wfe->wFormatTag);
						} else if (type == AP4_ATOM_TYPE_FLAC) {
							mt.subtype = MEDIASUBTYPE_FLAC_FRAMED;
							if (AP4_FLACSampleEntry* flacSample = dynamic_cast<AP4_FLACSampleEntry*>(atom)) {
								const AP4_DataBuffer* extraData = flacSample->GetData();
								wfe = (WAVEFORMATEX*)mt.ReallocFormatBuffer(sizeof(WAVEFORMATEX) + extraData->GetDataSize());
								wfe->cbSize = extraData->GetDataSize();
								memcpy(wfe + 1, extraData->GetData(), extraData->GetDataSize());
							}
							mts.Add(mt);

							mt.subtype = MEDIASUBTYPE_FLAC;
						} else if (type == WAVE_FORMAT_DTS2) {
							CString Suffix = L"DTS audio";

							m_pFile->Seek(sample.GetOffset());
							AP4_DataBuffer data;
							if (AP4_SUCCEEDED(sample.ReadData(data))) {
								const BYTE* start = data.GetData();
								const BYTE* end = start + data.GetDataSize();
								audioframe_t aframe;
								const int size = ParseDTSHeader(start);
								if (size) {
									if (start + size + 40 <= end) {
										const int sizehd = ParseDTSHDHeader(start + size, end - start - size, &aframe);
										if (sizehd) {
											WAVEFORMATEX* wfe = (WAVEFORMATEX*)mt.pbFormat;
											wfe->nSamplesPerSec = aframe.samplerate;
											wfe->nChannels = aframe.channels;
											if (aframe.param1) {
												wfe->wBitsPerSample = aframe.param1;
											}
											wfe->nBlockAlign = (wfe->nChannels * wfe->wBitsPerSample) / 8;
											if (aframe.param2 == DCA_PROFILE_HD_HRA) {
												Suffix = L"DTS-HD High Resolution Audio";
												wfe->nAvgBytesPerSec += CalcBitrate(aframe) / 8;
											} else {
												Suffix = L"DTS-HD Master Audio";
												wfe->nAvgBytesPerSec = 0;
											}
										}
									}
								} else if (ParseDTSHDHeader(start, end - start, &aframe) && aframe.param2 == DCA_PROFILE_EXPRESS) {
									WAVEFORMATEX* wfe = (WAVEFORMATEX*)mt.pbFormat;
									wfe->nSamplesPerSec = aframe.samplerate;
									wfe->nChannels = aframe.channels;
									wfe->wBitsPerSample = aframe.param1;
									wfe->nBlockAlign = wfe->nChannels * wfe->wBitsPerSample / 8;
									wfe->nAvgBytesPerSec = CalcBitrate(aframe) / 8;

									Suffix = L"DTS Express";
								}
							}

							SetTrackName(&TrackName, Suffix);
						} else if (db.GetDataSize() > 0) {
							//always needed extra data for QDM2
							wfe = (WAVEFORMATEX*)mt.ReallocFormatBuffer(sizeof(WAVEFORMATEX) + db.GetDataSize());
							wfe->cbSize = db.GetDataSize();
							memcpy(wfe + 1, db.GetData(), db.GetDataSize());
						}

						mts.Add(mt);

						if (AP4_DataInfoAtom* WfexAtom = dynamic_cast<AP4_DataInfoAtom*>(ase->GetChild(AP4_ATOM_TYPE_WFEX))) {
							const AP4_DataBuffer* di = WfexAtom->GetData();
							if (di && di->GetDataSize()) {
								wfe = (WAVEFORMATEX*)mt.ReallocFormatBuffer(di->GetDataSize());
								memcpy(wfe, di->GetData(), di->GetDataSize());

								mt.subtype = FOURCCMap(wfe->wFormatTag);
								mts.InsertAt(0, mt);
							}
						}

						break;
					} else {
						DLog(L"Unknow MP4 Stream %x" , type);
					}
				}
			}

			if (mts.IsEmpty()) {
				continue;
			}

			REFERENCE_TIME rtDuration = 10000i64 * track->GetDurationHighPrecision();
			if (m_rtDuration < rtDuration) {
				m_rtDuration = rtDuration;
			}
			if (rtVideoDuration < rtDuration && AP4_Track::TYPE_VIDEO == track->GetType())
				rtVideoDuration = rtDuration; // get the max video duration

			DWORD id = track->GetId();

			CStringW name, lang;
			name.Format(L"Output %d", id);

			if (!TrackName.IsEmpty()) {
				name = TrackName;
			}

			if (!TrackLanguage.IsEmpty()) {
				if (TrackLanguage != "und") {
					name.AppendFormat(L" (%S)", TrackLanguage);
				}
			}

			if (AP4_Track::TYPE_VIDEO == track->GetType() && videoSize == CSize(0, 0)) {
				for (int i = 0, j = mts.GetCount(); i < j; ++i) {
					BITMAPINFOHEADER bih;
					if (ExtractBIH(&mts[i], &bih)) {
						videoSize.cx = bih.biWidth;
						videoSize.cy = abs(bih.biHeight);
						break;
					}
				}
			}

			CAutoPtr<CBaseSplitterOutputPin> pPinOut(DNew CMP4SplitterOutputPin(mts, name, this, this, &hr));

			if (!TrackName.IsEmpty()) {
				pPinOut->SetProperty(L"NAME", TrackName);
			}
			if (!TrackLanguage.IsEmpty()) {
				pPinOut->SetProperty(L"LANG", CStringW(TrackLanguage));
			}

			EXECUTE_ASSERT(SUCCEEDED(AddOutputPin(id, pPinOut)));

			m_trackpos[id] = trackpos();
		}

		if (AP4_ChplAtom* chpl = dynamic_cast<AP4_ChplAtom*>(movie->GetMoovAtom()->FindChild("udta/chpl"))) {
			AP4_Array<AP4_ChplAtom::AP4_Chapter>& chapters = chpl->GetChapters();

			for (AP4_Cardinal i = 0; i < chapters.ItemCount(); ++i) {
				AP4_ChplAtom::AP4_Chapter& chapter = chapters[i];

				CString ChapterName = ConvertStr(chapter.Name.c_str());
				ChapAppend(chapter.Time, ChapterName);
			}
		} else if (ChapterTrackId != INT_MIN) {
			for (AP4_List<AP4_Track>::Item* item = movie->GetTracks().FirstItem();
					item;
					item = item->GetNext()) {
				AP4_Track* track = item->GetData();

				if (ChapterTrackId == track->GetId()) {
					char* buff = NULL;
					for (AP4_Cardinal i = 0; i < track->GetSampleCount(); i++) {
						AP4_Sample sample;
						AP4_DataBuffer data;
						track->ReadSample(i, sample, data);

						const AP4_Byte* ptr  = data.GetData();
						const AP4_Size avail = data.GetDataSize();
						CString ChapterName;

						if (avail > 2) {
							size_t size = (ptr[0] << 8) | ptr[1];
							if (size > avail - 2) {
								size = avail - 2;
							}

							bool bUTF16LE = false;
							bool bUTF16BE = false;
							if (avail > 4) {
								const WORD bom = (ptr[2] << 8) | ptr[3];
								if (bom == 0xfffe) {
									bUTF16LE = true;
								} else if (bom == 0xfeff) {
									bUTF16BE = true;
								}
							}

							if (bUTF16LE || bUTF16BE) {
								memcpy((BYTE*)ChapterName.GetBufferSetLength(size / 2), &ptr[2], size);
								if (bUTF16BE) {
									for (int i = 0, j = ChapterName.GetLength(); i < j; i++) {
										ChapterName.SetAt(i, (ChapterName[i] << 8) | (ChapterName[i] >> 8));
									}
								}
							} else {
								buff = DNew char[size + 1];
								memcpy(buff, &ptr[2], size);
								buff[size] = 0;

								ChapterName = ConvertStr(buff);

								SAFE_DELETE_ARRAY(buff);
							}

							const REFERENCE_TIME rtStart = FractionScale64(sample.GetCts(), UNITS, track->GetMediaTimeScale());
							ChapAppend(rtStart, ChapterName);
						}
					}

					break;
				}
			}
		}

		if (ChapGetCount()) {
			ChapSort();
		}

		if (AP4_ContainerAtom* ilst = dynamic_cast<AP4_ContainerAtom*>(movie->GetMoovAtom()->FindChild("udta/meta/ilst"))) {
			CStringW title, artist, writer, album, year, appl, desc, gen, track, copyright;

			for (AP4_List<AP4_Atom>::Item* item = ilst->GetChildren().FirstItem();
					item;
					item = item->GetNext()) {
				if (AP4_ContainerAtom* atom = dynamic_cast<AP4_ContainerAtom*>(item->GetData())) {
					if (AP4_DataAtom* data = dynamic_cast<AP4_DataAtom*>(atom->GetChild(AP4_ATOM_TYPE_DATA))) {
						const AP4_DataBuffer* db = data->GetData();

						if (atom->GetType() == AP4_ATOM_TYPE_TRKN) {
							if (db->GetDataSize() >= 4) {
								unsigned short n = (db->GetData()[2] << 8) | db->GetData()[3];
								if (n > 0 && n < 100) {
									track.Format(L"%02d", n);
								} else if (n >= 100) {
									track.Format(L"%d", n);
								}
							}
						} else if (atom->GetType() == AP4_ATOM_TYPE_COVR) {
							if (db->GetDataSize() > 10) {
								DWORD sync = GETDWORD(db->GetData());
								if ((sync & 0x00ffffff) == 0x00FFD8FF) { // SOI segment + first byte of next segment
									ResAppend(L"cover.jpg", L"cover", L"image/jpeg", (BYTE*)db->GetData(), (DWORD)db->GetDataSize());
								}
								else if (sync == MAKEFOURCC(0x89, 'P', 'N', 'G')) {
									ResAppend(L"cover.png", L"cover", L"image/png", (BYTE*)db->GetData(), (DWORD)db->GetDataSize());
								}
							}
						} else {
							CStringW str = UTF8To16(CStringA((LPCSTR)db->GetData(), db->GetDataSize()));

							switch (atom->GetType()) {
								case AP4_ATOM_TYPE_NAM:
									title = str;
									break;
								case AP4_ATOM_TYPE_ART:
									artist = str;
									break;
								case AP4_ATOM_TYPE_WRT:
									writer = str;
									break;
								case AP4_ATOM_TYPE_ALB:
									album = str;
									break;
								case AP4_ATOM_TYPE_DAY:
									year = str;
									break;
								case AP4_ATOM_TYPE_TOO:
									appl = str;
									break;
								case AP4_ATOM_TYPE_CMT:
									desc = str;
									break;
								case AP4_ATOM_TYPE_DESC:
									if (desc.IsEmpty()) {
										desc = str;
									}
									break;
								case AP4_ATOM_TYPE_GEN:
									gen = str;
									break;
								case AP4_ATOM_TYPE_CPRT:
									copyright = str;
									break;
							}
						}
					}
				}
			}

			if (!title.IsEmpty()) {
				SetProperty(L"TITL", title);
			}

			if (!album.IsEmpty()) {
				SetProperty(L"ALBUM", album);
			}

			if (!artist.IsEmpty()) {
				SetProperty(L"AUTH", artist);
			} else if (!writer.IsEmpty()) {
				SetProperty(L"AUTH", writer);
			}

			if (!appl.IsEmpty()) {
				SetProperty(L"APPL", appl);
			}

			if (!desc.IsEmpty()) {
				SetProperty(L"DESC", desc);
			}

			if (!copyright.IsEmpty()) {
				SetProperty(L"CPYR", copyright);
			}
		}

		const AP4_Duration fragmentdurationms = movie->GetFragmentsDurationMs();
		if (fragmentdurationms) {
			// override duration from 'sidx' atom
			const REFERENCE_TIME rtDuration = 10000i64 * fragmentdurationms;
			m_rtDuration = rtVideoDuration = rtDuration;
		}
	}

	if (rtVideoDuration > 0 && rtVideoDuration < m_rtDuration / 2) {
		m_rtDuration = rtVideoDuration; // fix incorrect duration
	}

	m_rtNewStop = m_rtStop = m_rtDuration;

	if (m_pOutputs.GetCount()) {
		AP4_Movie* movie = m_pFile->GetMovie();

		if (movie->HasFragmentsIndex()) {
			const AP4_Array<AP4_IndexTableEntry>& entries = movie->GetFragmentsIndexEntries();
			for (AP4_Cardinal i = 0; i < entries.ItemCount(); ++i) {
				SyncPoint sp = { entries[i].m_rt, __int64(entries[i].m_offset) };
				m_sps.Add(sp);
			}
		} else {
			POSITION pos = m_trackpos.GetStartPosition();
			while (pos) {
				CAtlMap<DWORD, trackpos>::CPair* pPair = m_trackpos.GetNext(pos);
				AP4_Track* track = movie->GetTrack(pPair->m_key);

				if (!track->HasIndex()) {
					continue;
				}

				const AP4_Array<AP4_IndexTableEntry>& entries = track->GetIndexEntries();
				for (AP4_Cardinal i = 0; i < entries.ItemCount(); ++i) {
					SyncPoint sp = { entries[i].m_rt, __int64(entries[i].m_offset) };
					m_sps.Add(sp);
				}

				break;
			}
		}
	}

	return m_pOutputs.GetCount() > 0 ? S_OK : E_FAIL;
}

bool CMP4SplitterFilter::DemuxInit()
{
	AP4_Movie* movie = m_pFile->GetMovie();

	POSITION pos = m_trackpos.GetStartPosition();
	while (pos) {
		CAtlMap<DWORD, trackpos>::CPair* pPair = m_trackpos.GetNext(pos);

		pPair->m_value.index = 0;
		pPair->m_value.ts = 0;
		pPair->m_value.offset = 0;

		AP4_Track* track = movie->GetTrack(pPair->m_key);

		AP4_Sample sample;
		if (AP4_SUCCEEDED(track->GetSample(0, sample))) {
			pPair->m_value.ts = sample.GetCts();
			pPair->m_value.offset = sample.GetOffset();
		}
	}

	return true;
}

void CMP4SplitterFilter::DemuxSeek(REFERENCE_TIME rt)
{
	if (m_pFile->IsStreaming() || m_rtDuration <= 0) {
		return;
	}

	AP4_Movie* movie = m_pFile->GetMovie();

	if (movie->HasFragmentsIndex()
			&& (AP4_FAILED(movie->SelectMoof(rt)))) {
		bSelectMoofSuccessfully = FALSE;
		return;
	}
	bSelectMoofSuccessfully = TRUE;

	if (rt <= 0) {
		DemuxInit();
		return;
	}

	POSITION pos = m_trackpos.GetStartPosition();
	while (pos) {
		CAtlMap<DWORD, trackpos>::CPair* pPair = m_trackpos.GetNext(pos);
		AP4_Track* track = movie->GetTrack(pPair->m_key);

		if (track->HasIndex() && track->GetIndexEntries().ItemCount()) {
			if (AP4_FAILED(track->GetIndexForRefTime(rt, pPair->m_value.index, pPair->m_value.ts, pPair->m_value.offset))) {
				continue;
			}
		} else {
			if (AP4_FAILED(track->GetSampleIndexForRefTime(rt, pPair->m_value.index))) {
				continue;
			}

			AP4_Sample sample;
			if (AP4_SUCCEEDED(track->GetSample(pPair->m_value.index, sample))) {
				pPair->m_value.ts = sample.GetCts();
				pPair->m_value.offset = sample.GetOffset();
			}
		}
	}
}

bool CMP4SplitterFilter::DemuxLoop()
{
	HRESULT hr = S_OK;

	if (!bSelectMoofSuccessfully) {
		return true;
	}

	m_pFile->Seek(0);
	AP4_Movie* movie = m_pFile->GetMovie();

	while (SUCCEEDED(hr) && !CheckRequest(NULL)) {

start:
		CAtlMap<DWORD, trackpos>::CPair* pPairNext = NULL;
		REFERENCE_TIME rtNext = _I64_MAX;
		ULONGLONG nextOffset = 0;

		POSITION pos = m_trackpos.GetStartPosition();
		while (pos) {
			CAtlMap<DWORD, trackpos>::CPair* pPair = m_trackpos.GetNext(pos);

			AP4_Track* track = movie->GetTrack(pPair->m_key);

			CBaseSplitterOutputPin* pPin = GetOutputPin((DWORD)track->GetId());
			if (!pPin->IsConnected()) {
				continue;
			}

			const REFERENCE_TIME rt = (REFERENCE_TIME)(10000000.0 / track->GetMediaTimeScale() * pPair->m_value.ts);

			if (pPair->m_value.index < track->GetSampleCount()) {
				if (!pPairNext
						|| (llabs(rtNext - rt) <= UNITS && pPair->m_value.offset < nextOffset)
						|| (llabs(rtNext - rt) > UNITS && rt < rtNext)) {
					pPairNext = pPair;
					nextOffset = pPair->m_value.offset;
					rtNext = rt;
				}
			}
		}

		if (!pPairNext) {
			if (movie->HasFragmentsIndex()
					&& (AP4_SUCCEEDED(movie->SwitchNextMoof()))) {
				DemuxInit();
				goto start;
			} else {
				break;
			}
		}

		AP4_Track* track = movie->GetTrack(pPairNext->m_key);

		CBaseSplitterOutputPin* pPin = GetOutputPin((DWORD)track->GetId());

		AP4_Sample sample;
		AP4_DataBuffer data;

		if (pPin && pPin->IsConnected() && AP4_SUCCEEDED(track->ReadSample(pPairNext->m_value.index, sample, data))) {
			const CMediaType& mt = pPin->CurrentMediaType();

			CAutoPtr<CPacket> p(DNew CPacket());
			p->TrackNumber = (DWORD)track->GetId();
			p->rtStart = FractionScale64(sample.GetCts(), UNITS, track->GetMediaTimeScale());
			p->rtStop = FractionScale64(sample.GetCts() + sample.GetDuration(), UNITS, track->GetMediaTimeScale());
			if (p->rtStop == p->rtStart && p->rtStart != INVALID_TIME) {
				p->rtStop++;
			}
			p->bSyncPoint = sample.IsSync();

			REFERENCE_TIME duration = p->rtStop - p->rtStart;

			if (track->GetType() == AP4_Track::TYPE_AUDIO
					&& mt.subtype != MEDIASUBTYPE_RAW_AAC1
					&& duration < 100000) { // duration < 10 ms (hack for PCM, ADPCM, Law and other)

				p->SetCount(0, (500000 / duration + 1) * data.GetDataSize()); // grouping > 50 ms
				p->SetData(data.GetData(), data.GetDataSize());

				while (duration < 500000 && AP4_SUCCEEDED(track->ReadSample(pPairNext->m_value.index + 1, sample, data))) {
					size_t size = p->GetCount();
					p->SetCount(size + data.GetDataSize());
					memcpy(p->GetData() + size, data.GetData(), data.GetDataSize());

					p->rtStop = FractionScale64(sample.GetCts() + sample.GetDuration(), UNITS, track->GetMediaTimeScale());

					duration = p->rtStop - p->rtStart;

					pPairNext->m_value.index++;
				}
			}
			else if (track->GetType() == AP4_Track::TYPE_TEXT) {
				const AP4_Byte* ptr = data.GetData();
				AP4_Size avail = data.GetDataSize();

				if (avail > 2) {
					AP4_UI16 size = (ptr[0] << 8) | ptr[1];

					if (size <= avail-2) {
						CStringA str;

						if (size >= 2 && ptr[2] == 0xfe && ptr[3] == 0xff) {
							CStringW wstr = CStringW((LPCWSTR)&ptr[2], size/2);
							for (int i = 0; i < wstr.GetLength(); ++i) {
								wstr.SetAt(i, ((WORD)wstr[i] >> 8) | ((WORD)wstr[i] << 8));
							}
							str = UTF16To8(wstr);
						} else {
							str = CStringA((LPCSTR)&ptr[2], size);
						}

						CStringA dlgln = str;
						dlgln.Replace("\r", "");
						dlgln.Replace("\n", "\\N");

						p->SetData((LPCSTR)dlgln, dlgln.GetLength());
					}
				}
			}
			else {
				p->SetData(data.GetData(), data.GetDataSize());

				if (track->m_hasPalette) {
					track->m_hasPalette = false;
					CAutoPtr<CPacket> p2(DNew CPacket());
					p2->SetData(track->GetPalette(), 1024);
					p->Append(*p2);

					CAutoPtr<CPacket> p3(DNew CPacket());
					static BYTE add[13] = {0x00, 0x00, 0x04, 0x00, 0x80, 0x8C, 0x4D, 0x9D, 0x10, 0x8E, 0x25, 0xE9, 0xFE};
					p3->SetData(add, _countof(add));
					p->Append(*p3);
				}
			}

			hr = DeliverPacket(p);
		}

		{
			AP4_Sample sample;
			if (AP4_SUCCEEDED(track->GetSample(++pPairNext->m_value.index, sample))) {
				pPairNext->m_value.ts = sample.GetCts();
				pPairNext->m_value.offset = sample.GetOffset();
			}
		}

		if (FAILED(m_pFile->GetLastReadError())) {
			break;
		}
	}

	return true;
}

// IKeyFrameInfo

STDMETHODIMP CMP4SplitterFilter::GetKeyFrameCount(UINT& nKFs)
{
	CheckPointer(m_pFile, E_UNEXPECTED);

	nKFs = m_sps.GetCount();
	return S_OK;
}

STDMETHODIMP CMP4SplitterFilter::GetKeyFrames(const GUID* pFormat, REFERENCE_TIME* pKFs, UINT& nKFs)
{
	CheckPointer(pFormat, E_POINTER);
	CheckPointer(pKFs, E_POINTER);
	CheckPointer(m_pFile, E_UNEXPECTED);

	if (*pFormat != TIME_FORMAT_MEDIA_TIME) {
		return E_INVALIDARG;
	}

	for (nKFs = 0; nKFs < m_sps.GetCount(); nKFs++) {
		pKFs[nKFs] = m_sps[nKFs].rt;
	}

	return S_OK;
}

//
// CMP4SourceFilter
//

CMP4SourceFilter::CMP4SourceFilter(LPUNKNOWN pUnk, HRESULT* phr)
	: CMP4SplitterFilter(pUnk, phr)
{
	m_clsid = __uuidof(this);
	m_pInput.Free();
}

//
// CMP4SplitterOutputPin
//

CMP4SplitterOutputPin::CMP4SplitterOutputPin(CAtlArray<CMediaType>& mts, LPCWSTR pName, CBaseFilter* pFilter, CCritSec* pLock, HRESULT* phr)
	: CBaseSplitterOutputPin(mts, pName, pFilter, pLock, phr)
{
}

CMP4SplitterOutputPin::~CMP4SplitterOutputPin()
{
}

HRESULT CMP4SplitterOutputPin::DeliverNewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate)
{
	CAutoLock cAutoLock(this);
	return __super::DeliverNewSegment(tStart, tStop, dRate);
}

HRESULT CMP4SplitterOutputPin::DeliverEndFlush()
{
	CAutoLock cAutoLock(this);
	return __super::DeliverEndFlush();
}

HRESULT CMP4SplitterOutputPin::DeliverPacket(CAutoPtr<CPacket> p)
{
	CAutoLock cAutoLock(this);

	if (p && m_mt.subtype == MEDIASUBTYPE_VP90) {
		REFERENCE_TIME rtStartTmp = p->rtStart;
		REFERENCE_TIME rtStopTmp  = p->rtStop;

		const BYTE* pData = p->GetData();
		size_t size = p->GetCount();

		const BYTE marker = pData[size - 1];
		if ((marker & 0xe0) == 0xc0) {
			HRESULT hr = S_OK;

			const BYTE nbytes = 1 + ((marker >> 3) & 0x3);
			BYTE n_frames = 1 + (marker & 0x7);
			const size_t idx_sz = 2 + n_frames * nbytes;
			if (size >= idx_sz && pData[size - idx_sz] == marker && nbytes >= 1 && nbytes <= 4) {
				const BYTE *idx = pData + size + 1 - idx_sz;

				while (n_frames--) {
					size_t sz = 0;
					switch(nbytes) {
						case 1: sz = (BYTE)*idx; break;
						case 2: sz = AV_RL16(idx); break;
						case 3: sz = AV_RL24(idx); break;
						case 4: sz = AV_RL32(idx); break;
					}

					idx += nbytes;
					if (sz > size || !sz) {
						break;
					}

					CAutoPtr<CPacket> packet(DNew CPacket());
					packet->SetData(pData, sz);

					packet->TrackNumber    = p->TrackNumber;
					packet->bDiscontinuity = p->bDiscontinuity;
					packet->bSyncPoint     = p->bSyncPoint;

					if (pData[0] & 0x2) {
						packet->rtStart = rtStartTmp;
						packet->rtStop  = rtStopTmp;
						rtStartTmp      = INVALID_TIME;
						rtStopTmp       = INVALID_TIME;
					}
					if (S_OK != (hr = __super::DeliverPacket(packet))) {
						break;
					}

					pData += sz;
					size -= sz;
				}
			}

			return hr;
		}
	}

	return __super::DeliverPacket(p);
}
