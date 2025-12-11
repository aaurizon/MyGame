#include <AWindow>

#include <IWindowImpl.h>
#include <Win32/AWindowImplWin32.h>
#include <cassert>

AWindow::AWindow(const std::string& title, int width, int height)
    : AWindow(title, width, height, nullptr, 0, 0, false) {}

AWindow::AWindow(const std::string& title, int width, int height, void* parentHandle, int x, int y, bool child) {
    impl_ = std::make_unique<AWindowImplWin32>();
    const bool created = impl_->create(title, width, height, parentHandle, x, y, child);
    assert(created && "Failed to create Win32 window");
}

AWindow::~AWindow() = default;

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
}
