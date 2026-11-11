import {
  DEFAULT_CAR_OPTIONS,
  DEFAULT_FEATURE_OPTIONS,
  DEFAULT_TRACK_OPTIONS,
} from "./constants.js";
import { normalizeExtraCarRowIds } from "../configure/carOptions/CarOptionsUtils";
import { DEFAULT_MAX_RACE_LENGTH } from "../configure/cupSpec/CupUtils";
import { getAccessibleStep } from "../navigation/steps";

function normalizeSpecRows(rows) {
  return (rows || []).map(row => ({
    customUnlock: null,
    ...row,
  }));
}

function normalizeCarsSpecState(carsSpecState) {
  if (!carsSpecState) return carsSpecState;

  return {
    ...carsSpecState,
    stockCars: normalizeSpecRows(carsSpecState.stockCars),
    dcCars: normalizeSpecRows(carsSpecState.dcCars),
    extraCars: normalizeExtraCarRowIds(normalizeSpecRows(carsSpecState.extraCars)),
  };
}

function normalizeTrackSpecState(trackSpecState) {
  if (!trackSpecState) return trackSpecState;

  return {
    ...trackSpecState,
    tracks: normalizeSpecRows(trackSpecState.tracks),
    cachedRoofTrackRow: trackSpecState.cachedRoofTrackRow ?? null,
  };
}

function normalizeCupSpecState(cupSpecState) {
  if (!cupSpecState) return cupSpecState;

  return {
    ...cupSpecState,
    toggleMaxRaceLength: cupSpecState.toggleMaxRaceLength ?? false,
    maxRaceLengthValue: cupSpecState.maxRaceLengthValue ?? DEFAULT_MAX_RACE_LENGTH,
    cups: (cupSpecState.cups || []).map(cup => ({
      ...cup,
      overrideMaxRaceLength: cup.overrideMaxRaceLength ?? false,
      toggleMaxRaceLength: cup.toggleMaxRaceLength ?? false,
      maxRaceLengthValue: cup.maxRaceLengthValue ?? DEFAULT_MAX_RACE_LENGTH,
    })),
  };
}

export function normalizeConfigureContext(configure) {
  if (!configure) return configure;

  return {
    ...configure,
    carOptions: {
      ...DEFAULT_CAR_OPTIONS,
      ...(configure.carOptions || {}),
    },
    trackOptions: {
      ...DEFAULT_TRACK_OPTIONS,
      ...(configure.trackOptions || {}),
    },
    featureOptions: {
      ...DEFAULT_FEATURE_OPTIONS,
      ...(configure.featureOptions || {}),
    },
    carsSpecState: normalizeCarsSpecState(configure.carsSpecState),
    trackSpecState: normalizeTrackSpecState(configure.trackSpecState),
    cupSpecState: normalizeCupSpecState(configure.cupSpecState),
  };
}

// Executed after the cache is loaded from file, to ensure that the app state is valid
// and compatible with the current version of the app. 
export function normalizeAppContext(state) {
  if (!state || Object.keys(state).length === 0) {
    return state;
  }

  const isLegacyCache = state.app?.mode === undefined;
  // Work from the previous session is no longer running; startup owns loading.
  const app = { ...state.app, isLoading: true, isScanning: false, isFetchingPack: false };
  // Legacy caches belong to Randomizer; new sessions still start with mode selection.
  if (isLegacyCache) {
    app.mode = "randomizer";
  }
  const defaultStep = isLegacyCache ? "setup" : "mode";
  app.activeStep = getAccessibleStep({ ...state, app }, app.activeStep ?? defaultStep);

  return {
    ...state,
    app,
    ...(state.configure ? { configure: normalizeConfigureContext(state.configure) } : {}),
  };
}
