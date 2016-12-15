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

#pragma once

#include <afxwin.h>
#include <afxcmn.h>
#include "../../filters/transform/BufferFilter/BufferFilter.h"
#include "FloatEdit.h"
#include <ResizableLib/ResizableDialog.h>
#include "PlayerBar.h"

//

template<class T>
class CFormatElem
{
public:
	CMediaType mt;
	T caps;
};

template<class T>
class CFormat : public CAutoPtrArray<CFormatElem<T> >
{
public:
	CString name;
	CFormat(CString name = L"") {
		this->name = name;
	}
	virtual ~CFormat() {}
};

template<class T>
class CFormatArray : public CAutoPtrArray<CFormat<T> >
{
public:
	virtual ~CFormatArray() {}

	CFormat<T>* Find(CString name, bool fCreate = false) {
		for (size_t i = 0; i < GetCount(); ++i) {
			if (GetAt(i)->name == name) {
				return(GetAt(i));
			}
		}

		if (fCreate) {
			CAutoPtr<CFormat<T> > pf(DNew CFormat<T>(name));
			CFormat<T>* tmp = pf;
			Add(pf);
			return(tmp);
		}

		return(NULL);
	}

	bool FindFormat(AM_MEDIA_TYPE* pmt, CFormat<T>** ppf) {
		if (!pmt) {
			return false;
		}

		for (size_t i = 0; i < GetCount(); ++i) {
			CFormat<T>* pf = GetAt(i);
			for (size_t j = 0; j < pf->GetCount(); ++j) {
				CFormatElem<T>* pfe = pf->GetAt(j);
				if (!pmt || (pfe->mt.majortype == pmt->majortype && pfe->mt.subtype == pmt->subtype)) {
					if (ppf) {
						*ppf = pf;
					}
					return true;
				}
			}
		}

		return false;
	}

	bool FindFormat(AM_MEDIA_TYPE* pmt, T* pcaps, CFormat<T>** ppf, CFormatElem<T>** ppfe) {
		if (!pmt && !pcaps) {
			return false;
		}

		for (size_t i = 0; i < GetCount(); ++i) {
			CFormat<T>* pf = GetAt(i);
			for (size_t j = 0; j < pf->GetCount(); ++j) {
				CFormatElem<T>* pfe = pf->GetAt(j);
				if ((!pmt || pfe->mt == *pmt) && (!pcaps || !memcmp(pcaps, &pfe->caps, sizeof(T)))) {
					if (ppf) {
						*ppf = pf;
					}
					if (ppfe) {
						*ppfe = pfe;
					}
					return true;
				}
			}
		}

		return false;
	}

	bool AddFormat(AM_MEDIA_TYPE* pmt, T caps) {
		if (!pmt) {
			return false;
		}

		if (FindFormat(pmt, NULL, NULL, NULL)) {
			DeleteMediaType(pmt);
			return false;
		}
		//		if (pmt->formattype == FORMAT_VideoInfo2) {DeleteMediaType(pmt); return false;} // TODO

		CFormat<T>* pf = Find(MakeFormatName(pmt), true);
		if (!pf) {
			DeleteMediaType(pmt);
			return false;
		}

		CAutoPtr<CFormatElem<T> > pfe(DNew CFormatElem<T>());
		pfe->mt = *pmt;
		pfe->caps = caps;
		pf->Add(pfe);

		return true;
	}

	bool AddFormat(AM_MEDIA_TYPE* pmt, void* pcaps, int size) {
		if (!pcaps) {
			return false;
		}
		ASSERT(size == sizeof(T));
		return AddFormat(pmt, *(T*)pcaps);
	}

	virtual CString MakeFormatName(AM_MEDIA_TYPE* pmt) PURE;
	virtual CString MakeDimensionName(CFormatElem<T>* pfe) PURE;
};

typedef CFormatElem<VIDEO_STREAM_CONFIG_CAPS> CVidFormatElem;
typedef CFormat<VIDEO_STREAM_CONFIG_CAPS> CVidFormat;

