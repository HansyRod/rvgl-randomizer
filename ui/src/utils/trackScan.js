import { invoke } from "@tauri-apps/api/core";

export function trackNeedsRefresh(track) {
  if (track?.trackLengthNormal == null) return true;
  return track.hasReversed && track.trackLengthReverse == null;
}

export function scanResultNeedsRefresh(scanResult) {
  if (!scanResult) return false;

  // Launcher packs are checked individually by refreshTracks so that pack
  // selections and cached track lists can be preserved.
  if (scanResult.installType !== "classic") return true;

  return (scanResult.tracks || []).some(trackNeedsRefresh);
}

export async function refreshTracks(scanResult, installPath) {
  if (!scanResult) return scanResult;

  if (scanResult.installType === "classic") {
    if (!scanResultNeedsRefresh(scanResult) || !installPath) return scanResult;

    const installRoot = installPath.replace(/[\\/]?[^\\/]*$/, "");
    const tracks = await invoke("scan_levels_folder", {
      folderPath: `${installRoot}\\levels`,
    });
    return { ...scanResult, tracks };
  }

  const contentPacks = await Promise.all((scanResult.contentPacks || []).map(async (pack) => {
    const tracks = pack.tracks || [];
    const shouldRefresh = (pack.useTracks && pack.hasTracks && tracks.length === 0)
      || tracks.some(trackNeedsRefresh);

    if (!shouldRefresh) return pack;

    try {
      const refreshedTracks = await invoke("scan_levels_folder", {
        folderPath: `${pack.absolutePath}\\levels`,
      });
      return { ...pack, tracks: refreshedTracks };
    } catch (error) {
      console.error(`Failed to refresh tracks for content pack ${pack.name}:`, error);
      return pack;
    }
  }));

  return { ...scanResult, contentPacks };
}
