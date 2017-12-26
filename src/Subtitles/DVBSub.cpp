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
#include "DVBSub.h"
#include "../DSUtil/GolombBuffer.h"

#if (0)		// Set to 1 to activate DVB subtitles traces
	#define TRACE_DVB	DLog
#else
	#define TRACE_DVB	__noop
#endif

#define BUFFER_CHUNK_GROW 0x1000

CDVBSub::CDVBSub()
	: CBaseSub(ST_DVB)
{
}

CDVBSub::~CDVBSub()
{
	Reset();
	SAFE_DELETE(m_pBuffer);
}

CDVBSub::DVB_PAGE* CDVBSub::FindPage(REFERENCE_TIME rt)
{
	POSITION pos = m_pages.GetHeadPosition();
	while (pos) {
		DVB_PAGE* pPage = m_pages.GetNext(pos);

		if (rt >= pPage->rtStart && rt < pPage->rtStop) {
			return pPage;
		}
	}
	return NULL;
}

CDVBSub::DVB_REGION* CDVBSub::FindRegion(DVB_PAGE* pPage, BYTE bRegionId)
{
	if (pPage != NULL) {
		POSITION pos = pPage->regions.GetHeadPosition();
		while (pos) {
			DVB_REGION* pRegion = pPage->regions.GetNext(pos);
			if (pRegion->id == bRegionId) {
				return pRegion;
			}
		}
	}
	return NULL;
}

CDVBSub::DVB_CLUT* CDVBSub::FindClut(DVB_PAGE* pPage, BYTE bClutId)
{
	if (pPage != NULL) {
		POSITION pos = pPage->CLUTs.GetHeadPosition();
		while (pos) {
			DVB_CLUT* pCLUT = pPage->CLUTs.GetNext(pos);
			if (pCLUT->id == bClutId) {
				return pCLUT;
			}
		}
	}
	return NULL;
}

CompositionObject* CDVBSub::FindObject(DVB_PAGE* pPage, SHORT sObjectId)
{
	if (pPage != NULL) {
		POSITION pos = pPage->objects.GetHeadPosition();
		while (pos) {
			CompositionObject* pObject = pPage->objects.GetNext(pos);
			if (pObject->m_object_id_ref == sObjectId) {
				return pObject;
			}
		}
	}
	return NULL;
}

HRESULT CDVBSub::AddToBuffer(BYTE* pData, int nSize)
{
	bool bFirstChunk = (*((LONG*)pData) & 0x00FFFFFF) == 0x000f0020;	// DVB sub start with 0x20 0x00 0x0F ...

	if (m_nBufferWritePos > 0 || bFirstChunk) {
		if (bFirstChunk) {
			m_nBufferWritePos	= 0;
			m_nBufferReadPos	= 0;
		}

		if (m_nBufferWritePos+nSize > m_nBufferSize) {
			if (m_nBufferWritePos+nSize > 20 * BUFFER_CHUNK_GROW) {
				// Too big to be a DVB sub !
				TRACE_DVB (_T("DVB - Too much data received..."));
				ASSERT (FALSE);

				Reset();
				return E_INVALIDARG;
			}

			BYTE* pPrev		= m_pBuffer;
			m_nBufferSize	= std::max(m_nBufferWritePos + nSize, m_nBufferSize + BUFFER_CHUNK_GROW);
			m_pBuffer		= DNew BYTE[m_nBufferSize];
			if (pPrev != NULL) {
				memcpy_s(m_pBuffer, m_nBufferSize, pPrev, m_nBufferWritePos);
				SAFE_DELETE(pPrev);
			}
		}
		memcpy_s(m_pBuffer + m_nBufferWritePos, m_nBufferSize, pData, nSize);
		m_nBufferWritePos += nSize;
		return S_OK;
	}
	return S_FALSE;
}

#define MARKER					\
	if (gb.BitRead(1) != 1) {	\
		ASSERT(FALSE);			\
		return E_FAIL;			\
	}