class CVidFormatArray : public CFormatArray<VIDEO_STREAM_CONFIG_CAPS>
{
public:
	CString MakeFormatName(AM_MEDIA_TYPE* pmt) {
		CString str(L"Default");

		if (!pmt) {
			return(str);
		}

		BITMAPINFOHEADER* bih = (pmt->formattype == FORMAT_VideoInfo)
								? &((VIDEOINFOHEADER*)pmt->pbFormat)->bmiHeader
								: (pmt->formattype == FORMAT_VideoInfo2)
								? &((VIDEOINFOHEADER2*)pmt->pbFormat)->bmiHeader
								: NULL;

		if (!bih) {
			// it may have a fourcc in the mediasubtype, let's check that

			WCHAR guid[100];
			memset(guid, 0, 100*sizeof(WCHAR));
			StringFromGUID2(pmt->subtype, guid, 100);

			if (CStringW(guid).MakeUpper().Find(L"0000-0010-8000-00AA00389B71") >= 0) {
				str.Format(L"%c%c%c%c",
						   (WCHAR)((pmt->subtype.Data1>>0)&0xff),
						   (WCHAR)((pmt->subtype.Data1>>8)&0xff),
						   (WCHAR)((pmt->subtype.Data1>>16)&0xff),
						   (WCHAR)((pmt->subtype.Data1>>24)&0xff));
			}

			return(str);
		}

		switch (bih->biCompression) {
			case BI_RGB:
				str.Format(L"RGB%d", bih->biBitCount);
				break;
			case BI_RLE8:
				str = L"RLE8";
				break;
			case BI_RLE4:
				str = L"RLE4";
				break;
			case BI_BITFIELDS:
				str.Format(L"BITF%d", bih->biBitCount);
				break;
			case BI_JPEG:
				str = L"JPEG";
				break;
			case BI_PNG:
				str = L"PNG";
				break;
			default:
				str.Format(L"%c%c%c%c",
						   (WCHAR)((bih->biCompression>>0)&0xff),
						   (WCHAR)((bih->biCompression>>8)&0xff),
						   (WCHAR)((bih->biCompression>>16)&0xff),
						   (WCHAR)((bih->biCompression>>24)&0xff));
				break;
		}

		return(str);
	}

	CString MakeDimensionName(CVidFormatElem* pfe) {
		CString str(L"Default");

		if (!pfe) {
			return(str);
		}

		BITMAPINFOHEADER* bih = (pfe->mt.formattype == FORMAT_VideoInfo)
								? &((VIDEOINFOHEADER*)pfe->mt.pbFormat)->bmiHeader
								: (pfe->mt.formattype == FORMAT_VideoInfo2)
								? &((VIDEOINFOHEADER2*)pfe->mt.pbFormat)->bmiHeader
								: NULL;

		if (bih == NULL) {
			return(str);
		}

		str.Format(L"%dx%d %.2f", bih->biWidth, bih->biHeight, (float)10000000/((VIDEOINFOHEADER*)pfe->mt.pbFormat)->AvgTimePerFrame);

		if (pfe->mt.formattype == FORMAT_VideoInfo2) {
			VIDEOINFOHEADER2* vih2 = (VIDEOINFOHEADER2*)pfe->mt.pbFormat;
			CString str2;
			str2.Format(L" i%02x %d:%d", vih2->dwInterlaceFlags, vih2->dwPictAspectRatioX, vih2->dwPictAspectRatioY);
			str += str2;
		}

		return(str);
	}
};

typedef CFormatElem<AUDIO_STREAM_CONFIG_CAPS> CAudFormatElem;
typedef CFormat<AUDIO_STREAM_CONFIG_CAPS> CAudFormat;

