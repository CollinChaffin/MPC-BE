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
#include "DirectVobSub.h"
#include "VSFilter.h"

CDirectVobSub::CDirectVobSub()
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	BYTE* pData;
	UINT nSize;

	m_iSelectedLanguage = 0;
	m_bHideSubtitles = !!theApp.GetProfileInt(ResStr(IDS_R_GENERAL), ResStr(IDS_RG_HIDE), 0);
	m_uSubPictToBuffer = theApp.GetProfileInt(ResStr(IDS_R_GENERAL), ResStr(IDS_RG_SUBPICTTOBUFFER), 10);
	m_bAnimWhenBuffering = !!theApp.GetProfileInt(ResStr(IDS_R_GENERAL), ResStr(IDS_RG_ANIMWHENBUFFERING), 1);
	m_bAllowDropSubPic = !!theApp.GetProfileInt(ResStr(IDS_R_GENERAL), ResStr(IDS_RG_ALLOW_DROPPING_SUBPIC), 1);
	m_bOverridePlacement = !!theApp.GetProfileInt(ResStr(IDS_R_TEXT), ResStr(IDS_RT_OVERRIDEPLACEMENT), 0);
	m_PlacementXperc = theApp.GetProfileInt(ResStr(IDS_R_TEXT), ResStr(IDS_RT_XPERC), 50);
	m_PlacementYperc = theApp.GetProfileInt(ResStr(IDS_R_TEXT), ResStr(IDS_RT_YPERC), 90);
	m_bBufferVobSub = !!theApp.GetProfileInt(ResStr(IDS_R_VOBSUB), ResStr(IDS_RV_BUFFER), 1);
	m_bOnlyShowForcedVobSubs = !!theApp.GetProfileInt(ResStr(IDS_R_VOBSUB), ResStr(IDS_RV_ONLYSHOWFORCEDSUBS), 0);
	m_bPolygonize = !!theApp.GetProfileInt(ResStr(IDS_R_VOBSUB), ResStr(IDS_RV_POLYGONIZE), 0);
	m_defStyle <<= theApp.GetProfileString(ResStr(IDS_R_TEXT), ResStr(IDS_RT_STYLE), L"");
	m_bFlipPicture = !!theApp.GetProfileInt(ResStr(IDS_R_GENERAL), ResStr(IDS_RG_FLIPPICTURE), 0);
	m_bFlipSubtitles = !!theApp.GetProfileInt(ResStr(IDS_R_GENERAL), ResStr(IDS_RG_FLIPSUBTITLES), 0);
	m_bOSD = !!theApp.GetProfileInt(ResStr(IDS_R_GENERAL), ResStr(IDS_RG_SHOWOSDSTATS), 0);
	m_bSaveFullPath = !!theApp.GetProfileInt(ResStr(IDS_R_GENERAL), ResStr(IDS_RG_SAVEFULLPATH), 0);
	m_nReloaderDisableCount = !!theApp.GetProfileInt(ResStr(IDS_R_GENERAL), ResStr(IDS_RG_DISABLERELOADER), 0) ? 1 : 0;
	m_SubtitleDelay = theApp.GetProfileInt(ResStr(IDS_R_TIMING), ResStr(IDS_RTM_SUBTITLEDELAY), 0);
	m_SubtitleSpeedMul = theApp.GetProfileInt(ResStr(IDS_R_TIMING), ResStr(IDS_RTM_SUBTITLESPEEDMUL), 1000);
	m_SubtitleSpeedDiv = theApp.GetProfileInt(ResStr(IDS_R_TIMING), ResStr(IDS_RTM_SUBTITLESPEEDDIV), 1000);
	m_bMediaFPSEnabled = !!theApp.GetProfileInt(ResStr(IDS_R_TIMING), ResStr(IDS_RTM_MEDIAFPSENABLED), 0);
	m_ePARCompensationType = static_cast<CSimpleTextSubtitle::EPARCompensationType>(theApp.GetProfileInt(ResStr(IDS_R_TEXT), ResStr(IDS_RT_AUTOPARCOMPENSATION), 0));
	pData = nullptr;
	if (theApp.GetProfileBinary(ResStr(IDS_R_TIMING), ResStr(IDS_RTM_MEDIAFPS), &pData, &nSize) && pData) {
		if (nSize != sizeof(m_MediaFPS)) {
			m_MediaFPS = 25.0;
		} else {
			memcpy(&m_MediaFPS, pData, sizeof(m_MediaFPS));
		}
		delete [] pData;
	}
	m_ZoomRect.left = m_ZoomRect.top = 0;
	m_ZoomRect.right = m_ZoomRect.bottom = 1;

	m_bForced = false;
}

