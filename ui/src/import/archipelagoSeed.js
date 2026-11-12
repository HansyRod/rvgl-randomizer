import { validateProfileName } from "../validation/generateValidators";

// Compatibility ceilings for this client.
// Patch releases preserve the contract; older major/minor versions remain accepted.
const SUPPORTED_RANDOMIZER_VERSION = { major: 0, minor: 3 };
const SUPPORTED_WORLD_VERSION = { major: 0, minor: 1 };

function validateVersion(value, field, supported, product) {
  if (typeof value !== "string" || !/^(0|[1-9]\d*)\.(0|[1-9]\d*)\.(0|[1-9]\d*)$/.test(value)) {
    throw new Error(`The Archipelago file has a missing or invalid ${field}.\nRegenerate the player file and multiworld with the current RVGL APWorld.`);
  }
  const [major, minor, patch] = value.split(".").map(Number);
  if (![major, minor, patch].every(Number.isSafeInteger)) {
    throw new Error(`The Archipelago file has an invalid ${field}.\nRegenerate the player file and multiworld with the current RVGL APWorld.`);
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
  const { slotName, goal } = config.archipelago;
  const { profileName, seed } = config.metadata || {};
  for (const [label, value] of [["slot name", slotName], ["profile name", profileName], ["goal", goal]]) {
    if (typeof value !== "string" || !value.trim()) {
      throw new Error(`The Archipelago file is missing a valid ${label}.`);
    }
  }
  const { is_stock_cars, is_stock_tracks } = config.global_options || {};
  if (typeof is_stock_cars !== "boolean" || typeof is_stock_tracks !== "boolean") {
    throw new Error("The Archipelago file is missing its required content settings.");
  }
  const profile = validateProfileName(profileName);
  if (profile.errors.length > 0) {
    throw new Error(profile.errors[0].message);
  }

  // DC definitions can exist even when disabled; the global options decide requirements.
  return { slotName, profileName, seed, goal, includeDcCars: !is_stock_cars, includeDcTrack: !is_stock_tracks };
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
