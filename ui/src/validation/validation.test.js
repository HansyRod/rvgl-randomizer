import assert from "node:assert/strict";

import {
  countEligibleCarsByFolderNames,
  countEligibleCarsByRating,
  countEligibleTracksByFolderNames,
  evaluatePresetSelection,
  getStockModePresetErrors,
} from "../configure/presets/presetValidation.js";
import { validateCupSpec } from "./cupValidators.js";
import { validateSelectedPreset } from "./presetValidators.js";
import { validateScan } from "./scanValidators.js";
import { isEffectiveStockCarsMode, isEffectiveStockTracksMode } from "./stockMode.js";
import { formatValidationList } from "./validationUtils.js";
import { STOCK_CARS, STOCK_TRACKS } from "../utils/constants.js";

function runTest(name, fn) {
  try {
    fn();
    console.log(`PASS ${name}`);
  } catch (error) {
    console.error(`FAIL ${name}`);
    throw error;
  }
}

function makeCar(folderName, rating) {
  return {
    folderName,
    rating,
    isSystemCar: false,
    hasValidFile: true,
  };
}

function makeTrack(folderName) {
  return {
    folderName,
    hasValidFile: true,
    trackType: 0,
  };
}

function makeClassicScan({ cars = [], tracks = [] } = {}) {
  return {
    installType: "classic",
    cars,
    tracks,
  };
}

function makeCarsByRating(counts) {
  const cars = [];

  Object.entries(counts).forEach(([ratingKey, count]) => {
    const rating = Number.parseInt(ratingKey, 10);
    for (let index = 0; index < count; index += 1) {
      cars.push(makeCar(`car_${rating}_${index}`, rating));
    }
  });

  return cars;
}

function makeStockCarsScan() {
  return makeClassicScan({
    cars: STOCK_CARS.map((folderName, index) => makeCar(folderName, Math.min(index, 5))),
    tracks: [makeTrack("custom_track_1"), makeTrack("custom_track_2")],
  });
}

function makeStockTracksScan() {
  return makeClassicScan({
    cars: makeCarsByRating({ 0: 42, 5: 14 }),
    tracks: STOCK_TRACKS.filter((track) => track !== "roof").map((folderName) => makeTrack(folderName)),
  });
}

function makeMixedStockEligibleScan() {
  return makeClassicScan({
    cars: [
      ...STOCK_CARS.map((folderName, index) => makeCar(folderName, Math.min(index, 4))),
      makeCar("custom_car_1", 5),
    ],
    tracks: [
      ...STOCK_TRACKS.filter((track) => track !== "roof").map((folderName) => makeTrack(folderName)),
      makeTrack("custom_track_1"),
    ],
  });
}

const RANDOM_STOCKS_REQUIRED_TRACKS = STOCK_TRACKS.filter((track) => track !== "roof");

const CURRENT_PRESET_FIXTURES = [
  {
    id: "balanced",
    validateSelection: ({ scanResult}) => getStockModePresetErrors(scanResult, "balanced")
  },
  {
    id: "all-rookies",
    validateSelection: ({ scanResult }) => {
      const errors = getStockModePresetErrors(scanResult, "all-rookies");
      const rookieCount = countEligibleCarsByRating(scanResult, 0);

      if (rookieCount < 42) {
        errors.push(`This preset requires at least 42 eligible Rookie cars, but only ${rookieCount} are currently available.`);
      }

      return errors;
    },
  },
  {
    id: "full-random",
    validateSelection: ({ scanResult }) => getStockModePresetErrors(scanResult, "full-random"),
  },
  {
    id: "challenge",
    validateSelection: ({ scanResult }) => {
      const errors = getStockModePresetErrors(scanResult, "challenge");
      const superProCount = countEligibleCarsByRating(scanResult, 5);

      if (superProCount < 14) {
        errors.push(`This preset requires at least 14 eligible Super Pro cars, but only ${superProCount} are currently available.`);
      }

      return errors;
    },
  },
  {
    id: "long-cups",
    validateSelection: ({ scanResult }) => {
      const errors = getStockModePresetErrors(scanResult, "long-cups");
      const superProCount = countEligibleCarsByRating(scanResult, 5);

      if (superProCount < 7) {
        errors.push(`This preset requires at least 7 eligible Super Pro cars, but only ${superProCount} are currently available.`);
      }

      return errors;
    },
  },
  {
    id: "random-stocks",
    stockMode: {
      cars: true,
      tracks: true,
    },
    validateSelection: ({ scanResult }) => {
      const errors = [];
      const availableStockCars = countEligibleCarsByFolderNames(scanResult, STOCK_CARS);
      const availableStockTracks = countEligibleTracksByFolderNames(scanResult, RANDOM_STOCKS_REQUIRED_TRACKS);

      if (availableStockCars < STOCK_CARS.length) {
        errors.push(`This preset requires all ${STOCK_CARS.length} stock cars, but only ${availableStockCars} are currently available.`);
      }

      if (availableStockTracks < RANDOM_STOCKS_REQUIRED_TRACKS.length) {
        errors.push(`This preset requires all ${RANDOM_STOCKS_REQUIRED_TRACKS.length} stock tracks, but only ${availableStockTracks} are currently available.`);
      }

      return errors;
    },
  },
];