HRESULT CDVBSub::ParseSample(BYTE* pData, long nLen, REFERENCE_TIME rtStart, REFERENCE_TIME rtStop)
{
	CheckPointer(pData, E_POINTER);

	HRESULT				hr = S_OK;
	DVB_SEGMENT_TYPE	nCurSegment;

	if (*((LONG*)pData) == 0xBD010000) {
		CGolombBuffer gb(pData, nLen);

		int headerSize = 9;

		gb.SkipBytes(4);
		WORD wLength = (WORD)gb.BitRead(16);
		UNREFERENCED_PARAMETER(wLength);

		if (gb.BitRead(2) != 2) {
			return E_FAIL;	// type
		}

		gb.BitRead(2);	// scrambling
		gb.BitRead(1);	// priority
		gb.BitRead(1);	// alignment
		gb.BitRead(1);	// copyright
		gb.BitRead(1);	// original
		BYTE fpts = (BYTE)gb.BitRead(1);	// fpts
		BYTE fdts = (BYTE)gb.BitRead(1);	// fdts
		gb.BitRead(1);	// escr
		gb.BitRead(1);	// esrate
		gb.BitRead(1);	// dsmtrickmode
		gb.BitRead(1);	// morecopyright
		gb.BitRead(1);	// crc
		gb.BitRead(1);	// extension
		gb.BitRead(8);	// hdrlen

		if (fpts) {
			BYTE b = (BYTE)gb.BitRead(4);
			if (!(fdts && b == 3 || !fdts && b == 2)) {
				ASSERT(0);
				return E_FAIL;
			}

			REFERENCE_TIME	pts = 0;
			pts |= gb.BitRead(3) << 30;
			MARKER; // 32..30
			pts |= gb.BitRead(15) << 15;
			MARKER; // 29..15
			pts |= gb.BitRead(15);
			MARKER; // 14..0
			pts = 10000*pts / 90;

			TRACE_DVB(_T("DVB - Received a packet with a presentation timestamp PTS = %i64d[%s]"), pts, ReftimeToString(pts));
			if (pts != rtStart) {
				TRACE_DVB(_T("DVB - WARNING: The parsed PTS doesn't match the sample start time (%i64d[%s])"), rtStart, ReftimeToString(rtStart));
				ASSERT(FALSE);
				rtStart = pts;
			}

			headerSize	+= 5;
		}

		nLen  -= headerSize;
		pData += headerSize;
	}

	if (AddToBuffer(pData, nLen) == S_OK) {
		CGolombBuffer	gb(m_pBuffer + m_nBufferReadPos, m_nBufferWritePos - m_nBufferReadPos);
		int				nLastPos = 0;

		while (gb.RemainingSize() >= 6) { // Ensure there is enough data to parse the entire segment header
			if (gb.ReadByte() == 0x0F) {
				WORD	wPageId;
				WORD	wSegLength;

				nCurSegment	= (DVB_SEGMENT_TYPE)gb.ReadByte();
				wPageId		= gb.ReadShort();
				wSegLength	= gb.ReadShort();

				if (gb.RemainingSize() < wSegLength) {
					hr = S_FALSE;
					break;
				}

				TRACE_DVB (_T("DVB - ParseSample: Segment = %s, PageId = %d, SegLength/Buffer = %d/%d"), GetSegmentType(nCurSegment), wPageId, wSegLength, gb.RemainingSize());

				switch (nCurSegment) {
					case PAGE : {
						if (m_pCurrentPage != NULL) {
							TRACE_DVB(_T("DVB - Force End display"));
							EnqueuePage(rtStart);
						}
						UpdateTimeStamp(rtStart);

						CAutoPtr<DVB_PAGE> pPage;
						ParsePage(gb, wSegLength, pPage);

						if (pPage->pageState == DPS_ACQUISITION || pPage->pageState == DPS_MODE_CHANGE) {
							m_pCurrentPage			= pPage;
							m_pCurrentPage->rtStart	= rtStart;
							m_pCurrentPage->rtStop	= m_pCurrentPage->rtStart + m_pCurrentPage->pageTimeOut * 10000000;

							TRACE_DVB(_T("DVB - Page started [pageState = %d] %s, TimeOut = %ds"), m_pCurrentPage->pageState, ReftimeToString(rtStart), m_pCurrentPage->pageTimeOut);
						} else if (!m_pages.IsEmpty()) {
							m_pCurrentPage			= pPage;
							m_pCurrentPage->rtStart	= rtStart;
							m_pCurrentPage->rtStop	= m_pCurrentPage->rtStart + m_pCurrentPage->pageTimeOut * 10000000;

							// Copy data from the previous page
							DVB_PAGE* pPrevPage = m_pages.GetTail();

							for (POSITION pos = pPrevPage->regions.GetHeadPosition(); pos;) {
								m_pCurrentPage->regions.AddTail(DNew DVB_REGION(*pPrevPage->regions.GetNext(pos)));
							}

							for (POSITION pos = pPrevPage->objects.GetHeadPosition(); pos;) {
								m_pCurrentPage->objects.AddTail(pPrevPage->objects.GetNext(pos)->Copy());
							}

							for (POSITION pos = pPrevPage->CLUTs.GetHeadPosition(); pos;) {
								m_pCurrentPage->CLUTs.AddTail(DNew DVB_CLUT(*pPrevPage->CLUTs.GetNext(pos)));
							}

							TRACE_DVB(_T("DVB - Page started [update] %s, TimeOut = %ds"), ReftimeToString(rtStart), m_pCurrentPage->pageTimeOut);
						} else {
							TRACE_DVB(_T("DVB - Page update ignored %s"), ReftimeToString(rtStart));
						}
					}
					break;
					case REGION :
						ParseRegion(gb, wSegLength);
						TRACE_DVB (_T("DVB - Region"));
						break;
					case CLUT :
						ParseClut(gb, wSegLength);
						TRACE_DVB (_T("DVB - Clut"));
						break;
					case OBJECT :
						ParseObject(gb, wSegLength);
						TRACE_DVB (_T("DVB - Object"));
						break;
					case DISPLAY :
						ParseDisplay(gb, wSegLength);
						TRACE_DVB (_T("DVB - Display"));
						break;
					case END_OF_DISPLAY :
						if (m_pCurrentPage == NULL) {
							TRACE_DVB(_T("DVB - Ignored End display %s: no current page"), ReftimeToString(rtStart));
						} else if (m_pCurrentPage->rtStart < rtStart) {
							TRACE_DVB(_T("DVB - End display"));
							EnqueuePage(rtStart);
						} else {
							TRACE_DVB(_T("DVB - Ignored End display %s: no information on page duration"), ReftimeToString(rtStart));
						}
						break;
					default :
						break;
				}
				nLastPos = gb.GetPos();
			}
		}
		m_nBufferReadPos += nLastPos;
	}

	return hr;
}

