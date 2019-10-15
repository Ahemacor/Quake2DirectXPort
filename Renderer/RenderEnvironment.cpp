#include "RenderEnvironment.h"
#include "Utils.h"
#include <dxgi1_6.h>
#include <cassert>

#pragma comment (lib, "d3dcompiler.lib")
#pragma comment (lib, "dxguid.lib")
#pragma comment (lib, "d3d12.lib")
#pragma comment (lib, "dxgi.lib")

RenderEnvironment::RenderEnvironment()
{
#if defined(DX12_DEBUG_LAYER)
    Microsoft::WRL::ComPtr<ID3D12Debug> debugInterface;
    ENSURE_RESULT(D3D12GetDebugInterface(IID_PPV_ARGS(&debugInterface)));
    debugInterface->EnableDebugLayer();
#endif
    InitializeFactory();
    InitializeAdapters();
    InitializeVideoModes();
    InitializeDevice();
}

RenderEnvironment::~RenderEnvironment()
{
    Release();
}

bool RenderEnvironment::Initialize(HWND handle, std::size_t width, std::size_t height)
{
    commandQueue = CreateCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT);
    parentWindowHandle = handle;
    windowWidth = width;
    windowHeight = height;

    InitializeSwapChain();
    rtvDescriptorHeap = CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_RTV, bufferCount);
    dsvDescriptorHeap = CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 1);
    InitializeAllocatorsAndCommandList();
    InitializeRenderTargetViews();
    InitializeDepthStencilView();
    InitializeSynchronizationObjects();
    InitializeViewportAndScissor();

    isInitialized = true;
    return isInitialized;
}

void RenderEnvironment::Release()
{
    if (isInitialized)
    {
        Synchronize();

        commandQueue.Reset();
        swapChain.Reset();
        rtvDescriptorHeap.Reset();
        dsvDescriptorHeap.Reset();
        depthStencil.Reset();
        for (auto rt : renderTargets) rt.Reset();
        for (auto ca : commandAllocators) ca.Reset();
        renderCommandList.Reset();
        renderCommandListState = CommandListState::CL_RESETED;
        CloseHandle(fenceEvent);

        isInitialized = false;
    }
}

void RenderEnvironment::InitializeFactory()
{
    factory.Reset();
    UINT createFactoryFlags = 0;
#if defined(DX12_DEBUG_LAYER)
    createFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;
#endif
    ENSURE_RESULT(CreateDXGIFactory2(createFactoryFlags, IID_PPV_ARGS(&factory)));
}

void RenderEnvironment::InitializeAdapters()
{
    assert(factory != nullptr);

    adapters.clear();

    auto checkAdapter = [](AdapterData& adapterData) mutable -> bool
    {
        HRESULT hr = D3D12CreateDevice(adapterData.adapter.Get(), D3D_FEATURE_LEVEL_12_0, __uuidof(ID3D12Device), nullptr);
        return SUCCEEDED(hr);
    };

    IDXGIAdapter* pAdapter;
    for (UINT index = 0; SUCCEEDED(factory->EnumAdapters(index, &pAdapter)); ++index )
    {
        AdapterData adapterData;
        adapterData.adapter = pAdapter;
        ENSURE_RESULT(pAdapter->GetDesc(&adapterData.description));
        if (checkAdapter(adapterData))
        {
            adapters.push_back(adapterData);
        }
    }
}

std::size_t RenderEnvironment::GetNumberOfAdapters()
{
    return adapters.size();
}

RenderEnvironment::AdapterData RenderEnvironment::GetAdapter(std::size_t index)
{
    assert(index < adapters.size());
    return adapters[index];
}

Microsoft::WRL::ComPtr<IDXGIAdapter> RenderEnvironment::GetMaxMemoryAdapter()
{
    AdapterData maxMemoryAdapterData = GetAdapter(0);
    for (auto& adapterData : adapters)
    {
        if (adapterData.description.DedicatedVideoMemory > maxMemoryAdapterData.description.DedicatedVideoMemory)
        {
            maxMemoryAdapterData = adapterData;
        }
    }

    return maxMemoryAdapterData.adapter;
}