const STOCK_MODE_BLOCKED_PRESET_FIXTURES = CURRENT_PRESET_FIXTURES.filter((preset) => preset.id !== "random-stocks");

runTest("formatValidationList shortens long lists for UI display", () => {
  const result = formatValidationList([
    "slot 1 (rc)",
    "slot 2 (mite)",
    "slot 3 (phat)",
    "slot 4 (moss)",
  ]);

  assert.equal(result, "slot 1 (rc), slot 2 (mite), slot 3 (phat), +1 more");
});

runTest("validateScan reports low car and track counts as errors", () => {
  const results = validateScan(
    {
      installType: "classic",
      cars: [],
      tracks: [],
    },
    "basic"
  );

  assert.equal(results.errors.length, 2);
  assert.equal(results.warnings.length, 0);
  assert.deepEqual(
    results.errors.map((issue) => issue.id).sort(),
    ["scan_insufficient_cars", "scan_insufficient_tracks"]
  );
});

runTest("a preset without validateSelection is always selectable", () => {
  const result = evaluatePresetSelection(
    { id: "test-preset", label: "Test Preset" },
    makeClassicScan()
  );

  assert.equal(result.isSelectable, true);
  assert.deepEqual(result.errors, []);
});

runTest("stock cars mode invalidates each current named non-stock preset", () => {
  const scanResult = makeStockCarsScan();

  STOCK_MODE_BLOCKED_PRESET_FIXTURES.forEach((preset) => {
    const result = evaluatePresetSelection(preset, scanResult);
    assert.equal(result.isSelectable, false, `${preset.id} should be invalid`);
    assert.ok(
      result.errors.includes("This preset cannot be used when Stock Content Mode is active."),
      `${preset.id} should report stock cars mode`
    );
  });
});

runTest("stock tracks mode invalidates each current named non-stock preset", () => {
  const scanResult = makeStockTracksScan();

  STOCK_MODE_BLOCKED_PRESET_FIXTURES.forEach((preset) => {
    const result = evaluatePresetSelection(preset, scanResult);
    assert.equal(result.isSelectable, false, `${preset.id} should be invalid`);
    assert.ok(
      result.errors.includes("This preset cannot be used when Stock Content Mode is active."),
      `${preset.id} should report stock tracks mode`
    );
  });
});

runTest("mixed content plus Random Stocks activates effective stock mode", () => {
  const scanResult = makeMixedStockEligibleScan();

  assert.equal(
    isEffectiveStockCarsMode(scanResult, "random-stocks"),
    true
  );
  assert.equal(
    isEffectiveStockTracksMode(scanResult, "random-stocks"),
    true
  );
});

runTest("mixed content plus non-stock preset does not activate effective stock mode", () => {
  const scanResult = makeMixedStockEligibleScan();

  assert.equal(
    isEffectiveStockCarsMode(scanResult, "balanced"),
    false
  );
  assert.equal(
    isEffectiveStockTracksMode(scanResult, "balanced"),
    false
  );
});

runTest("candidate non-stock presets stay selectable when Random Stocks is currently selected on mixed content", () => {
  const scanResult = makeMixedStockEligibleScan();
  const preset = CURRENT_PRESET_FIXTURES.find((entry) => entry.id === "balanced");

  const result = evaluatePresetSelection(
    preset,
    scanResult
  );

  assert.equal(result.isSelectable, true);
  assert.deepEqual(result.errors, []);
});

