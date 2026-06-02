import { CUSTOM_UNLOCK_COUNT_METHODS, CUSTOM_UNLOCK_SPECIFIC_METHODS } from "./constants";

const CUSTOM_SPECIFIC_METHODS = new Set(CUSTOM_UNLOCK_SPECIFIC_METHODS.map(method => method.val));
const CUSTOM_COUNT_METHODS = new Set(CUSTOM_UNLOCK_COUNT_METHODS.map(method => method.val));
const CUSTOM_UNLOCK_LABELS = {
  6: "Race Win",
  7: "Practice Star",
  8: "Time Trial",
  9: "Race Win",
  10: "Practice Star",
  11: "Time Trial",
  12: "Stunt Arena",
};

export function isCustomUnlockMethod(method) {
  const key = String(method);
  return CUSTOM_SPECIFIC_METHODS.has(key) || CUSTOM_COUNT_METHODS.has(key);
}

export function makeDefaultCustomUnlock(method) {
  const key = String(method);

  if (CUSTOM_SPECIFIC_METHODS.has(key)) {
    return {
      method: key,
      mode: "randomTracks",
      trackFolders: [],
      randomTrackCount: 1,
    };
  }

  if (CUSTOM_COUNT_METHODS.has(key)) {
    return {
      method: key,
      requiredCount: 1,
    };
  }

  return null;
}

export function normalizeCustomUnlockRow(row) {
  if (!row) return row;

  const attrObtain = row.attrObtain;

  if (!isCustomUnlockMethod(attrObtain)) {
    if (row.customUnlock == null) return row;
    return {
      ...row,
      customUnlock: null,
    };
  }

  if (row?.customUnlock?.method === String(attrObtain)) {
    return row;
  }

  return {
    ...row,
    customUnlock: makeDefaultCustomUnlock(attrObtain),
  };
}

export function getCustomUnlockSelectionLabel(customUnlock, trackByFolder = {}) {
  if (!customUnlock) return "Custom Unlock: Not Configured";

  const method = String(customUnlock.method);
  const label = CUSTOM_UNLOCK_LABELS[method] || "Custom Unlock";

  if (customUnlock.mode === "specificTracks") {
    const folders = customUnlock.trackFolders || [];
    if (folders.length === 0) return `${label}: No Tracks Selected`;

    const firstTrack = trackByFolder[folders[0]];
    const firstLabel = firstTrack?.name || folders[0];
    if (folders.length === 1) return `${label}: ${firstLabel}`;
    return `${label}: ${firstLabel}, +${folders.length - 1} more`;
  }

  if (customUnlock.mode === "randomTracks") {
    const count = customUnlock.randomTrackCount ?? 1;
    return `${label}: ${count} Random ${count === 1 ? "Track" : "Tracks"}`;
  }

  if (customUnlock.requiredCount !== undefined) {
    const count = customUnlock.requiredCount;
    if (method === "12") return `${label}: ${count} ${count === 1 ? "Star" : "Stars"}`;
    return `${label}: Any ${count} ${count === 1 ? "Track" : "Tracks"}`;
  }

  return `${label}: Not Configured`;
}
