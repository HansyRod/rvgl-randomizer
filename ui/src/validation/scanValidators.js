import { getAllCarsFromScan, getAllTracksFromScan, getIsStockTracks } from "./validationUtils";

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
      message: `Only ${validCars.length} eligible cars are available. Car randomization may have limited variety.`
    });
  }

  const isStockMode = getIsStockTracks(scanResult);
  const minTracks = isStockMode ? 13 : 14;

  if (validTracks.length < minTracks) {
    warnings.push({
      id: "scan_insufficient_tracks",
      scope: "scan",
      message: `Only ${validTracks.length} eligible tracks are available. Track randomization may have limited variety.`
    });
  }

  // Launcher-specific: no packs enabled
  if (scanResult.installType === "launcher") {
    const packsWithCars = (scanResult.contentPacks || []).filter(p => p.useCars);
    const packsWithTracks = (scanResult.contentPacks || []).filter(p => p.useTracks);

    if (packsWithCars.length === 0) {
      errors.push({
        id: "scan_no_car_packs",
        scope: "scan",
        message: "No car content packs are enabled. Enable at least one pack with cars before generating."
      });
    }

    if (packsWithTracks.length === 0) {
      errors.push({
        id: "scan_no_track_packs",
        scope: "scan",
        message: "No track content packs are enabled. Enable at least one pack with tracks before generating."
      });
    }
  }

  return { errors, warnings };
}