CDirectVobSub::~CDirectVobSub()
{
	CAutoLock cAutoLock(&m_propsLock);
}

STDMETHODIMP CDirectVobSub::get_FileName(WCHAR* fn)
{
	CAutoLock cAutoLock(&m_propsLock);

	if (!fn) {
		return E_POINTER;
	}

	wcscpy(fn, m_FileName);

	return S_OK;
}

STDMETHODIMP CDirectVobSub::put_FileName(WCHAR* fn)
{
	CAutoLock cAutoLock(&m_propsLock);

	if (!fn) {
		return E_POINTER;
	}

	CString tmp = fn;
	if (!m_FileName.Left(m_FileName.ReverseFind('.')+1).CompareNoCase(tmp.Left(tmp.ReverseFind('.')+1))) {
		return S_FALSE;
	}

	m_FileName = fn;

	return S_OK;
}

STDMETHODIMP CDirectVobSub::get_LanguageCount(int* nLangs)
{
	CAutoLock cAutoLock(&m_propsLock);

	return nLangs ? *nLangs = 0, S_OK : E_POINTER;
}

STDMETHODIMP CDirectVobSub::get_LanguageName(int iLanguage, WCHAR** ppName)
{
	return S_OK;
}

STDMETHODIMP CDirectVobSub::get_SelectedLanguage(int* iSelected)
{
	CAutoLock cAutoLock(&m_propsLock);

	return iSelected ? *iSelected = m_iSelectedLanguage, S_OK : E_POINTER;
}

STDMETHODIMP CDirectVobSub::put_SelectedLanguage(int iSelected)
{
	CAutoLock cAutoLock(&m_propsLock);

	if (m_iSelectedLanguage == iSelected) {
		return S_FALSE;
	}

	int nCount;
	if (FAILED(get_LanguageCount(&nCount))
			|| iSelected < 0
			|| iSelected >= nCount) {
		return E_FAIL;
	}

	m_iSelectedLanguage = iSelected;

	return S_OK;
}

STDMETHODIMP CDirectVobSub::get_HideSubtitles(bool* fHideSubtitles)
{
	CAutoLock cAutoLock(&m_propsLock);

	return fHideSubtitles ? *fHideSubtitles = m_bHideSubtitles, S_OK : E_POINTER;
}

STDMETHODIMP CDirectVobSub::put_HideSubtitles(bool fHideSubtitles)
{
	CAutoLock cAutoLock(&m_propsLock);

	if (m_bHideSubtitles == fHideSubtitles) {
		return S_FALSE;
	}

	m_bHideSubtitles = fHideSubtitles;

	return S_OK;
}

// deprecated
STDMETHODIMP CDirectVobSub::get_PreBuffering(bool* fDoPreBuffering)
{
	CAutoLock cAutoLock(&m_propsLock);

	return fDoPreBuffering ? *fDoPreBuffering = (m_uSubPictToBuffer > 0), S_OK : E_POINTER;
}

// deprecated
STDMETHODIMP CDirectVobSub::put_PreBuffering(bool fDoPreBuffering)
{
	CAutoLock cAutoLock(&m_propsLock);

	if ((m_uSubPictToBuffer > 0) == fDoPreBuffering) {
		return S_FALSE;
	}

	m_uSubPictToBuffer = fDoPreBuffering ? 4 : 0; // 4 is the default value for SubPictToBuffer

	return S_OK;
}

