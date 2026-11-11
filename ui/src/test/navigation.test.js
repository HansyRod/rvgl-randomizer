import assert from "node:assert/strict";
import test from "node:test";
import { DEFAULT_STATE } from "../AppProvider.jsx";
import { normalizeAppContext } from "../utils/configureContext.js";
import { canNavigateToStep, getAccessibleStep, getSteps } from "../navigation/steps.js";

function makeState(mode, overrides = {}) {
  return {
    ...structuredClone(DEFAULT_STATE),
    app: { ...DEFAULT_STATE.app, mode, activeStep: "mode" },
    setup: { ...DEFAULT_STATE.setup, installPath: "D:/RVGL/rvgl.exe", scanResult: {} },
    ...overrides,
  };
}

test("fresh state requires a mode before Setup, regardless of a cached installation", () => {
  for (const state of [DEFAULT_STATE, makeState(null)]) {
    assert.equal(canNavigateToStep(state, "mode"), true);
    for (const step of ["setup", "configure", "generate", "import", "play"]) {
      assert.equal(canNavigateToStep(state, step), false);
      assert.equal(getAccessibleStep(state, step), "mode");
    }
  }
});

test("each mode has its own route and rejects steps from the other route", () => {
  assert.deepEqual(getSteps("randomizer").map(step => step.id),
    ["mode", "setup", "configure", "generate", "play"]);
  assert.deepEqual(getSteps("archipelago").map(step => step.id),
    ["mode", "setup", "import", "play"]);
  assert.equal(canNavigateToStep(makeState("randomizer"), "import"), false);
  for (const step of ["configure", "generate", "unknown"]) {
    assert.equal(canNavigateToStep(makeState("archipelago"), step), false);
  }
});

test("Setup is accessible after selection while later steps require an installation and scan", () => {
  for (const mode of ["randomizer", "archipelago"]) {
    for (const setup of [{}, { installPath: "rvgl.exe" }, { scanResult: {} }]) {
      const state = makeState(mode, { setup });
      assert.equal(canNavigateToStep(state, "setup"), true);
      for (const step of getSteps(mode).slice(2)) {
        assert.equal(canNavigateToStep(state, step.id), false);
        assert.equal(getAccessibleStep(state, step.id), "setup");
      }
    }
  }
});

test("Randomizer Play requires its own file and does not use an AP selection", () => {
  const state = makeState("randomizer");
  state.archipelago.import.importedFilePath = "AP_Player.rvgl.json";
  assert.equal(canNavigateToStep(state, "play"), false);
  assert.equal(getAccessibleStep(state, "play"), "generate");
  state.generate.generatedFilePath = "normal.json";
  assert.equal(canNavigateToStep(state, "play"), true);
});

test("AP Play requires its own imported file, independently of the Randomizer file", () => {
  const state = makeState("archipelago");
  state.generate.generatedFilePath = "normal.json";
  assert.equal(canNavigateToStep(state, "import"), true);
  assert.equal(canNavigateToStep(state, "play"), false);
  assert.equal(getAccessibleStep(state, "play"), "import");
  state.archipelago.import.importedFilePath = "AP_Player.rvgl.json";
  assert.equal(canNavigateToStep(state, "play"), true);
  assert.equal(getAccessibleStep(state, "play"), "play");
  state.archipelago.import.importedFilePath = "";
  assert.equal(canNavigateToStep(state, "play"), false);
});

test("legacy cache migration selects Randomizer and preserves its saved screen and data", () => {
  const legacy = makeState(undefined);
  delete legacy.app.mode;
  delete legacy.archipelago;
  legacy.app.activeStep = "play";
  legacy.app.theme = "earthy";
  legacy.configure.preset = "custom";
  legacy.generate = {
    generatedFilePath: "normal.json", instanceName: "my-run", profileName: "my-profile",
    generatedHistory: [{ path: "normal.json", profileName: "my-profile" }], seedContext: null,
  };
  legacy.play = { extraArgs: "-window", extraPacks: ["soundtrack"], runningPid: 123 };
  const before = structuredClone(legacy);
  const normalized = normalizeAppContext(legacy);
  assert.equal(normalized.app.mode, "randomizer");
  assert.equal(normalized.app.activeStep, "play");
  assert.equal(normalized.app.theme, "earthy");
  assert.equal(normalized.configure.preset, "custom");
  assert.deepEqual(normalized.setup, before.setup);
  assert.deepEqual(normalized.generate, before.generate);
  assert.deepEqual(normalized.play, before.play);
  assert.deepEqual(legacy, before);
});

