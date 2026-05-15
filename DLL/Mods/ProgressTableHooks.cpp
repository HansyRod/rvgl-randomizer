#include "ProgressTableHooks.h"
#include "Addresses.h"
#include "RVGLFunctions.h"
#include "RandomizerState.h"
#include "TrackHooks.h"
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <vector>

namespace Randomizer {

FnDrawProgressTable Orig_DrawProgressTable = nullptr;

namespace {

constexpr int kStockRowsPerPage = 14;
constexpr int kCustomRowsPerPage = 21;
constexpr int kFirstCustomTrackIndex = 21;

constexpr float kPanelOffsetX = 42.0f;
constexpr float kPanelOffsetY = 112.0f;
constexpr float kPanelWidth = 556.0f;
constexpr float kPanelHeight = 320.0f;
constexpr float kPanelPadding = 4.0f;
constexpr float kGlyphWidth = 8.0f;
constexpr float kGlyphHeight = 12.0f;
constexpr float kHeaderYOffset = 24.0f;
constexpr float kRowHeight = 12.0f;
constexpr float kDifficultyGap = 12.0f;
constexpr float kTrackNameOffsetX = 8.0f;
constexpr float kTrackNameMaxWidth = 160.0f;
constexpr float kIconUChecked = 0.953125f;
constexpr float kIconUUnchecked = 0.90234375f;
constexpr float kIconV = 0.953125f;
constexpr float kIconUvSize = 0.04296875f;
constexpr int kProgressIconTextureSlot = 0x5e;

constexpr uint32_t kPanelColor = 0xb0181818;
constexpr uint32_t kHeaderColor = 0xffebebeb;
constexpr uint32_t kTrackNameColor = 0xff7d7d7d;
constexpr uint32_t kPageTextColor = 0xffebeb10;
constexpr uint32_t kStarsColor = 0xff10eb10;
constexpr uint32_t kDimTextColor = 0xff7d7d7d;
constexpr uint32_t kIconColor = 0xffffffff;

char* MutableText(const char* text) {
    return const_cast<char*>(text != nullptr ? text : "");
}

char** GetLocaleStrings() {
    return *reinterpret_cast<char***>(AbsFromRva(RVA_LOCALE_STRINGS_PTR));
}

const char* LocaleStringAtByteOffset(int byteOffset) {
    char** localeStrings = GetLocaleStrings();
    if (localeStrings == nullptr) {
        return "";
    }

    return *reinterpret_cast<char**>(reinterpret_cast<uintptr_t>(localeStrings) + byteOffset);
}

const char* LocaleStringAtIndex(int index) {
    char** localeStrings = GetLocaleStrings();
    if (localeStrings == nullptr) {
        return "";
    }

    return localeStrings[index];
}

const char* CupLabel(int difficulty) {
    return LocaleStringAtIndex(difficulty + 0x13b);
}

float TextWidth(const char* text) {
    if (text == nullptr || text[0] == '\0') {
        return 0.0f;
    }

    return static_cast<float>(RVGL_UTF8GetVisibleCharCount(MutableText(text))) * kGlyphWidth;
}

uintptr_t GetGallerySlotsBase() {
    return *reinterpret_cast<uintptr_t*>(AbsFromRva(RVA_GALLERY_SLOTS_PTR));
}

float ReadSlotFloat(int slotIndex, int offset) {
    const uintptr_t slots = GetGallerySlotsBase();
    if (slots == 0) {
        return 0.0f;
    }

    return *reinterpret_cast<float*>(slots + static_cast<uintptr_t>(slotIndex) * 0x140 + offset);
}

int ReadSlotInt(int slotIndex, int offset) {
    const uintptr_t slots = GetGallerySlotsBase();
    if (slots == 0) {
        return 0;
    }

    return *reinterpret_cast<int*>(slots + static_cast<uintptr_t>(slotIndex) * 0x140 + offset);
}

bool IsRaceTrack(const TrackInfo& track) {
    return track.gameType == 0 || track.gameType == 4;
}

bool ShouldShowStockSlot(int trackIndex) {
    if (trackIndex != 4) {
        return true;
    }

    return (RVGL_TrackIsAvailableForFrontend(trackIndex, true) & 0xff) != 0;
}

void RebuildProgressTableCache() {
    RandomizerContext& ctx = GetRandomizerContext();
    ProgressTableRuntimeState& state = ctx.progressTableState;

    state.stockTrackIndices.clear();
    state.customTrackIndices.clear();

    for (int trackIndex = 0; trackIndex < kStockRowsPerPage; ++trackIndex) {
        if (ShouldShowStockSlot(trackIndex)) {
            state.stockTrackIndices.push_back(trackIndex);
        }
    }

    std::vector<int> customTracks;
    const int trackCount = GetRuntimeTrackCount();

    for (int trackIndex = kFirstCustomTrackIndex; trackIndex < trackCount; ++trackIndex) {
        TrackInfo* track = GetTrackInfoByRuntimeIndex(trackIndex);
        if (track == nullptr) {
            continue;
        }

        if (!IsRaceTrack(*track)) {
            continue;
        }

        if ((track->trackAvailFlags & TRACKAVAIL_EXISTS) == 0) {
            continue;
        }

        customTracks.push_back(trackIndex);
    }

    std::stable_sort(customTracks.begin(), customTracks.end(), [](int lhs, int rhs) {
        const TrackInfo* leftTrack = GetTrackInfoByRuntimeIndex(lhs);
        const TrackInfo* rightTrack = GetTrackInfoByRuntimeIndex(rhs);
        if (leftTrack == nullptr || rightTrack == nullptr) {
            return lhs < rhs;
        }

        const int nameCompare = _stricmp(leftTrack->displayName, rightTrack->displayName);
        if (nameCompare != 0) {
            return nameCompare < 0;
        }

        return lhs < rhs;
    });

    state.customTrackIndices.insert(
        state.customTrackIndices.end(),
        customTracks.begin(),
        customTracks.end()
    );
    state.cachedTrackCount = trackCount;
    state.cacheValid = true;
}

void EnsureProgressTableCache() {
    RandomizerContext& ctx = GetRandomizerContext();
    ProgressTableRuntimeState& state = ctx.progressTableState;

    if (!state.cacheValid || state.cachedTrackCount != GetRuntimeTrackCount()) {
        RebuildProgressTableCache();
    }
}

std::vector<int> BuildVisibleTracksForPage(int page) {
    EnsureProgressTableCache();

    std::vector<int> tracks;
    tracks.reserve(page == 0 ? kStockRowsPerPage : kCustomRowsPerPage);

    if (page == 0) {
        tracks = GetRandomizerContext().progressTableState.stockTrackIndices;
        return tracks;
    }

    const std::vector<int>& customTracks = GetRandomizerContext().progressTableState.customTrackIndices;
    const int start = (page - 1) * kCustomRowsPerPage;
    const int end = (std::min)(start + kCustomRowsPerPage, static_cast<int>(customTracks.size()));

    for (int index = start; index < end; ++index) {
        tracks.push_back(customTracks[index]);
    }

    return tracks;
}

int GetProgressTablePageCount() {
    EnsureProgressTableCache();

    const int customTrackCount = static_cast<int>(GetRandomizerContext().progressTableState.customTrackIndices.size());
    return 1 + (customTrackCount + kCustomRowsPerPage - 1) / kCustomRowsPerPage;
}

void EnsureProgressLoaded(int trackIndex) {
    TrackInfo* track = GetTrackInfoByRuntimeIndex(trackIndex);
    if (track == nullptr) {
        return;
    }

    if ((track->trackProgressFlags & TRACKPROGRESS_PROGRESS_LOADED) == 0) {
        RVGL_TrackLoadProgressFromFile(trackIndex);
    }
}

void DrawText(float x, float y, uint32_t color, const char* text, float maxWidth = 0.0f) {
    RVGL_DrawTexturedQuad(x, y, kGlyphWidth, kGlyphHeight, color, MutableText(text), maxWidth, 0);
}

void DrawCenteredText(float centerX, float y, float width, uint32_t color, const char* text) {
    const float drawX = centerX + (width - TextWidth(text)) * 0.5f;
    DrawText(drawX, y, color, text, width);
}

void DrawProgressIcon(float x, float y, bool complete) {
    RVGL_DrawSprite2D(
        x,
        y,
        kGlyphHeight,
        kGlyphHeight,
        complete ? kIconUChecked : kIconUUnchecked,
        kIconV,
        kIconUvSize,
        kIconUvSize,
        kIconColor,
        kProgressIconTextureSlot,
        0
    );
}

bool IsTierComplete(int difficulty) {
    return Orig_CheckIfTierChampionshipWon(difficulty);
}

void DrawColumnHeaders(float panelX, float currentX, float currentY) {
    DrawText(currentX, currentY, kHeaderColor, LocaleStringAtByteOffset(0x858), 128.0f);

    const float iconX = panelX + 176.0f;
    DrawCenteredText(iconX - 20.0f, currentY, 80.0f, kHeaderColor, LocaleStringAtByteOffset(0x860));
    DrawCenteredText(iconX - 20.0f, currentY + kGlyphHeight, 80.0f, kHeaderColor, LocaleStringAtByteOffset(0x868));
    DrawCenteredText(iconX + 60.0f, currentY, 80.0f, kHeaderColor, LocaleStringAtByteOffset(0x870));
    DrawCenteredText(iconX + 140.0f, currentY, 80.0f, kHeaderColor, LocaleStringAtByteOffset(0x878));
    DrawCenteredText(iconX + 220.0f, currentY, 80.0f, kHeaderColor, LocaleStringAtByteOffset(0x880));
    DrawCenteredText(iconX + 300.0f, currentY, 80.0f, kHeaderColor, LocaleStringAtByteOffset(0x888));
}

void DrawTrackRow(float panelX, float currentX, float y, int trackIndex) {
    EnsureProgressLoaded(trackIndex);

    TrackInfo* track = GetTrackInfoByRuntimeIndex(trackIndex);
    if (track == nullptr) {
        return;
    }

    DrawText(currentX + kTrackNameOffsetX, y, kTrackNameColor, track->displayName, kTrackNameMaxWidth);

    const float iconX = panelX + 176.0f;
    DrawProgressIcon(iconX, y, (track->trackProgressFlags & TRACKPROGRESS_RACE_WON) != 0);
    DrawProgressIcon(iconX + 80.0f, y, (track->trackProgressFlags & TRACKPROGRESS_NORMAL_CHALLENGE_BEATEN) != 0);
    DrawProgressIcon(iconX + 160.0f, y, (track->trackProgressFlags & TRACKPROGRESS_REVERSE_CHALLENGE_BEATEN) != 0);
    DrawProgressIcon(iconX + 240.0f, y, (track->trackProgressFlags & TRACKPROGRESS_MIRROR_CHALLENGE_BEATEN) != 0);
    DrawProgressIcon(iconX + 320.0f, y, (track->trackProgressFlags & TRACKPROGRESS_PRACTICE_STAR) != 0);
}

int ReadMenuAction() {
    return *reinterpret_cast<int*>(AbsFromRva(RVA_MENU_ACTION));
}

void DrawFooter(float panelX, float currentX, float y, int page, int pageCount) {
    char stuntText[160] = {};
    std::snprintf(
        stuntText,
        sizeof(stuntText),
        "%s - %s",
        LocaleStringAtByteOffset(0xd0),
        LocaleStringAtByteOffset(0x8c0)
    );
    const float stuntTextWidth = TextWidth(stuntText);
    DrawText(currentX, y, kHeaderColor, stuntText, 448.0f);

    char starsText[80] = {};
    const int starsFound = *reinterpret_cast<int*>(AbsFromRva(RVA_TOTAL_STARS_EARNED));
    const int maxStars = *reinterpret_cast<int*>(AbsFromRva(RVA_MAX_POSSIBLE_STARS));
    std::snprintf(
        starsText,
        sizeof(starsText),
        "%d %s %d",
        starsFound,
        LocaleStringAtByteOffset(0x8c8),
        maxStars
    );
    DrawText(currentX + stuntTextWidth + kGlyphWidth, y, kStarsColor, starsText, 84.0f);

    char pageText[32] = {};
    std::snprintf(pageText, sizeof(pageText), "Page %d / %d", page + 1, pageCount);
    const float pageTextX = panelX + kPanelWidth - TextWidth(pageText) - kPanelPadding;
    DrawText(pageTextX, y, pageCount > 1 ? kPageTextColor : kDimTextColor, pageText, 128.0f);
}

} // anonymous namespace

void InvalidateProgressTableCache() {
    GetRandomizerContext().progressTableState.cacheValid = false;
}

void ClampProgressTablePage() {
    RandomizerContext& ctx = GetRandomizerContext();
    const int pageCount = GetProgressTablePageCount();
    ctx.progressTableState.currentPage =
        std::clamp(ctx.progressTableState.currentPage, 0, (std::max)(0, pageCount - 1));
}

void ResetProgressTablePage() {
    RandomizerContext& ctx = GetRandomizerContext();
    ctx.progressTableState.currentPage = 0;
    InvalidateProgressTableCache();
}

bool DecrementProgressTablePage(int) {
    RandomizerContext& ctx = GetRandomizerContext();
    ClampProgressTablePage();

    if (ctx.progressTableState.currentPage <= 0) {
        return false;
    }

    --ctx.progressTableState.currentPage;
    return true;
}

bool IncrementProgressTablePage(int) {
    RandomizerContext& ctx = GetRandomizerContext();
    ClampProgressTablePage();

    const int lastPage = GetProgressTablePageCount() - 1;
    if (ctx.progressTableState.currentPage >= lastPage) {
        return false;
    }

    ++ctx.progressTableState.currentPage;
    return true;
}

void Hook_DrawProgressTable(int slotIndex) {
    ClampProgressTablePage();

    const int action = ReadMenuAction();
    if (action == 2) {
        DecrementProgressTablePage(slotIndex);
    }
    else if (action == 3) {
        IncrementProgressTablePage(slotIndex);
    }

    const int page = GetRandomizerContext().progressTableState.currentPage;
    const int pageCount = GetProgressTablePageCount();
    const std::vector<int> tracks = BuildVisibleTracksForPage(page);
    const bool isCustomPage = page > 0;

    const float panelX = ReadSlotFloat(slotIndex, 0x114) + kPanelOffsetX;
    const float panelY = ReadSlotFloat(slotIndex, 0x118) + kPanelOffsetY;
    const int panelId = ReadSlotInt(slotIndex, 0x110);

    RVGL_UIDrawRoundedRect(panelX, panelY, kPanelWidth, kPanelHeight, panelId, 0, kPanelColor, 0xff, 1);
    RVGL_NetworkRenderStub();
    RVGL_SetupGLRenderState();

    const float currentX = panelX + kPanelPadding;
    const float currentY = panelY + kPanelPadding;
    float rowY = currentY + kHeaderYOffset;
    int currentDifficulty = -1;

    DrawColumnHeaders(panelX, currentX, currentY);

    if (isCustomPage) {
        rowY += kRowHeight;
    }

    for (int trackIndex : tracks) {
        TrackInfo* track = GetTrackInfoByRuntimeIndex(trackIndex);
        if (track == nullptr) {
            continue;
        }

        if (!isCustomPage) {
            const int difficulty = track->difficultyRating;
            if (difficulty != currentDifficulty) {
                if (currentDifficulty != -1) {
                    rowY += kDifficultyGap;
                }

                const char* cupLabel = CupLabel(difficulty);
                DrawText(currentX, rowY, kHeaderColor, cupLabel, 128.0f);
                DrawProgressIcon(currentX + TextWidth(cupLabel) + kPanelPadding, rowY, IsTierComplete(difficulty));
                rowY += kRowHeight;
                currentDifficulty = difficulty;
            }
        }

        DrawTrackRow(panelX, currentX, rowY, trackIndex);
        rowY += kRowHeight;
    }

    if (!isCustomPage && !ShouldShowStockSlot(4)) {
        rowY += kRowHeight;
    }

    const float footerY = isCustomPage
        ? currentY + kHeaderYOffset + kRowHeight + static_cast<float>(kCustomRowsPerPage) * kRowHeight + kRowHeight
        : rowY + kHeaderYOffset;
    DrawFooter(panelX, currentX, footerY, page, pageCount);
}

} // namespace Randomizer
