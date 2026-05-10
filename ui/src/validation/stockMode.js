import {
  hasAllStockCars,
  hasAllStockTracks,
  hasOnlyStockCarsLoaded,
  hasOnlyStockTracksLoaded,
} from "./validationUtils.js";
import { PRESETS } from "../configure/presets";


export function getSelectedPreset(presetId) {
  if (!presetId || presetId === "basic" || presetId === "custom") {
    return null;
  }

  return (PRESETS || []).find((preset) => preset.id === presetId) ?? null;
}

export function isEffectiveStockCarsMode(scanResult, presetId) {
  if (!hasAllStockCars(scanResult)) {
    return false;
  }

  return hasOnlyStockCarsLoaded(scanResult) || Boolean(getSelectedPreset(presetId)?.stockMode?.cars);
}

export function isEffectiveStockTracksMode(scanResult, presetId) {
  if (!hasAllStockTracks(scanResult)) {
    return false;
  }

  return hasOnlyStockTracksLoaded(scanResult) || Boolean(getSelectedPreset(presetId)?.stockMode?.tracks);
}
