#pragma once

#ifdef __cplusplus
#undef CINTERFACE
#endif

#include <wrl.h>
#include <d3d12.h>
#include <dxgi.h>
#include "d3dx12.h"
#include <vector>
#include <Windows.h>
#include <cstdint>

class RenderEnvironment
{
public:
    static constexpr unsigned int bufferCount = 3;

    struct AdapterData
    {
        Microsoft::WRL::ComPtr<IDXGIAdapter> adapter;
        DXGI_ADAPTER_DESC description = {};
    };

    RenderEnvironment();
    ~RenderEnvironment();

    bool Initialize(HWND handle, std::size_t width, std::size_t height);
    void Release();

    void ClearScreen();
    void Present();

    void Synchronize();

    Microsoft::WRL::ComPtr<ID3D12Device> GetDevice() { return device; }
    Microsoft::WRL::ComPtr<ID3D12CommandQueue> GetQueue() { return commandQueue; }
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> GetGraphicsCommandList();
    void ResetCommandList();
    void ExecuteCommandList();

    UINT GetVideoModesNumber();
    DXGI_MODE_DESC GetVideoMode(UINT index);

    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t numDescriptors, bool isShaderVisible = false);

private:
    RenderEnvironment(const RenderEnvironment&) = delete;
    RenderEnvironment& operator=(const RenderEnvironment&) = delete;

    void InitializeFactory();
    void InitializeAdapters();
    std::size_t GetNumberOfAdapters();
    AdapterData GetAdapter(std::size_t index);
    Microsoft::WRL::ComPtr<IDXGIAdapter> GetMaxMemoryAdapter();
    Microsoft::WRL::ComPtr<IDXGIOutput> GetAdapterOutput();
    void InitializeVideoModes();
    void InitializeDevice();
    Microsoft::WRL::ComPtr<ID3D12CommandQueue> CreateCommandQueue(D3D12_COMMAND_LIST_TYPE type);
    void InitializeSwapChain();
    UINT GetCurrentBufferIndex();
    void InitializeRenderTargetViews();
    void InitializeAllocatorsAndCommandList();
    void InitializeDepthStencilView();
    void InitializeSynchronizationObjects();
    void InitializeViewportAndScissor();
    void BarrierFromTargetToPresent();
    void BarrierFromPresentToTarget();

    HWND parentWindowHandle;
    std::size_t windowWidth;
    std::size_t windowHeight;

    Microsoft::WRL::ComPtr<IDXGIFactory> factory;
    std::vector<AdapterData> adapters;
    Microsoft::WRL::ComPtr<ID3D12Device> device;
    Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandQueue;
    Microsoft::WRL::ComPtr<IDXGISwapChain> swapChain;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rtvDescriptorHeap;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> dsvDescriptorHeap;
    Microsoft::WRL::ComPtr<ID3D12Resource> depthStencil;
    Microsoft::WRL::ComPtr<ID3D12Resource> renderTargets[bufferCount];
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocators[bufferCount];
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList;

    D3D12_VIEWPORT screenViewport = {};
    D3D12_RECT scissorRect = {};

    Microsoft::WRL::ComPtr<ID3D12Fence> fence;
    HANDLE fenceEvent = nullptr;
    std::uint64_t fenceValue = 0;

    bool vSynch = false;

    std::vector<DXGI_MODE_DESC> videoModeDescriptions;

    bool isInitialized = false;
};