STDMETHODIMP CDirectVobSub::get_SubPictToBuffer(unsigned int* uSubPictToBuffer)
{
	CAutoLock cAutoLock(&m_propsLock);

	return uSubPictToBuffer ? *uSubPictToBuffer = m_uSubPictToBuffer, S_OK : E_POINTER;
}

STDMETHODIMP CDirectVobSub::put_SubPictToBuffer(unsigned int uSubPictToBuffer)
{
	CAutoLock cAutoLock(&m_propsLock);

	if (m_uSubPictToBuffer == uSubPictToBuffer) {
		return S_FALSE;
	}

	m_uSubPictToBuffer = uSubPictToBuffer;

	return S_OK;
}

STDMETHODIMP CDirectVobSub::get_AnimWhenBuffering(bool* fAnimWhenBuffering)
{
	CAutoLock cAutoLock(&m_propsLock);

	return fAnimWhenBuffering ? *fAnimWhenBuffering = m_bAnimWhenBuffering, S_OK : E_POINTER;
}

STDMETHODIMP CDirectVobSub::put_AnimWhenBuffering(bool fAnimWhenBuffering)
{
	CAutoLock cAutoLock(&m_propsLock);

	if (m_bAnimWhenBuffering == fAnimWhenBuffering) {
		return S_FALSE;
	}

	m_bAnimWhenBuffering = fAnimWhenBuffering;

	return S_OK;
}

STDMETHODIMP CDirectVobSub::get_AllowDropSubPic(bool* fAllowDropSubPic)
{
	CAutoLock cAutoLock(&m_propsLock);

	return fAllowDropSubPic ? *fAllowDropSubPic = m_bAllowDropSubPic, S_OK : E_POINTER;
}

STDMETHODIMP CDirectVobSub::put_AllowDropSubPic(bool fAllowDropSubPic)
{
	CAutoLock cAutoLock(&m_propsLock);

	if (m_bAllowDropSubPic == fAllowDropSubPic) {
		return S_FALSE;
	}

	m_bAllowDropSubPic = fAllowDropSubPic;

	return S_OK;
}

STDMETHODIMP CDirectVobSub::get_Placement(bool* fOverridePlacement, int* xperc, int* yperc)
{
	CAutoLock cAutoLock(&m_propsLock);

	if (fOverridePlacement) {
		*fOverridePlacement = m_bOverridePlacement;
	}
	if (xperc) {
		*xperc = m_PlacementXperc;
	}
	if (yperc) {
		*yperc = m_PlacementYperc;
	}

	return S_OK;
}

STDMETHODIMP CDirectVobSub::put_Placement(bool fOverridePlacement, int xperc, int yperc)
{
	CAutoLock cAutoLock(&m_propsLock);

	if (m_bOverridePlacement == fOverridePlacement && m_PlacementXperc == xperc && m_PlacementYperc == yperc) {
		return S_FALSE;
	}

	m_bOverridePlacement = fOverridePlacement;
	m_PlacementXperc = xperc;
	m_PlacementYperc = yperc;

	return S_OK;
}

STDMETHODIMP CDirectVobSub::get_VobSubSettings(bool* fBuffer, bool* fOnlyShowForcedSubs, bool* fPolygonize)
{
	CAutoLock cAutoLock(&m_propsLock);

	if (fBuffer) {
		*fBuffer = m_bBufferVobSub;
	}
	if (fOnlyShowForcedSubs) {
		*fOnlyShowForcedSubs = m_bOnlyShowForcedVobSubs;
	}
	if (fPolygonize) {
		*fPolygonize = m_bPolygonize;
	}

	return S_OK;
}

STDMETHODIMP CDirectVobSub::put_VobSubSettings(bool fBuffer, bool fOnlyShowForcedSubs, bool fPolygonize)
{
	CAutoLock cAutoLock(&m_propsLock);

	if (m_bBufferVobSub == fBuffer && m_bOnlyShowForcedVobSubs == fOnlyShowForcedSubs && m_bPolygonize == fPolygonize) {
		return S_FALSE;
	}

	m_bBufferVobSub = fBuffer;
	m_bOnlyShowForcedVobSubs = fOnlyShowForcedSubs;
	m_bPolygonize = fPolygonize;

	return S_OK;
}

