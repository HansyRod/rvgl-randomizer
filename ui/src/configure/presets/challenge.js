import { DEFAULT_CAR_OPTIONS, DEFAULT_TRACK_OPTIONS } from "../../utils/constants";
import { makeDefaultCupSpecState, makeDefaultCupSpec } from "../cupSpec/CupSpecTab";
import { countEligibleCarsByRating, getStockModePresetErrors } from "./presetValidation";

const REQUIRED_SUPER_PROS = 14;

export const CHALLENGE_PRESET = {
  id: "challenge",
  label: "Challenge Mode",
  tag: "Expert",
  description:
    "Race against cars above your league! Start with 3 rookies and race in cups that are progressively more challenging. Requires a community content pack with Super Pro cars.",
  bullets: [
    "Start with 3 rookie cars, and unlock 4 cars in each rating by winning cups, races, time trials and practice stars.",
    "Race against Advanced cars in Bronze Cup, Semi-Pro cars in Silver Cup and Pro cars in Gold Cup.",
    "In each cup you'll have more and tougher opponents, and fewer retries.",
    "Platinum Cup is a special challenge against Super Pro cars. Can you withstand it?"
  ],
  validateSelection: ({ scanResult }) => {
    const errors = getStockModePresetErrors(scanResult, CHALLENGE_PRESET.id);
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
      numStartingCars: 3,
      enableStartingCarsRating: true,
      startingCarsRating: "0",
      includeChampionship: true,
      includeCheatOnly: true,
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
          attrObtain: "-1",
          attrRating: "Unchanged",
          id: "bigvolt",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "5"
        },
        {
          attrObtain: "-1",
          attrRating: "Unchanged",
          id: "bossvolt",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "5"
        },
        {
          attrObtain: "-1",
          attrRating: "Unchanged",
          id: "jg6rc",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "5"
        },
        {
          attrObtain: "-1",
          attrRating: "Unchanged",
          id: "tc12",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "5"
        },
        {
          attrObtain: "-1",
          attrRating: "Unchanged",
          id: "tc10",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "5"
        },
        {
          attrObtain: "-1",
          attrRating: "Unchanged",
          id: "tc8",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "5"
        },
        {
          attrObtain: "-1",
          attrRating: "Unchanged",
          id: "tc11",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "5"
        },
        {
          attrObtain: "-1",
          attrRating: "Unchanged",
          id: "tc9",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "5"
        },
        {
          attrObtain: "-1",
          attrRating: "Unchanged",
          id: "jg1jg7",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "5"
        },
        {
          attrObtain: "-1",
          attrRating: "Unchanged",
          id: "tc7",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "5"
        },
        {
          attrObtain: "-1",
          attrRating: "Unchanged",
          id: "jg3loco",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "5"
        },
        {
          attrObtain: "-1",
          attrRating: "Unchanged",
          id: "jg4snw35",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "5"
        },
        {
          attrObtain: "-1",
          attrRating: "Unchanged",
          id: "jg5purpxl",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "5"
        },
        {
          attrObtain: "-1",
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
          attrObtain: "5",
          attrRating: "Unchanged",
          id: "moss",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "0"
        },
        {
          attrObtain: "1",
          attrRating: "Unchanged",
          id: "mud",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "1"
        },
        {
          attrObtain: "2",
          attrRating: "Unchanged",
          id: "beatall",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "1"
        },
        {
          attrObtain: "3",
          attrRating: "Unchanged",
          id: "volken",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "1"
        },
        {
          attrObtain: "4",
          attrRating: "Unchanged",
          id: "tc6",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "1"
        },
        {
          attrObtain: "1",
          attrRating: "Unchanged",
          id: "dino",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "2"
        },
        {
          attrObtain: "2",
          attrRating: "Unchanged",
          id: "candy",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "2"
        },
        {
          attrObtain: "3",
          attrRating: "Unchanged",
          id: "gencar",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "2"
        },
        {
          attrObtain: "4",
          attrRating: "Unchanged",
          id: "tc4",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "2"
        },
        {
          attrObtain: "1",
          attrRating: "Unchanged",
          id: "mouse",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "3"
        },
        {
          attrObtain: "2",
          attrRating: "Unchanged",
          id: "flag",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "3"
        },
        {
          attrObtain: "3",
          attrRating: "Unchanged",
          id: "tc2",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "3"
        },
        {
          attrObtain: "4",
          attrRating: "Unchanged",
          id: "r5",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "3"
        },
        {
          attrObtain: "1",
          attrRating: "Unchanged",
          id: "tc5",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "4"
        },
        {
          attrObtain: "2",
          attrRating: "Unchanged",
          id: "sgt",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "4"
        },
        {
          attrObtain: "3",
          attrRating: "Unchanged",
          id: "tc3",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "4"
        },
        {
          attrObtain: "4",
          attrRating: "Unchanged",
          id: "adeon",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "4"
        },
        {
          attrObtain: "-1",
          attrRating: "Unchanged",
          id: "fone",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "2"
        },
        {
          attrObtain: "-1",
          attrRating: "Unchanged",
          id: "tc1",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "2"
        },
        {
          attrObtain: "-1",
          attrRating: "Unchanged",
          id: "rotor",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "3"
        },
        {
          attrObtain: "-1",
          attrRating: "Unchanged",
          id: "cougar",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "3"
        },
        {
          attrObtain: "-1",
          attrRating: "Unchanged",
          id: "sugo",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "4"
        },
        {
          attrObtain: "-1",
          attrRating: "Unchanged",
          id: "toyeca",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "4"
        },
        {
          attrObtain: "-1",
          attrRating: "Unchanged",
          id: "amw",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "4"
        },
        {
          attrObtain: "-1",
          attrRating: "Unchanged",
          id: "panga",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "4"
        }
      ]
    },
    cupSpecState: {
      ...makeDefaultCupSpecState(),
      stageMode: "random",
      allowMirror: true,
      allowReverse: true,
      allowReverseMirror: true,
      guaranteeFirstNormal: true,
      sameTrackHandling: "forbid",
      numStagesMin: 4,
      numStagesMax: 4,
      numLapsMin: 3,
      numLapsMax: 6,
      cups: [
        {
          ...makeDefaultCupSpec(0),
          carsPerClass: [0, 1, 5, 1, 0, 0],
          overrideCarsPerClass: true
        },
        {
          ...makeDefaultCupSpec(1),
          carsPerClass: [0, 0, 1, 5, 3, 0],
          numCars: 10,
          numTries: 2,
          pointsTable: [10, 8, 6, 5, 4, 3, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0],
          overrideCarsPerClass: true,
          overrideNumCars: true,
          overrideNumTries: true,
          overridePointsTable: true
        },
        {
          ...makeDefaultCupSpec(2),
          carsPerClass: [0, 0, 2, 7, 2, 0],
          numCars: 12,
          numTries: 1,
          pointsTable: [15, 12, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0, 0, 0, 0],
          overrideCarsPerClass: true,
          overrideNumCars: true,
          overrideNumTries: true,
          overridePointsTable: true
        },
        {
          ...makeDefaultCupSpec(3),
          carsPerClass: [0, 0, 0, 0, 1, 14],
          numCars: 16,
          numTries: 0,
          pointsTable: [10, 6, 4, 9, 9, 8, 8, 7, 7, 6, 6, 5, 5, 4, 4, 4],
          overrideCarsPerClass: true,
          overrideNumCars: true,
          overrideNumTries: true,
          overridePointsTable: true,
          overrideStageMode: true,
          stageMode: "userDefined",
          stages: [
            {
              "isMirror": true,
              "isReverse": true,
              "numLaps": null,
              "numLapsMax": 10,
              "numLapsMin": 5,
              sourcePool: "Random"
            },
            {
              "isMirror": true,
              "isReverse": true,
              "numLaps": null,
              "numLapsMax": 10,
              "numLapsMin": 5,
              sourcePool: "Random"
            },
            {
              "isMirror": true,
              "isReverse": true,
              "numLaps": null,
              "numLapsMax": 10,
              "numLapsMin": 5,
              sourcePool: "Random"
            },
            {
              "isMirror": true,
              "isReverse": true,
              "numLaps": null,
              "numLapsMax": 10,
              "numLapsMin": 5,
              sourcePool: "Random"
            },
            {
              "isMirror": true,
              "isReverse": true,
              "numLaps": null,
              "numLapsMax": 10,
              "numLapsMin": 5,
              sourcePool: "Random"
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