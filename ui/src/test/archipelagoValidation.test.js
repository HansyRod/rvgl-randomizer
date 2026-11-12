import assert from "node:assert/strict";
import test from "node:test";
import { validateArchipelago } from "../validation/archipelagoValidators.js";
import { validateLaunch } from "../validation/launchValidators.js";
import { getArchipelagoMetadata } from "../import/archipelagoSeed.js";

function state() {
  const category = (folderName, packName, type) => ({ status: "complete", missingFolders: [],
    sources: [{ packName }], items: [{ folderName, source: { packName, path: `D:/RVGL/packs/${packName}/${type}` } }],
  });
  return {
    app: { mode: "archipelago" },
    setup: { installPath: "D:/RVGL/packs/rvgl_win64/rvgl.exe", scanResult: {
      installType: "launcher", contentPacks: ["game_files", "dc_cars", "dc_track"].map(name => ({
        name, hasCars: true, hasTracks: true, useCars: false, useTracks: false,
      })),
    } },
    generate: { generatedFilePath: "D:/normal.json", seedContext: null },
    archipelago: {
      import: { importedFilePath: "D:/AP.rvgl.json", seedMetadata: {
        slotName: "Player", profileName: "ap_0123456789ab", goal: "platinum_cup", includeDcCars: false, includeDcTrack: false,
      } },
      setup: { scanError: "", scanResult: { categories: {
        stockCars: category("rc", "game_files", "cars"), stockTracks: category("stunts", "game_files", "levels"),
        dcCars: category("bigvolt", "dc_cars", "cars"), dcTracks: category("roof", "dc_track", "levels"),
      }, scanErrors: [{ packName: "unused_fallback", message: "Unreadable" }] } },
      play: { extraPacks: [] },
    },
  };
}

function validate(current) {
  return validateArchipelago(current.setup, current.archipelago, false);
}

async function withNative(run, respond = () => true) {
  const previousWindow = globalThis.window;
  const calls = [];
  globalThis.window = { __TAURI_INTERNALS__: { invoke: async (command, args) => {
    calls.push({ command, args });
    const response = respond(command, args);
    if (response instanceof Error) throw response;
    if (command === "check_missing_paths" && response === true) return { files: [], directories: [] };
    return response;
  } } };
  try { await run(calls); }
  finally {
    if (previousWindow === undefined) delete globalThis.window;
    else globalThis.window = previousWindow;
  }
}

test("stock content is mandatory; DC errors follow the two file options independently", () => {
  for (const includeDcCars of [false, true]) {
    for (const includeDcTrack of [false, true]) {
      const current = state();
      Object.assign(current.archipelago.import.seedMetadata, { includeDcCars, includeDcTrack });
      const categories = current.archipelago.setup.scanResult.categories;
      categories.dcCars.status = "missing";
      categories.dcTracks.status = "missing";
      const expected = [];
      if (includeDcCars) expected.push("ap_content_dcCars");
      if (includeDcTrack) expected.push("ap_content_dcTracks");
      assert.deepEqual(validate(current).errors.map(issue => issue.id), expected);
      categories.stockCars.status = "partial";
      categories.stockCars.missingFolders = ["rc"];
      categories.stockTracks.status = "partial";
      categories.stockTracks.missingFolders = ["stunts"];
      const errors = validate(current).errors;
      assert.equal(errors.length, expected.length + 2);
      assert.ok(errors.every(issue => issue.scope === "scan"));
      assert.match(errors.find(issue => issue.id === "ap_content_stockTracks").message, /Stunt Arena.*stunts/);
    }
  }
});

test("scan failure/pending state is actionable and unused fallback errors do not block complete content", () => {
  const current = state();
  assert.deepEqual(validate(current).errors, []);
  current.archipelago.setup.scanResult = null;
  assert.equal(validate(current).errors[0].id, "ap_scan_failed");
  assert.deepEqual(validateArchipelago(current.setup, current.archipelago, true).errors, []);
  current.archipelago.setup.scanError = "Read failed";
  const errors = validate(current).errors;
  assert.equal(errors.length, 1);
  assert.match(errors[0].message, /Read failed.*Refresh/);
});

test("AP run/profile issues use Import scope and existing profile rules", () => {
  const current = state();
  current.archipelago.import.importedFilePath = "";
  assert.equal(validate(current).errors[0].scope, "import");
  current.archipelago.import.importedFilePath = "D:/AP.rvgl.json";
  for (const profileName of ["", "1234567890123456", "../other"]) {
    current.archipelago.import.seedMetadata.profileName = profileName;
    assert.ok(validate(current).errors.some(issue => issue.scope === "import" && issue.field === "profileName"));
  }
  current.archipelago.import.seedMetadata.profileName = "playeré";
  assert.deepEqual(validate(current).errors, []);
  assert.equal(validate(current).warnings[0].scope, "import");
  current.archipelago.import.seedMetadata.slotName = 123;
  assert.equal(validate(current).errors[0].id, "ap_metadata_missing");
});

