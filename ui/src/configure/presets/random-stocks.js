import { DEFAULT_CAR_OPTIONS, DEFAULT_TRACK_OPTIONS, DC_CARS, makeDefaultCarsSpec, STOCK_CARS, STOCK_TRACKS } from "../../utils/constants";
import { makeDefaultCupSpecState } from "../cupSpec/CupSpecTab";

import { countEligibleCarsByFolderNames, countEligibleTracksByFolderNames } from "./presetValidation";

const REQUIRED_STOCK_TRACKS = STOCK_TRACKS.filter((trackId) => trackId !== "roof");

export const RANDOM_STOCKS_PRESET = {
  id: "random-stocks",
  label: "Random Stocks",
  tag: "Unmodded",
  stockMode: {
    cars: true,
    tracks: true,
  },
  description:
    "Race your familiar cars and tracks with a twist! Use this preset if you want to play only with the original game content.",
  bullets: [
    "All 28 stock cars and 13 stock tracks are included. Cars keep their original ratings.",
    "Start with 8 Rookies. Other car unlocks are randomized.",
    "The track list is randomized but keeps the original difficulty distribution.",
    "Each cup has 4 to 5 random stages, with 3 to 6 laps each. Other cup settings are unchanged.",
  ],
  validateSelection: ({ scanResult }) => {
    const errors = [];
    const availableStockCars = countEligibleCarsByFolderNames(scanResult, STOCK_CARS);
    const availableStockTracks = countEligibleTracksByFolderNames(scanResult, REQUIRED_STOCK_TRACKS);

    if (availableStockCars < STOCK_CARS.length) {
      errors.push(
        `This preset requires all ${STOCK_CARS.length} stock cars, but only ${availableStockCars} are currently available.`
      );
    }

    if (availableStockTracks < REQUIRED_STOCK_TRACKS.length) {
      errors.push(
        `This preset requires all ${REQUIRED_STOCK_TRACKS.length} stock tracks, but only ${availableStockTracks} are currently available.`
      );
    }

    return errors;
  },
  configure: {
    carOptions: {
      ...DEFAULT_CAR_OPTIONS,
      unlockMode: "randomUnlock",
      "includeStuntArena": true
    },
    carsSpecState: {
      includeStockCars: true,
      includeDcCars: false,
      dcCars: [],
      stockCars: [
        {
          attrObtain: "0",
          attrRating: "Unchanged",
          id: "rc",
          sourceObtain: "Random",
          sourcePool: "Stock",
          sourceRating: "0"
        },
        {
          attrObtain: "0",
          attrRating: "Unchanged",
          id: "mite",
          sourceObtain: "Random",
          sourcePool: "Stock",
          sourceRating: "0"
        },
        {
          attrObtain: "0",
          attrRating: "Unchanged",
          id: "phat",
          sourceObtain: "Random",
          sourcePool: "Stock",
          sourceRating: "0"
        },
        {
          attrObtain: "0",
          attrRating: "Unchanged",
          id: "moss",
          sourceObtain: "Random",
          sourcePool: "Stock",
          sourceRating: "0"
        },
        {
          attrObtain: "0",
          attrRating: "Unchanged",
          id: "mud",
          sourceObtain: "Random",
          sourcePool: "Stock",
          sourceRating: "0"
        },
        {
          attrObtain: "0",
          attrRating: "Unchanged",
          id: "beatall",
          sourceObtain: "Random",
          sourcePool: "Stock",
          sourceRating: "0"
        },
        {
          attrObtain: "0",
          attrRating: "Unchanged",
          id: "volken",
          sourceObtain: "Random",
          sourcePool: "Stock",
          sourceRating: "0"
        },
        {
          attrObtain: "0",
          attrRating: "Unchanged",
          id: "tc6",
          sourceObtain: "Random",
          sourcePool: "Stock",
          sourceRating: "0"
        },
        {
          attrObtain: "Random",
          attrRating: "Unchanged",
          id: "dino",
          sourceObtain: "Random",
          sourcePool: "Stock",
          sourceRating: "1"
        },
        {
          attrObtain: "Random",
          attrRating: "Unchanged",
          id: "candy",
          sourceObtain: "Random",
          sourcePool: "Stock",
          sourceRating: "1"
        },
        {
          attrObtain: "Random",
          attrRating: "Unchanged",
          id: "gencar",
          sourceObtain: "Random",
          sourcePool: "Stock",
          sourceRating: "1"
        },
        {
          attrObtain: "Random",
          attrRating: "Unchanged",
          id: "tc4",
          sourceObtain: "Random",
          sourcePool: "Stock",
          sourceRating: "1"
        },
        {
          attrObtain: "Random",
          attrRating: "Unchanged",
          id: "mouse",
          sourceObtain: "Random",
          sourcePool: "Stock",
          sourceRating: "1"
        },
        {
          attrObtain: "Random",
          attrRating: "Unchanged",
          id: "flag",
          sourceObtain: "Random",
          sourcePool: "Stock",
          sourceRating: "2"
        },
        {
          attrObtain: "Random",
          attrRating: "Unchanged",
          id: "tc2",
          sourceObtain: "Random",
          sourcePool: "Stock",
          sourceRating: "2"
        },
        {
          attrObtain: "Random",
          attrRating: "Unchanged",
          id: "r5",
          sourceObtain: "Random",
          sourcePool: "Stock",
          sourceRating: "2"
        },
        {
          attrObtain: "Random",
          attrRating: "Unchanged",
          id: "tc5",
          sourceObtain: "Random",
          sourcePool: "Stock",
          sourceRating: "2"
        },
        {
          attrObtain: "Random",
          attrRating: "Unchanged",
          id: "sgt",
          sourceObtain: "Random",
          sourcePool: "Stock",
          sourceRating: "2"
        },
        {
          attrObtain: "Random",
          attrRating: "Unchanged",
          id: "tc3",
          sourceObtain: "Random",
          sourcePool: "Stock",
          sourceRating: "3"
        },
        {
          attrObtain: "Random",
          attrRating: "Unchanged",
          id: "adeon",
          sourceObtain: "Random",
          sourcePool: "Stock",
          sourceRating: "3"
        },
        {
          attrObtain: "Random",
          attrRating: "Unchanged",
          id: "fone",
          sourceObtain: "Random",
          sourcePool: "Stock",
          sourceRating: "3"
        },
        {
          attrObtain: "Random",
          attrRating: "Unchanged",
          id: "tc1",
          sourceObtain: "Random",
          sourcePool: "Stock",
          sourceRating: "3"
        },
        {
          attrObtain: "Random",
          attrRating: "Unchanged",
          id: "rotor",
          sourceObtain: "Random",
          sourcePool: "Stock",
          sourceRating: "3"
        },
        {
          attrObtain: "Random",
          attrRating: "Unchanged",
          id: "cougar",
          sourceObtain: "Random",
          sourcePool: "Stock",
          sourceRating: "4"
        },
        {
          attrObtain: "Random",
          attrRating: "Unchanged",
          id: "sugo",
          sourceObtain: "Random",
          sourcePool: "Stock",
          sourceRating: "4"
        },
        {
          attrObtain: "Random",
          attrRating: "Unchanged",
          id: "toyeca",
          sourceObtain: "Random",
          sourcePool: "Stock",
          sourceRating: "4"
        },
        {
          attrObtain: "Random",
          attrRating: "Unchanged",
          id: "amw",
          sourceObtain: "Random",
          sourcePool: "Stock",
          sourceRating: "4"
        },
        {
          attrObtain: "Random",
          attrRating: "Unchanged",
          id: "panga",
          sourceObtain: "Random",
          sourcePool: "Stock",
          sourceRating: "4"
        }
      ]
    },
    cupSpecState: {
      ...makeDefaultCupSpecState(),
      stageMode: "random",
      sameTrackHandling: "forbid",
      allowMirror: true,
      allowReverse: true,
      allowReverseMirror: true,
      guaranteeFirstNormal: true,
      numLapsMax: 6,
      numLapsMin: 3,
      numStagesMax: 5,
      numStagesMin: 4
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
          sourcePool: "Stock"
        },
        {
          attrDifficulty: "1",
          attrObtain: "0",
          id: "market2",
          sourceDifficulty: "Random",
          sourcePool: "Stock"
        },
        {
          attrDifficulty: "1",
          attrObtain: "0",
          id: "muse2",
          sourceDifficulty: "Random",
          sourcePool: "Stock"
        },
        {
          attrDifficulty: "1",
          attrObtain: "0",
          id: "garden1",
          sourceDifficulty: "Random",
          sourcePool: "Stock"
        },
        {
          attrDifficulty: "2",
          attrObtain: "0",
          id: "toylite",
          sourceDifficulty: "Random",
          sourcePool: "Stock"
        },
        {
          attrDifficulty: "2",
          attrObtain: "0",
          id: "wild_west1",
          sourceDifficulty: "Random",
          sourcePool: "Stock"
        },
        {
          attrDifficulty: "2",
          attrObtain: "0",
          id: "toy2",
          sourceDifficulty: "Random",
          sourcePool: "Stock"
        },
        {
          attrDifficulty: "3",
          attrObtain: "0",
          id: "nhood2",
          sourceDifficulty: "Random",
          sourcePool: "Stock"
        },
        {
          attrDifficulty: "3",
          attrObtain: "0",
          id: "ship1",
          sourceDifficulty: "Random",
          sourcePool: "Stock"
        },
        {
          attrDifficulty: "3",
          attrObtain: "0",
          id: "muse1",
          sourceDifficulty: "Random",
          sourcePool: "Stock"
        },
        {
          attrDifficulty: "4",
          attrObtain: "0",
          id: "market1",
          sourceDifficulty: "Random",
          sourcePool: "Stock"
        },
        {
          attrDifficulty: "4",
          attrObtain: "0",
          id: "wild_west2",
          sourceDifficulty: "Random",
          sourcePool: "Stock"
        },
        {
          attrDifficulty: "4",
          attrObtain: "0",
          id: "ship2",
          sourceDifficulty: "Random",
          sourcePool: "Stock"
        }
      ]
    }
  },
}
