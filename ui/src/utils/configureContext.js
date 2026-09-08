import {
  DEFAULT_CAR_OPTIONS,
  DEFAULT_FEATURE_OPTIONS,
  DEFAULT_TRACK_OPTIONS,
} from "./constants.js";
import { normalizeExtraCarRowIds } from "../configure/carOptions/CarOptionsUtils";

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
    maxRaceLength: cupSpecState.maxRaceLength ?? null,
    cups: (cupSpecState.cups || []).map(cup => ({
      ...cup,
      overrideMaxRaceLength: cup.overrideMaxRaceLength ?? false,
      maxRaceLength: cup.maxRaceLength ?? null,
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

export function normalizeAppContext(state) {
  if (!state?.configure) return state;

  return {
    ...state,
    configure: normalizeConfigureContext(state.configure),
  };
}
