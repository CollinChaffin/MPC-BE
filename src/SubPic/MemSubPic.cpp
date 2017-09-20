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
#include "MemSubPic.h"

#include <emmintrin.h>

// color conv

unsigned char Clip_base[256*3];
unsigned char* Clip = Clip_base + 256;

const int c2y_cyb = int(0.114*219/255*65536+0.5);
const int c2y_cyg = int(0.587*219/255*65536+0.5);
const int c2y_cyr = int(0.299*219/255*65536+0.5);
const int c2y_cu = int(1.0/2.018*1024+0.5);
const int c2y_cv = int(1.0/1.596*1024+0.5);

int c2y_yb[256];
int c2y_yg[256];
int c2y_yr[256];

const int y2c_cbu = int(2.018*65536+0.5);
const int y2c_cgu = int(0.391*65536+0.5);
const int y2c_cgv = int(0.813*65536+0.5);
const int y2c_crv = int(1.596*65536+0.5);
int y2c_bu[256];
int y2c_gu[256];
int y2c_gv[256];
int y2c_rv[256];

const int cy_cy = int(255.0/219.0*65536+0.5);
const int cy_cy2 = int(255.0/219.0*32768+0.5);

bool fColorConvInitOK = false;

void ColorConvInit()
{
	if (fColorConvInitOK) {
		return;
	}

	int i;

	for (i = 0; i < 256; i++) {
		Clip_base[i] = 0;
		Clip_base[i+256] = i;
		Clip_base[i+512] = 255;
	}

	for (i = 0; i < 256; i++) {
		c2y_yb[i] = c2y_cyb*i;
		c2y_yg[i] = c2y_cyg*i;
		c2y_yr[i] = c2y_cyr*i;

		y2c_bu[i] = y2c_cbu*(i-128);
		y2c_gu[i] = y2c_cgu*(i-128);
		y2c_gv[i] = y2c_cgv*(i-128);
		y2c_rv[i] = y2c_crv*(i-128);
	}

	fColorConvInitOK = true;
}

//
// CMemSubPic
//

CMemSubPic::CMemSubPic(SubPicDesc& spd)
	: m_spd(spd)
{
	m_maxsize.SetSize(spd.w, spd.h);
	m_rcDirty.SetRect(0, 0, spd.w, spd.h);
}

CMemSubPic::~CMemSubPic()
{
	SAFE_DELETE_ARRAY(m_spd.bits);
}

// ISubPic

STDMETHODIMP_(void*) CMemSubPic::GetObject()
{
	return (void*)&m_spd;
}

STDMETHODIMP CMemSubPic::GetDesc(SubPicDesc& spd)
{
	spd.type = m_spd.type;
	spd.w = m_size.cx;
	spd.h = m_size.cy;
	spd.bpp = m_spd.bpp;
	spd.pitch = m_spd.pitch;
	spd.bits = m_spd.bits;
	spd.bitsU = m_spd.bitsU;
	spd.bitsV = m_spd.bitsV;
	spd.vidrect = m_vidrect;

	return S_OK;
}

STDMETHODIMP CMemSubPic::CopyTo(ISubPic* pSubPic)
{
	HRESULT hr;
	if (FAILED(hr = __super::CopyTo(pSubPic))) {
		return hr;
	}

	SubPicDesc src, dst;
	if (FAILED(GetDesc(src)) || FAILED(pSubPic->GetDesc(dst))) {
		return E_FAIL;
	}

	int w = m_rcDirty.Width(), h = m_rcDirty.Height();
	BYTE* s = (BYTE*)src.bits + src.pitch*m_rcDirty.top + m_rcDirty.left * 4;
	BYTE* d = (BYTE*)dst.bits + dst.pitch*m_rcDirty.top + m_rcDirty.left * 4;

	for (ptrdiff_t j = 0; j < h; j++, s += src.pitch, d += dst.pitch) {
		memcpy(d, s, w * 4);
	}

	return S_OK;
}

