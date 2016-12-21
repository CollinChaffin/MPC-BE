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
#include <math.h>
#include <InitGuid.h>
#include <moreuuids.h>
#include "DirectVobSubFilter.h"
#include "Scale2x.h"

extern int c2y_yb[256];
extern int c2y_yg[256];
extern int c2y_yr[256];
extern void ColorConvInit();

void BltLineRGB32(DWORD* d, BYTE* sub, int w, const GUID& subtype)
{
	if (subtype == MEDIASUBTYPE_YV12 || subtype == MEDIASUBTYPE_I420 || subtype == MEDIASUBTYPE_IYUV
		|| subtype == MEDIASUBTYPE_NV12) {
		BYTE* db = (BYTE*)d;
		BYTE* dbtend = db + w;

		for (; db < dbtend; sub += 4, db++) {
			if (sub[3] < 0xff) {
				int y = (c2y_yb[sub[0]] + c2y_yg[sub[1]] + c2y_yr[sub[2]] + 0x108000) >> 16;
				*db = y; // w/o colors
			}
		}
	}
	else if (subtype == MEDIASUBTYPE_P010 || subtype == MEDIASUBTYPE_P016)
	{
		// Y plane is 16 bits
		WORD* dstY = reinterpret_cast<WORD*>(d);
		WORD* dstYEnd = dstY + w; // What units is w given in?

		for (; dstY < dstYEnd; dstY++, sub += 4)
		{
			if (sub[3] < 0xff)
			{
				// Look up table generates a 32-bit luminance value which is then scaled to 16 bits.
				int y = (c2y_yb[sub[0]] + c2y_yg[sub[1]] + c2y_yr[sub[2]] + 0x108000) >> 16;

				// Do I need to perform any scaling as per SMPTE 274M?
				*dstY = y;
			}
		}
	}
    else if (subtype == MEDIASUBTYPE_YUY2) {
		WORD* ds = (WORD*)d;
		WORD* dstend = ds + w;

		for (; ds < dstend; sub+=4, ds++) {
			if (sub[3] < 0xff) {
				int y = (c2y_yb[sub[0]] + c2y_yg[sub[1]] + c2y_yr[sub[2]] + 0x108000) >> 16;
				*ds = 0x8000|y; // w/o colors
			}
		}
	} else if (subtype == MEDIASUBTYPE_RGB555) {
		WORD* ds = (WORD*)d;
		WORD* dstend = ds + w;

		for (; ds < dstend; sub += 4, ds++) {
			if (sub[3] < 0xff) {
				*ds = ((*((DWORD*)sub)>>9)&0x7c00)|((*((DWORD*)sub)>>6)&0x03e0)|((*((DWORD*)sub)>>3)&0x001f);
			}
		}
	} else if (subtype == MEDIASUBTYPE_RGB565) {
		WORD* ds = (WORD*)d;
		WORD* dstend = ds + w;

		for (; ds < dstend; sub += 4, ds++) {
			if (sub[3] < 0xff) {
				*ds = ((*((DWORD*)sub)>>8)&0xf800)|((*((DWORD*)sub)>>5)&0x07e0)|((*((DWORD*)sub)>>3)&0x001f);
			}
		}
	} else if (subtype == MEDIASUBTYPE_RGB24) {
		BYTE* dt = (BYTE*)d;
		BYTE* dstend = dt + w*3;

		for (; dt < dstend; sub += 4, dt+=3) {
			if (sub[3] < 0xff) {
				dt[0] = sub[0];
				dt[1] = sub[1];
				dt[2] = sub[2];
			}
		}
	} else if (subtype == MEDIASUBTYPE_RGB32 || subtype == MEDIASUBTYPE_ARGB32) {
		DWORD* dstend = d + w;

		for (; d < dstend; sub+=4, d++) {
			if (sub[3] < 0xff) {
				*d = *((DWORD*)sub)&0xffffff;
			}
		}
	}
}

HRESULT CDirectVobSubFilter::Copy(BYTE* pSub, BYTE* pIn, CSize sub, CSize in, int bpp, const GUID& subtype, DWORD black)
{
	int wIn = in.cx, hIn = in.cy, pitchIn = wIn*bpp>>3;
	int wSub = sub.cx, hSub = sub.cy, pitchSub = wSub*bpp>>3;
	bool fScale2x = wIn*2 <= wSub;

	if (fScale2x) {
		wIn <<= 1, hIn <<= 1;
	}

	int left = ((wSub - wIn)>>1)&~1;
	int mid = wIn;
	int right = left + ((wSub - wIn)&1);

	int dpLeft = left*bpp>>3;
	int dpMid = mid*bpp>>3;
	int dpRight = right*bpp>>3;

	ASSERT(wSub >= wIn);

	{
		int i = 0, j = 0;

		j += (hSub - hIn) >> 1;

		for (; i < j; i++, pSub += pitchSub) {
			memsetd(pSub, black, dpLeft+dpMid+dpRight);
		}

		j += hIn;

		if (hIn > hSub) {
			pIn += pitchIn * ((hIn - hSub) >> (fScale2x?2:1));
		}

		if (fScale2x) {
			Scale2x(subtype,
					pSub + dpLeft, pitchSub, pIn, pitchIn,
					in.cx, (min(j, hSub) - i) >> 1);

			for (ptrdiff_t k = min(j, hSub); i < k; i++, pIn += pitchIn, pSub += pitchSub) {
				memsetd(pSub, black, dpLeft);
				memsetd(pSub + dpLeft+dpMid, black, dpRight);
			}
		} else {
			for (ptrdiff_t k = min(j, hSub); i < k; i++, pIn += pitchIn, pSub += pitchSub) {
				memsetd(pSub, black, dpLeft);
				memcpy(pSub + dpLeft, pIn, dpMid);
				memsetd(pSub + dpLeft+dpMid, black, dpRight);
			}
		}

		j = hSub;

		for (; i < j; i++, pSub += pitchSub) {
			memsetd(pSub, black, dpLeft+dpMid+dpRight);
		}
	}

	return NOERROR;
}

