#pragma once

#include <functional>

//! winrt::com_ptr
#include <winrt/base.h>

//! ID3D11Device
//! D3D11CreateDevice
#include <d3d11.h>

//! CreateDirect3D11DeviceFromDXGIDevice
#include <Windows.Graphics.DirectX.Direct3D11.Interop.h>

#include <winrt/Windows.Foundation.h>

//! IGraphicsCaptureItemInterop
#include <Windows.Graphics.Capture.Interop.h>

//! winrt::Windows::Graphics::DirectX::Direct3D11::IDirect3DDevice
#include <winrt/Windows.Graphics.DirectX.Direct3D11.h>

//! winrt::Windows::Graphics::Capture::GraphicsCaptureItem
#include <winrt/Windows.Graphics.Capture.h>


class ScreenCaptureManager
{
	using CallbackFunction = std::function<void(
		const winrt::com_ptr<ID3D11DeviceContext>&,
		const winrt::Windows::Graphics::Capture::Direct3D11CaptureFrame&,
		const winrt::com_ptr<ID3D11Texture2D>&,
		const D3D11_TEXTURE2D_DESC&)>;
public:
	//! Constructor
	ScreenCaptureManager();

	//! Start
	bool Start(HWND hwnd,HMONITOR hmonitor);

	//! Stop
	void Stop();

	//! CaptureCursor Setting
	void SetCaptureCursor(bool enable);

	//! CaptureCursor Setting
	void SetCaptureCallback(CallbackFunction callback);

private:
	void OnFrameArrived(winrt::Windows::Graphics::Capture::Direct3D11CaptureFramePool const& sender, winrt::Windows::Foundation::IInspectable const& args);

private:
	bool captureCursor;

	CallbackFunction captureCallback;

	winrt::Windows::Graphics::SizeInt32 contentSize;
	winrt::Windows::Graphics::DirectX::DirectXPixelFormat pixelFormat;

	winrt::com_ptr<ID3D11Device> d3d11DevicePtr;
	winrt::com_ptr<ID3D11DeviceContext> d3d11DeviceContextPtr;
	winrt::Windows::Graphics::DirectX::Direct3D11::IDirect3DDevice d3d11RuntimeDevice;
	winrt::Windows::Graphics::Capture::GraphicsCaptureItem graphicsCaptureItem;

	winrt::Windows::Graphics::Capture::GraphicsCaptureSession graphicsCaptureSession;
	winrt::Windows::Graphics::Capture::Direct3D11CaptureFramePool captureFramePool;
	winrt::Windows::Graphics::Capture::Direct3D11CaptureFramePool::FrameArrived_revoker captureFramePoolArrivedRevoker;
};