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

#include <vector>
#include <memory>
#include "Ellipse.h"
#include "../SubPic/ISubPic.h"

#define PT_MOVETONC			0xfe
#define PT_BSPLINETO		0xfc
#define PT_BSPLINEPATCHTO	0xfa

struct SubPicDesc;


using tSpanBuffer = std::vector<std::pair<unsigned __int64, unsigned __int64>>;

struct COutlineData {
	int mWidth, mHeight;
	int mPathOffsetX, mPathOffsetY;
	int mWideBorder;
	tSpanBuffer mOutline, mWideOutline;

	COutlineData()
		: mWidth(0)
		, mHeight(0)
		, mPathOffsetX(0)
		, mPathOffsetY(0)
		, mWideBorder(0) {}
};

typedef std::shared_ptr<COutlineData> COutlineDataSharedPtr;

struct COverlayData {
	int mOffsetX, mOffsetY;
	int mOverlayWidth, mOverlayHeight, mOverlayPitch;
	byte* mpOverlayBufferBody, *mpOverlayBufferBorder;

	COverlayData()
		: mOffsetX(0)
		, mOffsetY(0)
		, mOverlayWidth(0)
		, mOverlayHeight(0)
		, mOverlayPitch(0)
		, mpOverlayBufferBody(NULL)
		, mpOverlayBufferBorder(NULL) {}

	COverlayData(const COverlayData& overlayData)
		: mOffsetX(overlayData.mOffsetX)
		, mOffsetY(overlayData.mOffsetY)
		, mOverlayWidth(overlayData.mOverlayWidth)
		, mOverlayHeight(overlayData.mOverlayHeight)
		, mOverlayPitch(overlayData.mOverlayPitch) {
		if (mOverlayPitch > 0 && mOverlayHeight > 0) {
			mpOverlayBufferBody = (byte*)_aligned_malloc(mOverlayPitch * mOverlayHeight, 16);
			mpOverlayBufferBorder = (byte*)_aligned_malloc(mOverlayPitch * mOverlayHeight, 16);
			if (!mpOverlayBufferBody || !mpOverlayBufferBorder) {
				mOffsetX = mOffsetY = 0;
				mOverlayWidth = mOverlayHeight = 0;
				DeleteOverlay();
			}
			memcpy(mpOverlayBufferBody, overlayData.mpOverlayBufferBody, mOverlayPitch * mOverlayHeight);
			memcpy(mpOverlayBufferBorder, overlayData.mpOverlayBufferBorder, mOverlayPitch * mOverlayHeight);
		} else {
			mpOverlayBufferBody = mpOverlayBufferBorder = NULL;
		}
	}

	~COverlayData() {
		DeleteOverlay();
	}

	COverlayData& operator=(const COverlayData& overlayData) {
		mOffsetX = overlayData.mOffsetX;
		mOffsetY = overlayData.mOffsetY;
		mOverlayWidth = overlayData.mOverlayWidth;
		mOverlayHeight = overlayData.mOverlayHeight;
		mOverlayPitch = overlayData.mOverlayPitch;

		DeleteOverlay();
		if (mOverlayPitch > 0 && mOverlayHeight > 0) {
			mpOverlayBufferBody = (byte*)_aligned_malloc(mOverlayPitch * mOverlayHeight, 16);
			mpOverlayBufferBorder = (byte*)_aligned_malloc(mOverlayPitch * mOverlayHeight, 16);
			if (!mpOverlayBufferBody || !mpOverlayBufferBorder) {
				mOffsetX = mOffsetY = 0;
				mOverlayWidth = mOverlayHeight = 0;
				DeleteOverlay();
			}
			memcpy(mpOverlayBufferBody, overlayData.mpOverlayBufferBody, mOverlayPitch * mOverlayHeight);
			memcpy(mpOverlayBufferBorder, overlayData.mpOverlayBufferBorder, mOverlayPitch * mOverlayHeight);
		} else {
			mpOverlayBufferBody = mpOverlayBufferBorder = NULL;
		}

		return *this;
	};

	void DeleteOverlay() {
		if (mpOverlayBufferBody) {
			_aligned_free(mpOverlayBufferBody);
			mpOverlayBufferBody = NULL;
		}
		if (mpOverlayBufferBorder) {
			_aligned_free(mpOverlayBufferBorder);
			mpOverlayBufferBorder = NULL;
		}
	}
};

typedef std::shared_ptr<COverlayData> COverlayDataSharedPtr;

class Rasterizer
{
	bool fFirstSet;
	CPoint firstp, lastp;

protected:
	BYTE* mpPathTypes;
	POINT* mpPathPoints;
	int mPathPoints;
	bool m_bUseAVX2;

private:
	enum {
		LINE_DOWN,
		LINE_UP
	};


	struct Edge {
		int next;
		int posandflag;
	}* mpEdgeBuffer;
	unsigned int mEdgeHeapSize;
	unsigned int mEdgeNext;

	unsigned int* mpScanBuffer;

protected:
	CEllipseSharedPtr m_pEllipse;
	COutlineDataSharedPtr m_pOutlineData;
	COverlayDataSharedPtr m_pOverlayData;

private:
	void _TrashPath();
	void _ReallocEdgeBuffer(unsigned int edges);
	void _EvaluateBezier(int ptbase, bool fBSpline);
	void _EvaluateLine(int pt1idx, int pt2idx);
	void _EvaluateLine(int x0, int y0, int x1, int y1);
	// The following function is templated and forcingly inlined for performance sake
	template<int flag> __forceinline void _EvaluateLine(int x0, int y0, int x1, int y1);
	static void _OverlapRegion(tSpanBuffer& dst, const tSpanBuffer& src, int dx, int dy);
	void CreateWidenedRegionFast(int borderX, int borderY);

public:
	Rasterizer();
	virtual ~Rasterizer();

	bool BeginPath(HDC hdc);
	bool EndPath(HDC hdc);
	bool PartialBeginPath(HDC hdc, bool bClearPath);
	bool PartialEndPath(HDC hdc, long dx, long dy);
	bool ScanConvert();
	bool CreateWidenedRegion(int borderX, int borderY);
	bool Rasterize(int xsub, int ysub, int fBlur, double fGaussianBlur);
	int getOverlayWidth() const;

	CRect Draw(SubPicDesc& spd, CRect& clipRect, byte* pAlphaMask, int xsub, int ysub, const DWORD* switchpts, bool fBody, bool fBorder) const;
	void FillSolidRect(SubPicDesc& spd, int x, int y, int nWidth, int nHeight, DWORD lColor) const;
};
