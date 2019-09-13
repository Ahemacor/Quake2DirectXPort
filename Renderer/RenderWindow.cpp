#include "RenderWindow.h"
#include "CppWrapper.h"
#include "r_local.h"
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

ID3D11Device* RenderWindow::GetDevice()
{
    return device.Get();
}

void RenderWindow::CreateBuffer(const D3D11_BUFFER_DESC* pDesc, const void* pSrcMem, ID3D11Buffer** outBufferAddr)
{
    D3D11_SUBRESOURCE_DATA srd = { pSrcMem, 0, 0 };
    device->CreateBuffer(pDesc, (pSrcMem == nullptr ? nullptr : &srd), outBufferAddr);
    buffers.emplace_back(*outBufferAddr);
}

HRESULT RenderWindow::CreateTexture2D(const D3D11_TEXTURE2D_DESC* pDesc, const D3D11_SUBRESOURCE_DATA* pInitialData, ID3D11Texture2D** ppTexture2D)
{
    return device->CreateTexture2D(pDesc, pInitialData, ppTexture2D);
}

HRESULT RenderWindow::CreateShaderResourceView(ID3D11Resource* pResource, const D3D11_SHADER_RESOURCE_VIEW_DESC* pDesc, ID3D11ShaderResourceView** ppSRView)
{
    return device->CreateShaderResourceView(pResource, pDesc, ppSRView);
}

ID3D11DeviceContext* RenderWindow::GetDeviceContext()
{
    return deviceContext.Get();
}

IDXGISwapChain* RenderWindow::GetSwapchain()
{
    return swapchain.Get();
}

ID3D11RenderTargetView* RenderWindow::GetRTV()
{
    return renderTargetView.Get();
}

ID3D11RenderTargetView** RenderWindow::GetRTVAddr()
{
    return renderTargetView.GetAddressOf();
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

HWND RenderWindow::GetWindowHandle()
{
    return handle;
}

bool RenderWindow::InitDirectX()
{
    DXGI_SWAP_CHAIN_DESC sd;
    IDXGIFactory* pFactory = NULL;

    // we don't support any pre-d3d11 feature levels
    D3D_FEATURE_LEVEL FeatureLevels[] = { D3D_FEATURE_LEVEL_11_0 };

    memset(&sd, 0, sizeof(sd));

    // set up the mode properly
    DXGI_MODE_DESC desc = GetMode(currentMode);
    memcpy(&sd.BufferDesc, &desc, sizeof(DXGI_MODE_DESC));
    if (!fullscreen)
    {
        sd.BufferDesc.Width = width;
        sd.BufferDesc.Height = height;
    }

    sd.BufferCount = 1;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = handle;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

    // always initially create a windowed swapchain at the desired resolution, then switch to fullscreen post-creation if it was requested
    //sd.Windowed = fullscreen;
    sd.Windowed = true;

    // now we try to create it
    if (FAILED(D3D11CreateDeviceAndSwapChain(
        NULL,
        D3D_DRIVER_TYPE_HARDWARE,
        NULL,
        0, //D3D11_CREATE_DEVICE_DEBUG,
        FeatureLevels,
        sizeof(FeatureLevels) / sizeof(FeatureLevels[0]),
        D3D11_SDK_VERSION,
        &sd,
        swapchain.GetAddressOf(),
        device.GetAddressOf(),
        NULL,
        deviceContext.GetAddressOf())))
    {
        //ri.Sys_Error(ERR_FATAL, "D3D11CreateDeviceAndSwapChain failed");
        return false;
    }

    // now we disable stuff that we want to handle ourselves
    /*if (SUCCEEDED(swapchain->GetParent(IID_IDXGIFactory, (void**)& pFactory)))
    {
        pFactory->MakeWindowAssociation(handle, DXGI_MWA_NO_WINDOW_CHANGES | DXGI_MWA_NO_ALT_ENTER);
        pFactory->Release();
    }*/

    // now we switch to fullscreen after the device, context and swapchain are created, but before we create the rendertargets, so that we don't need to respond to WM_SIZE
    swapchain->SetFullscreenState(fullscreen, NULL);
    //d3d_SwapChain->lpVtbl->SetFullscreenState (d3d_SwapChain, TRUE, NULL);

    // create the initial frame buffer that we're going to use for all of our rendering
    ID3D11Texture2D* pBackBuffer = NULL;
    D3D11_TEXTURE2D_DESC descRT;
    ID3D11Texture2D* pDepthStencil = NULL;
    D3D11_TEXTURE2D_DESC descDepth;

    // Get a pointer to the back buffer
    if (FAILED(swapchain->GetBuffer(0, IID_ID3D11Texture2D, (LPVOID*)& pBackBuffer)))
    {
        //ri.Sys_Error(ERR_FATAL, "d3d_SwapChain->GetBuffer failed");
        return false;
    }

    // get the description of the backbuffer for creating the depth buffer from it
    pBackBuffer->GetDesc(&descRT);

    // Create a render-target view
    device->CreateRenderTargetView((ID3D11Resource*)pBackBuffer, NULL, renderTargetView.GetAddressOf());
    pBackBuffer->Release();

    // create the depth buffer with the same dimensions and multisample levels as the backbuffer
    descDepth.Width = descRT.Width;
    descDepth.Height = descRT.Height;
    descDepth.SampleDesc.Count = descRT.SampleDesc.Count;
    descDepth.SampleDesc.Quality = descRT.SampleDesc.Quality;

    // fill in the rest of it
    descDepth.MipLevels = 1;
    descDepth.ArraySize = 1;
    descDepth.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    descDepth.Usage = D3D11_USAGE_DEFAULT;
    descDepth.BindFlags = D3D11_BIND_DEPTH_STENCIL;
    descDepth.CPUAccessFlags = 0;
    descDepth.MiscFlags = 0;

    // and create it
    if (SUCCEEDED(CreateTexture2D(&descDepth, NULL, &pDepthStencil)))
    {
        // Create the depth stencil view
        device->CreateDepthStencilView((ID3D11Resource*)pDepthStencil, NULL, depthStencilView.GetAddressOf());
        pDepthStencil->Release();
    }

    // clear to black
    float clearColor[] = { 0, 0, 0, 0 };
    deviceContext->ClearRenderTargetView(renderTargetView.Get(), clearColor);
    // and run a present to make them show
    deviceContext->OMSetRenderTargets(1, renderTargetView.GetAddressOf(), depthStencilView.Get());
    swapchain->Present(0, 0);


    // load all of our initial objects
    // each subsystem creates it's objects then registers it's handlers, following which the reset handler runs to complete object creation
    SLInitShaders(); // must be done first before any other shaders are created
    R_InitMain();
    R_InitSurfaces();
    R_InitParticles();
    R_InitSprites();
    R_InitLight();
    R_InitWarp();
    R_InitSky();
    R_InitMesh();
    R_InitBeam();
    R_InitNull();

    // success
    return true;
}

bool RenderWindow::InitWindow(int width_, int height_, int mode, bool fullscreen)
{
    width = width_;
    height = height_;
    currentMode = mode;
    
    // Register the frame class 
    WNDCLASS		wc;
    wc.style = 0;
    wc.lpfnWndProc = wndproc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = hInstance;
    wc.hIcon = 0;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)COLOR_GRAYTEXT;
    wc.lpszMenuName = 0;
    wc.lpszClassName = window_class.c_str();

    if (!RegisterClass(&wc)) exit(-1);
        //ri.Sys_Error(ERR_FATAL, "Couldn't register window class");

    int exstyle = 0;
    int stylebits = 0;
    RECT rect;
    rect.left = 0;
    rect.top = 0;
    if (fullscreen)
    {
        exstyle = WS_EX_TOPMOST;
        stylebits = WS_POPUP | WS_VISIBLE;
        rect.right = GetMode(currentMode).Width;
        rect.bottom = GetMode(currentMode).Height;
    }
    else
    {
        exstyle = 0;
        stylebits = WS_OVERLAPPED | WS_BORDER | WS_CAPTION | WS_VISIBLE;
        rect.right = width;
        rect.bottom = height;
    }

    AdjustWindowRect(&rect, stylebits, FALSE);

    handle = CreateWindowEx(
        exstyle,
        window_class.c_str(),
        window_title.c_str(),
        stylebits,
        rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top,//x, y, w, h,
        NULL,
        NULL,
        hInstance,
        NULL);

    if (handle == nullptr) exit(-1);
        //ri.Sys_Error(ERR_FATAL, "Couldn't create window");

    ShowWindow(handle, SW_SHOW);
    UpdateWindow(handle);

    // init all the gl stuff for the window
    if (!InitDirectX())
    {
        //ri.Con_Printf(PRINT_ALL, "VID_CreateWindow() - GLimp_InitGL failed\n");
        return false;
    }

    SetForegroundWindow(handle);
    SetFocus(handle);

    // let the sound and input subsystems know about the new window
    //ri.Vid_NewWindow();
    return true;
}

