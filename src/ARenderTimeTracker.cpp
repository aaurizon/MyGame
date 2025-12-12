#include <ARenderTimeTracker>

#include <cstdio>
#include <AWindow>

namespace {

const char* backendName(EGraphicsBackend backend) {
    switch (backend) {
    case EGraphicsBackend::OpenGL: return "OpenGL";
    case EGraphicsBackend::Vulkan: return "Vulkan";
    case EGraphicsBackend::DirectX11: return "DX11";
    case EGraphicsBackend::DirectX12: return "DX12";
    default: return "None";
    }
}

} // namespace

ARenderTimeTracker::ARenderTimeTracker(std::chrono::milliseconds reportInterval)
    : m_lastReport(std::chrono::steady_clock::now())
    , m_reportInterval(reportInterval) {}

void ARenderTimeTracker::trackDraw(AWindow& window) {
    static ARenderTimeTracker s_defaultTracker{};
    s_defaultTracker.timeDraw(window);
}

void ARenderTimeTracker::timeDraw(AWindow& window) {
    const EGraphicsBackend backend = window.getGraphicsBackend();
    const auto start = std::chrono::steady_clock::now();
    window.display();
    const auto end = std::chrono::steady_clock::now();
    const double elapsedMs =
        std::chrono::duration<double, std::milli>(end - start).count();
    record(backend, elapsedMs);
}

void ARenderTimeTracker::record(EGraphicsBackend backend, double elapsedMilliseconds) {
    if (backend == EGraphicsBackend::None) {
        return;
    }

    const size_t idx = static_cast<size_t>(backend);
    m_stats[idx].accumulatedMs += elapsedMilliseconds;
    m_stats[idx].samples++;

    const auto now = std::chrono::steady_clock::now();
    if (now - m_lastReport >= m_reportInterval) {
        reportAndReset();
        m_lastReport = now;
    }
}

void ARenderTimeTracker::reportAndReset() {
    double totalMs = 0.0;
    for (const auto& stat : m_stats) {
        totalMs += stat.accumulatedMs;
    }
    if (totalMs <= 0.0) {
        m_stats.fill({});
        return;
    }

    std::printf("[Render CPU] ");
    bool first = true;
    for (size_t i = 0; i < m_stats.size(); ++i) {
        const auto& stat = m_stats[i];
        if (stat.samples == 0) {
            continue;
        }
        const double percent = (stat.accumulatedMs / totalMs) * 100.0;
        const double averageMs = stat.accumulatedMs / static_cast<double>(stat.samples);
        std::printf("%s%s: %.1f%% (avg %.3f ms)",
                    first ? "" : " | ",
                    backendName(static_cast<EGraphicsBackend>(i)),
                    percent,
                    averageMs);
        first = false;
    }
    std::printf("\n");
    m_stats.fill({});
}
