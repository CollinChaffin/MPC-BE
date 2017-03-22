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

#pragma once

#include "DX9RenderingEngine.h"
#include <dx/d3dx9.h>
#include "CPUUsage.h"
#include "GPUUsage.h"
#include "MemUsage.h"
#include <deque>
#include <mutex>
#include <vector>
#include <MediaOffset3D.h>

#define VMRBITMAP_UPDATE 0x80000000

#define NB_JITTER        126

extern bool g_bNoDuration;
extern bool g_bExternalSubtitleTime;

class CFocusThread;

namespace DSObjects
{

	class CDX9AllocatorPresenter
		: public CDX9RenderingEngine
		, public ID3DFullscreenControl
	{
	public:
		CCritSec	m_VMR9AlphaBitmapLock;
		void		UpdateAlphaBitmap();

	protected:
		UINT	m_CurrentAdapter;
		UINT	m_AdapterCount;

		bool	m_bAlternativeVSync;
		bool	m_bCompositionEnabled;
		int		m_OrderedPaint;
		int		m_VSyncMode;
		bool	m_bDesktopCompositionDisabled;
		bool	m_bIsFullscreen;
		bool	m_bNeedCheckSample;
		DWORD	m_MainThreadId;

		CAffectingRenderersSettings m_LastAffectingSettings;

		HMODULE m_hDWMAPI;
		HRESULT (__stdcall * m_pDwmIsCompositionEnabled)(__out BOOL* pfEnabled);
		HRESULT (__stdcall * m_pDwmEnableComposition)(UINT uCompositionAction);
		HMODULE m_hD3D9;
		HRESULT (__stdcall * m_pDirect3DCreate9Ex)(UINT SDKVersion, IDirect3D9Ex**);

		CCritSec m_RenderLock;

		CComPtr<IDirectDraw> m_pDirectDraw;

		CComPtr<IDirect3DTexture9>	m_pOSDTexture;
		CComPtr<IDirect3DSurface9>	m_pOSDSurface;
		CComPtr<ID3DXLine>			m_pLine;
		CComPtr<ID3DXFont>			m_pFont;
		CComPtr<ID3DXSprite>		m_pSprite;

		bool SettingsNeedResetDevice();

		void LockD3DDevice();
		void UnlockD3DDevice();

		HRESULT CreateDevice(CString &_Error);
		HRESULT AllocSurfaces();
		void DeleteSurfaces();

		HRESULT ResetD3D9Device();
		HRESULT InitializeISR(CString& _Error, const CSize& desktopSize);

		LONGLONG m_LastAdapterCheck;
		UINT GetAdapter(IDirect3D9 *pD3D);
		DWORD GetVertexProcessing();

		bool GetVBlank(int &_ScanLine, int &_bInVBlank, bool _bMeasureTime);
		bool WaitForVBlankRange(int &_RasterStart, int _RasterEnd, bool _bWaitIfInside, bool _bNeedAccurate, bool _bMeasure, bool &_bTakenLock);
		bool WaitForVBlank(bool &_Waited, bool &_bTakenLock);
		int GetVBlackPos();
		void CalculateJitter(LONGLONG PerformanceCounter);
		virtual void OnVBlankFinished(bool fAll, LONGLONG PerformanceCounter) {}

		// Casimir666
		void				ResetStats();
		void				DrawStats();
		virtual void		OnResetDevice() {};
		void				SendResetRequest();

		double GetFrameTime();
		double GetFrameRate();

		int						m_nTearingPos;
		VMR9AlphaBitmap			m_VMR9AlphaBitmap;
		CAutoVectorPtr<BYTE>	m_VMR9AlphaBitmapData;
		CRect					m_VMR9AlphaBitmapRect;
		int						m_VMR9AlphaBitmapWidthBytes;

		HRESULT (__stdcall *m_pD3DXLoadSurfaceFromMemory)(
			_In_       LPDIRECT3DSURFACE9 pDestSurface,
			_In_ const PALETTEENTRY       *pDestPalette,
			_In_ const RECT               *pDestRect,
			_In_       LPCVOID            pSrcMemory,
			_In_       D3DFORMAT          SrcFormat,
			_In_       UINT               SrcPitch,
			_In_ const PALETTEENTRY       *pSrcPalette,
			_In_ const RECT               *pSrcRect,
			_In_       DWORD              Filter,
			_In_       D3DCOLOR           ColorKey
		);
		HRESULT (__stdcall *m_pD3DXCreateLine)(
			_In_  LPDIRECT3DDEVICE9 pDevice,
			_Out_ LPD3DXLINE        *ppLine
		);
		HRESULT (__stdcall *m_pD3DXCreateFont)(
			_In_  LPDIRECT3DDEVICE9 pDevice,
			_In_  INT               Height,
			_In_  UINT              Width,
			_In_  UINT              Weight,
			_In_  UINT              MipLevels,
			_In_  BOOL              Italic,
			_In_  DWORD             CharSet,
			_In_  DWORD             OutputPrecision,
			_In_  DWORD             Quality,
			_In_  DWORD             PitchAndFamily,
			_In_  LPCTSTR           pFacename,
			_Out_ LPD3DXFONT        *ppFont
		);
		HRESULT (__stdcall *m_pD3DXCreateSprite)(
			_In_  LPDIRECT3DDEVICE9 pDevice,
			_Out_ LPD3DXSPRITE      *ppSprite
		);

		long					m_nUsedBuffer;

		double					m_fAvrFps;						// Estimate the real FPS
		double					m_fJitterStdDev;				// Estimate the Jitter std dev
		int						m_iJitterMean;
		double					m_fSyncOffsetStdDev;
		double					m_fSyncOffsetAvr;
		double					m_DetectedRefreshRate;

		CCritSec				m_RefreshRateLock;
		double					m_DetectedRefreshTime;
		double					m_DetectedRefreshTimePrim;
		double					m_DetectedScanlineTime;
		double					m_DetectedScanlineTimePrim;
		double					m_DetectedScanlinesPerFrame;

		double GetRefreshRate() {
			if (m_DetectedRefreshRate) {
				return m_DetectedRefreshRate;
			}
			return m_refreshRate;
		}

		LONG GetScanLines() {
			if (m_DetectedRefreshRate) {
				return (LONG)m_DetectedScanlinesPerFrame;
			}
			return m_ScreenSize.cy;
		}

		double					m_ldDetectedRefreshRateList[100];
		double					m_ldDetectedScanlineRateList[100];
		int						m_DetectedRefreshRatePos;
		bool					m_bSyncStatsAvailable;
		int						m_pJitter [NB_JITTER];			// Jitter buffer for stats
		LONGLONG				m_pllSyncOffset [NB_JITTER];	// Sync offset time stats
		LONGLONG				m_llLastPerf;
		LONGLONG				m_JitterStdDev;
		LONGLONG				m_MaxJitter;
		LONGLONG				m_MinJitter;
		LONGLONG				m_MaxSyncOffset;
		LONGLONG				m_MinSyncOffset;
		unsigned				m_nNextJitter;
		unsigned				m_nNextSyncOffset;
		REFERENCE_TIME			m_rtTimePerFrame;
		double					m_DetectedFrameRate;
		double					m_DetectedFrameTime;
		double					m_DetectedFrameTimeStdDev;
		bool					m_DetectedLock;
		LONGLONG				m_DetectedFrameTimeHistory[60];
		double					m_DetectedFrameTimeHistoryHistory[500];
		int						m_DetectedFrameTimePos;

		double					m_TextScale;

		int						m_VBlankEndWait;
		int						m_VBlankStartWait;
		LONGLONG				m_VBlankWaitTime;
		LONGLONG				m_VBlankLockTime;
		int						m_VBlankMin;
		int						m_VBlankMinCalc;
		int						m_VBlankMax;
		int						m_VBlankEndPresent;
		LONGLONG				m_VBlankStartMeasureTime;
		int						m_VBlankStartMeasure;

		LONGLONG				m_PresentWaitTime;
		LONGLONG				m_PresentWaitTimeMin;
		LONGLONG				m_PresentWaitTimeMax;

		LONGLONG				m_PaintTime;
		LONGLONG				m_PaintTimeMin;
		LONGLONG				m_PaintTimeMax;

		LONGLONG				m_WaitForGPUTime;

		LONGLONG				m_RasterStatusWaitTime;
		LONGLONG				m_RasterStatusWaitTimeMin;
		LONGLONG				m_RasterStatusWaitTimeMax;
		LONGLONG				m_RasterStatusWaitTimeMaxCalc;

		double					m_ClockDiffCalc;
		double					m_ClockDiffPrim;
		double					m_ClockDiff;

		double					m_TimeChangeHistory[100];
		double					m_ClockChangeHistory[100];
		int						m_ClockTimeChangeHistoryPos;
		double					m_ModeratedTimeSpeed;
		double					m_ModeratedTimeSpeedPrim;
		double					m_ModeratedTimeSpeedDiff;

		bool					m_bCorrectedFrameTime;
		int						m_FrameTimeCorrection;
		LONGLONG				m_LastFrameDuration;
		LONGLONG				m_LastSampleTime;

		CString					m_strMixerFmtIn;
		CString					m_strMixerFmtOut;
		CString					m_strSurfaceFmt;
		CString					m_strBackbufferFmt;

		CString					m_D3D9Device;
		CString					m_D3D9DeviceName;

		CString					m_Decoder;

		void					FillAddingField(CComPtr<IPin> pPin, CMediaType* mt);

		CString					m_MonitorName;
		UINT16					m_nMonitorHorRes, m_nMonitorVerRes;

		CRect					m_rcMonitor;

		D3DPRESENT_PARAMETERS	m_d3dpp;

		CCPUUsage				m_CPUUsage;
		CGPUUsage				m_GPUUsage;
		CMemUsage				m_MemUsage;

		CFocusThread*			m_FocusThread;

		bool					m_bMVC_Base_View_R_flag;
		int						m_nStereoOffsetInPixels;

		CComQIPtr<IAMStreamSelect> m_pSS;
		int                        m_nCurrentSubtitlesStream;
		std::vector<int>           m_stereo_subtitle_offset_ids;
		std::deque<MediaOffset3D>  m_mediaOffsetQueue;
		std::mutex                 m_mutexOffsetQueue;

	public:
		CDX9AllocatorPresenter(HWND hWnd, bool bFullscreen, HRESULT& hr, bool bIsEVR, CString &_Error);
		~CDX9AllocatorPresenter();

		// ISubPicAllocatorPresenter3
		STDMETHODIMP_(bool) Paint(bool fAll);
		STDMETHODIMP GetDIB(BYTE* lpDib, DWORD* size);
		STDMETHODIMP ClearPixelShaders(int target);
		STDMETHODIMP AddPixelShader(int target, LPCSTR sourceCode, LPCSTR profile);
		STDMETHODIMP_(bool) ResetDevice();
		STDMETHODIMP_(bool) DisplayChange();

		// ID3DFullscreenControl
		STDMETHODIMP SetD3DFullscreen(bool fEnabled);
		STDMETHODIMP GetD3DFullscreen(bool* pfEnabled);
	};
}
