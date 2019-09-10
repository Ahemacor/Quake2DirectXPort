#include "RenderWindow.h"
#include <Windows.h>

#include <cassert>

#pragma comment (lib, "d3d11.lib")
#pragma comment (lib, "dxgi.lib")

RenderWindow::RenderWindow()
{
    LoadModes();
}

RenderWindow::~RenderWindow()
{

}

UINT RenderWindow::GetModesNumber()
{
    assert(d3d_VideoModes.size() > 0);
    return d3d_VideoModes.size();
}

DXGI_MODE_DESC RenderWindow::GetMode(UINT index)
{
    assert(index >=0 && index < d3d_VideoModes.size());
    return d3d_VideoModes[index];
}

void RenderWindow::SetDevice(ID3D11Device* pDevice)
{
    device = pDevice;
}

ID3D11Device* RenderWindow::GetDevice()
{
    return device.Get();
}

void RenderWindow::SetDeviceContext(ID3D11DeviceContext* pDeviceContext)
{
    deviceContext = pDeviceContext;
}

ID3D11DeviceContext* RenderWindow::GetDeviceContext()
{
    return deviceContext.Get();
}

void RenderWindow::SetSwapchain(IDXGISwapChain* pSwapChain)
{
    swapchain = pSwapChain;
}

IDXGISwapChain* RenderWindow::GetSwapchain()
{
    return swapchain.Get();
}

void RenderWindow::SetRTV(ID3D11RenderTargetView* pRTV)
{
    renderTargetView = pRTV;
}

ID3D11RenderTargetView* RenderWindow::GetRTV()
{
    return renderTargetView.Get();
}

ID3D11RenderTargetView** RenderWindow::GetRTVAddr()
{
    return renderTargetView.GetAddressOf();
}

void RenderWindow::SetDSV(ID3D11DepthStencilView* pDSV)
{
    depthStencilView = pDSV;
}

ID3D11DepthStencilView* RenderWindow::GetDSV()
{
    return depthStencilView.Get();
}


IDXGIAdapter* RenderWindow::GetFirstAdapter()
{
    Microsoft::WRL::ComPtr<IDXGIFactory> pFactory;

    // Create a DXGIFactory object.
    HRESULT hr = CreateDXGIFactory(__uuidof(IDXGIFactory), reinterpret_cast<void**>(pFactory.GetAddressOf()));
    if (FAILED(hr))
    {
        exit(-1);
    }

    IDXGIAdapter* pAdapter;
    UINT index = 0;
    hr = pFactory->EnumAdapters(index, &pAdapter);
    if (FAILED(hr))
    {
        exit(-1);
    }

    return pAdapter;
}


IDXGIOutput* RenderWindow::GetAdapterOutput(IDXGIAdapter* pAdapter)
{
    IDXGIOutput* d3d_Output = NULL;

    DXGI_OUTPUT_DESC Desc;
    IDXGIOutput* pOutput = nullptr;
    for (UINT i = 0; pAdapter->EnumOutputs(i, &pOutput) != DXGI_ERROR_NOT_FOUND; ++i)
    {
        if (FAILED(pOutput->GetDesc(&Desc))) continue;

        if (!Desc.AttachedToDesktop) continue;

        if (Desc.Rotation == DXGI_MODE_ROTATION_ROTATE90 || Desc.Rotation == DXGI_MODE_ROTATION_ROTATE180 || Desc.Rotation == DXGI_MODE_ROTATION_ROTATE270) continue;

        // this is a valid output now
        d3d_Output = pOutput;
        break;
    }

    return d3d_Output;
}

std::vector<DXGI_MODE_DESC> RenderWindow::GetOutputVideoModes(IDXGIOutput* output, DXGI_FORMAT fmt)
{
    std::vector<DXGI_MODE_DESC> videoModes;
    const UINT EnumFlags = (DXGI_ENUM_MODES_SCALING | DXGI_ENUM_MODES_INTERLACED);
    UINT NumModes = 0;
    // get modes on this adapter (note this is called twice per design) - note that the first time ModeList must be NULL or it will return 0 modes
    if (SUCCEEDED(output->GetDisplayModeList(fmt, EnumFlags, &NumModes, NULL)))
    {
        videoModes.resize(NumModes);
        DXGI_MODE_DESC* vectorData = const_cast<DXGI_MODE_DESC*>(videoModes.data());
        if (FAILED(output->GetDisplayModeList(fmt, EnumFlags, &NumModes, vectorData)))
        {
            videoModes.clear();
        }
    }
    
    return videoModes;
}

void RenderWindow::LoadModes()
{
    d3d_VideoModes.clear();

    int i, j, biggest;
    IDXGIAdapter* d3d_Adapter = GetFirstAdapter();
    IDXGIOutput* d3d_Output = GetAdapterOutput(d3d_Adapter);

    std::vector<DXGI_MODE_DESC> allModes = GetOutputVideoModes(d3d_Output, DXGI_FORMAT_R8G8B8A8_UNORM);


    // cleanup the modes by removing unspecified modes where a specified option or options exists, then copy them out to the final array
    for (i = 0; i < allModes.size(); ++i)
    {
        DXGI_MODE_DESC* mode = &allModes[i];
        BOOL deadmode = FALSE;

        // don't allow < 640x480 modes
        if (mode->Width < 640) continue;
        if (mode->Height < 480) continue;

        if (mode->Scaling == DXGI_MODE_SCALING_UNSPECIFIED)
        {
            for (j = 0; j < allModes.size(); j++)
            {
                DXGI_MODE_DESC* mode2 = &allModes[i];

                if (mode->Format != mode2->Format) continue;
                if (mode->Height != mode2->Height) continue;
                if (mode->RefreshRate.Denominator != mode2->RefreshRate.Denominator) continue;
                if (mode->RefreshRate.Numerator != mode2->RefreshRate.Numerator) continue;
                if (mode->ScanlineOrdering != mode2->ScanlineOrdering) continue;
                if (mode->Width != mode2->Width) continue;

                if (mode2->Scaling != DXGI_MODE_SCALING_UNSPECIFIED)
                {
                    deadmode = TRUE;
                    break;
                }
            }
        }

        if (mode->ScanlineOrdering == DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED)
        {
            for (j = 0; j < allModes.size(); j++)
            {
                DXGI_MODE_DESC* mode2 = &allModes[i];

                if (mode->Format != mode2->Format) continue;
                if (mode->Height != mode2->Height) continue;
                if (mode->RefreshRate.Denominator != mode2->RefreshRate.Denominator) continue;
                if (mode->RefreshRate.Numerator != mode2->RefreshRate.Numerator) continue;
                if (mode->Scaling != mode2->Scaling) continue;
                if (mode->Width != mode2->Width) continue;

                if (mode2->ScanlineOrdering != DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED)
                {
                    deadmode = TRUE;
                    break;
                }
            }
        }

        if (deadmode) continue;

        // this is a real mode now - copy it out
        d3d_VideoModes.push_back(*mode);
    }
}