STDMETHODIMP CMemSubPic::ClearDirtyRect(DWORD color)
{
	if (m_rcDirty.IsRectEmpty()) {
		return S_FALSE;
	}

	BYTE* p = (BYTE*)m_spd.bits + m_spd.pitch*m_rcDirty.top + m_rcDirty.left*(m_spd.bpp>>3);
	for (ptrdiff_t j = 0, h = m_rcDirty.Height(); j < h; j++, p += m_spd.pitch) {
		int w = m_rcDirty.Width();
#ifdef _WIN64
		memsetd(p, color, w*4);
#else
		__asm {
			mov eax, color
			mov ecx, w
			mov edi, p
			cld
			rep stosd
		}
#endif
	}

	m_rcDirty.SetRectEmpty();

	return S_OK;
}

STDMETHODIMP CMemSubPic::Lock(SubPicDesc& spd)
{
	return GetDesc(spd);
}

STDMETHODIMP CMemSubPic::Unlock(RECT* pDirtyRect)
{
	m_rcDirty = pDirtyRect ? *pDirtyRect : CRect(0,0,m_spd.w,m_spd.h);

	if (m_rcDirty.IsRectEmpty()) {
		return S_OK;
	}

	if(m_spd.type == MSP_YUY2 || m_spd.type == MSP_YV12 || m_spd.type == MSP_IYUV || m_spd.type == MSP_AYUV
		|| m_spd.type == MSP_P010 || m_spd.type == MSP_P016 || m_spd.type == MSP_NV12) {
		ColorConvInit();

		if(m_spd.type == MSP_YUY2 || m_spd.type == MSP_YV12 || m_spd.type == MSP_IYUV
			|| m_spd.type == MSP_P010 || m_spd.type == MSP_P016 || m_spd.type == MSP_NV12)
		{
			m_rcDirty.left &= ~1;
			m_rcDirty.right = (m_rcDirty.right+1)&~1;

			if(m_spd.type == MSP_YV12 || m_spd.type == MSP_IYUV
				|| m_spd.type == MSP_P010 || m_spd.type == MSP_P016 || m_spd.type == MSP_NV12) {
				m_rcDirty.top &= ~1;
				m_rcDirty.bottom = (m_rcDirty.bottom+1)&~1;
			}
		}
	}

	int w = m_rcDirty.Width(), h = m_rcDirty.Height();
	BYTE* top = (BYTE*)m_spd.bits + m_spd.pitch*m_rcDirty.top + m_rcDirty.left*4;
	BYTE* bottom = top + m_spd.pitch*h;

	if (m_spd.type == MSP_RGB16) {
		for (; top < bottom ; top += m_spd.pitch) {
			DWORD* s = (DWORD*)top;
			DWORD* e = s + w;
			for (; s < e; s++) {
				*s = ((*s>>3)&0x1f000000)|((*s>>8)&0xf800)|((*s>>5)&0x07e0)|((*s>>3)&0x001f);
				//				*s = (*s&0xff000000)|((*s>>8)&0xf800)|((*s>>5)&0x07e0)|((*s>>3)&0x001f);
			}
		}
	} else if (m_spd.type == MSP_RGB15) {
		for (; top < bottom; top += m_spd.pitch) {
			DWORD* s = (DWORD*)top;
			DWORD* e = s + w;
			for (; s < e; s++) {
				*s = ((*s>>3)&0x1f000000)|((*s>>9)&0x7c00)|((*s>>6)&0x03e0)|((*s>>3)&0x001f);
				//				*s = (*s&0xff000000)|((*s>>9)&0x7c00)|((*s>>6)&0x03e0)|((*s>>3)&0x001f);
			}
		}
	} else if(m_spd.type == MSP_YUY2 || m_spd.type == MSP_YV12 || m_spd.type == MSP_IYUV
		|| m_spd.type == MSP_P010 || m_spd.type == MSP_P016 || m_spd.type == MSP_NV12) {
		for(; top < bottom ; top += m_spd.pitch) {
			BYTE* s = top;
			BYTE* e = s + w*4;
			for(; s < e; s+=8) { // ARGB ARGB -> AxYU AxYV
				if((s[3]+s[7]) < 0x1fe) {
					s[1] = (c2y_yb[s[0]] + c2y_yg[s[1]] + c2y_yr[s[2]] + 0x108000) >> 16;
					s[5] = (c2y_yb[s[4]] + c2y_yg[s[5]] + c2y_yr[s[6]] + 0x108000) >> 16;

					int scaled_y = (s[1]+s[5]-32) * cy_cy2;

					s[0] = Clip[(((((s[0]+s[4])<<15) - scaled_y) >> 10) * c2y_cu + 0x800000 + 0x8000) >> 16];
					s[4] = Clip[(((((s[2]+s[6])<<15) - scaled_y) >> 10) * c2y_cv + 0x800000 + 0x8000) >> 16];
				} else {
					s[1] = s[5] = 0x10;
					s[0] = s[4] = 0x80;
				}
			}
		}
	} else if (m_spd.type == MSP_AYUV) {
		for (; top < bottom ; top += m_spd.pitch) {
			BYTE* s = top;
			BYTE* e = s + w*4;
			for (; s < e; s+=4) { // ARGB -> AYUV
				if (s[3] < 0xff) {
					int y = (c2y_yb[s[0]] + c2y_yg[s[1]] + c2y_yr[s[2]] + 0x108000) >> 16;
					int scaled_y = (y-32) * cy_cy;
					s[1] = Clip[((((s[0]<<16) - scaled_y) >> 10) * c2y_cu + 0x800000 + 0x8000) >> 16];
					s[0] = Clip[((((s[2]<<16) - scaled_y) >> 10) * c2y_cv + 0x800000 + 0x8000) >> 16];
					s[2] = y;
				} else {
					s[0] = s[1] = 0x80;
					s[2] = 0x10;
				}
			}
		}
	}

	return S_OK;
}

