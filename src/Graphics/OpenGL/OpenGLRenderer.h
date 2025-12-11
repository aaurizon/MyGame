#pragma once

#include <Graphics/IRendererImpl.h>
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#include <gl/GL.h>
#include <AWorld>
#include <vector>
#include <string>

class OpenGLRenderer : public IRendererImpl {
public:
    OpenGLRenderer();
    ~OpenGLRenderer() override;

    bool initialize(void* nativeWindow, int width, int height) override;
    void shutdown() override;
    void resize(int width, int height) override;
    void draw(const AViewport& viewport) override;
    void setWorld(AWorld* world) override;

private:
    void setupContext(HWND hwnd);
    void setupState();
    void drawEntity(const AEntity& entity, const glm::mat4& view, const glm::mat4& projection);
    void drawOverlayText(const AViewport& viewport);
    GLuint getFontBase(int pixelHeight, HFONT& outFont);
    int measureTextWidth(const std::string& text, HFONT font) const;

    HWND hwnd_{nullptr};
    HDC hdc_{nullptr};
    HGLRC hglrc_{nullptr};
    int width_{0};
    int height_{0};
    AWorld* world_{nullptr};

    struct FontEntry {
        int size{0};
        GLuint base{0};
        HFONT font{nullptr};
    };
    std::vector<FontEntry> fontCache_;
};
