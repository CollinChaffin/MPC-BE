/*
 * (C) 2014-2017 see Authors.txt
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

#include "AudioFile.h"

const uint8_t AMR_header[6]   = { '#', '!', 'A', 'M', 'R', 0x0A };
const uint8_t AMRWB_header[9] = { '#', '!', 'A', 'M', 'R', '-', 'W', 'B', 0x0A };

class CAMRFile : public CAudioFile
{
	struct frame_t {
		UINT64 size : 8, pos : 54;
	};

	int                  m_framelen;
	bool                 m_isAMRWB;
	unsigned             m_currentframe;
	std::vector<frame_t> m_seek_table;

public:
	CAMRFile();
	~CAMRFile();

	HRESULT Open(CBaseSplitterFile* pFile);
	REFERENCE_TIME Seek(REFERENCE_TIME rt);
	int GetAudioFrame(CPacket* packet, REFERENCE_TIME rtStart);
	CString GetName() const { return L"AMR"; };
};
