#include <AWorld>

void AWorld::addEntity(AEntity* entity) {
    if (!entity) {
        return;
    }
    entities_.emplace_back(entity);
}

const std::vector<std::unique_ptr<AEntity>>& AWorld::getEntities() const {
    return entities_;
}
