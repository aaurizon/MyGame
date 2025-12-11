#include <AViewport>
#include <AWorld>

AViewport::AViewport(int width, int height) : width_(width), height_(height) {}

void AViewport::setWorld(AWorld& world) {
    world_ = &world;
}

AWorld* AViewport::getWorld() const {
    return world_;
}

void AViewport::setSize(int width, int height) {
    width_ = width;
    height_ = height;
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