class CAudFormatArray : public CFormatArray<AUDIO_STREAM_CONFIG_CAPS>
{
public:
	CString MakeFormatName(AM_MEDIA_TYPE* pmt) {
		CString str(L"Unknown");

		if (!pmt) {
			return(str);
		}

		WAVEFORMATEX* wfe = (pmt->formattype == FORMAT_WaveFormatEx)
							? (WAVEFORMATEX*)pmt->pbFormat
							: NULL;

		if (!wfe) {
			WCHAR guid[100];
			memset(guid, 0, 100*sizeof(WCHAR));
			StringFromGUID2(pmt->subtype, guid, 100);

			if (CStringW(guid).MakeUpper().Find(L"0000-0010-8000-00AA00389B71") >= 0) {
				str.Format(L"0x%04x", pmt->subtype.Data1);
			}

			return(str);
		}

		switch (wfe->wFormatTag) {
			case 1:
				str = L"PCM ";
				break;
			default:
				str.Format(L"0x%03x ", wfe->wFormatTag);
				break;
		}

		return(str);
	}

	CString MakeDimensionName(CAudFormatElem* pfe) {
		CString str(L"Unknown");

		if (!pfe) {
			return(str);
		}

		WAVEFORMATEX* wfe = (pfe->mt.formattype == FORMAT_WaveFormatEx)
							? (WAVEFORMATEX*)pfe->mt.pbFormat
							: NULL;

		if (!wfe) {
			return(str);
		}

		str.Empty();
		CString str2;

		str2.Format(L"%6dKHz ", wfe->nSamplesPerSec);
		str += str2;

		str2.Format(L"%dbps ", wfe->wBitsPerSample);
		str += str2;

		switch (wfe->nChannels) {
			case 1:
				str += L"mono ";
				break;
			case 2:
				str += L"stereo ";
				break;
			default:
				str2.Format(L"%d channels ", wfe->nChannels);
				str += str2;
				break;
		}

		str2.Format(L"%3dkbps ", wfe->nAvgBytesPerSec*8/1000);
		str += str2;

		return(str);
	}
};

//

struct Codec {
	CComPtr<IMoniker> pMoniker;
	CComPtr<IBaseFilter> pBF;
	CString FriendlyName;
	CComBSTR DisplayName;
};

typedef CAtlArray<Codec> CCodecArray;

class CMainFrame;

// CPlayerCaptureDialog dialog

class CPlayerCaptureDialog : public CResizableDialog
{
	//DECLARE_DYNAMIC(CPlayerCaptureDialog)

private:
	CMainFrame* m_pMainFrame;

	// video input
	CStringW m_VidDisplayName;
	CComPtr<IAMStreamConfig> m_pAMVSC;
	CComPtr<IAMCrossbar> m_pAMXB;
	CComPtr<IAMTVTuner> m_pAMTuner;
	CComPtr<IAMVfwCaptureDialogs> m_pAMVfwCD;
	CVidFormatArray m_vfa;

	// audio input
	CStringW m_AudDisplayName;
	CComPtr<IAMStreamConfig> m_pAMASC;
	CInterfaceArray<IAMAudioInputMixer> m_pAMAIM;
	CAudFormatArray m_afa;

	// video codec
	CCodecArray m_pVidEncArray;
	CVidFormatArray m_vcfa;

	// audio codec
	CCodecArray m_pAudEncArray;
	CAudFormatArray m_acfa;

	void EmptyVideo();
	void EmptyAudio();

	void UpdateMediaTypes();
	void UpdateUserDefinableControls();
	void UpdateVideoCodec();
	void UpdateAudioCodec();
	void UpdateMuxer();
	void UpdateOutputControls();

	void UpdateGraph();

	CMap<HWND, HWND&, BOOL, BOOL&> m_wndenabledmap;
	void EnableControls(CWnd* pWnd, bool fEnable);

	bool m_fEnableOgm;

public:
	CPlayerCaptureDialog(CMainFrame* pMainFrame);
	virtual ~CPlayerCaptureDialog();

	BOOL Create(CWnd* pParent = NULL);

	// Dialog Data
	enum { IDD = IDD_CAPTURE_DLG };

