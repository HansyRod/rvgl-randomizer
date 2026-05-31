export const CUSTOM_UNLOCK_METHOD_KEYS = [
  "includeSpecificRaceWin",
  "includeSpecificPracticeStar",
  "includeSpecificTimeTrial",
  "includeRaceWinCount",
  "includePracticeStarCount",
  "includeTimeTrialCount",
  "includeStuntArenaStarCount",
];

const CUSTOM_UNLOCK_RANGES = [
  { label: "Specific Race Win track count", enabledKey: "includeSpecificRaceWin", minKey: "specificRaceWinTrackCountMin", maxKey: "specificRaceWinTrackCountMax", trackBound: true },
  { label: "Specific Practice Star track count", enabledKey: "includeSpecificPracticeStar", minKey: "specificPracticeStarTrackCountMin", maxKey: "specificPracticeStarTrackCountMax", trackBound: true },
  { label: "Specific Time Trial track count", enabledKey: "includeSpecificTimeTrial", minKey: "specificTimeTrialTrackCountMin", maxKey: "specificTimeTrialTrackCountMax", trackBound: true },
  { label: "Race Win Count threshold", enabledKey: "includeRaceWinCount", minKey: "raceWinCountMin", maxKey: "raceWinCountMax", trackBound: true },
  { label: "Practice Star Count threshold", enabledKey: "includePracticeStarCount", minKey: "practiceStarCountMin", maxKey: "practiceStarCountMax", trackBound: true },
  { label: "Time Trial Count threshold", enabledKey: "includeTimeTrialCount", minKey: "timeTrialCountMin", maxKey: "timeTrialCountMax", trackBound: true },
  { label: "Stunt Arena Star Count threshold", enabledKey: "includeStuntArenaStarCount", minKey: "stuntArenaStarCountMin", maxKey: "stuntArenaStarCountMax", hardMax: 20 },
];

export function hasEnabledCustomUnlockMethod(options) {
  return CUSTOM_UNLOCK_METHOD_KEYS.some((key) => options?.[key]);
}

export function getCustomUnlockTrackCountMax(trackSpecState, allTracks, isStockMode) {
  const specTracks = trackSpecState?.tracks || [];
  const sourceTracks = specTracks.length > 0
    ? specTracks
    : (allTracks || []).map((track) => ({ id: track.folderName }));

  if (!isStockMode) {
    return sourceTracks.length;
  }

  return sourceTracks.filter((track) => {
    const folder = track?.id || track?.sourcePool || track?.folderName || "";
    return folder.toLowerCase() !== "roof";
  }).length;
}

export function validateCustomUnlockRanges(options, errors, scope, limits = {}) {
  const trackCountMax = Number(limits.trackCountMax);

  CUSTOM_UNLOCK_RANGES.forEach((range) => {
    if (!options?.[range.enabledKey]) {
      return;
    }

    const min = Number(options?.[range.minKey] ?? 1);
    const max = Number(options?.[range.maxKey] ?? 1);

    if (!Number.isFinite(min) || min < 1) {
      errors.push({
        id: `${scope}_${range.minKey}_invalid`,
        scope,
        message: `${range.label} minimum must be at least 1.`,
      });
    }

    if (!Number.isFinite(max) || max < 1) {
      errors.push({
        id: `${scope}_${range.maxKey}_invalid`,
        scope,
        message: `${range.label} maximum must be at least 1.`,
      });
    }

    if (Number.isFinite(min) && Number.isFinite(max) && max < min) {
      errors.push({
        id: `${scope}_${range.maxKey}_below_min`,
        scope,
        message: `${range.label} maximum must be greater than or equal to its minimum.`,
      });
    }

    if (range.hardMax && Number.isFinite(max) && max > range.hardMax) {
      errors.push({
        id: `${scope}_${range.maxKey}_too_high`,
        scope,
        message: `${range.label} maximum cannot exceed ${range.hardMax}.`,
      });
    }

    if (range.trackBound && Number.isFinite(trackCountMax) && trackCountMax > 0) {
      if (Number.isFinite(min) && min > trackCountMax) {
        errors.push({
          id: `${scope}_${range.minKey}_too_high`,
          scope,
          message: `${range.label} minimum cannot exceed the current track count (${trackCountMax}).`,
        });
      }

      if (Number.isFinite(max) && max > trackCountMax) {
        errors.push({
          id: `${scope}_${range.maxKey}_too_high`,
          scope,
          message: `${range.label} maximum cannot exceed the current track count (${trackCountMax}).`,
        });
      }
    }
  });
}
