#pragma once

#include <Windows/PreWindowsApi.h>
THIRD_PARTY_INCLUDES_START
#include "d3d12sdklayers.h"
#include "dxcapi.h"
#include "d3d12.h"
//Note that the header include order. Include Agility SDK headers before windows sdk headers.
#include "d3dx12/d3dx12.h"
#include <dxgi1_4.h>
#if USE_PIX
#include "pix3.h"
#include <shlobj.h>
#include <strsafe.h>
#endif
THIRD_PARTY_INCLUDES_END
#include <Windows/PostWindowsApi.h>

#define DxCheck(Call)                                           \
	do                                                          \
	{                                                           \
		HRESULT hr = Call;                                      \
		if(FAILED(hr)) {                                        \
			OutputDxError(hr, #Call, __FILE__, __LINE__);       \
		}                                                       \
	} while (0)

#define EnumError(ErrorInfo)						\
	case ErrorInfo : return TEXT(#ErrorInfo);

DECLARE_LOG_CATEGORY_EXTERN(LogDx12, Log, All);
inline DEFINE_LOG_CATEGORY(LogDx12);

namespace FRAMEWORK
{
	inline const TCHAR* GetErrorText(HRESULT hr)
	{
		switch (hr)
		{
			// Win32 Common Error
			EnumError(S_OK)
			EnumError(S_FALSE)
			EnumError(E_UNEXPECTED)
			EnumError(E_NOTIMPL)
			EnumError(E_OUTOFMEMORY)
			EnumError(E_INVALIDARG)
			EnumError(E_NOINTERFACE)
			EnumError(E_POINTER)
			EnumError(E_HANDLE)
			EnumError(E_ABORT)
			EnumError(E_FAIL)
			EnumError(E_ACCESSDENIED)
			EnumError(E_PENDING)

			//DXGI Common Error
			EnumError(DXGI_ERROR_DEVICE_HUNG)
			EnumError(DXGI_ERROR_DEVICE_REMOVED)
			EnumError(DXGI_ERROR_DEVICE_RESET)
			EnumError(DXGI_ERROR_DRIVER_INTERNAL_ERROR)
			EnumError(DXGI_ERROR_FRAME_STATISTICS_DISJOINT)
			EnumError(DXGI_ERROR_GRAPHICS_VIDPN_SOURCE_IN_USE)
			EnumError(DXGI_ERROR_INVALID_CALL)
			EnumError(DXGI_ERROR_MORE_DATA)
			EnumError(DXGI_ERROR_NONEXCLUSIVE)
			EnumError(DXGI_ERROR_NOT_CURRENTLY_AVAILABLE)
			EnumError(DXGI_ERROR_NOT_FOUND)
			EnumError(DXGI_ERROR_REMOTE_CLIENT_DISCONNECTED)
			EnumError(DXGI_ERROR_REMOTE_OUTOFMEMORY)
			EnumError(DXGI_ERROR_RESTRICT_TO_OUTPUT_STALE)
			EnumError(DXGI_ERROR_SDK_COMPONENT_MISSING)
			EnumError(DXGI_ERROR_SESSION_DISCONNECTED)
			EnumError(DXGI_ERROR_UNSUPPORTED)
			EnumError(DXGI_ERROR_WAIT_TIMEOUT)
			EnumError(DXGI_ERROR_WAS_STILL_DRAWING)
			EnumError(DXGI_ERROR_ACCESS_LOST)
			EnumError(DXGI_ERROR_CANNOT_PROTECT_CONTENT)
		}
		return TEXT("Unknown Error");
	}

	void OutputDxError(HRESULT hr, const ANSICHAR* Code, const ANSICHAR* Filename, uint32 Line);

#if USE_PIX
	inline std::wstring GetLatestWinPixGpuCapturerPath()
	{
		LPWSTR programFilesPath = nullptr;
		SHGetKnownFolderPath(FOLDERID_ProgramFiles, KF_FLAG_DEFAULT, NULL, &programFilesPath);

		std::wstring pixSearchPath = programFilesPath + std::wstring(L"\\Microsoft PIX\\*");

		WIN32_FIND_DATA findData;
		bool foundPixInstallation = false;
		wchar_t newestVersionFound[MAX_PATH];

		HANDLE hFind = FindFirstFile(pixSearchPath.c_str(), &findData);
		if (hFind != INVALID_HANDLE_VALUE)
		{
			do
			{
				if (((findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY) &&
					(findData.cFileName[0] != '.'))
				{
					if (!foundPixInstallation || wcscmp(newestVersionFound, findData.cFileName) <= 0)
					{
						foundPixInstallation = true;
						StringCchCopy(newestVersionFound, _countof(newestVersionFound), findData.cFileName);
					}
				}
			} while (FindNextFile(hFind, &findData) != 0);
		}

		FindClose(hFind);

		if (!foundPixInstallation)
		{
			// TODO: Error, no PIX installation found
		}

		wchar_t output[MAX_PATH];
		StringCchCopy(output, pixSearchPath.length(), pixSearchPath.data());
		StringCchCat(output, MAX_PATH, &newestVersionFound[0]);
		StringCchCat(output, MAX_PATH, L"\\WinPixGpuCapturer.dll");

		return &output[0];
	}
#endif
}

