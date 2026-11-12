import { validateProfileName } from "../validation/generateValidators";

// Compatibility ceilings for this client.
// Patch releases preserve the contract; older major/minor versions remain accepted.
const SUPPORTED_RANDOMIZER_VERSION = { major: 0, minor: 3 };
const SUPPORTED_WORLD_VERSION = { major: 0, minor: 1 };

function rejectArchipelagoFile(reason) {
  throw new Error(`${reason}\nRegenerate the player file and multiworld with the current RVGL APWorld.`);
}

// The DLL stores slot numbers and mapping IDs as signed 32-bit integers.
function isMappingId(value) {
  return Number.isInteger(value) && value >= -2147483648 && value <= 2147483647;
}

function validateMappings(archipelago) {
  for (const [map, label] of [[archipelago.locations, "location"], [archipelago.items, "item"]]) {
    if (!map || typeof map !== "object" || Array.isArray(map)) {
      rejectArchipelagoFile(`The Archipelago file has invalid ${label} information.`);
    }
    const ids = Object.values(map);
    if (!ids.every(isMappingId) || new Set(ids).size !== ids.length) {
      rejectArchipelagoFile(`The Archipelago file has invalid or duplicate ${label} IDs.`);
    }
  }
  const { goalEventKeys, locations } = archipelago;
  if (!Array.isArray(goalEventKeys) || goalEventKeys.length === 0
    || goalEventKeys.some(key => typeof key !== "string" || !key.trim())
    || new Set(goalEventKeys).size !== goalEventKeys.length
    || goalEventKeys.some(key => !Object.hasOwn(locations, key))) {
    rejectArchipelagoFile("The Archipelago file has invalid goal requirements.");
  }
}

function validateVersion(value, field, supported, product) {
  if (typeof value !== "string" || !/^(0|[1-9]\d*)\.(0|[1-9]\d*)\.(0|[1-9]\d*)$/.test(value)) {
    rejectArchipelagoFile(`The Archipelago file has a missing or invalid ${field}.`);
  }
  const [major, minor, patch] = value.split(".").map(Number);
  if (![major, minor, patch].every(Number.isSafeInteger)) {
    rejectArchipelagoFile(`The Archipelago file has an invalid ${field}.`);
  }
  if (major > supported.major || (major === supported.major && minor > supported.minor)) {
    throw new Error(`The Archipelago file uses ${field} ${value}.\nThis version supports ${product} versions through ${supported.major}.${supported.minor}.x.\nUpdate RVGL Randomizer to load this file.`);
  }
}

export const ARCHIPELAGO_GOALS = {
  platinum_cup: "Platinum Cup: Win Platinum Cup.",
  carsanity: "Carsanity: Win a Single Race with every enabled car.",
  tracksanity: "Tracksanity: Win a Single Race on every enabled track.",
  tracksanity_plus: "Tracksanity Plus: Win a Single Race, find the Practice star, and beat the Normal Time Trial on every enabled track.",
};

export function getArchipelagoMetadata(config) {
  if (!config?.archipelago || typeof config.archipelago !== "object") {
    throw new Error("This is not an Archipelago file.\nSelect the .rvgl.json generated for your player slot.");
  }
  validateVersion(config.archipelago.worldVersion, "archipelago.worldVersion", SUPPORTED_WORLD_VERSION, "RVGL APWorld");
  validateVersion(config.metadata?.version, "metadata.version", SUPPORTED_RANDOMIZER_VERSION, "RVGL Randomizer");
  const { slotName, goal, yamlOptions } = config.archipelago;
  const { profileName, seed, generatedAt } = config.metadata || {};
  for (const [label, value] of [["slot name", slotName], ["profile name", profileName], ["goal", goal], ["seed identity", seed]]) {
    if (typeof value !== "string" || !value.trim()) {
      rejectArchipelagoFile(`The Archipelago file is missing a valid ${label}.`);
    }
  }
  const { is_stock_cars, is_stock_tracks } = config.global_options || {};
  if (typeof is_stock_cars !== "boolean" || typeof is_stock_tracks !== "boolean") {
    rejectArchipelagoFile("The Archipelago file is missing its required content settings.");
  }
  const profile = validateProfileName(profileName);
  if (profile.errors.length > 0) {
    rejectArchipelagoFile(profile.errors[0].message);
  }
  if (!isMappingId(config.archipelago.slotNumber) || config.archipelago.slotNumber <= 0) {
    rejectArchipelagoFile("The Archipelago file has an invalid player slot number.");
  }
  validateMappings(config.archipelago);

  // DC definitions can exist even when disabled; the global options decide requirements.
  return {
    slotName,
    profileName,
    generatedAt,
    seed,
    goal,
    includeDcCars: !is_stock_cars,
    includeDcTrack: !is_stock_tracks,
    yamlOptions
  };
}

export function getArchipelagoCategoryIds(metadata) {
  const categoryIds = ["stockCars", "stockTracks"];
  if (metadata?.includeDcCars) {
    categoryIds.push("dcCars");
  }
  if (metadata?.includeDcTrack) {
    categoryIds.push("dcTracks");
  }
  return categoryIds;
}

export function getArchipelagoRequiredContent(metadata, scanResult) {
  const complete = id => scanResult?.categories?.[id]?.status === "complete";
  const requiredContent = [{ name: "Stock Content", available: complete("stockCars") && complete("stockTracks") }];
  if (metadata.includeDcCars) {
    requiredContent.push({ name: "Dreamcast Cars", available: complete("dcCars") });
  }
  if (metadata.includeDcTrack) {
    requiredContent.push({ name: "Dreamcast Track", available: complete("dcTracks") });
  }
  return requiredContent;
}
