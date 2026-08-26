import { getAllTracksFromScan, getTrackSpecAvailableFolders, isGenericTrackSpecPool } from "./validationUtils";
import {
  NATIVE_MAX_CUP_CARS,
  EXTENDED_MAX_CUP_CARS,
  CUP_POINTS_TABLE_LENGTH,
} from "../utils/constants.js";
import {
  getCupOpponentCandidates,
  getCupOpponentReferenceKey,
} from "../configure/cupSpec/CupOpponentUtils";

const CUP_NAMES = ["Bronze Cup", "Silver Cup", "Gold Cup", "Platinum Cup"];
const RATING_LABELS = ["Rookie", "Amateur", "Advanced", "Semi-Pro", "Pro", "Super Pro"];

function getOpponentReferenceLabel(reference) {
  if (reference?.type === "car") {
    return reference.name || reference.folder || "Unknown car";
  }
  if (reference?.type === "slot") {
    const category = reference.category === "dc" ? "DC" : "Stock";
    return `${category} Slot ${(reference.index ?? 0) + 1}`;
  }
  return "Unknown opponent";
}

function formatPosition(position) {
  const lastTwo = position % 100;
  if (lastTwo >= 11 && lastTwo <= 13) return `${position}th`;

  switch (position % 10) {
    case 1: return `${position}st`;
    case 2: return `${position}nd`;
    case 3: return `${position}rd`;
    default: return `${position}th`;
  }
}

