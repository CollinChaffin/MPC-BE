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

#include <ffmpeg/libavcodec/dxva_internal.h>

#define DBGLOG_LEVEL 0

#if defined(_DEBUG) && DBGLOG_LEVEL > 0
	#define CHECK_HR_FALSE(x)	hr = ##x; if (FAILED(hr)) { DbgLog((LOG_TRACE, 3, L"DXVA Error : 0x%08x, %s : %i", hr, CString(__FILE__), __LINE__)); return S_FALSE; }
	#define CHECK_HR_FRAME(x)	hr = ##x; if (FAILED(hr)) { DbgLog((LOG_TRACE, 3, L"DXVA Error : 0x%08x, %s : %i", hr, CString(__FILE__), __LINE__)); CHECK_HR_FALSE (EndFrame()); return S_FALSE; }
#else
	#define CHECK_HR_FALSE(x)	hr = ##x; if (FAILED(hr)) { return S_FALSE; }
	#define CHECK_HR_FRAME(x)	hr = ##x; if (FAILED(hr)) { CHECK_HR_FALSE (EndFrame()); return S_FALSE; }
#endif

#define MAX_RETRY_ON_PENDING	50
#define DO_DXVA_PENDING_LOOP(x)	nTry = 0; \
								while (FAILED(hr = x) && nTry < MAX_RETRY_ON_PENDING) \
								{ \
									if (hr != E_PENDING) break; \
									Sleep(3); \
									nTry++; \
								}

class CMPCVideoDecFilter;
struct AVFrame;

class CDXVA2Decoder
{
	struct SampleWrapper {
		CComPtr<IMediaSample>		pSample	= NULL;
	};

	CComPtr<IDirectXVideoDecoder>	m_pDirectXVideoDec;
	DXVA2_DecodeExecuteParams		m_ExecuteParams;

public :
	static CDXVA2Decoder*			CreateDXVA2Decoder(CMPCVideoDecFilter* pFilter, IDirectXVideoDecoder* pDirectXVideoDec, const GUID* guidDecoder, DXVA2_ConfigPictureDecode* pDXVA2Config);
	virtual							~CDXVA2Decoder();

	void							UpdateDXVA2Context();

	void							Flush() { m_dxva_context.report_id = 0; };
	HRESULT							DeliverFrame(int got_picture, REFERENCE_TIME rtStart, REFERENCE_TIME rtStop);

	virtual HRESULT					CopyBitstream(BYTE* pDXVABuffer, UINT& nSize, UINT nDXVASize = UINT_MAX) PURE;
	virtual HRESULT					ProcessDXVAFrame(IMediaSample* pSample) PURE;

	int								get_buffer_dxva(struct AVFrame *pic);
	static void						release_buffer_dxva(void *opaque, uint8_t *data);

protected :
	CDXVA2Decoder(CMPCVideoDecFilter* pFilter, IDirectXVideoDecoder* pDirectXVideoDec, const GUID* guidDecoder, DXVA2_ConfigPictureDecode* pDXVA2Config, int CompressedBuffersSize);

	HRESULT							AddExecuteBuffer(DWORD CompressedBufferType, UINT nSize = 0, void* pBuffer = NULL);
	HRESULT							Execute();
	HRESULT							BeginFrame(IMediaSample* pSampleToDeliver);
	HRESULT							EndFrame();

	HRESULT							DeliverDXVAFrame();
	HRESULT							GetFreeSurfaceIndex(int& nSurfaceIndex, IMediaSample** ppSampleToDeliver);

	HRESULT							GetSampleWrapperData(AVFrame* pFrame, IMediaSample** pSample, REFERENCE_TIME* rtStart, REFERENCE_TIME* rtStop);

	GUID							m_guidDecoder;
	CMPCVideoDecFilter*				m_pFilter;

	DXVA2_ConfigPictureDecode		m_DXVA2Config;

	UINT							m_ctx_pic_num;
	dxva_context					m_dxva_context;
};
