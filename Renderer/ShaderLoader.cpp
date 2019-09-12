#include "ShaderLoader.h"
#include "CppWrapper.h"
#include <cassert>

#define STR_EXPAND(tok) #tok
#define DEFINE_CONSTANT_REGISTER(n) {#n, "c"STR_EXPAND (n)}
#define DEFINE_SAMPLER_REGISTER(n) {#n, "s"STR_EXPAND (n)}

void ShaderLoader::InitShaders()
{
    // set up bundle 0 as the NULL shader which may be used to explicitly unbind all shaders from the pipeline
    ShaderSet shaderSet;
    shaderSet.inputLayout = nullptr;
    shaderSet.vertexShader = nullptr;
    shaderSet.geometryShader = nullptr;
    shaderSet.pixelShader = nullptr;
    shaders.push_back(shaderSet);

    // load the compiler (or ensure it's loaded
    // statically linking to d3dcompiler.lib causes it to use d3dcompiler_47.dll which is not on Windows 7 so link dynamically instead
    if (!hInstCompiler || !QD3DCompile) // these should always be NULL
    {
        for (int i = 99; i > 32; i--)
        {
            char libname[256];
            sprintf(libname, "d3dcompiler_%i.dll", i);

            if ((hInstCompiler = LoadLibrary(libname)) != NULL)
            {
                if ((QD3DCompile = (pD3DCompile)GetProcAddress(hInstCompiler, "D3DCompile")) != NULL)
                    return;
                else
                {
                    FreeLibrary(hInstCompiler);
                    hInstCompiler = NULL;
                }
            }
        }
    }

    // error conditions
    assert(hInstCompiler != nullptr);
    assert(QD3DCompile != nullptr);
}

void ShaderLoader::ShutdownShaders()
{
    // release shaders
    shaders.clear();

    // release cBuffers
    for (ID3D11Buffer* cb : constantBuffers)
    {
        if (cb != nullptr)
        {
            cb->Release();
            cb = nullptr;
        }
    } 
    maxSlot = 0;

    // these should no longer be used
    QD3DCompile = nullptr;

    if (hInstCompiler)
    {
        FreeLibrary(hInstCompiler);
        hInstCompiler = NULL;
    }
}

int ShaderLoader::LoadResourceData(int resourceid, void** resbuf)
{
    // per MSDN, UnlockResource is obsolete and does nothing any more.  There is
    // no way to free the memory used by a resource after you're finished with it.
    // If you ask me this is kinda fucked, but what do I know?  We'll just leak it.
    if (resbuf)
    {
        HRSRC hResInfo = FindResource(NULL, MAKEINTRESOURCE(resourceid), RT_RCDATA);

        if (hResInfo)
        {
            HGLOBAL hResData = LoadResource(NULL, hResInfo);

            if (hResData)
            {
                resbuf[0] = (byte*)LockResource(hResData);
                return SizeofResource(NULL, hResInfo);
            }
        }
    }

    return 0;
}

HRESULT ShaderLoader::CompileShader(LPCVOID pSrcData, SIZE_T SrcDataSize, CONST D3D_SHADER_MACRO* pDefines, LPCSTR pEntrypoint, LPCSTR pTarget, ID3DBlob** ppCode)
{
    // and compile it
    ID3DBlob* errorBlob = NULL;
    HRESULT hr = QD3DCompile(pSrcData, SrcDataSize, NULL, pDefines, NULL, pEntrypoint, pTarget, 0, 0, ppCode, &errorBlob);

    // even if the compile succeeded there may be warnings so report everything here
    if (errorBlob)
    {
        //char* compileError = (char*)errorBlob->GetBufferPointer();
        //ri.Con_Printf(PRINT_ALL, "Error compiling %s:\n%s\n", pEntrypoint, compileError);
        //errorBlob->Release();
        assert(false);
    }

    return hr;
}

