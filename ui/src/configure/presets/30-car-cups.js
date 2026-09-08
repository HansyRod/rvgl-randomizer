import { DEFAULT_CAR_OPTIONS, DEFAULT_TRACK_OPTIONS } from "../../utils/constants";
import { makeDefaultCupSpecState, makeDefaultCupSpec } from "../cupSpec/CupSpecDefaults";
import { getStockModePresetErrors } from "./presetValidation";

export const THIRTY_CAR_CUPS_PRESET = {
  id: "30-car-cups",
  label: "30-Car Cups",
  tag: "Chaos",
  description:
    "Play against a full field of cars in each cup. With 29 opponents, can you still come out on top?",
  bullets: [
    "Cars and tracks are randomized, but cars keep their original ratings and progression is unchanged.",
    "Cups are expanded to include 30 cars in each cup, and unchanged otherwise.",
    "CPU opponents for each cup are selected proportionally to the original rating allocation in the cup.",
    "Extra cars allow a pool large enough to fill each cup without duplicates, but extras cannot be unlocked."
  ],
  validateSelection: ({ scanResult }) => getStockModePresetErrors(scanResult, THIRTY_CAR_CUPS_PRESET.id),
  configure: {
    featureOptions: {
      enable30CarMode: true
    },
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
      extraCars: [
        {
          attrObtain: "-1",
          attrRating: "0",
          id: "extra-1",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "0"
        },
        {
          attrObtain: "-1",
          attrRating: "0",
          id: "extra-2",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "0"
        },
        {
          attrObtain: "-1",
          attrRating: "0",
          id: "extra-3",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "0"
        },
        {
          attrObtain: "-1",
          attrRating: "0",
          id: "extra-4",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "0"
        },
        {
          attrObtain: "-1",
          attrRating: "0",
          id: "extra-5",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "0"
        },
        {
          attrObtain: "-1",
          attrRating: "0",
          id: "extra-6",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "0"
        },
        {
          attrObtain: "-1",
          attrRating: "0",
          id: "extra-7",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "0"
        },
        {
          attrObtain: "-1",
          attrRating: "0",
          id: "extra-8",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "0"
        },
        {
          attrObtain: "-1",
          attrRating: "0",
          id: "extra-9",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "0"
        },
        {
          attrObtain: "-1",
          attrRating: "0",
          id: "extra-10",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "0"
        },
        {
          attrObtain: "-1",
          attrRating: "0",
          id: "extra-11",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "0"
        },
        {
          attrObtain: "-1",
          attrRating: "0",
          id: "extra-12",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "0"
        },
        {
          attrObtain: "-1",
          attrRating: "0",
          id: "extra-13",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "0"
        },
        {
          attrObtain: "-1",
          attrRating: "0",
          id: "extra-14",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "0"
        },
        {
          attrObtain: "-1",
          attrRating: "0",
          id: "extra-15",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "0"
        },
        {
          attrObtain: "-1",
          attrRating: "0",
          id: "extra-16",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "0"
        },
        {
          attrObtain: "-1",
          attrRating: "0",
          id: "extra-17",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "0"
        },
        {
          attrObtain: "-1",
          attrRating: "0",
          id: "extra-18",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "0"
        },
        {
          attrObtain: "-1",
          attrRating: "0",
          id: "extra-19",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "0"
        },
        {
          attrObtain: "-1",
          attrRating: "0",
          id: "extra-20",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "0"
        },
        {
          attrObtain: "-1",
          attrRating: "0",
          id: "extra-21",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "0"
        },
        {
          attrObtain: "-1",
          attrRating: "1",
          id: "extra-22",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "1"
        },
        {
          attrObtain: "-1",
          attrRating: "1",
          id: "extra-23",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "1"
        },
        {
          attrObtain: "-1",
          attrRating: "1",
          id: "extra-24",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "1"
        },
        {
          attrObtain: "-1",
          attrRating: "1",
          id: "extra-25",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "1"
        },
        {
          attrObtain: "-1",
          attrRating: "1",
          id: "extra-26",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "1"
        },
        {
          attrObtain: "-1",
          attrRating: "1",
          id: "extra-27",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "1"
        },
        {
          attrObtain: "-1",
          attrRating: "1",
          id: "extra-28",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "1"
        },
        {
          attrObtain: "-1",
          attrRating: "1",
          id: "extra-29",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "1"
        },
        {
          attrObtain: "-1",
          attrRating: "1",
          id: "extra-30",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "1"
        },
        {
          attrObtain: "-1",
          attrRating: "1",
          id: "extra-31",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "1"
        },
        {
          attrObtain: "-1",
          attrRating: "2",
          id: "extra-32",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "2"
        },
        {
          attrObtain: "-1",
          attrRating: "2",
          id: "extra-33",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "2"
        },
        {
          attrObtain: "-1",
          attrRating: "2",
          id: "extra-34",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "2"
        },
        {
          attrObtain: "-1",
          attrRating: "2",
          id: "extra-35",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "2"
        },
        {
          attrObtain: "-1",
          attrRating: "2",
          id: "extra-36",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "2"
        },
        {
          attrObtain: "-1",
          attrRating: "2",
          id: "extra-37",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "2"
        },
        {
          attrObtain: "-1",
          attrRating: "2",
          id: "extra-38",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "2"
        },
        {
          attrObtain: "-1",
          attrRating: "2",
          id: "extra-39",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "2"
        },
        {
          attrObtain: "-1",
          attrRating: "2",
          id: "extra-40",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "2"
        },
        {
          attrObtain: "-1",
          attrRating: "3",
          id: "extra-41",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "3"
        },
        {
          attrObtain: "-1",
          attrRating: "3",
          id: "extra-42",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "3"
        },
        {
          attrObtain: "-1",
          attrRating: "3",
          id: "extra-43",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "3"
        },
        {
          attrObtain: "-1",
          attrRating: "3",
          id: "extra-44",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "3"
        },
        {
          attrObtain: "-1",
          attrRating: "3",
          id: "extra-45",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "3"
        },
        {
          attrObtain: "-1",
          attrRating: "4",
          id: "extra-46",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "4"
        },
        {
          attrObtain: "-1",
          attrRating: "4",
          id: "extra-47",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "4"
        },
        {
          attrObtain: "-1",
          attrRating: "4",
          id: "extra-48",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "4"
        },
        {
          attrObtain: "-1",
          attrRating: "4",
          id: "extra-49",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "4"
        },
        {
          attrObtain: "-1",
          attrRating: "4",
          id: "extra-50",
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
      numCars: 30,
      pointsTable: [
        100, 90, 85, 80, 75, 70,
        65, 60, 55, 50, 46, 42,
        38, 34, 30, 27, 24, 21,
        18, 15, 12, 9, 8, 7,
        6, 5, 4, 3, 2, 1
      ],
      cups: [
        {
          ...makeDefaultCupSpec(0),
          carsPerClass: [29, 0, 0, 0, 0, 0],
          overrideCarsPerClass: true
        },
        {
          ...makeDefaultCupSpec(1),
          carsPerClass: [0, 17, 12, 0, 0, 0],
          overrideCarsPerClass: true,
        },
        {
          ...makeDefaultCupSpec(2),
          carsPerClass: [0, 0, 17, 12, 0, 0],
          overrideCarsPerClass: true,
        },
        {
          ...makeDefaultCupSpec(3),
          carsPerClass: [0, 0, 5, 12, 12, 0],
          overrideCarsPerClass: true,
        }
      ]

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
