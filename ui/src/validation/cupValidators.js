const CUP_NAMES = ["Bronze", "Silver", "Gold", "Platinum"];

export function validateCupSpec(cupSpecState, trackSpecState) {
  const errors = [];
  const warnings = [];

  if (!cupSpecState?.enabled) return { errors, warnings };

  // Estimate how many tracks will be resolved
  const trackCount = trackSpecState?.includeTracks !== false
    ? (trackSpecState?.tracks?.length ?? 14)
    : 14; // stock tracks always available as fallback

  // Default stage mode needs enough distinct tracks for Platinum
  if (cupSpecState.stageMode === "default" && trackCount < 6) {
    warnings.push({
      id: "cup_insufficient_tracks_default",
      scope: "cupSpec",
      message: "The Default Stages layout for the Platinum cup requires at least 6 distinct tracks. Some stages may not fill correctly."
    });
  }

  // Per-cup cars-per-class sum validation
  cupSpecState.cups?.forEach((cup, i) => {
    const numCars = cup.overrideGlobal
      ? (cup.numCars ?? cupSpecState.numCars)
      : cupSpecState.numCars;

    const expected = numCars - 1;
    const sum = (cup.carsPerClass || []).reduce((s, v) => s + (Number(v) || 0), 0);

    if (sum !== expected) {
      errors.push({
        id: `cup_cpc_mismatch_${i}`,
        scope: "cupSpec",
        field: `cups[${i}].carsPerClass`,
        message: `${CUP_NAMES[i]} Cup: Cars Per Class sums to ${sum} but should be ${expected} (numCars − 1).`
      });
    }
  });

  // Laps range sanity
  const checkLaps = (min, max, label) => {
    if (min > max) {
      errors.push({
        id: `cup_laps_invalid_${label}`,
        scope: "cupSpec",
        message: `${label}: Laps Min (${min}) is greater than Laps Max (${max}).`
      });
    }
  };

  checkLaps(cupSpecState.numLapsMin, cupSpecState.numLapsMax, "Global");

  cupSpecState.cups?.forEach((cup, i) => {
    if (cup.overrideGlobal && cup.numLapsMin != null && cup.numLapsMax != null) {
      checkLaps(
        cup.numLapsMin,
        cup.numLapsMax,
        `${CUP_NAMES[i]} Cup`
      );
    }
  });

  // User-defined mode: check each cup has at least one stage
  if (cupSpecState.stageMode === "userDefined") {
    cupSpecState.cups?.forEach((cup, i) => {
      if (!cup.stages || cup.stages.length === 0) {
        errors.push({
          id: `cup_no_stages_${i}`,
          scope: "cupSpec",
          field: `cups[${i}].stages`,
          message: `${CUP_NAMES[i]} Cup has no stages defined. Add at least one stage or switch to a different Stage Mode.`
        });
      }
    });

    // Build the set of all available track folders from the track spec.
    // We use the spec's sourcePool values to infer what's likely available,
    // but for specific folder references we check the track spec directly.
    // The track spec already validated stale refs, so here we only need to
    // check stage sourcePool values that are specific folder names.
    const availableTrackFolders = new Set(
      (trackSpecState?.tracks || [])
        .map(t => t.sourcePool?.toLowerCase())
        .filter(p =>
          p &&
          p !== "full random" &&
          p !== "stock" &&
          p !== "custom" &&
          !p.startsWith("pack:")
        )
    );

    cupSpecState.cups?.forEach((cup, cupIdx) => {
      (cup.stages || []).forEach((stage, stageIdx) => {
        const pool = stage.sourcePool;
        const isSpecificFolder =
          pool &&
          pool !== "Random" &&
          !/^\d$/.test(pool) && // difficulty tier "1"-"4"
          !pool.startsWith("Pack:");

        if (isSpecificFolder && !availableTrackFolders.has(pool.toLowerCase())) {
          errors.push({
            id: `cup_stage_track_missing_${cupIdx}_${stageIdx}`,
            scope: "cupSpec",
            field: `cups[${cupIdx}].stages[${stageIdx}]`,
            message:
              `${cupNames[cupIdx]} Cup, Stage ${stageIdx + 1}: track "${pool}" ` +
              `is not present in the current track pool. ` +
              `Add it to the Track Spec or choose a different track.`
          });
        }
      });
    });
  }


  return { errors, warnings };
}