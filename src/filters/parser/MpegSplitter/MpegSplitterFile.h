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

#include <atlbase.h>
#include <atlcoll.h>
#include "../BaseSplitter/BaseSplitter.h"
#include "../../../DSUtil/GolombBuffer.h"

#define NO_SUBTITLE_PID  1 // Fake PID use for the "No subtitle" entry
#define NO_SUBTITLE_NAME L"No subtitle"

#define ISVALIDPID(pid)  (pid >= 0x1 && pid < 0x1fff)

enum MPEG_TYPES {
	mpeg_invalid,
	mpeg_ps,
	mpeg_ts,
	mpeg_pva
};

class CMpegSplitterFile : public CBaseSplitterFileEx
{
	CAtlMap<WORD, BYTE> m_pid2pes;

	CAtlMap<DWORD, seqhdr> seqh;
	CAtlMap<DWORD, CAtlArray<BYTE>> avch;
	CAtlMap<DWORD, CAtlArray<BYTE>> hevch;

	template<class T, int validCount = 5>
	class CValidStream {
		BYTE m_nValidStream;
		T m_val;
	public:
		CValidStream() {
			m_nValidStream = 0;
			memset(&m_val, 0, sizeof(m_val));
		}
		void Handle(T& val) {
			if (m_val == val) {
				m_nValidStream++;
			} else {
				m_nValidStream = 0;
			}
			memcpy(&m_val, &val, sizeof(val));
		}
		BOOL IsValid() const { return m_nValidStream >= validCount; }
	};

	CAtlMap<DWORD, CValidStream<latm_aachdr, 3>>	m_aaclatmValid;
	CAtlMap<DWORD, CValidStream<aachdr>>			m_aacValid;
	CAtlMap<DWORD, CValidStream<ac3hdr>>			m_ac3Valid;
	CAtlMap<DWORD, CValidStream<mpahdr>>			m_mpaValid;

	BOOL m_bOpeningCompleted;

	HRESULT Init(IAsyncReader* pAsyncReader);

	BOOL m_bIMKH_CCTV;

	typedef CAtlArray<SyncPoint> SyncPoints;
	CAtlMap<DWORD, SyncPoints> m_SyncPoints;

	int m_tslen = 0; // transport stream packet length (188 or 192 bytes, auto-detected)
	REFERENCE_TIME m_rtPTSOffset = 0;

public:
	struct pshdr {
		mpeg_t type;
		UINT64 scr, bitrate;
	};

	struct pssyshdr {
		DWORD rate_bound;
		BYTE video_bound, audio_bound;
		bool fixed_rate, csps;
		bool sys_video_loc_flag, sys_audio_loc_flag;
	};

	struct peshdr {
		WORD len;

		BYTE type:2, fpts:1, fdts:1;
		REFERENCE_TIME pts, dts;

		// mpeg1 stuff
		UINT64 std_buff_size;

		// mpeg2 stuff
		BYTE scrambling:2, priority:1, alignment:1, copyright:1, original:1;
		BYTE escr:1, esrate:1, dsmtrickmode:1, morecopyright:1, crc:1, extension:1;
		BYTE hdrlen;

		BYTE id_ext;

		struct peshdr() {
			memset(this, 0, sizeof(*this));
		}
	};

	// http://multimedia.cx/mirror/av_format_v1.pdf
	struct pvahdr
	{
		WORD sync; // 'VA'
		BYTE streamid; // 1 - video, 2 - audio
		BYTE counter;
		BYTE res1; // 0x55
		BYTE res2:3;
		BYTE fpts:1;
		BYTE postbytes:2;
		BYTE prebytes:2;
		WORD length;
		REFERENCE_TIME pts;

		__int64 hdrpos;
	};

	struct trhdr
	{
		__int64 PCR;
		__int64 next;
		int bytes;
		WORD pid:13;
		BYTE sync; // 0x47
		BYTE error:1;
		BYTE payloadstart:1;
		BYTE transportpriority:1;
		BYTE scrambling:2;
		BYTE adapfield:1;
		BYTE payload:1;
		BYTE counter:4;
		// if adapfield set
		BYTE length;
		BYTE discontinuity:1;
		BYTE randomaccess:1;
		BYTE priority:1;
		BYTE fPCR:1;
		BYTE OPCR:1;
		BYTE splicingpoint:1;
		BYTE privatedata:1;
		BYTE extension:1;

		__int64 hdrpos;
		// TODO: add more fields here when the flags above are set (they aren't very interesting...)
	};

