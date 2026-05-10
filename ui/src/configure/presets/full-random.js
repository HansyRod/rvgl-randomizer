import { DEFAULT_CAR_OPTIONS, STOCK_CARS, DC_CARS, STOCK_TRACKS, makeDefaultCarsSpec, makeDefaultTrackSpec } from "../../utils/constants";
import { makeDefaultCupSpecState } from "../cupSpec/CupSpecTab";
import { getStockModePresetErrors } from "./presetValidation";

export const FULL_RANDOM_PRESET = {
  id: "full-random",
  label: "Full Random",
  tag: "Chaos",
  description:
    "Anything is possible! Play with no restrictions on cars, tracks and cups.",
  bullets: [
    "Cars are fully randomized and don't have to keep their original ratings.",
    "Cars and tracks can be unlocked in any way.",
    "Cups have a random number of stages, and each stage has a random number of laps.",
    "You can play reverse / mirror tracks on cups with no restrictions."
  ],
  validateSelection: ({ scanResult }) => getStockModePresetErrors(scanResult),
  configure: {
    carOptions: {
      ...DEFAULT_CAR_OPTIONS,
      unlockMode: "random",
      includeChampionship: true,
      includeCheatOnly: true,
      includePracticeStars: true,
      includeSingleRace: true,
      includeStartingCar: true,
      includeStuntArena: true,
      includeSuperPro: true,
      includeTimeTrial: true,
    },
    carsSpecState: {
      includeStockCars: true,
      includeDcCars: true,
      stockCars: makeDefaultCarsSpec(STOCK_CARS),
      dcCars: makeDefaultCarsSpec(DC_CARS)
    },
    cupSpecState: {
      ...makeDefaultCupSpecState(),
      allowMirror: true,
      allowReverse: true,
      allowReverseMirror: true,
      guaranteeFirstNormal: false,
      numLapsMin: 1,
      numLapsMax: 20,
      numStagesMin: 1,
      numStagesMax: 16,
      sameTrackHandling: "allowAny",
      stageMode: "random"
    },
    trackOptions: {
      unlockMode: "random",
      includeStuntArena: true,
      includeDefault: true,
      includeTimeTrial: true,
      includePractice: true,
      includeSingleRace: true,
    },
    trackSpecState: {
      includeTracks: true,
      tracks: makeDefaultTrackSpec(STOCK_TRACKS)
    }
  },
};