import { STOCK_CARS, STOCK_TRACKS } from "../utils/constants";

function isEligibleCar(car) {
  return car && !car.isSystemCar && car.hasValidFile;
}

function isEligibleTrack(track) {
  return track && track.hasValidFile && track.trackType === 0;
}

export function getAllCarsFromScan(scanResult) {
  if (!scanResult) return [];
  const cars = scanResult.installType === "classic"
    ? (scanResult.cars || [])
    : (scanResult.contentPacks || [])
    .filter(p => p.useCars)
    .flatMap(p => p.cars);

  return cars.filter(isEligibleCar);
}

export function getAllTracksFromScan(scanResult) {
  if (!scanResult) return [];
  const tracks = scanResult.installType === "classic"
    ? (scanResult.tracks || [])
    : (scanResult.contentPacks || [])
    .filter(p => p.useTracks)
    .flatMap(p => p.tracks);

  return tracks.filter(isEligibleTrack);
}

export function getIsStockCars(scanResult) {
  const allCars = getAllCarsFromScan(scanResult);
  if (allCars.length !== 28) return false;
  
  const stockSet = new Set(STOCK_CARS.map(c => c.toLowerCase()));
  return allCars.every(c => stockSet.has(c.folderName.toLowerCase()));
}

export function getIsStockTracks(scanResult) {
  const allTracks = getAllTracksFromScan(scanResult);
  if (allTracks.length !== 13) return false;
  
  const baseTracksWithoutRoof = new Set(STOCK_TRACKS.map(t => t.toLowerCase()).filter(t => t !== "roof"));
  return allTracks.every(t => baseTracksWithoutRoof.has(t.folderName.toLowerCase()));
}

export function formatValidationList(items, maxVisible = 3) {
  const list = (items || []).filter(Boolean);
  if (list.length <= maxVisible) {
    return list.join(", ");
  }

  const visible = list.slice(0, maxVisible).join(", ");
  return `${visible}, +${list.length - maxVisible} more`;
}
