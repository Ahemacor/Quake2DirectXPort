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
        cb->Release();
    }
    constantBuffers.clear();

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

char* ShaderLoader::LoadShaderSource(int resourceID)
{
    // we can't use ID3DInclude in C (we probably can but it's writing our own COM crap in C and life is not short enough for that)
    // so this is how we'll handle includes
    char* includesource = NULL;
    char* shadersource = NULL;

    // load the individual resources
    int includelength = LoadResourceData(IDR_COMMON, (void**)& includesource);
    int shaderlength = LoadResourceData(resourceID, (void**)& shadersource);

    // alloc sufficient memory for both
    char* fullsource = (char*)ri.Load_AllocMemory(includelength + shaderlength + 2);

    // and combine them
    strcpy(fullsource, includesource);
    strcpy(&fullsource[includelength], shadersource);

    // ensure this happens because resource-loading of an RCDATA resource will not necessarily terminate strings
    fullsource[includelength + shaderlength] = 0;

    return fullsource;
}

int ShaderLoader::CreateShaderBundle(int resourceID, const char* vsentry, const char* gsentry, const char* psentry, D3D11_INPUT_ELEMENT_DESC* layout, int numlayout)
{
    shaderbundle_t* sb = &d3d_Shaders[d3d_NumShaders];

    // set up to create
    char* shadersource = D_LoadShaderSource(resourceID);
    int shaderlength = strlen(shadersource);

    // explicitly NULL everything so that we can safely destroy stuff if anything fails
    memset(sb, 0, sizeof(*sb));

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

        if (SUCCEEDED(D_CompileShader(shadersource, shaderlength, vsdefines, vsentry, "vs_5_0", &vsBlob)))
        {
            if (vsBlob)
            {
                //d3d_Device->lpVtbl->CreateVertexShader (d3d_Device, (DWORD *) vsBlob->lpVtbl->GetBufferPointer (vsBlob), vsBlob->lpVtbl->GetBufferSize (vsBlob), NULL, &sb->VertexShader);
                RWGetDevice()->lpVtbl->CreateVertexShader(RWGetDevice(), (DWORD*)vsBlob->lpVtbl->GetBufferPointer(vsBlob), vsBlob->lpVtbl->GetBufferSize(vsBlob), NULL, &sb->VertexShader);

                // allowed to be NULL for drawing without buffers
                if (layout && numlayout)
                {
                    //d3d_Device->lpVtbl->CreateInputLayout(d3d_Device, layout, numlayout, vsBlob->lpVtbl->GetBufferPointer (vsBlob), vsBlob->lpVtbl->GetBufferSize (vsBlob), &sb->InputLayout);
                    RWGetDevice()->lpVtbl->CreateInputLayout(RWGetDevice(), layout, numlayout, vsBlob->lpVtbl->GetBufferPointer(vsBlob), vsBlob->lpVtbl->GetBufferSize(vsBlob), &sb->InputLayout);
                }

                vsBlob->lpVtbl->Release(vsBlob);
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

        if (SUCCEEDED(D_CompileShader(shadersource, shaderlength, gsdefines, gsentry, "gs_5_0", &gsBlob)))
        {
            if (gsBlob)
            {
                //d3d_Device->lpVtbl->CreateGeometryShader (d3d_Device, (DWORD *) gsBlob->lpVtbl->GetBufferPointer (gsBlob), gsBlob->lpVtbl->GetBufferSize (gsBlob), NULL, &sb->GeometryShader);
                RWGetDevice()->lpVtbl->CreateGeometryShader(RWGetDevice(), (DWORD*)gsBlob->lpVtbl->GetBufferPointer(gsBlob), gsBlob->lpVtbl->GetBufferSize(gsBlob), NULL, &sb->GeometryShader);

                gsBlob->lpVtbl->Release(gsBlob);
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

        if (SUCCEEDED(D_CompileShader(shadersource, shaderlength, psdefines, psentry, "ps_5_0", &psBlob)))
        {
            if (psBlob)
            {
                //d3d_Device->lpVtbl->CreatePixelShader(d3d_Device, (DWORD *) psBlob->lpVtbl->GetBufferPointer (psBlob), psBlob->lpVtbl->GetBufferSize (psBlob), NULL, &sb->PixelShader);
                RWGetDevice()->lpVtbl->CreatePixelShader(RWGetDevice(), (DWORD*)psBlob->lpVtbl->GetBufferPointer(psBlob), psBlob->lpVtbl->GetBufferSize(psBlob), NULL, &sb->PixelShader);

                psBlob->lpVtbl->Release(psBlob);
            }
        }
    }

    // throw away memory
    ri.Load_FreeMemory();

    d3d_NumShaders++;

    // this is the actual shader id that was loaded
    return (d3d_NumShaders - 1);
}

void ShaderLoader::BindShaderBundle(int sb)
{
    static ID3D11InputLayout* oldil = NULL;
    static ID3D11VertexShader* oldvs = NULL;
    static ID3D11GeometryShader* oldgs = NULL;
    static ID3D11PixelShader* oldps = NULL;

    if (sb < 0 || sb >= MAX_SHADERS)
    {
        // shader bundle 0 is the NULL shader bundle which may be explicitly used to clear down all bindings
        D_BindShaderBundle(0);
        return;
    }

    if (oldil != d3d_Shaders[sb].InputLayout)
    {
        //d3d_Context->lpVtbl->IASetInputLayout (d3d_Context, d3d_Shaders[sb].InputLayout);
        RWGetDeviceContext()->lpVtbl->IASetInputLayout(RWGetDeviceContext(), d3d_Shaders[sb].InputLayout);
        oldil = d3d_Shaders[sb].InputLayout;
    }

    if (oldvs != d3d_Shaders[sb].VertexShader)
    {
        //d3d_Context->lpVtbl->VSSetShader (d3d_Context, d3d_Shaders[sb].VertexShader, NULL, 0);
        RWGetDeviceContext()->lpVtbl->VSSetShader(RWGetDeviceContext(), d3d_Shaders[sb].VertexShader, NULL, 0);
        oldvs = d3d_Shaders[sb].VertexShader;
    }

    if (oldgs != d3d_Shaders[sb].GeometryShader)
    {
        //d3d_Context->lpVtbl->GSSetShader (d3d_Context, d3d_Shaders[sb].GeometryShader, NULL, 0);
        RWGetDeviceContext()->lpVtbl->GSSetShader(RWGetDeviceContext(), d3d_Shaders[sb].GeometryShader, NULL, 0);
        oldgs = d3d_Shaders[sb].GeometryShader;
    }

    if (oldps != d3d_Shaders[sb].PixelShader)
    {
        //d3d_Context->lpVtbl->PSSetShader (d3d_Context, d3d_Shaders[sb].PixelShader, NULL, 0);
        RWGetDeviceContext()->lpVtbl->PSSetShader(RWGetDeviceContext(), d3d_Shaders[sb].PixelShader, NULL, 0);
        oldps = d3d_Shaders[sb].PixelShader;
    }
}

void ShaderLoader::RegisterConstantBuffer(ID3D11Buffer* cBuffer, int slot)
{
    if (slot >= 0 && slot < D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT)
    {
        if (slot > d3d_MaxCBufferSlot)
            d3d_MaxCBufferSlot = slot;

        d3d_ConstantBuffers[slot] = cBuffer;
    }
    else ri.Sys_Error(ERR_FATAL, "D_RegisterConstantBuffer : slot out of range");
}

void ShaderLoader::BindConstantBuffers()
{
    RWGetDeviceContext()->VSSetConstantBuffers(0, d3d_MaxCBufferSlot + 1, d3d_ConstantBuffers);
    RWGetDeviceContext()->GSSetConstantBuffers(0, d3d_MaxCBufferSlot + 1, d3d_ConstantBuffers);
    RWGetDeviceContext()->PSSetConstantBuffers(0, d3d_MaxCBufferSlot + 1, d3d_ConstantBuffers);
}