HRESULT CDVBSub::EndOfStream()
{
	// Enqueue the last page if necessary.
	TRACE_DVB(_T("DVB - EndOfStream"));
	if (m_pCurrentPage) {
		TRACE_DVB(_T("DVB - EndOfStream : Enqueue last page"));
		EnqueuePage(INVALID_TIME);
	} else {
		TRACE_DVB(_T("DVB - EndOfStream : ignored - no page to enqueue\n"));
	}

	return S_OK;
}


void CDVBSub::CleanOld(REFERENCE_TIME rt)
{
	// Cleanup old PG
	while (m_pages.GetCount() && m_pages.GetHead()->rtStart < rt) {
		DVB_PAGE* pPage_old = m_pages.GetHead();
		if (!pPage_old->rendered) {
			TRACE_DVB (_T("DVB - remove unrendered object, %s => %s, (rt = %s)"),
						ReftimeToString(pPage_old->rtStart), ReftimeToString(pPage_old->rtStop),
						ReftimeToString(rt));
		}
		m_pages.RemoveHeadNoReturn();
		delete pPage_old;
	}
}

HRESULT CDVBSub::Render(SubPicDesc& spd, REFERENCE_TIME rt, RECT& bbox)
{
	HRESULT hr = E_FAIL;

	if (DVB_PAGE* pPage = FindPage(rt)) {
		pPage->rendered = true;
		TRACE_DVB(_T("DVB - Renderer - %s - %s"), ReftimeToString(pPage->rtStart), ReftimeToString(pPage->rtStop));

		int nRegion = 1, nObject = 1;
		for (POSITION pos = pPage->regionsPos.GetHeadPosition(); pos; nRegion++) {
			DVB_REGION_POS regionPos = pPage->regionsPos.GetNext(pos);
			if (DVB_REGION* pRegion = FindRegion(pPage, regionPos.id)) {
				if (DVB_CLUT* pCLUT = FindClut(pPage, pRegion->CLUT_id)) {
					for (POSITION posO = pRegion->objects.GetHeadPosition(); posO; nObject++) {
						DVB_OBJECT objectPos = pRegion->objects.GetNext(posO);
						if (CompositionObject* pObject = FindObject(pPage, objectPos.object_id)) {
							SHORT nX = regionPos.horizAddr + objectPos.object_horizontal_position;
							SHORT nY = regionPos.vertAddr  + objectPos.object_vertical_position;
							pObject->m_width  = pRegion->width;
							pObject->m_height = pRegion->height;
							pObject->SetPalette(pCLUT->size, pCLUT->palette, yuvMatrix == L"709" ? true : yuvMatrix == L"601" ? false : m_Display.width > 720, convertType);

							InitSpd(spd, m_Display.width, m_Display.height);
							pObject->RenderDvb(spd, nX, nY, m_bResizedRender ? &m_spd : NULL);
							TRACE_DVB(_T(" --> %d/%d - %d/%d"), nRegion, pPage->regionsPos.GetCount(), nObject, pRegion->objects.GetCount());
						}
					}
				}
			}
		}

		bbox.left	= 0;
		bbox.top	= 0;
		bbox.right	= spd.w;
		bbox.bottom	= spd.h;

		FinalizeRender(spd);

		hr = S_OK;
	}

	return hr;
}

