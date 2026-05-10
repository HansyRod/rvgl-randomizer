import { formatValidationList, getAllTracksFromScan } from "./validationUtils";
import { isEffectiveStockTracksMode } from "./stockMode";

export function validateTrackSpec(trackSpecState, trackOptions, scanResult, preset) {
  const errors = [];
  const warnings = [];
  const infos = [];

  const allTracks = getAllTracksFromScan(scanResult);

  const isStockMode = isEffectiveStockTracksMode(scanResult, preset);
  if (isStockMode) {
    infos.push({
      id: "tracks_stock_mode_active",
      scope: "trackSpec",
      message: "Stock Content Mode is active for tracks. Only 13 basic stock tracks will be used."
    });
  }

  if (allTracks.length === 0) {
    errors.push({
      id: "tracks_none_available",
      scope: "trackSpec",
      message: "No playable tracks are available in the selected content."
    });
    return { errors, warnings, infos };
  }

  const allTrackFolders = new Set(allTracks.map(t => t.folderName.toLowerCase()));

  if (trackSpecState?.includeTracks === false) {

    // Check stale original-track references
    const staleStockRefs = [];
    const trackIds = (trackSpecState?.tracks || []).map(row => row?.id).filter(Boolean);

    // When stock tracks are not randomized, validate they exist in the source pool
    trackIds.forEach((track, i) => {
      if (!allTrackFolders.has(track)) {
        staleStockRefs.push(`slot ${i + 1} (${track})`);
      }
    })

    if (staleStockRefs.length > 0) {
      warnings.push({
        id: "tracks_stale_stock_refs",
        scope: "trackSpec",
        message: `Some stock track slots are missing from the selected content: ${formatValidationList(staleStockRefs)}. Those tracks will be unavailable.`
      });
    }

    return { errors, warnings, infos };
  }

  // Unlock method pool must have at least one method enabled when randomizing obtain values
  const showObtainControls =
    trackOptions?.unlockMode === "random" || trackOptions?.unlockMode === "randomUnlock";

  if (showObtainControls) {
    const anyMethodAllowed =
      (trackOptions.includeDefault    ?? true) ||
      (trackOptions.includeTimeTrial  ?? true) ||
      (trackOptions.includePractice   ?? true) ||
      (trackOptions.includeSingleRace ?? true) ||
      trackOptions.includeStuntArena;

    if (!anyMethodAllowed) {
      errors.push({
        id: "tracks_no_unlock_methods",
        scope: "trackOptions",
        message: "At least one unlock method must be enabled. Enable at least one method in \"Allowed Unlock Methods\".",
      });
    }
  }

  // Stale specific-track references
  const staleTracks = [];

  (trackSpecState?.tracks || []).forEach((row, i) => {
    const isSpecific =
      row.sourcePool &&
      row.sourcePool !== "Full Random" &&
      row.sourcePool !== "Stock" &&
      row.sourcePool !== "Custom" &&
      !row.sourcePool.startsWith("Pack:");

    if (isSpecific && !allTrackFolders.has(row.sourcePool.toLowerCase())) {
      staleTracks.push(`Slot ${i + 1} (${row.sourcePool})`);
    }
  });

  if (staleTracks.length > 0) {
    warnings.push({
      id: "tracks_stale_specific_refs",
      scope: "trackSpec",
      message: `Some track slots point to tracks that are no longer available: ${formatValidationList(staleTracks)}. Those slots will use random tracks instead.`
    });
  }

  return { errors, warnings, infos };
}