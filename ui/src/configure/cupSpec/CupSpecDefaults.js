import {
  DEFAULT_POINTS,
  DEFAULT_CARS_PER_CLASS,
} from "./CupUtils.jsx";

export function makeDefaultCupSpec(index) {
  return {
    index,
    // Per-field override flags (each independent)
    overrideStageMode: false,
    overrideNumCars: false,
    overrideCarsPerClass: false,
    overrideNumTries: false,
    overridePerRacePlace: false,
    overrideOverallPlace: false,
    overridePointsTable: false,
    overrideOpponents: false,
    overrideNumStagesMin: false,
    overrideNumStagesMax: false,
    overrideNumLapsMin: false,
    overrideNumLapsMax: false,
    overrideMaxRaceLength: false,
    // Per-cup values (used when the corresponding override flag is true)
    stageMode: "default",
    numStagesMin: 3,
    numStagesMax: 6,
    numLapsMin: 2,
    numLapsMax: 8,
    maxRaceLength: null,
    stages: [],
    numCars: 8,
    numTries: 3,
    perRaceRequiredPlace: 3,
    overallRequiredPlace: 1,
    pointsTable: [...DEFAULT_POINTS],
    carsPerClass: [...DEFAULT_CARS_PER_CLASS[index]],
    opponents: Array.from({ length: 6 }, () => []),
  };
}

export function makeDefaultCupSpecState() {
  return {
    enabled: true,
    stageMode: "default",
    guaranteeFirstNormal: true,
    sameTrackHandling: "forbid",
    allowReverse: true,
    allowMirror: false,
    allowReverseMirror: false,
    numCars: 8,
    numTries: 3,
    perRaceRequiredPlace: 3,
    overallRequiredPlace: 1,
    pointsTable: [...DEFAULT_POINTS],
    numLapsMin: 2,
    numLapsMax: 8,
    maxRaceLength: null,
    numStagesMin: 3,
    numStagesMax: 6,
    cups: [0, 1, 2, 3].map(makeDefaultCupSpec),
  };
}
