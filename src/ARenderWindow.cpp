#include <ARenderWindow>

#include <Graphics/IRendererImpl.h>
#include <Graphics/OpenGL/OpenGLRenderer.h>
#include <Graphics/Vulkan/VulkanRenderer.h>
#include <Graphics/DirectX11/DirectX11Renderer.h>
#include <Graphics/DirectX12/DirectX12Renderer.h>
#include <cassert>

namespace {

std::unique_ptr<IRendererImpl> createRenderer(EGraphicsBackend backend) {
    switch (backend) {
    case EGraphicsBackend::OpenGL:
        return std::make_unique<OpenGLRenderer>();
    case EGraphicsBackend::Vulkan:
        return std::make_unique<VulkanRenderer>();
    case EGraphicsBackend::DirectX11:
        return std::make_unique<DirectX11Renderer>();
    case EGraphicsBackend::DirectX12:
        return std::make_unique<DirectX12Renderer>();
    default:
        return std::make_unique<OpenGLRenderer>();
    }
}

} // namespace

class ARenderWindow::Impl {
public:
    Impl(int width, int height, EGraphicsBackend backend, ARenderWindow& owner)
        : viewport_(width, height, 0, 0), backend_(backend), owner_(owner) {
        // Base AWindow already created the platform window; we only need a renderer.
        recreateRenderer(owner_.getNativeHandle(), width, height);
    }

    ~Impl() {
        if (renderer_) {
            renderer_->shutdown();
        }
    }

    void display() {
        if (!renderer_) {
            return;
        }

        // Keep viewport in sync with the window client size.
        const int w = owner_.getWidth();
        const int h = owner_.getHeight();
        if (w != lastWidth_ || h != lastHeight_) {
            viewport_.setRect(0, 0, w, h);
            renderer_->resize(w, h);
            lastWidth_ = w;
            lastHeight_ = h;
        }

        renderer_->setWorld(viewport_.getWorld());
        renderer_->draw(viewport_);
    }

    void setGraphicsBackend(EGraphicsBackend backend) {
        if (backend_ == backend) {
            return;
        }
        backend_ = backend;
        if (renderer_) {
            renderer_->shutdown();
        }
        renderer_.reset();
        recreateRenderer(owner_.getNativeHandle(), owner_.getWidth(), owner_.getHeight());
    }

    EGraphicsBackend getGraphicsBackend() const { return backend_; }
    AViewport& getViewport() { return viewport_; }

private:
    void recreateRenderer(void* nativeHandle, int width, int height) {
        renderer_ = createRenderer(backend_);
        if (!renderer_->initialize(nativeHandle, width, height)) {
            renderer_.reset();
            return;
        }
        lastWidth_ = width;
        lastHeight_ = height;
        renderer_->setWorld(viewport_.getWorld());
    }

    AViewport viewport_;
    std::unique_ptr<IRendererImpl> renderer_;
    EGraphicsBackend backend_;
    int lastWidth_{0};
    int lastHeight_{0};
    ARenderWindow& owner_;
};

ARenderWindow::ARenderWindow(const std::string& title, int width, int height, EGraphicsBackend backend)
    : AWindow(title, width, height) {
    renderImpl_ = std::make_unique<Impl>(width, height, backend, *this);
}

ARenderWindow::ARenderWindow(AWindow& parent, int x, int y, int width, int height, EGraphicsBackend backend)
    : AWindow("", width, height, parent.getNativeHandle(), x, y, true) {
    renderImpl_ = std::make_unique<Impl>(width, height, backend, *this);
}

ARenderWindow::~ARenderWindow() = default;

void ARenderWindow::display() { renderImpl_->display(); }

void ARenderWindow::setGraphicsBackend(EGraphicsBackend backend) { renderImpl_->setGraphicsBackend(backend); }

EGraphicsBackend ARenderWindow::getGraphicsBackend() const { return renderImpl_->getGraphicsBackend(); }

AViewport& ARenderWindow::getViewport() { return renderImpl_->getViewport(); }

void ARenderWindow::setRect(int x, int y, int width, int height) {
    AWindow::setRect(x, y, width, height);
    // Force resize on next display.
    renderImpl_->getViewport().setRect(0, 0, width, height);
}
