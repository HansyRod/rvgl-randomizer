import assert from "node:assert/strict";
import test from "node:test";
import { getArchipelagoPacks } from "../play/archipelagoPacks.js";

function pack(name, overrides = {}) {
  return { name, hasCars: false, hasTracks: false, hasOtherContent: false, useCars: false, useTracks: false, ...overrides };
}

function discovery(sources = {}) {
  return { categories: Object.fromEntries(Object.entries({
    stockCars: "game_files", stockTracks: "game_files", dcCars: "dc_cars", dcTracks: "dc_track", ...sources,
  }).map(([id, packName]) => [id, { sources: [{ packName }] }])) };
}

test("AP packs follow independent file requirements rather than Randomizer selections", () => {
  const scanResult = { contentPacks: [
    pack("game_files", { hasCars: true, hasTracks: true }),
    pack("dc_cars", { hasCars: true }), pack("dc_track", { hasTracks: true }),
    pack("randomizer_only", { hasCars: true, useCars: true }),
  ] };
  const before = structuredClone(scanResult);
  for (const includeDcCars of [false, true]) {
    for (const includeDcTrack of [false, true]) {
      const result = getArchipelagoPacks(scanResult, discovery(), { includeDcCars, includeDcTrack }, []);
      const expected = ["game_files"];
      if (includeDcCars) expected.push("dc_cars");
      if (includeDcTrack) expected.push("dc_track");
      assert.deepEqual(new Set(result.selectedContentPacks.map(entry => entry.name)), new Set(expected));
      assert.equal(result.packlist.includes("randomizer_only"), false);
    }
  }
  assert.deepEqual(scanResult, before);
});

test("AP extras include mixed content and exclude car/track-only packs and required packs", () => {
  const scanResult = { contentPacks: [
    pack("game_files", { hasCars: true, hasOtherContent: true }),
    pack("rvgl_win64"), pack("music", { hasOtherContent: true }),
    pack("cars_and_music", { hasCars: true, hasOtherContent: true, useCars: true }),
    pack("tracks_and_sounds", { hasTracks: true, hasOtherContent: true }),
    pack("cars_only", { hasCars: true }), pack("tracks_only", { hasTracks: true }),
    pack("both_only", { hasCars: true, hasTracks: true }),
  ] };
  const extras = ["music", "cars_and_music", "tracks_and_sounds", "cars_only", "missing_pack", "game_files"];
  const result = getArchipelagoPacks(scanResult, discovery(), {}, extras);
  assert.deepEqual(result.extraPackCandidates.map(entry => entry.name), ["music", "cars_and_music", "tracks_and_sounds"]);
  assert.deepEqual(result.packlist, ["music", "cars_and_music", "tracks_and_sounds", "game_files", "rvgl_win64", "rvgl_assets"]);
  assert.deepEqual(extras, ["music", "cars_and_music", "tracks_and_sounds", "cars_only", "missing_pack", "game_files"]);
});

test("AP packs load extras, required content, then automatic packs, with case-insensitive deduplication", () => {
  const scanResult = { contentPacks: [
    pack("GAME_FILES", { hasCars: true }), pack("RVGL_DCPACK", { hasCars: true }),
    pack("fallback", { hasTracks: true }), pack("extras", { hasOtherContent: true }),
    pack("RVGL_WIN64", { hasOtherContent: true }),
  ] };
  const result = getArchipelagoPacks(scanResult, discovery({
    stockCars: "GAME_FILES", stockTracks: "fallback", dcCars: "RVGL_DCPACK", dcTracks: "RVGL_DCPACK",
  }), { includeDcCars: true, includeDcTrack: true }, ["extras", "RVGL_WIN64", "RVGL_DCPACK"]);
  assert.deepEqual(result.packlist, ["extras", "GAME_FILES", "fallback", "RVGL_DCPACK", "rvgl_win64", "rvgl_assets"]);
  assert.equal(new Set(result.packlist.map(name => name.toLowerCase())).size, result.packlist.length);
});

test("fallback installs omit absent game_files; present game_files remains automatic and loads last", () => {
  const fallback = pack("fallback", { hasCars: true, hasTracks: true });
  const sources = discovery({ stockCars: "fallback", stockTracks: "fallback" });
  const without = getArchipelagoPacks({ contentPacks: [fallback] }, sources, {}, []);
  assert.deepEqual(without.packlist, ["fallback", "rvgl_win64", "rvgl_assets"]);
  const withGameFiles = getArchipelagoPacks({ contentPacks: [fallback, pack("game_files")] }, sources, {}, []);
  assert.deepEqual(withGameFiles.autoPacks, ["rvgl_win64", "rvgl_assets", "game_files"]);
  assert.deepEqual(withGameFiles.packlist, ["fallback", "rvgl_win64", "rvgl_assets", "game_files"]);
});

test("required packs combine multiple sources once and tolerate discovery not being ready", () => {
  const scanResult = { contentPacks: [pack("game_files"), pack("a_pack"), pack("b_pack")] };
  const sources = discovery();
  sources.categories.stockCars.sources = [{ packName: "game_files" }, { packName: "a_pack" }, { packName: "b_pack" }];
  sources.categories.stockTracks.sources = [{ packName: "a_pack" }, { packName: null }];
  const result = getArchipelagoPacks(scanResult, sources, {}, []);
  assert.deepEqual(result.packlist, ["game_files", "a_pack", "b_pack", "rvgl_win64", "rvgl_assets"]);
  assert.equal(result.selectedContentPacks.length, 3);
  const pending = getArchipelagoPacks(scanResult, null, {}, []);
  assert.deepEqual(pending.selectedContentPacks, []);
});
