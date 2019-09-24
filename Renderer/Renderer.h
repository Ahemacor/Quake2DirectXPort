#pragma once

#ifdef __cplusplus
#undef CINTERFACE
#endif

#include <d3d12.h>
#include <dxgi.h>
#include <wrl.h>
#include "RenderEnvironment.h"
#include "PipelineStateManager.h"
#include "ResourceManager.h"
#include <map>
#include <string>

class Renderer
{
public:
    Renderer();
    ~Renderer();

    bool Init(RenderEnvironment* environment);
    void Release();
    void RenderImpl();

    //void CreateTexture();

private:
    RenderEnvironment* pRenderEnv = nullptr;

    //Microsoft::WRL::ComPtr<ID3D12Resource> imageResource;
    //std::vector<std::uint8_t> imageData;

    //Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptorHeap;

    //Microsoft::WRL::ComPtr<ID3D12Resource> uploadImage;
    //Microsoft::WRL::ComPtr<ID3D12Resource> uploadBuffer;

    PipelineStateManager stateManager;
    ResourceManager resourceManager;
    bool isInitialized = false;
};