STDMETHODIMP CDirectVobSub::get_TextSettings(void* lf, int lflen, COLORREF* color, bool* fShadow, bool* fOutline, bool* fAdvancedRenderer)
{
	CAutoLock cAutoLock(&m_propsLock);

	if (lf) {
		if (lflen == sizeof(LOGFONTA)) {
			strncpy_s(((LOGFONTA*)lf)->lfFaceName, LF_FACESIZE, CStringA(m_defStyle.fontName), _TRUNCATE);
		} else if (lflen == sizeof(LOGFONTW)) {
			wcsncpy_s(((LOGFONTW*)lf)->lfFaceName, LF_FACESIZE, CStringW(m_defStyle.fontName), _TRUNCATE);
		} else {
			return E_INVALIDARG;
		}

		((LOGFONT*)lf)->lfCharSet = m_defStyle.charSet;
		((LOGFONT*)lf)->lfItalic = m_defStyle.fItalic;
		((LOGFONT*)lf)->lfHeight = m_defStyle.fontSize;
		((LOGFONT*)lf)->lfWeight = m_defStyle.fontWeight;
		((LOGFONT*)lf)->lfStrikeOut = m_defStyle.fStrikeOut;
		((LOGFONT*)lf)->lfUnderline = m_defStyle.fUnderline;
	}

	if (color) {
		*color = m_defStyle.colors[0];
	}
	if (fShadow) {
		*fShadow = (m_defStyle.shadowDepthX!=0) || (m_defStyle.shadowDepthY!=0);
	}
	if (fOutline) {
		*fOutline = (m_defStyle.outlineWidthX+m_defStyle.outlineWidthY)>0;
	}
	if (fAdvancedRenderer) {
		*fAdvancedRenderer = m_bAdvancedRenderer;
	}

	return S_OK;
}

STDMETHODIMP CDirectVobSub::put_TextSettings(void* lf, int lflen, COLORREF color, bool fShadow, bool fOutline, bool fAdvancedRenderer)
{
	CAutoLock cAutoLock(&m_propsLock);

	if (lf) {
		if (lflen == sizeof(LOGFONTA)) {
			m_defStyle.fontName = ((LOGFONTA*)lf)->lfFaceName;
		} else if (lflen == sizeof(LOGFONTW)) {
			m_defStyle.fontName = ((LOGFONTW*)lf)->lfFaceName;
		} else {
			return E_INVALIDARG;
		}

		m_defStyle.charSet = ((LOGFONT*)lf)->lfCharSet;
		m_defStyle.fItalic = !!((LOGFONT*)lf)->lfItalic;
		m_defStyle.fontSize = ((LOGFONT*)lf)->lfHeight;
		m_defStyle.fontWeight = ((LOGFONT*)lf)->lfWeight;
		m_defStyle.fStrikeOut = !!((LOGFONT*)lf)->lfStrikeOut;
		m_defStyle.fUnderline = !!((LOGFONT*)lf)->lfUnderline;

		if (m_defStyle.fontSize < 0) {
			HDC hdc = ::GetDC(0);
			m_defStyle.fontSize = -m_defStyle.fontSize * 72 / GetDeviceCaps(hdc, LOGPIXELSY);
			::ReleaseDC(0, hdc);
		}

	}

	m_defStyle.colors[0] = color;
	m_defStyle.shadowDepthX = m_defStyle.shadowDepthY = fShadow?2:0;
	m_defStyle.outlineWidthX = m_defStyle.outlineWidthY = fOutline?2:0;

	return S_OK;

}

STDMETHODIMP CDirectVobSub::get_Flip(bool* fPicture, bool* fSubtitles)
{
	CAutoLock cAutoLock(&m_propsLock);

	if (fPicture) {
		*fPicture = m_bFlipPicture;
	}
	if (fSubtitles) {
		*fSubtitles = m_bFlipSubtitles;
	}

	return S_OK;
}

