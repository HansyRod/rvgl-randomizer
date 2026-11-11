import assert from "node:assert/strict";
import test from "node:test";
import { STOCK_CARS, DC_CARS, STOCK_TRACKS } from "../utils/constants.js";
import {
  ARCHIPELAGO_CONTENT_CATEGORIES,
  discoverArchipelagoContent,
  resolveArchipelagoPacks,
} from "../setup/archipelagoContent.js";

const EXE_PATH = "D:/RVGL/rvgl.exe";
const STOCK_LEVELS = [...STOCK_TRACKS.filter(folder => folder !== "roof"), "stunts"];

function car(folderName, overrides = {}) {
  return { folderName, name: folderName, hasValidFile: true, isSystemCar: false, ...overrides };
}

function track(folderName, overrides = {}) {
  return { folderName, name: folderName, hasValidFile: true, trackType: folderName === "stunts" ? 2 : 0, ...overrides };
}

function pack(name, cars = [], tracks = [], overrides = {}) {
  return {
    name, absolutePath: `D:/RVGL/packs/${name}`,
    hasCars: cars.length > 0, hasTracks: tracks.length > 0,
    useCars: false, useTracks: false, cars, tracks, ...overrides,
  };
}

function preferredPacks() {
  return [
    pack("game_files", STOCK_CARS.map(folder => car(folder)), STOCK_LEVELS.map(folder => track(folder))),
    pack("rvgl_dcpack", DC_CARS.map(folder => car(folder)), [track("roof")]),
  ];
}

function launcher(contentPacks) {
  return { installType: "launcher", contentPacks };
}

function classic(cars = [...STOCK_CARS, ...DC_CARS].map(folder => car(folder)), tracks = [...STOCK_LEVELS, "roof"].map(folder => track(folder))) {
  return { installType: "classic", cars, tracks };
}

function unexpectedScan() {
  assert.fail("No fallback pack should be scanned");
}

test("discovery waits for an installation and scan", async () => {
  assert.equal(await discoverArchipelagoContent(null, EXE_PATH, unexpectedScan), null);
  assert.equal(await discoverArchipelagoContent(classic(), "", unexpectedScan), null);
});

test("classic discovery reports all categories and folder sources, including Stunt Arena", async () => {
  const discovery = await discoverArchipelagoContent(classic(), "D:\\RVGL\\rvgl.exe", unexpectedScan);
  for (const [id, definition] of Object.entries(ARCHIPELAGO_CONTENT_CATEGORIES)) {
    const category = discovery.categories[id];
    assert.equal(category.status, "complete");
    assert.equal(category.items.length, definition.folders.length);
    assert.deepEqual(category.missingFolders, []);
    assert.deepEqual(category.sources, [{ packName: null, path: `D:/RVGL/${definition.type === "cars" ? "cars" : "levels"}` }]);
  }
  assert.equal(discovery.categories.stockCars.items.length, 28);
  assert.equal(discovery.categories.dcCars.items.length, 14);
  assert.equal(discovery.categories.stockTracks.items.length, 14);
  assert.equal(discovery.categories.stockTracks.items.find(item => item.folderName === "stunts").trackType, 2);
  assert.deepEqual(resolveArchipelagoPacks(discovery), []);
});

test("missing Stunt Arena makes stock tracks partial and DC requirements stay independent", async () => {
  const scan = classic(STOCK_CARS.map(folder => car(folder)), STOCK_TRACKS.map(folder => track(folder)));
  const discovery = await discoverArchipelagoContent(scan, EXE_PATH, unexpectedScan);
  assert.equal(discovery.categories.stockCars.status, "complete");
  assert.equal(discovery.categories.dcCars.status, "missing");
  assert.equal(discovery.categories.dcTracks.status, "complete");
  assert.equal(discovery.categories.stockTracks.status, "partial");
  assert.deepEqual(discovery.categories.stockTracks.missingFolders, ["stunts"]);

  const withoutRoof = await discoverArchipelagoContent(classic(undefined, STOCK_LEVELS.map(folder => track(folder))), EXE_PATH);
  assert.equal(withoutRoof.categories.dcCars.status, "complete");
  assert.equal(withoutRoof.categories.dcTracks.status, "missing");
});

