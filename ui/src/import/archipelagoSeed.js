import { validateProfileName } from "../validation/generateValidators";

export const ARCHIPELAGO_GOALS = {
  platinum_cup: "Platinum Cup: Win Platinum Cup.",
  carsanity: "Carsanity: Win a Single Race with every enabled car.",
  tracksanity: "Tracksanity: Win a Single Race on every enabled track.",
  tracksanity_plus: "Tracksanity Plus: Win a Single Race, find the Practice star, and beat the Normal Time Trial on every enabled track.",
};

export function getArchipelagoMetadata(config) {
  if (!config?.archipelago || typeof config.archipelago !== "object") {
    throw new Error("This is not an Archipelago file. Select the .rvgl.json generated for your player slot.");
  }
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
