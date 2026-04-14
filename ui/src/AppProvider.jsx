import { createContext, useContext, useState } from "react";
import { DEFAULT_CAR_OPTIONS, DEFAULT_TRACK_OPTIONS, STOCK_CARS, DC_CARS, STOCK_TRACKS, makeDefaultCarsSpec, makeDefaultTrackSpec } from "./utils/constants";
import { makeDefaultCupSpecState } from "./components/cupSpec/CupSpecTab";
const DEFAULT_STATE = {
  app: {
    isLoading: true,
    theme: "dark",
    activeTab: "setup", // Wizard step: "setup" | "configure" | "generate" | "play"
    isScanning: false,
    isFetchingPack: false,
  },
  install: {
    installPath: "",
    scanResult: null,
    setupTab: "Install", // Setup sub-tab: "Install" | "Cars" | "Tracks"
  },
  randomizer: {
    carOptions: DEFAULT_CAR_OPTIONS,
    trackOptions: DEFAULT_TRACK_OPTIONS,
    carsSpecState: {
      includeStockCars: true,
      includeDcCars: true,
      stockCars: makeDefaultCarsSpec(STOCK_CARS),
      dcCars: makeDefaultCarsSpec(DC_CARS)
    },
    trackSpecState: {
      includeTracks: true,
      tracks: makeDefaultTrackSpec(STOCK_TRACKS)
    },
    cupSpecState: makeDefaultCupSpecState(),
    configureTab: "car-options", // Configure sub-tab: "car-options" | "stock-cars-spec" | "dc-cars-spec" | "track-options" | "track-spec" | "cup-spec"
  },
  output: {
    generatedFilePath: "",
    instanceName: "randomized-instance",
    profileName: "player1",
  },
  launch: {
    extraArgs: "",
    extraPacks: [],
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