Microsoft::WRL::ComPtr<IDXGIOutput> RenderEnvironment::GetAdapterOutput()
{
    Microsoft::WRL::ComPtr<IDXGIAdapter> maxMemoryAdapter = GetMaxMemoryAdapter();
    Microsoft::WRL::ComPtr<IDXGIOutput> output;

    DXGI_OUTPUT_DESC Desc;
    IDXGIOutput* pOutput = nullptr;
    for (UINT i = 0; maxMemoryAdapter->EnumOutputs(i, &pOutput) != DXGI_ERROR_NOT_FOUND; ++i)
    {
        if (FAILED(pOutput->GetDesc(&Desc))) continue;

        if (!Desc.AttachedToDesktop) continue;

        if (Desc.Rotation == DXGI_MODE_ROTATION_ROTATE90 || Desc.Rotation == DXGI_MODE_ROTATION_ROTATE180 || Desc.Rotation == DXGI_MODE_ROTATION_ROTATE270) continue;

        // this is a valid output now
        output = pOutput;
        break;
    }

    return output;
}

void RenderEnvironment::InitializeVideoModes()
{
    Microsoft::WRL::ComPtr<IDXGIOutput> output = GetAdapterOutput();
    std::vector<DXGI_MODE_DESC> allVideoModes;
    const UINT EnumFlags = (DXGI_ENUM_MODES_SCALING | DXGI_ENUM_MODES_INTERLACED);
    UINT NumModes = 0;
    if (SUCCEEDED(output->GetDisplayModeList(DXGI_FORMAT_R8G8B8A8_UNORM, EnumFlags, &NumModes, NULL)))
    {
        allVideoModes.resize(NumModes);
        DXGI_MODE_DESC* vectorData = const_cast<DXGI_MODE_DESC*>(allVideoModes.data());
        if (FAILED(output->GetDisplayModeList(DXGI_FORMAT_R8G8B8A8_UNORM, EnumFlags, &NumModes, vectorData)))
        {
            allVideoModes.clear();
        }
    }

    videoModeDescriptions.clear();
    // cleanup the modes by removing unspecified modes where a specified option or options exists, then copy them out to the final array
    for (int i = 0; i < allVideoModes.size(); ++i)
    {
        DXGI_MODE_DESC* mode = &allVideoModes[i];
        BOOL deadmode = FALSE;

        // don't allow < 640x480 modes
        if (mode->Width < 640) continue;
        if (mode->Height < 480) continue;

        if (mode->Scaling == DXGI_MODE_SCALING_UNSPECIFIED)
        {
            for (int j = 0; j < allVideoModes.size(); j++)
            {
                DXGI_MODE_DESC* mode2 = &allVideoModes[i];

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
            for (int j = 0; j < allVideoModes.size(); j++)
            {
                DXGI_MODE_DESC* mode2 = &allVideoModes[i];

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
        videoModeDescriptions.push_back(*mode);
    }
}

void RenderEnvironment::InitializeDevice()
{
    ENSURE_RESULT(D3D12CreateDevice(GetMaxMemoryAdapter().Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(device.GetAddressOf())));

    // Enable debug messages in debug mode.
#if defined(DX12_DEBUG_LAYER)
    Microsoft::WRL::ComPtr<ID3D12InfoQueue> pInfoQueue;
    if (SUCCEEDED(device.As(&pInfoQueue)))
    {
        pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
        pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
        pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, TRUE);

        // Suppress whole categories of messages
        //D3D12_MESSAGE_CATEGORY Categories[] = {};

        // Suppress messages based on their severity level
        D3D12_MESSAGE_SEVERITY Severities[] =
        {
            D3D12_MESSAGE_SEVERITY_INFO
        };

        // Suppress individual messages by their ID
        D3D12_MESSAGE_ID DenyIds[] = {
            D3D12_MESSAGE_ID_CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE,   // I'm really not sure how to avoid this message.
            D3D12_MESSAGE_ID_MAP_INVALID_NULLRANGE,                         // This warning occurs when using capture frame while graphics debugging.
            D3D12_MESSAGE_ID_UNMAP_INVALID_NULLRANGE,                       // This warning occurs when using capture frame while graphics debugging.
        };

        D3D12_INFO_QUEUE_FILTER NewFilter = {};
        //NewFilter.DenyList.NumCategories = _countof(Categories);
        //NewFilter.DenyList.pCategoryList = Categories;
        NewFilter.DenyList.NumSeverities = _countof(Severities);
        NewFilter.DenyList.pSeverityList = Severities;
        NewFilter.DenyList.NumIDs = _countof(DenyIds);
        NewFilter.DenyList.pIDList = DenyIds;

        ENSURE_RESULT(pInfoQueue->PushStorageFilter(&NewFilter));
    }
#endif
}

Microsoft::WRL::ComPtr<ID3D12CommandQueue> RenderEnvironment::CreateCommandQueue(D3D12_COMMAND_LIST_TYPE type)
{
    assert(device != nullptr);

    D3D12_COMMAND_QUEUE_DESC desc = {};
    desc.Type = type;
    desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
    desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    desc.NodeMask = 0;

    Microsoft::WRL::ComPtr<ID3D12CommandQueue> createdQueue;
    ENSURE_RESULT(device->CreateCommandQueue(&desc, IID_PPV_ARGS(createdQueue.GetAddressOf())));

    return createdQueue;
}

void RenderEnvironment::InitializeSwapChain()
{
    assert(factory != nullptr);
    assert(device != nullptr);
    assert(commandQueue != nullptr);
    assert(parentWindowHandle != nullptr);

    Microsoft::WRL::ComPtr<IDXGIFactory2> dxgiFactory2;
    ENSURE_RESULT(factory.As(&dxgiFactory2));

    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
    swapChainDesc.Width = windowWidth;
    swapChainDesc.Height = windowHeight;
    swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.Stereo = FALSE;
    swapChainDesc.SampleDesc = { 1, 0 };
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.BufferCount = bufferCount;
    swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
    swapChainDesc.Flags = 0;

    Microsoft::WRL::ComPtr<IDXGISwapChain1> dxgiSwapChain1;
    ENSURE_RESULT(dxgiFactory2->CreateSwapChainForHwnd(commandQueue.Get(),
                                                       parentWindowHandle,
                                                       &swapChainDesc,
                                                       nullptr,
                                                       nullptr,
                                                       &dxgiSwapChain1));
    ENSURE_RESULT(dxgiSwapChain1.As(&swapChain));
}

UINT RenderEnvironment::GetCurrentBufferIndex()
{
    assert(swapChain != nullptr);
    Microsoft::WRL::ComPtr<IDXGISwapChain3> swapChain3;
    ENSURE_RESULT(swapChain.As(&swapChain3));
    return swapChain3->GetCurrentBackBufferIndex();
}

Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> RenderEnvironment::CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t numDescriptors, bool isShaderVisible)
{
    assert(device != nullptr);

    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptorHeap;

    D3D12_DESCRIPTOR_HEAP_DESC desc = {};
    desc.NumDescriptors = numDescriptors;
    desc.Type = type;
    if (isShaderVisible)
    {
        desc.NodeMask = 0;
        desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    }
    ENSURE_RESULT(device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&descriptorHeap)));

    return descriptorHeap;
}

