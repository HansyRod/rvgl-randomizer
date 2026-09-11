import { DEFAULT_CAR_OPTIONS, DEFAULT_TRACK_OPTIONS } from "../../utils/constants";
import { makeDefaultCupSpec, makeDefaultCupSpecState } from "../cupSpec/CupSpecDefaults";
import { getStockModePresetErrors } from "./presetValidation";

export const CUSTOM_UNLOCKS_PRESET = {
  id: "custom-unlocks",
  label: "Custom Unlocks",
  tags: ["Expert", "New!"],
  description:
    "Put a twist on the original game's progression! Can you find all the hidden ways to unlock more cars and tracks?",
  bullets: [
    "Start with 2 Rookie cars and 2 tracks unlocked.",
    "All other cars and tracks are unlocked via custom unlock methods.",
    "Cup progression is unaffected, but beating a cup doesn't give car or track rewards.",
    "Car rating and cup rules are unchanged."
  ],
  validateSelection: ({ scanResult }) => getStockModePresetErrors(scanResult, CUSTOM_UNLOCKS_PRESET.id),
  configure: {
    carOptions: {
      ...DEFAULT_CAR_OPTIONS,
      unlockMode: "randomUnlock"
    },
    carsSpecState: {
      includeStockCars: true,
      includeDcCars: true,
      dcCars: [
        {
          attrObtain: "7",
          attrRating: "0",
          customUnlock: {
            method: "7",
            mode: "randomTracks",
            randomTrackCount: 1,
          },
          id: "bigvolt",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "0"
        },
        {
          attrObtain: "8",
          attrRating: "0",
          customUnlock: {
            method: "8",
            mode: "randomTracks",
            randomTrackCount: 1,
          },
          id: "bossvolt",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "0"
        },
        {
          attrObtain: "6",
          attrRating: "1",
          customUnlock: {
            method: "6",
            mode: "randomTracks",
            randomTrackCount: 2,
          },
          id: "jg6rc",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "1"
        },
        {
          attrObtain: "7",
          attrRating: "1",
          customUnlock: {
            method: "7",
            mode: "randomTracks",
            randomTrackCount: 2,
          },
          id: "tc12",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "1"
        },
        {
          attrObtain: "8",
          attrRating: "1",
          customUnlock: {
            method: "8",
            mode: "randomTracks",
            randomTrackCount: 2,
          },
          id: "tc10",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "1"
        },
        {
          attrObtain: "6",
          attrRating: "2",
          customUnlock: {
            method: "6",
            mode: "randomTracks",
            randomTrackCount: 3,
          },
          id: "tc8",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "2"
        },
        {
          attrObtain: "7",
          attrRating: "2",
          customUnlock: {
            method: "7",
            mode: "randomTracks",
            randomTrackCount: 3,
          },
          id: "tc11",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "2"
        },
        {
          attrObtain: "8",
          attrRating: "2",
          customUnlock: {
            method: "8",
            mode: "randomTracks",
            randomTrackCount: 3,
          },
          id: "tc9",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "2"
        },
        {
          attrObtain: "6",
          attrRating: "3",
          customUnlock: {
            method: "6",
            mode: "randomTracks",
            randomTrackCount: 5,
          },
          id: "jg1jg7",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "3"
        },
        {
          attrObtain: "7",
          attrRating: "3",
          customUnlock: {
            method: "7",
            mode: "randomTracks",
            randomTrackCount: 5,
          },
          id: "tc7",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "3"
        },
        {
          attrObtain: "8",
          attrRating: "3",
          customUnlock: {
            method: "8",
            mode: "randomTracks",
            randomTrackCount: 5,
          },
          id: "jg3loco",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "3"
        },
        {
          attrObtain: "6",
          attrRating: "4",
          customUnlock: {
            method: "6",
            mode: "randomTracks",
            randomTrackCount: 7,
          },
          id: "jg4snw35",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "4"
        },
        {
          attrObtain: "7",
          attrRating: "4",
          customUnlock: {
            method: "7",
            mode: "randomTracks",
            randomTrackCount: 7,
          },
          id: "jg5purpxl",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "4"
        },
        {
          attrObtain: "8",
          attrRating: "4",
          customUnlock: {
            method: "8",
            mode: "randomTracks",
            randomTrackCount: 7,
          },
          id: "jg2fulonx",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "4"
        }
      ],
      extraCars: [],
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
          attrObtain: "9",
          attrRating: "0",
          customUnlock: {
            method: "9",
            requiredCount: 2,
          },
          id: "phat",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "0"
        },
        {
          attrObtain: "10",
          attrRating: "0",
          customUnlock: {
            method: "10",
            requiredCount: 2,
          },
          id: "moss",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "0"
        },
        {
          attrObtain: "11",
          attrRating: "0",
          customUnlock: {
            method: "11",
            requiredCount: 2,
          },
          id: "mud",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "0"
        },
        {
          attrObtain: "9",
          attrRating: "1",
          customUnlock: {
            method: "9",
            requiredCount: 4,
          },
          id: "beatall",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "1"
        },
        {
          attrObtain: "10",
          attrRating: "1",
          customUnlock: {
            method: "10",
            requiredCount: 4,
          },
          id: "volken",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "1"
        },
        {
          attrObtain: "11",
          attrRating: "1",
          customUnlock: {
            method: "11",
            requiredCount: 4,
          },
          id: "tc6",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "1"
        },
        {
          attrObtain: "12",
          attrRating: "1",
          customUnlock: {
            method: "12",
            requiredCount: 5,
          },
          id: "dino",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "1"
        },
        {
          attrObtain: "9",
          attrRating: "1",
          customUnlock: {
            method: "9",
            requiredCount: 6,
          },
          id: "candy",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "1"
        },
        {
          attrObtain: "10",
          attrRating: "1",
          customUnlock: {
            method: "10",
            requiredCount: 6,
          },
          id: "gencar",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "1"
        },
        {
          attrObtain: "11",
          attrRating: "1",
          customUnlock: {
            method: "11",
            requiredCount: 6,
          },
          id: "tc4",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "1"
        },
        {
          attrObtain: "9",
          attrRating: "2",
          customUnlock: {
            method: "9",
            requiredCount: 8,
          },
          id: "mouse",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "2"
        },
        {
          attrObtain: "10",
          attrRating: "2",
          customUnlock: {
            method: "10",
            requiredCount: 8,
          },
          id: "flag",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "2"
        },
        {
          attrObtain: "11",
          attrRating: "2",
          customUnlock: {
            method: "11",
            requiredCount: 8,
          },
          id: "tc2",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "2"
        },
        {
          attrObtain: "12",
          attrRating: "2",
          customUnlock: {
            method: "12",
            requiredCount: 10,
          },
          id: "r5",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "2"
        },
        {
          attrObtain: "9",
          attrRating: "2",
          customUnlock: {
            method: "9",
            requiredCount: 10,
          },
          id: "tc5",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "2"
        },
        {
          attrObtain: "10",
          attrRating: "2",
          customUnlock: {
            method: "10",
            requiredCount: 10,
          },
          id: "sgt",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "2"
        },
        {
          attrObtain: "11",
          attrRating: "2",
          customUnlock: {
            method: "11",
            requiredCount: 10,
          },
          id: "tc3",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "2"
        },
        {
          attrObtain: "9",
          attrRating: "3",
          customUnlock: {
            method: "9",
            requiredCount: 12,
          },
          id: "adeon",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "3"
        },
        {
          attrObtain: "10",
          attrRating: "3",
          customUnlock: {
            method: "10",
            requiredCount: 12,
          },
          id: "fone",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "3"
        },
        {
          attrObtain: "11",
          attrRating: "3",
          customUnlock: {
            method: "11",
            requiredCount: 12,
          },
          id: "tc1",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "3"
        },
        {
          attrObtain: "12",
          attrRating: "3",
          customUnlock: {
            method: "12",
            requiredCount: 15,
          },
          id: "rotor",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "3"
        },
        {
          attrObtain: "9",
          attrRating: "3",
          customUnlock: {
            method: "9",
            requiredCount: 14,
          },
          id: "cougar",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "3"
        },
        {
          attrObtain: "10",
          attrRating: "3",
          customUnlock: {
            method: "10",
            requiredCount: 14,
          },
          id: "sugo",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "3"
        },
        {
          attrObtain: "11",
          attrRating: "3",
          customUnlock: {
            method: "11",
            requiredCount: 14,
          },
          id: "toyeca",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "3"
        },
        {
          attrObtain: "12",
          attrRating: "4",
          customUnlock: {
            method: "12",
            requiredCount: 20,
          },
          id: "amw",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "4"
        },
        {
          attrObtain: "6",
          attrRating: "0",
          customUnlock: {
            method: "6",
            mode: "randomTracks",
            randomTrackCount: 1,
          },
          id: "panga",
          sourceObtain: "Random",
          sourcePool: "Full Random",
          sourceRating: "0"
        }
      ]
    },
    cupSpecState: {
      ...makeDefaultCupSpecState(),
      cups: [
        {
          ...makeDefaultCupSpec(0),
          overrideMaxRaceLength: true,
          toggleMaxRaceLength: true,
          maxRaceLengthValue: 2500,
        },
        {
          ...makeDefaultCupSpec(1),
          overrideMaxRaceLength: true,
          toggleMaxRaceLength: true,
          maxRaceLengthValue: 3250,
        },
        {
          ...makeDefaultCupSpec(2),
          overrideMaxRaceLength: true,
          toggleMaxRaceLength: true,
          maxRaceLengthValue: 4000,
        },
        {
          ...makeDefaultCupSpec(3),
          overrideMaxRaceLength: true,
          toggleMaxRaceLength: true,
          maxRaceLengthValue: 4750,
        },
      ],
    },
    trackOptions: {
      ...DEFAULT_TRACK_OPTIONS,
      unlockMode: "random"
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
          attrObtain: "9",
          customUnlock: {
            method: "9",
            requiredCount: 2,
          },
          id: "muse2",
          sourceDifficulty: "Random",
          sourcePool: "Full Random"
        },
        {
          attrDifficulty: "1",
          attrObtain: "10",
          customUnlock: {
            method: "10",
            requiredCount: 2,
          },
          id: "garden1",
          sourceDifficulty: "Random",
          sourcePool: "Full Random"
        },
        {
          attrDifficulty: "2",
          attrObtain: "11",
          customUnlock: {
            method: "11",
            requiredCount: 2,
          },
          id: "roof",
          sourceDifficulty: "Random",
          sourcePool: "Full Random"
        },
        {
          attrDifficulty: "2",
          attrObtain: "12",
          customUnlock: {
            method: "12",
            requiredCount: 6,
          },
          id: "toylite",
          sourceDifficulty: "Random",
          sourcePool: "Full Random"
        },
        {
          attrDifficulty: "2",
          attrObtain: "9",
          customUnlock: {
            method: "9",
            requiredCount: 5,
          },
          id: "wild_west1",
          sourceDifficulty: "Random",
          sourcePool: "Full Random"
        },
        {
          attrDifficulty: "2",
          attrObtain: "10",
          customUnlock: {
            method: "10",
            requiredCount: 5,
          },
          id: "toy2",
          sourceDifficulty: "Random",
          sourcePool: "Full Random"
        },
        {
          attrDifficulty: "3",
          attrObtain: "11",
          customUnlock: {
            method: "11",
            requiredCount: 5,
          },
          id: "nhood2",
          sourceDifficulty: "Random",
          sourcePool: "Full Random"
        },
        {
          attrDifficulty: "3",
          attrObtain: "12",
          customUnlock: {
            method: "12",
            requiredCount: 12,
          },
          id: "ship1",
          sourceDifficulty: "Random",
          sourcePool: "Full Random"
        },
        {
          attrDifficulty: "3",
          attrObtain: "9",
          customUnlock: {
            method: "9",
            requiredCount: 10,
          },
          id: "muse1",
          sourceDifficulty: "Random",
          sourcePool: "Full Random"
        },
        {
          attrDifficulty: "4",
          attrObtain: "10",
          customUnlock: {
            method: "10",
            requiredCount: 10,
          },
          id: "market1",
          sourceDifficulty: "Random",
          sourcePool: "Full Random"
        },
        {
          attrDifficulty: "4",
          attrObtain: "11",
          customUnlock: {
            method: "11",
            requiredCount: 10,
          },
          id: "wild_west2",
          sourceDifficulty: "Random",
          sourcePool: "Full Random"
        },
        {
          attrDifficulty: "4",
          attrObtain: "12",
          customUnlock: {
            method: "12",
            requiredCount: 18,
          },
          id: "ship2",
          sourceDifficulty: "Random",
          sourcePool: "Full Random"
        }
      ]
    }
  },
}
