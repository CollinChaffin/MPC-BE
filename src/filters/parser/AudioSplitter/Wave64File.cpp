/*
 * (C) 2014-2016 see Authors.txt
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
#include "Wave64File.h"

const uint8_t w64_guid_list[16] = {'l', 'i', 's', 't', 0x2E, 0x91, 0xCF, 0x11, 0xA5, 0xD6, 0x28, 0xDB, 0x04, 0xC1, 0x00, 0x00};
const uint8_t w64_guid_fmt [16] = {'f', 'm', 't', ' ', 0xF3, 0xAC, 0xD3, 0x11, 0x8C, 0xD1, 0x00, 0xC0, 0x4F, 0x8E, 0xDB, 0x8A};
const uint8_t w64_guid_fact[16] = {'f', 'a', 'c', 't', 0xF3, 0xAC, 0xD3, 0x11, 0x8C, 0xD1, 0x00, 0xC0, 0x4F, 0x8E, 0xDB, 0x8A};
const uint8_t w64_guid_data[16] = {'d', 'a', 't', 'a', 0xF3, 0xAC, 0xD3, 0x11, 0x8C, 0xD1, 0x00, 0xC0, 0x4F, 0x8E, 0xDB, 0x8A};
const uint8_t w64_guid_levl[16] = {'l', 'e', 'v', 'l', 0xF3, 0xAC, 0xD3, 0x11, 0x8C, 0xD1, 0x00, 0xC0, 0x4F, 0x8E, 0xDB, 0x8A};
const uint8_t w64_guid_junk[16] = {'j', 'u', 'n', 'k', 0xF3, 0xAC, 0xD3, 0x11, 0x8C, 0xD1, 0x00, 0xC0, 0x4F, 0x8E, 0xDB, 0x8A};
const uint8_t w64_guid_bext[16] = {'b', 'e', 'x', 't', 0xF3, 0xAC, 0xD3, 0x11, 0x8C, 0xD1, 0x00, 0xC0, 0x4F, 0x8E, 0xDB, 0x8A};
const uint8_t w64_guid_marker[16] = {0x56, 0x62, 0xF7, 0xAB, 0x2D, 0x39, 0xD2, 0x11, 0x86, 0xC7, 0x00, 0xC0, 0x4F, 0x8E, 0xDB, 0x8A};
const uint8_t w64_guid_summarylist[16] = {0xBC, 0x94, 0x5F, 0x92, 0x5A, 0x52, 0xD2, 0x11, 0x86, 0xDC, 0x00, 0xC0, 0x4F, 0x8E, 0xDB, 0x8A};

#pragma pack(push, 1)
typedef union {
		struct {
			BYTE guid[16];
			LONGLONG size;
		};
		BYTE data[24];
	} chunk64_t;
#pragma pack(pop)

//
// Wave64File
//

CWave64File::CWave64File()
	: CWAVFile()
{
}

HRESULT CWave64File::Open(CBaseSplitterFile* pFile)
{
	m_pFile = pFile;
	m_startpos = 0;

	m_pFile->Seek(0);
	BYTE data[40];
	if (m_pFile->ByteRead(data, sizeof(data)) != S_OK
			|| memcmp(data, w64_guid_riff, 16) != 0
			|| *(LONGLONG*)(data+16) < (16 + 16 + 4 + sizeof(PCMWAVEFORMAT) + 16 + 16)
			// w64_guid_wave + w64_guid_fmt + fmt.size + sizeof(PCMWAVEFORMAT) + w64_guid_data + data.size)
			|| memcmp(data+24, w64_guid_wave, 16) != 0) {
		return E_FAIL;
	}
	__int64 end = std::min((__int64)*(LONGLONG*)(data + 16), m_pFile->GetLength());

	chunk64_t Chunk64;

	while (m_pFile->ByteRead(Chunk64.data, sizeof(Chunk64)) == S_OK && m_pFile->GetPos() < end) {
		if (Chunk64.size < sizeof(Chunk64)) {
			DLog(L"CWave64File::Open() : bad chunk size");
			return E_FAIL;
		}

		__int64 pos = m_pFile->GetPos();
		Chunk64.size -= sizeof(Chunk64);

		if (memcmp(Chunk64.guid, w64_guid_fmt, 16) == 0) {
			if (m_fmtdata || Chunk64.size < sizeof(PCMWAVEFORMAT) || Chunk64.size > 65536) {
				DLog(L"CWave64File::Open() : bad format");
				return E_FAIL;
			}
			m_fmtsize = std::max(Chunk64.size, (__int64)sizeof(WAVEFORMATEX)); // PCMWAVEFORMAT to WAVEFORMATEX
			m_fmtdata = DNew BYTE[m_fmtsize];
			memset(m_fmtdata, 0, m_fmtsize);
			if (m_pFile->ByteRead(m_fmtdata, Chunk64.size) != S_OK) {
				DLog(L"CWave64File::Open() : format can not be read.");
				return E_FAIL;
			}
		}
		else if (memcmp(Chunk64.guid, w64_guid_data, 16) == 0) {
			m_startpos	= m_pFile->GetPos();
			m_length	= std::min(Chunk64.size, m_pFile->GetLength() - m_startpos);
		}
		else if (memcmp(Chunk64.guid, w64_guid_list, 16) != 0
				&& memcmp(Chunk64.guid, w64_guid_fact, 16) != 0
				&& memcmp(Chunk64.guid, w64_guid_levl, 16) != 0
				&& memcmp(Chunk64.guid, w64_guid_junk, 16) != 0
				&& memcmp(Chunk64.guid, w64_guid_bext, 16) != 0
				&& memcmp(Chunk64.guid, w64_guid_marker, 16) != 0
				&& memcmp(Chunk64.guid, w64_guid_summarylist, 16) != 0) {
			DLog(L"CWave64File::Open() : bad or unknown chunk guid.");
			return E_FAIL;
		}

		m_pFile->Seek((pos + Chunk64.size + 7) & ~7i64);
	}

	if (!m_startpos || !ProcessWAVEFORMATEX()) {
		return E_FAIL;
	}

	m_length		-= m_length % m_nBlockAlign;
	m_endpos		= m_startpos + m_length;
	m_rtduration	= 10000000i64 * m_length / m_nAvgBytesPerSec;

	CheckDTSAC3CD();

	return S_OK;
}
