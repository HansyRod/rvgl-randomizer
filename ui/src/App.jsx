import { useState, useEffect } from "react";
import { invoke } from "@tauri-apps/api/core";
import { open } from "@tauri-apps/plugin-dialog";
import Sidebar from "./components/Sidebar";
import MainContent from "./components/MainContent";
import "./App.css";

export default function App() {
  const [installPath, setInstallPath] = useState("");
  const [scanResult, setScanResult] = useState(null);
  const [activeTab, setActiveTab] = useState("cars");
  const [theme, setTheme] = useState("dark");
  const [isScanning, setIsScanning] = useState(false);
  const [isFetchingPack, setIsFetchingPack] = useState(false);

  useEffect(() => {
    document.documentElement.setAttribute("data-theme", theme);
  }, [theme]);

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

  async function togglePack(packIndex) {
    if (!scanResult) return;
    
    // Copy state properly
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
                  [{scanResult.installType === "launcher" ? "RVGL Launcher Install" : "Classic Install"}]
                </span>
              )}
              <p style={{ margin: 0 }}>{installPath || "No installation selected. Browse to rvgl.exe to begin."}</p>
            </div>
          </div>
          <div className="header-actions">
            <button className="primary" onClick={handleScanSetup}>
              {isScanning ? "Scanning..." : "Browse RVGL"}
            </button>
          </div>
        </div>
        {scanResult && (
          <div className="tabs">
            <button 
              className={`tab ${activeTab === 'cars' ? 'active' : ''}`}
              onClick={() => setActiveTab('cars')}
            >
              Cars
            </button>
            <button 
              className={`tab ${activeTab === 'tracks' ? 'active' : ''}`}
              onClick={() => setActiveTab('tracks')}
            >
              Tracks
            </button>
            <button 
              className={`tab ${activeTab === 'debug' ? 'active' : ''}`}
              onClick={() => setActiveTab('debug')}
            >
              Debug
            </button>
          </div>
        )}
      </header>

      {scanResult && (
        <div className="dashboard">
          {activeTab === 'debug' ? (
            <div style={{ flex: 1, padding: "1.5rem", display: "flex", flexDirection: "column", boxSizing: "border-box" }}>
              <textarea 
                readOnly 
                value={JSON.stringify(scanResult, null, 2)}
                style={{ 
                  flex: 1, 
                  width: "100%", 
                  borderRadius: "8px", 
                  fontFamily: "monospace", 
                  padding: "1rem", 
                  backgroundColor: "var(--bg-secondary)", 
                  color: "var(--text-primary)", 
                  border: "1px solid var(--border-color)",
                  outline: "none",
                  resize: "none",
                  boxSizing: "border-box"
                }}
              />
            </div>
          ) : (
            <>
              {scanResult.installType === "launcher" && (
                 <Sidebar 
                   packs={scanResult.contentPacks} 
                   activeTab={activeTab} 
                   onTogglePack={togglePack} 
                 />
              )}
              <MainContent 
                 scanResult={scanResult} 
                 activeTab={activeTab} 
                 installPath={installPath}
              />
            </>
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
