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
#include <emmintrin.h>
#include <moreuuids.h>

#include "AvgLines.h"

void Scale2x_YV( int w, int h, BYTE* d, int dpitch, BYTE* s, int spitch )
{
	BYTE* s1;
	BYTE* s2;
	BYTE* d1;

	for (s1 = s, s2 = s + h*spitch, d1 = d; s1 < s2; d1 += dpitch) { // TODO: replace this mess with mmx code
		BYTE* stmp = s1 + spitch;
		BYTE* dtmp = d1 + dpitch;

		for (BYTE* s3 = s1 + (w-1); s1 < s3; s1 += 1, d1 += 2) {
			d1[0] = s1[0];
			d1[1] = (s1[0]+s1[1])>>1;
		}

		d1[0] = d1[1] = s1[0];

		s1 += 1;
		d1 += 2;

		s1 = stmp;
		d1 = dtmp;
	}

	AvgLines8(d, h*2, dpitch);
}

void Scale2x_YUY2_SSE2( BYTE* s1, BYTE* d1, int w )
{
	const uint64_t __0xffffffff00000000 = 0xffffffff00000000;
	const uint64_t __0x00000000ffffffff = 0x00000000ffffffff;
	const uint64_t __0x00ff00ff00ff00ff = 0x00ff00ff00ff00ff;

	const __m128i mm4 = _mm_loadl_epi64((const __m128i *)&__0x00ff00ff00ff00ff);	//movq	mm0, __0x00ff00ff00ff00ff
	const __m128i mm5 = _mm_loadl_epi64((const __m128i *)&__0x00000000ffffffff);	//movq	mm0, __0x00000000ffffffff
	const __m128i mm6 = _mm_loadl_epi64((const __m128i *)&__0xffffffff00000000);	//movq	mm0, __0xffffffff00000000
	for (BYTE* s3 = s1 + ((w>>1)-1)*4; s1 < s3; s1 += 4, d1 += 8) {
		__m128i mm0 = _mm_loadl_epi64((const __m128i *)(s1));	//movq	mm0, [esi]
		__m128i mm2 = _mm_move_epi64(mm0);						//movq	mm2, mm0
		mm0 = _mm_and_si128(mm0, mm4);							//pand	mm0, mm4	// mm0 = 00y400y300y200y1
		mm2 = _mm_srli_epi16(mm2, 8);							//psrlw	mm2, 8		// mm2 = 00u200v200u100v1
		__m128i mm1 = _mm_move_epi64(mm0);						//movq	mm1, mm0
		mm0 = _mm_and_si128(mm0, mm5);							//pand	mm0, mm5	// mm0 = 0000000000y200y1
		mm1 = _mm_slli_epi64(mm1, 16);							//psllq	mm1, 16
		mm1 = _mm_and_si128(mm1, mm6);							//pand	mm1, mm6	// mm1 = 00y300y200000000
		mm1 = _mm_or_si128(mm1, mm0);							//por	mm1, mm0	// mm1 = 00y300y200y200y1
		mm0 = _mm_unpacklo_epi8(mm0, mm0);						//punpcklwd mm0, mm0	// mm0 = 00y200y200y100y1
		mm0 = _mm_adds_epi16(mm0,mm1);							//paddw	mm0, mm1
		mm0 = _mm_srli_epi16(mm0, 1);							//psrlw	mm0, 1		// mm0 = (mm0 + mm1) / 2
		mm1 = _mm_move_epi64(mm2);								//movq	mm1, mm2
		mm1 = _mm_unpacklo_epi32(mm1, mm1);						//punpckldq	mm1, mm1 // mm1 = 00u100v100u100v1
		mm1 = _mm_adds_epi16(mm1,mm2);							//paddw	mm1, mm2
		mm1 = _mm_srli_epi16(mm1, 1);							//psrlw	mm1, 1		// mm1 = (mm1 + mm2) / 2
		mm1 = _mm_slli_epi64(mm1, 8);							//psllw	mm1, 8
		mm1 = _mm_or_si128(mm0, mm1);							//por		mm0, mm1	// mm0 = (v1+v2)/2|(y2+y3)/2|(u1+u2)/2|y2|v1|(y1+y2)/2|u1|y1
		_mm_storel_epi64((__m128i *)(d1), mm0);					//movq	[edi], mm0
	}

	*d1++ = s1[0];
	*d1++ = s1[1];
	*d1++ =(s1[0]+s1[2])>>1;
	*d1++ = s1[3];

	*d1++ = s1[2];
	*d1++ = s1[1];
	*d1++ = s1[2];
	*d1++ = s1[3];

	s1 += 4;
}

