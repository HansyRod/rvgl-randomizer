import { STOCK_CARS, STOCK_TRACKS } from "../utils/constants";
import { getPlayableCarsFromScan, getPlayableTracksFromScan } from "../utils/scanContent";

export function getAllCarsFromScan(scanResult) {
  return getPlayableCarsFromScan(scanResult);
}

export function getAllTracksFromScan(scanResult) {
  return getPlayableTracksFromScan(scanResult);
}

export function isGenericTrackSpecPool(pool) {
  const normalized = String(pool || "").toLowerCase();
  return (
    !normalized ||
    normalized === "full random" ||
    normalized === "stock" ||
    normalized === "custom" ||
    normalized.startsWith("pack:")
  );
}

export function getTrackSpecAvailableFolders(trackSpecState, allTracks = []) {
  const allTrackFolders = new Set(
    (allTracks || [])
      .map(track => track.folderName?.toLowerCase())
      .filter(Boolean)
  );

  if (trackSpecState?.includeTracks) {
    return new Set(
      (trackSpecState?.tracks || [])
        .map(track => track.sourcePool?.toLowerCase())
        .filter(pool =>
          pool &&
          !isGenericTrackSpecPool(pool) &&
          (allTrackFolders.size === 0 || allTrackFolders.has(pool))
        )
    );
  }

  return new Set(
    (trackSpecState?.tracks || [])
      .map(track => track.id?.toLowerCase())
      .filter(folder => folder && (allTrackFolders.size === 0 || allTrackFolders.has(folder)))
  );
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
