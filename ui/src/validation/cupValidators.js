const CUP_NAMES = ["Bronze Cup", "Silver Cup", "Gold Cup", "Platinum Cup"];

import { STOCK_TRACKS } from "../utils/constants";

export function validateCupSpec(cupSpecState, trackSpecState) {
  const errors = [];
  const warnings = [];

  if (!cupSpecState?.enabled) return { errors, warnings };

  // ── Per-cup cars-per-class sum ─────────────────────────────────────────────
  // Default distributions (designed for 8 cars, sum = 7 each).
  const DEFAULT_CARS_PER_CLASS = [
    [7, 0, 0, 0, 0, 0],
    [0, 4, 3, 0, 0, 0],
    [0, 0, 4, 3, 0, 0],
    [0, 0, 1, 3, 3, 0],
  ];

  cupSpecState.cups?.forEach((cup, i) => {
    // Effective numCars for this cup
    const numCars = cup.overrideNumCars
      ? (cup.numCars ?? cupSpecState.numCars)
      : cupSpecState.numCars;

    const expected = numCars - 1;

    // Effective carsPerClass for this cup
    const effectiveCpc = cup.overrideCarsPerClass
      ? (cup.carsPerClass || [])
      : (DEFAULT_CARS_PER_CLASS[i] || []);

    const sum = effectiveCpc.reduce((s, v) => s + (Number(v) || 0), 0);

    if (sum !== expected) {
      const hint = !cup.overrideCarsPerClass
        ? " Enable the Cars per Class override to configure it."
        : "";
      errors.push({
        id: `cup_cpc_mismatch_${i}`,
        scope: "cupSpec",
        field: `cups[${i}].carsPerClass`,
        message: `${CUP_NAMES[i]} Cars per Class must add up to ${expected}. It currently adds up to ${sum}.${hint}`
      });
    }
  });


  // ── Laps range sanity ──────────────────────────────────────────────────────
  const checkLaps = (min, max, label) => {
    if (min > max) {
      errors.push({
        id: `cup_laps_invalid_${label}`,
        scope: "cupSpec",
        message: `${label}: Minimum laps cannot be greater than maximum laps.`
      });
    }
  };

  checkLaps(cupSpecState.numLapsMin, cupSpecState.numLapsMax, "Global settings");

  cupSpecState.cups?.forEach((cup, i) => {
    // Laps range override is only meaningful when overrideStageMode is active
    if (cup.overrideStageMode && cup.numLapsMin != null && cup.numLapsMax != null) {
      checkLaps(cup.numLapsMin, cup.numLapsMax, CUP_NAMES[i]);
    }
  });

  // ── User-defined stages validation ────────────────────────────────────────
  // Compute each cup's effective stage mode (per-cup override beats global).
  const effectiveMode = (cup) =>
    cup.overrideStageMode ? (cup.stageMode ?? cupSpecState.stageMode) : cupSpecState.stageMode;

  // Build the set of specific track folders declared in the track spec
  // (used only to validate specific-folder stage references).
  const availableTrackFolders = trackSpecState.includeTracks ? 
    new Set(
    (trackSpecState?.tracks || [])
      .map(t => t.sourcePool?.toLowerCase())
      .filter(p =>
        p &&
        p !== "full random" &&
        p !== "stock" &&
        p !== "custom" &&
        !p.startsWith("pack:")
      )
  ) : new Set(STOCK_TRACKS);

  cupSpecState.cups?.forEach((cup, cupIdx) => {
    if (effectiveMode(cup) !== "userDefined") return;

    // Must have at least one stage
    if (!cup.stages || cup.stages.length === 0) {
      errors.push({
        id: `cup_no_stages_${cupIdx}`,
        scope: "cupSpec",
        field: `cups[${cupIdx}].stages`,
        message: `${CUP_NAMES[cupIdx]} has no stages. Add at least one stage or choose another stage mode.`
      });
    }

    // Validate specific-folder references (skip Random, slot:N, difficulty tiers)
    (cup.stages || []).forEach((stage, stageIdx) => {
      const pool = stage.sourcePool;
      const isSpecificFolder =
        pool &&
        pool !== "Random" &&
        !pool.startsWith("slot:") &&  // slot:N — valid, resolved at generation time
        !/^\d$/.test(pool) &&          // difficulty tier "1"-"4"
        !pool.startsWith("Pack:");

      if (isSpecificFolder && !availableTrackFolders.has(pool.toLowerCase())) {
        errors.push({
          id: `cup_stage_track_missing_${cupIdx}_${stageIdx}`,
          scope: "cupSpec",
          field: `cups[${cupIdx}].stages[${stageIdx}]`,
          message:
            `${CUP_NAMES[cupIdx]}, Stage ${stageIdx + 1}: "${pool}" ` +
            `is not available in the current track setup. ` +
            `Add it in Track Specification or choose another track.`
        });
      }
    });
  });

  // ── Total distinct specific tracks ≤ 14 ──────────────────────────────────
  // RVGL loads exactly 14 track slots. Every track folder that is pinned by
  // name (in either the Track Spec or User-Defined cup stages) must fit within
  // those 14 slots. If the union of all pinned folder names exceeds 14,
  // generation is impossible.

  const isGenericTrackSpecPool = (p) =>
    !p || p === "Full Random" || p === "Stock" || p === "Custom" ||
    p.toLowerCase().startsWith("pack:");

  const isGenericStagePool = (p) =>
    !p || p === "Random" || p.startsWith("slot:") || /^\d$/.test(p);

  // Specific folders pinned in the Track Specification
  const pinnedInTrackSpec = new Set(
    (trackSpecState?.tracks || [])
      .map(t => t.sourcePool)
      .filter(p => !isGenericTrackSpecPool(p))
      .map(p => p.toLowerCase())
  );

  // Specific folders referenced in User-Defined cup stages
  const pinnedInCupStages = new Set();
  cupSpecState.cups?.forEach(cup => {
    if (effectiveMode(cup) !== "userDefined") return;
    (cup.stages || []).forEach(stage => {
      if (!isGenericStagePool(stage.sourcePool)) {
        pinnedInCupStages.add(stage.sourcePool.toLowerCase());
      }
    });
  });

  const allPinned = new Set([...pinnedInTrackSpec, ...pinnedInCupStages]);
  if (allPinned.size > 14) {
    errors.push({
      id: "cup_too_many_specific_tracks",
      scope: "cupSpec",
      message:
        `${allPinned.size} distinct tracks are required (` +
        `${pinnedInTrackSpec.size} pinned in Track Specification, ` +
        `${pinnedInCupStages.size} in Cup Stages), ` +
        `but the game only supports 14 track slots. ` +
        `Reduce the number of specific track references.`
    });
  }

  return { errors, warnings };
}