STDMETHODIMP CDirectVobSub::put_Flip(bool fPicture, bool fSubtitles)
{
	CAutoLock cAutoLock(&m_propsLock);

	if (m_bFlipPicture == fPicture && m_bFlipSubtitles == fSubtitles) {
		return S_FALSE;
	}

	m_bFlipPicture = fPicture;
	m_bFlipSubtitles = fSubtitles;

	return S_OK;
}

STDMETHODIMP CDirectVobSub::get_OSD(bool* fOSD)
{
	CAutoLock cAutoLock(&m_propsLock);

	return fOSD ? *fOSD = m_bOSD, S_OK : E_POINTER;
}

STDMETHODIMP CDirectVobSub::put_OSD(bool fOSD)
{
	CAutoLock cAutoLock(&m_propsLock);

	if (m_bOSD == fOSD) {
		return S_FALSE;
	}

	m_bOSD = fOSD;

	return S_OK;
}

STDMETHODIMP CDirectVobSub::get_SaveFullPath(bool* fSaveFullPath)
{
	CAutoLock cAutoLock(&m_propsLock);

	return fSaveFullPath ? *fSaveFullPath = m_bSaveFullPath, S_OK : E_POINTER;
}

STDMETHODIMP CDirectVobSub::put_SaveFullPath(bool fSaveFullPath)
{
	CAutoLock cAutoLock(&m_propsLock);

	if (m_bSaveFullPath == fSaveFullPath) {
		return S_FALSE;
	}

	m_bSaveFullPath = fSaveFullPath;

	return S_OK;
}

STDMETHODIMP CDirectVobSub::get_SubtitleTiming(int* delay, int* speedmul, int* speeddiv)
{
	CAutoLock cAutoLock(&m_propsLock);

	if (delay) {
		*delay = m_SubtitleDelay;
	}
	if (speedmul) {
		*speedmul = m_SubtitleSpeedMul;
	}
	if (speeddiv) {
		*speeddiv = m_SubtitleSpeedDiv;
	}

	return S_OK;
}

STDMETHODIMP CDirectVobSub::put_SubtitleTiming(int delay, int speedmul, int speeddiv)
{
	CAutoLock cAutoLock(&m_propsLock);

	if (m_SubtitleDelay == delay && m_SubtitleSpeedMul == speedmul && m_SubtitleSpeedDiv == speeddiv) {
		return S_FALSE;
	}

	m_SubtitleDelay = delay;
	m_SubtitleSpeedMul = speedmul;
	if (speeddiv > 0) {
		m_SubtitleSpeedDiv = speeddiv;
	}

	return S_OK;
}

STDMETHODIMP CDirectVobSub::get_MediaFPS(bool* fEnabled, double* fps)
{
	CAutoLock cAutoLock(&m_propsLock);

	if (fEnabled) {
		*fEnabled = m_bMediaFPSEnabled;
	}
	if (fps) {
		*fps = m_MediaFPS;
	}

	return S_OK;
}

STDMETHODIMP CDirectVobSub::put_MediaFPS(bool fEnabled, double fps)
{
	CAutoLock cAutoLock(&m_propsLock);

	if (m_bMediaFPSEnabled == fEnabled && m_MediaFPS == fps) {
		return S_FALSE;
	}

	m_bMediaFPSEnabled = fEnabled;
	if (fps > 0) {
		m_MediaFPS = fps;
	}

	return S_OK;
}

STDMETHODIMP CDirectVobSub::get_ZoomRect(NORMALIZEDRECT* rect)
{
	CAutoLock cAutoLock(&m_propsLock);

	if (!rect) {
		return E_POINTER;
	}

	*rect = m_ZoomRect;

	return S_OK;
}

STDMETHODIMP CDirectVobSub::put_ZoomRect(NORMALIZEDRECT* rect)
{
	CAutoLock cAutoLock(&m_propsLock);

	if (!rect) {
		return E_POINTER;
	}

	if (!memcmp(&m_ZoomRect, rect, sizeof(m_ZoomRect))) {
		return S_FALSE;
	}

	m_ZoomRect = *rect;

	return S_OK;
}

