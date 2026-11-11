import assert from "node:assert/strict";
import test from "node:test";
import { scanInstallation } from "../setup/scanInstallation.js";
import { scanArchipelagoContent } from "../setup/archipelagoContent.js";
import { STOCK_CARS, DC_CARS, STOCK_TRACKS } from "../utils/constants.js";

const EXE_PATH = "D:/RVGL/packs/rvgl_win64/rvgl.exe";

function pack(name, overrides = {}) {
  return {
    name, absolutePath: `D:/RVGL/packs/${name}`,
    hasCars: true, hasTracks: true, useCars: false, useTracks: false,
    cars: [], tracks: [], ...overrides,
  };
}

function launcher(contentPacks) {
  return { installType: "launcher", contentPacks };
}

async function withNativeScan(result, loadedPacks, run) {
  const previousWindow = globalThis.window;
  const calls = [];
  globalThis.window = { __TAURI_INTERNALS__: { invoke: async (command, args) => {
    calls.push({ command, args });
    if (command === "scan_install") {
      return result;
    }
    if (command === "scan_pack_folder") {
      const loaded = loadedPacks[args.folderPath];
      if (loaded instanceof Error) {
        throw loaded;
      }
      assert.ok(loaded, `Unexpected pack read: ${args.folderPath}`);
      return { ...loaded, useCars: args.useCars && loaded.hasCars, useTracks: args.useTracks && loaded.hasTracks };
    }
    assert.fail(`Unexpected native command: ${command}`);
  } } };
  try {
    await run(calls);
  } finally {
    if (previousWindow === undefined) {
      delete globalThis.window;
    } else {
      globalThis.window = previousWindow;
    }
  }
}

test("same-install refresh preserves independent pack choices and reloads selected custom content", async () => {
  const previous = launcher([
    pack("game_files"),
    pack("rvgl_dcpack", { useTracks: true }),
    pack("custom_cars", { useCars: true, cars: [{ folderName: "old_car" }] }),
    pack("custom_tracks", { useTracks: true, tracks: [{ folderName: "old_track" }] }),
  ]);
  const fresh = launcher([
    pack("custom_tracks"),
    pack("game_files", { useCars: true, useTracks: true, cars: [{ folderName: "rc" }] }),
    pack("custom_cars"),
    pack("rvgl_dcpack", { useCars: true, useTracks: true }),
  ]);
  const loaded = {
    "D:/RVGL/packs/custom_cars": pack("custom_cars", { cars: [{ folderName: "new_car" }] }),
    "D:/RVGL/packs/custom_tracks": pack("custom_tracks", { tracks: [{ folderName: "new_track" }] }),
  };
  const before = structuredClone(previous);
  await withNativeScan(fresh, loaded, async calls => {
    const result = await scanInstallation(EXE_PATH, EXE_PATH, previous);
    const byName = Object.fromEntries(result.contentPacks.map(entry => [entry.name, entry]));
    assert.equal(byName.game_files.useCars, false);
    assert.equal(byName.game_files.useTracks, false);
    assert.equal(byName.rvgl_dcpack.useCars, false);
    assert.equal(byName.rvgl_dcpack.useTracks, true);
    assert.equal(byName.custom_cars.useCars, true);
    assert.equal(byName.custom_cars.useTracks, false);
    assert.deepEqual(byName.custom_cars.cars, [{ folderName: "new_car" }]);
    assert.equal(byName.custom_tracks.useCars, false);
    assert.equal(byName.custom_tracks.useTracks, true);
    assert.deepEqual(byName.custom_tracks.tracks, [{ folderName: "new_track" }]);
    assert.deepEqual(calls, [
      { command: "scan_install", args: { executablePath: EXE_PATH } },
      { command: "scan_pack_folder", args: { folderPath: "D:/RVGL/packs/custom_tracks", useCars: false, useTracks: true } },
      { command: "scan_pack_folder", args: { folderPath: "D:/RVGL/packs/custom_cars", useCars: true, useTracks: false } },
    ]);
    assert.deepEqual(previous, before);
  });
});

