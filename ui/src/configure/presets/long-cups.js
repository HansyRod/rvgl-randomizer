import { DEFAULT_CAR_OPTIONS, DEFAULT_TRACK_OPTIONS } from "../../utils/constants";
import { makeDefaultCupSpecState, makeDefaultCupSpec } from "../cupSpec/CupSpecTab";
import { countEligibleCarsByRating, getStockModePresetErrors } from "./presetValidation";

const REQUIRED_SUPER_PROS = 7;

export const LONG_CUPS_PRESET = {
  id: "long-cups",
  label: "Long Cups",
  tag: "Expert",
  description:
    "Race all 14 tracks in each cup in a marathon against 15 AI opponents. Requires a community content pack with Super Pro cars.",
  bullets: [
    "Start with 7 Rookie cars. There are 7 cars of each rating to unlock, with random unlock criteria.",
    "Each cup has 15 AI opponents spread by 3 different ratings, with 5 cars from each rating.",
    "Bronze Cup has 5 Rookies, 5 Amateurs and 5 Advanced; Platinum cup has 5 Semi-Pros, 5 Pros and 5 Super Pros.",
    "Each cup has 14 stages, one in each track. Bronze Cup races in Normal version, Silver Cup in Reverse, Gold Cup in Mirror, and Platinum Cup in Reverse Mirror.",
    "Progression is free between stages, but you need to finish 1st overall to progress to the next cup."
  ],
  validateSelection: ({ scanResult }) => {
    const errors = getStockModePresetErrors(scanResult, LONG_CUPS_PRESET.id);
    const superProCount = countEligibleCarsByRating(scanResult, 5);

    if (superProCount < REQUIRED_SUPER_PROS) {
      errors.push(
        `This preset requires at least ${REQUIRED_SUPER_PROS} eligible Super Pro cars, but only ${superProCount} are currently available.`
      );
    }

    return errors;
  },
  configure: {
    carOptions: {
      ...DEFAULT_CAR_OPTIONS,
      unlockMode: "randomUnlock",
      enableStartingCars: true,
      numStartingCars: 7,
      enableStartingCarsRating: true,
      startingCarsRating: "0",
      includeChampionship: true,
      includeCheatOnly: false,
      includePracticeStars: true,
      includeSingleRace: true,
      includeStartingCar: false,
      includeStuntArena: true,
      includeSuperPro: true,
      includeTimeTrial: true,
    },
    carsSpecState: {
      includeStockCars: true,
      includeDcCars: true,
      dcCars: [
        {
          attrObtain: "Random",
          attrRating: "Unchanged",
          id: "bigvolt",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "4"
        },
        {
          attrObtain: "Random",
          attrRating: "Unchanged",
          id: "bossvolt",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "4"
        },
        {
          attrObtain: "Random",
          attrRating: "Unchanged",
          id: "jg6rc",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "4"
        },
        {
          attrObtain: "Random",
          attrRating: "Unchanged",
          id: "tc12",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "4"
        },
        {
          attrObtain: "Random",
          attrRating: "Unchanged",
          id: "tc10",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "4"
        },
        {
          attrObtain: "Random",
          attrRating: "Unchanged",
          id: "tc8",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "4"
        },
        {
          attrObtain: "Random",
          attrRating: "Unchanged",
          id: "tc11",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "4"
        },
        {
          attrObtain: "1",
          attrRating: "Unchanged",
          id: "tc9",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "5"
        },
        {
          attrObtain: "1",
          attrRating: "Unchanged",
          id: "jg1jg7",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "5"
        },
        {
          attrObtain: "1",
          attrRating: "Unchanged",
          id: "tc7",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "5"
        },
        {
          attrObtain: "1",
          attrRating: "Unchanged",
          id: "jg3loco",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "5"
        },
        {
          attrObtain: "1",
          attrRating: "Unchanged",
          id: "jg4snw35",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "5"
        },
        {
          attrObtain: "1",
          attrRating: "Unchanged",
          id: "jg5purpxl",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "5"
        },
        {
          attrObtain: "1",
          attrRating: "Unchanged",
          id: "jg2fulonx",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "5"
        }
      ],
      stockCars: [
        {
          attrObtain: "0",
          attrRating: "Unchanged",
          id: "rc",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "0"
        },
        {
          attrObtain: "0",
          attrRating: "Unchanged",
          id: "mite",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "0"
        },
        {
          attrObtain: "0",
          attrRating: "Unchanged",
          id: "phat",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "0"
        },
        {
          attrObtain: "0",
          attrRating: "Unchanged",
          id: "moss",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "0"
        },
        {
          attrObtain: "0",
          attrRating: "Unchanged",
          id: "mud",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "0"
        },
        {
          attrObtain: "0",
          attrRating: "Unchanged",
          id: "beatall",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "0"
        },
        {
          attrObtain: "0",
          attrRating: "Unchanged",
          id: "volken",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "0"
        },
        {
          attrObtain: "Random",
          attrRating: "Unchanged",
          id: "tc6",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "1"
        },
        {
          attrObtain: "Random",
          attrRating: "Unchanged",
          id: "dino",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "1"
        },
        {
          attrObtain: "Random",
          attrRating: "Unchanged",
          id: "candy",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "1"
        },
        {
          attrObtain: "Random",
          attrRating: "Unchanged",
          id: "gencar",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "1"
        },
        {
          attrObtain: "Random",
          attrRating: "Unchanged",
          id: "tc4",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "1"
        },
        {
          attrObtain: "Random",
          attrRating: "Unchanged",
          id: "mouse",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "1"
        },
        {
          attrObtain: "Random",
          attrRating: "Unchanged",
          id: "flag",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "1"
        },
        {
          attrObtain: "Random",
          attrRating: "Unchanged",
          id: "tc2",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "2"
        },
        {
          attrObtain: "Random",
          attrRating: "Unchanged",
          id: "r5",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "2"
        },
        {
          attrObtain: "Random",
          attrRating: "Unchanged",
          id: "tc5",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "2"
        },
        {
          attrObtain: "Random",
          attrRating: "Unchanged",
          id: "sgt",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "2"
        },
        {
          attrObtain: "Random",
          attrRating: "Unchanged",
          id: "tc3",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "2"
        },
        {
          attrObtain: "Random",
          attrRating: "Unchanged",
          id: "adeon",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "2"
        },
        {
          attrObtain: "Random",
          attrRating: "Unchanged",
          id: "fone",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "2"
        },
        {
          attrObtain: "Random",
          attrRating: "Unchanged",
          id: "tc1",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "3"
        },
        {
          attrObtain: "Random",
          attrRating: "Unchanged",
          id: "rotor",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "3"
        },
        {
          attrObtain: "Random",
          attrRating: "Unchanged",
          id: "cougar",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "3"
        },
        {
          attrObtain: "Random",
          attrRating: "Unchanged",
          id: "sugo",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "3"
        },
        {
          attrObtain: "Random",
          attrRating: "Unchanged",
          id: "toyeca",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "3"
        },
        {
          attrObtain: "Random",
          attrRating: "Unchanged",
          id: "amw",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "3"
        },
        {
          attrObtain: "Random",
          attrRating: "Unchanged",
          id: "panga",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "3"
        }
      ]
    },
    cupSpecState: {
      ...makeDefaultCupSpecState(),
      stageMode: "userDefined",
      numCars: 16,
      numTries: 1,
      perRaceRequiredPlace: 16,
      pointsTable: [25, 20, 16, 13, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0],
      cups: [
        {
          ...makeDefaultCupSpec(0),
          carsPerClass: [5, 5, 5, 0, 0, 0],
          overrideCarsPerClass: true,
          stages: [
            {
              isMirror: false,
              isReverse: false,
              numLaps: null,
              numLapsMax: 6,
              numLapsMin: 3,
              sourcePool: "slot:0"
            },
            {
              isMirror: false,
              isReverse: false,
              numLaps: null,
              numLapsMax: 6,
              numLapsMin: 3,
              sourcePool: "slot:1"
            },
            {
              isMirror: false,
              isReverse: false,
              numLaps: null,
              numLapsMax: 6,
              numLapsMin: 3,
              sourcePool: "slot:2"
            },
            {
              isMirror: false,
              isReverse: false,
              numLaps: null,
              numLapsMax: 6,
              numLapsMin: 3,
              sourcePool: "slot:3"
            },
            {
              isMirror: false,
              isReverse: false,
              numLaps: null,
              numLapsMax: 6,
              numLapsMin: 3,
              sourcePool: "slot:4"
            },
            {
              isMirror: false,
              isReverse: false,
              numLaps: null,
              numLapsMax: 6,
              numLapsMin: 3,
              sourcePool: "slot:5"
            },
            {
              isMirror: false,
              isReverse: false,
              numLaps: null,
              numLapsMax: 6,
              numLapsMin: 3,
              sourcePool: "slot:6"
            },
            {
              isMirror: false,
              isReverse: false,
              numLaps: null,
              numLapsMax: 6,
              numLapsMin: 3,
              sourcePool: "slot:7"
            },
            {
              isMirror: false,
              isReverse: false,
              numLaps: null,
              numLapsMax: 6,
              numLapsMin: 3,
              sourcePool: "slot:8"
            },
            {
              isMirror: false,
              isReverse: false,
              numLaps: null,
              numLapsMax: 6,
              numLapsMin: 3,
              sourcePool: "slot:9"
            },
            {
              isMirror: false,
              isReverse: false,
              numLaps: null,
              numLapsMax: 6,
              numLapsMin: 3,
              sourcePool: "slot:10"
            },
            {
              isMirror: false,
              isReverse: false,
              numLaps: null,
              numLapsMax: 6,
              numLapsMin: 3,
              sourcePool: "slot:11"
            },
            {
              isMirror: false,
              isReverse: false,
              numLaps: null,
              numLapsMax: 6,
              numLapsMin: 3,
              sourcePool: "slot:12"
            },
            {
              isMirror: false,
              isReverse: false,
              numLaps: null,
              numLapsMax: 6,
              numLapsMin: 3,
              sourcePool: "slot:13"
            }
          ]
        },
        {
          ...makeDefaultCupSpec(1),
          carsPerClass: [0, 5, 5, 5, 0, 0],
          overrideCarsPerClass: true,
          stages: [
            {
              isMirror: false,
              isReverse: true,
              numLaps: null,
              numLapsMax: 6,
              numLapsMin: 3,
              sourcePool: "slot:0"
            },
            {
              isMirror: false,
              isReverse: true,
              numLaps: null,
              numLapsMax: 6,
              numLapsMin: 3,
              sourcePool: "slot:1"
            },
            {
              isMirror: false,
              isReverse: true,
              numLaps: null,
              numLapsMax: 6,
              numLapsMin: 3,
              sourcePool: "slot:2"
            },
            {
              isMirror: false,
              isReverse: true,
              numLaps: null,
              numLapsMax: 6,
              numLapsMin: 3,
              sourcePool: "slot:3"
            },
            {
              isMirror: false,
              isReverse: true,
              numLaps: null,
              numLapsMax: 6,
              numLapsMin: 3,
              sourcePool: "slot:4"
            },
            {
              isMirror: false,
              isReverse: true,
              numLaps: null,
              numLapsMax: 6,
              numLapsMin: 3,
              sourcePool: "slot:5"
            },
            {
              isMirror: false,
              isReverse: true,
              numLaps: null,
              numLapsMax: 6,
              numLapsMin: 3,
              sourcePool: "slot:6"
            },
            {
              isMirror: false,
              isReverse: true,
              numLaps: null,
              numLapsMax: 6,
              numLapsMin: 3,
              sourcePool: "slot:7"
            },
            {
              isMirror: false,
              isReverse: true,
              numLaps: null,
              numLapsMax: 6,
              numLapsMin: 3,
              sourcePool: "slot:8"
            },
            {
              isMirror: false,
              isReverse: true,
              numLaps: null,
              numLapsMax: 6,
              numLapsMin: 3,
              sourcePool: "slot:9"
            },
            {
              isMirror: false,
              isReverse: true,
              numLaps: null,
              numLapsMax: 6,
              numLapsMin: 3,
              sourcePool: "slot:10"
            },
            {
              isMirror: false,
              isReverse: true,
              numLaps: null,
              numLapsMax: 6,
              numLapsMin: 3,
              sourcePool: "slot:11"
            },
            {
              isMirror: false,
              isReverse: true,
              numLaps: null,
              numLapsMax: 6,
              numLapsMin: 3,
              sourcePool: "slot:12"
            },
            {
              isMirror: false,
              isReverse: true,
              numLaps: null,
              numLapsMax: 6,
              numLapsMin: 3,
              sourcePool: "slot:13"
            }
          ]
        },
        {
          ...makeDefaultCupSpec(2),
          carsPerClass: [0, 0, 5, 5, 5, 0],
          overrideCarsPerClass: true,
          stages: [
            {
              isMirror: true,
              isReverse: false,
              numLaps: null,
              numLapsMax: 6,
              numLapsMin: 3,
              sourcePool: "slot:0"
            },
            {
              isMirror: true,
              isReverse: false,
              numLaps: null,
              numLapsMax: 6,
              numLapsMin: 3,
              sourcePool: "slot:1"
            },
            {
              isMirror: true,
              isReverse: false,
              numLaps: null,
              numLapsMax: 6,
              numLapsMin: 3,
              sourcePool: "slot:2"
            },
            {
              isMirror: true,
              isReverse: false,
              numLaps: null,
              numLapsMax: 6,
              numLapsMin: 3,
              sourcePool: "slot:3"
            },
            {
              isMirror: true,
              isReverse: false,
              numLaps: null,
              numLapsMax: 6,
              numLapsMin: 3,
              sourcePool: "slot:4"
            },
            {
              isMirror: true,
              isReverse: false,
              numLaps: null,
              numLapsMax: 6,
              numLapsMin: 3,
              sourcePool: "slot:5"
            },
            {
              isMirror: true,
              isReverse: false,
              numLaps: null,
              numLapsMax: 6,
              numLapsMin: 3,
              sourcePool: "slot:6"
            },
            {
              isMirror: true,
              isReverse: false,
              numLaps: null,
              numLapsMax: 6,
              numLapsMin: 3,
              sourcePool: "slot:7"
            },
            {
              isMirror: true,
              isReverse: false,
              numLaps: null,
              numLapsMax: 6,
              numLapsMin: 3,
              sourcePool: "slot:8"
            },
            {
              isMirror: true,
              isReverse: false,
              numLaps: null,
              numLapsMax: 6,
              numLapsMin: 3,
              sourcePool: "slot:9"
            },
            {
              isMirror: true,
              isReverse: false,
              numLaps: null,
              numLapsMax: 6,
              numLapsMin: 3,
              sourcePool: "slot:10"
            },
            {
              isMirror: true,
              isReverse: false,
              numLaps: null,
              numLapsMax: 6,
              numLapsMin: 3,
              sourcePool: "slot:11"
            },
            {
              isMirror: true,
              isReverse: false,
              numLaps: null,
              numLapsMax: 6,
              numLapsMin: 3,
              sourcePool: "slot:12"
            },
            {
              isMirror: true,
              isReverse: false,
              numLaps: null,
              numLapsMax: 6,
              numLapsMin: 3,
              sourcePool: "slot:13"
            }
          ]
        },
        {
          ...makeDefaultCupSpec(3),
          carsPerClass: [0, 0, 0, 5, 5, 5],
          overrideCarsPerClass: true,
          stages: [
            {
              isMirror: true,
              isReverse: true,
              numLaps: null,
              numLapsMax: 6,
              numLapsMin: 3,
              sourcePool: "slot:0"
            },
            {
              isMirror: true,
              isReverse: true,
              numLaps: null,
              numLapsMax: 6,
              numLapsMin: 3,
              sourcePool: "slot:1"
            },
            {
              isMirror: true,
              isReverse: true,
              numLaps: null,
              numLapsMax: 6,
              numLapsMin: 3,
              sourcePool: "slot:2"
            },
            {
              isMirror: true,
              isReverse: true,
              numLaps: null,
              numLapsMax: 6,
              numLapsMin: 3,
              sourcePool: "slot:3"
            },
            {
              isMirror: true,
              isReverse: true,
              numLaps: null,
              numLapsMax: 6,
              numLapsMin: 3,
              sourcePool: "slot:4"
            },
            {
              isMirror: true,
              isReverse: true,
              numLaps: null,
              numLapsMax: 6,
              numLapsMin: 3,
              sourcePool: "slot:5"
            },
            {
              isMirror: true,
              isReverse: true,
              numLaps: null,
              numLapsMax: 6,
              numLapsMin: 3,
              sourcePool: "slot:6"
            },
            {
              isMirror: true,
              isReverse: true,
              numLaps: null,
              numLapsMax: 6,
              numLapsMin: 3,
              sourcePool: "slot:7"
            },
            {
              isMirror: true,
              isReverse: true,
              numLaps: null,
              numLapsMax: 6,
              numLapsMin: 3,
              sourcePool: "slot:8"
            },
            {
              isMirror: true,
              isReverse: true,
              numLaps: null,
              numLapsMax: 6,
              numLapsMin: 3,
              sourcePool: "slot:9"
            },
            {
              isMirror: true,
              isReverse: true,
              numLaps: null,
              numLapsMax: 6,
              numLapsMin: 3,
              sourcePool: "slot:10"
            },
            {
              isMirror: true,
              isReverse: true,
              numLaps: null,
              numLapsMax: 6,
              numLapsMin: 3,
              sourcePool: "slot:11"
            },
            {
              isMirror: true,
              isReverse: true,
              numLaps: null,
              numLapsMax: 6,
              numLapsMin: 3,
              sourcePool: "slot:12"
            },
            {
              isMirror: true,
              isReverse: true,
              numLaps: null,
              numLapsMax: 6,
              numLapsMin: 3,
              sourcePool: "slot:13"
            }
          ]
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