STDMETHODIMP CDirectVobSub::UpdateRegistry()
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	CAutoLock cAutoLock(&m_propsLock);

	theApp.WriteProfileInt(ResStr(IDS_R_GENERAL), ResStr(IDS_RG_HIDE), m_bHideSubtitles);
	theApp.WriteProfileInt(ResStr(IDS_R_GENERAL), ResStr(IDS_RG_SUBPICTTOBUFFER), m_uSubPictToBuffer);
	theApp.WriteProfileInt(ResStr(IDS_R_GENERAL), ResStr(IDS_RG_ANIMWHENBUFFERING), m_bAnimWhenBuffering);
	theApp.WriteProfileInt(ResStr(IDS_R_GENERAL), ResStr(IDS_RG_ALLOW_DROPPING_SUBPIC), m_bAllowDropSubPic);
	theApp.WriteProfileInt(ResStr(IDS_R_TEXT), ResStr(IDS_RT_OVERRIDEPLACEMENT), m_bOverridePlacement);
	theApp.WriteProfileInt(ResStr(IDS_R_TEXT), ResStr(IDS_RT_XPERC), m_PlacementXperc);
	theApp.WriteProfileInt(ResStr(IDS_R_TEXT), ResStr(IDS_RT_YPERC), m_PlacementYperc);
	theApp.WriteProfileInt(ResStr(IDS_R_VOBSUB), ResStr(IDS_RV_BUFFER), m_bBufferVobSub);
	theApp.WriteProfileInt(ResStr(IDS_R_VOBSUB), ResStr(IDS_RV_ONLYSHOWFORCEDSUBS), m_bOnlyShowForcedVobSubs);
	theApp.WriteProfileInt(ResStr(IDS_R_VOBSUB), ResStr(IDS_RV_POLYGONIZE), m_bPolygonize);
	CString style;
	theApp.WriteProfileString(ResStr(IDS_R_TEXT), ResStr(IDS_RT_STYLE), style <<= m_defStyle);
	theApp.WriteProfileInt(ResStr(IDS_R_GENERAL), ResStr(IDS_RG_FLIPPICTURE), m_bFlipPicture);
	theApp.WriteProfileInt(ResStr(IDS_R_GENERAL), ResStr(IDS_RG_FLIPSUBTITLES), m_bFlipSubtitles);
	theApp.WriteProfileInt(ResStr(IDS_R_GENERAL), ResStr(IDS_RG_SHOWOSDSTATS), m_bOSD);
	theApp.WriteProfileInt(ResStr(IDS_R_GENERAL), ResStr(IDS_RG_SAVEFULLPATH), m_bSaveFullPath);
	theApp.WriteProfileInt(ResStr(IDS_R_TIMING), ResStr(IDS_RTM_SUBTITLEDELAY), m_SubtitleDelay);
	theApp.WriteProfileInt(ResStr(IDS_R_TIMING), ResStr(IDS_RTM_SUBTITLESPEEDMUL), m_SubtitleSpeedMul);
	theApp.WriteProfileInt(ResStr(IDS_R_TIMING), ResStr(IDS_RTM_SUBTITLESPEEDDIV), m_SubtitleSpeedDiv);
	theApp.WriteProfileInt(ResStr(IDS_R_TIMING), ResStr(IDS_RTM_MEDIAFPSENABLED), m_bMediaFPSEnabled);
	theApp.WriteProfileBinary(ResStr(IDS_R_TIMING), ResStr(IDS_RTM_MEDIAFPS), (BYTE*)&m_MediaFPS, sizeof(m_MediaFPS));
	theApp.WriteProfileInt(ResStr(IDS_R_TEXT), ResStr(IDS_RT_AUTOPARCOMPENSATION), m_ePARCompensationType);

	return S_OK;
}

STDMETHODIMP CDirectVobSub::HasConfigDialog(int iSelected)
{
	return E_NOTIMPL;
}

STDMETHODIMP CDirectVobSub::ShowConfigDialog(int iSelected, HWND hWndParent)
{
	return E_NOTIMPL;
}

