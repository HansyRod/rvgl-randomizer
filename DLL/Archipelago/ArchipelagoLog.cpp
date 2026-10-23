#include "ArchipelagoLog.h"
#include "ArchipelagoLogBuffer.h"
#include "Logger.h"
#include "RVGLFunctions.h"

#include <windows.h>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <mutex>
#include <string>

namespace Randomizer {
namespace {

struct ArchipelagoLogState {
    std::mutex mutex;
    ArchipelagoLogBuffer buffer;
    uint64_t lastMessageMs = 0;
};

ArchipelagoLogState& GetArchipelagoLogState() {
    static ArchipelagoLogState state;
    return state;
}

} // anonymous namespace

void LogArchipelagoMessage(const char* message) {
    if (message == nullptr) {
        return;
    }

    // Tag the base log entry while keeping the in-game message as supplied.
    const std::string fileMessage = std::string("[Archipelago] ") + message;
    Logger::TimestampLog(fileMessage.c_str());

    ArchipelagoLogState& state = GetArchipelagoLogState();
    std::lock_guard<std::mutex> lock(state.mutex);
    if (state.buffer.Add(message)) {
        state.lastMessageMs = GetTickCount64();
    }
}

void LogArchipelagoMessagef(const char* format, ...) {
    if (format == nullptr) {
        return;
    }

    char buffer[512]{};
    va_list args;
    va_start(args, format);
    std::vsnprintf(buffer, sizeof(buffer) - 1, format, args);
    va_end(args);
    LogArchipelagoMessage(buffer);
}

void DrawArchipelagoLog() {
    constexpr uint64_t visibleDurationMs = 7500;

    std::deque<std::string> lines;
    {
        ArchipelagoLogState& state = GetArchipelagoLogState();
        std::lock_guard<std::mutex> lock(state.mutex);
        if (GetTickCount64() - state.lastMessageMs >= visibleDurationMs) {
            return;
        }
        lines = state.buffer.GetLines();
    }
    if (lines.empty()) {
        return;
    }

    constexpr float panelX = 8.0f;
    constexpr float panelY = 8.0f;
    constexpr float padding = 6.0f;
    constexpr float textWidth = 6.0f;
    constexpr float textHeight = 12.0f;
    constexpr float rowHeight = 14.0f;
    constexpr uint32_t panelColor = 0xb0181818; // Translucent dark gray.
    constexpr uint32_t textColor = 0xffebebeb; // Opaque light gray.
    constexpr float contentWidth = ArchipelagoLogBuffer::lineColumns * textWidth;
    const float panelHeight = padding * 2.0f + rowHeight * static_cast<float>(lines.size());

    // Keep pending native UI and the screen fade behind the log, and flush our
    // text before the frame is presented or captured in a screenshot.
    RVGL_FlushDeferredUIBatches();
    RVGL_UIDrawRoundedRect(panelX, panelY, contentWidth + padding * 2.0f, panelHeight,
        0, 0, panelColor, 0xff, 1);
    RVGL_SetupGLRenderState();

    float y = panelY + padding;
    for (std::string& line : lines) {
        RVGL_DrawUIText(panelX + padding, y, textWidth, textHeight, textColor,
            line.data(), contentWidth, 0);
        y += rowHeight;
    }
    RVGL_FlushDeferredUIBatches();
}

} // namespace Randomizer
