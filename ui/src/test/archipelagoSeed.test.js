import assert from "node:assert/strict";
import test from "node:test";
import { ARCHIPELAGO_GOALS, getArchipelagoMetadata, getArchipelagoRequiredContent } from "../import/archipelagoSeed.js";

function config(includeDcCars = false, includeDcTrack = false) {
  return {
    metadata: { seed: "123456789", profileName: "ap_0123456789ab" },
    archipelago: { slotName: "My Player", goal: "platinum_cup" },
    global_options: { is_stock_cars: !includeDcCars, is_stock_tracks: !includeDcTrack },
    // Disabled DC definitions and item mappings can still be present in exports.
    dcCars: [{ folder: "bigvolt" }],
    tracks: [{ folder: "roof" }],
  };
}

test("AP summary keeps the exported identity and derives independent DC requirements from global options", () => {
  for (const includeDcCars of [false, true]) {
    for (const includeDcTrack of [false, true]) {
      const file = config(includeDcCars, includeDcTrack);
      const before = structuredClone(file);
      const metadata = getArchipelagoMetadata(file);
      assert.deepEqual(metadata, {
        slotName: "My Player", profileName: "ap_0123456789ab", seed: "123456789",
        goal: "platinum_cup", includeDcCars, includeDcTrack,
      });
      const expected = ["Stock Content"];
      if (includeDcCars) expected.push("Dreamcast Cars");
      if (includeDcTrack) expected.push("Dreamcast Track");
      assert.deepEqual(getArchipelagoRequiredContent(metadata, null).map(entry => entry.name), expected);
      assert.deepEqual(file, before);
    }
  }
});

test("non-AP and incomplete files are rejected before replacing an imported run", () => {
  for (const file of [null, {}, [], { metadata: { profileName: "normal" } }, { archipelago: "invalid" }]) {
    assert.throws(() => getArchipelagoMetadata(file), /not an Archipelago file/);
  }
  for (const [section, field, label] of [
    ["archipelago", "slotName", "slot name"],
    ["archipelago", "goal", "goal"],
    ["metadata", "profileName", "profile name"],
  ]) {
    for (const value of [undefined, null, "", "   ", 123]) {
      const file = config();
      file[section][field] = value;
      assert.throws(() => getArchipelagoMetadata(file), new RegExp(`valid ${label}`));
    }
  }
  for (const field of ["is_stock_cars", "is_stock_tracks"]) {
    const file = config();
    file.global_options[field] = "false";
    assert.throws(() => getArchipelagoMetadata(file), /required content settings/);
  }
});

test("required content uses AP discovery, combining stock cars and tracks and checking DC groups independently", () => {
  const metadata = getArchipelagoMetadata(config(true, true));
  const scanResult = { categories: {
    stockCars: { status: "complete" }, stockTracks: { status: "partial" },
    dcCars: { status: "complete" }, dcTracks: { status: "missing" },
  } };
  assert.deepEqual(getArchipelagoRequiredContent(metadata, scanResult), [
    { name: "Stock Content", available: false },
    { name: "Dreamcast Cars", available: true },
    { name: "Dreamcast Track", available: false },
  ]);
  scanResult.categories.stockTracks.status = "complete";
  assert.equal(getArchipelagoRequiredContent(metadata, scanResult)[0].available, true);
  scanResult.categories.stockCars.status = "partial";
  assert.equal(getArchipelagoRequiredContent(metadata, scanResult)[0].available, false);
});

test("all current goals have readable descriptions; unfamiliar goals remain readable without inventing compatibility policy", () => {
  for (const goal of ["platinum_cup", "carsanity", "tracksanity", "tracksanity_plus"]) {
    const file = config();
    file.archipelago.goal = goal;
    assert.ok(ARCHIPELAGO_GOALS[getArchipelagoMetadata(file).goal]);
  }
  const file = config();
  file.archipelago.goal = "future_goal";
  assert.equal(getArchipelagoMetadata(file).goal, "future_goal");
  assert.equal(ARCHIPELAGO_GOALS.future_goal, undefined);
});
