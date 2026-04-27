import { getAllTracksFromScan } from "./validationUtils";
import { STOCK_TRACKS } from "../utils/constants";

export function validateTrackSpec(trackSpecState, trackOptions, scanResult) {
  const errors = [];
  const warnings = [];

  const allTracks = getAllTracksFromScan(scanResult)
    .filter(t => t.hasValidFile && t.trackType === 0);

  if (allTracks.length === 0) {
    errors.push({
      id: "tracks_none_available",
      scope: "trackSpec",
      message: "No valid tracks are available in the current scan."
    });
    return { errors, warnings };
  }

  const allTrackFolders = new Set(allTracks.map(t => t.folderName.toLowerCase()));

  if (trackSpecState?.includeTracks === false) {

    // Check stale original-track references
    const staleStockRefs = [];

    // When stock tracks are not randomized, validate they exist in the source pool
    STOCK_TRACKS.forEach((track, i) => {
      if (!allTrackFolders.has(track)) {
        staleStockRefs.push(`slot ${i + 1} (${track})`);
      }
    })

    if (staleStockRefs.length > 0) {
      warnings.push({
        id: "tracks_stale_stock_refs",
        scope: "trackSpec",
        message: `Stock tracks are not randomized but some stock tracks are not found: ${staleStockRefs.join(", ")}. These tracks will be unavailable and cups using them will not work.`
      });
    }

    return { errors, warnings };
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
      message: `Some track slots reference folders not found in the current scan: ${staleTracks.join(", ")}. These will fall back to random picks.`
    });
  }

  return { errors, warnings };
}