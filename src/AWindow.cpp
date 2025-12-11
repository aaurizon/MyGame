#include <AWindow>

#include <IWindowImpl.h>
#include <Win32/AWindowImplWin32.h>
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
        return nullptr;
    }
}

} // namespace

AWindow::AWindow(const std::string& title, int width, int height, EGraphicsBackend backend)
    : AWindow(title, width, height, nullptr, 0, 0, false, backend) {}

AWindow::AWindow(AWindow& parent, int x, int y, int width, int height, EGraphicsBackend backend)
    : AWindow("", width, height, parent.getNativeHandle(), x, y, true, backend) {}

AWindow::AWindow(const std::string& title, int width, int height, void* parentHandle, int x, int y, bool child, EGraphicsBackend backend)
    : viewport_(width, height, 0, 0), backend_(backend) {
    impl_ = std::make_unique<AWindowImplWin32>();
    const bool created = impl_->create(title, width, height, parentHandle, x, y, child);
    assert(created && "Failed to create Win32 window");
    if (backend_ != EGraphicsBackend::None) {
        recreateRenderer(width, height);
    }
}

AWindow::~AWindow() {
    if (renderer_) {
        renderer_->shutdown();
    }
}

bool AWindow::isOpen() const {
    return impl_ ? impl_->isOpen() : false;
}

void AWindow::close() {
    if (impl_) {
        impl_->close();
    }
}

void AWindow::setTitle(const std::string& title) {
    if (impl_) {
        impl_->setTitle(title);
    }
}

std::vector<std::shared_ptr<AEvent>> AWindow::pollEvents() {
    if (!impl_) {
        return {};
    }
    return impl_->pollEvents();
}

void AWindow::setCursorGrabbed(bool grabbed) {
    if (impl_) {
        impl_->setCursorGrabbed(grabbed);
    }
}

bool AWindow::isCursorGrabbed() const {
    if (impl_) {
        return impl_->isCursorGrabbed();
    }
    return false;
}

int AWindow::getWidth() const {
    return impl_ ? impl_->getWidth() : 0;
}

int AWindow::getHeight() const {
    return impl_ ? impl_->getHeight() : 0;
}

void* AWindow::getNativeHandle() const {
    return impl_ ? impl_->getNativeHandle() : nullptr;
}

void AWindow::setRect(int x, int y, int width, int height) {
    if (impl_) {
        impl_->setRect(x, y, width, height);
    }
    viewport_.setRect(0, 0, width, height);
    lastWidth_ = -1;
    lastHeight_ = -1;
}

void AWindow::display() {
    if (!renderer_) {
        return;
    }

    const int w = getWidth();
    const int h = getHeight();
    if (w != lastWidth_ || h != lastHeight_) {
        viewport_.setRect(0, 0, w, h);
        renderer_->resize(w, h);
        lastWidth_ = w;
        lastHeight_ = h;
    }

    renderer_->setWorld(viewport_.getWorld());
    renderer_->draw(viewport_);
}

void AWindow::setGraphicsBackend(EGraphicsBackend backend) {
    if (backend_ == backend) {
        return;
    }

    backend_ = backend;
    if (renderer_) {
        renderer_->shutdown();
    }
    renderer_.reset();
    if (backend_ != EGraphicsBackend::None) {
        recreateRenderer(getWidth(), getHeight());
    }
}

EGraphicsBackend AWindow::getGraphicsBackend() const {
    return backend_;
}

AViewport& AWindow::getViewport() {
    return viewport_;
}

void AWindow::recreateRenderer(int width, int height) {
    renderer_ = createRenderer(backend_);
    if (!renderer_) {
        return;
    }
    if (!renderer_->initialize(getNativeHandle(), width, height)) {
        renderer_.reset();
        backend_ = EGraphicsBackend::None;
        return;
    }
    lastWidth_ = width;
    lastHeight_ = height;
    renderer_->setWorld(viewport_.getWorld());
}
