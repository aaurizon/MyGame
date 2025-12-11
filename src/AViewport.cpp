#include <AViewport>
#include <AWorld>
#include <algorithm>

AViewport::AViewport(int width, int height, int x, int y)
    : x_(x), y_(y), width_(width), height_(height) {}

void AViewport::setWorld(AWorld& world) {
    world_ = &world;
}

AWorld* AViewport::getWorld() const {
    return world_;
}

void AViewport::setRect(int x, int y, int width, int height) {
    x_ = x;
    y_ = y;
    width_ = width;
    height_ = height;
}

void AViewport::setSize(int width, int height) {
    width_ = width;
    height_ = height;
}

void AViewport::setPosition(int x, int y) {
    x_ = x;
    y_ = y;
}

int AViewport::getX() const {
    return x_;
}

int AViewport::getY() const {
    return y_;
}

int AViewport::getWidth() const {
    return width_;
}

int AViewport::getHeight() const {
    return height_;
}

float AViewport::getAspectRatio() const {
    if (height_ == 0) {
        return 1.0f;
    }
    return static_cast<float>(width_) / static_cast<float>(height_);
}

void AViewport::setViewMatrix(const glm::mat4& view) {
    view_ = view;
}

void AViewport::setProjectionMatrix(const glm::mat4& projection) {
    projection_ = projection;
}

const glm::mat4& AViewport::getViewMatrix() const {
    return view_;
}

const glm::mat4& AViewport::getProjectionMatrix() const {
    return projection_;
}

void AViewport::addOverlay(Overlay& overlay) {
    overlays_.push_back(&overlay);
}

void AViewport::removeOverlay(Overlay& overlay) {
    overlays_.erase(std::remove(overlays_.begin(), overlays_.end(), &overlay), overlays_.end());
}

void AViewport::clearOverlays() {
    overlays_.clear();
}

const std::vector<Overlay*>& AViewport::getOverlays() const {
    return overlays_;
}