/*
void Scale2x_YUY2_c( BYTE* s1, BYTE* d1, int w )
{
	for (BYTE* s3 = s1 + ((w>>1)-1)*4; s1 < s3; s1 += 4, d1 += 8) {
		d1[0] = s1[0];
		d1[1] = s1[1];
		d1[2] = (s1[0]+s1[2])>>1;
		d1[3] = s1[3];

		d1[4] = s1[2];
		d1[5] = (s1[1]+s1[5])>>1;
		d1[6] = (s1[2]+s1[4])>>1;
		d1[7] = (s1[3]+s1[7])>>1;
	}

	*d1++ = s1[0];
	*d1++ = s1[1];
	*d1++ =(s1[0]+s1[2])>>1;
	*d1++ = s1[3];

	*d1++ = s1[2];
	*d1++ = s1[1];
	*d1++ = s1[2];
	*d1++ = s1[3];

	s1 += 4;
}
*/

void Scale2x_YUY2( int w, int h, BYTE* d, int dpitch, BYTE* s, int spitch )
{
	BYTE* s1;
	BYTE* s2;
	BYTE* d1;

	for (s1 = s, s2 = s + h*spitch, d1 = d; s1 < s2; d1 += dpitch) {
		// row0, 4 pixels: y1|u1|y2|v1|y3|u2|y4|v2
		// ->
		// row0, 8 pixels: y1|u1|(y1+y2)/2|v1|y2|(u1+u2)/2|(y2+y3)/2|(v1+v2)/2

		Scale2x_YUY2_SSE2(s1, d1, w);

		s1 += spitch;
		d1 += dpitch;
	}

	AvgLines8(d, h*2, dpitch);
}

void Scale2x_RGB555( int w, int h, BYTE* d, int dpitch, BYTE* s, int spitch )
{
	BYTE* s1;
	BYTE* s2;
	BYTE* d1;

	for (s1 = s, s2 = s + h*spitch, d1 = d; s1 < s2; d1 += dpitch) { // TODO: replace this mess with mmx code
		BYTE* stmp = s1 + spitch;
		BYTE* dtmp = d1 + dpitch;

		for (BYTE* s3 = s1 + (w-1)*2; s1 < s3; s1 += 2, d1 += 4) {
			*((WORD*)d1) = *((WORD*)s1);
			*((WORD*)d1+1) =
				((((*((WORD*)s1)&0x7c00) + (*((WORD*)s1+1)&0x7c00)) >> 1)&0x7c00)|
				((((*((WORD*)s1)&0x03e0) + (*((WORD*)s1+1)&0x03e0)) >> 1)&0x03e0)|
				((((*((WORD*)s1)&0x001f) + (*((WORD*)s1+1)&0x001f)) >> 1)&0x001f);
		}

		*((WORD*)d1) = *((WORD*)s1);
		*((WORD*)d1+1) = *((WORD*)s1);

		s1 += 2;
		d1 += 4;

		s1 = stmp;
		d1 = dtmp;
	}

	AvgLines555(d, h*2, dpitch);
}

void Scale2x_RGB565( int w, int h, BYTE* d, int dpitch, BYTE* s, int spitch )
{
	BYTE* s1;
	BYTE* s2;
	BYTE* d1;

	for (s1 = s, s2 = s + h*spitch, d1 = d; s1 < s2; d1 += dpitch) { // TODO: replace this mess with mmx code
		BYTE* stmp = s1 + spitch;
		BYTE* dtmp = d1 + dpitch;

		for (BYTE* s3 = s1 + (w-1)*2; s1 < s3; s1 += 2, d1 += 4) {
			*((WORD*)d1) = *((WORD*)s1);
			*((WORD*)d1+1) =
				((((*((WORD*)s1)&0xf800) + (*((WORD*)s1+1)&0xf800)) >> 1)&0xf800)|
				((((*((WORD*)s1)&0x07e0) + (*((WORD*)s1+1)&0x07e0)) >> 1)&0x07e0)|
				((((*((WORD*)s1)&0x001f) + (*((WORD*)s1+1)&0x001f)) >> 1)&0x001f);
		}

		*((WORD*)d1) = *((WORD*)s1);
		*((WORD*)d1+1) = *((WORD*)s1);

		s1 += 2;
		d1 += 4;

		s1 = stmp;
		d1 = dtmp;
	}

	AvgLines565(d, h*2, dpitch);
}

