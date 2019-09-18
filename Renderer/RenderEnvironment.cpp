#include "RenderEnvironment.h"
#include <dxgi1_6.h>
#include <cassert>

#pragma comment (lib, "d3dcompiler.lib")
#pragma comment (lib, "dxguid.lib")
#pragma comment (lib, "d3d12.lib")
#pragma comment (lib, "dxgi.lib")

static inline void ThrowIfFailed(HRESULT hr)
{
    if (FAILED(hr))
    {
        throw std::exception();
    }
}

RenderEnvironment::RenderEnvironment(HWND handle, std::size_t width, std::size_t height)
: parentWindowHandle(handle)
, windowWidth(width)
, windowHeight(height)
{
    InitializeViewportAndScissor();
}

RenderEnvironment::~RenderEnvironment()
{
    Synchronize();
    CloseHandle(fenceEvent);
}

void RenderEnvironment::InitializeAll()
{
    InitializeFactory();
    InitializeAdapters();
    InitializeDevice();
    commandQueue = CreateCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT);
    InitializeSwapChain();
    rtvDescriptorHeap = CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_RTV, bufferCount);
    dsvDescriptorHeap = CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 1);
    InitializeAllocatorsAndCommandList();
    InitializeRenderTargetViews();
    InitializeDepthStencilView();
    InitializeSynchronizationObjects();
}

void RenderEnvironment::InitializeFactory()
{
    factory.Reset();
    ThrowIfFailed(CreateDXGIFactory(IID_PPV_ARGS(&factory)));
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
        ThrowIfFailed(pAdapter->GetDesc(&adapterData.description));
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

void RenderEnvironment::InitializeDevice()
{
    ThrowIfFailed(D3D12CreateDevice(GetMaxMemoryAdapter().Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(device.GetAddressOf())));
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
    ThrowIfFailed(device->CreateCommandQueue(&desc, IID_PPV_ARGS(createdQueue.GetAddressOf())));

    return createdQueue;
}

void RenderEnvironment::InitializeSwapChain()
{
    assert(factory != nullptr);
    assert(device != nullptr);
    assert(commandQueue != nullptr);
    assert(parentWindowHandle != nullptr);

    Microsoft::WRL::ComPtr<IDXGIFactory2> dxgiFactory2;
    ThrowIfFailed(factory.As(&dxgiFactory2));

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
    ThrowIfFailed(dxgiFactory2->CreateSwapChainForHwnd(commandQueue.Get(),
                                                       parentWindowHandle,
                                                       &swapChainDesc,
                                                       nullptr,
                                                       nullptr,
                                                       &dxgiSwapChain1));
    ThrowIfFailed(dxgiSwapChain1.As(&swapChain));
}

UINT RenderEnvironment::GetCurrentBufferIndex()
{
    assert(swapChain != nullptr);
    Microsoft::WRL::ComPtr<IDXGISwapChain3> swapChain3;
    ThrowIfFailed(swapChain.As(&swapChain3));
    return swapChain3->GetCurrentBackBufferIndex();
}

Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> RenderEnvironment::CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t numDescriptors)
{
    assert(device != nullptr);

    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptorHeap;

    D3D12_DESCRIPTOR_HEAP_DESC desc = {};
    desc.NumDescriptors = numDescriptors;
    desc.Type = type;

    ThrowIfFailed(device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&descriptorHeap)));

    return descriptorHeap;
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
        ThrowIfFailed(swapChain->GetBuffer(i, IID_PPV_ARGS(&backBuffer)));

        D3D12_RENDER_TARGET_VIEW_DESC viewDesc;
        viewDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
        viewDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
        viewDesc.Texture2D.MipSlice = 0;
        viewDesc.Texture2D.PlaneSlice = 0;
        device->CreateRenderTargetView(backBuffer.Get(), &viewDesc, rtvHandle);
        renderTargets[i] = backBuffer;
        rtvHandle.Offset(rtvDescriptorSize);
    }
}

void RenderEnvironment::InitializeAllocatorsAndCommandList()
{
    assert(device != nullptr);
    assert(swapChain != nullptr);

    auto type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    for (int i = 0; i < bufferCount; ++i)
    {
        device->CreateCommandAllocator(type, IID_PPV_ARGS(&commandAllocators[i]));
    }

    auto currentBufferIndex = GetCurrentBufferIndex();
    ThrowIfFailed(device->CreateCommandList(0, type, commandAllocators[currentBufferIndex].Get(), nullptr, IID_PPV_ARGS(&commandList)));
    ThrowIfFailed(commandList->Close());
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

    ThrowIfFailed(device->CreateCommittedResource(
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

    ThrowIfFailed(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)));
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
    assert(commandQueue != nullptr);
    assert(fence != nullptr);
    assert(fenceEvent != nullptr);

    ThrowIfFailed(commandQueue->Signal(fence.Get(), ++fenceValue));

    if (fence->GetCompletedValue() < fenceValue)
    {
        ThrowIfFailed(fence->SetEventOnCompletion(fenceValue, fenceEvent));
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
    commandList->ResourceBarrier(1, &barrier);
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
    commandList->ResourceBarrier(1, &barrier);
}

void RenderEnvironment::ClearScreen()
{
    // Prepare command list.
    auto currentBufferIndex = GetCurrentBufferIndex();
    ThrowIfFailed(commandAllocators[currentBufferIndex]->Reset());
    ThrowIfFailed(commandList->Reset(commandAllocators[currentBufferIndex].Get(), nullptr));

    // Reset resources (descr heps, root sig ...)
    auto rtvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    D3D12_CPU_DESCRIPTOR_HANDLE renderTargetHandle;
    CD3DX12_CPU_DESCRIPTOR_HANDLE::InitOffsetted(renderTargetHandle, rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), currentBufferIndex, rtvDescriptorSize);

    // Setup viewport.
    commandList->RSSetViewports(1, &screenViewport);
    commandList->RSSetScissorRects(1, &scissorRect);

    BarrierFromPresentToTarget();

    // Record commands.
    const float clearColor[] = { 0.1f, 0.1f, 0.1f, 1.0f };
    commandList->ClearRenderTargetView(renderTargetHandle, clearColor, 0, nullptr);
    commandList->ClearDepthStencilView(dsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

    // Set the back buffer as the render target.
    commandList->OMSetRenderTargets(1, &renderTargetHandle, FALSE, &dsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
    
}

void RenderEnvironment::Present()
{
    BarrierFromTargetToPresent();

    ThrowIfFailed(commandList->Close());

    // Execute command list
    ID3D12CommandList* ppCommandLists[] = { commandList.Get() };
    commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

    // Present
    ThrowIfFailed(swapChain->Present(vSynch, 0));

    Synchronize();
}