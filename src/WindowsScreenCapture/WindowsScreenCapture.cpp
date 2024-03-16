
// STD
#include <iostream>
#include <unordered_map>
#include <algorithm>

// DirectX
#include <inspectable.h>

#include <wincodec.h>
#include <ScreenGrab.h>

#include "WindowsUtility.h"
#include "ScreenCaptureManager.h"

std::unordered_map<std::wstring, std::wstring> ParseArgs(int argc, wchar_t* argv[])
{
	std::unordered_map<std::wstring, std::wstring> argMap;
	auto currentKey = std::wstring();
	auto currentValue = std::wstring();
	auto appendAndClearMap = [&]()
		{
			if (!currentKey.empty())
			{
				std::transform(
					currentKey.begin(),
					currentKey.end(),
					currentKey.begin(), ::tolower);
				argMap[currentKey] = currentValue;
			}
			currentKey.clear();
			currentValue.clear();
		};
	for (int i = 1; i < argc; ++i)
	{
		const auto argStr = std::wstring(argv[i]);
		if (0 < argStr.length() && argStr[0] == '/')
		{
			auto splitIndex = argStr.find_first_of(':', 1);
			if (splitIndex != std::wstring::npos)
			{
				appendAndClearMap();
				currentKey = argStr.substr(1, splitIndex - 1);
				if (splitIndex + 1 < argStr.length())
				{
					currentValue = argStr.substr(splitIndex + 1);
				}
			}
			continue;
		}
		if (!currentKey.empty())
		{
			if (!currentValue.empty())
			{
				currentValue += ' ';
			}
			currentValue += argStr;
		}
	}
	appendAndClearMap();
	return argMap;
}

int wmain(int argc, wchar_t* argv[])
{
	winrt::init_apartment();

	// Parse
	auto argMap = ParseArgs(argc, argv);

	// Args
	// Licence
	if (auto outputItr = argMap.find(L"licence"); outputItr != argMap.end())
	{
		// DirectXTK
		std::cout << "[Licence]" << std::endl;
		std::cout << "EDirectXTK" << std::endl;
		std::cout << R"(
    MIT License

    Copyright (c) Microsoft Corporation.

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in all
    copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
    SOFTWARE
)" << std::endl;
	}

	// Output
	auto outputFile = std::wstring();
	if (auto outputItr = argMap.find(L"output"); outputItr != argMap.end())
	{
		outputFile = outputItr->second;
	}
	else
	{
		std::cerr << "No arguments specified.(/Output:)" << std::endl;
		std::cerr << "ex. /Output: c://output.png" << std::endl;
		return 1;
	}

	// CaptureCursor
	auto captureCursor = false;
	if (auto captureCursorItr = argMap.find(L"capturecursor"); captureCursorItr != argMap.end() &&
		(captureCursorItr->second == L"1" || captureCursorItr->second == L"Enable" || captureCursorItr->second == L"True"))
	{
		captureCursor = true;
	}

	auto targetHwnd = (HWND)nullptr;
	auto targetHmonitor = (HMONITOR)nullptr;
	// HMONITOR
	if (auto monitorItr = argMap.find(L"monitor"); monitorItr != argMap.end())
	{
		// Monitor 
		auto monitorIndex = std::stoi(monitorItr->second);
		targetHmonitor = WindowsUtility::GetMonitorHwnd(monitorIndex);
	}

	// HWND
	if (!targetHmonitor)
	{
		// ProcessId
		auto processIdNum = (unsigned long)0;
		if (auto processIdItr = argMap.find(L"processid"); processIdItr != argMap.end())
		{
			processIdNum = std::stoi(processIdItr->second);
		}
		// ProcessName
		auto processName = std::wstring();
		if (auto processNameItr = argMap.find(L"processname"); processNameItr != argMap.end())
		{
			processName = processNameItr->second;
		}
		if (processIdNum == 0 && processName.empty())
		{
			return 1;
		}
		targetHwnd = WindowsUtility::SearchHWND(processIdNum, processName);
	}
	if (!targetHwnd && !targetHmonitor)
	{
		std::cerr << "No arguments specified.(/ProcessId: or /ProcessName:)" << std::endl;
		std::cerr << "ex. /ProcessId: 10000" << std::endl;
		std::cerr << "ex. /ProcessName: Taskmgr.exe" << std::endl;
		return 1;
	}

	// Timeout
	auto timeoutSec = (float)5.0;
	if (auto timeoutItr = argMap.find(L"timeoutsec"); timeoutItr != argMap.end())
	{
		// Monitor 
		timeoutSec = std::stof(timeoutItr->second);
	}

	// Capture
	auto exitFlag = std::atomic_bool(false);
	auto manager = ScreenCaptureManager();
	manager.SetCaptureCursor(captureCursor);
	manager.SetCaptureCallback([&](
		const winrt::com_ptr<ID3D11DeviceContext>& d3d11DeviceContextPtr,
		const winrt::Windows::Graphics::Capture::Direct3D11CaptureFrame& frame,
		const winrt::com_ptr<ID3D11Texture2D>& texture2DPtr,
		const D3D11_TEXTURE2D_DESC& desc)
		{
			if (exitFlag.load(std::memory_order_acquire))
			{
				return;
			}
			auto result = DirectX::SaveWICTextureToFile(
				d3d11DeviceContextPtr.get(),
				texture2DPtr.get(),
				GUID_ContainerFormatPng,
				outputFile.c_str(),
				&GUID_WICPixelFormat32bppBGRA);
			winrt::check_hresult(result);
			exitFlag.store(true, std::memory_order_release);
		});

	// Start
	const auto startTime = std::chrono::steady_clock::now();
	if (!manager.Start(targetHwnd, targetHmonitor))
	{
		std::cerr << "Failed start capture." << std::endl;
		return 1;
	}

	// Wait Capture
	while (WindowsUtility::IsValidHwnd(targetHwnd) ||
		WindowsUtility::IsValidHmonitor(targetHmonitor))
	{
		// Exit
		if (exitFlag.load(std::memory_order_acquire))
		{
			break;
		}
		// Timeout
		const auto currentTime = std::chrono::steady_clock::now();
		const auto deltaMilli = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - startTime).count();
		if (timeoutSec * 1000 < deltaMilli)
		{
			break;
		}

		// Sleep
		Sleep(50);
	}

	// Stop
	manager.Stop();

	// Invalid
	if (targetHwnd && !WindowsUtility::IsValidHwnd(targetHwnd))
	{
		std::cerr << "Could not get window." << std::endl;
		return 1;
	}
	if (targetHmonitor && !WindowsUtility::IsValidHmonitor(targetHmonitor))
	{
		std::cerr << "Could not get monitor." << std::endl;
		return 1;
	}
	// Timeout
	if (!exitFlag.load(std::memory_order_acquire))
	{
		std::cerr << "Capture timed out." << std::endl;
		return 1;
	}
	// Success
	std::wcout << outputFile.c_str() << std::endl;
	return 0;
}
