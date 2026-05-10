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

export function hasAllStockCars(scanResult) {
  const allCars = getAllCarsFromScan(scanResult);
  const stockSet = new Set(STOCK_CARS.map(c => c.toLowerCase()));
  const availableStockCars = new Set(
    allCars
      .map((car) => car.folderName?.toLowerCase())
      .filter((folderName) => stockSet.has(folderName))
  );

  return availableStockCars.size === STOCK_CARS.length;
}

export function hasOnlyStockCarsLoaded(scanResult) {
  const allCars = getAllCarsFromScan(scanResult);
  if (allCars.length !== STOCK_CARS.length) return false;

  const stockSet = new Set(STOCK_CARS.map((car) => car.toLowerCase()));
  return allCars.every((car) => stockSet.has(car.folderName.toLowerCase()));
}

export function hasAllStockTracks(scanResult) {
  const allTracks = getAllTracksFromScan(scanResult);
  const baseTracksWithoutRoof = new Set(STOCK_TRACKS.map(t => t.toLowerCase()).filter(t => t !== "roof"));
  const availableStockTracks = new Set(
    allTracks
      .map((track) => track.folderName?.toLowerCase())
      .filter((folderName) => baseTracksWithoutRoof.has(folderName))
  );

  return availableStockTracks.size === baseTracksWithoutRoof.size;
}

export function hasOnlyStockTracksLoaded(scanResult) {
  const allTracks = getAllTracksFromScan(scanResult);
  if (allTracks.length !== STOCK_TRACKS.length - 1) return false;

  const baseTracksWithoutRoof = new Set(STOCK_TRACKS.map((track) => track.toLowerCase()).filter((track) => track !== "roof"));
  return allTracks.every((track) => baseTracksWithoutRoof.has(track.folderName.toLowerCase()));
}

export function formatValidationList(items, maxVisible = 3) {
  const list = (items || []).filter(Boolean);
  if (list.length <= maxVisible) {
    return list.join(", ");
  }

  const visible = list.slice(0, maxVisible).join(", ");
  return `${visible}, +${list.length - maxVisible} more`;
}
