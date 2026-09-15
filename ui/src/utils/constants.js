export const CAR_RATINGS = {
  [-2]: "Unknown",
  [-1]: "None",
  0: "Rookie",
  1: "Amateur",
  2: "Advanced",
  3: "Semi-Pro",
  4: "Pro",
  5: "Super Pro"
};

export const OBTAIN_METHODS = {
  [-2]: "Unknown",
  [-1]: "Cheat Only",
  0: "Starting Car",
  1: "Championship",
  2: "Time Trial",
  3: "Practice",
  4: "Single Race"
};

export const TRACK_DIFFICULTIES = {
  1: "Easy",
  2: "Medium",
  3: "Hard",
  4: "Extreme"
};

export const TRACK_OBTAIN_METHODS = {
  [-1]: "Cheat Only",
  0: "Default",
  1: "Championship",
  2: "Time Trial",
  3: "Practice",
  4: "Single Race",
  5: "Stunt Arena"
};

export const CUSTOM_UNLOCK_METHODS = {
  6: "Specific Race Win",
  7: "Specific Practice Star",
  8: "Specific Time Trial",
  9: "Race Win Count",
  10: "Practice Star Count",
  11: "Time Trial Count",
  12: "Stunt Arena Star Count"
};

export const CUSTOM_UNLOCK_DESCRIPTIONS = {
  6: "Unlock after winning every listed prerequisite race.",
  7: "Unlock after collecting the practice star on every listed prerequisite track.",
  8: "Unlock after beating the normal time trial on every listed prerequisite track.",
  9: "Unlock after winning the configured number of races.",
  10: "Unlock after collecting the configured number of practice stars.",
  11: "Unlock after beating the configured number of normal time trials.",
  12: "Unlock after collecting the configured number of Stunt Arena stars."
};

export const CUSTOM_UNLOCK_SPECIFIC_METHODS = [
  { val: "6", label: CUSTOM_UNLOCK_METHODS[6], description: CUSTOM_UNLOCK_DESCRIPTIONS[6] },
  { val: "7", label: CUSTOM_UNLOCK_METHODS[7], description: CUSTOM_UNLOCK_DESCRIPTIONS[7] },
  { val: "8", label: CUSTOM_UNLOCK_METHODS[8], description: CUSTOM_UNLOCK_DESCRIPTIONS[8] }
];

export const CUSTOM_UNLOCK_COUNT_METHODS = [
  { val: "9", label: CUSTOM_UNLOCK_METHODS[9], description: CUSTOM_UNLOCK_DESCRIPTIONS[9] },
  { val: "10", label: CUSTOM_UNLOCK_METHODS[10], description: CUSTOM_UNLOCK_DESCRIPTIONS[10] },
  { val: "11", label: CUSTOM_UNLOCK_METHODS[11], description: CUSTOM_UNLOCK_DESCRIPTIONS[11] },
  { val: "12", label: CUSTOM_UNLOCK_METHODS[12], description: CUSTOM_UNLOCK_DESCRIPTIONS[12] }
];

export const CUSTOM_UNLOCK_TARGET_OBTAINS_LIST = [
  ...CUSTOM_UNLOCK_SPECIFIC_METHODS,
  ...CUSTOM_UNLOCK_COUNT_METHODS
];

export const OVERRIDE_CARBOXES = [
  "adeon", "amw", "beatall", "bigvolt", "bossvolt", "candy", "cougar", "dino",
  "flag", "fone", "gencar", "jg1jg7", "jg2fulonx", "jg3loco", "jg4snw35",
  "jg5purpxl", "jg6rc", "mite", "moss", "mouse", "mud", "panga", "path", "r5",
  "rc", "rotor", "sgt", "sugo", "tc1", "tc10", "tc11", "tc12", "tc2", "tc3",
  "tc4", "tc5", "tc6", "tc7", "tc8", "tc9", "toyeca", "volken"
];