	struct psihdr
	{
		BYTE section_syntax_indicator:1;
		BYTE zero:1;
		BYTE reserved1:2;
		int section_length:12;
		WORD transport_stream_id;
		BYTE table_id;
		BYTE reserved2:2;
		BYTE version_number:5;
		BYTE current_next_indicator:1;
		BYTE section_number;
		BYTE last_section_number;

		BYTE hdr_size;
	};

	bool ReadPS(pshdr& h);              // program stream header
	bool ReadPSS(pssyshdr& h);          // program stream system header

	bool ReadPES(peshdr& h, BYTE code); // packetized elementary stream

	bool ReadPVA(pvahdr& h, bool fSync = true);

	bool ReadTR(trhdr& h, bool fSync = true);
	bool ReadPSI(psihdr& h);

	enum stream_codec {
		NONE,
		H264,
		MVC,
		HEVC,
		MPEG,
		VC1,
		OPUS,
		DIRAC,
		AC3,
		EAC3,
		DTS,
		ARIB,
		PGS,
		DVB,
		TELETEXT
	};

	enum stream_type {
		video,
		audio,
		subpic,
		stereo,
		unknown
	};

	CHdmvClipInfo &m_ClipInfo;
	CMpegSplitterFile(IAsyncReader* pAsyncReader, HRESULT& hr, CHdmvClipInfo &ClipInfo, bool ForcedSub, int AC3CoreOnly, bool SubEmptyPin);

	BOOL CheckKeyFrame(CAtlArray<BYTE>& pData, stream_codec codec);
	REFERENCE_TIME NextPTS(DWORD TrackNum, stream_codec codec, __int64& nextPos, BOOL bKeyFrameOnly = FALSE, REFERENCE_TIME rtLimit = _I64_MAX);

	MPEG_TYPES m_type;

	BOOL m_bPESPTSPresent;

	int m_rate; // byte/sec

	int m_AC3CoreOnly;
	bool m_ForcedSub, m_SubEmptyPin;

	REFERENCE_TIME m_rtMin;
	REFERENCE_TIME m_rtMax;

	__int64 m_posMin;

	struct stream {
		CMediaType mt;
		std::vector<CMediaType> mts;

		WORD pid   = 0;
		BYTE pesid = 0;
		BYTE ps1id = 0;

		WORD tlxPage = 0;

		bool lang_set = false;
		char lang[4]  = {};

		struct {
			bool bDTSCore = false;
			bool bDTSHD   = false;
		} dts;

		stream_codec codec = stream_codec::NONE;

		operator DWORD() const {
			return pid ? pid : ((pesid << 8) | ps1id);
		}

		bool operator == (const stream& s) const {
			return (DWORD)*this == (DWORD)s;
		}
	};

	class CStreamList : public CAtlList<stream>
	{
	public:
		void Insert(stream& s, int type) {
			if (type == stream_type::subpic) {
				if (s.pid == NO_SUBTITLE_PID) {
					AddTail(s);
					return;
				}
				for (POSITION pos = GetHeadPosition(); pos; GetNext(pos)) {
					stream& s2 = GetAt(pos);
					if (s.pid < s2.pid || s2.pid == NO_SUBTITLE_PID) {
						InsertBefore(pos, s);
						return;
					}
				}
				AddTail(s);
			} else {
				Insert(s);
			}
		}

		void Insert(stream& s) {
			AddTail(s);
			if (GetCount() > 1) {
				for (size_t j = 0; j < GetCount(); j++) {
					for (size_t i = 0; i < GetCount() - 1; i++) {
						if (GetAt(FindIndex(i)) > GetAt(FindIndex(i+1))) {
							SwapElements(FindIndex(i), FindIndex(i+1));
						}
					}
				}
			}
		}

		void Replace(stream& source, stream& dest) {
			for (POSITION pos = GetHeadPosition(); pos; GetNext(pos)) {
				stream& s = GetAt(pos);
				if (source == s) {
					SetAt(pos, dest);
					return;
				}
			}
		}

		static CString ToString(int type) {
			return
				type == video	? L"Video" :
				type == audio	? L"Audio" :
				type == subpic	? L"Subtitle" :
				type == stereo	? L"Stereo" :
								  L"Unknown";
		}

		const stream* FindStream(DWORD pid) {
			for (POSITION pos = GetHeadPosition(); pos; GetNext(pos)) {
				const stream& s = GetAt(pos);
				if (s == pid) {
					return &s;
				}
			}

			return NULL;
		}
	};
	typedef CStreamList CStreamsList[unknown];
	CStreamsList m_streams;

