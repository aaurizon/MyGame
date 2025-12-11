#include <AEntity>

#include <glm/gtc/matrix_transform.hpp>

AEntity::AEntity(const std::vector<glm::vec3>& vertices) : vertices_(vertices) {}

AEntity* AEntity::createTriangle(const glm::vec3& a, const glm::vec3& b, const glm::vec3& c) {
    return new AEntity({a, b, c});
}

AEntity* AEntity::createRectangle(float width, float height) {
    const float hw = width * 0.5f;
    const float hh = height * 0.5f;
    // Rectangle on X-Y plane centered on origin at Z = 0.
    std::vector<glm::vec3> verts = {
        {-hw, -hh, 0.0f},
        { hw, -hh, 0.0f},
        { hw,  hh, 0.0f},
        {-hw,  hh, 0.0f},
    };
    return new AEntity(verts);
}

const std::vector<glm::vec3>& AEntity::getVertices() const {
    return vertices_;
}

const AEntity::Color& AEntity::getColor() const {
    return color_;
}

void AEntity::setPosition(const glm::vec3& pos) {
    position_ = pos;
}

const glm::vec3& AEntity::getPosition() const {
    return position_;
}

void AEntity::setColor(const Color& color) {
    color_ = color;
}