void Scale2x_RGB24( int w, int h, BYTE* d, int dpitch, BYTE* s, int spitch )
{
	BYTE* s1;
	BYTE* s2;
	BYTE* d1;

	for (s1 = s, s2 = s + h*spitch, d1 = d; s1 < s2; d1 += dpitch) { // TODO: replace this mess with mmx code
		BYTE* stmp = s1 + spitch;
		BYTE* dtmp = d1 + dpitch;

		for (BYTE* s3 = s1 + (w-1)*3; s1 < s3; s1 += 3, d1 += 6) {
			d1[0] = s1[0];
			d1[1] = s1[1];
			d1[2] = s1[2];
			d1[3] = (s1[0]+s1[3])>>1;
			d1[4] = (s1[1]+s1[4])>>1;
			d1[5] = (s1[2]+s1[5])>>1;
		}

		d1[0] = d1[3] = s1[0];
		d1[1] = d1[4] = s1[1];
		d1[2] = d1[5] = s1[2];

		s1 += 3;
		d1 += 6;

		s1 = stmp;
		d1 = dtmp;
	}

	AvgLines8(d, h*2, dpitch);
}

void Scale2x_XRGB32_SSE2( BYTE* s1, BYTE* d1, int w )
{
	const __m128i mm_zero = _mm_setzero_si128();				//pxor	mm0, mm0
	for (BYTE* s3 = s1 + (w-1)*4; s1 < s3; s1 += 4, d1 += 8) {

		__m128i mm1 = _mm_loadl_epi64((const __m128i *)(s1));	//movq	mm1, [esi]
		__m128i mm2 = _mm_move_epi64(mm1);						//movq	mm2, mm1

		mm1 = _mm_unpacklo_epi8(mm1,mm_zero);					//punpcklbw mm1, mm0	// mm1 = 00xx00r100g100b1
		mm2 = _mm_unpacklo_epi8(mm2,mm_zero);					//punpckhbw mm2, mm0	// mm2 = 00xx00r200g200b2

		mm2 = _mm_adds_epi16(mm2,mm1);							//paddw	mm2, mm1
		mm2 = _mm_srli_epi16(mm2, 1);							//psrlw	mm2, 1		// mm2 = (mm1 + mm2) / 2

		mm1 = _mm_packus_epi16(mm1,mm2);						//packuswb	mm1, mm2

		_mm_storel_epi64((__m128i *)(d1), mm1);					//movq	[edi], mm0
	}

	*((DWORD*)d1) = *((DWORD*)s1);
	*((DWORD*)d1+1) = *((DWORD*)s1);

	s1 += 4;
	d1 += 8;
}

/*
void Scale2x_XRGB32_c( BYTE* s1, BYTE* d1, int w )
{
	for (BYTE* s3 = s1 + (w-1)*4; s1 < s3; s1 += 3, d1 += 6) {
		d1[0] = s1[0];
		d1[1] = s1[1];
		d1[2] = s1[2];
		d1[3] = s1[3];

		d1[4] = (s1[0]+s1[4])>>1;
		d1[5] = (s1[1]+s1[5])>>1;
		d1[6] = (s1[2]+s1[6])>>1;
		d1[7] = (s1[4]+s1[7])>>1;
	}

	*((DWORD*)d1) = *((DWORD*)s1);
	*((DWORD*)d1+1) = *((DWORD*)s1);

	s1 += 4;
	d1 += 8;
}
*/

void Scale2x_XRGB32( int w, int h, BYTE* d, int dpitch, BYTE* s, int spitch )
{
	BYTE* s1;
	BYTE* s2;
	BYTE* d1;

	for (s1 = s, s2 = s + h*spitch, d1 = d; s1 < s2; d1 += dpitch) {
		Scale2x_XRGB32_SSE2(s1, d1, w);

		s1 += spitch;
		d1 += dpitch;
	}

	AvgLines8(d, h*2, dpitch);
}

/* ResX2 */
void Scale2x(const GUID& subtype, BYTE* d, int dpitch, BYTE* s, int spitch, int w, int h)
{
	if (subtype == MEDIASUBTYPE_YV12 || subtype == MEDIASUBTYPE_I420 || subtype == MEDIASUBTYPE_IYUV) {
		Scale2x_YV(w, h, d, dpitch, s, spitch);
	} else if (subtype == MEDIASUBTYPE_YUY2) {
		Scale2x_YUY2(w, h, d, dpitch, s, spitch);
	} else if (subtype == MEDIASUBTYPE_RGB555) {
		Scale2x_RGB555(w, h, d, dpitch, s, spitch);
	} else if (subtype == MEDIASUBTYPE_RGB565) {
		Scale2x_RGB565(w, h, d, dpitch, s, spitch);
	} else if (subtype == MEDIASUBTYPE_RGB24) {
		Scale2x_RGB24(w, h, d, dpitch, s, spitch);
	} else if (subtype == MEDIASUBTYPE_RGB32 || subtype == MEDIASUBTYPE_ARGB32) {
		Scale2x_XRGB32(w, h, d, dpitch, s, spitch);
	}
}
