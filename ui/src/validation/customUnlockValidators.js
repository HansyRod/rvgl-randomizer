import { CUSTOM_UNLOCK_COUNT_METHODS, CUSTOM_UNLOCK_SPECIFIC_METHODS } from "../utils/constants";

export const CUSTOM_UNLOCK_METHOD_KEYS = [
  "includeSpecificRaceWin",
  "includeSpecificPracticeStar",
  "includeSpecificTimeTrial",
  "includeRaceWinCount",
  "includePracticeStarCount",
  "includeTimeTrialCount",
  "includeStuntArenaStarCount",
];

const SPECIFIC_CUSTOM_UNLOCK_METHODS = new Set(CUSTOM_UNLOCK_SPECIFIC_METHODS.map(method => method.val));
const COUNT_CUSTOM_UNLOCK_METHODS = new Set(CUSTOM_UNLOCK_COUNT_METHODS.map(method => method.val));

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

export function validateCustomUnlockRows(rows, errors, options = {}) {
  const {
    scope,
    rowLabelPrefix,
    availableTrackFolders = new Set(),
    getKnownTargetTrackFolder = () => null,
  } = options;

  (rows || []).forEach((row, index) => {
    const method = String(row?.attrObtain ?? "");
    if (!SPECIFIC_CUSTOM_UNLOCK_METHODS.has(method) && !COUNT_CUSTOM_UNLOCK_METHODS.has(method)) {
      return;
    }

    const rowLabel = makeRowLabel(rowLabelPrefix, index, row);
    const customUnlock = row?.customUnlock;

    if (!customUnlock || String(customUnlock.method) !== method) {
      errors.push({
        id: `${scope}_custom_unlock_missing_${index}`,
        scope,
        message: `${rowLabel}: custom unlock condition is missing or does not match the selected unlock method.`,
      });
      return;
    }

    if (SPECIFIC_CUSTOM_UNLOCK_METHODS.has(method)) {
      validateSpecificTrackCondition(customUnlock, errors, {
        scope,
        rowLabel,
        index,
        availableTrackFolders,
        knownTargetTrackFolder: getKnownTargetTrackFolder(row),
      });
      return;
    }

    validateCountCondition(customUnlock, errors, {
      scope,
      rowLabel,
      index,
    });
  });
}

function validateSpecificTrackCondition(customUnlock, errors, options) {
  const { scope, rowLabel, index, availableTrackFolders, knownTargetTrackFolder } = options;

  if (customUnlock.mode === "randomTracks") {
    const randomTrackCount = Number(customUnlock.randomTrackCount);
    if (!Number.isFinite(randomTrackCount) || randomTrackCount < 1) {
      errors.push({
        id: `${scope}_custom_unlock_random_track_count_invalid_${index}`,
        scope,
        message: `${rowLabel}: custom unlock random track count must be greater than 0.`,
      });
    }
    return;
  }

  if (customUnlock.mode !== "specificTracks") {
    errors.push({
      id: `${scope}_custom_unlock_specific_mode_invalid_${index}`,
      scope,
      message: `${rowLabel}: custom unlock must use either specific tracks or a random track count.`,
    });
    return;
  }

  const trackFolders = customUnlock.trackFolders || [];
  if (trackFolders.length === 0) {
    errors.push({
      id: `${scope}_custom_unlock_specific_tracks_empty_${index}`,
      scope,
      message: `${rowLabel}: custom unlock requires at least one prerequisite track.`,
    });
    return;
  }

  const staleFolders = trackFolders.filter((folder) => !availableTrackFolders.has(String(folder).toLowerCase()));
  staleFolders.forEach((folder, folderIndex) => {
    errors.push({
      id: `${scope}_custom_unlock_specific_track_missing_${index}_${folderIndex}`,
      scope,
      message:
        `${rowLabel}: "${folder}" is not available in the current track setup. ` +
        `Add it in Track Specification or choose another track.`,
    });
  });

  if (
    knownTargetTrackFolder &&
    trackFolders.some((folder) => String(folder).toLowerCase() === knownTargetTrackFolder.toLowerCase())
  ) {
    errors.push({
      id: `${scope}_custom_unlock_self_dependency_${index}`,
      scope,
      message: `${rowLabel}: custom unlock cannot require the target track itself (${knownTargetTrackFolder}).`,
    });
  }
}

function validateCountCondition(customUnlock, errors, options) {
  const { scope, rowLabel, index } = options;
  const requiredCount = Number(customUnlock.requiredCount);

  if (!Number.isFinite(requiredCount) || requiredCount < 1) {
    errors.push({
      id: `${scope}_custom_unlock_required_count_invalid_${index}`,
      scope,
      message: `${rowLabel}: custom unlock required count must be greater than 0.`,
    });
  }
}

function makeRowLabel(prefix, index, row) {
  const id = row?.id ? ` (${row.id})` : "";
  return `${prefix} slot ${index + 1}${id}`;
}