export function validateCupSpec(
  cupSpecState,
  trackSpecState,
  scanResult,
  featureOptions = {},
  carsSpecState = {},
  preset = "custom"
) {
  const errors = [];
  const warnings = [];

  if (!cupSpecState?.enabled) return { errors, warnings };

  const maxCupCars = featureOptions.enable30CarMode
    ? EXTENDED_MAX_CUP_CARS
    : NATIVE_MAX_CUP_CARS;
  const globalNumCars = cupSpecState.numCars ?? 8;
  const globalPerRacePlace = cupSpecState.perRaceRequiredPlace ?? 3;
  const globalOverallPlace = cupSpecState.overallRequiredPlace ?? 1;
  const globalPoints = cupSpecState.pointsTable;

  const isValidInteger = value => Number.isInteger(value);
  const isValidCarCount = value =>
    isValidInteger(value) && value >= 1 && value <= maxCupCars;

  const checkCarCount = (value, label, field, id) => {
    if (!isValidCarCount(value)) {
      const extendedModeHint = !featureOptions.enable30CarMode &&
        typeof value === "number" &&
        value > NATIVE_MAX_CUP_CARS
        ? " Enable 30-Car Mode to use up to 30 cars."
        : "";
      errors.push({
        id,
        scope: "cupSpec",
        field,
        message: `${label}: Choose a total car count from 1 through ${maxCupCars}.${extendedModeHint}`
      });
      return false;
    }
    return true;
  };

  const checkRequiredPlace = (value, numCars, label, field, id) => {
    const upperBound = isValidCarCount(numCars) ? numCars : maxCupCars;
    if (!isValidInteger(value) || value < 1 || value > upperBound) {
      errors.push({
        id,
        scope: "cupSpec",
        field,
        message: `${label}: Choose a finishing position from 1st through ${formatPosition(upperBound)} place.`
      });
    }
  };

  const checkPointsTable = (points, numCars, label, field, idPrefix) => {
    if (!Array.isArray(points)) {
      errors.push({
        id: `${idPrefix}_missing`,
        scope: "cupSpec",
        field,
        message: `${label}: Enter points for every finishing position in this cup.`
      });
      return;
    }

    const activePositions = isValidCarCount(numCars) ? numCars : 1;
    if (points.length < activePositions) {
      errors.push({
        id: `${idPrefix}_too_short`,
        scope: "cupSpec",
        field,
        message: `${label}: Add points values through ${formatPosition(activePositions)} place.`
      });
    }

    if (points.length > CUP_POINTS_TABLE_LENGTH) {
      errors.push({
        id: `${idPrefix}_too_long`,
        scope: "cupSpec",
        field,
        message: `${label}: Remove points values after ${formatPosition(CUP_POINTS_TABLE_LENGTH)} place.`
      });
    }

    const invalidIndex = points.findIndex(value => !isValidInteger(value) || value < 0);
    if (invalidIndex !== -1) {
      errors.push({
        id: `${idPrefix}_invalid_value`,
        scope: "cupSpec",
        field,
        message: `${label}: Points for ${formatPosition(invalidIndex + 1)} place must be zero or greater.`
      });
    }
  };

  checkCarCount(
    globalNumCars,
    "Global settings",
    "numCars",
    "cup_num_cars_invalid_global"
  );
  checkRequiredPlace(
    globalPerRacePlace,
    globalNumCars,
    "Global settings: Minimum position per race",
    "perRaceRequiredPlace",
    "cup_per_race_place_invalid_global"
  );
  checkRequiredPlace(
    globalOverallPlace,
    globalNumCars,
    "Global settings: Minimum overall position",
    "overallRequiredPlace",
    "cup_overall_place_invalid_global"
  );
  checkPointsTable(
    globalPoints,
    globalNumCars,
    "Global points table",
    "pointsTable",
    "cup_points_invalid_global"
  );

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
      ? (cup.numCars ?? globalNumCars)
      : globalNumCars;

    const numCarsIsValid = cup.overrideNumCars
      ? checkCarCount(
          numCars,
          CUP_NAMES[i],
          `cups[${i}].numCars`,
          `cup_num_cars_invalid_${i}`
        )
      : isValidCarCount(globalNumCars);

    const perRacePlace = cup.overridePerRacePlace
      ? (cup.perRaceRequiredPlace ?? globalPerRacePlace)
      : globalPerRacePlace;
    const overallPlace = cup.overrideOverallPlace
      ? (cup.overallRequiredPlace ?? globalOverallPlace)
      : globalOverallPlace;

    if (cup.overridePerRacePlace || cup.overrideNumCars) {
      checkRequiredPlace(
        perRacePlace,
        numCars,
        `${CUP_NAMES[i]}: Minimum position per race`,
        `cups[${i}].perRaceRequiredPlace`,
        `cup_per_race_place_invalid_${i}`
      );
    }
    if (cup.overrideOverallPlace || cup.overrideNumCars) {
      checkRequiredPlace(
        overallPlace,
        numCars,
        `${CUP_NAMES[i]}: Minimum overall position`,
        `cups[${i}].overallRequiredPlace`,
        `cup_overall_place_invalid_${i}`
      );
    }
    if (cup.overridePointsTable) {
      checkPointsTable(
        cup.pointsTable,
        numCars,
        `${CUP_NAMES[i]} points table`,
        `cups[${i}].pointsTable`,
        `cup_points_invalid_${i}`
      );
    } else if (cup.overrideNumCars) {
      checkPointsTable(
        globalPoints,
        numCars,
        `${CUP_NAMES[i]} inherited points table`,
        "pointsTable",
        `cup_points_invalid_${i}`
      );
    }

    const expected = numCarsIsValid ? numCars - 1 : null;

    // Effective carsPerClass for this cup
    const effectiveCpc = cup.overrideCarsPerClass
      ? (cup.carsPerClass || [])
      : (DEFAULT_CARS_PER_CLASS[i] || []);

    const cpcIsValid = Array.isArray(effectiveCpc) &&
      effectiveCpc.length === 6 &&
      effectiveCpc.every(value => isValidInteger(value) && value >= 0);
    const sum = cpcIsValid
      ? effectiveCpc.reduce((s, v) => s + v, 0)
      : 0;

    if (expected !== null && (!cpcIsValid || sum !== expected)) {
      const hint = !cup.overrideCarsPerClass
        ? " Enable the Cars per Class override to configure it."
        : "";
      errors.push({
        id: `cup_cpc_mismatch_${i}`,
        scope: "cupSpec",
        field: `cups[${i}].carsPerClass`,
        message: cpcIsValid
          ? `${CUP_NAMES[i]} Cars per Class must add up to ${expected} CPU opponents. It currently adds up to ${sum}.${hint}`
          : `${CUP_NAMES[i]} Cars per Class cannot include a negative number of cars for a class.${hint}`
      });
    }

    // ── Specific opponents ──
    // Cars Per Class determines how many configured opponents are used for a
    // rating. A non-zero class may have one additional fallback entry, but
    // there is no global limit on the total opponents reference list.
    if (cup.overrideOpponents && scanResult) {
      const opponents = cup.opponents;

      if (!Array.isArray(opponents)) {
        errors.push({
          id: `cup_opponents_invalid_${i}`,
          scope: "cupSpec",
          field: `cups[${i}].opponents`,
          message: `${CUP_NAMES[i]} Specific Opponents must contain one list for each rating.`
        });
      } else if (
        opponents.length !== RATING_LABELS.length ||
        opponents.some(selectionGroup => !Array.isArray(selectionGroup))
      ) {
        errors.push({
          id: `cup_opponents_invalid_${i}`,
          scope: "cupSpec",
          field: `cups[${i}].opponents`,
          message: `${CUP_NAMES[i]} Specific Opponents must contain one list for each rating.`
        });
      } else {
        const candidateByKey = new Map(
          getCupOpponentCandidates({ scanResult, carsSpecState, preset })
            .flatMap(group => group.candidates)
            .map(candidate => [
              getCupOpponentReferenceKey(candidate.reference),
              candidate,
            ])
        );
        const seenReferences = new Set();
        const countsByRating = Array(RATING_LABELS.length).fill(0);
        const reportUnavailable = (reference, rating, opponentIndex, field) => {
          const ratingLabel = Number.isInteger(rating)
            ? `${RATING_LABELS[rating]} `
            : "";
          errors.push({
            id: `cup_opponent_invalid_${i}_${rating}_${opponentIndex}`,
            scope: "cupSpec",
            field,
            message:
              `${CUP_NAMES[i]} - Specific Opponents: ` +
              `${getOpponentReferenceLabel(reference)} is no longer available. ` +
              `Choose a different ${ratingLabel}car or slot.`
          });
        };

        const validateReference = (reference, rating, opponentIndex, field) => {
          const key = getCupOpponentReferenceKey(reference);
          const candidate = candidateByKey.get(key);

          if (!key || !candidate ||
              candidate.rating !== rating) {
            reportUnavailable(reference, rating, opponentIndex, field);
            return;
          }

          if (seenReferences.has(key)) {
            errors.push({
              id: `cup_opponent_duplicate_${i}_${rating}_${opponentIndex}`,
              scope: "cupSpec",
              field,
              message:
                `${CUP_NAMES[i]} - Specific Opponents: ` +
                `${getOpponentReferenceLabel(reference)} is already selected. ` +
                `Choose a different ${ratingLabel}car or slot.`
            });
            return;
          }

          seenReferences.add(key);
          countsByRating[rating] += 1;
        };

        opponents.forEach((selectionGroup, rating) => {
          selectionGroup.forEach((reference, opponentIndex) => {
            validateReference(
              reference,
              rating,
              opponentIndex,
              `cups[${i}].opponents[${rating}][${opponentIndex}]`
            );
          });
        });

        if (cpcIsValid) {
          countsByRating.forEach((count, rating) => {
            const quota = effectiveCpc[rating];
            const maximum = quota === 0 ? 0 : quota + 1;
            if (count <= maximum) return;

            const limitDescription = quota === 0
              ? "Cars Per Class is set to 0 for this class"
              : `Cars Per Class allows ${quota} CPU opponent${quota === 1 ? "" : "s"} plus one fallback`;
            errors.push({
              id: `cup_opponent_count_invalid_${i}_${rating}`,
              scope: "cupSpec",
              field: `cups[${i}].opponents`,
              message:
                `${CUP_NAMES[i]} has too many ${RATING_LABELS[rating]} opponents configured. ` +
                `${limitDescription} (maximum ${maximum}).`
            });
          });
        }
      }
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

    const effectiveStageMode = cup.overrideStageMode ? cup.stageMode : cupSpecState.stageMode;
    const effectiveNumLapsMin = cup.overrideNumLapsMin ? cup.numLapsMin : cupSpecState.numLapsMin;
    const effectiveNumLapsMax = cup.overrideNumLapsMax ? cup.numLapsMax : cupSpecState.numLapsMax;

    // Laps range override is only meaningful when using random stage mode
    if (effectiveStageMode === "random") {
      if (cup.overrideNumLapsMin || cup.overrideNumLapsMax) {
        checkLaps(effectiveNumLapsMin, effectiveNumLapsMax, CUP_NAMES[i]);
      }
    }
  });

  // ── Stage count range sanity ──────────────────────────────────────────────────────
  const checkStages = (min, max, label) => {
    if (min > max) {
      errors.push({
        id: `cup_stages_invalid_${label}`,
        scope: "cupSpec",
        message: `${label}: Minimum number of stages cannot be greater than maximum number of stages.`
      });
    }
  };

  checkStages(cupSpecState.numStagesMin, cupSpecState.numStagesMax, "Global settings");

  cupSpecState.cups?.forEach((cup, i) => {

    const effectiveStageMode = cup.overrideStageMode ? cup.stageMode : cupSpecState.stageMode;
    const effectiveNumStagesMin = cup.overrideNumStagesMin ? cup.numStagesMin : cupSpecState.numStagesMin;
    const effectiveNumStagesMax = cup.overrideNumStagesMax ? cup.numStagesMax : cupSpecState.numStagesMax;

    // Stage count range override is only meaningful when using random stage mode
    if (effectiveStageMode === "random") {
      if (cup.overrideNumStagesMin || cup.overrideNumStagesMax) {
        checkStages(effectiveNumStagesMin, effectiveNumStagesMax, CUP_NAMES[i]);
      }
    }
  });

  // ── User-defined stages validation ────────────────────────────────────────
  // Compute each cup's effective stage mode (per-cup override beats global).
  const effectiveMode = (cup) =>
    cup.overrideStageMode ? (cup.stageMode ?? cupSpecState.stageMode) : cupSpecState.stageMode;
  const activeSlotCount = trackSpecState?.tracks?.length ?? 14;
  const allTracks = getAllTracksFromScan(scanResult);
  const availableTrackFolders = getTrackSpecAvailableFolders(trackSpecState, allTracks);

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
      const slotMatch = typeof pool === "string" ? pool.match(/^slot:(\d+)$/i) : null;

      if (slotMatch) {
        const slotIndex = parseInt(slotMatch[1], 10);
        if (!Number.isInteger(slotIndex) || slotIndex < 0 || slotIndex >= activeSlotCount) {
          errors.push({
            id: `cup_stage_slot_invalid_${cupIdx}_${stageIdx}`,
            scope: "cupSpec",
            field: `cups[${cupIdx}].stages[${stageIdx}]`,
            message:
              `${CUP_NAMES[cupIdx]}, Stage ${stageIdx + 1}: Slot ${slotIndex + 1} is not available ` +
              `in the current track setup. Choose a slot between 1 and ${activeSlotCount}.`
          });
        }
        return;
      }

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
  // RVGL loads a fixed number of track slots. Every track folder that is pinned by
  // name (in either the Track Spec or User-Defined cup stages) must fit within
  // the active slot count. If the union of all pinned folder names exceeds it,
  // generation is impossible.

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
  if (allPinned.size > activeSlotCount) {
    errors.push({
      id: "cup_too_many_specific_tracks",
      scope: "cupSpec",
      message:
        `${allPinned.size} distinct tracks are required (` +
        `${pinnedInTrackSpec.size} pinned in Track Specification, ` +
        `${pinnedInCupStages.size} in Cup Stages), ` +
        `but the current setup only supports ${activeSlotCount} track slots. ` +
        `Reduce the number of specific track references.`
    });
  }

  return { errors, warnings };
}