export const STOCK_CARS = [
  "rc", "mite", "phat", "moss", "mud", "beatall", "volken",
  "tc6", "dino", "candy", "gencar", "tc4", "mouse", "flag",
  "tc2", "r5", "tc5", "sgt", "tc3", "adeon", "fone",
  "tc1", "rotor", "cougar", "sugo", "toyeca", "amw", "panga"
];

export const DC_CARS = [
  "bigvolt", "bossvolt", "jg6rc", "tc12", "tc10", "tc8", "tc11",
  "tc9", "jg1jg7", "tc7", "jg3loco", "jg4snw35", "jg5purpxl", "jg2fulonx"
];

export const STOCK_TRACKS = [
  "nhood1", "market2", "muse2", "garden1", "roof", "toylite", "wild_west1",
  "toy2", "nhood2", "ship1", "muse1", "market1", "wild_west2", "ship2"
];

export const RATINGS_LIST = [
  { val: "Random", label: "Random" },
  { val: "0", label: "Rookie" },
  { val: "1", label: "Amateur" },
  { val: "2", label: "Advanced" },
  { val: "3", label: "Semi-Pro" },
  { val: "4", label: "Pro" },
  { val: "5", label: "Super Pro" }
];

export const ATTR_RATINGS_LIST = [
  { val: "Random", label: "Random" },
  { val: "Unchanged", label: "Unchanged" },
  { val: "0", label: "Rookie" },
  { val: "1", label: "Amateur" },
  { val: "2", label: "Advanced" },
  { val: "3", label: "Semi-Pro" },
  { val: "4", label: "Pro" },
  { val: "5", label: "Super Pro" }
];

export const OBTAINS_LIST = [
  { val: "Random", label: "Random" },
  { val: "0", label: "Starting Car" },
  { val: "1", label: "Championship" },
  { val: "2", label: "Time Trial" },
  { val: "3", label: "Practice" },
  { val: "4", label: "Single Race" },
  { val: "-1", label: "Cheat Only" }
];

export const ATTR_OBTAINS_LIST = [
  { val: "Random", label: "Random" },
  { val: "Unchanged", label: "Unchanged" },
  { val: "0", label: "Starting Car" },
  { val: "1", label: "Championship" },
  { val: "2", label: "Time Trial" },
  { val: "3", label: "Practice" },
  { val: "4", label: "Single Race" },
  { val: "5", label: "Stunt Arena" },
  { val: "-1", label: "Cheat Only" },
  ...CUSTOM_UNLOCK_TARGET_OBTAINS_LIST
];

export const TRACK_DIFFICULTY_SOURCE_LIST = [
  { val: "Random", label: "Random" },
  { val: "1", label: "Easy" },
  { val: "2", label: "Medium" },
  { val: "3", label: "Hard" },
  { val: "4", label: "Extreme" }
];

export const TRACK_DIFFICULTY_ATTR_LIST = [
  { val: "Random", label: "Random" },
  { val: "Unchanged", label: "Unchanged" },
  { val: "1", label: "Easy" },
  { val: "2", label: "Medium" },
  { val: "3", label: "Hard" },
  { val: "4", label: "Extreme" }
];

export const TRACK_OBTAINS_LIST = [
  { val: "Random", label: "Random" },
  { val: "-1", label: "Cheat Only" },
  { val: "0", label: "Default" },
  { val: "1", label: "Championship" },
  { val: "2", label: "Time Trial" },
  { val: "3", label: "Practice" },
  { val: "4", label: "Single Race" },
  { val: "5", label: "Stunt Arena" },
  ...CUSTOM_UNLOCK_TARGET_OBTAINS_LIST
];

