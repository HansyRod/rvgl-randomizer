import { invoke } from "@tauri-apps/api/core";

export async function scanInstallation(path, previousPath, previousScanResult) {
  const result = await invoke("scan_install", { executablePath: path });
  if (path !== previousPath || result.installType !== "launcher" || previousScanResult?.installType !== "launcher") {
    return result;
  }

  const previousPacks = new Map((previousScanResult.contentPacks || []).map(pack => [pack.name.toLowerCase(), pack]));
  const contentPacks = await Promise.all(result.contentPacks.map(async pack => {
    const previousPack = previousPacks.get(pack.name.toLowerCase());
    if (!previousPack) {
      return pack;
    }

    const useCars = Boolean(previousPack.useCars && pack.hasCars);
    const useTracks = Boolean(previousPack.useTracks && pack.hasTracks);
    // The native installation scan only reads its default-enabled content.
    // Load other selected packs so restoring their flags also restores their content.
    if ((useCars && !pack.useCars) || (useTracks && !pack.useTracks)) {
      return invoke("scan_pack_folder", {
        folderPath: pack.absolutePath,
        useCars,
        useTracks,
      });
    }
    return { ...pack, useCars, useTracks };
  }));

  return { ...result, contentPacks };
}
