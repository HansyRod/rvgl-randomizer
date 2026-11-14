import assert from "node:assert/strict";
import test from "node:test";
import { loadArchipelagoFile } from "../import/archipelagoFile.js";

const jsonPath = "D:/Seeds/AP_123456_P2_Player.rvgl.json";
const archivePath = "D:/Seeds/RenamedDownload.aprvgl";
const config = {
  metadata: { version: "0.3.0", seed: "123456", profileName: "ap_0123456789ab" },
  archipelago: {
    worldVersion: "0.1.0", slotNumber: 2, slotName: "Player", goal: "platinum_cup",
    locations: { "championship_win:4": 3000305 }, items: { "cup:4": 3000300 },
    goalEventKeys: ["championship_win:4"],
  },
  global_options: { is_stock_cars: true, is_stock_tracks: true },
};

async function withBackend(handler, run) {
  const previousWindow = globalThis.window;
  const calls = [];
  globalThis.window = { __TAURI_INTERNALS__: { invoke: async (command, args) => {
    calls.push({ command, args });
    return handler(command, args);
  } } };
  try {
    await run(calls);
  } finally {
    if (previousWindow === undefined) delete globalThis.window;
    else globalThis.window = previousWindow;
  }
}

test("JSON uses the existing reader; archives extract first, then use that same reader", async () => {
  for (const file of [jsonPath, archivePath]) {
    await withBackend(command => command === "read_config_file" ? config : jsonPath, async calls => {
      const result = await loadArchipelagoFile(file);
      assert.equal(result.configPath, jsonPath);
      assert.equal(result.metadata.slotName, "Player");
      assert.deepEqual(calls.map(call => call.command), file === jsonPath
        ? ["read_config_file"] : ["extract_archipelago_file", "read_config_file"]);
      assert.deepEqual(calls.at(-1).args, { filePath: jsonPath });
    });
  }
});

test("Save As preserves the internal filename and cancellation stops the import", async () => {
  for (const destination of ["E:/Runs/AP_123456_P2_Player.rvgl.json", null]) {
    await withBackend((command, args) => {
      if (command === "extract_archipelago_file") {
        if (args.outputPath) return args.outputPath;
        throw { fileName: "AP_123456_P2_Player.rvgl.json", message: "Access denied" };
      }
      return command === "plugin:dialog|save" ? destination : config;
    }, async calls => {
      const result = await loadArchipelagoFile(archivePath);
      assert.equal(result?.configPath ?? null, destination);
      assert.equal(calls[1].args.options.defaultPath, "AP_123456_P2_Player.rvgl.json");
      if (destination) {
        assert.deepEqual(calls[2].args, { filePath: archivePath, outputPath: destination });
        assert.deepEqual(calls[3], { command: "read_config_file", args: { filePath: destination } });
      } else {
        assert.equal(calls.length, 2);
      }
    });
  }
});