void RenderEnvironment::SetViewport(const D3D12_VIEWPORT& viewPort)
{
    screenViewport = viewPort;
}

void RenderEnvironment::InitializeRenderTargetViews()
{
    assert(device != nullptr);
    assert(rtvDescriptorHeap != nullptr);

    auto rtvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
    Microsoft::WRL::ComPtr<ID3D12Resource> backBuffer;
    for (int i = 0; i < bufferCount; ++i)
    {
        ENSURE_RESULT(swapChain->GetBuffer(i, IID_PPV_ARGS(&backBuffer)));

        D3D12_RENDER_TARGET_VIEW_DESC viewDesc;
        viewDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
        viewDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
        viewDesc.Texture2D.MipSlice = 0;
        viewDesc.Texture2D.PlaneSlice = 0;
        device->CreateRenderTargetView(backBuffer.Get(), &viewDesc, rtvHandle);
        renderTargets[i] = backBuffer;
        rtvHandle.Offset(rtvDescriptorSize);
    }

    renderState = RenderState::RENDER_PRESENT;
}

void RenderEnvironment::InitializeAllocatorsAndCommandList()
{
    assert(device != nullptr);
    assert(swapChain != nullptr);

    auto type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    for (int i = 0; i < bufferCount; ++i)
    {
        ENSURE_RESULT(device->CreateCommandAllocator(type, IID_PPV_ARGS(&commandAllocators[i])));
    }

    auto currentBufferIndex = GetCurrentBufferIndex();
    ENSURE_RESULT(device->CreateCommandList(0, type, commandAllocators[currentBufferIndex].Get(), nullptr, IID_PPV_ARGS(&renderCommandList)));
    ENSURE_RESULT(renderCommandList->Close());
}


