/*
 * (C) 2011-2017 see Authors.txt
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
#include <Shlobj.h>
#include "WinAPIUtils.h"
#include "Log.h"
#include "../apps/mplayerc/resource.h"

// retrieves the monitor EDID info
// thanks to JanWillem32 for this code
static UINT16 const gk_au16CodePage437ForEDIDLookup[256] = {
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,// first 32 control characters taken out, to prevent them from printing
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0020, 0x0021, 0x0022, 0x0023, 0x0024, 0x0025, 0x0026, 0x0027, 0x0028, 0x0029, 0x002A, 0x002B, 0x002C, 0x002D, 0x002E, 0x002F,
	0x0030, 0x0031, 0x0032, 0x0033, 0x0034, 0x0035, 0x0036, 0x0037, 0x0038, 0x0039, 0x003A, 0x003B, 0x003C, 0x003D, 0x003E, 0x003F,
	0x0040, 0x0041, 0x0042, 0x0043, 0x0044, 0x0045, 0x0046, 0x0047, 0x0048, 0x0049, 0x004A, 0x004B, 0x004C, 0x004D, 0x004E, 0x004F,
	0x0050, 0x0051, 0x0052, 0x0053, 0x0054, 0x0055, 0x0056, 0x0057, 0x0058, 0x0059, 0x005A, 0x005B, 0x005C, 0x005D, 0x005E, 0x005F,
	0x0060, 0x0061, 0x0062, 0x0063, 0x0064, 0x0065, 0x0066, 0x0067, 0x0068, 0x0069, 0x006A, 0x006B, 0x006C, 0x006D, 0x006E, 0x006F,
	0x0070, 0x0071, 0x0072, 0x0073, 0x0074, 0x0075, 0x0076, 0x0077, 0x0078, 0x0079, 0x007A, 0x007B, 0x007C, 0x007D, 0x007E, 0x0000,// control character DEL (0x7F) taken out
	0x00C7, 0x00FC, 0x00E9, 0x00E2, 0x00E4, 0x00E0, 0x00E5, 0x00E7, 0x00EA, 0x00EB, 0x00E8, 0x00EF, 0x00EE, 0x00EC, 0x00C4, 0x00C5,
	0x00C9, 0x00E6, 0x00C6, 0x00F4, 0x00F6, 0x00F2, 0x00FB, 0x00F9, 0x00FF, 0x00D6, 0x00DC, 0x00A2, 0x00A3, 0x00A5, 0x20A7, 0x0192,
	0x00E1, 0x00ED, 0x00F3, 0x00FA, 0x00F1, 0x00D1, 0x00AA, 0x00BA, 0x00BF, 0x2310, 0x00AC, 0x00BD, 0x00BC, 0x00A1, 0x00AB, 0x00BB,
	0x2591, 0x2592, 0x2593, 0x2502, 0x2524, 0x2561, 0x2562, 0x2556, 0x2555, 0x2563, 0x2551, 0x2557, 0x255D, 0x255C, 0x255B, 0x2510,
	0x2514, 0x2534, 0x252C, 0x251C, 0x2500, 0x253C, 0x255E, 0x255F, 0x255A, 0x2554, 0x2569, 0x2566, 0x2560, 0x2550, 0x256C, 0x2567,
	0x2568, 0x2564, 0x2565, 0x2559, 0x2558, 0x2552, 0x2553, 0x256B, 0x256A, 0x2518, 0x250C, 0x2588, 0x2584, 0x258C, 0x2590, 0x2580,
	0x03B1, 0x00DF, 0x0393, 0x03C0, 0x03A3, 0x03C3, 0x00B5, 0x03C4, 0x03A6, 0x0398, 0x03A9, 0x03B4, 0x221E, 0x03C6, 0x03B5, 0x2229,
	0x2261, 0x00B1, 0x2265, 0x2264, 0x2320, 0x2321, 0x00F7, 0x2248, 0x00B0, 0x2219, 0x00B7, 0x221A, 0x207F, 0x00B2, 0x25A0, 0x00A0
};

bool ReadDisplay(CString szDevice, CString* MonitorName, UINT16* MonitorHorRes, UINT16* MonitorVerRes)
{
	wchar_t szMonitorName[14] = { 0 };
	UINT16 nMonitorHorRes = 0;
	UINT16 nMonitorVerRes = 0;

	if (MonitorHorRes) {
		*MonitorHorRes = 0;
	}

	if (MonitorVerRes) {
		*MonitorVerRes = 0;
	}

	DISPLAY_DEVICE DisplayDevice;
	DisplayDevice.cb = sizeof(DISPLAY_DEVICEW);

	DWORD dwDevNum = 0;
	while (EnumDisplayDevicesW(nullptr, dwDevNum++, &DisplayDevice, 0)) {

		if ((DisplayDevice.StateFlags & DISPLAY_DEVICE_ACTIVE)
			&& !(DisplayDevice.StateFlags & DISPLAY_DEVICE_MIRRORING_DRIVER)
			&& !_wcsicmp(DisplayDevice.DeviceName, szDevice)) {

			DWORD dwMonNum = 0;
			while (EnumDisplayDevicesW(szDevice, dwMonNum++, &DisplayDevice, 0)) {
				if (DisplayDevice.StateFlags & DISPLAY_DEVICE_ACTIVE) {
					size_t len = wcslen(DisplayDevice.DeviceID);
					wchar_t* szDeviceIDshort = DisplayDevice.DeviceID + len - 43;// fixed at 43 characters

					HKEY hKey0;
					static wchar_t const gk_szRegCcsEnumDisplay[] = L"SYSTEM\\CurrentControlSet\\Enum\\DISPLAY\\";
					LSTATUS ls = RegOpenKeyExW(HKEY_LOCAL_MACHINE, gk_szRegCcsEnumDisplay, 0, KEY_ENUMERATE_SUB_KEYS | KEY_READ, &hKey0);
					if (ls == ERROR_SUCCESS) {
						DWORD i = 0;
						for (;;) {// iterate over the child keys
							DWORD cbName = _countof(DisplayDevice.DeviceKey);
							ls = RegEnumKeyExW(hKey0, i, DisplayDevice.DeviceKey, &cbName, nullptr, nullptr, nullptr, nullptr);
							if (ls == ERROR_NO_MORE_ITEMS) {
								break;
							}

							if (ls == ERROR_SUCCESS) {
								static wchar_t DeviceName[MAX_PATH] = { 0 };
								memcpy(DeviceName, gk_szRegCcsEnumDisplay, sizeof(gk_szRegCcsEnumDisplay) - 2);// chop off null character
								memcpy(DeviceName + _countof(gk_szRegCcsEnumDisplay) - 1, DisplayDevice.DeviceKey, (cbName << 1) + 2);
								wchar_t* pEnd0 = DeviceName + _countof(gk_szRegCcsEnumDisplay) - 1 + cbName;
								HKEY hKey1;
								ls = RegOpenKeyExW(HKEY_LOCAL_MACHINE, DeviceName, 0, KEY_ENUMERATE_SUB_KEYS | KEY_READ, &hKey1);

								if (ls == ERROR_SUCCESS) {
									DWORD j = 0;
									for (;;) {// iterate over the grandchild keys
										cbName = _countof(DisplayDevice.DeviceKey);
										ls = RegEnumKeyExW(hKey1, j, DisplayDevice.DeviceKey, &cbName, nullptr, nullptr, nullptr, nullptr);
										if (ls == ERROR_NO_MORE_ITEMS) {
											break;
										}

										if (ls == ERROR_SUCCESS) {
											*pEnd0 = L'\\';
											memcpy(pEnd0 + 1, DisplayDevice.DeviceKey, (cbName << 1) + 2);
											wchar_t* pEnd1 = pEnd0 + 1 + cbName;

											HKEY hKey2;
											ls = RegOpenKeyExW(HKEY_LOCAL_MACHINE, DeviceName, 0, KEY_ENUMERATE_SUB_KEYS | KEY_READ, &hKey2);
											if (ls == ERROR_SUCCESS) {

												static wchar_t const szTDriverKeyN[] = L"Driver";
												cbName = sizeof(DisplayDevice.DeviceKey);// re-use it here
												ls = RegQueryValueExW(hKey2, szTDriverKeyN, nullptr, nullptr, (LPBYTE)DisplayDevice.DeviceKey, &cbName);
												if (ls == ERROR_SUCCESS) {
													if (!wcscmp(szDeviceIDshort, DisplayDevice.DeviceKey)) {
														static wchar_t const szTDevParKeyN[] = L"\\Device Parameters";
														memcpy(pEnd1, szTDevParKeyN, sizeof(szTDevParKeyN));
														static wchar_t const szkEDIDKeyN[] = L"EDID";
														cbName = sizeof(DisplayDevice.DeviceKey);// 256, perfectly suited to receive a copy of the 128 or 256 bytes of EDID data

														HKEY hKey3;
														ls = RegOpenKeyExW(HKEY_LOCAL_MACHINE, DeviceName, 0, KEY_ENUMERATE_SUB_KEYS | KEY_READ, &hKey3);
														if (ls == ERROR_SUCCESS) {

															ls = RegQueryValueExW(hKey3, szkEDIDKeyN, nullptr, nullptr, (LPBYTE)DisplayDevice.DeviceKey, &cbName);
															if ((ls == ERROR_SUCCESS) && (cbName > 127)) {
																UINT8* EDIDdata = (UINT8*)DisplayDevice.DeviceKey;
																// memo: bytes 25 to 34 contain the default chromaticity coordinates

																// pixel clock in 10 kHz units (0.01�655.35 MHz)
																UINT16 u16PixelClock = *(UINT16*)(EDIDdata + 54);
																if (u16PixelClock) {// if the descriptor for pixel clock is 0, the descriptor block is invalid
																	// horizontal active pixels
																	nMonitorHorRes = (UINT16(EDIDdata[58] & 0xF0) << 4) | EDIDdata[56];
																	// vertical active pixels
																	nMonitorVerRes = (UINT16(EDIDdata[61] & 0xF0) << 4) | EDIDdata[59];

																	// validate and identify extra descriptor blocks
																	// memo: descriptor block identifier 0xFB is used for additional white point data
																	ptrdiff_t k = 12;
																	if (!*(UINT16*)(EDIDdata + 72) && (EDIDdata[75] == 0xFC)) {// descriptor block 2, the first 16 bits must be zero, else the descriptor contains detailed timing data, identifier 0xFC is used for monitor name
																		do {
																			szMonitorName[k] = gk_au16CodePage437ForEDIDLookup[EDIDdata[77 + k]];
																		} while (--k >= 0);
																	}
																	else if (!*(UINT16*)(EDIDdata + 90) && (EDIDdata[93] == 0xFC)) {// descriptor block 3
																		do {
																			szMonitorName[k] = gk_au16CodePage437ForEDIDLookup[EDIDdata[95 + k]];
																		} while (--k >= 0);
																	}
																	else if (!*(UINT16*)(EDIDdata + 108) && (EDIDdata[111] == 0xFC)) {// descriptor block 4
																		do {
																			szMonitorName[k] = gk_au16CodePage437ForEDIDLookup[EDIDdata[113 + k]];
																		} while (--k >= 0);
																	}
																}
																RegCloseKey(hKey3);
																RegCloseKey(hKey2);
																RegCloseKey(hKey1);
																RegCloseKey(hKey0);

																if (wcslen(szMonitorName) && nMonitorHorRes && nMonitorVerRes) {
																	if (MonitorName) {
																		*MonitorName = szMonitorName;
																	}
																	if (MonitorHorRes) {
																		*MonitorHorRes = nMonitorHorRes;
																	}
																	if (MonitorVerRes) {
																		*MonitorVerRes = nMonitorVerRes;
																	}

																	return true;
																}
																return false;
															}
															RegCloseKey(hKey3);
														}
													}
												}
												RegCloseKey(hKey2);
											}
										}
										++j;
									}
									RegCloseKey(hKey1);
								}
							}
							++i;
						}
						RegCloseKey(hKey0);
					}
					break;
				}
			}
			break;
		}
	}

	return false;
}

bool CFileGetStatus(LPCWSTR lpszFileName, CFileStatus& status)
{
	try {
		return !!CFile::GetStatus(lpszFileName, status);
	} catch (CException* e) {
		// MFCBUG: E_INVALIDARG / "Parameter is incorrect" is thrown for certain cds (vs2003)
		// http://groups.google.co.uk/groups?hl=en&lr=&ie=UTF-8&threadm=OZuXYRzWDHA.536%40TK2MSFTNGP10.phx.gbl&rnum=1&prev=/groups%3Fhl%3Den%26lr%3D%26ie%3DISO-8859-1
		DLog(L"CFile::GetStatus() has thrown an exception");
		e->Delete();
		return false;
	}
}
