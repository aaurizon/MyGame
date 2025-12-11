#include <ARenderWindow>

#include <Graphics/IRendererImpl.h>
#include <Graphics/OpenGL/OpenGLRenderer.h>
#include <Graphics/Vulkan/VulkanRenderer.h>
#include <Win32/AWindowImplWin32.h>
#include <cassert>

class ARenderWindow::Impl {
public:
    Impl(const std::string& title, int width, int height, EGraphicsBackend backend)
        : viewport(width, height), backend_(backend) {
        window_ = std::make_unique<AWindowImplWin32>();
        const bool created = window_->create(title, width, height);
        assert(created && "Failed to create Win32 window");
        recreateRenderer(backend_);
    }

    ~Impl() {
        if (renderer_) {
            renderer_->shutdown();
        }
    }

    bool isOpen() const { return window_->isOpen(); }

    void close() { window_->close(); }

    std::vector<std::shared_ptr<AEvent>> pollEvents() {
        return window_->pollEvents();
    }

    void display() {
        if (!renderer_) {
            return;
        }

        viewport.setSize(window_->getWidth(), window_->getHeight());
        renderer_->setWorld(viewport.getWorld());
        renderer_->draw(viewport);
    }

    void setGraphicsBackend(EGraphicsBackend backend) {
        if (backend_ == backend) {
            return;
        }
        if (renderer_) {
            renderer_->shutdown();
            renderer_.reset();
        }
        backend_ = backend;
        recreateRenderer(backend_);
    }

    EGraphicsBackend getGraphicsBackend() const {
        return backend_;
    }

    AViewport& getViewport() { return viewport; }

private:
    void recreateRenderer(EGraphicsBackend backend) {
        switch (backend) {
        case EGraphicsBackend::OpenGL:
            renderer_ = std::make_unique<OpenGLRenderer>();
            break;
        case EGraphicsBackend::Vulkan:
            renderer_ = std::make_unique<VulkanRenderer>();
            break;
        default:
            renderer_ = std::make_unique<OpenGLRenderer>();
            backend_ = EGraphicsBackend::OpenGL;
            break;
        }

        renderer_->initialize(window_->getNativeHandle(), viewport.getWidth(), viewport.getHeight());
        renderer_->setWorld(viewport.getWorld());
    }

    std::unique_ptr<IWindowImpl> window_;
    std::unique_ptr<IRendererImpl> renderer_;
    AViewport viewport;
    EGraphicsBackend backend_;
};

ARenderWindow::ARenderWindow(const std::string& title, int width, int height, EGraphicsBackend backend)
    : impl_(std::make_unique<Impl>(title, width, height, backend)) {}

ARenderWindow::~ARenderWindow() = default;

bool ARenderWindow::isOpen() const { return impl_->isOpen(); }

void ARenderWindow::close() { impl_->close(); }

std::vector<std::shared_ptr<AEvent>> ARenderWindow::pollEvents() { return impl_->pollEvents(); }

void ARenderWindow::display() { impl_->display(); }

void ARenderWindow::setGraphicsBackend(EGraphicsBackend backend) { impl_->setGraphicsBackend(backend); }

EGraphicsBackend ARenderWindow::getGraphicsBackend() const { return impl_->getGraphicsBackend(); }

AViewport& ARenderWindow::getViewport() { return impl_->getViewport(); }