static void AlphaBlt_YUY2_SSE2(int w, int h, BYTE* d, int dstpitch, BYTE* s, int srcpitch)
{
	unsigned int ia;
	static const __int64 _8181 = 0x0080001000800010i64;
	static const __m128i mm_zero = _mm_setzero_si128();
	static const __m128i mm_8181 = _mm_loadl_epi64((const __m128i *)&_8181);

	for (ptrdiff_t j = 0; j < h; j++, s += srcpitch, d += dstpitch) {
		DWORD* d2 = (DWORD*)d;
		BYTE* s2 = s;
		BYTE* s2end = s2 + w * 4;

		for (; s2 < s2end; s2 += 8, d2++) {
			ia = (s2[3] + s2[7]) >> 1;
			if (ia < 0xff) {
				const unsigned int c = (s2[4] << 24) | (s2[5] << 16) | (s2[0] << 8) | s2[1]; // (v << 24) | (y2 << 16) | (u << 8) | y1;
				ia = (ia << 24) | (s2[7] << 16) | (ia << 8) | s2[3];

				// SSE2
				__m128i mm_c = _mm_cvtsi32_si128(c);
				mm_c = _mm_unpacklo_epi8(mm_c, mm_zero);
				__m128i mm_d = _mm_cvtsi32_si128(*d2);
				mm_d = _mm_unpacklo_epi8(mm_d, mm_zero);
				__m128i mm_a = _mm_cvtsi32_si128(ia);
				mm_a = _mm_unpacklo_epi8(mm_a, mm_zero);
				mm_a = _mm_srli_epi16(mm_a, 1);
				mm_d = _mm_sub_epi16(mm_d, mm_8181);
				mm_d = _mm_mullo_epi16(mm_d, mm_a);
				mm_d = _mm_srai_epi16(mm_d, 7);
				mm_d = _mm_adds_epi16(mm_d, mm_c);
				mm_d = _mm_packus_epi16(mm_d, mm_d);
				*d2 = (DWORD)_mm_cvtsi128_si32(mm_d);
			}
		}
	}
}

