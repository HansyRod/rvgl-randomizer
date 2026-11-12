import assert from "node:assert/strict";
import test from "node:test";
import { ARCHIPELAGO_GOALS, getArchipelagoMetadata, getArchipelagoRequiredContent } from "../import/archipelagoSeed.js";

function config(includeDcCars = false, includeDcTrack = false) {
  return {
    metadata: { version: "0.3.0", seed: "123456789", profileName: "ap_0123456789ab" },
    archipelago: { worldVersion: "0.1.0", slotNumber: 1, slotName: "My Player", goal: "platinum_cup",
      locations: { "championship_win:4": 3000305 }, items: { "cup:4": 3000300 }, goalEventKeys: ["championship_win:4"] },
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
        slotName: "My Player", profileName: "ap_0123456789ab", seed: "123456789", generatedAt: undefined,
        goal: "platinum_cup", includeDcCars, includeDcTrack, yamlOptions: undefined,
      });
      const expected = ["Stock Content"];
      if (includeDcCars) expected.push("Dreamcast Cars");
      if (includeDcTrack) expected.push("Dreamcast Track");
      assert.deepEqual(getArchipelagoRequiredContent(metadata, null).map(entry => entry.name), expected);
      assert.deepEqual(file, before);
    }
  }
});

test("AP import preserves the generation timestamp and accepts older exports without one", () => {
  const file = config();
  file.metadata.generatedAt = "2026-10-24T09:20:54.681395400+00:00";
  assert.equal(getArchipelagoMetadata(file).generatedAt, file.metadata.generatedAt);
  delete file.metadata.generatedAt;
  assert.equal(getArchipelagoMetadata(file).generatedAt, undefined);
});

test("AP import accepts supported and older versions independently of patch differences", () => {
  for (const version of ["0.3.0", "0.3.99", "0.2.7", "0.0.0"]) {
    for (const worldVersion of ["0.1.0", "0.1.99", "0.0.0"]) {
      const file = config();
      file.metadata.version = version;
      file.archipelago.worldVersion = worldVersion;
      assert.doesNotThrow(() => getArchipelagoMetadata(file));
    }
  }
});

test("AP import retains YAML options including generic settings without changing their values", () => {
  const file = config();
  file.archipelago.yamlOptions = {
    logic_difficulty: "expert", include_single_race_wins: false, carsanity: "disabled",
    progression_balancing: 35, local_items: ["RC Bandit"], start_inventory: { "RC Bandit": 2 },
  };
  const before = structuredClone(file);
  assert.deepEqual(getArchipelagoMetadata(file).yamlOptions, before.archipelago.yamlOptions);
  assert.deepEqual(file, before);
});

test("AP import rejects newer major/minor versions and tells the user to update", () => {
  for (const [section, field, versions] of [
    ["metadata", "version", ["0.4.0", "0.10.0", "1.0.0"]],
    ["archipelago", "worldVersion", ["0.2.0", "0.10.0", "1.0.0"]],
  ]) {
    for (const version of versions) {
      const file = config();
      file[section][field] = version;
      assert.throws(() => getArchipelagoMetadata(file), error => {
        assert.ok(error.message.includes(`${section}.${field} ${version}`));
        assert.match(error.message, /Update RVGL Randomizer/);
        return true;
      });
    }
  }
});

test("AP import requires both version fields and rejects malformed or legacy-only versions", () => {
  for (const [section, field] of [["metadata", "version"], ["archipelago", "worldVersion"]]) {
    for (const value of [undefined, null, 1, "", "0.1", "v0.1.0", "0.1.0-beta", "0.1.0+build", " 0.1.0", "0.01.0", "-1.0.0", "0.1.9007199254740992"]) {
      const file = config();
      file[section][field] = value;
      assert.throws(() => getArchipelagoMetadata(file), error => {
        assert.ok(error.message.includes(`${section}.${field}`));
        assert.match(error.message, /Regenerate/);
        return true;
      });
    }
  }
  const file = config();
  delete file.archipelago.worldVersion;
  file.archipelago.schemaVersion = 1;
  assert.throws(() => getArchipelagoMetadata(file), /archipelago.worldVersion/);
});

test("non-AP and incomplete files are rejected before replacing an imported run", () => {
  for (const file of [null, {}, [], { metadata: { profileName: "normal" } }, { archipelago: "invalid" }]) {
    assert.throws(() => getArchipelagoMetadata(file), /not an Archipelago file/);
  }
  for (const [section, field, label] of [
    ["archipelago", "slotName", "slot name"],
    ["archipelago", "goal", "goal"],
    ["metadata", "profileName", "profile name"],
    ["metadata", "seed", "seed identity"],
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

test("AP import requires a positive slot number within the DLL integer range", () => {
  for (const slotNumber of [undefined, null, "1", 0, -1, 1.5, 2147483648]) {
    const file = config();
    file.archipelago.slotNumber = slotNumber;
    assert.throws(() => getArchipelagoMetadata(file), /invalid player slot number/);
  }
});

test("AP import validates mapping types, integer ranges and uniqueness within each map", () => {
  for (const field of ["locations", "items"]) {
    for (const value of [undefined, null, [], "invalid", 123,
      { first: "1" }, { first: null }, { first: 1.5 }, { first: NaN }, { first: Infinity },
      { first: -2147483649 }, { first: 2147483648 }, { first: 1, second: 1 }]) {
      const file = config();
      file.archipelago[field] = value;
      assert.throws(() => getArchipelagoMetadata(file), /invalid.*(location|item)/);
    }
  }
  const file = config();
  file.archipelago.locations = { "championship_win:4": 0, first: -2147483648, last: 2147483647 };
  file.archipelago.items = { "": 0, "filler:Battery": -2147483648, "cup:4": 2147483647 };
  assert.doesNotThrow(() => getArchipelagoMetadata(file));
});

test("AP import rejects empty, duplicate, malformed and unmapped goal requirements", () => {
  for (const goalEventKeys of [undefined, null, "championship_win:4", [], [null], [1], [""], ["   "],
    ["championship_win:4", "championship_win:4"], ["missing"], ["championship_win:4", "missing"], ["constructor"]]) {
    const file = config();
    file.archipelago.goalEventKeys = goalEventKeys;
    assert.throws(() => getArchipelagoMetadata(file), error => {
      assert.equal(error.message, "The Archipelago file has invalid goal requirements.\nRegenerate the player file and multiworld with the current RVGL APWorld.");
      return true;
    });
  }
});

test("AP import accepts multiple goal requirements without requiring dormant unlock tokens", () => {
  const file = config();
  file.archipelago.locations["single_race_win:nhood1"] = 2000001;
  file.archipelago.goalEventKeys.push("single_race_win:nhood1");
  file.archipelago.items = {};
  file.dcCars[0].customUnlock = { archipelagoItem: "car:bigvolt" };
  file.tracks[0].customUnlock = { archipelagoItem: "track:roof" };
  const before = structuredClone(file);
  assert.doesNotThrow(() => getArchipelagoMetadata(file));
  assert.deepEqual(file, before);
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