STDMETHODIMP CDirectVobSub::IsSubtitleReloaderLocked(bool* fLocked)
{
	CAutoLock cAutoLock(&m_propsLock);

	if (!fLocked) {
		return E_POINTER;
	}

	bool fDisabled;
	get_SubtitleReloader(&fDisabled);

	*fLocked = fDisabled || m_nReloaderDisableCount > 0;

	return S_OK;
}

STDMETHODIMP CDirectVobSub::LockSubtitleReloader(bool fLock)
{
	CAutoLock cAutoLock(&m_propsLock);

	if (fLock) {
		m_nReloaderDisableCount++;
	} else {
		m_nReloaderDisableCount--;
	}

	ASSERT(m_nReloaderDisableCount >= 0);
	if (m_nReloaderDisableCount < 0) {
		m_nReloaderDisableCount = 0;
	}

	return S_OK;
}

STDMETHODIMP CDirectVobSub::get_SubtitleReloader(bool* fDisabled)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	CAutoLock cAutoLock(&m_propsLock);

	if (fDisabled) {
		*fDisabled = !!theApp.GetProfileInt(ResStr(IDS_R_GENERAL), ResStr(IDS_RG_DISABLERELOADER), 0);
	}

	return S_OK;
}

STDMETHODIMP CDirectVobSub::put_SubtitleReloader(bool fDisable)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	CAutoLock cAutoLock(&m_propsLock);

	bool b;
	get_SubtitleReloader(&b);
	if (b == fDisable) {
		return S_FALSE;
	}

	theApp.WriteProfileInt(ResStr(IDS_R_GENERAL), ResStr(IDS_RG_DISABLERELOADER), fDisable);

	return S_OK;
}

STDMETHODIMP CDirectVobSub::get_ExtendPicture(int* horizontal, int* vertical, int* resx2, int* resx2minw, int* resx2minh)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	CAutoLock cAutoLock(&m_propsLock);

	if (horizontal) {
		*horizontal = theApp.GetProfileInt(ResStr(IDS_R_GENERAL), ResStr(IDS_RG_MOD32FIX), 0) & 1;
	}
	if (vertical) {
		*vertical = theApp.GetProfileInt(ResStr(IDS_R_GENERAL), ResStr(IDS_RG_EXTPIC), 0);
	}
	if (resx2) {
		*resx2 = 0;//theApp.GetProfileInt(ResStr(IDS_R_GENERAL), ResStr(IDS_RG_RESX2), 2) & 3;
	}
	if (resx2minw) {
		*resx2minw = theApp.GetProfileInt(ResStr(IDS_R_GENERAL), ResStr(IDS_RG_RESX2MINW), 384);
	}
	if (resx2minh) {
		*resx2minh = theApp.GetProfileInt(ResStr(IDS_R_GENERAL), ResStr(IDS_RG_RESX2MINH), 288);
	}

	return S_OK;
}

STDMETHODIMP CDirectVobSub::put_ExtendPicture(int horizontal, int vertical, int resx2, int resx2minw, int resx2minh)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	CAutoLock cAutoLock(&m_propsLock);

	int i[5];
	get_ExtendPicture(i, i+1, i+2, i+3, i+4);
	if (i[0] == horizontal && i[1] == vertical && i[2] == resx2 && i[3] == resx2minw && i[4] == resx2minh) {
		return S_FALSE;
	}

	theApp.WriteProfileInt(ResStr(IDS_R_GENERAL), ResStr(IDS_RG_MOD32FIX), horizontal & 1);
	theApp.WriteProfileInt(ResStr(IDS_R_GENERAL), ResStr(IDS_RG_EXTPIC), vertical);
	theApp.WriteProfileInt(ResStr(IDS_R_GENERAL), ResStr(IDS_RG_RESX2), resx2 & 3);
	theApp.WriteProfileInt(ResStr(IDS_R_GENERAL), ResStr(IDS_RG_RESX2MINW), resx2minw);
	theApp.WriteProfileInt(ResStr(IDS_R_GENERAL), ResStr(IDS_RG_RESX2MINH), resx2minh);

	return S_OK;
}

