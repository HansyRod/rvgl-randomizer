import { useState, useEffect } from "react";
import { invoke } from "@tauri-apps/api/core";
import { open, confirm } from "@tauri-apps/plugin-dialog";
import Sidebar from "./components/Sidebar";
import MainContent from "./components/MainContent";
import StockCarsFullSpecTab from "./components/StockCarsFullSpecTab";
import DcCarsFullSpecTab from "./components/DcCarsFullSpecTab";

// NEW TAB IMPORTS
import PacksTab from "./components/PacksTab";
import GenerationTab from "./components/GenerationTab";
import LaunchTab from "./components/LaunchTab";

import "./App.css";

export default function App() {
  const [installPath, setInstallPath] = useState("");
  const [scanResult, setScanResult] = useState(null);
  const [activeTab, setActiveTab] = useState("cars");
  const [carsSpecState, setCarsSpecState] = useState(null);
  
  // New State variables for Phase 2/3
  const [generatedFilePath, setGeneratedFilePath] = useState("");
  const [instanceName, setInstanceName] = useState("randomized-instance"); // NEW: Lifted state with new default
  const [extraArgs, setExtraArgs] = useState("");
  const [extraPacks, setExtraPacks] = useState([]);
  const [isAppLoading, setIsAppLoading] = useState(true);

  const [debugView, setDebugView] = useState("scanResult");
  const [theme, setTheme] = useState("dark");
  const [isScanning, setIsScanning] = useState(false);
  const [isFetchingPack, setIsFetchingPack] = useState(false);

  useEffect(() => {
    document.documentElement.setAttribute("data-theme", theme);
  }, [theme]);

  // LOAD CACHE ON MOUNT
  useEffect(() => {
    const initCache = async () => {
      try {
        const cache = await invoke("load_cache");
        if (cache.installPath) setInstallPath(cache.installPath);
        if (cache.scanResult) setScanResult(cache.scanResult);
        if (cache.extraArgs) setExtraArgs(cache.extraArgs);
      } catch (error) {
        console.error("Failed to load cache:", error);
      } finally {
        setIsAppLoading(false);
      }
    };
    initCache();
  }, []);

  // SAVE CACHE ON CHANGE
  useEffect(() => {
    if (isAppLoading) return;
    invoke("save_cache", {
      data: { installPath, scanResult, extraArgs }
    }).catch(console.error);
  }, [installPath, scanResult, extraArgs, isAppLoading]);


  async function handleScanSetup() {
    const selected = await open({
      multiple: false,
      directory: false,
      filters: [{ name: 'Executable', extensions: ['exe', ''] }]
    });

    if (selected) {
      const path = selected.path || selected;
      setInstallPath(path);
      setIsScanning(true);
      try {
        const result = await invoke("scan_install", { executablePath: path });
        setScanResult(result);
      } catch (err) {
        console.error("Error scanning:", err);
      } finally {
        setIsScanning(false);
      }
    }
  }

  async function handleClearData() {
    const confirmed = await confirm("Are you sure you want to clear the app cache?", {
      title: "Clear Data",
      kind: "warning",
    });

    if (confirmed) {
      await invoke("clear_cache");
      setInstallPath("");
      setScanResult(null);
      setGeneratedFilePath("");
      setExtraArgs("");
      setExtraPacks([]);
      setActiveTab("cars");
    }
  }

  async function togglePack(packIndex) {
    if (!scanResult) return;
    const newResult = JSON.parse(JSON.stringify(scanResult));
    const pack = newResult.contentPacks[packIndex];
    
    if (activeTab === "cars") {
      pack.useCars = !pack.useCars;
      if (pack.useCars && pack.cars.length === 0 && pack.hasCars) {
        setIsFetchingPack(true);
        try {
          pack.cars = await invoke("scan_cars_folder", { folderPath: `${pack.absolutePath}\\cars` });
        } catch(e) { console.error(e); }
        finally { setIsFetchingPack(false); }
      }
    } else {
      pack.useTracks = !pack.useTracks;
      if (pack.useTracks && pack.tracks.length === 0 && pack.hasTracks) {
        setIsFetchingPack(true);
        try {
          pack.tracks = await invoke("scan_levels_folder", { folderPath: `${pack.absolutePath}\\levels` });
        } catch(e) { console.error(e); }
        finally { setIsFetchingPack(false); }
      }
    }
    setScanResult(newResult);
  }

  if (isAppLoading) {
    return <div className="container"><div className="loading-overlay">Initializing...</div></div>;
  }

  return (
    <div className="container">
      {isFetchingPack && <div className="loading-overlay">Loading Assets...</div>}
      <header>
        <div className="header-top">
          <div className="header-title-area">
            <h1>RVGL Randomizer</h1>
            <div style={{ display: "flex", alignItems: "center", gap: "0.5rem", marginTop: "0.25rem" }}>
              {scanResult && (
                <span className="badge" style={{ padding: "0.3rem 0.5rem", fontSize: "0.75rem", backgroundColor: "var(--accent)" }}>
                  [{scanResult.installType === "launcher" ? "Launcher Install" : "Classic Install"}]
                </span>
              )}
              <p style={{ margin: 0 }}>{installPath || "No installation selected. Browse to rvgl.exe to begin."}</p>
              
              <button className="primary" style={{ padding: "0.25rem 0.75rem", fontSize: "0.75rem" }} onClick={handleScanSetup}>
                {isScanning ? "Scanning..." : "Browse RVGL"}
              </button>

              {installPath && (
                <>
                  <button onClick={() => invoke("scan_install", { executablePath: installPath }).then(setScanResult)} style={{ padding: "0.25rem 0.75rem", fontSize: "0.75rem" }}>
                    ↺ Refresh
                  </button>
                  <button onClick={handleClearData} style={{ padding: "0.25rem 0.75rem", fontSize: "0.75rem", backgroundColor: "#7a2626", color: "white" }}>
                    Clear Cache
                  </button>
                </>
              )}
            </div>
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
            <button className={`tab ${activeTab === 'stock-cars-spec' ? 'active' : ''}`} onClick={() => setActiveTab('stock-cars-spec')}>
              Stock Cars Spec
            </button>
            <button className={`tab ${activeTab === 'dc-cars-spec' ? 'active' : ''}`} onClick={() => setActiveTab('dc-cars-spec')}>
              DC Cars Spec
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
          {(activeTab === 'cars' || activeTab === 'tracks') && (
            <>
              {scanResult.installType === "launcher" && (
                 <Sidebar 
                   packs={scanResult.contentPacks} 
                   activeTab={activeTab} 
                   onTogglePack={togglePack} 
                   setScanResult={setScanResult}
                   scanResult={scanResult}
                 />
              )}
              <MainContent 
                 scanResult={scanResult} 
                 activeTab={activeTab} 
                 installPath={installPath}
              />
            </>
          )}

          {activeTab === 'stock-cars-spec' && (
            <div style={{ flex: 1, overflowY: "auto", padding: "1rem" }}>
              <StockCarsFullSpecTab
                scanResult={scanResult}
                specState={carsSpecState}
                setSpecState={setCarsSpecState}
              />
            </div>
          )}

          {activeTab === 'dc-cars-spec' && (
            <div style={{ flex: 1, overflowY: "auto", padding: "1rem" }}>
              <DcCarsFullSpecTab
                scanResult={scanResult}
                specState={carsSpecState}
                setSpecState={setCarsSpecState}
              />
            </div>
          )}

          {/* NEW ROUTING TABS */}
          {activeTab === 'packs' && scanResult.installType === 'launcher' && (
            <div style={{ flex: 1, overflowY: "auto", padding: "1rem" }}>
              <PacksTab 
                scanResult={scanResult} 
                extraPacks={extraPacks} 
                setExtraPacks={setExtraPacks} 
              />
            </div>
          )}

          {activeTab === 'generation' && (
             <div style={{ flex: 1, overflowY: "auto", padding: "1rem" }}>
               <GenerationTab 
                 scanResult={scanResult}
                 specState={carsSpecState}
                 generatedFilePath={generatedFilePath}
                 setGeneratedFilePath={setGeneratedFilePath}
                 instanceName={instanceName}
                 setInstanceName={setInstanceName}
               />
             </div>
          )}

          {activeTab === 'launch' && (
             <div style={{ flex: 1, overflowY: "auto", padding: "1rem" }}>
               <LaunchTab 
                 installPath={installPath}
                 scanResult={scanResult}
                 generatedFilePath={generatedFilePath}
                 extraArgs={extraArgs}
                 setExtraArgs={setExtraArgs}
                 extraPacks={extraPacks}
               />
             </div>
          )}

        </div>
      )}

      <footer>
        <div className="theme-selector">
          <label style={{ fontSize: "0.85rem", fontWeight: "bold" }}>Theme</label>
          <select value={theme} onChange={e => setTheme(e.target.value)}>
            <option value="dark">Dark</option>
            <option value="light">Light</option>
            <option value="earthy">Earthy</option>
          </select>
        </div>
      </footer>
    </div>
  );
}