void CDirectVobSubFilter::PrintMessages(BYTE* pOut)
{
	if (!m_hdc || !m_hbm) {
		return;
	}

	ColorConvInit();

	const GUID& subtype = m_pOutput->CurrentMediaType().subtype;

	BITMAPINFOHEADER bihOut;
	ExtractBIH(&m_pOutput->CurrentMediaType(), &bihOut);

	CString msg, tmp;

	if (m_bOSD) {
		CString input = GetGUIDString(m_pInput->CurrentMediaType().subtype);
		if (!input.Left(13).CompareNoCase(L"MEDIASUBTYPE_")) {
			input = input.Mid(13);
		}
		CString output = GetGUIDString(m_pOutput->CurrentMediaType().subtype);
		if (!output.Left(13).CompareNoCase(L"MEDIASUBTYPE_")) {
			output = output.Mid(13);
		}

		tmp.Format(L"in: %dx%d %s\nout: %dx%d %s\n",
				   m_w, m_h,
				   input,
				   bihOut.biWidth, bihOut.biHeight,
				   output);
		msg += tmp;

		tmp.Format(L"real fps: %.3f, current fps: %.3f\nmedia time: %d, subtitle time: %d [ms]\nframe number: %d (calculated)\nrate: %.4f\n",
				   m_fps, m_bMediaFPSEnabled?m_MediaFPS:fabs(m_fps),
				   (int)m_tPrev.Millisecs(), (int)(CalcCurrentTime()/10000),
				   (int)(m_tPrev.m_time * m_fps / 10000000),
				   m_pInput->CurrentRate());
		msg += tmp;

		CAutoLock cAutoLock(&m_csQueueLock);

		if (m_pSubPicQueue) {
			int nSubPics = -1;
			REFERENCE_TIME rtNow = -1, rtStart = -1, rtStop = -1;
			m_pSubPicQueue->GetStats(nSubPics, rtNow, rtStart, rtStop);
			tmp.Format(L"queue stats: %I64d - %I64d [ms]\n", rtStart/10000, rtStop/10000);
			msg += tmp;

			for (int i = 0; i < nSubPics; i++) {
				m_pSubPicQueue->GetStats(i, rtStart, rtStop);
				tmp.Format(L"%d: %I64d - %I64d [ms]\n", i, rtStart/10000, rtStop/10000);
				msg += tmp;
			}

		}
	}

	if (msg.IsEmpty()) {
		return;
	}

	HANDLE hOldBitmap = SelectObject(m_hdc, m_hbm);
	HANDLE hOldFont = SelectObject(m_hdc, m_hfont);

	SetTextColor(m_hdc, 0xffffff);
	SetBkMode(m_hdc, TRANSPARENT);
	SetMapMode(m_hdc, MM_TEXT);

	BITMAP bm;
	GetObject(m_hbm, sizeof(BITMAP), &bm);

	CRect r(0, 0, bm.bmWidth, bm.bmHeight);
	DrawText(m_hdc, msg, wcslen(msg), &r, DT_CALCRECT|DT_EXTERNALLEADING|DT_NOPREFIX|DT_WORDBREAK);

	r += CPoint(10, 10);
	r &= CRect(0, 0, bm.bmWidth, bm.bmHeight);

	DrawText(m_hdc, msg, wcslen(msg), &r, DT_LEFT|DT_TOP|DT_NOPREFIX|DT_WORDBREAK);

	BYTE* pIn = (BYTE*)bm.bmBits;
	int pitchIn = bm.bmWidthBytes;
	int pitchOut = bihOut.biWidth * bihOut.biBitCount >> 3;

	if (subtype == MEDIASUBTYPE_YV12 || subtype == MEDIASUBTYPE_I420 || subtype == MEDIASUBTYPE_IYUV
		|| subtype== MEDIASUBTYPE_NV12) {
		pitchOut = bihOut.biWidth;
	} else if (subtype == MEDIASUBTYPE_P010 || subtype == MEDIASUBTYPE_P016) {
		pitchOut = bihOut.biWidth * 2;
	}

	pitchIn = (pitchIn+3)&~3;
	pitchOut = (pitchOut+3)&~3;

	if (bihOut.biHeight > 0 && bihOut.biCompression <= 3) { // flip if the dst bitmap is flipped rgb (m_hbm is a top-down bitmap, not like the subpictures)
		pOut += pitchOut * (abs(bihOut.biHeight)-1);
		pitchOut = -pitchOut;
	}

	pIn += pitchIn * r.top;
	pOut += pitchOut * r.top;

	for (ptrdiff_t w = min(r.right, m_w), h = r.Height(); h--; pIn += pitchIn, pOut += pitchOut) {
		BltLineRGB32((DWORD*)pOut, pIn, w, subtype);
		memsetd(pIn, 0xff000000, r.right*4);
	}

	SelectObject(m_hdc, hOldBitmap);
	SelectObject(m_hdc, hOldFont);
}