HRESULT CDVBSub::GetTextureSize(POSITION pos, SIZE& MaxTextureSize, SIZE& VideoSize, POINT& VideoTopLeft)
{
	MaxTextureSize.cx = VideoSize.cx = m_Display.width;
	MaxTextureSize.cy = VideoSize.cy = m_Display.height;

	VideoTopLeft.x	= 0;
	VideoTopLeft.y	= 0;

	return S_OK;
}

POSITION CDVBSub::GetStartPosition(REFERENCE_TIME rt, double fps, bool CleanOld/* = false*/)
{
	if (CleanOld) {
		CDVBSub::CleanOld(rt);
	}
	POSITION pos = m_pages.GetHeadPosition();
	while (pos) {
		DVB_PAGE* pPage = m_pages.GetAt(pos);
		if (pPage->rtStop <= rt) {
			m_pages.GetNext(pos);
		} else {
			break;
		}
	}

	return pos;
}

POSITION CDVBSub::GetNext(POSITION pos)
{
	m_pages.GetNext(pos);
	return pos;
}

REFERENCE_TIME CDVBSub::GetStart(POSITION nPos)
{
	DVB_PAGE* pPage = m_pages.GetAt(nPos);
	return pPage != NULL ? pPage->rtStart : INVALID_TIME;
}

REFERENCE_TIME CDVBSub::GetStop(POSITION nPos)
{
	DVB_PAGE* pPage = m_pages.GetAt(nPos);
	return pPage != NULL ? pPage->rtStop : INVALID_TIME;
}

void CDVBSub::Reset()
{
	m_nBufferReadPos	= 0;
	m_nBufferWritePos	= 0;
	m_pCurrentPage.Free();

	while (!m_pages.IsEmpty()) {
		DVB_PAGE* pPage = m_pages.RemoveHead();
		delete pPage;
	}

}

HRESULT CDVBSub::ParsePage(CGolombBuffer& gb, WORD wSegLength, CAutoPtr<DVB_PAGE>& pPage)
{
	int nEnd = gb.GetPos() + wSegLength;

	pPage.Attach(DNew DVB_PAGE());
	pPage->pageTimeOut			= gb.ReadByte();
	pPage->pageVersionNumber	= (BYTE)gb.BitRead(4);
	pPage->pageState			= (BYTE)gb.BitRead(2);
	gb.BitRead(2);  // Reserved
	while (gb.GetPos() < nEnd) {
		DVB_REGION_POS regionPos;
		regionPos.id			= gb.ReadByte();
		gb.ReadByte();  // Reserved
		regionPos.horizAddr		= gb.ReadShort();
		regionPos.vertAddr		= gb.ReadShort();
		pPage->regionsPos.AddTail(regionPos);
	}

	return S_OK;
}