test("folder matching ignores case but rejects missing primary files, system cars and wrong track types", async () => {
  const discovery = await discoverArchipelagoContent(classic([
    car("RC"), car("mite", { hasValidFile: false }), car("phat", { isSystemCar: true }), car("custom_car"),
  ], [track("NHOOD1"), track("garden1", { trackType: 2 }), track("roof", { hasValidFile: false }), track("stunts", { trackType: 0 })]), EXE_PATH);
  assert.deepEqual(discovery.categories.stockCars.items.map(item => item.folderName), ["RC"]);
  assert.deepEqual(discovery.categories.stockTracks.items.map(item => item.folderName), ["NHOOD1"]);
  assert.equal(discovery.categories.dcTracks.status, "missing");
  assert.ok(discovery.categories.stockTracks.missingFolders.includes("stunts"));
});

test("complete preferred packs need no fallback reads and disabled pool flags are untouched", async () => {
  const scan = launcher([...preferredPacks(), pack("custom", [], [], { hasCars: true, hasTracks: true })]);
  const before = structuredClone(scan);
  const discovery = await discoverArchipelagoContent(scan, EXE_PATH, unexpectedScan);
  for (const category of Object.values(discovery.categories)) {
    assert.equal(category.status, "complete");
  }
  assert.deepEqual(discovery.categories.stockTracks.sources.map(source => source.packName), ["game_files"]);
  assert.deepEqual(discovery.categories.dcCars.sources.map(source => source.packName), ["rvgl_dcpack"]);
  assert.deepEqual(scan, before);
});

test("missing preferred packs scan fallbacks in stable order and stop when content is complete", async () => {
  const calls = [];
  const scan = launcher([
    pack("z_unused", [], [], { hasCars: true, hasTracks: true }),
    pack("a_content", [], [], { hasCars: true, hasTracks: true }),
  ]);
  const discovery = await discoverArchipelagoContent(scan, EXE_PATH, async candidate => {
    calls.push(candidate.name);
    return pack(candidate.name, [...STOCK_CARS, ...DC_CARS, "custom_car"].map(folder => car(folder)), [...STOCK_LEVELS, "roof", "custom_track"].map(folder => track(folder)));
  });
  assert.deepEqual(calls, ["a_content"]);
  assert.ok(Object.values(discovery.categories).every(category => category.status === "complete"));
  assert.equal(discovery.packs[0].cars.some(item => item.folderName === "custom_car"), false);
  assert.deepEqual(resolveArchipelagoPacks(discovery), ["a_content"]);
});

test("partial preferred content can be completed from multiple fallback sources", async () => {
  const [stock, dc] = preferredPacks();
  stock.cars = stock.cars.filter(item => !["rc", "mite"].includes(item.folderName));
  const contents = {
    a_pack: pack("a_pack", [car("rc")]),
    b_pack: pack("b_pack", [car("mite")]),
  };
  const scan = launcher([stock, dc, pack("b_pack", [], [], { hasCars: true }), pack("a_pack", [], [], { hasCars: true })]);
  const before = structuredClone(scan);
  const discovery = await discoverArchipelagoContent(scan, EXE_PATH, async candidate => contents[candidate.name]);
  assert.equal(discovery.categories.stockCars.status, "complete");
  assert.deepEqual(discovery.categories.stockCars.items.slice(0, 2).map(item => item.source.packName), ["a_pack", "b_pack"]);
  assert.deepEqual(new Set(discovery.categories.stockCars.sources.map(source => source.packName)), new Set(["game_files", "a_pack", "b_pack"]));
  assert.deepEqual(resolveArchipelagoPacks(discovery), ["a_pack", "b_pack", "game_files", "rvgl_dcpack"]);
  assert.deepEqual(scan, before);
});

test("track-only gaps skip car-only packs and discover stunts from fallback levels", async () => {
  const [stock, dc] = preferredPacks();
  stock.tracks = stock.tracks.filter(item => item.folderName !== "stunts");
  const calls = [];
  const discovery = await discoverArchipelagoContent(launcher([
    stock, dc, pack("a_cars", [], [], { hasCars: true }), pack("b_music"),
    pack("c_levels", [], [], { hasTracks: true }),
  ]), EXE_PATH, async candidate => {
    calls.push(candidate.name);
    return pack(candidate.name, [], [track("stunts")]);
  });
  assert.deepEqual(calls, ["c_levels"]);
  assert.equal(discovery.categories.stockTracks.status, "complete");
  assert.equal(discovery.categories.stockTracks.items.find(item => item.folderName === "stunts").source.path, "D:/RVGL/packs/c_levels/levels");
});

