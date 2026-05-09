import { getAllCarsFromScan, getAllTracksFromScan, getIsStockCars, getIsStockTracks } from "./validationUtils";

export function validateScan(scanResult) {
  const errors = [];
  const warnings = [];

  const allCars = getAllCarsFromScan(scanResult);
  const allTracks = getAllTracksFromScan(scanResult);

  const isStockCarsMode = getIsStockCars(scanResult);
  const minCars = isStockCarsMode ? 28 : 42;

  if (allCars.length < minCars) {
    errors.push({
      id: "scan_insufficient_cars",
      scope: "scan",
      message: `Only ${allCars.length} eligible cars are available, but generation requires at least ${minCars}.`
    });
  }

  const isStockMode = getIsStockTracks(scanResult);
  const minTracks = isStockMode ? 13 : 14;

  if (allTracks.length < minTracks) {
    errors.push({
      id: "scan_insufficient_tracks",
      scope: "scan",
      message: `Only ${allTracks.length} eligible tracks are available, but generation requires at least ${minTracks}.`
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