HRESULT CDVBSub::ParseDisplay(CGolombBuffer& gb, WORD wSegLength)
{
	m_Display.version_number		= (BYTE)gb.BitRead(4);
	m_Display.display_window_flag	= (BYTE)gb.BitRead(1);
	gb.BitRead(3);	// reserved
	m_Display.width					= gb.ReadShort() + 1;
	m_Display.height				= gb.ReadShort() + 1;
	if (m_Display.display_window_flag) {
		m_Display.horizontal_position_minimun	= gb.ReadShort();
		m_Display.horizontal_position_maximum	= gb.ReadShort();
		m_Display.vertical_position_minimun		= gb.ReadShort();
		m_Display.vertical_position_maximum		= gb.ReadShort();
	}

	return S_OK;
}

HRESULT CDVBSub::ParseRegion(CGolombBuffer& gb, WORD wSegLength)
{
	HRESULT hr = E_FAIL;
	int nEnd = gb.GetPos() + wSegLength;

	if (m_pCurrentPage && wSegLength >= 9) {
		BYTE id = gb.ReadByte();
		DVB_REGION* pRegion = FindRegion(m_pCurrentPage, id);

		bool bIsNewRegion = (pRegion == NULL);
		if (bIsNewRegion) {
			pRegion = DNew DVB_REGION();
		}

		pRegion->id = id;
		pRegion->version_number			= (BYTE)gb.BitRead(4);
		pRegion->fill_flag				= (BYTE)gb.BitRead(1);
		gb.BitRead(3);  // Reserved
		pRegion->width					= gb.ReadShort();
		pRegion->height					= gb.ReadShort();
		pRegion->level_of_compatibility	= (BYTE)gb.BitRead(3);
		pRegion->depth					= (BYTE)gb.BitRead(3);
		gb.BitRead(2);  // Reserved
		pRegion->CLUT_id				= gb.ReadByte();
		pRegion->_8_bit_pixel_code		= gb.ReadByte();
		pRegion->_4_bit_pixel_code		= (BYTE)gb.BitRead(4);
		pRegion->_2_bit_pixel_code		= (BYTE)gb.BitRead(2);
		gb.BitRead(2);  // Reserved

		while (gb.GetPos() < nEnd) {
			DVB_OBJECT object;
			object.object_id					= gb.ReadShort();
			object.object_type					= (BYTE)gb.BitRead(2);
			object.object_provider_flag			= (BYTE)gb.BitRead(2);
			object.object_horizontal_position	= (SHORT)gb.BitRead(12);
			gb.BitRead(4);  // Reserved
			object.object_vertical_position = (SHORT)gb.BitRead(12);
			if (object.object_type == 0x01 || object.object_type == 0x02) {
				object.foreground_pixel_code	= gb.ReadByte();
				object.background_pixel_code	= gb.ReadByte();
			}
			pRegion->objects.AddTail(object);
		}

		if (bIsNewRegion) {
			m_pCurrentPage->regions.AddTail(pRegion);
		}

		hr = S_OK;
	}

	return hr;
}

