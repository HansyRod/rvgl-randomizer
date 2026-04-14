import { invoke } from "@tauri-apps/api/core";

export const handleTogglePack = async (packIndex, type, scanResult, updateCategoryCtx) => {
  if (!scanResult) return;
  const newResult = structuredClone(scanResult);
  const pack = newResult.contentPacks[packIndex];

  if (type === "cars") {
    pack.useCars = !pack.useCars;
    if (pack.useCars && pack.cars.length === 0 && pack.hasCars) {
      updateCategoryCtx("app", { isFetchingPack: true });
      try {
        pack.cars = await invoke("scan_cars_folder", { folderPath: `${pack.absolutePath}\\cars` });
      } catch (e) {
        console.error(e);
      } finally {
        updateCategoryCtx("app", { isFetchingPack: false });
      }
    }
  } else if (type === "tracks") {
    pack.useTracks = !pack.useTracks;
    if (pack.useTracks && pack.tracks.length === 0 && pack.hasTracks) {
      updateCategoryCtx("app", { isFetchingPack: true });
      try {
        pack.tracks = await invoke("scan_levels_folder", { folderPath: `${pack.absolutePath}\\levels` });
      } catch (e) {
        console.error(e);
      } finally {
        updateCategoryCtx("app", { isFetchingPack: false });
      }
    }
  }
  updateCategoryCtx("setup", { scanResult: newResult });
};