void RenderWindow::CloseWindow()
{
    //if (d3d_Device && d3d_SwapChain && d3d_Context)
    if (GetDevice() && GetSwapchain() && GetDeviceContext())
    {
        if (fullscreen)
        {
            // DXGI prohibits destruction of a swapchain when in a fullscreen mode so go back to windowed first
            //d3d_SwapChain->lpVtbl->SetFullscreenState (d3d_SwapChain, FALSE, NULL);
            swapchain->SetFullscreenState(FALSE, NULL);
            fullscreen = false;
        }

        // clear to black
        float clearColor[] = { 0, 0, 0, 0 };
        deviceContext->ClearRenderTargetView(renderTargetView.Get(), clearColor);
        // and run a present to make them show
        deviceContext->OMSetRenderTargets(1, renderTargetView.GetAddressOf(), depthStencilView.Get());
        swapchain->Present(0, 0);

        // finally chuck out all cached state in the context
        //d3d_Context->lpVtbl->OMSetRenderTargets (d3d_Context, 0, NULL, NULL);
        deviceContext->OMSetRenderTargets(0, NULL, NULL);
        //d3d_Context->lpVtbl->ClearState (d3d_Context);
        deviceContext->ClearState();
        //d3d_Context->lpVtbl->Flush (d3d_Context);
        deviceContext->Flush();
    }

    // handle special objects
    SLShutdownShaders();
    R_ShutdownSurfaces();
    R_ShutdownLight();
    R_ShutdownWarp();
    R_ShutdownSky();
    R_ShutdownMesh();
    R_ShutdownBeam();
    R_ShutdownSprites();

    device.Reset();
    deviceContext.Reset();
    swapchain.Reset();
    renderTargetView.Reset();
    depthStencilView.Reset();
    d3d_VideoModes.clear();

    if (handle)
    {
        ShowWindow(handle, SW_HIDE);
        DestroyWindow(handle);
        handle = NULL;
    }

    UnregisterClass(window_class.c_str(), hInstance);

    buffers.clear();

}