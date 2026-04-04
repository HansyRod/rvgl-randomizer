import { useState, useEffect } from "react";
import { invoke } from "@tauri-apps/api/core";
import { open } from "@tauri-apps/plugin-dialog";
import Sidebar from "./components/Sidebar";
import MainContent from "./components/MainContent";
import CarsFullSpecTab from "./components/CarsFullSpecTab";
import "./App.css";

export default function App() {
  const [installPath, setInstallPath] = useState("");
  const [scanResult, setScanResult] = useState(null);
  const [activeTab, setActiveTab] = useState("cars");
  const [carsSpecState, setCarsSpecState] = useState(null);
  const [debugView, setDebugView] = useState("scanResult");
  const [theme, setTheme] = useState("dark");
  const [isScanning, setIsScanning] = useState(false);
  const [isFetchingPack, setIsFetchingPack] = useState(false);
  const [isGenerating, setIsGenerating] = useState(false);
  const [generateStatus, setGenerateStatus] = useState(null); // { ok: bool, msg: string }

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

  async function handleGenerate() {
    if (!scanResult || !carsSpecState) return;

    // Derive the output path: same directory as rvgl.exe, file named result.json
    const exeDir = installPath.replace(/[\\/][^\\/]+$/, "");
    const outputPath = exeDir + "\\result.json";

    setIsGenerating(true);
    setGenerateStatus(null);
    try {
      const msg = await invoke("generate_result", {
        scanResult,
        specState: carsSpecState,
        outputPath,
      });
      setGenerateStatus({ ok: true, msg });
    } catch (err) {
      setGenerateStatus({ ok: false, msg: String(err) });
    } finally {
      setIsGenerating(false);
    }
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
              <button className="primary" style={{ padding: "0.25rem 0.75rem", fontSize: "0.75rem" }} onClick={handleScanSetup}>
                {isScanning ? "Scanning..." : "Browse RVGL"}
              </button>
            </div>
          </div>
          <div className="header-actions">
            {scanResult && carsSpecState && (
              <div style={{ display: "flex", flexDirection: "column", alignItems: "flex-end", gap: "0.3rem" }}>
                <button
                  className="primary"
                  onClick={handleGenerate}
                  disabled={isGenerating}
                  style={{ padding: "0.35rem 1rem", fontSize: "0.85rem" }}
                >
                  {isGenerating ? "Generating…" : "⚡ Generate"}
                </button>
                {generateStatus && (
                  <span style={{
                    fontSize: "0.75rem",
                    fontFamily: "monospace",
                    color: generateStatus.ok ? "var(--text-primary)" : "var(--error-color, #c0392b)"
                  }}>
                    {generateStatus.ok ? "✓" : "✗"} {generateStatus.msg}
                  </span>
                )}
              </div>
            )}
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
              className={`tab ${activeTab === 'cars-full-spec' ? 'active' : ''}`}
              onClick={() => setActiveTab('cars-full-spec')}
            >
              Cars Full Spec
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
            <div style={{ flex: 1, padding: "1.5rem", display: "flex", flexDirection: "column", boxSizing: "border-box", gap: "1rem" }}>
              <div style={{ display: "flex", alignItems: "center", gap: "1rem" }}>
                <label style={{ fontWeight: "bold" }}>View State:</label>
                <select value={debugView} onChange={(e) => setDebugView(e.target.value)} style={{ width: "auto", minWidth: "200px" }}>
                  <option value="scanResult">scanResult</option>
                  <option value="carsSpecState">carsSpecState</option>
                </select>
              </div>
              <textarea 
                readOnly 
                value={JSON.stringify(debugView === "scanResult" ? scanResult : carsSpecState, null, 2)}
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
          ) : activeTab === 'cars-full-spec' ? (
             <div style={{ flex: 1, overflowY: "auto" }}>
               <CarsFullSpecTab
                 scanResult={scanResult}
                 specState={carsSpecState}
                 setSpecState={setCarsSpecState}
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