runTest("All Rookies requires at least 42 rookie cars", () => {
  const preset = CURRENT_PRESET_FIXTURES.find((entry) => entry.id === "all-rookies");

  const failingResult = evaluatePresetSelection(
    preset,
    makeClassicScan({
      cars: makeCarsByRating({ 0: 41, 5: 14 }),
      tracks: [makeTrack("custom_track_1"), makeTrack("custom_track_2")],
    })
  );

  const passingResult = evaluatePresetSelection(
    preset,
    makeClassicScan({
      cars: makeCarsByRating({ 0: 42, 5: 14 }),
      tracks: [makeTrack("custom_track_1"), makeTrack("custom_track_2")],
    })
  );

  assert.equal(failingResult.isSelectable, false);
  assert.ok(failingResult.errors.some((error) => error.includes("at least 42 eligible Rookie cars")));
  assert.equal(passingResult.isSelectable, true);
});

runTest("Challenge Mode requires at least 14 Super Pro cars", () => {
  const preset = CURRENT_PRESET_FIXTURES.find((entry) => entry.id === "challenge");

  const failingResult = evaluatePresetSelection(
    preset,
    makeClassicScan({
      cars: makeCarsByRating({ 0: 42, 5: 13 }),
      tracks: [makeTrack("custom_track_1"), makeTrack("custom_track_2")],
    })
  );

  const passingResult = evaluatePresetSelection(
    preset,
    makeClassicScan({
      cars: makeCarsByRating({ 0: 42, 5: 14 }),
      tracks: [makeTrack("custom_track_1"), makeTrack("custom_track_2")],
    })
  );

  assert.equal(failingResult.isSelectable, false);
  assert.ok(failingResult.errors.some((error) => error.includes("at least 14 eligible Super Pro cars")));
  assert.equal(passingResult.isSelectable, true);
});

runTest("Long Cups requires at least 7 Super Pro cars", () => {
  const preset = CURRENT_PRESET_FIXTURES.find((entry) => entry.id === "long-cups");

  const failingResult = evaluatePresetSelection(
    preset,
    makeClassicScan({
      cars: makeCarsByRating({ 0: 42, 5: 6 }),
      tracks: [makeTrack("custom_track_1"), makeTrack("custom_track_2")],
    })
  );

  const passingResult = evaluatePresetSelection(
    preset,
    makeClassicScan({
      cars: makeCarsByRating({ 0: 42, 5: 7 }),
      tracks: [makeTrack("custom_track_1"), makeTrack("custom_track_2")],
    })
  );

  assert.equal(failingResult.isSelectable, false);
  assert.ok(failingResult.errors.some((error) => error.includes("at least 7 eligible Super Pro cars")));
  assert.equal(passingResult.isSelectable, true);
});

runTest("Random Stocks requires all 28 stock cars", () => {
  const preset = CURRENT_PRESET_FIXTURES.find((entry) => entry.id === "random-stocks");

  const failingResult = evaluatePresetSelection(
    preset,
    makeClassicScan({
      cars: [
        ...STOCK_CARS.slice(0, STOCK_CARS.length - 1).map((folderName) => makeCar(folderName, 0)),
        makeCar("custom_car_1", 0),
      ],
      tracks: RANDOM_STOCKS_REQUIRED_TRACKS.map((folderName) => makeTrack(folderName)),
    })
  );

  const passingResult = evaluatePresetSelection(
    preset,
    makeClassicScan({
      cars: STOCK_CARS.map((folderName) => makeCar(folderName, 0)),
      tracks: RANDOM_STOCKS_REQUIRED_TRACKS.map((folderName) => makeTrack(folderName)),
    })
  );

  assert.equal(failingResult.isSelectable, false);
  assert.ok(failingResult.errors.some((error) => error.includes("all 28 stock cars")));
  assert.equal(passingResult.isSelectable, true);
});

