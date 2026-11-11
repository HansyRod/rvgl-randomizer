import { getArchipelagoCategoryIds } from "../import/archipelagoSeed";

export function getArchipelagoPacks(scanResult, discovery, metadata, extraPacks) {
  const allPacks = scanResult?.contentPacks || [];
  const categoryIds = getArchipelagoCategoryIds(metadata);
  // Resolve only categories needed by the run. Loading a source pack loads both its cars and tracks.
  const sources = categoryIds.flatMap(id => discovery?.categories[id].sources || []);
  const requiredNames = [...new Set(sources.map(source => source.packName).filter(Boolean))];
  const selectedContentPacks = requiredNames.map(name => allPacks.find(pack => pack.name === name)).filter(Boolean);
  // Add launch-time support/extra packs to the required content packs.
  const autoPacks = ["rvgl_win64", "rvgl_assets"];
  const gameFiles = allPacks.find(pack => pack.name.toLowerCase() === "game_files");
  if (gameFiles && !requiredNames.includes(gameFiles.name)) {
    autoPacks.push(gameFiles.name);
  }

  const required = new Set([...autoPacks, ...requiredNames].map(name => name.toLowerCase()));
  const extraPackCandidates = allPacks.filter(pack => !required.has(pack.name.toLowerCase())
    && ((!pack.hasCars && !pack.hasTracks) || pack.hasOtherContent));
  const selectedExtras = extraPackCandidates.filter(pack => extraPacks.includes(pack.name)).map(pack => pack.name);
  const names = [...new Map([...autoPacks, ...selectedExtras, ...requiredNames].map(name => [name.toLowerCase(), name])).values()];
  // Later packs overwrite earlier files, including files supplied by extras. Put preferred packs last.
  const preferred = name => ["game_files", "rvgl_dcpack"].includes(name.toLowerCase());
  const packlist = [...names.filter(name => !preferred(name)), ...names.filter(preferred)];

  return { autoPacks, selectedContentPacks, extraPackCandidates, packlist };
}
