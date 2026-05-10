import { getAllCarsFromScan, getAllTracksFromScan } from "../../validation/validationUtils";
import { isEffectiveStockCarsMode, isEffectiveStockTracksMode, getSelectedPreset } from "../../validation/stockMode";

export function getStockModePresetErrors(scanResult, presetId) {
  const isStockCarsMode = isEffectiveStockCarsMode(scanResult, presetId);
  const isStockTracksMode = isEffectiveStockTracksMode(scanResult, presetId);
  const preset = getSelectedPreset(presetId);
  const supportsStockCarsMode = !isStockCarsMode || preset?.stockMode?.cars;
  const supportsStockTracksMode = !isStockTracksMode || preset?.stockMode?.tracks;

  if (!supportsStockCarsMode || !supportsStockTracksMode) {
    const carCount = getAllCarsFromScan(scanResult).length;
    const trackCount = getAllTracksFromScan(scanResult).length;
    return [`Stock Mode is active. This preset requires at least 42 cars and 14 tracks, but only ${carCount} cars and ${trackCount} tracks are available.`];
  }

  return [];
}

export function countEligibleCarsByRating(scanResult, rating) {
  return getAllCarsFromScan(scanResult).filter((car) => car.rating === rating).length;
}

function countAvailableFolderNames(entries, folderNames) {
  const availableFolderNames = new Set(
    (entries || []).map((entry) => entry.folderName?.toLowerCase()).filter(Boolean)
  );

  return (folderNames || []).filter((folderName) => availableFolderNames.has(folderName.toLowerCase())).length;
}

export function countEligibleCarsByFolderNames(scanResult, folderNames) {
  return countAvailableFolderNames(getAllCarsFromScan(scanResult), folderNames);
}

export function countEligibleTracksByFolderNames(scanResult, folderNames) {
  return countAvailableFolderNames(getAllTracksFromScan(scanResult), folderNames);
}

export function evaluatePresetSelection(preset, scanResult) {
  if (!scanResult) {
    return { isSelectable: true, errors: [] };
  }

  const errors = typeof preset?.validateSelection === "function"
    ? (preset.validateSelection({ scanResult }) || []).filter(Boolean)
    : [];

  return {
    isSelectable: errors.length === 0,
    errors,
  };
}

export function getPresetById(presetId, presets) {
  return (presets || []).find((preset) => preset.id === presetId) ?? null;
}

export function evaluateSelectedPreset(configure, scanResult, presets) {
  const presetId = configure?.preset;
  if (!scanResult || !presetId || presetId === "custom" || presetId === "basic") {
    return null;
  }

  const preset = getPresetById(presetId, presets);
  if (!preset) {
    return null;
  }

  return {
    preset,
    presetId,
    ...evaluatePresetSelection(preset, scanResult),
  };
}
