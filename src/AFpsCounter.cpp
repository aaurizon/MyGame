#include <AFpsCounter>

AFpsCounter::AFpsCounter() : lastTime_(Clock::now()) {}

float AFpsCounter::tick() {
    const auto now = Clock::now();
    const float delta = std::chrono::duration<float>(now - lastTime_).count();
    lastTime_ = now;

    accumulated_ += delta;
    ++frames_;
    if (accumulated_ >= 1.0f) {
        fps_ = static_cast<float>(frames_) / accumulated_;
        accumulated_ = 0.0f;
        frames_ = 0;
    }

    return delta;
}

float AFpsCounter::getFps() const {
    return fps_;
}

void AFpsCounter::reset() {
    lastTime_ = Clock::now();
    accumulated_ = 0.0f;
    frames_ = 0;
    fps_ = 0.0f;
}
