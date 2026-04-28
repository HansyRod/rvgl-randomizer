export function getAllCarsFromScan(scanResult) {
  if (!scanResult) return [];
  if (scanResult.installType === "classic") return scanResult.cars || [];
  return (scanResult.contentPacks || [])
    .filter(p => p.useCars)
    .flatMap(p => p.cars)
    .filter(c => !c.isSystemCar && c.hasValidFile);
}

export function getAllTracksFromScan(scanResult) {
  if (!scanResult) return [];
  if (scanResult.installType === "classic") return scanResult.tracks || [];
  return (scanResult.contentPacks || [])
    .filter(p => p.useTracks)
    .flatMap(p => p.tracks)
    .filter(t => t.hasValidFile && t.trackType === 0);
}

export function formatValidationList(items, maxVisible = 3) {
  const list = (items || []).filter(Boolean);
  if (list.length <= maxVisible) {
    return list.join(", ");
  }

  const visible = list.slice(0, maxVisible).join(", ");
  return `${visible}, +${list.length - maxVisible} more`;
}