test("pack identity ignores case and fresh defaults are kept for newly installed packs", async () => {
  const previous = launcher([pack("GAME_FILES"), pack("removed", { useCars: true })]);
  const fresh = launcher([
    pack("game_files", { useCars: true, useTracks: true }),
    pack("rvgl_dcpack", { useCars: true, useTracks: true }),
    pack("new_custom"),
  ]);
  await withNativeScan(fresh, {}, async calls => {
    const result = await scanInstallation(EXE_PATH, EXE_PATH, previous);
    assert.deepEqual(result.contentPacks.map(entry => [entry.name, entry.useCars, entry.useTracks]), [
      ["game_files", false, false], ["rvgl_dcpack", true, true], ["new_custom", false, false],
    ]);
    assert.equal(calls.length, 1);
  });
});

test("different installations and initial scans keep native defaults", async () => {
  const previous = launcher([pack("game_files"), pack("custom", { useCars: true })]);
  const fresh = launcher([pack("game_files", { useCars: true, useTracks: true }), pack("custom")]);
  await withNativeScan(fresh, {}, async calls => {
    assert.equal(await scanInstallation("D:/Other/rvgl.exe", EXE_PATH, previous), fresh);
    assert.equal(await scanInstallation(EXE_PATH, "", null), fresh);
    assert.deepEqual(calls.map(call => call.command), ["scan_install", "scan_install"]);
  });
});

test("classic installations retain their native scan result", async () => {
  const fresh = { installType: "classic", cars: [{ folderName: "rc" }], tracks: [] };
  await withNativeScan(fresh, {}, async calls => {
    assert.equal(await scanInstallation(EXE_PATH, EXE_PATH, fresh), fresh);
    assert.equal(calls.length, 1);
  });
});

test("removed content folders disable their selections and eager packs are not read twice", async () => {
  const previous = launcher([
    pack("custom", { useCars: true }),
    pack("game_files", { useCars: true, useTracks: true }),
  ]);
  const fresh = launcher([
    pack("custom", { hasCars: false }),
    pack("game_files", { useCars: true, useTracks: true }),
  ]);
  await withNativeScan(fresh, {}, async calls => {
    const result = await scanInstallation(EXE_PATH, EXE_PATH, previous);
    assert.equal(result.contentPacks[0].useCars, false);
    assert.equal(result.contentPacks[0].useTracks, false);
    assert.equal(result.contentPacks[1].useCars, true);
    assert.equal(result.contentPacks[1].useTracks, true);
    assert.equal(calls.length, 1);
  });
});

test("an unreadable selected pack rejects the refresh without mutating previous content", async () => {
  const previous = launcher([pack("custom", { useCars: true, cars: [{ folderName: "old_car" }] })]);
  const before = structuredClone(previous);
  await withNativeScan(launcher([pack("custom")]), { "D:/RVGL/packs/custom": new Error("Pack disappeared") }, async () => {
    await assert.rejects(scanInstallation(EXE_PATH, EXE_PATH, previous), /Pack disappeared/);
    assert.deepEqual(previous, before);
  });
});

test("AP scanning still sees stock and DC content when their Randomizer pools are disabled", async () => {
  const car = folderName => ({ folderName, hasValidFile: true, isSystemCar: false });
  const track = folderName => ({ folderName, hasValidFile: true, trackType: folderName === "stunts" ? 2 : 0 });
  const previous = launcher([pack("game_files"), pack("rvgl_dcpack")]);
  const fresh = launcher([
    pack("game_files", {
      useCars: true, useTracks: true, cars: STOCK_CARS.map(car),
      tracks: [...STOCK_TRACKS.filter(folder => folder !== "roof"), "stunts"].map(track),
    }),
    pack("rvgl_dcpack", { useCars: true, useTracks: true, cars: DC_CARS.map(car), tracks: [track("roof")] }),
  ]);
  await withNativeScan(fresh, {}, async calls => {
    const result = await scanInstallation(EXE_PATH, EXE_PATH, previous);
    const apResult = await scanArchipelagoContent(result, EXE_PATH);
    assert.ok(result.contentPacks.every(entry => !entry.useCars && !entry.useTracks));
    assert.ok(Object.values(apResult.categories).every(category => category.status === "complete"));
    assert.equal(calls.length, 1);
  });
});