runTest("Random Stocks requires all 13 stock tracks", () => {
  const preset = CURRENT_PRESET_FIXTURES.find((entry) => entry.id === "random-stocks");

  const failingResult = evaluatePresetSelection(
    preset,
    makeClassicScan({
      cars: STOCK_CARS.map((folderName) => makeCar(folderName, 0)),
      tracks: [
        ...RANDOM_STOCKS_REQUIRED_TRACKS.slice(0, RANDOM_STOCKS_REQUIRED_TRACKS.length - 1).map((folderName) => makeTrack(folderName)),
        makeTrack("custom_track_1"),
      ],
    })
  );

  const passingResult = evaluatePresetSelection(
    preset,
    makeClassicScan({
      cars: STOCK_CARS.map((folderName) => makeCar(folderName, 0)),
      tracks: RANDOM_STOCKS_REQUIRED_TRACKS.map((folderName) => makeTrack(folderName)),
    })
  );

  assert.equal(failingResult.isSelectable, false);
  assert.ok(failingResult.errors.some((error) => error.includes("all 13 stock tracks")));
  assert.equal(passingResult.isSelectable, true);
});

runTest("selected custom and unknown preset ids do not emit preset errors", () => {
  const scanResult = makeClassicScan({
    cars: makeCarsByRating({ 0: 42, 5: 14 }),
    tracks: [makeTrack("custom_track_1"), makeTrack("custom_track_2")],
  });

  const customResult = validateSelectedPreset({ preset: "custom" }, scanResult, CURRENT_PRESET_FIXTURES);
  const unknownResult = validateSelectedPreset({ preset: "missing-preset" }, scanResult, CURRENT_PRESET_FIXTURES);

  assert.deepEqual(customResult.errors, []);
  assert.deepEqual(unknownResult.errors, []);
});

runTest("selected invalid preset emits a preset validation error", () => {
  const results = validateSelectedPreset(
    { preset: "challenge" },
    makeClassicScan({
      cars: makeCarsByRating({ 0: 42, 5: 13 }),
      tracks: [makeTrack("custom_track_1"), makeTrack("custom_track_2")],
    }),
    CURRENT_PRESET_FIXTURES
  );

  assert.equal(results.errors.length, 1);
  assert.equal(results.errors[0].scope, "preset");
  assert.match(results.errors[0].message, /The selected preset has unmet requirements:/);
  assert.match(results.errors[0].message, /at least 14 eligible Super Pro cars/);
});

runTest("selected Random Stocks stays valid on mixed content when all stock content is present", () => {
  const results = validateSelectedPreset(
    { preset: "random-stocks" },
    makeMixedStockEligibleScan(),
    CURRENT_PRESET_FIXTURES
  );

  assert.deepEqual(results.errors, []);
});

runTest("validateScan uses stock thresholds for mixed content when Random Stocks is selected", () => {
  const results = validateScan(
    makeMixedStockEligibleScan(),
    "random-stocks"
  );

  assert.deepEqual(results.errors, []);
});

runTest("validateScan keeps normal thresholds for mixed content when a non-stock preset is selected", () => {
  const results = validateScan(
    makeMixedStockEligibleScan(),
    "balanced"
  );

  assert.equal(results.errors.length, 1);
  assert.equal(results.errors[0].id, "scan_insufficient_cars");
  assert.match(results.errors[0].message, /requires at least 42/);
});

runTest("validateCupSpec reports missing user-defined stage tracks without throwing", () => {
  const results = validateCupSpec(
    {
      enabled: true,
      stageMode: "userDefined",
      numCars: 8,
      numLapsMin: 2,
      numLapsMax: 3,
      cups: [
        {
          overrideGlobal: false,
          carsPerClass: [2, 2, 2, 1],
          stages: [{ sourcePool: "missing_track" }],
        },
        {
          overrideGlobal: false,
          carsPerClass: [2, 2, 2, 1],
          stages: [{ sourcePool: "track_b" }],
        },
        {
          overrideGlobal: false,
          carsPerClass: [2, 2, 2, 1],
          stages: [{ sourcePool: "track_c" }],
        },
        {
          overrideGlobal: false,
          carsPerClass: [2, 2, 2, 1],
          stages: [{ sourcePool: "track_d" }],
        },
      ],
    },
    {
      includeTracks: true,
      tracks: [
        { sourcePool: "track_a" },
        { sourcePool: "track_b" },
        { sourcePool: "track_c" },
        { sourcePool: "track_d" },
      ],
    }
  );

  const issue = results.errors.find((entry) => entry.id === "cup_stage_track_missing_0_0");
  assert.ok(issue);
  assert.match(
    issue.message,
    /Bronze Cup, Stage 1: "missing_track" is not available in the current track setup\./
  );
});
