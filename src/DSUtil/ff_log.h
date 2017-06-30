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

#pragma once

extern "C" {
	#include <ffmpeg/libavutil/log.h>
}

#define LOG_BUF_LEN 2048
inline void ff_log(void* ptr, int level, const char *fmt, va_list valist)
{
#ifdef DEBUG_OR_LOG
	if (level <= AV_LOG_VERBOSE) {
		static int print_prefix = 1;
		static char line[LOG_BUF_LEN] = {};

		av_log_format_line(ptr, level, fmt, valist, line, sizeof(line), &print_prefix);

		const size_t len = strnlen_s(line, LOG_BUF_LEN);
		if (len > 0 && line[len - 1] == '\n') {
			line[len - 1] = 0;
		}

		DLog(L"FF_LOG : %S", line);
	}
#endif
}
