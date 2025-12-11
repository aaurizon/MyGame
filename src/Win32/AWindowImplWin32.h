#pragma once

#include <IWindowImpl.h>
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#include <glm/vec2.hpp>
#include <memory>
#include <string>
#include <vector>

class AWindowImplWin32 : public IWindowImpl {
public:
    AWindowImplWin32();
    ~AWindowImplWin32() override;

    bool create(const std::string& title, int width, int height) override;
    void close() override;
    bool isOpen() const override;

    std::vector<std::shared_ptr<AEvent>> pollEvents() override;

    void* getNativeHandle() const override;
    int getWidth() const override;
    int getHeight() const override;
    void setTitle(const std::string& title) override;

private:
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    void pushEvent(const std::shared_ptr<AEvent>& event);
    void handleSizeChange(LPARAM lParam);

    HWND hwnd_{nullptr};
    bool open_{false};
    int width_{0};
    int height_{0};
    glm::vec2 lastMouse_{0.0f, 0.0f};
    std::vector<std::shared_ptr<AEvent>> events_;
};
