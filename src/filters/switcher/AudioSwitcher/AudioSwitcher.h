/*
 * (C) 2003-2006 Gabest
 * (C) 2006-2016 see Authors.txt
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

#include "StreamSwitcher.h"
#include "AudioNormalizer.h"
#include "../../../AudioTools/Mixer.h"
#include "../../../AudioTools/BassRedirect.h"

#define AudioSwitcherName L"MPC AudioSwitcher"

template <typename T>
class CSimpleBuffer
{
	T* m_data = NULL;
	size_t m_size = 0;

public:
	CSimpleBuffer() {};
	~CSimpleBuffer()
	{
		if (m_data) {
			delete[] m_data;
		}
	}

	// returns pointer to the data
	T* Data() { return m_data; }
	// returns the number of elements
	size_t Size() { return m_size; }
	// returns allocated size in bytes
	size_t Bytes() { return m_size * sizeof(T); }

	// set new size. old data will be lost
	void SetSize(const size_t size)
	{
		if (m_data) {
			delete[] m_data;
		}
		if (size) {
			m_data = DNew T[size];
		} else {
			m_data = NULL;
		}
		m_size = size;
	}

	// increase the size if necessary. old data may be lost
	void ExpandSize(const size_t size)
	{
		if (size > m_size) {
			SetSize(size);
		}
	}
};

enum {
	SPK_MONO = 0,
	SPK_STEREO,
	SPK_4_0,
	SPK_5_0,
	SPK_5_1,
	SPK_7_1
};

interface __declspec(uuid("CEDB2890-53AE-4231-91A3-B0AAFCD1DBDE"))
IAudioSwitcherFilter :
public IUnknown {
	STDMETHOD(SetChannelMixer) (bool bMixer, int nLayout) PURE;
	STDMETHOD(SetAudioGain) (float fGain_dB) PURE;
	STDMETHOD(GetAutoVolumeControl) (bool& bAutoVolumeControl, bool& bNormBoost, int& iNormLevel, int& iNormRealeaseTime) PURE;
	STDMETHOD(SetAutoVolumeControl) (bool bAutoVolumeControl, bool bNormBoost, int iNormLevel, int iNormRealeaseTime) PURE;
	STDMETHOD_(REFERENCE_TIME, GetAudioTimeShift) () PURE;
	STDMETHOD(SetAudioTimeShift) (REFERENCE_TIME rtAudioTimeShift) PURE;
	STDMETHOD(SetBassRedirect) (bool bBassRedirect) PURE;
};

class __declspec(uuid("18C16B08-6497-420e-AD14-22D21C2CEAB7"))
	CAudioSwitcherFilter : public CStreamSwitcherFilter, public IAudioSwitcherFilter
{
	CSimpleBuffer<float> m_buffer;

	CMixer	m_Mixer;
	bool	m_bMixer;
	int		m_nMixerLayout;

	CBassRedirect m_BassRedirect;
	bool	m_bBassRedirect;

	CAudioNormalizer m_AudioNormalizer;
	bool	m_bAutoVolumeControl;
	bool	m_bNormBoost;
	int		m_iNormLevel;
	int		m_iNormRealeaseTime;

	float	m_fGain_dB;
	float	m_fGainFactor;

	REFERENCE_TIME m_rtAudioTimeShift;

	REFERENCE_TIME m_rtNextStart;

public:
	CAudioSwitcherFilter(LPUNKNOWN lpunk, HRESULT* phr);
	~CAudioSwitcherFilter();

	DECLARE_IUNKNOWN
	STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);

	HRESULT CheckMediaType(const CMediaType* pmt);
	HRESULT Transform(IMediaSample* pIn, IMediaSample* pOut);
	void TransformMediaType(CMediaType& mt);

	// IAudioSwitcherFilter
	STDMETHODIMP SetChannelMixer(bool bMixer, int nLayout);
	STDMETHODIMP SetAudioGain(float fGain_dB);
	STDMETHODIMP GetAutoVolumeControl(bool& bAutoVolumeControl, bool& bNormBoost, int& iNormLevel, int& iNormRealeaseTime);
	STDMETHODIMP SetAutoVolumeControl(bool bAutoVolumeControl, bool bNormBoost, int iNormLevel, int iNormRealeaseTime);
	STDMETHODIMP_(REFERENCE_TIME) GetAudioTimeShift();
	STDMETHODIMP SetAudioTimeShift(REFERENCE_TIME rtAudioTimeShift);
	STDMETHODIMP SetBassRedirect(bool bBassRedirect);

	// IAMStreamSelect
	STDMETHODIMP Enable(long lIndex, DWORD dwFlags);
};