	CComboBox m_vidinput;
	CComboBox m_vidtype;
	CComboBox m_viddimension;
	CSpinButtonCtrl m_vidhor;
	CSpinButtonCtrl m_vidver;
	CEdit m_vidhoredit;
	CEdit m_vidveredit;
	CFloatEdit m_vidfpsedit;
	float m_vidfps;
	CButton m_vidsetres;
	CComboBox m_audinput;
	CComboBox m_audtype;
	CComboBox m_auddimension;
	CComboBox m_vidcodec;
	CComboBox m_vidcodectype;
	CComboBox m_vidcodecdimension;
	BOOL m_fVidOutput;
	CButton m_vidoutput;
	int m_fVidPreview;
	CButton m_vidpreview;
	CComboBox m_audcodec;
	CComboBox m_audcodectype;
	CComboBox m_audcodecdimension;
	BOOL m_fAudOutput;
	CButton m_audoutput;
	int m_fAudPreview;
	CButton m_audpreview;
	int m_nVidBuffers;
	int m_nAudBuffers;
	CString m_file;
	CButton m_recordbtn;
	BOOL m_fSepAudio;
	int m_muxtype;
	CComboBox m_muxctrl;

	CMediaType m_mtv, m_mta, m_mtcv, m_mtca;
	CComPtr<IBaseFilter> m_pVidEnc, m_pAudEnc, m_pMux, m_pDst, m_pAudMux, m_pAudDst;
	CComPtr<IMoniker> m_pVidEncMoniker, m_pAudEncMoniker;
	CComPtr<IBaseFilter> m_pVidBuffer, m_pAudBuffer;

public:
	void SetupVideoControls(CStringW DisplayName, IAMStreamConfig* pAMSC, IAMCrossbar* pAMXB, IAMTVTuner* pAMTuner);
	void SetupVideoControls(CStringW DisplayName, IAMStreamConfig* pAMSC, IAMVfwCaptureDialogs* pAMVfwCD);
	void SetupAudioControls(CStringW DisplayName, IAMStreamConfig* pAMSC, const CInterfaceArray<IAMAudioInputMixer>& pAMAIM);

	bool IsTunerActive();

	bool SetVideoInput(int input);
	bool SetVideoChannel(int channel);
	bool SetAudioInput(int input);

	int GetVideoInput();
	int GetVideoChannel();
	int GetAudioInput();

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	virtual BOOL OnInitDialog();
	virtual void OnOK() {}
	virtual void OnCancel() {}

	DECLARE_MESSAGE_MAP()

public:
	afx_msg void OnDestroy();
	afx_msg void OnVideoInput();
	afx_msg void OnVideoType();
	afx_msg void OnVideoDimension();
	afx_msg void OnOverrideVideoDimension();
	afx_msg void OnAudioInput();
	afx_msg void OnAudioType();
	afx_msg void OnAudioDimension();
	afx_msg void OnRecordVideo();
	afx_msg void OnVideoCodec();
	afx_msg void OnVideoCodecType();
	afx_msg void OnVideoCodecDimension();
	afx_msg void OnRecordAudio();
	afx_msg void OnAudioCodec();
	afx_msg void OnAudioCodecType();
	afx_msg void OnAudioCodecDimension();
	afx_msg void OnOpenFile();
	afx_msg void OnRecord();
	afx_msg void OnEnChangeEdit9();
	afx_msg void OnEnChangeEdit12();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnBnClickedVidAudPreview();
	afx_msg void OnBnClickedCheck7();
	afx_msg void OnCbnSelchangeCombo14();
};

// CPlayerCaptureBar

class CPlayerCaptureBar : public CPlayerBar
{
	DECLARE_DYNAMIC(CPlayerCaptureBar)

public:
	CPlayerCaptureBar(CMainFrame* pMainFrame);
	virtual ~CPlayerCaptureBar();

	BOOL Create(CWnd* pParentWnd, UINT defDockBarID);

public:
	CPlayerCaptureDialog m_capdlg;

protected:
	virtual BOOL PreTranslateMessage(MSG* pMsg);

	DECLARE_MESSAGE_MAP()
};