std::unique_ptr<char[]> ShaderLoader::LoadShaderSource(int resourceID)
{
    // we can't use ID3DInclude in C (we probably can but it's writing our own COM crap in C and life is not short enough for that)
    // so this is how we'll handle includes
    char* includesource = NULL;
    char* shadersource = NULL;

    // load the individual resources
    int includelength = LoadResourceData(IDR_COMMON, (void**)& includesource);
    int shaderlength = LoadResourceData(resourceID, (void**)& shadersource);

    // alloc sufficient memory for both
    //char* fullsource = (char*)ri.Load_AllocMemory(includelength + shaderlength + 2);
    std::unique_ptr<char[]> fullsource(new char[includelength + shaderlength + 2]);

    // and combine them
    strcpy(fullsource.get(), includesource);
    strcpy(fullsource.get() + includelength, shadersource);

    // ensure this happens because resource-loading of an RCDATA resource will not necessarily terminate strings
    fullsource[includelength + shaderlength] = 0;

    return fullsource;
}

int ShaderLoader::CreateShaderBundle(int resourceID, const char* vsentry, const char* gsentry, const char* psentry, D3D11_INPUT_ELEMENT_DESC* layout, int numlayout)
{
    shaders.push_back(ShaderSet());
    ShaderSet& sb = shaders.back();

    // set up to create
    std::unique_ptr<char[]> shadersource = LoadShaderSource(resourceID);
    int shaderlength = strlen(shadersource.get());

    // explicitly NULL everything so that we can safely destroy stuff if anything fails
   // memset(sb, 0, sizeof(*sb));

    // failed to load the resource
    if (!shaderlength || !shadersource) return -1;

    // make the vertex shader
    if (vsentry)
    {
        const D3D_SHADER_MACRO vsdefines[] = {
            {"VERTEXSHADER", "1"},
            {NULL, NULL}
        };

        ID3DBlob* vsBlob = NULL;

        if (SUCCEEDED(CompileShader(shadersource.get(), shaderlength, vsdefines, vsentry, "vs_5_0", &vsBlob)))
        {
            if (vsBlob)
            {
                //d3d_Device->lpVtbl->CreateVertexShader (d3d_Device, (DWORD *) vsBlob->lpVtbl->GetBufferPointer (vsBlob), vsBlob->lpVtbl->GetBufferSize (vsBlob), NULL, &sb->VertexShader);
                RWGetDevice()->CreateVertexShader((DWORD*)vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), NULL, sb.vertexShader.GetAddressOf());

                // allowed to be NULL for drawing without buffers
                if (layout && numlayout)
                {
                    //d3d_Device->lpVtbl->CreateInputLayout(d3d_Device, layout, numlayout, vsBlob->lpVtbl->GetBufferPointer (vsBlob), vsBlob->lpVtbl->GetBufferSize (vsBlob), &sb->InputLayout);
                    RWGetDevice()->CreateInputLayout(layout, numlayout, vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), sb.inputLayout.GetAddressOf());
                }

                vsBlob->Release();
            }
        }
    }

    // make the geometry shader
    if (gsentry)
    {
        const D3D_SHADER_MACRO gsdefines[] = {
            {"GEOMETRYSHADER", "1"},
            {NULL, NULL}
        };

        ID3DBlob* gsBlob = NULL;

        if (SUCCEEDED(CompileShader(shadersource.get(), shaderlength, gsdefines, gsentry, "gs_5_0", &gsBlob)))
        {
            if (gsBlob)
            {
                //d3d_Device->lpVtbl->CreateGeometryShader (d3d_Device, (DWORD *) gsBlob->lpVtbl->GetBufferPointer (gsBlob), gsBlob->lpVtbl->GetBufferSize (gsBlob), NULL, &sb->GeometryShader);
                RWGetDevice()->CreateGeometryShader((DWORD*)gsBlob->GetBufferPointer(), gsBlob->GetBufferSize(), NULL, sb.geometryShader.GetAddressOf());

                gsBlob->Release();
            }
        }
    }

    // make the pixel shader
    if (psentry)
    {
        const D3D_SHADER_MACRO psdefines[] = {
            {"PIXELSHADER", "1"},
            {NULL, NULL}
        };

        ID3DBlob* psBlob = NULL;

        if (SUCCEEDED(CompileShader(shadersource.get(), shaderlength, psdefines, psentry, "ps_5_0", &psBlob)))
        {
            if (psBlob)
            {
                //d3d_Device->lpVtbl->CreatePixelShader(d3d_Device, (DWORD *) psBlob->lpVtbl->GetBufferPointer (psBlob), psBlob->lpVtbl->GetBufferSize (psBlob), NULL, &sb->PixelShader);
                RWGetDevice()->CreatePixelShader((DWORD*)psBlob->GetBufferPointer(), psBlob->GetBufferSize(), NULL, sb.pixelShader.GetAddressOf());

                psBlob->Release();
            }
        }
    }

    // this is the actual shader id that was loaded
    return (shaders.size() - 1);
}