/*
void AlphaBlt_YUY2_C(int w, int h, BYTE* d, int dstpitch, BYTE* s, int srcpitch)
{
	unsigned int ia;

	for (ptrdiff_t j = 0; j < h; j++, s += srcpitch, d += dstpitch) {
		DWORD* d2 = (DWORD*)d;
		BYTE* s2 = s;
		BYTE* s2end = s2 + w*4;

		for (; s2 < s2end; s2 += 8, d2++) {
			ia = (s2[3]+s2[7])>>1;
			if (ia < 0xff) {
				//unsigned int c = (s2[4]<<24)|(s2[5]<<16)|(s2[0]<<8)|s2[1]; // (v<<24)|(y2<<16)|(u<<8)|y1;

				// YUY2 colorspace fix. rewritten from sse2 asm
				DWORD y1 = (DWORD)(((((*d2&0xff)-0x10)*(s2[3]>>1))>>7)+s2[1])&0xff;			// y1
				DWORD uu = (DWORD)((((((*d2>>8)&0xff)-0x80)*(ia>>1))>>7)+s2[0])&0xff;		// u
				DWORD y2 = (DWORD)((((((*d2>>16)&0xff)-0x10)*(s2[7]>>1))>>7)+s2[5])&0xff;	// y2
				DWORD vv = (DWORD)((((((*d2>>24)&0xff)-0x80)*(ia>>1))>>7)+s2[4])&0xff;		// v
				*d2 = (y1)|(uu<<8)|(y2<<16)|(vv<<24);
			}
		}
	}
}
*/

