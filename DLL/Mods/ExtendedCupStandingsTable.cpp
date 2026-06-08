#include "ExtendedCupStandingsTable.h"
#include "30CarMod.h"
#include "RVGLFunctions.h"
#include "RVGLMemory.h"
#include <algorithm>
#include <cmath>
#include <cstdio>

namespace Randomizer {

namespace {

char* MutableText(const char* text) {
    return const_cast<char*>(text != nullptr ? text : "");
}

const char* LocaleString(int index, const char* fallback) {
    char** strings = GetLocaleStrings();
    if (strings == nullptr || strings[index] == nullptr) {
        return fallback;
    }

    return strings[index];
}

bool IsCupStandingsTableVisible() {
    int* displayState = GetPostRaceMenuDisplayState();
    return displayState != nullptr && *displayState == 4;
}

float GetUiViewportCenterX() {
    constexpr float nativeUiWidth = 640.0f;
    constexpr float nativeUiHalfWidth = nativeUiWidth * 0.5f;

    UiViewportRuntime* viewport = GetUiViewportRuntime();
    if (viewport == nullptr) {
        return nativeUiHalfWidth;
    }

    if (!std::isfinite(viewport->centerX) || viewport->centerX <= 0.0f) {
        return nativeUiHalfWidth;
    }

    return viewport->centerX;
}

float GetCenteredPanelX(float panelWidth) {
    constexpr float nativeUiWidth = 640.0f;
    constexpr float nativeUiHalfWidth = nativeUiWidth * 0.5f;

    const float uiScale = GetCupProgressUiCoordScale();
    const float scale = std::isfinite(uiScale) && uiScale > 0.0f ? uiScale : 0.5f;
    return (nativeUiWidth - panelWidth) * scale + GetUiViewportCenterX() * scale - nativeUiHalfWidth;
}

const CarInfo* GetCarInfoByModelId(int modelId) {
    CarInfo* cars = GetCarInfoTable();
    if (cars == nullptr || modelId < 0 || modelId >= GetTotalCarModelCount()) {
        return nullptr;
    }

    return &cars[modelId];
}

void DrawSizedText(
    float x,
    float y,
    float width,
    float height,
    uint32_t color,
    const char* text,
    float maxWidth = 0.0f
) {
    RVGL_DrawUIText(x, y, width, height, color, MutableText(text), maxWidth, 0);
}

void DrawRightSizedText(
    float rightX,
    float y,
    float width,
    float height,
    uint32_t color,
    const char* text
) {
    const int64_t len = RVGL_UTF8GetVisibleCharCount(MutableText(text));
    DrawSizedText(rightX - static_cast<float>(len) * width, y, width, height, color, text);
}

const char* GetFinishSuffix(int zeroBasedPosition) {
    const int oneBasedPosition = zeroBasedPosition + 1;
    const int lastTwoDigits = oneBasedPosition % 100;
    if (lastTwoDigits >= 11 && lastTwoDigits <= 13) {
        return LocaleString(0x122, "th");
    }

    const int lastDigit = oneBasedPosition % 10;
    if (lastDigit == 1) {
        return LocaleString(0x11f, "st");
    }
    if (lastDigit == 2) {
        return LocaleString(0x120, "nd");
    }
    if (lastDigit == 3) {
        return LocaleString(0x121, "rd");
    }

    return LocaleString(0x122, "th");
}

void FormatStageFinish(const CupParticipantEntry& standing, int stage, char (&outText)[16]) {
    const int finishPosition = standing.finishPositionByStage[stage];
    std::snprintf(
        outText,
        sizeof(outText),
        "%d%s",
        finishPosition + 1,
        GetFinishSuffix(finishPosition)
    );
}

void FormatStandingName(
    const CupParticipantEntry& standing,
    const char* playerName,
    char (&outName)[20]
) {
    if (standing.participantIndex == 0) {
        std::snprintf(outName, sizeof(outName), "%.*s", 19, playerName != nullptr ? playerName : "Player");
        return;
    }

    const CarInfo* car = GetCarInfoByModelId(standing.modelId);
    if (car == nullptr || car->displayName[0] == '\0') {
        std::snprintf(outName, sizeof(outName), "Car %02d", standing.modelId);
        return;
    }

    std::snprintf(outName, sizeof(outName), "%.*s", 19, car->displayName);
}

} // anonymous namespace

void DrawExtendedCupStandingsTable(
    bool active,
    CupProfile* activeCup,
    const ExtendedCupResultsState& results,
    const char* playerName
) {
    if (!active || activeCup == nullptr) {
        return;
    }

    const int count = std::clamp(activeCup->numCars, 0, randomizerMaxCarCount);
    if (count <= vanillaMaxCarCount) {
        return;
    }

    if (!IsCupStandingsTableVisible()) {
        return;
    }

    constexpr int maxStageColumns = 5;

    const int currentStage = std::clamp(GetCurrentCupStageIndex(), 0, 15);
    const int visibleStageCount = std::clamp(currentStage + 1, 1, maxStageColumns);
    const int firstVisibleStage = currentStage - visibleStageCount + 1;

    const float textWidth = count > 27 ? 6.2f : (count > 24 ? 7.0f : (count > 20 ? 7.5f : 8.0f));
    const float textHeight = count > 27 ? 11.0f : (count > 24 ? 12.5f : (count > 20 ? 14.0f : 16.0f));
    const float rowHeight = textHeight;

    const float panelY = 70.0f;
    const float contentPaddingY = 4.0f;
    const float headerY = panelY + contentPaddingY;
    const float rowStartY = headerY + rowHeight + 1.0f;
    const float nameMaxWidth = 118.0f;
    const float stageSpacing = count > 27 ? 30.0f : (count > 24 ? 33.0f : (count > 20 ? 35.0f : 40.0f));
    const float contentPaddingX = 4.0f;
    const float nameToStageGap = count > 27 ? 8.0f : (count > 24 ? 10.0f : (count > 20 ? 12.0f : 16.0f));
    const float stageColumnRightInset = 20.0f;
    const float pointsGap = 20.0f;
    const float pendingGap = 28.0f;
    const float stageStartFromTable = nameMaxWidth + nameToStageGap;
    const float pointsRightFromTable =
        stageStartFromTable +
        static_cast<float>(visibleStageCount) * stageSpacing +
        pointsGap;
    const float pendingRightFromTable = pointsRightFromTable + pendingGap;
    const float panelWidth = pendingRightFromTable + contentPaddingX * 2.0f;
    const float panelX = GetCenteredPanelX(panelWidth);
    const float tableX = panelX + contentPaddingX;
    const float stageStartX = tableX + stageStartFromTable;
    const float pointsRightX = tableX + pointsRightFromTable;
    const float pendingRightX = tableX + pendingRightFromTable;
    const float panelHeight = std::clamp(
        rowStartY - panelY + static_cast<float>(count) * rowHeight + contentPaddingY,
        180.0f,
        362.0f
    );

    RVGL_UIDrawRoundedRect(panelX, panelY, panelWidth, panelHeight, 0, 0, 0xb0181818, 0xff, 1);
    RVGL_FlushDeferredUIBatches();
    RVGL_SetupGLRenderState();

    DrawSizedText(
        tableX,
        headerY,
        textWidth,
        textHeight,
        0xff00ffff,
        LocaleString(0x115, "Standings"),
        nameMaxWidth
    );

    for (int stageColumn = 0; stageColumn < visibleStageCount; ++stageColumn) {
        char stageLabel[8] = {};
        std::snprintf(stageLabel, sizeof(stageLabel), "%d", firstVisibleStage + stageColumn + 1);
        DrawRightSizedText(
            stageStartX + static_cast<float>(stageColumn) * stageSpacing + stageColumnRightInset,
            headerY,
            textWidth,
            textHeight,
            0xff00ffff,
            stageLabel
        );
    }
    DrawRightSizedText(pointsRightX, headerY, textWidth, textHeight, 0xff00ffff, LocaleString(0x129, "Pts"));

    for (int row = 0; row < count; ++row) {
        const CupParticipantEntry& standing = results.standings[row];
        const float y = rowStartY + static_cast<float>(row) * rowHeight;

        char name[20] = {};
        FormatStandingName(standing, playerName, name);
        DrawSizedText(tableX, y, textWidth, textHeight, 0xffffffff, name, nameMaxWidth);

        for (int stageColumn = 0; stageColumn < visibleStageCount; ++stageColumn) {
            char finishText[16] = {};
            FormatStageFinish(standing, firstVisibleStage + stageColumn, finishText);
            DrawRightSizedText(
                stageStartX + static_cast<float>(stageColumn) * stageSpacing + stageColumnRightInset,
                y,
                textWidth,
                textHeight,
                0xff00ff00,
                finishText
            );
        }

        char points[16] = {};
        std::snprintf(points, sizeof(points), "%2.2d", standing.totalPoints);
        DrawRightSizedText(pointsRightX, y, textWidth, textHeight, 0xffffff00, points);

        if (standing.pendingPoints != 0) {
            char pending[16] = {};
            std::snprintf(pending, sizeof(pending), "%+d", standing.pendingPoints);
            DrawRightSizedText(pendingRightX, y, textWidth, textHeight, 0xffff0000, pending);
        }
    }
}

} // namespace Randomizer
