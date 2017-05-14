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

#include "../../../DSUtil/Packet.h"
#include "BaseSplitterOutputPin.h"
#include "Teletext.h"

struct MpegParseContext {
	bool   bFrameStartFound = false;
	UINT64 state64          = 0;
};

class CBaseSplitterParserOutputPin : public CBaseSplitterOutputPin, protected CCritSec
{
	class CH264Packet : public CPacket
	{
		public:
			BOOL bSliceExist = FALSE;
	};

	CAutoPtr<CPacket>         m_p;
	CAutoPtrList<CH264Packet> m_pl;

	MpegParseContext m_ParseContext;
	CTeletext        m_teletext;

	bool  m_bHasAccessUnitDelimiters = false;
	bool  m_bFlushed                 = false;
	bool  m_bEndOfStream             = false;
	int   m_truehd_framelength       = 0;

	WORD  m_nChannels      = 0;
	DWORD m_nSamplesPerSec = 0;
	WORD  m_wBitsPerSample = 0;

	int   m_adx_block_size = 0;

	DWORD packetFlag       = 0;

protected:
	HRESULT DeliverNewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate);
	HRESULT DeliverPacket(CAutoPtr<CPacket> p);

	HRESULT Flush();

	void InitPacket(CPacket* pSource);

	HRESULT ParseAAC(CAutoPtr<CPacket> p);
	HRESULT ParseAACLATM(CAutoPtr<CPacket> p);
	HRESULT ParseAnnexB(CAutoPtr<CPacket> p, bool bConvertToAVCC);
	HRESULT ParseHEVC(CAutoPtr<CPacket> p);
	HRESULT ParseVC1(CAutoPtr<CPacket> p);
	HRESULT ParseHDMVLPCM(CAutoPtr<CPacket> p);
	HRESULT ParseAC3(CAutoPtr<CPacket> p);
	HRESULT ParseTrueHD(CAutoPtr<CPacket> p, BOOL bCheckAC3 = TRUE);
	HRESULT ParseDirac(CAutoPtr<CPacket> p);
	HRESULT ParseVobSub(CAutoPtr<CPacket> p);
	HRESULT ParseAdxADPCM(CAutoPtr<CPacket> p);
	HRESULT ParseDTS(CAutoPtr<CPacket> p);
	HRESULT ParseTeletext(CAutoPtr<CPacket> p);
	HRESULT ParseMpegVideo(CAutoPtr<CPacket> p);

public:
	CBaseSplitterParserOutputPin(CAtlArray<CMediaType>& mts, LPCWSTR pName, CBaseFilter* pFilter, CCritSec* pLock, HRESULT* phr);
	virtual ~CBaseSplitterParserOutputPin();

	HRESULT DeliverEndOfStream();
};
