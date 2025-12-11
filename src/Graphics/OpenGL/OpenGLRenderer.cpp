#include "OpenGLRenderer.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <algorithm>
#include <cassert>
#include <string>
#include <vector>

namespace {

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
    if (hglrc_ && !fontCache_.empty()) {
        wglMakeCurrent(hdc_, hglrc_);
        for (const auto& entry : fontCache_) {
            if (entry.base != 0) {
                glDeleteLists(entry.base, 256);
            }
            if (entry.font) {
                DeleteObject(entry.font);
            }
        }
        fontCache_.clear();
    }

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

    if (world_) {
        const glm::mat4& view = viewport.getViewMatrix();
        const glm::mat4& projection = viewport.getProjectionMatrix();

        for (const auto& entityPtr : world_->getEntities()) {
            drawEntity(*entityPtr, view, projection);
        }
    }

    // Draw overlay into the back buffer using OpenGL so it is stable across swaps.
    drawOverlayText(viewport);
    SwapBuffers(hdc_);
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

GLuint OpenGLRenderer::getFontBase(int pixelHeight, HFONT& outFont) {
    for (const auto& entry : fontCache_) {
        if (entry.size == pixelHeight && entry.base != 0 && entry.font != nullptr) {
            outFont = entry.font;
            return entry.base;
        }
    }

    HFONT font = CreateFontA(
        -pixelHeight, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
        DEFAULT_PITCH | FF_DONTCARE, "Consolas");
    if (!font) {
        return 0;
    }

    HGDIOBJ old = SelectObject(hdc_, font);
    GLuint base = glGenLists(256);
    if (base != 0) {
        wglUseFontBitmapsA(hdc_, 0, 256, base);
        fontCache_.push_back(FontEntry{pixelHeight, base, font});
    } else {
        DeleteObject(font);
        font = nullptr;
    }
    if (old) {
        SelectObject(hdc_, old);
    }
    outFont = font;
    return base;
}

int OpenGLRenderer::measureTextWidth(const std::string& text, HFONT font) const {
    if (!font || !hdc_) {
        return 0;
    }
    HGDIOBJ old = SelectObject(hdc_, font);
    SIZE extent{};
    GetTextExtentPoint32A(hdc_, text.c_str(), static_cast<int>(text.size()), &extent);
    if (old) {
        SelectObject(hdc_, old);
    }
    return static_cast<int>(extent.cx);
}

void OpenGLRenderer::drawOverlayText(const AViewport& viewport) {
    if (!hdc_ || !hglrc_) {
        return;
    }

    struct ResolvedText {
        std::string text;
        int x{0};
        int y{0};
        bool alignRight{false};
        int pixelHeight{16};
        AEntity::Color color{};
    };

    std::vector<ResolvedText> resolved;

    auto appendScreenText = [&](const AText& text) {
        resolved.push_back(ResolvedText{
            text.getText(),
            text.getPosition().x,
            text.getPosition().y,
            text.isAlignRight(),
            text.getPixelHeight(),
            text.getColor()
        });
    };

    auto appendFloatingText = [&](const AFloatingText& text) {
        POINT screen{};
        if (!projectToScreen(text.getWorldPosition(), viewport.getViewMatrix(), viewport.getProjectionMatrix(), width_, height_, screen)) {
            return;
        }
        resolved.push_back(ResolvedText{
            text.getText(),
            screen.x,
            screen.y,
            false,
            text.getPixelHeight(),
            text.getColor()
        });
    };

    for (const auto* overlay : viewport.getOverlays()) {
        if (!overlay) {
            continue;
        }
        for (const auto& text : overlay->getTexts()) {
            appendScreenText(text);
        }
        for (const auto& floating : overlay->getFloatingTexts()) {
            appendFloatingText(floating);
        }
    }

    if (auto* world = viewport.getWorld()) {
        for (const auto& floating : world->getFloatingTexts()) {
            appendFloatingText(*floating);
        }
    }

    if (resolved.empty()) {
        return;
    }

    glPushAttrib(GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0, width_, height_, 0, -1, 1);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    for (const auto& overlay : resolved) {
        HFONT fontHandle = nullptr;
        GLuint base = getFontBase(overlay.pixelHeight, fontHandle);
        if (base == 0 || !fontHandle) {
            continue;
        }

        int textWidth = measureTextWidth(overlay.text, fontHandle);
        int x = overlay.alignRight ? (width_ - overlay.x - textWidth) : overlay.x;
        int y = overlay.y + overlay.pixelHeight; // baseline adjustment for top-left origin

        glColor4f(overlay.color.r, overlay.color.g, overlay.color.b, overlay.color.a);
        glRasterPos2i(x, y);
        glListBase(base);
        glCallLists(static_cast<GLsizei>(overlay.text.size()), GL_UNSIGNED_BYTE, overlay.text.c_str());
    }

    glPopMatrix(); // modelview
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopAttrib();
}