export const DEFAULT_CAR_OPTIONS = {
  unlockMode: "random",       // "random" | "unchanged" | "baseGame" | "randomRatings" | "randomUnlock"
  enableStartingCars: false,
  numStartingCars: 0,
  enableStartingCarsPool: false,
  startingCarsPool: "Full Random",
  enableStartingCarsRating: false,
  startingCarsRating: "Random",
  includeCheatOnly: false,
  includeStuntArena: false,
  includeStartingCar: true,
  includeChampionship: true,
  includeTimeTrial: true,
  includePracticeStars: true,
  includeSingleRace: true,
  includeSpecificRaceWin: false,
  includeSpecificPracticeStar: false,
  includeSpecificTimeTrial: false,
  includeRaceWinCount: false,
  includePracticeStarCount: false,
  includeTimeTrialCount: false,
  includeStuntArenaStarCount: false,
  specificRaceWinTrackCountMin: 1,
  specificRaceWinTrackCountMax: 1,
  specificPracticeStarTrackCountMin: 1,
  specificPracticeStarTrackCountMax: 1,
  specificTimeTrialTrackCountMin: 1,
  specificTimeTrialTrackCountMax: 1,
  raceWinCountMin: 1,
  raceWinCountMax: 14,
  practiceStarCountMin: 1,
  practiceStarCountMax: 14,
  timeTrialCountMin: 1,
  timeTrialCountMax: 14,
  stuntArenaStarCountMin: 1,
  stuntArenaStarCountMax: 20,
  includeSuperPro: true,
  poolRatingDistributions: {
    "0": { enabled: false, min: 0, max: 42 },
    "1": { enabled: false, min: 0, max: 42 },
    "2": { enabled: false, min: 0, max: 42 },
    "3": { enabled: false, min: 0, max: 42 },
    "4": { enabled: false, min: 0, max: 42 },
    "5": { enabled: false, min: 0, max: 42 },
  },
  attrRatingDistributions: {
    "0": { enabled: false, min: 0, max: 42 },
    "1": { enabled: false, min: 0, max: 42 },
    "2": { enabled: false, min: 0, max: 42 },
    "3": { enabled: false, min: 0, max: 42 },
    "4": { enabled: false, min: 0, max: 42 },
    "5": { enabled: false, min: 0, max: 42 },
  },
};

export const DEFAULT_TRACK_OPTIONS = {
  unlockMode: "random", // random | randomUnlock | randomDifficulty | unchanged | baseGame
  includeStuntArena: false,
  includeDefault: true,
  includeTimeTrial: true,
  includePractice: true,
  includeSingleRace: true,
  includeSpecificRaceWin: false,
  includeSpecificPracticeStar: false,
  includeSpecificTimeTrial: false,
  includeRaceWinCount: false,
  includePracticeStarCount: false,
  includeTimeTrialCount: false,
  includeStuntArenaStarCount: false,
  specificRaceWinTrackCountMin: 1,
  specificRaceWinTrackCountMax: 1,
  specificPracticeStarTrackCountMin: 1,
  specificPracticeStarTrackCountMax: 1,
  specificTimeTrialTrackCountMin: 1,
  specificTimeTrialTrackCountMax: 1,
  raceWinCountMin: 1,
  raceWinCountMax: 14,
  practiceStarCountMin: 1,
  practiceStarCountMax: 14,
  timeTrialCountMin: 1,
  timeTrialCountMax: 14,
  stuntArenaStarCountMin: 1,
  stuntArenaStarCountMax: 20,
};

export const DEFAULT_FEATURE_OPTIONS = {
  loadExtraCars: false,
  loadExtraTracks: false,
  loadExtraCups: false,
  enable30CarMode: false,
  enableKnockoutMode: false,
};

export const NATIVE_MAX_CUP_CARS = 16;
export const EXTENDED_MAX_CUP_CARS = 30;
export const CUP_POINTS_TABLE_LENGTH = EXTENDED_MAX_CUP_CARS;

export function makeDefaultTrackSpec(ids) {
  return ids.map(id => ({
    id,
    sourcePool: "Full Random",
    sourceDifficulty: "Random",
    attrDifficulty: "Random",
    attrObtain: "Random",
    customUnlock: null,
  }));
}

export function makeDefaultCarsSpec(ids) {
  return ids.map(makeDefaultCarSpec);
}

export function makeDefaultCarSpec(id) {
  return {
    id,
    sourcePool: "Full Random",
    sourceRating: "Random",
    sourceObtain: "Random",
    attrRating: "Random",
    attrObtain: "Random",
    customUnlock: null,
  };
}