STDMETHODIMP CDirectVobSub::get_LoadSettings(int* level, bool* fExternalLoad, bool* fWebLoad, bool* fEmbeddedLoad)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	CAutoLock cAutoLock(&m_propsLock);

	if (level) {
		*level = theApp.GetProfileInt(ResStr(IDS_R_GENERAL), ResStr(IDS_RG_LOADLEVEL), 0) & 3;
	}
	if (fExternalLoad) {
		*fExternalLoad = !!theApp.GetProfileInt(ResStr(IDS_R_GENERAL), ResStr(IDS_RG_EXTERNALLOAD), 1);
	}
	if (fWebLoad) {
		*fWebLoad = !!theApp.GetProfileInt(ResStr(IDS_R_GENERAL), ResStr(IDS_RG_WEBLOAD), 0);
	}
	if (fEmbeddedLoad) {
		*fEmbeddedLoad = !!theApp.GetProfileInt(ResStr(IDS_R_GENERAL), ResStr(IDS_RG_EMBEDDEDLOAD), 1);
	}

	return S_OK;
}

STDMETHODIMP CDirectVobSub::put_LoadSettings(int level, bool fExternalLoad, bool fWebLoad, bool fEmbeddedLoad)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	CAutoLock cAutoLock(&m_propsLock);

	int i;
	bool b[3];
	get_LoadSettings(&i, b, b+1, b+2);
	if (i == level && b[0] == fExternalLoad && b[1] == fWebLoad && b[2] == fEmbeddedLoad) {
		return S_FALSE;
	}

	theApp.WriteProfileInt(ResStr(IDS_R_GENERAL), ResStr(IDS_RG_LOADLEVEL), level & 3);
	theApp.WriteProfileInt(ResStr(IDS_R_GENERAL), ResStr(IDS_RG_EXTERNALLOAD), fExternalLoad);
	theApp.WriteProfileInt(ResStr(IDS_R_GENERAL), ResStr(IDS_RG_WEBLOAD), fWebLoad);
	theApp.WriteProfileInt(ResStr(IDS_R_GENERAL), ResStr(IDS_RG_EMBEDDEDLOAD), fEmbeddedLoad);

	return S_OK;
}

// IDirectVobSub2

STDMETHODIMP CDirectVobSub::AdviseSubClock(ISubClock* pSubClock)
{
	m_pSubClock = pSubClock;
	return S_OK;
}

STDMETHODIMP_(bool) CDirectVobSub::get_Forced()
{
	return m_bForced;
}

STDMETHODIMP CDirectVobSub::put_Forced(bool fForced)
{
	m_bForced = fForced;
	return S_OK;
}

STDMETHODIMP CDirectVobSub::get_TextSettings(STSStyle* pDefStyle)
{
	CheckPointer(pDefStyle, E_POINTER);

	CAutoLock cAutoLock(&m_propsLock);

	*pDefStyle = m_defStyle;

	return S_OK;
}

STDMETHODIMP CDirectVobSub::put_TextSettings(STSStyle* pDefStyle)
{
	CheckPointer(pDefStyle, E_POINTER);

	CAutoLock cAutoLock(&m_propsLock);

	if (m_defStyle == *pDefStyle) {
		return S_FALSE;
	}

	m_defStyle = *pDefStyle;

	return S_OK;
}

STDMETHODIMP CDirectVobSub::get_AspectRatioSettings(CSimpleTextSubtitle::EPARCompensationType* ePARCompensationType)
{
	CAutoLock cAutoLock(&m_propsLock);

	*ePARCompensationType = m_ePARCompensationType;

	return S_OK;
}

STDMETHODIMP CDirectVobSub::put_AspectRatioSettings(CSimpleTextSubtitle::EPARCompensationType* ePARCompensationType)
{
	CAutoLock cAutoLock(&m_propsLock);

	m_ePARCompensationType = *ePARCompensationType;

	return S_OK;
}

// IFilterVersion

STDMETHODIMP_(DWORD) CDirectVobSub::GetFilterVersion()
{
	return 0x0234;
}