	void SearchStreams(__int64 start, __int64 stop, DWORD msTimeOut = INFINITE);
	DWORD AddStream(WORD pid, BYTE pesid, BYTE ps1id, DWORD len, BOOL bAddStream = TRUE);
	void  AddHdmvPGStream(WORD pid, LPCSTR language_code);
	CMpegSplitterFile::CStreamList* GetMasterStream();
	bool IsHdmv() {
		return m_ClipInfo.IsHdmv();
	};

	// program map table - mpeg-ts
	struct program {
		WORD program_number      = 0;
		struct stream {
			WORD			pid  = 0;
			PES_STREAM_TYPE	type = PES_STREAM_TYPE::INVALID;
		};
		std::vector<stream> streams;

		CString name;

		size_t streamCount(CStreamsList s) {
			size_t cnt = 0;
			for (auto stream = streams.begin(); stream != streams.end(); stream++) {
				if (stream->pid) {
					for (int type = stream_type::video; type <= stream_type::subpic; type++) {
						if (s[type].FindStream(stream->pid)) {
							cnt++;
							break;
						}
					}
				}
			}

			return cnt;
		}

		bool streamFind(WORD pid, int* pStream = NULL) {
			for (size_t i = 0; i < streams.size(); i++) {
				if (streams[i].pid == pid) {
					if (pStream) {
						*pStream = i;
					}
					return true;
				}
			}

			return false;
		}
	};

	class CPrograms : public CAtlMap<WORD, program>
	{
		CStreamsList* s;
	public:
		CPrograms(CStreamsList* sList) {
			s = sList;
		}
		size_t GetValidCount() {
			size_t cnt = 0;
			POSITION pos = GetStartPosition();
			while (pos) {
				program* p = &GetNextValue(pos);
				if (p->streamCount(*s)) {
					cnt++;
				}
			}

			return cnt;
		}

		const program* FindProgram(WORD pid) {
			POSITION pos = GetStartPosition();
			while (pos) {
				program* p = &GetNextValue(pos);
				for (auto stream = p->streams.begin(); stream != p->streams.end(); stream++) {
					if (stream->pid == pid) {
						return p;
					}
				}
			}

			return NULL;
		}
	} m_programs;

	struct programData {
		BYTE table_id      = 0;
		int section_length = 0;
		bool bFinished     = false;
		CAtlArray<BYTE> pData;

		void Finish() {
			table_id       = 0;
			section_length = 0;
			bFinished      = true;
			pData.RemoveAll();
		}

		bool IsFull() { return !pData.IsEmpty() && pData.GetCount() == section_length; }
	};
	CAtlMap<WORD, programData> m_ProgramData;

	void SearchPrograms(__int64 start, __int64 stop);
	void ReadPrograms(const trhdr& h);
	void ReadPAT(CAtlArray<BYTE>& pData);
	void ReadPMT(CAtlArray<BYTE>& pData, WORD pid);
	void ReadSDT(CAtlArray<BYTE>& pData, BYTE table_id);
	void ReadVCT(CAtlArray<BYTE>& pData, BYTE table_id);

	const program* FindProgram(WORD pid, int* pStream = NULL, const CHdmvClipInfo::Stream** pClipInfo = NULL);

	// program stream map - mpeg-ps
	PES_STREAM_TYPE m_psm[256] = {};
	void UpdatePSM();

	bool GetStreamType(WORD pid, PES_STREAM_TYPE &stream_type);

	struct teletextPage {
		bool bSubtitle = false;
		WORD page      = 0;
		char lang[4]   = {};

		bool operator == (const teletextPage& tlxPage) const {
			return (page == tlxPage.page
				&& bSubtitle == tlxPage.bSubtitle
				&& !strcmp(lang, tlxPage.lang));
		}
	};
	typedef std::vector<teletextPage> teletextPages;

	struct streamData {
		struct {
			char            lang[4] = {};
			CAtlArray<BYTE> extraData;
			teletextPages   tlxPages;
		} pmt;

		BOOL         usePTS = FALSE;
		stream_codec codec  = stream_codec::NONE;

		streamData& operator = (const streamData& src) {
			strcpy_s(pmt.lang, src.pmt.lang);
			pmt.extraData.Append(src.pmt.extraData);
			pmt.tlxPages = src.pmt.tlxPages;

			usePTS = src.usePTS;
			codec  = src.codec;

			return *this;
		}
	};
	CAtlMap<WORD, streamData> m_streamData;
};
