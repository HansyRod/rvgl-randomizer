import { createContext, useContext, useState } from "react";
import {
  DEFAULT_CAR_OPTIONS,
  DEFAULT_FEATURE_OPTIONS,
  DEFAULT_TRACK_OPTIONS,
  STOCK_CARS,
  DC_CARS,
  STOCK_TRACKS,
  makeDefaultCarsSpec,
  makeDefaultTrackSpec,
} from "./utils/constants";
import { makeDefaultCupSpecState } from "./configure/cupSpec/CupSpecTab";
export const DEFAULT_STATE = {
  app: {
    isLoading: true,
    theme: "dark",
    activeStep: "setup", // Wizard step: "setup" | "configure" | "generate" | "play"
    isScanning: false,
    isFetchingPack: false,
  },
  setup: {
    installPath: "",
    scanResult: null,
    installError: "",
    setupTab: "install", // Setup sub-tab: "install" | "cars" | "tracks"
    installHistory: [], // List of { path, installType }
  },
  configure: {
    carOptions: DEFAULT_CAR_OPTIONS,
    trackOptions: DEFAULT_TRACK_OPTIONS,
    featureOptions: DEFAULT_FEATURE_OPTIONS,
    carsSpecState: {
      includeStockCars: true,
      includeDcCars: true,
      stockCars: makeDefaultCarsSpec(STOCK_CARS),
      dcCars: makeDefaultCarsSpec(DC_CARS),
      extraCars: []
    },
    trackSpecState: {
      includeTracks: true,
      tracks: makeDefaultTrackSpec(STOCK_TRACKS),
      cachedRoofTrackRow: null
    },
    cupSpecState: makeDefaultCupSpecState(),
    preset: "basic",           // Selected preset id, or "custom" for manual configuration
    configureTab: "presets",   // Configure sub-tab: "presets" | "global-options" | "car-options" | "stock-cars-spec" | "dc-cars-spec" | "track-options" | "track-spec" | "cup-spec"
  },
  generate: {
    generatedFilePath: "",
    instanceName: "randomized-instance",
    profileName: "player1",
    generatedHistory: [],
    seedContext: null,
  },
  play: {
    extraArgs: "",
    extraPacks: [],
    runningPid: 0
  }
};

const AppContext = createContext();

export function AppProvider({ children }) {
  const [state, setState] = useState(DEFAULT_STATE);

  // Update a specific nested category (e.g., updateContext('app', { theme: 'light' }))
  const updateCategoryCtx = (category, newData) => {
    setState((prevState) => ({
      ...prevState,
      [category]: {
        ...prevState[category],
        ...newData,
      },
    }));
  };

  const resetContext = () => {
    setState(DEFAULT_STATE);
  };

  const updateContext = (fullNewState) => {
    if (!fullNewState) return;

    setState((prevState) => {
      const mergedState = { ...prevState };
      
      // Loop through the categories in the incoming state
      Object.keys(fullNewState).forEach((category) => {
        // Only merge if the category exists in our default state
        if (mergedState[category]) {
          mergedState[category] = {
            ...mergedState[category],
            ...fullNewState[category]
          };
        }
      });
      
      return mergedState;
    });
  };

  return (
    <AppContext.Provider value={{ state, updateContext, updateCategoryCtx, resetContext }}>
      {children}
    </AppContext.Provider>
  );
}

// Custom hook for easier importing
export const useAppContext = () => useContext(AppContext);