#include <Overlay>

AText& Overlay::addText(const AText& text) {
    texts_.push_back(text);
    return texts_.back();
}

AFloatingText& Overlay::addFloatingText(const AFloatingText& text) {
    floatingTexts_.push_back(text);
    return floatingTexts_.back();
}

std::vector<AText>& Overlay::getTexts() {
    return texts_;
}

const std::vector<AText>& Overlay::getTexts() const {
    return texts_;
}

std::vector<AFloatingText>& Overlay::getFloatingTexts() {
    return floatingTexts_;
}

const std::vector<AFloatingText>& Overlay::getFloatingTexts() const {
    return floatingTexts_;
}
