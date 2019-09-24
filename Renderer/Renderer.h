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

    PipelineStateManager stateManager;
    ResourceManager resourceManager;

private:
    RenderEnvironment* pRenderEnv = nullptr;

    bool isInitialized = false;

};
