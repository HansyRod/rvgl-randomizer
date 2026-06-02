export function getPlayableCarsFromScan(scanResult) {
  if (!scanResult) return [];

  const cars = scanResult.installType === "classic"
    ? (scanResult.cars || [])
    : (scanResult.contentPacks || [])
      .filter(pack => pack.useCars)
      .flatMap(pack => pack.cars);

  return cars.filter(car => car && !car.isSystemCar && car.hasValidFile);
}

export function getPlayableTracksFromScan(scanResult) {
  if (!scanResult) return [];

  const tracks = scanResult.installType === "classic"
    ? (scanResult.tracks || [])
    : (scanResult.contentPacks || [])
      .filter(pack => pack.useTracks)
      .flatMap(pack => pack.tracks);

  return tracks.filter(track => track && track.hasValidFile && track.trackType === 0);
}

export function indexByFolder(items) {
  const map = {};
  for (const item of items || []) {
    if (item?.folderName) map[item.folderName] = item;
  }
  return map;
}