void RenderEnvironment::InitializeDepthStencilView()
{
    assert(device != nullptr);
    assert(dsvDescriptorHeap != nullptr);

    const DXGI_FORMAT depthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
    // Allocate 2-D surface as the depth/stencil buffer and create a depth/stencil view on this surface
    CD3DX12_HEAP_PROPERTIES depthHeapProperties(D3D12_HEAP_TYPE_DEFAULT);

    D3D12_RESOURCE_DESC depthStencilDesc = CD3DX12_RESOURCE_DESC::Tex2D(depthStencilFormat, windowWidth, windowHeight, 1, 1);
    depthStencilDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
    depthStencilDesc.Format = DXGI_FORMAT_R24G8_TYPELESS;

    D3D12_CLEAR_VALUE depthOptimizedClearValue = {};
    depthOptimizedClearValue.Format = depthStencilFormat;
    depthOptimizedClearValue.DepthStencil.Depth = 1.0f;
    depthOptimizedClearValue.DepthStencil.Stencil = 0;

    ENSURE_RESULT(device->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
        D3D12_HEAP_FLAG_NONE,
        &depthStencilDesc,
        D3D12_RESOURCE_STATE_DEPTH_WRITE,
        &depthOptimizedClearValue,
        IID_PPV_ARGS(depthStencil.GetAddressOf())));

    // Create descriptor to mip level 0 of entire resource using the format of the resource
    D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
    dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
    dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    dsvDesc.Format = depthStencilFormat;
    dsvDesc.Texture2D.MipSlice = 0;

    device->CreateDepthStencilView(depthStencil.Get(), &dsvDesc, dsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
}

void RenderEnvironment::InitializeSynchronizationObjects()
{
    assert(device != nullptr);

    ENSURE_RESULT(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)));
    fenceEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL);

    assert(fence != nullptr && fenceEvent != nullptr && "InitializeSynchronizationObjects() failed.");
}

void RenderEnvironment::InitializeViewportAndScissor()
{
    screenViewport.TopLeftX = 0;
    screenViewport.TopLeftY = 0;
    screenViewport.Width = static_cast<float>(windowWidth);
    screenViewport.Height = static_cast<float>(windowHeight);
    screenViewport.MinDepth = 0.0f;
    screenViewport.MaxDepth = 1.0f;

    scissorRect = { 0, 0, (LONG)windowWidth, (LONG)windowHeight };
}

void RenderEnvironment::Synchronize()
{
    assert(isInitialized == true);
    assert(commandQueue != nullptr);
    assert(fence != nullptr);
    assert(fenceEvent != nullptr);

    ENSURE_RESULT(commandQueue->Signal(fence.Get(), ++fenceValue));

    if (fence->GetCompletedValue() < fenceValue)
    {
        ENSURE_RESULT(fence->SetEventOnCompletion(fenceValue, fenceEvent));
        WaitForSingleObjectEx(fenceEvent, INFINITE, FALSE);
    }
}

// Transition the swap chain back to present.
void RenderEnvironment::BarrierFromTargetToPresent()
{
    auto currentBufferIndex = GetCurrentBufferIndex();
    D3D12_RESOURCE_BARRIER barrier;
    barrier.Transition.pResource = renderTargets[currentBufferIndex].Get();
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    renderCommandList->ResourceBarrier(1, &barrier);
}

