#include "OpenGLRenderer.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <algorithm>
#include <cassert>
#include <string>

namespace {

COLORREF toColorRef(const AEntity::Color& c) {
    return RGB(
        static_cast<BYTE>(std::clamp(c.r, 0.0f, 1.0f) * 255.0f),
        static_cast<BYTE>(std::clamp(c.g, 0.0f, 1.0f) * 255.0f),
        static_cast<BYTE>(std::clamp(c.b, 0.0f, 1.0f) * 255.0f));
}

bool projectToScreen(const glm::vec3& world, const glm::mat4& view, const glm::mat4& projection, int width, int height, POINT& out) {
    glm::vec4 clip = projection * view * glm::vec4(world, 1.0f);
    if (clip.w <= 0.0f) {
        return false;
    }
    glm::vec3 ndc = glm::vec3(clip) / clip.w;
    if (ndc.z < -1.0f || ndc.z > 1.0f) {
        return false;
    }
    out.x = static_cast<int>((ndc.x * 0.5f + 0.5f) * static_cast<float>(width));
    out.y = static_cast<int>((1.0f - (ndc.y * 0.5f + 0.5f)) * static_cast<float>(height));
    return true;
}

} // namespace

OpenGLRenderer::OpenGLRenderer() = default;

OpenGLRenderer::~OpenGLRenderer() {
    shutdown();
}

bool OpenGLRenderer::initialize(void* nativeWindow, int width, int height) {
    hwnd_ = static_cast<HWND>(nativeWindow);
    width_ = width;
    height_ = height;

    setupContext(hwnd_);
    setupState();
    return true;
}

void OpenGLRenderer::shutdown() {
    if (hglrc_) {
        wglMakeCurrent(nullptr, nullptr);
        wglDeleteContext(hglrc_);
        hglrc_ = nullptr;
    }
    if (hdc_ && hwnd_) {
        ReleaseDC(hwnd_, hdc_);
        hdc_ = nullptr;
    }
    hwnd_ = nullptr;
}

void OpenGLRenderer::resize(int width, int height) {
    width_ = width;
    height_ = height;
    if (hdc_ && hglrc_) {
        wglMakeCurrent(hdc_, hglrc_);
        glViewport(0, 0, width_, height_);
    }
}

void OpenGLRenderer::draw(const AViewport& viewport) {
    if (!hdc_ || !hglrc_) {
        return;
    }

    if (width_ != viewport.getWidth() || height_ != viewport.getHeight()) {
        resize(viewport.getWidth(), viewport.getHeight());
    }

    wglMakeCurrent(hdc_, hglrc_);
    glViewport(0, 0, width_, height_);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (!world_) {
        SwapBuffers(hdc_);
        return;
    }

    const glm::mat4& view = viewport.getViewMatrix();
    const glm::mat4& projection = viewport.getProjectionMatrix();

    for (const auto& entityPtr : world_->getEntities()) {
        drawEntity(*entityPtr, view, projection);
    }

    SwapBuffers(hdc_);
    drawOverlayText(viewport);
}

void OpenGLRenderer::setWorld(AWorld* world) {
    world_ = world;
}

void OpenGLRenderer::setupContext(HWND hwnd) {
    hdc_ = GetDC(hwnd);
    PIXELFORMATDESCRIPTOR pfd{};
    pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 32;
    pfd.cDepthBits = 24;
    pfd.cStencilBits = 8;
    pfd.iLayerType = PFD_MAIN_PLANE;

    int pixelFormat = GetPixelFormat(hdc_);
    if (pixelFormat == 0) {
        pixelFormat = ChoosePixelFormat(hdc_, &pfd);
        SetPixelFormat(hdc_, pixelFormat, &pfd);
    }

    hglrc_ = wglCreateContext(hdc_);
    wglMakeCurrent(hdc_, hglrc_);
}

void OpenGLRenderer::setupState() {
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glClearDepth(1.0);
    // Shared default clear color across backends: opaque black for consistency and speed.
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
}

void OpenGLRenderer::drawEntity(const AEntity& entity, const glm::mat4& view, const glm::mat4& projection) {
    const auto& vertices = entity.getVertices();
    if (vertices.empty()) {
        return;
    }
    const auto& vertexColors = entity.getVertexColors();
    const bool hasPerVertexColors = vertexColors.size() == vertices.size();

    const glm::mat4 model = glm::translate(glm::mat4(1.0f), entity.getPosition());

    glMatrixMode(GL_PROJECTION);
    glLoadMatrixf(glm::value_ptr(projection));

    glMatrixMode(GL_MODELVIEW);
    const glm::mat4 viewModel = view * model;
    glLoadMatrixf(glm::value_ptr(viewModel));

    const auto color = entity.getColor();

    GLenum primitive = vertices.size() == 3 ? GL_TRIANGLES : GL_TRIANGLE_FAN;
    glBegin(primitive);
    for (size_t i = 0; i < vertices.size(); ++i) {
        if (hasPerVertexColors) {
            const auto& c = vertexColors[i];
            glColor4f(c.r, c.g, c.b, c.a);
        } else {
            glColor4f(color.r, color.g, color.b, color.a);
        }
        const auto& v = vertices[i];
        glVertex3f(v.x, v.y, v.z);
    }
    glEnd();
}

void OpenGLRenderer::drawOverlayText(const AViewport& viewport) {
    if (!hdc_) {
        return;
    }

    const auto& overlays = viewport.getOverlayTexts();
    if (overlays.empty()) {
        return;
    }

    SetBkMode(hdc_, TRANSPARENT);

    for (const auto& overlay : overlays) {
        POINT screen{};
        if (overlay.screenSpace) {
            screen.x = overlay.screenPosition.x;
            screen.y = overlay.screenPosition.y;
        } else if (!projectToScreen(overlay.worldPosition, viewport.getViewMatrix(), viewport.getProjectionMatrix(), width_, height_, screen)) {
            continue;
        }

        HFONT font = CreateFontA(
            -overlay.pixelHeight, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
            DEFAULT_PITCH | FF_DONTCARE, "Consolas");
        HFONT oldFont = nullptr;
        if (font) {
            oldFont = (HFONT)SelectObject(hdc_, font);
        }

        SetTextColor(hdc_, toColorRef(overlay.color));

        SIZE extent{};
        GetTextExtentPoint32A(hdc_, overlay.text.c_str(), static_cast<int>(overlay.text.size()), &extent);
        int x = screen.x;
        if (overlay.alignRight) {
            x -= extent.cx;
        }
        TextOutA(hdc_, x, screen.y, overlay.text.c_str(), static_cast<int>(overlay.text.size()));

        if (font) {
            SelectObject(hdc_, oldFont);
            DeleteObject(font);
        }
    }
}