test("import rejects an unsafe or overlong exported profile before selecting it", () => {
  for (const profileName of ["../../file", "1234567890123456"]) {
    assert.throws(() => getArchipelagoMetadata({ metadata: { version: "0.3.0", seed: "test", profileName }, archipelago: { worldVersion: "0.1.0", slotName: "Player", goal: "carsanity" },
      global_options: { is_stock_cars: true, is_stock_tracks: true } }), /Profile name/);
  }
});

test("live AP checks target only required primary files and the actual launch packs", async () => {
  for (const includeDcCars of [false, true]) {
    for (const includeDcTrack of [false, true]) {
      const current = state();
      Object.assign(current.archipelago.import.seedMetadata, { includeDcCars, includeDcTrack });
      await withNative(async calls => {
        assert.deepEqual((await validateLaunch(current)).errors, []);
        const paths = calls.find(call => call.command === "check_missing_paths").args;
        const expectedFiles = ["D:/RVGL/packs/game_files/cars/rc/parameters.txt", "D:/RVGL/packs/game_files/levels/stunts/stunts.inf"];
        if (includeDcCars) expectedFiles.push("D:/RVGL/packs/dc_cars/cars/bigvolt/parameters.txt");
        if (includeDcTrack) expectedFiles.push("D:/RVGL/packs/dc_track/levels/roof/roof.inf");
        assert.deepEqual(paths.files, expectedFiles);
        assert.equal(paths.directories.includes("D:/RVGL/packs/dc_cars"), includeDcCars);
        assert.equal(paths.directories.includes("D:/RVGL/packs/dc_track"), includeDcTrack);
        assert.ok(paths.directories.includes("D:/RVGL/packs/rvgl_assets"));
        assert.ok(calls.every(call => !call.command.startsWith("read_") && !call.command.startsWith("scan_")));
      });
    }
  }
});

test("deleted required content/packs and failed verification block launch and recover after repair", async () => {
  const current = state();
  let broken = false;
  await withNative(async () => {
    assert.deepEqual((await validateLaunch(current)).errors, []);
    broken = true;
    const errors = (await validateLaunch(current)).errors;
    assert.deepEqual(errors.map(issue => issue.id), ["ap_content_files_missing", "ap_launch_packs_missing"]);
    assert.ok(errors.every(issue => issue.scope === "scan"));
    broken = false;
    assert.deepEqual((await validateLaunch(current)).errors, []);
  }, (command, args) => command === "check_missing_paths"
    ? { files: broken ? [args.files[0]] : [], directories: broken ? [args.directories[0]] : [] } : true);
  await withNative(async () => {
    assert.equal((await validateLaunch(current)).errors[0].id, "ap_content_check_failed");
  }, command => command === "check_missing_paths" ? new Error("Cannot check paths") : true);
});

test("common missing launch resources use the shared findings and portraits remain a warning", async () => {
  const current = state();
  await withNative(async () => {
    const result = await validateLaunch(current);
    assert.deepEqual(result.errors.map(issue => issue.id), ["launch_config_missing", "launch_executable_missing", "launch_dll_missing", "launch_packlist_not_writable"]);
    assert.equal(result.errors[0].scope, "import");
    assert.equal(result.errors[1].scope, "scan");
    assert.equal(result.errors[2].scope, "launch");
    assert.equal(result.warnings[0].id, "launch_carbox_missing");
  }, command => command === "check_missing_paths" ? { files: [], directories: [] } : false);
});

test("classic launches check content without Launcher directories; Randomizer keeps its own existing checks", async () => {
  const current = state();
  current.setup.scanResult.installType = "classic";
  await withNative(async calls => {
    assert.deepEqual((await validateLaunch(current)).errors, []);
    assert.deepEqual(calls.find(call => call.command === "check_missing_paths").args.directories, []);
    assert.equal(calls.some(call => call.command === "check_path_writable"), false);
  });
  current.app.mode = "randomizer";
  current.setup.scanResult.installType = "launcher";
  current.generate.seedContext = { setup: { requiredPacks: ["game_files", "absent"] } };
  await withNative(async calls => {
    const result = await validateLaunch(current);
    assert.deepEqual(result.errors, []);
    assert.deepEqual(result.warnings.map(issue => issue.id), ["launch_packs_not_found", "launch_missing_packs"]);
    assert.ok(result.warnings.every(issue => issue.scope === "launch"));
    assert.equal(calls.some(call => call.command === "check_missing_paths"), false);
  });
});
