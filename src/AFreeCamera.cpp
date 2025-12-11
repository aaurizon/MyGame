#include <AFreeCamera>

#include <glm/gtc/matrix_transform.hpp>
#include <cmath>
#include <iostream>

AFreeCamera::AFreeCamera(AViewport& viewport, const glm::vec3& position, const glm::vec3& lookAt)
    : position_(position) {
    viewports_.push_back(&viewport);
    forward_ = glm::normalize(lookAt - position_);
    yaw_ = std::atan2(forward_.y, forward_.x);
    pitch_ = std::asin(forward_.z);
    updateMatrices();
}

void AFreeCamera::dispatchEvent(const std::shared_ptr<AEvent>& event) {
    if (!event) {
        return;
    }

    if (!inputEnabled_) {
        return;
    }

    if (event->is<AEvent::KeyPressed>()) {
        handleKeyPressed(event->getIf<AEvent::KeyPressed>()->scancode);
    } else if (const auto* moveEvent = event->getIf<AEvent::MouseMoved>()) {
        handleMouseMoved(moveEvent);
    }
}

void AFreeCamera::addViewport(AViewport& viewport) {
    viewports_.push_back(&viewport);
    updateMatrices();
}

void AFreeCamera::refreshMatrices() {
    updateMatrices();
}

const glm::vec3& AFreeCamera::getPosition() const {
    return position_;
}

const glm::vec3& AFreeCamera::getForward() const {
    return forward_;
}

void AFreeCamera::updateMatrices() {
    forward_.x = std::cos(pitch_) * std::cos(yaw_);
    forward_.y = std::cos(pitch_) * std::sin(yaw_);
    forward_.z = std::sin(pitch_);
    forward_ = glm::normalize(forward_);

    const glm::vec3 focus = position_ + forward_;
    glm::mat4 view = glm::lookAt(position_, focus, up_);

    for (auto* viewport : viewports_) {
        if (!viewport) {
            continue;
        }
        const float aspect = viewport->getAspectRatio();
        glm::mat4 projection = glm::perspective(glm::radians(60.0f), aspect, 0.1f, 1000.0f);
        viewport->setViewMatrix(view);
        viewport->setProjectionMatrix(projection);
    }

    // Simple runtime feedback to verify camera controls.
    const float yawDeg = glm::degrees(yaw_);
    const float pitchDeg = glm::degrees(pitch_);
    std::cout << "[Camera] pos(" << position_.x << ", " << position_.y << ", " << position_.z
              << ") forward(" << forward_.x << ", " << forward_.y << ", " << forward_.z
              << ") yaw " << yawDeg << " deg"
              << " pitch " << pitchDeg << " deg" << std::endl;
}

void AFreeCamera::handleKeyPressed(EEventKey::Scancode code) {
    glm::vec3 right = glm::normalize(glm::cross(forward_, up_));
    switch (code) {
    case EEventKey::Scancode::W:
    case EEventKey::Scancode::Up:
        position_ += forward_ * moveStep_;
        break;
    case EEventKey::Scancode::S:
    case EEventKey::Scancode::Down:
        position_ -= forward_ * moveStep_;
        break;
    case EEventKey::Scancode::A:
    case EEventKey::Scancode::Left:
        position_ -= right * moveStep_;
        break;
    case EEventKey::Scancode::D:
    case EEventKey::Scancode::Right:
        position_ += right * moveStep_;
        break;
    case EEventKey::Scancode::R:
        position_.z += moveStep_;
        break;
    case EEventKey::Scancode::F:
        position_.z -= moveStep_;
        break;
    default:
        break;
    }

    updateMatrices();
}

void AFreeCamera::handleMouseMoved(const AEvent::MouseMoved* moveEvent) {
    // Mouse right turns camera right (positive yaw), mouse up pitches up.
    yaw_ -= moveEvent->delta.x * mouseSensitivity_;
    pitch_ -= moveEvent->delta.y * mouseSensitivity_;

    const float maxPitch = glm::radians(89.0f);
    if (pitch_ > maxPitch) pitch_ = maxPitch;
    if (pitch_ < -maxPitch) pitch_ = -maxPitch;

    updateMatrices();
}
