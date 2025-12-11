#include "DirectX12Renderer.h"

#include <AEntity>
#include <AWorld>
#include <AViewport>
#include <ARenderOverlay>
#include <AText>
#include <AFloatingText>
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <limits>
#include <string>
#include <vector>

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

struct ResolvedText {
    std::string text;
    int x{0};
    int y{0};
    bool alignRight{false};
    int pixelHeight{16};
    AEntity::Color color{};
};

std::vector<ResolvedText> collectTexts(const AViewport& viewport) {
    std::vector<ResolvedText> out;

    auto appendScreenText = [&](const AText& text) {
        out.push_back(ResolvedText{
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
        if (!projectToScreen(text.getWorldPosition(), viewport.getViewMatrix(), viewport.getProjectionMatrix(), viewport.getWidth(), viewport.getHeight(), screen)) {
            return;
        }
        out.push_back(ResolvedText{
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

    return out;
}

void drawOverlayText(HDC dc, const AViewport& viewport) {
    if (!dc) {
        return;
    }
    const auto resolved = collectTexts(viewport);
    if (resolved.empty()) {
        return;
    }

    SetBkMode(dc, TRANSPARENT);

    for (const auto& overlay : resolved) {
        HFONT font = CreateFontA(
            -overlay.pixelHeight, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
            DEFAULT_PITCH | FF_DONTCARE, "Consolas");
        HFONT oldFont = nullptr;
        if (font) {
            oldFont = (HFONT)SelectObject(dc, font);
        }

        SetTextColor(dc, toColorRef(overlay.color));

        SIZE extent{};
        GetTextExtentPoint32A(dc, overlay.text.c_str(), static_cast<int>(overlay.text.size()), &extent);
        int x = overlay.alignRight ? (viewport.getWidth() - overlay.x - extent.cx) : overlay.x;
        TextOutA(dc, x, overlay.y, overlay.text.c_str(), static_cast<int>(overlay.text.size()));

        if (font) {
            SelectObject(dc, oldFont);
            DeleteObject(font);
        }
    }
}

} // namespace

DirectX12Renderer::DirectX12Renderer() = default;

DirectX12Renderer::~DirectX12Renderer() {
    shutdown();
}

bool DirectX12Renderer::initialize(void* nativeWindow, int width, int height) {
    hwnd_ = static_cast<HWND>(nativeWindow);
    width_ = width;
    height_ = height;
    ensureBackBuffer(width_, height_);
    return hwnd_ != nullptr;
}

void DirectX12Renderer::shutdown() {
    releaseBackBuffer();
    hwnd_ = nullptr;
}

void DirectX12Renderer::resize(int width, int height) {
    width_ = width;
    height_ = height;
    ensureBackBuffer(width_, height_);
}

void DirectX12Renderer::setWorld(AWorld* world) {
    world_ = world;
}

void DirectX12Renderer::draw(const AViewport& viewport) {
    if (!hwnd_) {
        return;
    }

    HDC hdc = GetDC(hwnd_);

    if (viewport.getWidth() != backBufferWidth_ || viewport.getHeight() != backBufferHeight_) {
        resize(viewport.getWidth(), viewport.getHeight());
    }

    if (viewport.getWidth() <= 0 || viewport.getHeight() <= 0) {
        ReleaseDC(hwnd_, hdc);
        return;
    }

    if (!backBufferDC_ || !backBufferBitmap_ || !colorBits_ || depthBuffer_.empty()) {
        ReleaseDC(hwnd_, hdc);
        return;
    }

    // Fast clear to opaque black (shared default across software backends).
    std::memset(colorBits_, 0, static_cast<size_t>(colorStride_) * static_cast<size_t>(backBufferHeight_));
    std::fill(depthBuffer_.begin(), depthBuffer_.end(), 1.0f);

    if (!world_) {
        BitBlt(hdc, 0, 0, backBufferWidth_, backBufferHeight_, backBufferDC_, 0, 0, SRCCOPY);
        ReleaseDC(hwnd_, hdc);
        return;
    }

    const glm::mat4 view = viewport.getViewMatrix();
    const glm::mat4 projection = viewport.getProjectionMatrix();

    struct ScreenVertex {
        glm::vec2 pos{};
        float depth01{1.0f};
        AEntity::Color color{};
    };

    struct ClipVertex {
        glm::vec4 pos{};
        AEntity::Color color{};
    };

    enum class ClipPlane {
        Left, Right, Bottom, Top, Near, Far
    };

    auto interpolateClip = [](const ClipVertex& a, const ClipVertex& b, float t) {
        ClipVertex out;
        out.pos = a.pos + t * (b.pos - a.pos);
        out.color.r = a.color.r + t * (b.color.r - a.color.r);
        out.color.g = a.color.g + t * (b.color.g - a.color.g);
        out.color.b = a.color.b + t * (b.color.b - a.color.b);
        out.color.a = a.color.a + t * (b.color.a - a.color.a);
        return out;
    };

    auto inside = [](const ClipVertex& v, ClipPlane plane) {
        switch (plane) {
        case ClipPlane::Left:   return v.pos.x >= -v.pos.w;
        case ClipPlane::Right:  return v.pos.x <=  v.pos.w;
        case ClipPlane::Bottom: return v.pos.y >= -v.pos.w;
        case ClipPlane::Top:    return v.pos.y <=  v.pos.w;
        case ClipPlane::Near:   return v.pos.z >= -v.pos.w;
        case ClipPlane::Far:    return v.pos.z <=  v.pos.w;
        default: return false;
        }
    };

    auto computeT = [](const glm::vec4& a, const glm::vec4& b, ClipPlane plane) -> float {
        auto safeDiv = [](float num, float den) {
            if (std::abs(den) < 1e-6f) {
                return 0.0f;
            }
            float t = num / den;
            return std::clamp(t, 0.0f, 1.0f);
        };
        switch (plane) {
        case ClipPlane::Left:   return safeDiv((-(a.w + a.x)), ((b.w - a.w) + (b.x - a.x)));
        case ClipPlane::Right:  return safeDiv((a.w - a.x),     ((b.w - a.w) - (b.x - a.x)));
        case ClipPlane::Bottom: return safeDiv((-(a.w + a.y)), ((b.w - a.w) + (b.y - a.y)));
        case ClipPlane::Top:    return safeDiv((a.w - a.y),     ((b.w - a.w) - (b.y - a.y)));
        case ClipPlane::Near:   return safeDiv((-(a.w + a.z)), ((b.w - a.w) + (b.z - a.z)));
        case ClipPlane::Far:    return safeDiv((a.w - a.z),     ((b.w - a.w) - (b.z - a.z)));
        default: return 0.0f;
        }
    };

    auto clipPolygon = [&](const std::vector<ClipVertex>& input, ClipPlane plane) {
        std::vector<ClipVertex> out;
        if (input.empty()) {
            return out;
        }
        const size_t count = input.size();
        for (size_t i = 0; i < count; ++i) {
            const ClipVertex& current = input[i];
            const ClipVertex& next = input[(i + 1) % count];
            const bool currentInside = inside(current, plane);
            const bool nextInside = inside(next, plane);

            if (currentInside && nextInside) {
                out.push_back(next);
            } else if (currentInside && !nextInside) {
                float t = computeT(current.pos, next.pos, plane);
                out.push_back(interpolateClip(current, next, t));
            } else if (!currentInside && nextInside) {
                float t = computeT(current.pos, next.pos, plane);
                out.push_back(interpolateClip(current, next, t));
                out.push_back(next);
            }
        }
        return out;
    };

    auto rasterizeTriangle = [&](const ScreenVertex& v0, const ScreenVertex& v1, const ScreenVertex& v2, bool interpolateColor, const AEntity::Color& uniformColor) {
        const float minXf = std::floor(std::min({v0.pos.x, v1.pos.x, v2.pos.x}));
        const float maxXf = std::ceil(std::max({v0.pos.x, v1.pos.x, v2.pos.x}));
        const float minYf = std::floor(std::min({v0.pos.y, v1.pos.y, v2.pos.y}));
        const float maxYf = std::ceil(std::max({v0.pos.y, v1.pos.y, v2.pos.y}));

        const int minX = static_cast<int>(std::clamp(minXf, 0.0f, static_cast<float>(backBufferWidth_ - 1)));
        const int maxX = static_cast<int>(std::clamp(maxXf, 0.0f, static_cast<float>(backBufferWidth_ - 1)));
        const int minY = static_cast<int>(std::clamp(minYf, 0.0f, static_cast<float>(backBufferHeight_ - 1)));
        const int maxY = static_cast<int>(std::clamp(maxYf, 0.0f, static_cast<float>(backBufferHeight_ - 1)));

        const glm::vec2 p0 = v0.pos;
        const glm::vec2 p1 = v1.pos;
        const glm::vec2 p2 = v2.pos;

        auto edge = [](const glm::vec2& a, const glm::vec2& b, float px, float py) {
            return (b.x - a.x) * (py - a.y) - (b.y - a.y) * (px - a.x);
        };

        const float area = edge(p0, p1, p2.x, p2.y);
        if (std::abs(area) < 1e-5f) {
            return;
        }
        const float invArea = 1.0f / area;

        for (int y = minY; y <= maxY; ++y) {
            for (int x = minX; x <= maxX; ++x) {
                const float px = static_cast<float>(x) + 0.5f;
                const float py = static_cast<float>(y) + 0.5f;

                float w0 = edge(p1, p2, px, py);
                float w1 = edge(p2, p0, px, py);
                float w2 = edge(p0, p1, px, py);

                if ((w0 >= 0 && w1 >= 0 && w2 >= 0 && area > 0) || (w0 <= 0 && w1 <= 0 && w2 <= 0 && area < 0)) {
                    w0 *= invArea;
                    w1 *= invArea;
                    w2 *= invArea;
                    const float depth = v0.depth01 * w0 + v1.depth01 * w1 + v2.depth01 * w2;

                    const size_t idx = static_cast<size_t>(y) * static_cast<size_t>(backBufferWidth_) + static_cast<size_t>(x);
                    if (depth < depthBuffer_[idx]) {
                        depthBuffer_[idx] = depth;

                        AEntity::Color c = uniformColor;
                        if (interpolateColor) {
                            c.r = v0.color.r * w0 + v1.color.r * w1 + v2.color.r * w2;
                            c.g = v0.color.g * w0 + v1.color.g * w1 + v2.color.g * w2;
                            c.b = v0.color.b * w0 + v1.color.b * w1 + v2.color.b * w2;
                            c.a = v0.color.a * w0 + v1.color.a * w1 + v2.color.a * w2;
                        }

                        uint8_t* pxPtr = colorBits_ + static_cast<size_t>(y) * static_cast<size_t>(colorStride_) + static_cast<size_t>(x) * 4;
                        pxPtr[0] = static_cast<uint8_t>(std::clamp(c.b, 0.0f, 1.0f) * 255.0f);
                        pxPtr[1] = static_cast<uint8_t>(std::clamp(c.g, 0.0f, 1.0f) * 255.0f);
                        pxPtr[2] = static_cast<uint8_t>(std::clamp(c.r, 0.0f, 1.0f) * 255.0f);
                        pxPtr[3] = 255;
                    }
                }
            }
        }
    };

    for (const auto& entityPtr : world_->getEntities()) {
        const auto& vertices = entityPtr->getVertices();
        if (vertices.size() < 3) {
            continue;
        }

        const glm::mat4 model = glm::translate(glm::mat4(1.0f), entityPtr->getPosition());
        const glm::mat4 mvp = projection * view * model;

        const auto& vertexColors = entityPtr->getVertexColors();
        const bool hasPerVertexColors = vertexColors.size() == vertices.size();

        std::vector<ClipVertex> clipVertices;
        clipVertices.reserve(vertices.size());
        for (size_t i = 0; i < vertices.size(); ++i) {
            glm::vec4 clip = mvp * glm::vec4(vertices[i], 1.0f);
            ClipVertex cv;
            cv.pos = clip;
            cv.color = hasPerVertexColors ? vertexColors[i] : entityPtr->getColor();
            clipVertices.push_back(cv);
        }

        if (clipVertices.size() < 3) {
            continue;
        }

        std::array<ClipPlane, 6> planes = {
            ClipPlane::Left, ClipPlane::Right,
            ClipPlane::Bottom, ClipPlane::Top,
            ClipPlane::Near, ClipPlane::Far
        };
        for (ClipPlane plane : planes) {
            clipVertices = clipPolygon(clipVertices, plane);
            if (clipVertices.size() < 3) {
                break;
            }
        }
        if (clipVertices.size() < 3) {
            continue;
        }

        std::vector<ScreenVertex> transformed;
        transformed.reserve(clipVertices.size());
        for (const auto& cv : clipVertices) {
            glm::vec3 ndc = glm::vec3(cv.pos) / cv.pos.w;
            ScreenVertex sv;
            sv.pos.x = (ndc.x * 0.5f + 0.5f) * static_cast<float>(viewport.getWidth());
            sv.pos.y = (1.0f - (ndc.y * 0.5f + 0.5f)) * static_cast<float>(viewport.getHeight());
            sv.depth01 = ndc.z * 0.5f + 0.5f;
            sv.color = cv.color;
            transformed.push_back(sv);
        }

        if (transformed.size() < 3) {
            continue;
        }

        const bool interpolateColor = hasPerVertexColors;
        const AEntity::Color uniform = entityPtr->getColor();

        auto emitTriangle = [&](size_t i0, size_t i1, size_t i2) {
            rasterizeTriangle(transformed[i0], transformed[i1], transformed[i2], interpolateColor, uniform);
        };

        if (transformed.size() == 3) {
            emitTriangle(0, 1, 2);
        } else if (transformed.size() == 4) {
            emitTriangle(0, 1, 2);
            emitTriangle(0, 2, 3);
        } else {
            for (size_t i = 1; i + 1 < transformed.size(); ++i) {
                emitTriangle(0, i, i + 1);
            }
        }
    }

    drawOverlayText(backBufferDC_, viewport);

    BitBlt(hdc, 0, 0, backBufferWidth_, backBufferHeight_, backBufferDC_, 0, 0, SRCCOPY);
    ReleaseDC(hwnd_, hdc);
}

void DirectX12Renderer::ensureBackBuffer(int width, int height) {
    if (!hwnd_) {
        return;
    }
    if (width <= 0 || height <= 0) {
        releaseBackBuffer();
        return;
    }
    if (backBufferBitmap_ && width == backBufferWidth_ && height == backBufferHeight_) {
        return;
    }

    releaseBackBuffer();

    HDC windowDC = GetDC(hwnd_);
    backBufferDC_ = CreateCompatibleDC(windowDC);

    BITMAPINFO bmi{};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = width;
    bmi.bmiHeader.biHeight = -height;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    backBufferBitmap_ = CreateDIBSection(backBufferDC_, &bmi, DIB_RGB_COLORS, reinterpret_cast<void**>(&colorBits_), nullptr, 0);
    if (!backBufferBitmap_) {
        DeleteDC(backBufferDC_);
        backBufferDC_ = nullptr;
        colorBits_ = nullptr;
        backBufferWidth_ = backBufferHeight_ = 0;
        depthBuffer_.clear();
        ReleaseDC(hwnd_, windowDC);
        return;
    }

    backBufferOldBitmap_ = (HBITMAP)SelectObject(backBufferDC_, backBufferBitmap_);
    colorStride_ = width * 4;

    backBufferWidth_ = width;
    backBufferHeight_ = height;
    depthBuffer_.assign(static_cast<size_t>(width) * static_cast<size_t>(height), 1.0f);

    ReleaseDC(hwnd_, windowDC);
}

void DirectX12Renderer::releaseBackBuffer() {
    if (backBufferDC_) {
        if (backBufferOldBitmap_) {
            SelectObject(backBufferDC_, backBufferOldBitmap_);
            backBufferOldBitmap_ = nullptr;
        }
        if (backBufferBitmap_) {
            DeleteObject(backBufferBitmap_);
            backBufferBitmap_ = nullptr;
        }
        DeleteDC(backBufferDC_);
        backBufferDC_ = nullptr;
    }
    colorBits_ = nullptr;
    colorStride_ = 0;
    backBufferWidth_ = 0;
    backBufferHeight_ = 0;
    depthBuffer_.clear();
}
