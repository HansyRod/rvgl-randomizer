import { DEFAULT_CAR_OPTIONS, DEFAULT_TRACK_OPTIONS } from "../../utils/constants";
import { makeDefaultCupSpecState } from "../cupSpec/CupSpecTab";

export const ALL_ROOKIES_PRESET = {
  id: "all-rookies",
  label: "All Rookies",
  tag: "Beginner-friendly",
  description:
    "Explore the variety of Rookie cars with this chill preset. Requires a community content pack to get the necessary amount of rookie cars.",
  bullets: [
    "All 42 cars included are originally Rookies.",
    "Start with 10 cars, and unlock more by winning championships and single races.",
    "Get 14 random tracks, with 4 Easy, 4 Medium, 3 Hard and 3 Extreme.",
    "Easier cups: Finish top 5 to progress to the next stage, get more retries, and finish top 3 overall to win the championship."
  ],
  configure: {
    carOptions: {
      ...DEFAULT_CAR_OPTIONS,
      unlockMode: "random",
      includeChampionship: true,
      includeSingleRace: true,
      includeCheatOnly: false,
      includePracticeStars: false,
      includeStartingCar: false,
      includeStuntArena: false,
      includeTimeTrial: false,
      includeSuperPro: false,
      enableStartingCars: true,
      numStartingCars: 8,
      poolRatingDistributions: {
        0: {
          enabled: true,
          max: 42,
          min: 42
        },
        1: {
          enabled: true,
          max: 0,
          min: 0
        },
        2: {
          enabled: true,
          max: 0,
          min: 0
        },
        3: {
          enabled: true,
          max: 0,
          min: 0
        },
        4: {
          enabled: true,
          max: 0,
          min: 0
        },
        5: {
          enabled: true,
          max: 0,
          min: 0
        }
      },
    },
    carsSpecState: {
      includeStockCars: true,
      includeDcCars: true,
      dcCars: [
        {
          attrObtain: "0",
          attrRating: "0",
          id: "bigvolt",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "Random"
        },
        {
          attrObtain: "0",
          attrRating: "0",
          id: "bossvolt",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "Random"
        },
        {
          attrObtain: "4",
          attrRating: "1",
          id: "jg6rc",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "Random"
        },
        {
          attrObtain: "4",
          attrRating: "1",
          id: "tc12",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "Random"
        },
        {
          attrObtain: "4",
          attrRating: "1",
          id: "tc10",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "Random"
        },
        {
          attrObtain: "4",
          attrRating: "2",
          id: "tc11",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "Random"
        },
        {
          attrObtain: "4",
          attrRating: "2",
          id: "tc9",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "Random"
        },
        {
          attrObtain: "4",
          attrRating: "3",
          id: "jg1jg7",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "Random"
        },
        {
          attrObtain: "4",
          attrRating: "3",
          id: "tc7",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "Random"
        },
        {
          attrObtain: "4",
          attrRating: "3",
          id: "jg3loco",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "Random"
        },
        {
          attrObtain: "4",
          attrRating: "4",
          id: "jg4snw35",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "Random"
        },
        {
          attrObtain: "4",
          attrRating: "4",
          id: "jg5purpxl",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "Random"
        },
        {
          attrObtain: "4",
          attrRating: "4",
          id: "jg2fulonx",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "Random"
        }
      ],
      stockCars: [
        {
          attrObtain: "0",
          attrRating: "0",
          id: "rc",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "Random"
        },
        {
          attrObtain: "0",
          attrRating: "0",
          id: "mite",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "Random"
        },
        {
          attrObtain: "0",
          attrRating: "0",
          id: "phat",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "Random"
        },
        {
          attrObtain: "0",
          attrRating: "0",
          id: "moss",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "Random"
        },
        {
          attrObtain: "0",
          attrRating: "0",
          id: "mud",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "Random"
        },
        {
          attrObtain: "0",
          attrRating: "0",
          id: "beatall",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "Random"
        },
        {
          attrObtain: "0",
          attrRating: "0",
          id: "volken",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "Random"
        },
        {
          attrObtain: "0",
          attrRating: "0",
          id: "tc6",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "Random"
        },
        {
          attrObtain: "1",
          attrRating: "1",
          id: "dino",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "Random"
        },
        {
          attrObtain: "1",
          attrRating: "1",
          id: "candy",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "Random"
        },
        {
          attrObtain: "1",
          attrRating: "1",
          id: "gencar",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "Random"
        },
        {
          attrObtain: "1",
          attrRating: "1",
          id: "tc4",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "Random"
        },
        {
          attrObtain: "1",
          attrRating: "1",
          id: "mouse",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "Random"
        },
        {
          attrObtain: "1",
          attrRating: "2",
          id: "flag",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "Random"
        },
        {
          attrObtain: "1",
          attrRating: "2",
          id: "tc2",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "Random"
        },
        {
          attrObtain: "1",
          attrRating: "2",
          id: "r5",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "Random"
        },
        {
          attrObtain: "1",
          attrRating: "2",
          id: "tc5",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "Random"
        },
        {
          attrObtain: "1",
          attrRating: "2",
          id: "sgt",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "Random"
        },
        {
          attrObtain: "1",
          attrRating: "3",
          id: "tc3",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "Random"
        },
        {
          attrObtain: "1",
          attrRating: "3",
          id: "adeon",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "Random"
        },
        {
          attrObtain: "1",
          attrRating: "3",
          id: "fone",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "Random"
        },
        {
          attrObtain: "1",
          attrRating: "3",
          id: "tc1",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "Random"
        },
        {
          attrObtain: "1",
          attrRating: "3",
          id: "rotor",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "Random"
        },
        {
          attrObtain: "1",
          attrRating: "4",
          id: "cougar",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "Random"
        },
        {
          attrObtain: "1",
          attrRating: "4",
          id: "sugo",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "Random"
        },
        {
          attrObtain: "1",
          attrRating: "4",
          id: "toyeca",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "Random"
        },
        {
          attrObtain: "1",
          attrRating: "4",
          id: "amw",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "Random"
        },
        {
          attrObtain: "1",
          attrRating: "4",
          id: "panga",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "Random"
        }
      ]
    },
    cupSpecState: {
      ...makeDefaultCupSpecState(),
      numTries: 10,
      overallRequiredPlace: 3,
      perRaceRequiredPlace: 5,
      pointsTable: [10, 8, 6, 5, 4, 3, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0],
    },
    trackOptions: {
      ...DEFAULT_TRACK_OPTIONS,
      unlockMode: "baseGame"
    },
    trackSpecState: {
      includeTracks: true,
      tracks: [
        {
          attrDifficulty: "1",
          attrObtain: "0",
          id: "nhood1",
          sourceDifficulty: "Random",
          sourcePool: "Full Random"
        },
        {
          attrDifficulty: "1",
          attrObtain: "0",
          id: "market2",
          sourceDifficulty: "Random",
          sourcePool: "Full Random"
        },
        {
          attrDifficulty: "1",
          attrObtain: "0",
          id: "muse2",
          sourceDifficulty: "Random",
          sourcePool: "Full Random"
        },
        {
          attrDifficulty: "1",
          attrObtain: "0",
          id: "garden1",
          sourceDifficulty: "Random",
          sourcePool: "Full Random"
        },
        {
          attrDifficulty: "2",
          attrObtain: "0",
          id: "roof",
          sourceDifficulty: "Random",
          sourcePool: "Full Random"
        },
        {
          attrDifficulty: "2",
          attrObtain: "0",
          id: "toylite",
          sourceDifficulty: "Random",
          sourcePool: "Full Random"
        },
        {
          attrDifficulty: "2",
          attrObtain: "0",
          id: "wild_west1",
          sourceDifficulty: "Random",
          sourcePool: "Full Random"
        },
        {
          attrDifficulty: "2",
          attrObtain: "0",
          id: "toy2",
          sourceDifficulty: "Random",
          sourcePool: "Full Random"
        },
        {
          attrDifficulty: "3",
          attrObtain: "0",
          id: "nhood2",
          sourceDifficulty: "Random",
          sourcePool: "Full Random"
        },
        {
          attrDifficulty: "3",
          attrObtain: "0",
          id: "ship1",
          sourceDifficulty: "Random",
          sourcePool: "Full Random"
        },
        {
          attrDifficulty: "3",
          attrObtain: "0",
          id: "muse1",
          sourceDifficulty: "Random",
          sourcePool: "Full Random"
        },
        {
          attrDifficulty: "4",
          attrObtain: "0",
          id: "market1",
          sourceDifficulty: "Random",
          sourcePool: "Full Random"
        },
        {
          attrDifficulty: "4",
          attrObtain: "0",
          id: "wild_west2",
          sourceDifficulty: "Random",
          sourcePool: "Full Random"
        },
        {
          attrDifficulty: "4",
          attrObtain: "0",
          id: "ship2",
          sourceDifficulty: "Random",
          sourcePool: "Full Random"
        }
      ]
    }
  },
};