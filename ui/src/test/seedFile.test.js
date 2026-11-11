import assert from "node:assert/strict";
import test from "node:test";
import { getFileName, selectSeedFile } from "../utils/seedFile.js";

test("the shared JSON chooser preserves native paths and treats cancellation as no selection", async () => {
  const previousWindow = globalThis.window;
  let selection;
  const calls = [];
  globalThis.window = { __TAURI_INTERNALS__: { invoke: async (command, args) => {
    calls.push({ command, args });
    return selection;
  } } };
  try {
    for (const [value, expected] of [
      [null, ""], [undefined, ""], ["D:\\Seeds\\Player.rvgl.json", "D:\\Seeds\\Player.rvgl.json"],
      [{ path: "D:/Seeds/Player.rvgl.json" }, "D:/Seeds/Player.rvgl.json"], [{ path: 123 }, ""],
    ]) {
      selection = value;
      assert.equal(await selectSeedFile("D:/Seeds"), expected);
    }
    assert.ok(calls.every(call => call.command === "plugin:dialog|open"));
    assert.deepEqual(calls[0].args.options, {
      multiple: false, defaultPath: "D:/Seeds", filters: [{ name: "JSON Config", extensions: ["json"] }],
    });
    assert.equal(getFileName("D:\\Seeds\\Player.rvgl.json"), "Player.rvgl.json");
    assert.equal(getFileName("D:/Seeds/Player.rvgl.json"), "Player.rvgl.json");
  } finally {
    if (previousWindow === undefined) delete globalThis.window;
    else globalThis.window = previousWindow;
  }
});
