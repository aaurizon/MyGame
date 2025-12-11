#include <ARenderOverlay>

AText& ARenderOverlay::addText(const AText& text) {
    texts_.push_back(std::make_unique<AText>(text));
    return *texts_.back();
}

AFloatingText& ARenderOverlay::addFloatingText(const AFloatingText& text) {
    floatingTexts_.push_back(std::make_unique<AFloatingText>(text));
    return *floatingTexts_.back();
}

std::vector<std::unique_ptr<AText>>& ARenderOverlay::getTexts() {
    return texts_;
}

const std::vector<std::unique_ptr<AText>>& ARenderOverlay::getTexts() const {
    return texts_;
}

std::vector<std::unique_ptr<AFloatingText>>& ARenderOverlay::getFloatingTexts() {
    return floatingTexts_;
}

const std::vector<std::unique_ptr<AFloatingText>>& ARenderOverlay::getFloatingTexts() const {
    return floatingTexts_;
}