STDMETHODIMP CMemSubPic::AlphaBlt(RECT* pSrc, RECT* pDst, SubPicDesc* pTarget)
{
	ASSERT(pTarget);

	if (!pSrc || !pDst || !pTarget) {
		return E_POINTER;
	}

	const SubPicDesc& src = m_spd;
	SubPicDesc dst = *pTarget;

	if (src.type != dst.type) {
		return E_INVALIDARG;
	}

	CRect rs(*pSrc), rd(*pDst);

	if (dst.h < 0) {
		dst.h		= -dst.h;
		rd.bottom	= dst.h - rd.bottom;
		rd.top		= dst.h - rd.top;
	}

	if (rs.Width() != rd.Width() || rs.Height() != abs(rd.Height())) {
		return E_INVALIDARG;
	}

	int w = rs.Width(), h = rs.Height();
	BYTE* s = (BYTE*)src.bits + src.pitch * rs.top + ((rs.left * src.bpp) >> 3); //rs.left * 4;
	BYTE* d = (BYTE*)dst.bits + dst.pitch * rd.top + ((rd.left * dst.bpp) >> 3);

	if (rd.top > rd.bottom) {
		if (dst.type == MSP_RGB32 || dst.type == MSP_RGB24
				|| dst.type == MSP_RGB16 || dst.type == MSP_RGB15
				|| dst.type == MSP_YUY2 || dst.type == MSP_AYUV) {
			d = (BYTE*)dst.bits + dst.pitch * (rd.top - 1) + (rd.left * dst.bpp >> 3);
		} else if (dst.type == MSP_YV12 || dst.type == MSP_IYUV) {
			d = (BYTE*)dst.bits + dst.pitch * (rd.top - 1) + (rd.left * 8 >> 3);
		} else if (dst.type == MSP_NV12) {
			d = (BYTE*)dst.bits + dst.pitch * (rd.top - 1) + rd.left;
		} else if (dst.type == MSP_P010 || dst.type == MSP_P016) {
			d = (BYTE*)dst.bits + dst.pitch * (rd.top - 1) + rd.left * 2;
		} else {
			return E_NOTIMPL;
		}

		dst.pitch = -dst.pitch;
	}

	switch (dst.type) {
		case MSP_P010:
		case MSP_P016:
			{
				// Alpha blend the Y plane. Source is UYxAVYxA packed values (converted by Unlock())
				// destination is P010/P016 surface.
				int bitDepth = (dst.type == MSP_P016) ? 16 : 10;

				for(ptrdiff_t j = 0; j < h; j++, s += src.pitch, d += dst.pitch)
				{
					BYTE* s2 = s;
					BYTE* s2end = s2 + w * 4;
					WORD* d2 = (WORD*)d;
					for(; s2 < s2end; s2 += 4, d2++)
					{
						if(s2[3] < 0xff)
						{
							// Convert current luminance to 8-bit value.
							WORD dstLum = d2[0] >> 8;

							// Perform calculation in 8-bit.
							WORD result = (((dstLum - 0x10) * s2[3]) >> 8) + s2[1];

							// Convert to 10/16 bit value.
							d2[0] = result << 8;
						}
					}
				}

				break;
			}
		case MSP_RGBA:
				for (ptrdiff_t j = 0; j < h; j++, s += src.pitch, d += dst.pitch) {
					BYTE* s2 = s;
					BYTE* s2end = s2 + w * 4;
					DWORD* d2 = (DWORD*)d;
					for (; s2 < s2end; s2 += 4, d2++) {
						if (s2[3] < 0xff) {
							DWORD bd =0x00000100 -( (DWORD) s2[3]);
							DWORD B = ((*((DWORD*)s2)&0x000000ff)<<8)/bd;
							DWORD V = ((*((DWORD*)s2)&0x0000ff00)/bd)<<8;
							DWORD R = (((*((DWORD*)s2)&0x00ff0000)>>8)/bd)<<16;
							*d2 = B | V | R
								  | (0xff000000-(*((DWORD*)s2)&0xff000000))&0xff000000;
						}
					}
				}
			break;
		case MSP_RGB32:
		case MSP_AYUV:
				for (ptrdiff_t j = 0; j < h; j++, s += src.pitch, d += dst.pitch) {
					BYTE* s2 = s;
					BYTE* s2end = s2 + w*4;

					DWORD* d2 = (DWORD*)d;
					for (; s2 < s2end; s2 += 4, d2++) {
#ifdef _WIN64
						DWORD ia = 256-s2[3];
						if (s2[3] < 0xff) {
							*d2 = ((((*d2&0x00ff00ff)*s2[3])>>8) + (((*((DWORD*)s2)&0x00ff00ff)*ia)>>8)&0x00ff00ff)
									| ((((*d2&0x0000ff00)*s2[3])>>8) + (((*((DWORD*)s2)&0x0000ff00)*ia)>>8)&0x0000ff00);
						}
#else
						if (s2[3] < 0xff) {
							*d2 = ((((*d2&0x00ff00ff)*s2[3])>>8) + (*((DWORD*)s2)&0x00ff00ff)&0x00ff00ff)
									| ((((*d2&0x0000ff00)*s2[3])>>8) + (*((DWORD*)s2)&0x0000ff00)&0x0000ff00);
						}
#endif
					}
				}
			break;
		case MSP_RGB24:
				for (ptrdiff_t j = 0; j < h; j++, s += src.pitch, d += dst.pitch) {
					BYTE* s2 = s;
					BYTE* s2end = s2 + w * 4;
					BYTE* d2 = d;
					for (; s2 < s2end; s2 += 4, d2 += 3) {
						if (s2[3] < 0xff) {
							d2[0] = ((d2[0] *s2[3]) >> 8) + s2[0];
							d2[1] = ((d2[1] *s2[3]) >> 8) + s2[1];
							d2[2] = ((d2[2] *s2[3]) >> 8) + s2[2];
						}
					}
				}
			break;
		case MSP_RGB16:
				for (ptrdiff_t j = 0; j < h; j++, s += src.pitch, d += dst.pitch) {
					BYTE* s2 = s;
					BYTE* s2end = s2 + w * 4;
					WORD* d2 = (WORD*)d;
					for (; s2 < s2end; s2 += 4, d2++) {
						if (s2[3] < 0x1f) {
							*d2 = (WORD)((((((*d2&0xf81f)*s2[3])>>5) + (*(DWORD*)s2&0xf81f))&0xf81f)
										 | (((((*d2&0x07e0)*s2[3])>>5) + (*(DWORD*)s2&0x07e0))&0x07e0));
						}
					}
				}
			break;
		case MSP_RGB15:
				for (ptrdiff_t j = 0; j < h; j++, s += src.pitch, d += dst.pitch) {
					BYTE* s2 = s;
					BYTE* s2end = s2 + w * 4;
					WORD* d2 = (WORD*)d;
					for (; s2 < s2end; s2 += 4, d2++) {
						if (s2[3] < 0x1f) {
							*d2 = (WORD)((((((*d2&0x7c1f)*s2[3])>>5) + (*(DWORD*)s2&0x7c1f))&0x7c1f)
										 | (((((*d2&0x03e0)*s2[3])>>5) + (*(DWORD*)s2&0x03e0))&0x03e0));
						}
					}
				}
			break;
		case MSP_YUY2:
			AlphaBlt_YUY2_SSE2(w, h, d, dst.pitch, s, src.pitch);
			break;
		case MSP_YV12:
		case MSP_NV12:
		case MSP_IYUV:
				for (ptrdiff_t j = 0; j < h; j++, s += src.pitch, d += dst.pitch) {
					BYTE* s2 = s;
					BYTE* s2end = s2 + w * 4;
					BYTE* d2 = d;
					for (; s2 < s2end; s2 += 4, d2++) {
						if (s2[3] < 0xff) {
							d2[0] = (((d2[0] - 0x10) * s2[3]) >> 8) + s2[1];
						}
					}
				}
			break;
		default:
				return E_NOTIMPL;
	}

	dst.pitch = abs(dst.pitch);

	if (dst.type == MSP_P010 || dst.type == MSP_P016) {
		// Alpha blend UV plane. UV is interleaved. Each UV represents a 2x2 block of pixels
		// so we need to sample the current row and the row after for source color info.
		// Source is UYxAVYxA.
		int h2 = h / 2;

		BYTE* ss = (BYTE*)src.bits + src.pitch * rs.top + rs.left * 4;
		BYTE* dstUV = (BYTE*)dst.bits + dst.pitch * dst.h;

		// Shift position to start of dirty rectangle. Need to divide dirty rectangle height
		// by 2 because each row of UV values is 2 rows of pixels in the source.
		if (rd.top > rd.bottom) {
			dstUV = dstUV + dst.pitch * (rd.top / 2 - 1) + rd.left * 2;
			dst.pitch = -dst.pitch;
		} else {
			dstUV = dstUV + dst.pitch * rd.top / 2 + rd.left * 2;
		}

		for(ptrdiff_t j = 0; j < h2; j++, ss += src.pitch * 2, dstUV += dst.pitch) {
			BYTE* srcData = ss;
			BYTE* srcDataEnd = srcData + w * 4;
			WORD* dstData = (WORD*)dstUV;
			for (; srcData < srcDataEnd; srcData += 8, dstData += 2) {
				// Sample 2x2 block of alpha values
				unsigned int ia = (srcData[3] + srcData[3 + src.pitch] + srcData[7] + srcData[7 + src.pitch]) >> 2;

				if (ia < 255) {
					WORD result;

					// Convert U to 8-bit
					WORD dstUV = dstData[0] >> 8;

					// Alpha blend U
					result = (((dstUV - 0x80) * ia) >> 8) + ((srcData[0] + srcData[src.pitch]) >> 1);

					// Convert U to 10/16 bit value;
					dstData[0] = result << 8;

					// Repeat for V
					dstUV = dstData[1] >> 8;
					result = (((dstUV - 0x80) * ia) >> 8) + ((srcData[4] + srcData[4 + src.pitch]) >> 1);
					dstData[1] = result << 8;
				}
			}
		}
	} else if (dst.type == MSP_YV12 || dst.type == MSP_IYUV) {
		int h2 = h / 2;

		if (!dst.pitchUV) {
			dst.pitchUV = dst.pitch / 2;
		}

		BYTE* ss[2];
		ss[0] = (BYTE*)src.bits + src.pitch * rs.top + rs.left * 4;
		ss[1] = ss[0] + 4;

		if (!dst.bitsU || !dst.bitsV) {
			dst.bitsU = (BYTE*)dst.bits + dst.pitch * dst.h;
			dst.bitsV = dst.bitsU + dst.pitchUV * dst.h / 2;

			if (dst.type == MSP_YV12) {
				BYTE* p = dst.bitsU;
				dst.bitsU = dst.bitsV;
				dst.bitsV = p;
			}
		}

		BYTE* dd[2];
		dd[0] = dst.bitsU + dst.pitchUV * rd.top / 2 + rd.left / 2;
		dd[1] = dst.bitsV + dst.pitchUV * rd.top / 2 + rd.left / 2;

		if (rd.top > rd.bottom) {
			dd[0] = dst.bitsU + dst.pitchUV * (rd.top / 2 - 1) + rd.left / 2;
			dd[1] = dst.bitsV + dst.pitchUV * (rd.top / 2 - 1) + rd.left / 2;
			dst.pitchUV = -dst.pitchUV;
		}

		for (ptrdiff_t i = 0; i < 2; i++) {
			s = ss[i];
			d = dd[i];
			BYTE* is = ss[1-i];
			for (ptrdiff_t j = 0; j < h2; j++, s += src.pitch * 2, d += dst.pitchUV, is += src.pitch * 2) {
				BYTE* s2 = s;
				BYTE* s2end = s2 + w * 4;
				BYTE* d2 = d;
				BYTE* is2 = is;
				for (; s2 < s2end; s2 += 8, d2++, is2 += 8) {
					unsigned int ia = (s2[3]+s2[3+src.pitch]+is2[3]+is2[3+src.pitch])>>2;
					if (ia < 0xff) {
						*d2 = (((*d2-0x80)*ia)>>8) + ((s2[0]+s2[src.pitch])>>1);
					}
				}
			}
		}
	} else if (dst.type == MSP_NV12) {
		int h2 = h/2;

		BYTE* ss[2];
		ss[0] = (BYTE*)src.bits + src.pitch * rs.top + rs.left * 4;
		ss[1] = ss[0] + 4;

		if (!dst.bitsU) {
			dst.bitsU = (BYTE*)dst.bits + dst.pitch * dst.h;
		}

		BYTE* dd[2];
		dd[0] = dst.bitsU + dst.pitch * rd.top / 2 + rd.left;
		if (rd.top > rd.bottom) {
			dd[0] = dst.bitsU + dst.pitch*(rd.top/2 - 1) + rd.left;
			dst.pitch = - dst.pitch;
		}
		dd[1] = dd[0] + 1;

		for (ptrdiff_t i = 0; i < 2; i++) {
			s = ss[i];
			d = dd[i];
			BYTE* is = ss[1-i];
			for (ptrdiff_t j = 0; j < h2; j++, s += src.pitch * 2, d += dst.pitch, is += src.pitch * 2) {
				BYTE* s2 = s;
				BYTE* s2end = s2 + w * 4;
				BYTE* d2 = d;
				BYTE* is2 = is;
				for (; s2 < s2end; s2 += 8, d2 += 2, is2 += 8) {
					unsigned int ia = (s2[3]+s2[3+src.pitch]+is2[3]+is2[3+src.pitch])>>2;
					if (ia < 255) {
						*d2 = (((*d2-0x80)*ia)>>8) + ((s2[0]+s2[src.pitch])>>1);
					}
				}
			}
		}
	}

	return S_OK;
}

//
// CMemSubPicAllocator
//

CMemSubPicAllocator::CMemSubPicAllocator(int type, SIZE maxsize)
	: CSubPicAllocatorImpl(maxsize, false)
	, m_type(type)
	, m_maxsize(maxsize)
{
}

// ISubPicAllocatorImpl

bool CMemSubPicAllocator::Alloc(bool fStatic, ISubPic** ppSubPic)
{
	if (!ppSubPic) {
		return false;
	}

	SubPicDesc spd;
	spd.w = m_maxsize.cx;
	spd.h = m_maxsize.cy;
	spd.bpp = 32;
	spd.pitch = (spd.w*spd.bpp)>>3;
	spd.type = m_type;
	spd.bits = DNew BYTE[spd.pitch*spd.h];
	if (!spd.bits) {
		return false;
	}

	*ppSubPic = DNew CMemSubPic(spd);
	if (!(*ppSubPic)) {
		return false;
	}

	(*ppSubPic)->AddRef();

	return true;
}
