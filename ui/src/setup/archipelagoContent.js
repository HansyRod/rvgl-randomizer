import { invoke } from "@tauri-apps/api/core";
import { STOCK_CARS, DC_CARS, STOCK_TRACKS } from "../utils/constants";

export const ARCHIPELAGO_CONTENT_CATEGORIES = {
  stockCars: { type: "cars", preferredPack: "game_files", folders: STOCK_CARS },
  dcCars: { type: "cars", preferredPack: "rvgl_dcpack", folders: DC_CARS },
  stockTracks: {
    type: "tracks",
    preferredPack: "game_files",
    folders: [...STOCK_TRACKS.filter(folder => folder !== "roof"), "stunts"],
  },
  dcTracks: { type: "tracks", preferredPack: "rvgl_dcpack", folders: ["roof"] },
};

const PREFERRED_PACKS = ["game_files", "rvgl_dcpack"];

function comparePacks(a, b) {
  const nameA = a.name.toLowerCase();
  const nameB = b.name.toLowerCase();
  if (nameA < nameB) {
    return -1;
  }
  if (nameA > nameB) {
    return 1;
  }
  return 0;
}

function normalizePath(path) {
  return path.replace(/\\/g, "/").replace(/\/+$/, "");
}

function isAvailable(item, type, folder) {
  if (item.folderName.toLowerCase() !== folder || !item.hasValidFile) {
    return false;
  }
  if (type === "cars") {
    return !item.isSystemCar;
  }
  return item.trackType === (folder === "stunts" ? 2 : 0);
}

function contentSource(pack, type) {
  const directory = type === "cars" ? "cars" : "levels";
  return { packName: pack.name, path: `${normalizePath(pack.absolutePath)}/${directory}` };
}

function summarizeCategory(packs, definition) {
  const preferred = packs.filter(pack => pack.name?.toLowerCase() === definition.preferredPack);
  const others = packs.filter(pack => pack.name?.toLowerCase() !== definition.preferredPack);
  const orderedPacks = [...preferred, ...others];
  const items = [];
  const missingFolders = [];
  const sources = [];

  for (const folder of definition.folders) {
    let found = false;
    for (const pack of orderedPacks) {
      const item = (pack[definition.type] || []).find(candidate => isAvailable(candidate, definition.type, folder));
      if (item) {
        const source = contentSource(pack, definition.type);
        items.push({ ...item, source });
        if (!sources.some(entry => entry.packName === source.packName && entry.path === source.path)) {
          sources.push(source);
        }
        found = true;
        break;
      }
    }
    if (!found) {
      missingFolders.push(folder);
    }
  }

  let status = "complete";
  if (items.length === 0) {
    status = "missing";
  } else if (missingFolders.length > 0) {
    status = "partial";
  }
  return { status, items, missingFolders, sources };
}

function summarizeContent(packs) {
  return Object.fromEntries(Object.entries(ARCHIPELAGO_CONTENT_CATEGORIES)
    .map(([id, definition]) => [id, summarizeCategory(packs, definition)]));
}

async function scanContentPack(pack) {
  return invoke("scan_pack_folder", {
    folderPath: pack.absolutePath,
    useCars: false,
    useTracks: false,
  });
}

// Accept a fresh scan_install result. Preferred packs have already been read eagerly.
// Fallback scans stay local to discovery and never change Randomizer pack selections.
export async function scanArchipelagoContent(scanResult, installPath, scanPack = scanContentPack) {
  if (!scanResult || !installPath) {
    return null;
  }

  if (scanResult.installType === "classic") {
    const root = normalizePath(installPath).replace(/\/[^/]+$/, "");
    const packs = [{
      name: null,
      absolutePath: root,
      cars: scanResult.cars,
      tracks: scanResult.tracks,
    }];
    return { categories: summarizeContent(packs), scanErrors: [] };
  }

  const contentPacks = [...(scanResult.contentPacks || [])].sort(comparePacks);
  const packs = contentPacks.filter(pack => PREFERRED_PACKS.includes(pack.name.toLowerCase()));
  const fallbacks = contentPacks.filter(pack => !PREFERRED_PACKS.includes(pack.name.toLowerCase()));
  const scanErrors = [];
  let categories = summarizeContent(packs);

  for (const pack of fallbacks) {
    const needsCars = categories.stockCars.status !== "complete" || categories.dcCars.status !== "complete";
    const needsTracks = categories.stockTracks.status !== "complete" || categories.dcTracks.status !== "complete";
    if (!needsCars && !needsTracks) {
      break;
    }
    if (!(needsCars && pack.hasCars) && !(needsTracks && pack.hasTracks)) {
      continue;
    }

    try {
      packs.push(await scanPack(pack));
      categories = summarizeContent(packs);
    } catch (error) {
      scanErrors.push({
        packName: pack.name,
        path: pack.absolutePath,
        message: error instanceof Error ? error.message : String(error),
      });
    }
  }

  return { categories, scanErrors };
}

// Resolve only categories needed by the run. Loading a source pack loads both its cars and tracks.
// This returns content packs only; launch-time support/extra packs are added separately.
export function resolveArchipelagoPacks(discovery, categoryIds = Object.keys(ARCHIPELAGO_CONTENT_CATEGORIES)) {
  if (!discovery) {
    return [];
  }

  const categories = categoryIds.map(id => discovery.categories[id]);
  const selectedNames = [...new Set(categories.flatMap(category => category.sources.map(source => source.packName)).filter(Boolean))];
  const preferred = selectedNames.filter(name => PREFERRED_PACKS.includes(name.toLowerCase()));
  const others = selectedNames.filter(name => !PREFERRED_PACKS.includes(name.toLowerCase()));
  // Later packs overwrite earlier files. Put preferred packs last.
  const ordered = [...others, ...preferred];

  return ordered;
}
