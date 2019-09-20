#pragma once

#ifdef __cplusplus
#undef CINTERFACE
#endif

#include <d3d12.h>
#include <dxgi.h>
#include <wrl.h>
#include "RenderEnvironment.h"

#ifdef __cplusplus
extern "C" {
#endif

class Renderer
{
public:
    void Init(RenderEnvironment* environment);
    void Release();
    void RenderImpl();

    void CreateRootSignature();
    void CreatePipelineStateObject();

private:
    void CreateTestMesh();

    RenderEnvironment* pRenderEnv;

    Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> pso;
    Microsoft::WRL::ComPtr<ID3D12Resource> uploadBuffer;

    Microsoft::WRL::ComPtr<ID3D12Resource> vertexBuffer;
    D3D12_VERTEX_BUFFER_VIEW vertexBufferView;

    Microsoft::WRL::ComPtr<ID3D12Resource> indexBuffer;
    D3D12_INDEX_BUFFER_VIEW indexBufferView;

    Microsoft::WRL::ComPtr<ID3DBlob> vertexShaderBlob;
    Microsoft::WRL::ComPtr<ID3DBlob> pixelShaderBlob;

};

#ifdef __cplusplus
}
#endif
