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

#pragma once

#include <VirtDisk.h>

class CDiskImage
{
private:
	enum DriveType {NONE, WIN8, DTLITE, VCD};

	DriveType	m_DriveType;
	WCHAR		m_DriveLetter;

	// Windows 8 VirtualDisk
	HMODULE		m_hVirtualDiskModule;
	HRESULT		(__stdcall * m_OpenVirtualDiskFunc)(
				__in     PVIRTUAL_STORAGE_TYPE         VirtualStorageType,
				__in     PCWSTR                        Path,
				__in     VIRTUAL_DISK_ACCESS_MASK      VirtualDiskAccessMask,
				__in     OPEN_VIRTUAL_DISK_FLAG        Flags,
				__in_opt POPEN_VIRTUAL_DISK_PARAMETERS Parameters,
				__out    PHANDLE                       Handle);
	HRESULT		(__stdcall * m_AttachVirtualDiskFunc)(
				__in     HANDLE                          VirtualDiskHandle,
				__in_opt PSECURITY_DESCRIPTOR            SecurityDescriptor,
				__in     ATTACH_VIRTUAL_DISK_FLAG        Flags,
				__in     ULONG                           ProviderSpecificFlags,
				__in_opt PATTACH_VIRTUAL_DISK_PARAMETERS Parameters,
				__in_opt LPOVERLAPPED                    Overlapped);
	HRESULT		(__stdcall * m_GetVirtualDiskPhysicalPathFunc)(
				__in                              HANDLE VirtualDiskHandle,
				__inout                           PULONG DiskPathSizeInBytes,
				__out_bcount(DiskPathSizeInBytes) PWSTR  DiskPath);
	HANDLE		m_VHDHandle;

	// DAEMON Tools Lite
	enum dtdrive {dt_none, dt_dt, dt_scsi};
	dtdrive		m_dtdrive;
	CString		m_dtlite_path;

	// Virtual CloneDrive
	CString		m_vcd_path;

public:
	CDiskImage();
	~CDiskImage();

	void Init();
	bool DriveAvailable();
	const LPCWSTR GetExts();
	bool CheckExtension(LPCWSTR pathName);

	WCHAR MountDiskImage(LPCWSTR pathName);
	void UnmountDiskImage();

	WCHAR GetDriveLetter() { return m_DriveLetter; };

private:
	// Windows 8 VirtualDisk
	WCHAR MountWin8(LPCWSTR pathName);

	// DAEMON Tools Lite
	WCHAR MountDTLite(LPCWSTR pathName);

	// Virtual CloneDrive
	WCHAR MountVCD(LPCWSTR pathName);
};
