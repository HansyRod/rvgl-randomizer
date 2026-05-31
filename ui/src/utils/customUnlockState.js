import { CUSTOM_UNLOCK_COUNT_METHODS, CUSTOM_UNLOCK_SPECIFIC_METHODS } from "./constants";

const CUSTOM_SPECIFIC_METHODS = new Set(CUSTOM_UNLOCK_SPECIFIC_METHODS.map(method => method.val));
const CUSTOM_COUNT_METHODS = new Set(CUSTOM_UNLOCK_COUNT_METHODS.map(method => method.val));

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
