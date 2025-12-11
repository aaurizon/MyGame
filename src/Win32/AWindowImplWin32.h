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

    bool create(const std::string& title, int width, int height, void* parentHandle, int x, int y, bool child) override;
    void close() override;
    bool isOpen() const override;

    std::vector<std::shared_ptr<AEvent>> pollEvents() override;

    void* getNativeHandle() const override;
    int getWidth() const override;
    int getHeight() const override;
    void setTitle(const std::string& title) override;
    void setRect(int x, int y, int width, int height) override;
    void setCursorGrabbed(bool grabbed) override;
    bool isCursorGrabbed() const override;

private:
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    void pushEvent(const std::shared_ptr<AEvent>& event);
    void handleSizeChange(LPARAM lParam);
    void centerCursor();

    HWND hwnd_{nullptr};
    bool child_{false};
    bool open_{false};
    bool cursorGrabbed_{false};
    int width_{0};
    int height_{0};
    glm::vec2 lastMouse_{0.0f, 0.0f};
    std::vector<std::shared_ptr<AEvent>> events_;
};