void ShaderLoader::BindShaderBundle(int sb)
{
    static ID3D11InputLayout* oldil = nullptr;
    static ID3D11VertexShader* oldvs = nullptr;
    static ID3D11GeometryShader* oldgs = nullptr;
    static ID3D11PixelShader* oldps = nullptr;

    if (sb < 0 || sb >= shaders.size())
    {
        // shader bundle 0 is the NULL shader bundle which may be explicitly used to clear down all bindings
        BindShaderBundle(0);
        return;
    }

    if (oldil != shaders[sb].inputLayout.Get())
    {
        //d3d_Context->lpVtbl->IASetInputLayout (d3d_Context, d3d_Shaders[sb].InputLayout);
        RWGetDeviceContext()->IASetInputLayout(shaders[sb].inputLayout.Get());
        oldil = shaders[sb].inputLayout.Get();
    }

    if (oldvs != shaders[sb].vertexShader.Get())
    {
        //d3d_Context->lpVtbl->VSSetShader (d3d_Context, d3d_Shaders[sb].VertexShader, NULL, 0);
        RWGetDeviceContext()->VSSetShader(shaders[sb].vertexShader.Get(), nullptr, 0);
        oldvs = shaders[sb].vertexShader.Get();
    }

    if (oldgs != shaders[sb].geometryShader.Get())
    {
        //d3d_Context->lpVtbl->GSSetShader (d3d_Context, d3d_Shaders[sb].GeometryShader, NULL, 0);
        RWGetDeviceContext()->GSSetShader(shaders[sb].geometryShader.Get(), nullptr, 0);
        oldgs = shaders[sb].geometryShader.Get();
    }

    if (oldps != shaders[sb].pixelShader.Get())
    {
        //d3d_Context->lpVtbl->PSSetShader (d3d_Context, d3d_Shaders[sb].PixelShader, NULL, 0);
        RWGetDeviceContext()->PSSetShader(shaders[sb].pixelShader.Get(), nullptr, 0);
        oldps = shaders[sb].pixelShader.Get();
    }
}

void ShaderLoader::RegisterConstantBuffer(ID3D11Buffer* cBuffer, int slot)
{
    if (slot >= 0 && slot < NUM_OF_SLOTS)
    {
        if (slot > maxSlot)
            maxSlot = slot;

        constantBuffers[slot] = cBuffer;
    }
    else
    {
        assert(false);
        exit(-1);
    }
}

void ShaderLoader::BindConstantBuffers()
{
    // d3d_MaxCBufferSlot is 0-based so add 1 for the actual number
    RWGetDeviceContext()->VSSetConstantBuffers(0, maxSlot + 1, constantBuffers.data());
    RWGetDeviceContext()->GSSetConstantBuffers(0, maxSlot + 1, constantBuffers.data());
    RWGetDeviceContext()->PSSetConstantBuffers(0, maxSlot + 1, constantBuffers.data());
}