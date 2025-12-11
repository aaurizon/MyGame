#include <AWorld>
#include <AFloatingText>

void AWorld::addEntity(AEntity* entity) {
    if (!entity) {
        return;
    }
    entities_.emplace_back(entity);
}

const std::vector<std::unique_ptr<AEntity>>& AWorld::getEntities() const {
    return entities_;
}

void AWorld::addFloatingText(AFloatingText* text) {
    if (!text) {
        return;
    }
    floatingTexts_.emplace_back(text);
}

const std::vector<std::unique_ptr<AFloatingText>>& AWorld::getFloatingTexts() const {
    return floatingTexts_;
}
