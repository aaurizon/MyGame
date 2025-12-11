#pragma once

#include <AViewport>
#include <memory>

class IRendererImpl {
public:
    virtual ~IRendererImpl() = default;

    virtual bool initialize(void* nativeWindow, int width, int height) = 0;
    virtual void shutdown() = 0;
    virtual void resize(int width, int height) = 0;
    virtual void draw(const AViewport& viewport) = 0;
    virtual void setWorld(class AWorld* world) = 0;
};