test("fallback scan failures are reported and other sources can still complete the content", async () => {
  const [stock, dc] = preferredPacks();
  stock.cars = stock.cars.filter(item => item.folderName !== "rc");
  const discovery = await discoverArchipelagoContent(launcher([
    stock, dc, pack("a_broken", [], [], { hasCars: true }), pack("b_good", [], [], { hasCars: true }),
  ]), EXE_PATH, async candidate => {
    if (candidate.name === "a_broken") {
      throw new Error("Cannot read pack");
    }
    return pack(candidate.name, [car("rc")]);
  });
  assert.equal(discovery.categories.stockCars.status, "complete");
  assert.deepEqual(discovery.scanErrors, [{ packName: "a_broken", path: "D:/RVGL/packs/a_broken", message: "Cannot read pack" }]);
});

test("preferred pack names are case-insensitive and input enumeration cannot change the result", async () => {
  const [stock, dc] = preferredPacks();
  stock.name = "GAME_FILES";
  dc.name = "RVGL_DCPACK";
  const first = await discoverArchipelagoContent(launcher([stock, dc]), EXE_PATH, unexpectedScan);
  const second = await discoverArchipelagoContent(launcher([dc, stock]), EXE_PATH, unexpectedScan);
  assert.deepEqual(first, second);
  assert.equal(first.categories.stockCars.items[0].source.packName, "GAME_FILES");
  assert.deepEqual(resolveArchipelagoPacks(first), ["GAME_FILES", "RVGL_DCPACK"]);
});

test("fallback reads use the existing native scan without enabling Randomizer pools", async () => {
  const previousWindow = globalThis.window;
  const calls = [];
  globalThis.window = { __TAURI_INTERNALS__: { invoke: async (command, args) => {
    calls.push({ command, args });
    return pack("fallback", [car("rc")]);
  } } };
  try {
    const discovery = await discoverArchipelagoContent(launcher([pack("fallback", [], [], { hasCars: true })]), EXE_PATH);
    assert.equal(discovery.categories.stockCars.items.length, 1);
    assert.deepEqual(calls, [{ command: "scan_pack_folder", args: {
      folderPath: "D:/RVGL/packs/fallback", useCars: false, useTracks: false,
    } }]);
  } finally {
    if (previousWindow === undefined) {
      delete globalThis.window;
    } else {
      globalThis.window = previousWindow;
    }
  }
});

test("whole-pack ordering preserves a preferred duplicate while loading a fallback for a different car", async () => {
  const [stock, dc] = preferredPacks();
  stock.cars = stock.cars.filter(item => item.folderName !== "mite");
  const discovery = await discoverArchipelagoContent(launcher([
    stock, dc, pack("z_fallback", [], [], { hasCars: true }),
  ]), EXE_PATH, async () => pack("z_fallback", [car("mite"), car("rc", { name: "Fallback replacement" })]));
  assert.equal(discovery.categories.stockCars.items.find(item => item.folderName === "rc").source.packName, "game_files");
  const resolution = resolveArchipelagoPacks(discovery);
  assert.deepEqual(resolution, ["z_fallback", "game_files", "rvgl_dcpack"]);
});

test("optional DC source packs are included only for requested categories", async () => {
  const [stock, dc] = preferredPacks();
  const discovery = await discoverArchipelagoContent(launcher([dc, stock]), EXE_PATH, unexpectedScan);
  assert.deepEqual(resolveArchipelagoPacks(discovery), ["game_files", "rvgl_dcpack"]);
  assert.deepEqual(resolveArchipelagoPacks(discovery, ["stockCars", "stockTracks"]), ["game_files"]);
  assert.deepEqual(resolveArchipelagoPacks(discovery, ["dcCars", "dcTracks"]), ["rvgl_dcpack"]);
});