// Transition back buffer.
void RenderEnvironment::BarrierFromPresentToTarget()
{
    auto currentBufferIndex = GetCurrentBufferIndex();
    D3D12_RESOURCE_BARRIER barrier;
    barrier.Transition.pResource = renderTargets[currentBufferIndex].Get();
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    renderCommandList->ResourceBarrier(1, &barrier);
}

void RenderEnvironment::ClearScreen()
{
    if (renderState == RenderState::RENDER_PRESENT)
    {
        renderState = RenderState::RENDER_TARGET;

        if (renderCommandListState != CommandListState::CL_RESETED) ResetRenderCommandList();

        BarrierFromPresentToTarget();

        // Record commands.
        float clearColor[] = { 0.0f, 0.0f, 0.0f, 1.0f };

        auto rtvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
        D3D12_CPU_DESCRIPTOR_HANDLE renderTargetHandle;
        CD3DX12_CPU_DESCRIPTOR_HANDLE::InitOffsetted(renderTargetHandle, rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), GetCurrentBufferIndex(), rtvDescriptorSize);

        renderCommandList->ClearRenderTargetView(renderTargetHandle, clearColor, 0, nullptr);
        renderCommandList->ClearDepthStencilView(dsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

        ExecuteRenderCommandList();
    }
}

void RenderEnvironment::Present(UINT SyncInterval, UINT Flags)
{
    if (renderState == RenderState::RENDER_TARGET)
    {
        renderState = RenderState::RENDER_PRESENT;

        if (renderCommandListState != CommandListState::CL_RESETED) ResetRenderCommandList();
        BarrierFromTargetToPresent();
        ExecuteRenderCommandList();

        ENSURE_RESULT(swapChain->Present(SyncInterval, Flags));

        Synchronize();
    }
}

void RenderEnvironment::SetupRenderingCommandList()
{
    auto rtvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    D3D12_CPU_DESCRIPTOR_HANDLE renderTargetHandle;
    CD3DX12_CPU_DESCRIPTOR_HANDLE::InitOffsetted(renderTargetHandle, rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), GetCurrentBufferIndex(), rtvDescriptorSize);

    renderCommandList->RSSetViewports(1, &screenViewport);
    renderCommandList->RSSetScissorRects(1, &scissorRect);
    renderCommandList->OMSetRenderTargets(1, &renderTargetHandle, FALSE, &dsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
}

Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> RenderEnvironment::GetRenderCommandList()
{
    //ASSERT(renderCommandListState == CommandListState::CL_RESETED);
    if (renderCommandListState == CommandListState::CL_EXECUTED)
    {
        ResetRenderCommandList();
        SetupRenderingCommandList();
    }

    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> grphCmdLst;
    ENSURE_RESULT(renderCommandList.As(&grphCmdLst));
    return grphCmdLst;
}

void RenderEnvironment::ResetRenderCommandList()
{
    ASSERT(renderCommandListState == CommandListState::CL_EXECUTED);

    Synchronize();
    auto currentBufferIndex = GetCurrentBufferIndex();
    ENSURE_RESULT(commandAllocators[currentBufferIndex]->Reset());
    ENSURE_RESULT(renderCommandList->Reset(commandAllocators[currentBufferIndex].Get(), nullptr));

    renderCommandListState = CommandListState::CL_RESETED;
}

void RenderEnvironment::ExecuteRenderCommandList()
{
    ASSERT(renderCommandListState == CommandListState::CL_RESETED);

    ENSURE_RESULT(renderCommandList->Close());
    ID3D12CommandList* ppCommandLists[] = { renderCommandList.Get() };
       commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);
    Synchronize();

    renderCommandListState = CommandListState::CL_EXECUTED;
}



UINT RenderEnvironment::GetVideoModesNumber()
{
    return videoModeDescriptions.size();
}

DXGI_MODE_DESC RenderEnvironment::GetVideoMode(UINT index)
{
    return videoModeDescriptions[index];
}