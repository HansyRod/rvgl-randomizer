import { DEFAULT_CAR_OPTIONS, DEFAULT_TRACK_OPTIONS } from "./constants.js";

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
    carsSpecState: normalizeCarsSpecState(configure.carsSpecState),
    trackSpecState: normalizeTrackSpecState(configure.trackSpecState),
  };
}

export function normalizeAppContext(state) {
  if (!state?.configure) return state;

  return {
    ...state,
    configure: normalizeConfigureContext(state.configure),
  };
}
