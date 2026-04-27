import { getAllCarsFromScan, getAllTracksFromScan } from "./validationUtils";

export function validateScan(scanResult) {
  const errors = [];
  const warnings = [];

  const allCars = getAllCarsFromScan(scanResult);
  const allTracks = getAllTracksFromScan(scanResult);

  const validCars = allCars.filter(c => !c.isSystemCar && c.hasValidFile);
  const validTracks = allTracks.filter(t => t.hasValidFile && t.trackType === 0);

  if (validCars.length < 28) {
    warnings.push({
      id: "scan_insufficient_cars",
      scope: "scan",
      message: `Only ${validCars.length} valid cars found across enabled pools. At least 28 are needed to fill all stock slots without duplicates.`
    });
  }

  if (validTracks.length < 14) {
    warnings.push({
      id: "scan_insufficient_tracks",
      scope: "scan",
      message: `Only ${validTracks.length} valid tracks found. At least 14 are needed to fill the track spec.`
    });
  }

  // Launcher-specific: no packs enabled
  if (scanResult.installType === "launcher") {
    const packsWithCars = (scanResult.contentPacks || []).filter(p => p.useCars);
    const packsWithTracks = (scanResult.contentPacks || []).filter(p => p.useTracks);

    if (packsWithCars.length === 0) {
      warnings.push({
        id: "scan_no_car_packs",
        scope: "scan",
        message: "No content packs are enabled for cars. The randomizer will have no cars to draw from."
      });
    }

    if (packsWithTracks.length === 0) {
      warnings.push({
        id: "scan_no_track_packs",
        scope: "scan",
        message: "No content packs are enabled for tracks. The randomizer will have no tracks to draw from."
      });
    }
  }

  return { errors, warnings };
}