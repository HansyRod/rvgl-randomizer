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
  { val: "-1", label: "Cheat Only" }
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
  { val: "5", label: "Stunt Arena" }
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
};

export function makeDefaultTrackSpec(ids) {
  return ids.map(id => ({
    id,
    sourcePool: "Full Random",
    sourceDifficulty: "Random",
    attrDifficulty: "Random",
    attrObtain: "Random",
  }));
}

export function makeDefaultCarsSpec(ids) {
  return ids.map(id => ({
    id,
    sourcePool: "Full Random",
    sourceRating: "Random",
    sourceObtain: "Random",
    attrRating: "Random",
    attrObtain: "Random",
  }));
}
