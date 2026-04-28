import assert from "node:assert/strict";

import { validateCupSpec } from "./cupValidators.js";
import { validateScan } from "./scanValidators.js";
import { formatValidationList } from "./validationUtils.js";

function runTest(name, fn) {
  try {
    fn();
    console.log(`PASS ${name}`);
  } catch (error) {
    console.error(`FAIL ${name}`);
    throw error;
  }
}

runTest("formatValidationList shortens long lists for UI display", () => {
  const result = formatValidationList([
    "slot 1 (rc)",
    "slot 2 (mite)",
    "slot 3 (phat)",
    "slot 4 (moss)",
  ]);

  assert.equal(result, "slot 1 (rc), slot 2 (mite), slot 3 (phat), +1 more");
});

runTest("validateScan reports low car and track counts as warnings", () => {
  const results = validateScan({
    installType: "classic",
    cars: [],
    tracks: [],
  });

  assert.equal(results.errors.length, 0);
  assert.equal(results.warnings.length, 2);
  assert.deepEqual(
    results.warnings.map((issue) => issue.id).sort(),
    ["scan_insufficient_cars", "scan_insufficient_tracks"]
  );
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