HRESULT CDVBSub::ParseClut(CGolombBuffer& gb, WORD wSegLength)
{
	HRESULT hr = E_FAIL;
	int nEnd = gb.GetPos() + wSegLength;

	if (m_pCurrentPage && wSegLength > 2) {
		BYTE id = gb.ReadByte();
		DVB_CLUT* pClut = FindClut(m_pCurrentPage, id);

		bool bIsNewClut = (pClut == NULL);
		if (bIsNewClut) {
			pClut = DNew DVB_CLUT();
		}

		pClut->id = id;
		pClut->version_number = (BYTE)gb.BitRead(4);
		gb.BitRead(4);  // Reserved

		pClut->size = 0;
		while (gb.GetPos() < nEnd) {
			pClut->palette[pClut->size].entry_id = gb.ReadByte();

			BYTE _2_bit = (BYTE)gb.BitRead(1);
			BYTE _4_bit = (BYTE)gb.BitRead(1);
			BYTE _8_bit = (BYTE)gb.BitRead(1);
			UNREFERENCED_PARAMETER(_2_bit);
			UNREFERENCED_PARAMETER(_4_bit);
			UNREFERENCED_PARAMETER(_8_bit);
			gb.BitRead(4);  // Reserved

			if (gb.BitRead(1)) {
				pClut->palette[pClut->size].Y	= gb.ReadByte();
				pClut->palette[pClut->size].Cr	= gb.ReadByte();
				pClut->palette[pClut->size].Cb	= gb.ReadByte();
				pClut->palette[pClut->size].T	= 0xff - gb.ReadByte();
			} else {
				pClut->palette[pClut->size].Y	= (BYTE)gb.BitRead(6) << 2;
				pClut->palette[pClut->size].Cr	= (BYTE)gb.BitRead(4) << 4;
				pClut->palette[pClut->size].Cb	= (BYTE)gb.BitRead(4) << 4;
				pClut->palette[pClut->size].T	= 0xff - ((BYTE)gb.BitRead(2) << 6);
			}
			if (!pClut->palette[pClut->size].Y) {
				pClut->palette[pClut->size].Cr	= 0;
				pClut->palette[pClut->size].Cb	= 0;
				pClut->palette[pClut->size].T	= 0;
			}

			pClut->size++;
		}

		if (bIsNewClut) {
			m_pCurrentPage->CLUTs.AddTail(pClut);
		}

		hr = S_OK;
	}

	return hr;
}

HRESULT CDVBSub::ParseObject(CGolombBuffer& gb, WORD wSegLength)
{
	HRESULT hr = E_FAIL;

	if (m_pCurrentPage && wSegLength > 2) {
		SHORT id = gb.ReadShort();
		CompositionObject* pObject = FindObject(m_pCurrentPage, id);

		bool bIsNewObject = (pObject == NULL);
		if (bIsNewObject) {
			pObject = DNew CompositionObject();
		}

		pObject->m_object_id_ref  = id;
		pObject->m_version_number = (BYTE)gb.BitRead(4);

		BYTE object_coding_method = (BYTE)gb.BitRead(2); // object_coding_method
		gb.BitRead(1);  // non_modifying_colour_flag
		gb.BitRead(1);  // reserved

		if (object_coding_method == 0x00) {
			pObject->SetRLEData(gb.GetBufferPos(), wSegLength - 3, wSegLength - 3);
			gb.SkipBytes(wSegLength - 3);

			if (bIsNewObject) {
				m_pCurrentPage->objects.AddTail(pObject);
			}

			hr = S_OK;
		} else {
			delete pObject;
			hr = E_NOTIMPL;
		}
	}

	return hr;
}

HRESULT CDVBSub::EnqueuePage(REFERENCE_TIME rtStop)
{
	ASSERT(m_pCurrentPage != NULL);
	if (m_pCurrentPage->rtStart < rtStop && m_pCurrentPage->rtStop > rtStop) {
		m_pCurrentPage->rtStop = rtStop;
	}
	TRACE_DVB(_T(" %s (%s - %s)"), ReftimeToString(rtStop), ReftimeToString(m_pCurrentPage->rtStart), ReftimeToString(m_pCurrentPage->rtStop));
	m_pages.AddTail(m_pCurrentPage.Detach());

	return S_OK;
}

HRESULT CDVBSub::UpdateTimeStamp(REFERENCE_TIME rtStop)
{
	HRESULT hr = S_FALSE;
	POSITION pos = m_pages.GetTailPosition();
	while (pos) {
		DVB_PAGE* pPage = m_pages.GetPrev(pos);
		if (pPage->rtStop > rtStop) {
			TRACE_DVB(_T("DVB - Updated end of display %s - %s --> %s - %s"),
					  ReftimeToString(pPage->rtStart),
					  ReftimeToString(pPage->rtStop),
					  ReftimeToString(pPage->rtStart),
					  ReftimeToString(rtStop));
			pPage->rtStop = rtStop;
			hr = S_OK;
		} else {
			break;
		}
	}

	return hr;
}