test("valid cached routes restore their screens without changing either mode's run state", () => {
  for (const mode of ["randomizer", "archipelago"]) {
    const state = makeState(mode);
    state.generate.generatedFilePath = "normal.json";
    state.play.extraArgs = "-window";
    state.setup.setupTab = "tracks";
    state.archipelago = {
      setup: { scanResult: null },
      import: {
        importedFilePath: "AP_Player.rvgl.json", seedMetadata: { profileName: "ap_123" },
        importedHistory: [{ path: "AP_Player.rvgl.json" }],
      },
      play: { extraPacks: ["music"] },
    };
    for (const step of getSteps(mode).filter(step => canNavigateToStep(state, step.id))) {
      state.app.activeStep = step.id;
      const normalized = normalizeAppContext(state);
      assert.equal(normalized.app.mode, mode);
      assert.equal(normalized.app.activeStep, step.id);
      assert.equal(normalized.setup.setupTab, "tracks");
      assert.deepEqual(normalized.generate, state.generate);
      assert.deepEqual(normalized.play, state.play);
      assert.deepEqual(normalized.archipelago, state.archipelago);
    }
  }
});

test("cache normalization falls back to the missing prerequisite", () => {
  const state = makeState("randomizer");
  state.app.activeStep = "play";
  assert.equal(normalizeAppContext(state).app.activeStep, "generate");
  state.setup.scanResult = null;
  assert.equal(normalizeAppContext(state).app.activeStep, "setup");
  state.app.mode = "archipelago";
  state.setup.scanResult = {};
  assert.equal(normalizeAppContext(state).app.activeStep, "import");
  state.app.activeStep = "configure";
  assert.equal(normalizeAppContext(state).app.activeStep, "setup");
});

test("cache loading clears interrupted work flags and keeps startup loading active", () => {
  for (const mode of ["randomizer", "archipelago"]) {
    const state = makeState(mode);
    state.app = { ...state.app, isLoading: false, isScanning: true, isFetchingPack: true };
    state.setup.setupTab = "tracks";
    state.play.extraArgs = "-window";
    const before = structuredClone(state);
    const normalized = normalizeAppContext(state);

    assert.deepEqual(normalized.app, {
      ...state.app, isLoading: true, isScanning: false, isFetchingPack: false,
    });
    assert.deepEqual(normalized.setup, state.setup);
    assert.deepEqual(normalized.play, state.play);
    assert.deepEqual(normalized.archipelago, state.archipelago);
    assert.deepEqual(state, before);
  }
});

test("empty caches stay empty and unknown cached modes fall back to Mode without changing saved data", () => {
  const emptyCache = {};
  assert.equal(normalizeAppContext(emptyCache), emptyCache);
  assert.equal(normalizeAppContext(null), null);
  const state = makeState("unsupported");
  state.app.activeStep = "configure";
  const normalized = normalizeAppContext(state);
  assert.equal(normalized.app.mode, "unsupported");
  assert.equal(normalized.app.activeStep, "mode");
  assert.deepEqual(normalized.generate, state.generate);
});

test("AP state is grouped by tab with separate packs and shared command-line arguments", () => {
  assert.equal(DEFAULT_STATE.setup.setupTab, "install");
  assert.notEqual(DEFAULT_STATE.generate.generatedHistory, DEFAULT_STATE.archipelago.import.importedHistory);
  assert.notEqual(DEFAULT_STATE.play.extraPacks, DEFAULT_STATE.archipelago.play.extraPacks);
  assert.equal(DEFAULT_STATE.play.extraArgs, "");
  assert.equal(DEFAULT_STATE.archipelago.play.extraArgs, undefined);
  assert.equal(DEFAULT_STATE.play.runningPid, 0);
  assert.equal(DEFAULT_STATE.archipelago.play.runningPid, undefined);
});

test("legacy caches fall back when prerequisites are missing, while explicit unselected modes stay unselected", () => {
  const state = makeState(undefined);
  delete state.app.mode;
  state.app.activeStep = "play";
  assert.equal(normalizeAppContext(state).app.activeStep, "generate");
  state.setup.scanResult = null;
  assert.equal(normalizeAppContext(state).app.activeStep, "setup");
  delete state.app.activeStep;
  assert.equal(normalizeAppContext(state).app.activeStep, "setup");
  state.app.mode = null;
  assert.equal(normalizeAppContext(state).app.mode, null);
  assert.equal(normalizeAppContext(state).app.activeStep, "mode");
});

