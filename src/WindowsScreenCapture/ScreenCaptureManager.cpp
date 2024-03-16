#include "ScreenCaptureManager.h"

//! IInspectable
#include <inspectable.h>

ScreenCaptureManager::ScreenCaptureManager()
	: captureCursor(false)
	, contentSize()
	, pixelFormat(winrt::Windows::Graphics::DirectX::DirectXPixelFormat::B8G8R8A8UIntNormalized)
	, d3d11DevicePtr()
	, d3d11RuntimeDevice()
	, graphicsCaptureItem(nullptr)
	, graphicsCaptureSession(nullptr)
	, captureFramePool(nullptr)
	, captureFramePoolArrivedRevoker()
{
}

bool ScreenCaptureManager::Start(HWND hwnd, HMONITOR hmonitor)
{
	try
	{
		// Device
		HRESULT result;
		result = D3D11CreateDevice(
			nullptr,
			D3D_DRIVER_TYPE_HARDWARE,
			nullptr,
			D3D11_CREATE_DEVICE_BGRA_SUPPORT,
			nullptr,
			0,
			D3D11_SDK_VERSION,
			d3d11DevicePtr.put(),
			nullptr,
			d3d11DeviceContextPtr.put());
		winrt::check_hresult(result);
		auto dxgiDevice = d3d11DevicePtr.as<IDXGIDevice>();
		winrt::com_ptr<::IInspectable> d3d11RuntimeDeviceInspectable;
		result = CreateDirect3D11DeviceFromDXGIDevice(dxgiDevice.get(), d3d11RuntimeDeviceInspectable.put());
		winrt::check_hresult(result);
		d3d11RuntimeDevice = d3d11RuntimeDeviceInspectable.as<winrt::Windows::Graphics::DirectX::Direct3D11::IDirect3DDevice>();

		// CaptureItem
		auto factory = winrt::get_activation_factory<winrt::Windows::Graphics::Capture::GraphicsCaptureItem>();
		auto interop = factory.as<IGraphicsCaptureItemInterop>();
		if (hwnd != nullptr)
		{
			result = interop->CreateForWindow(hwnd, winrt::guid_of<ABI::Windows::Graphics::Capture::IGraphicsCaptureItem>(), winrt::put_abi(graphicsCaptureItem));
		}
		else if (hmonitor != nullptr)
		{
			result = interop->CreateForMonitor(hmonitor, winrt::guid_of<ABI::Windows::Graphics::Capture::IGraphicsCaptureItem>(), winrt::put_abi(graphicsCaptureItem));
		}
		winrt::check_hresult(result);

		// Create
		if (!graphicsCaptureItem)
		{
			return false;
		}
		contentSize = graphicsCaptureItem.Size();
		captureFramePool = winrt::Windows::Graphics::Capture::Direct3D11CaptureFramePool::CreateFreeThreaded(
			d3d11RuntimeDevice,
			pixelFormat,
			1,
			contentSize);
		captureFramePoolArrivedRevoker = captureFramePool.FrameArrived(winrt::auto_revoke, { this, &ScreenCaptureManager::OnFrameArrived });

		// Start
		graphicsCaptureSession = captureFramePool.CreateCaptureSession(graphicsCaptureItem);
		graphicsCaptureSession.IsCursorCaptureEnabled(captureCursor);
		graphicsCaptureSession.StartCapture();

		return true;
	}
	catch (const winrt::hresult_error& e)
	{
		const auto code = e.code();
		const auto message = e.message();
		return false;
	}
}

void ScreenCaptureManager::Stop()
{
	captureFramePoolArrivedRevoker.revoke();

	if (graphicsCaptureSession)
	{
		graphicsCaptureSession.Close();
		graphicsCaptureSession = nullptr;
	}
	if (captureFramePool)
	{
		captureFramePool.Close();
		captureFramePool = nullptr;
	}
	if (d3d11RuntimeDevice)
	{
		d3d11RuntimeDevice = nullptr;
	}
	if (graphicsCaptureItem)
	{
		graphicsCaptureItem = nullptr;
	}
}

void ScreenCaptureManager::SetCaptureCursor(bool enable)
{
	if (captureCursor != enable)
	{
		captureCursor = enable;
	}
}

void ScreenCaptureManager::SetCaptureCallback(CallbackFunction callback)
{
	captureCallback = callback;
}

void ScreenCaptureManager::OnFrameArrived(winrt::Windows::Graphics::Capture::Direct3D11CaptureFramePool const& sender, winrt::Windows::Foundation::IInspectable const& args)
{
	try
	{
		auto frameData = sender.TryGetNextFrame();
		auto frameSize = frameData.ContentSize();
		if (contentSize.Width != frameSize.Width || contentSize.Height != frameSize.Height)
		{
			contentSize = frameSize;
			captureFramePool.Recreate(d3d11RuntimeDevice, pixelFormat, 2, frameSize);
			return;
		}

		HRESULT result;
		winrt::com_ptr<ID3D11Texture2D> surface;
		auto frameSurface = frameData.Surface().as<::Windows::Graphics::DirectX::Direct3D11::IDirect3DDxgiInterfaceAccess>();
		result = frameSurface->GetInterface(winrt::guid_of<ID3D11Texture2D>(), surface.put_void());
		winrt::check_hresult(result);

		D3D11_TEXTURE2D_DESC desc;
		surface->GetDesc(&desc);
		if (desc.Width != frameSize.Width || desc.Height != frameSize.Height)
		{
			return;
		}

		if (captureCallback)
		{
			captureCallback(d3d11DeviceContextPtr, frameData, surface, desc);
		}
	}
	catch (const winrt::hresult_error& e)
	{
		const auto code = e.code();
		const auto message = e.message();
	}
}
