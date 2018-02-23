/*
 * (C) 2014-2018 see Authors.txt
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

#include <basestruct.h>
#include <vector>

// A dynamic byte array with a guaranteed padded block at the end and no member initialization.
class CPaddedArray : private std::vector<NoInitByte>
{
private:
	size_t m_padsize;

public:
	CPaddedArray(size_t padsize)
		: m_padsize(padsize)
	{
	}

	uint8_t* Data()
	{
		return (uint8_t*)__super::data(); // don't use "&front().value" here, because it does not work for an empty array
	}

	size_t Size()
	{
		const size_t count = __super::size();
		return (count > m_padsize) ? count - m_padsize : 0;
	}

	bool Resize(size_t nNewSize)
	{
		try {
			__super::resize(nNewSize + m_padsize);
		}
		catch (...) {
			return false;
		}
		memset(Data() + nNewSize, 0, m_padsize);
		return true;
	}

	void Clear()
	{
		__super::clear();
	}

	bool Append(uint8_t* p, size_t nSize)
	{
		const size_t oldSize = Size();
		if (Resize(oldSize + nSize)) {
			memcpy(Data() + oldSize, p, nSize);
			return true;
		}
		return false;
	}

	void RemoveHead(size_t nSize)
	{
		const size_t count = Size();
		if (nSize >= count) {
			Clear();
		} else {
			memmove(Data(), Data() + nSize, count - nSize);
			Resize(count - nSize);
		}
	}
};
