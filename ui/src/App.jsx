import { useEffect } from "react";
import { invoke } from "@tauri-apps/api/core";
import { confirm } from "@tauri-apps/plugin-dialog";

import { useAppContext } from "./AppProvider";

import "./App.css";

// Install components
import ScanSetup from "./components/install/ScanSetup";
import CarsTab from "./components/install/CarsTab";
import TracksTab from "./components/install/TracksTab";

// Car Options components
import CarOptionsTab from "./components/carOptions/CarOptionsTab";

// Car Spec components
import StockCarsFullSpecTab from "./components/carSpec/StockCarsFullSpecTab";
import DcCarsFullSpecTab from "./components/carSpec/DcCarsFullSpecTab";

// Track Options components
import TrackOptionsTab from "./components/trackOptions/TrackOptionsTab";
import TrackSpecTab from "./components/trackOptions/TrackSpecTab";

// Cup Spec components
import CupSpecTab from "./components/cupSpec/CupSpecTab";

// Other tabs
import PacksTab from "./components/PacksTab";
import GenerationTab from "./components/GenerationTab";
import LaunchTab from "./components/LaunchTab";


export default function App() {

  const { state, resetContext, updateContext, updateCategoryCtx } = useAppContext();

  // Destructure categories
  const { app, install } = state;
  
  // Destructure individual variables
  const { isLoading, theme, activeTab, isFetchingPack } = app;
  const { scanResult } = install;

  // const [debugView, setDebugView] = useState("scanResult");

  useEffect(() => {
    document.documentElement.setAttribute("data-theme", theme);
  }, [theme]);

  // LOAD CACHE ON MOUNT
  useEffect(() => {
    const initCache = async () => {
      try {
        const cache = await invoke("load_cache");
        updateContext(cache);
      } catch (error) {
        console.error("Failed to load cache:", error);
      } finally {
        updateCategoryCtx("app", { isLoading: false });
      }
    };
    initCache();
  }, []);

  // SAVE CACHE ON CHANGE
  useEffect(() => {
    if (isLoading) return;
    invoke("save_cache", { data: state }).catch(console.error);
  }, [state, isLoading]);


  async function handleClearData() {
    const confirmed = await confirm("Are you sure you want to clear the app cache?", {
      title: "Clear Data",
      kind: "warning",
    });

    if (confirmed) {
      await invoke("clear_cache");
      resetContext();
      updateCategoryCtx("app", { isLoading: false });
    }
  }

  const setActiveTab = (tab) => {
    updateCategoryCtx("app", { activeTab: tab });
  };

  if (isLoading) {
    return <div className="container"><div className="loading-overlay">Initializing...</div></div>;
  }

  return (
    <div className="container">
      {isFetchingPack && <div className="loading-overlay">Loading Assets...</div>}
      <header>
        <div className="header-top">
          <div className="header-title-area">
            <h1>RVGL Randomizer</h1>
            <ScanSetup />
          </div>
        </div>
        
        {scanResult && (
          <div className="tabs">
            <button className={`tab ${activeTab === 'cars' ? 'active' : ''}`} onClick={() => setActiveTab('cars')}>
              Cars
            </button>
            <button className={`tab ${activeTab === 'tracks' ? 'active' : ''}`} onClick={() => setActiveTab('tracks')}>
              Tracks
            </button>
            <button className={`tab ${activeTab === 'car-options' ? 'active' : ''}`} onClick={() => setActiveTab('car-options')}>
              Car Options
            </button>
            <button className={`tab ${activeTab === 'stock-cars-spec' ? 'active' : ''}`} onClick={() => setActiveTab('stock-cars-spec')}>
              Stock Cars Spec
            </button>
            <button className={`tab ${activeTab === 'dc-cars-spec' ? 'active' : ''}`} onClick={() => setActiveTab('dc-cars-spec')}>
              DC Cars Spec
            </button>
            <button className={`tab ${activeTab === 'track-options' ? 'active' : ''}`} onClick={() => setActiveTab('track-options')}>
              Track Options
            </button>
            <button className={`tab ${activeTab === 'track-spec' ? 'active' : ''}`} onClick={() => setActiveTab('track-spec')}>
              Track Spec
            </button>
            <button className={`tab ${activeTab === 'cup-spec' ? 'active' : ''}`} onClick={() => setActiveTab('cup-spec')}>
              Cup Spec
            </button>            
            {scanResult.installType === 'launcher' && (
              <button className={`tab ${activeTab === 'packs' ? 'active' : ''}`} onClick={() => setActiveTab('packs')}>
                Launch Packs
              </button>
            )}
            <button className={`tab ${activeTab === 'generation' ? 'active' : ''}`} onClick={() => setActiveTab('generation')}>
              Randomize
            </button>
            <button className={`tab ${activeTab === 'launch' ? 'active' : ''}`} onClick={() => setActiveTab('launch')}>
              Launch
            </button>
          </div>
        )}
      </header>

      {scanResult && (
        <div className="dashboard">
          {/* ORIGINAL BROWSING TABS */}
          { activeTab === 'cars' && <CarsTab />}
          { activeTab === 'tracks' && <TracksTab />}

          {activeTab === 'car-options' && (
            <div style={{ flex: 1, overflowY: "auto" }}>
              <CarOptionsTab />
            </div>
          )}

          {activeTab === 'stock-cars-spec' && (
            <div style={{ flex: 1, overflowY: "auto", padding: "1rem" }}>
              <StockCarsFullSpecTab />
            </div>
          )}

          {activeTab === 'dc-cars-spec' && (
            <div style={{ flex: 1, overflowY: "auto", padding: "1rem" }}>
              <DcCarsFullSpecTab />
            </div>
          )}

          {activeTab === 'track-options' && (
            <div style={{ flex: 1, overflowY: "auto" }}>
              <TrackOptionsTab />
            </div>
          )}

          {activeTab === 'track-spec' && (
            <div style={{ flex: 1, overflowY: "auto", padding: "1rem" }}>
              <TrackSpecTab />
            </div>
          )}

          {activeTab === 'cup-spec' && (
            <div style={{ flex: 1, overflowY: "auto", padding: "1rem" }}>
              <CupSpecTab />
            </div>
          )}

          {/* NEW ROUTING TABS */}
          {activeTab === 'packs' && scanResult.installType === 'launcher' && (
            <div style={{ flex: 1, overflowY: "auto", padding: "1rem" }}>
              <PacksTab />
            </div>
          )}

          {activeTab === 'generation' && (
             <div style={{ flex: 1, overflowY: "auto", padding: "1rem" }}>
               <GenerationTab />
             </div>
          )}

          {activeTab === 'launch' && (
             <div style={{ flex: 1, overflowY: "auto", padding: "1rem" }}>
               <LaunchTab />
             </div>
          )}

        </div>
      )}

      <footer>
        <button onClick={handleClearData} style={{ padding: "0.25rem 0.75rem", fontSize: "0.75rem", backgroundColor: "#7a2626", color: "white" }}>
          Clear Cache
        </button>
        <div className="theme-selector">
          <label style={{ fontSize: "0.85rem", fontWeight: "bold" }}>Theme</label>
          <select value={theme} onChange={e => updateCategoryCtx("app", { theme: e.target.value })}>
            <option value="dark">Dark</option>
            <option value="light">Light</option>
            <option value="earthy">Earthy</option>
          </select>
        </div>
      </footer>
    </div>
  );
}