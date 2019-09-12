#pragma once

#ifdef __cplusplus
#undef CINTERFACE

#include "r_local.h"
#include <d3d11.h>
#include <wrl/client.h>
#include <windows.h>
#include <D3Dcompiler.h>
#include <vector>
#include <string>

struct ShaderSet
{
    Microsoft::WRL::ComPtr<ID3D11InputLayout> inputLayout;

    Microsoft::WRL::ComPtr<ID3D11VertexShader> vertexShader;
    Microsoft::WRL::ComPtr<ID3D11GeometryShader> geometryShader;
    Microsoft::WRL::ComPtr<ID3D11PixelShader> pixelShader;
};

class ShaderLoader
{
public:
    void InitShaders();
    void ShutdownShaders();
    int LoadResourceData(int resourceid, void** resbuf);
    HRESULT CompileShader(LPCVOID pSrcData, SIZE_T SrcDataSize, CONST D3D_SHADER_MACRO* pDefines, LPCSTR pEntrypoint, LPCSTR pTarget, ID3DBlob** ppCode);
    char* LoadShaderSource(int resourceID);
    int CreateShaderBundle(int resourceID, const char* vsentry, const char* gsentry, const char* psentry, D3D11_INPUT_ELEMENT_DESC* layout, int numlayout);
    void BindShaderBundle(int sb);
    void RegisterConstantBuffer(ID3D11Buffer* cBuffer, int slot);
    void BindConstantBuffers();

private:
    HINSTANCE hInstCompiler = nullptr;
    pD3DCompile QD3DCompile = nullptr;

    std::vector<ShaderSet> shaders;
    std::vector<ID3D11Buffer*> constantBuffers;
};

#endif