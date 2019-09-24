#include "Renderer.h"
#include "Utils.h"
#include <cassert>

Renderer::Renderer() {}

Renderer::~Renderer()
{
    Release();
}

bool Renderer::Init(RenderEnvironment* environment)
{
    assert(environment != nullptr);
    pRenderEnv = environment;

    stateManager.Initialize(pRenderEnv->GetDevice());
    resourceManager.Initialize(pRenderEnv);

    isInitialized = true;
    return isInitialized;
}

void Renderer::Release()
{
    pRenderEnv = nullptr;

    stateManager.Release();
    resourceManager.Release();

    isInitialized = false;
}
