#include <ARenderOverlay>

AText& ARenderOverlay::addText(const AText& text) {
    texts_.push_back(text);
    return texts_.back();
}

AFloatingText& ARenderOverlay::addFloatingText(const AFloatingText& text) {
    floatingTexts_.push_back(text);
    return floatingTexts_.back();
}

std::vector<AText>& ARenderOverlay::getTexts() {
    return texts_;
}

const std::vector<AText>& ARenderOverlay::getTexts() const {
    return texts_;
}

std::vector<AFloatingText>& ARenderOverlay::getFloatingTexts() {
    return floatingTexts_;
}

const std::vector<AFloatingText>& ARenderOverlay::getFloatingTexts() const {
    return floatingTexts_;
}
