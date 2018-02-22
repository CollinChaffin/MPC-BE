/*
 * (C) 2016-2018 see Authors.txt
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

#include <d3d9.h>

bool RetrieveBitmapData(unsigned w, unsigned h, unsigned bpp, BYTE* dst, BYTE* src, int srcpitch);

HRESULT DumpDX9Surface(IDirect3DDevice9* pD3DDev, IDirect3DSurface9* pSurface, wchar_t* filename);
HRESULT DumpDX9Texture(IDirect3DDevice9* pD3DDev, IDirect3DTexture9* pTexture, wchar_t* filename);
HRESULT DumpDX9RenderTarget(IDirect3DDevice9* pD3DDev, wchar_t* filename);

HRESULT SaveRAWVideoAsBMP(BYTE* data, DWORD format, unsigned pitch, unsigned width, unsigned height, wchar_t* filename);
