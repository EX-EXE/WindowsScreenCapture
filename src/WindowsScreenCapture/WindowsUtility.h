#pragma once

// STD
#include <string>

// Win
#include <windows.h>
#include <winuser.h>
#include <psapi.h>
#include <dwmapi.h>
#pragma comment (lib, "Dwmapi")

struct SearchHwndInfo
{
public:
	SearchHwndInfo(unsigned long processId, const std::wstring& processName)
		: ProcessId(processId)
		, ProcessName(processName)
		, ResultHwnd(nullptr)
	{
	}

public:
	unsigned long ProcessId;
	const std::wstring& ProcessName;
	HWND ResultHwnd;
};

struct SearchHmonitorInfo
{
public:
	SearchHmonitorInfo(const int index)
		: CurrentIndex(0)
		, SearchIndex(index)
		, Result(nullptr)
	{
	}

public:
	int CurrentIndex;
	int SearchIndex;

	HMONITOR Result;
};

class WindowsUtility
{
public:
	static bool IsValidHwnd(HWND hwnd)
	{
		if (!hwnd)
		{
			return false;
		}
		if (GetShellWindow() == hwnd)
		{
			return false;
		}
		if (IsWindow(hwnd) == 0)
		{
			return false;
		}
		if (!IsWindowVisible(hwnd))
		{
			return false;
		}
		if (GetAncestor(hwnd, GA_ROOT) != hwnd)
		{
			return false;
		}
		const auto style = GetWindowLong(hwnd, GWL_STYLE);
		if ((style & WS_DISABLED) == WS_DISABLED)
		{
			return false;
		}

		// UWP
		auto cloaked = FALSE;
		auto result = DwmGetWindowAttribute(hwnd, DWMWA_CLOAKED, &cloaked, sizeof(cloaked));
		if (result == S_OK && cloaked == DWM_CLOAKED_SHELL)
		{
			return false;
		}

		return true;
	}

	static bool IsValidHmonitor(HMONITOR hMonitor)
	{
		if (!hMonitor)
		{
			return false;
		}
		MONITORINFO info;
		info.cbSize = sizeof(MONITORINFO);
		return GetMonitorInfo(hMonitor, &info) != 0;
	}


	static HWND SearchHWND(const unsigned long processId, const std::wstring& processName)
	{
		auto info = SearchHwndInfo(processId, processName);

		EnumWindows(
			[](HWND hwnd, LPARAM lParam)
			{
				if (!IsValidHwnd(hwnd))
				{
					return TRUE;
				}

				auto info = reinterpret_cast<SearchHwndInfo*>(lParam);

				DWORD windowProcessId;
				GetWindowThreadProcessId(hwnd, &windowProcessId);

				// ProcessId
				if (windowProcessId == info->ProcessId)
				{
					info->ResultHwnd = hwnd;
					return FALSE;
				}

				// ProcessName
				if (!info->ProcessName.empty())
				{
					wchar_t windowProcessName[MAX_PATH] = { 0 };
					int windowProcessNameBufferSize = sizeof(windowProcessName) / sizeof(wchar_t);
					auto windowProcessHandle = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, windowProcessId);
					if (windowProcessHandle)
					{
						// Window Name
						GetWindowText(hwnd, windowProcessName, windowProcessNameBufferSize);
						auto windowTitleStr = std::wstring(windowProcessName);
						auto findwindowTitleItr = windowTitleStr.rfind(info->ProcessName);
						if (findwindowTitleItr != std::wstring::npos)
						{
							info->ResultHwnd = hwnd;
							return FALSE;
						}

						// Process Image Name
						auto windowProcessNameLength = GetProcessImageFileName(windowProcessHandle, windowProcessName, windowProcessNameBufferSize);
						if (0 < windowProcessNameLength)
						{
							auto windowImageNameStr = std::wstring(windowProcessName);
							auto findName = windowImageNameStr.rfind(info->ProcessName);
							if (findName != std::wstring::npos)
							{
								info->ResultHwnd = hwnd;
								return FALSE;
							}
						}
						CloseHandle(windowProcessHandle);
					}
				}
				return TRUE;
			},
			reinterpret_cast<LPARAM>(&info)
		);
		return info.ResultHwnd;
	}

	static HWND GetDesktopHwnd()
	{
		return GetDesktopWindow();
	}

	static HMONITOR GetMonitorHwnd(const int searchIndex)
	{
		auto result = (HMONITOR)nullptr;
		auto info = SearchHmonitorInfo(searchIndex);
		EnumDisplayMonitors(
			NULL,
			NULL,
			[](HMONITOR hMonitor, HDC hdc, LPRECT lpRect, LPARAM lParam)
			{
				auto info = reinterpret_cast<SearchHmonitorInfo*>(lParam);
				if (info->CurrentIndex == info->SearchIndex)
				{
					info->Result = hMonitor;
					return FALSE;
				}

				++info->CurrentIndex;
				return TRUE;
			},
			reinterpret_cast<LPARAM>(&info)
		);
		return info.Result;
	}
};