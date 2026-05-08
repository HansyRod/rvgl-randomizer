import { DEFAULT_CAR_OPTIONS, DEFAULT_TRACK_OPTIONS } from "../../utils/constants";
import { makeDefaultCupSpecState } from "../cupSpec/CupSpecTab";

export const STOCK_LIKE_PRESET = {
  id: "stock-like",
  label: "Stock-like",
  tag: "Beginner-friendly",
  description:
    "This preset uses the original game's progression for cars, tracks and cups, while randomizing the list of cars and tracks available.",
  bullets: [
    "Start with 8 rookie cars (+ 1 DC), and unlock cars using the same criteria as the original game.",
    "Cars keep their original ratings.",
    "Get 14 random tracks, with 4 Easy, 4 Medium, 3 Hard and 3 Extreme.",
    "Cups use the same rules as original cups, with the randomized track list."
  ],
  configure: {
    carOptions: {
      ...DEFAULT_CAR_OPTIONS,
      unlockMode: "baseGame"
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
          sourceRating: "0"
        },
        {
          attrObtain: "1",
          attrRating: "2",
          id: "bossvolt",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "2"
        },
        {
          attrObtain: "2",
          attrRating: "1",
          id: "jg6rc",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "1"
        },
        {
          attrObtain: "2",
          attrRating: "1",
          id: "tc12",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "1"
        },
        {
          attrObtain: "2",
          attrRating: "1",
          id: "tc10",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "1"
        },
        {
          attrObtain: "2",
          attrRating: "2",
          id: "tc8",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "2"
        },
        {
          attrObtain: "2",
          attrRating: "2",
          id: "tc11",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "2"
        },
        {
          attrObtain: "2",
          attrRating: "2",
          id: "tc9",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "2"
        },
        {
          attrObtain: "2",
          attrRating: "3",
          id: "jg1jg7",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "3"
        },
        {
          attrObtain: "2",
          attrRating: "3",
          id: "tc7",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "3"
        },
        {
          attrObtain: "2",
          attrRating: "3",
          id: "jg3loco",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "3"
        },
        {
          attrObtain: "2",
          attrRating: "4",
          id: "jg4snw35",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "4"
        },
        {
          attrObtain: "2",
          attrRating: "4",
          id: "jg5purpxl",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "4"
        },
        {
          attrObtain: "2",
          attrRating: "4",
          id: "jg2fulonx",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "4"
        }
      ],
      stockCars: [
        {
          attrObtain: "0",
          attrRating: "0",
          id: "rc",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "0"
        },
        {
          attrObtain: "0",
          attrRating: "0",
          id: "mite",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "0"
        },
        {
          attrObtain: "0",
          attrRating: "0",
          id: "phat",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "0"
        },
        {
          attrObtain: "0",
          attrRating: "0",
          id: "moss",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "0"
        },
        {
          attrObtain: "0",
          attrRating: "0",
          id: "mud",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "0"
        },
        {
          attrObtain: "0",
          attrRating: "0",
          id: "beatall",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "0"
        },
        {
          attrObtain: "0",
          attrRating: "0",
          id: "volken",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "0"
        },
        {
          attrObtain: "0",
          attrRating: "0",
          id: "tc6",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "0"
        },
        {
          attrObtain: "1",
          attrRating: "1",
          id: "dino",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "1"
        },
        {
          attrObtain: "1",
          attrRating: "1",
          id: "candy",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "1"
        },
        {
          attrObtain: "3",
          attrRating: "1",
          id: "gencar",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "1"
        },
        {
          attrObtain: "3",
          attrRating: "1",
          id: "tc4",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "1"
        },
        {
          attrObtain: "4",
          attrRating: "1",
          id: "mouse",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "1"
        },
        {
          attrObtain: "1",
          attrRating: "2",
          id: "flag",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "2"
        },
        {
          attrObtain: "1",
          attrRating: "2",
          id: "tc2",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "2"
        },
        {
          attrObtain: "3",
          attrRating: "2",
          id: "r5",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "2"
        },
        {
          attrObtain: "3",
          attrRating: "2",
          id: "tc5",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "2"
        },
        {
          attrObtain: "4",
          attrRating: "2",
          id: "sgt",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "2"
        },
        {
          attrObtain: "1",
          attrRating: "3",
          id: "tc3",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "3"
        },
        {
          attrObtain: "1",
          attrRating: "3",
          id: "adeon",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "3"
        },
        {
          attrObtain: "3",
          attrRating: "3",
          id: "fone",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "3"
        },
        {
          attrObtain: "3",
          attrRating: "3",
          id: "tc1",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "3"
        },
        {
          attrObtain: "4",
          attrRating: "3",
          id: "rotor",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "3"
        },
        {
          attrObtain: "1",
          attrRating: "4",
          id: "cougar",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "4"
        },
        {
          attrObtain: "1",
          attrRating: "4",
          id: "sugo",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "4"
        },
        {
          attrObtain: "3",
          attrRating: "4",
          id: "toyeca",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "4"
        },
        {
          attrObtain: "3",
          attrRating: "4",
          id: "amw",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "4"
        },
        {
          attrObtain: "4",
          attrRating: "4",
          id: "panga",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "4"
        }
      ]
    },
    cupSpecState: {
      ...makeDefaultCupSpecState(),
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
}